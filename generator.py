"""CADAC ROCKET6G data-generation backend for gray-box state-space identification.

This module owns everything between "a clean CADAC checkout" and "an ``.npz`` a
PyTorch ``Dataset`` can read". It is deliberately isolated from the training code:
nothing here imports torch, and nothing here knows what a model looks like.

Pipeline
--------
1. Fetch a clean ROCKET6G checkout (``fetch_source``).
2. Patch the C++ so the true 6-DOF state reaches the plot file at usable
   precision (``patch_source``) -- see PATCHES below for why each is required.
3. Compile with a Linux/Colab-compatible g++ invocation (``build``).
4. Emit an ``input.asc`` with a ``MONTE`` block and per-parameter dispersions
   (``write_input``).
5. Run once; CADAC loops the Monte Carlo internally (``run``).
6. Parse ``plot1.asc``, split runs, finite-difference, mask staging transients,
   and write ``.npz`` (``parse_plot_file`` / ``build_dataset``).

Stock CADAC needs three source patches before its output is usable as training
data. All three are verified against the upstream ROCKET6G sources:

``PLOT_FLAGS``
    The stock plot file carries position/velocity only in polar form
    (``lonx/latx/alt`` + ``dvbe/psivdx/thtvdx``). The Cartesian state lives in the
    module-variable array but is not flagged for plot output. Each ``init()``
    call's final argument is the output field; adding ``plot`` exposes it. ``ABII``
    is included because it is CADAC's own computed acceleration and therefore a
    free ground-truth check on our finite differences.

``precision``
    ``Hyper::plot_data`` never sets stream precision, so values are written at the
    ``ostream`` default of 6 significant digits. ``SBII`` is ~6.37e6 m, which
    quantises to +/-5 m; differenced over a 0.1 s plot step that is +/-100 m/s of
    pure round-off (measured: 54.9 m/s median error on ``dSBII/dt`` vs ``VBII``,
    3.1% relative). Raising precision drops that to ~5e-4 m/s.

``width``
    Consequence of the above: at 14 digits the numbers overflow the 16-character
    column width and run together with no separator, making the file unparseable.
    The two patches are a pair; neither works alone.
"""

from __future__ import annotations

import argparse
import os
import re
import shutil
import subprocess
import sys
import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import Iterator, Sequence

import numpy as np

CADAC_REPO = "https://github.com/missiondesignsolutions/CADAC.git"

#: ``(file, variable, output-field-after-patch)``. The final ``init()`` argument is
#: the output field; ``plot`` routes the variable into ``plot1.asc``.
PLOT_FLAGS: tuple[tuple[str, str, str], ...] = (
    ("newton.cpp", "SBII", "com,plot"),   # inertial position      - m
    ("newton.cpp", "VBII", "com,plot"),   # inertial velocity      - m/s
    ("newton.cpp", "ABII", "plot"),       # inertial acceleration  - m/s^2
    ("newton.cpp", "FSPB", "scrn,plot"),  # specific force, body   - m/s^2
    ("euler.cpp", "WBIB", "plot"),        # body angular rate      - rad/s
    ("forces.cpp", "FAPB", "plot"),       # aero+propulsive force  - N
)

#: Do not add 3x3 matrices (``TBI``) here. ``Hyper::plot_data`` calls ``.vec()`` on
#: any uppercase-named variable, and ``Variable`` stores ``VEC`` and ``MAT``
#: separately -- a 3x3 would emit three silent zeros. Attitude is already fully
#: covered by the ``phibdx/thtbdx/psibdx`` Euler angles, which are plotted by stock.
UNPLOTTABLE = frozenset({"TBI", "TBID", "TBIC", "IBBB"})

#: MSVC-era source: needs ``-fpermissive`` for ~30 ``extra qualification 'Matrix::'``
#: errors, and the forced includes because ``strcpy``/``system`` are used unguarded.
COMPILE_CMD: tuple[str, ...] = (
    "g++", "-w", "-fpermissive", "-std=c++03", "-O2",
    "-include", "cstring", "-include", "cstdlib", "-include", "cstdio",
    "-o", "rocket6g",
)

#: Sentinel written into the ``time`` column on the final row of every Monte Carlo
#: run (``Hyper::plot_data``, the ``merge && index==0`` branch). Runs are appended
#: to a single ``plot1.asc`` under one banner, so this is the only run delimiter.
RUN_DELIMITER_TIME = -1.0

#: Trajectory Dynamics only -- the scoped first subsystem. Attitude is carried in
#: the parameter vector rather than the state: the rotational subsystem is out of
#: scope, and the Euler-rate kinematics are singular at theta = 90 deg, which is
#: exactly the launch attitude (``thtbdx`` starts at 90). Adding the rotational
#: states (WBIB) later is purely additive.
DEFAULT_STATE: tuple[str, ...] = (
    "SBII1", "SBII2", "SBII3",  # inertial position - m
    "VBII1", "VBII2", "VBII3",  # inertial velocity - m/s
)

DEFAULT_PARAMS: tuple[str, ...] = (
    "vmass",    # vehicle mass            - kg
    "thrust",   # thrust                  - N
    "xcg",      # cg from nose            - m
    "fmassr",   # remaining fuel mass     - kg
    "vmach",    # Mach number             - ND
    "pdynmc",   # dynamic pressure        - Pa
    "alt",      # altitude                - m
    "alphax",   # angle of attack         - deg
    "betax",    # sideslip angle          - deg
    # Attitude and geodetic position: needed to rotate body-frame thrust into the
    # inertial frame (TBI = TBD(euler) * TDI(lon, lat, time)). physics.py reads all
    # five out of p in body_to_inertial(); omitting them raises KeyError there.
    "phibdx", "thtbdx", "psibdx",  # Euler angles, body wrt geodetic - deg
    "lonx", "latx",                # geodetic longitude / latitude   - deg
)

#: Truth columns for validating finite differences, as ``state -> CADAC's own
#: derivative``. ``d(VBII)/dt`` should reproduce ``ABII`` to round-off.
TRUTH_DERIVATIVES: dict[str, str] = {
    "VBII1": "ABII1", "VBII2": "ABII2", "VBII3": "ABII3",
}

#: ``name -> (mean, sigma)`` for a Gaussian dispersion on an ``input.asc`` scalar.
#: Defaults disperse launch attitude, gross mass and specific impulse -- enough to
#: give trajectory diversity without leaving the aero deck's valid envelope.
DEFAULT_DISPERSIONS: dict[str, tuple[float, float]] = {
    "thtbdx": (90.0, 2.0),       # launch pitch          - deg
    "psibdx": (-83.0, 2.0),      # launch yaw            - deg
    "vmass0": (48984.0, 500.0),  # stage-1 gross mass    - kg
    "spi": (279.2, 3.0),         # stage-1 specific impulse - s
}


@dataclass
class GeneratorConfig:
    """Everything the pipeline needs. Serialised into the ``.npz`` for provenance."""

    work_dir: Path = Path("cadac_work")
    out_path: Path = Path("data/rocket6g.npz")
    source: str = "input_insertion.asc"

    #: Local CADAC checkout to copy ROCKET6G from. ``None`` clones upstream, which
    #: stays the default -- see :func:`fetch_source` for why a local copy is opt-in.
    cadac_source: Path | None = None

    n_runs: int = 200
    seed: int = 1234

    plot_step: float = 0.01
    int_step: float = 0.001
    endtime: float = 190.0

    dispersions: dict[str, tuple[float, float]] = field(
        default_factory=lambda: dict(DEFAULT_DISPERSIONS)
    )
    state_vars: Sequence[str] = DEFAULT_STATE
    param_vars: Sequence[str] = DEFAULT_PARAMS

    #: Samples to drop either side of a detected thrust discontinuity. Stage
    #: burnout/ignition steps thrust, so a central difference straddling one is
    #: meaningless (measured spikes to 14 m/s^2 against a ~41 m/s^2 typical
    #: acceleration).
    stage_mask_halfwidth: int = 2
    #: Fraction of peak thrust that counts as a discontinuity between samples.
    stage_thrust_jump_frac: float = 0.05

    @property
    def src_dir(self) -> Path:
        return self.work_dir / "ROCKET6G"

    @property
    def exe(self) -> Path:
        return self.src_dir / "rocket6g"


# --------------------------------------------------------------------------- #
# 1-3. source, patch, build
# --------------------------------------------------------------------------- #

def _find_rocket6g(root: Path) -> Path:
    """Locate the ROCKET6G source directory inside a CADAC checkout.

    Upstream keeps the examples at the top level; the pyCAS packaging keeps them
    under ``example/``. Both are accepted so a local checkout works either way.
    """
    for candidate in (root, root / "ROCKET6G", root / "example" / "ROCKET6G"):
        if (candidate / "newton.cpp").is_file():
            return candidate
    raise RuntimeError(
        f"no ROCKET6G under {root} (looked for newton.cpp in ./, ROCKET6G/, "
        "example/ROCKET6G/)"
    )


def fetch_source(cfg: GeneratorConfig, force: bool = False) -> Path:
    """Populate ``cfg.src_dir`` with ROCKET6G. Idempotent unless ``force``.

    Uses ``cfg.cadac_source`` when set -- a local CADAC checkout, no network. That
    is not the default: every measured number in this framework came from the
    upstream repo, and a local checkout may be an edited copy (pyCAS ships a
    cleaned ROCKET6G whose MSVC declarations are already fixed). Diff-check a
    local source against upstream before relying on it.
    """
    if cfg.src_dir.exists() and not force:
        print(f"[fetch] reusing {cfg.src_dir}")
        return cfg.src_dir
    if cfg.src_dir.exists():
        shutil.rmtree(cfg.src_dir)

    cfg.work_dir.mkdir(parents=True, exist_ok=True)
    if cfg.cadac_source is not None:
        source = _find_rocket6g(Path(cfg.cadac_source).expanduser())
        print(f"[fetch] local source {source}")
    else:
        clone = cfg.work_dir / "_cadac_clone"
        if not clone.exists():
            print(f"[fetch] cloning {CADAC_REPO}")
            subprocess.run(
                ["git", "clone", "--depth", "1", CADAC_REPO, str(clone)],
                check=True, capture_output=True, text=True,
            )
        source = _find_rocket6g(clone)

    shutil.copytree(source, cfg.src_dir)
    print(f"[fetch] ROCKET6G -> {cfg.src_dir}")
    return cfg.src_dir


def _set_plot_flag(text: str, var: str, out_field: str) -> tuple[str, bool]:
    """Rewrite the trailing output-field argument of ``init("<var>", ..., "...")``."""
    if var in UNPLOTTABLE:
        raise ValueError(
            f"{var} is a 3x3 matrix; Hyper::plot_data would emit three zeros for it. "
            "Use the Euler angles (phibdx/thtbdx/psibdx) instead."
        )
    pattern = re.compile(rf'(\.init\("{re.escape(var)}"[^;\n]*?,\s*")([^"]*)("\s*\);)')
    new_text, n = pattern.subn(rf'\g<1>{out_field}\g<3>', text)
    if n > 1:
        raise RuntimeError(f"{var}: expected one init() call, found {n}")
    return new_text, bool(n)


def patch_source(cfg: GeneratorConfig) -> None:
    """Apply the three patches described in the module docstring. Idempotent."""
    for filename, var, out_field in PLOT_FLAGS:
        path = cfg.src_dir / filename
        text = path.read_text()
        patched, found = _set_plot_flag(text, var, out_field)
        if not found:
            raise RuntimeError(
                f"no init(\"{var}\") in {filename} -- upstream CADAC layout changed"
            )
        path.write_text(patched)
    print(f"[patch] plot flags set for {', '.join(v for _, v, _ in PLOT_FLAGS)}")

    path = cfg.src_dir / "hyper_functions.cpp"
    text = path.read_text()
    start = text.index("void Hyper::plot_data")
    end = text.index("\n}", start)
    body = text[start:end]

    if "fplot.precision(" not in body:
        body = body.replace(
            "fplot.setf(ios::left);",
            "fplot.setf(ios::left);\n\tfplot.precision(14);",
            1,
        )
        print("[patch] plot_data precision 6 -> 14 significant digits")

    n_width = body.count("fplot.width(16)")
    if n_width:
        body = body.replace("fplot.width(16)", "fplot.width(26)")
        print(f"[patch] plot_data column width 16 -> 26 ({n_width} sites)")

    path.write_text(text[:start] + body + text[end:])


def build(cfg: GeneratorConfig) -> Path:
    """Compile ROCKET6G. Returns the executable path.

    Prefers a bundled ``Makefile`` when one exists -- some CADAC packagings ship
    one, and it already carries the right flags for that copy. Upstream has no
    Makefile and needs ``COMPILE_CMD``'s workarounds, so that is the fallback.
    """
    sources = sorted(p.name for p in cfg.src_dir.glob("*.cpp"))
    if not sources:
        raise RuntimeError(f"no .cpp sources in {cfg.src_dir}")

    if (cfg.src_dir / "Makefile").is_file():
        print("[build] make (bundled Makefile) ...")
        result = subprocess.run(
            ["make", "-j", str(os.cpu_count() or 1)], cwd=cfg.src_dir,
            capture_output=True, text=True,
        )
        if result.returncode == 0 and cfg.exe.exists():
            print(f"[build] -> {cfg.exe}")
            return cfg.exe
        print("[build] make failed, falling back to explicit g++")

    print(f"[build] g++ on {len(sources)} sources ...")
    result = subprocess.run(
        [*COMPILE_CMD, *sources], cwd=cfg.src_dir,
        capture_output=True, text=True,
    )
    if result.returncode != 0:
        raise RuntimeError(f"compilation failed:\n{result.stderr[:4000]}")
    print(f"[build] -> {cfg.exe}")
    return cfg.exe


# --------------------------------------------------------------------------- #
# 4-5. input generation and execution
# --------------------------------------------------------------------------- #

def _disperse_line(line: str, dispersions: dict[str, tuple[float, float]]) -> str:
    """Wrap a scalar assignment in a ``GAUSS`` dispersion if it is being randomised.

    CADAC input lines are ``<indent><name>  <value>  //comment``. The stochastic
    form is ``<indent>GAUSS <name>  <mean>  <sigma>  //comment``.
    """
    match = re.match(r"^(\s*)([A-Za-z_]\w*)(\s+)(\S+)(.*)$", line)
    if not match:
        return line
    indent, name, _, _, tail = match.groups()
    if name not in dispersions:
        return line
    mean, sigma = dispersions[name]
    return f"{indent}GAUSS {name}  {mean:g}  {sigma:g}{tail}"


def write_input(cfg: GeneratorConfig) -> Path:
    """Derive ``input.asc`` from the stock scenario: MONTE block, timing, dispersions."""
    template = (cfg.src_dir / cfg.source).read_text().split("\n")
    seen: set[str] = set()
    out: list[str] = []

    for line in template:
        stripped = line.strip()

        if stripped.startswith("MONTE"):
            out.append(f"MONTE {cfg.n_runs} {cfg.seed}")
            continue
        if stripped.startswith("ENDTIME"):
            out.append(f"ENDTIME {cfg.endtime:g}")
            continue
        for key, value in (("plot_step", cfg.plot_step), ("int_step", cfg.int_step)):
            if stripped.startswith(key):
                out.append(f"\t{key} {value:g}")
                break
        else:
            dispersed = _disperse_line(line, cfg.dispersions)
            if dispersed != line:
                seen.add(stripped.split()[0])
            out.append(dispersed)

    missing = set(cfg.dispersions) - seen
    if missing:
        # A silent miss means that parameter is simply never randomised, and the
        # dataset would be narrower than requested with no visible symptom.
        raise RuntimeError(
            f"dispersion targets not found in {cfg.source}: {sorted(missing)}"
        )

    path = cfg.src_dir / "input.asc"
    path.write_text("\n".join(out))
    print(
        f"[input] MONTE {cfg.n_runs} seed={cfg.seed} plot_step={cfg.plot_step:g} "
        f"dispersing {sorted(seen)}"
    )
    return path


def run(cfg: GeneratorConfig, timeout: float = 7200.0) -> Path:
    """Execute the simulation. CADAC loops the Monte Carlo internally."""
    plot_file = cfg.src_dir / "plot1.asc"
    plot_file.unlink(missing_ok=True)

    print(f"[run] {cfg.n_runs} Monte Carlo run(s) ...")
    result = subprocess.run(
        [f"./{cfg.exe.name}"], cwd=cfg.src_dir, stdin=subprocess.DEVNULL,
        capture_output=True, text=True, timeout=timeout,
    )
    if not plot_file.exists() or plot_file.stat().st_size == 0:
        raise RuntimeError(f"no trajectory output.\n{result.stdout[-2000:]}")
    print(f"[run] plot1.asc {plot_file.stat().st_size / 1e6:.1f} MB")
    return plot_file


# --------------------------------------------------------------------------- #
# 6. parsing and dataset assembly
# --------------------------------------------------------------------------- #

def parse_plot_file(path: Path) -> tuple[list[str], np.ndarray]:
    """Parse ``plot1.asc`` into column names and a ``(n_rows, n_cols)`` array.

    Layout: a banner line, then ``0  0  <n_columns>``, then ``n_columns`` whitespace-
    delimited names wrapped five per line, then the data in the same wrapping. All
    Monte Carlo runs share one banner and are appended, so this returns every run
    concatenated; use :func:`split_runs` to separate them.
    """
    lines = path.read_text().split("\n")
    n_cols = int(lines[1].split()[-1])

    names: list[str] = []
    cursor = 2
    while len(names) < n_cols:
        names.extend(lines[cursor].split())
        cursor += 1
    if len(names) != n_cols:
        raise ValueError(f"header declares {n_cols} columns, parsed {len(names)}")

    flat = np.fromstring(" ".join(lines[cursor:]), sep=" ")
    n_rows = flat.size // n_cols
    if n_rows == 0:
        raise ValueError(f"{path} contains a header but no data rows")
    return names, flat[: n_rows * n_cols].reshape(n_rows, n_cols)


def split_runs(data: np.ndarray, time_col: int) -> Iterator[np.ndarray]:
    """Yield each Monte Carlo run, dropping the ``time == -1`` delimiter rows."""
    time = data[:, time_col]
    start = 0
    for end in np.flatnonzero(time == RUN_DELIMITER_TIME):
        if end > start:
            yield data[start:end]
        start = end + 1
    if start < len(data):
        yield data[start:]


def _staging_mask(thrust: np.ndarray, halfwidth: int, jump_frac: float) -> np.ndarray:
    """``True`` where a sample is safely far from a thrust discontinuity.

    Stage burnout and ignition step the thrust, so a central difference straddling
    one measures the step rather than the dynamics.
    """
    keep = np.ones(len(thrust), dtype=bool)
    scale = np.abs(thrust).max()
    if scale <= 0:
        return keep
    for idx in np.flatnonzero(np.abs(np.diff(thrust)) > jump_frac * scale):
        lo = max(0, idx - halfwidth)
        hi = min(len(thrust), idx + halfwidth + 2)
        keep[lo:hi] = False
    return keep


def _central_diff(values: np.ndarray, time: np.ndarray) -> np.ndarray:
    """Central difference on the interior; returns rows ``1 .. n-2``."""
    return (values[2:] - values[:-2]) / (time[2:] - time[:-2])[:, None]


def build_dataset(cfg: GeneratorConfig, plot_file: Path) -> dict[str, np.ndarray]:
    """Assemble ``(x, p, xdot)`` samples from every Monte Carlo run."""
    names, data = parse_plot_file(plot_file)
    index = {name: i for i, name in enumerate(names)}

    missing = [v for v in (*cfg.state_vars, *cfg.param_vars) if v not in index]
    if missing:
        raise RuntimeError(f"columns absent from plot1.asc: {missing}")

    state_cols = [index[v] for v in cfg.state_vars]
    param_cols = [index[v] for v in cfg.param_vars]
    time_col, thrust_col = index["time"], index["thrust"]

    # Optional ground-truth derivative columns, used only to score our differencing.
    truth_pairs = [
        (i, index[TRUTH_DERIVATIVES[v]])
        for i, v in enumerate(cfg.state_vars)
        if TRUTH_DERIVATIVES.get(v) in index
    ]

    xs, ps, xdots, run_ids, times = [], [], [], [], []
    n_raw = 0

    for run_id, run_data in enumerate(split_runs(data, time_col)):
        if len(run_data) < 3:
            continue
        time = run_data[:, time_col]
        x = run_data[:, state_cols]
        p = run_data[:, param_cols]

        xdot = _central_diff(x, time)
        interior = slice(1, len(run_data) - 1)
        n_raw += len(xdot)

        keep = _staging_mask(
            run_data[:, thrust_col], cfg.stage_mask_halfwidth, cfg.stage_thrust_jump_frac
        )[interior]

        xs.append(x[interior][keep])
        ps.append(p[interior][keep])
        xdots.append(xdot[keep])
        times.append(time[interior][keep])
        run_ids.append(np.full(keep.sum(), run_id))

    if not xs:
        raise RuntimeError("no usable samples -- check ENDTIME and plot_step")

    # float64 throughout, deliberately. |SBII| is ~6.4e6 m, where float32 spacing is
    # 0.5 m -- differencing that at dt=0.05 would reintroduce ~10 m/s of quantisation
    # noise, the same failure the plot-file precision patch exists to prevent. Cast to
    # float32 in the Dataset if the training loop wants it, after normalisation.
    dataset = {
        "x": np.concatenate(xs),
        "p": np.concatenate(ps),
        "xdot": np.concatenate(xdots),
        "t": np.concatenate(times),
        "run_id": np.concatenate(run_ids).astype(np.int32),
        "state_names": np.array(cfg.state_vars),
        "param_names": np.array(cfg.param_vars),
    }

    n_kept = len(dataset["x"])
    print(
        f"[dataset] {run_id + 1} runs, {n_kept} samples "
        f"({n_raw - n_kept} masked at staging transients)"
    )

    if truth_pairs:
        # d(VBII)/dt against CADAC's own ABII. This is the check that the plot-file
        # precision patch actually took; unpatched it lands around 4e-2 m/s^2.
        errors = []
        for run_id, run_data in enumerate(split_runs(data, time_col)):
            if len(run_data) < 3:
                continue
            time = run_data[:, time_col]
            xdot = _central_diff(run_data[:, state_cols], time)
            keep = _staging_mask(
                run_data[:, thrust_col], cfg.stage_mask_halfwidth,
                cfg.stage_thrust_jump_frac,
            )[1:len(run_data) - 1]
            for state_i, truth_col in truth_pairs:
                truth = run_data[1:-1, truth_col]
                errors.append(np.abs(xdot[:, state_i] - truth)[keep])
        residual = np.concatenate(errors)
        print(
            f"[dataset] FD vs CADAC ABII: median={np.median(residual):.3e} "
            f"p99={np.percentile(residual, 99):.3e} max={residual.max():.3e} m/s^2"
        )
        dataset["fd_residual_p99"] = np.array(np.percentile(residual, 99))

    return dataset


def save(dataset: dict[str, np.ndarray], cfg: GeneratorConfig) -> Path:
    cfg.out_path.parent.mkdir(parents=True, exist_ok=True)
    np.savez_compressed(
        cfg.out_path, **dataset,
        config=np.array(str(cfg)),
        dt=np.array(cfg.plot_step),
    )
    print(f"[save] {cfg.out_path} ({cfg.out_path.stat().st_size / 1e6:.1f} MB)")
    return cfg.out_path


def generate(cfg: GeneratorConfig, force_rebuild: bool = False) -> Path:
    """Run the full pipeline end to end in a single batch."""
    prepare(cfg, force_rebuild=force_rebuild)
    write_input(cfg)
    return save(build_dataset(cfg, run(cfg)), cfg)


# --------------------------------------------------------------------------- #
# chunked generation
# --------------------------------------------------------------------------- #

#: Arrays concatenated when merging chunks. Everything else in a chunk file is
#: metadata that must agree rather than accumulate.
_STACKED = ("x", "p", "xdot", "t", "run_id")

#: Seed stride between chunks. Wide enough that no two chunks can draw overlapping
#: dispersion sequences, and stable so a chunk's seed never depends on how many
#: chunks were requested -- that is what makes extending a campaign reproducible.
CHUNK_SEED_STRIDE = 1000


def prepare(cfg: GeneratorConfig, force_rebuild: bool = False) -> Path:
    """Fetch, patch and compile. Safe to call repeatedly; compiles at most once."""
    fetch_source(cfg, force=force_rebuild)
    patch_source(cfg)
    if force_rebuild or not cfg.exe.exists():
        build(cfg)
    return cfg.exe


def generate_chunked(
    cfg: GeneratorConfig,
    chunk_size: int = 10,
    chunk_dir: Path | None = None,
    force_rebuild: bool = False,
) -> Path:
    """Generate ``cfg.n_runs`` runs in batches, then merge.

    Each batch is simulated, parsed to ``chunk_XXX.npz``, and its ``plot1.asc``
    deleted before the next starts. Three reasons:

    - **Disk.** At ``plot_step=0.01`` a run writes ~63 MB of ASCII, so one large
      ``MONTE`` would hold multiple GB before anything is parsed. Peak usage here
      is one batch.
    - **Resumability.** A chunk whose ``.npz`` already exists is skipped, so an
      interrupted campaign resumes where it stopped rather than restarting.
    - **Extension.** Chunk ``i`` always uses ``seed = cfg.seed + i*CHUNK_SEED_STRIDE``
      and ``run_id`` offset ``i*chunk_size``, both independent of the total
      requested. Raising ``cfg.n_runs`` and re-running therefore computes only the
      new chunks, with no duplicated draws and no colliding ids.

    The merged result is identical to what a single batch of ``cfg.n_runs`` would
    have produced.
    """
    if chunk_size < 1:
        raise ValueError("chunk_size must be >= 1")

    chunk_dir = Path(chunk_dir) if chunk_dir else cfg.out_path.parent / "chunks"
    chunk_dir.mkdir(parents=True, exist_ok=True)

    n_chunks = -(-cfg.n_runs // chunk_size)  # ceil
    base_seed, total_runs = cfg.seed, cfg.n_runs
    print(f"[chunk] {total_runs} runs in {n_chunks} chunk(s) of <= {chunk_size}")

    paths: list[Path] = []
    start = time.monotonic()
    computed = 0

    for i in range(n_chunks):
        path = chunk_dir / f"chunk_{i:03d}.npz"
        paths.append(path)
        if path.exists():
            print(f"[chunk] {i + 1}/{n_chunks} exists, skipping {path.name}")
            continue

        runs = min(chunk_size, total_runs - i * chunk_size)
        cfg.n_runs = runs
        cfg.seed = base_seed + i * CHUNK_SEED_STRIDE
        print(f"\n[chunk] {i + 1}/{n_chunks}  {runs} runs  seed={cfg.seed}")

        chunk_start = time.monotonic()
        write_input(cfg)
        plot_file = run(cfg)
        dataset = build_dataset(cfg, plot_file)
        dataset["run_id"] = dataset["run_id"] + i * chunk_size

        np.savez_compressed(
            path, **dataset, config=np.array(str(cfg)), dt=np.array(cfg.plot_step)
        )
        # Reclaim before the next batch; this is the whole point of chunking.
        plot_file.unlink(missing_ok=True)

        computed += 1
        elapsed = time.monotonic() - chunk_start
        print(f"[chunk] {path.name} in {elapsed:.0f}s ({elapsed / runs:.1f}s/run)")
        remaining = n_chunks - i - 1
        if remaining:
            done = time.monotonic() - start
            print(f"[chunk] ~{done / computed * remaining / 60:.0f} min remaining")

    cfg.n_runs, cfg.seed = total_runs, base_seed
    return merge_datasets(paths, cfg.out_path)


def merge_datasets(paths: Sequence[Path], out_path: Path) -> Path:
    """Concatenate chunk files into one dataset.

    Metadata (state/parameter names, timestep) must agree across chunks -- a
    mismatch means they came from different configurations and silently stacking
    them would produce columns that mean different things in different rows.
    """
    paths = [Path(p) for p in paths]
    missing = [p.name for p in paths if not p.exists()]
    if missing:
        raise RuntimeError(f"missing chunk files: {missing}")

    merged: dict[str, list[np.ndarray]] = {k: [] for k in _STACKED}
    meta: dict[str, np.ndarray] = {}

    for path in paths:
        with np.load(path, allow_pickle=True) as chunk:
            for key in _STACKED:
                merged[key].append(chunk[key])
            for key in ("state_names", "param_names", "dt"):
                if key in meta and not np.array_equal(meta[key], chunk[key]):
                    raise RuntimeError(
                        f"{path.name}: {key} differs from earlier chunks "
                        f"({chunk[key]} vs {meta[key]}) -- chunks are not comparable"
                    )
                meta[key] = chunk[key]

    dataset = {k: np.concatenate(v) for k, v in merged.items()} | meta
    out_path = Path(out_path)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    np.savez_compressed(out_path, **dataset)

    n_runs = len(np.unique(dataset["run_id"]))
    print(
        f"\n[merge] {len(paths)} chunks -> {out_path} "
        f"({len(dataset['x'])} samples, {n_runs} runs, "
        f"{out_path.stat().st_size / 1e6:.1f} MB)"
    )
    return out_path


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("-n", "--n-runs", type=int, default=GeneratorConfig.n_runs)
    parser.add_argument("--seed", type=int, default=GeneratorConfig.seed)
    parser.add_argument("--plot-step", type=float, default=GeneratorConfig.plot_step)
    parser.add_argument("--endtime", type=float, default=GeneratorConfig.endtime)
    parser.add_argument("--work-dir", type=Path, default=GeneratorConfig.work_dir)
    parser.add_argument("-o", "--out", type=Path, default=GeneratorConfig.out_path)
    parser.add_argument("--rebuild", action="store_true", help="re-clone and recompile")
    parser.add_argument(
        "--chunk-size", type=int, default=0,
        help="generate in batches of this many runs (resumable); 0 = single batch",
    )
    parser.add_argument(
        "--cadac-source", type=Path, default=None,
        help="local CADAC checkout to copy ROCKET6G from instead of cloning",
    )
    args = parser.parse_args(argv)

    cfg = GeneratorConfig(
        work_dir=args.work_dir, out_path=args.out, n_runs=args.n_runs,
        seed=args.seed, plot_step=args.plot_step, endtime=args.endtime,
        cadac_source=args.cadac_source,
    )
    try:
        if args.chunk_size:
            prepare(cfg, force_rebuild=args.rebuild)
            generate_chunked(cfg, chunk_size=args.chunk_size)
        else:
            generate(cfg, force_rebuild=args.rebuild)
    except (RuntimeError, ValueError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
