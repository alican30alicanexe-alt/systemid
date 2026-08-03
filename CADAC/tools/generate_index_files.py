#!/usr/bin/env python3
"""
Generate INDEX.md files for all CADAC components

Parses component header comments and creates structured INDEX.md files
in YAML format for use by the simulation builder.
"""

import os
import re
from pathlib import Path
from typing import Dict, List, Optional, Tuple


class ComponentParser:
    """Parse component .cpp files to extract metadata"""

    def __init__(self, filepath: str):
        self.filepath = filepath
        self.content = Path(filepath).read_text()
        self.header = self._extract_header()

    def _extract_header(self) -> str:
        """Extract the header comment block"""
        # Look for the header pattern: ////// ... //////
        lines = self.content.split('\n')
        header_lines = []
        in_header = False

        for line in lines:
            if line.startswith('////'):
                if not in_header:
                    in_header = True
                    continue
                else:
                    # End of header
                    break
            elif in_header:
                # Remove leading // and whitespace
                clean_line = re.sub(r'^//\s*', '', line)
                header_lines.append(clean_line)

        return '\n'.join(header_lines)

    def parse_metadata(self) -> Dict:
        """Parse component metadata from header"""
        metadata = {
            'name': '',
            'category': '',
            'dof': '',
            'description': ''
        }

        # Extract COMPONENT name
        match = re.search(r'COMPONENT:\s*(\w+)', self.header)
        if match:
            metadata['name'] = match.group(1)

        # Extract CATEGORY
        match = re.search(r'CATEGORY:\s*(\w+)', self.header)
        if match:
            metadata['category'] = match.group(1)

        # Extract DoF
        match = re.search(r'DoF:\s*([\w/]+)', self.header)
        if match:
            metadata['dof'] = match.group(1)

        # Extract DESCRIPTION (first line)
        match = re.search(r'DESCRIPTION:\s*\n//\s*(.+)', self.header)
        if match:
            metadata['description'] = match.group(1).strip()

        return metadata

    def parse_lifecycle(self) -> Dict:
        """Determine which lifecycle methods are present"""
        # Check USAGE section for method signatures
        has_def = bool(re.search(r'def_\w+\(\)', self.header))
        has_init = bool(re.search(r'init_\w+\(\)', self.header))
        has_exec = True  # Most components have exec

        # Check MODULES section in USAGE
        modules_match = re.search(r'MODULES section:\s*\n//\s*\w+\s+([\w,]+)', self.header)
        if modules_match:
            modules = modules_match.group(1).lower()
            has_def = 'def' in modules
            has_init = 'init' in modules
            has_exec = 'exec' in modules

        return {
            'def': has_def,
            'init': has_init,
            'exec': has_exec
        }

    def parse_io_section(self, section_name: str) -> List[Dict]:
        """Parse INPUTS, OUTPUTS, or PARAMETERS section"""
        items = []

        # Split header into sections by looking for section headers
        # Section headers are all-caps words followed by optional text in parens and a colon
        sections = re.split(r'\n([A-Z][A-Z\s]+(?:\([^)]*\))?:)', self.header)

        # Find the target section
        section_text = None
        for i in range(1, len(sections), 2):
            if section_name in sections[i]:
                section_text = sections[i+1] if i+1 < len(sections) else ""
                break

        if not section_text:
            return items

        # Parse each line: vehicle[index] - name - type - description (which includes unit)
        # Format: vehicle[0]  - grav - double - Gravity acceleration m/s^2
        line_pattern = r'vehicle\[([^\]]+)\]\s+-\s+(\w+)\s+-\s+([\w()x]+)\s+-\s+(.+?)$'

        for match in re.finditer(line_pattern, section_text, re.MULTILINE):
            index_str, name, type_str, full_description = match.groups()

            # Split description into description and unit
            # Pattern: "Description unit" where unit is often "m/s", "deg", "kg", etc.
            desc_parts = full_description.rsplit(' ', 1)
            if len(desc_parts) == 2 and ('/' in desc_parts[1] or desc_parts[1] in ['m', 's', 'kg', 'deg', 'rad', 'N', 'Pa', 'K', 'ND', 'dimensionless', 'm^2', 'm^3', 'kg/m^3', 'kg/s', 'm/s', 'm/s^2', 'rad/s', 'N·m']):
                description = desc_parts[0]
                unit = desc_parts[1]
            else:
                description = full_description
                unit = 'dimensionless'

            # Parse index (handle ranges like "10-12")
            index = index_str.strip()

            item = {
                'index': index,
                'name': name.strip(),
                'type': type_str.strip(),
                'unit': unit.strip(),
                'description': description.strip()
            }

            # Add source field for inputs
            if section_name == 'INPUTS':
                item['source'] = self._infer_source(name)

            # Add scope field for outputs
            if section_name == 'OUTPUTS':
                item['scope'] = self._infer_scope(description, type_str)

            items.append(item)

        return items

    def _infer_source(self, var_name: str) -> str:
        """Infer which module provides an input variable"""
        # Common patterns
        if var_name in ['time', 'event_time']:
            return 'kinematics'
        if var_name in ['grav', 'GRAVG', 'GRAVL']:
            return 'environment'
        if var_name in ['rho', 'press', 'vsound', 'tempk']:
            return 'environment'
        if var_name in ['vmass', 'IBBB', 'thrust']:
            return 'propulsion'
        if var_name in ['WBIB', 'WBII', 'ppx', 'qqx', 'rrx']:
            return 'euler'
        if var_name in ['VBII', 'VBED', 'SBII', 'lonx', 'latx', 'alt']:
            return 'newton'
        if var_name in ['TBI', 'TBD', 'psibdx', 'thtbdx', 'phibdx']:
            return 'kinematics'
        if var_name in ['FAPB', 'FMB']:
            return 'forces'
        if var_name.startswith('FP') or var_name.startswith('FM'):
            return 'actuators'

        return 'unknown'

    def _infer_scope(self, description: str, type_str: str) -> str:
        """Infer the scope of an output variable"""
        desc_lower = description.lower()

        if 'state' in desc_lower or '(state)' in description:
            return 'state'
        if 'diagnostic' in desc_lower or 'display' in desc_lower:
            return 'diagnostic'

        return 'inter-module'

    def parse_dependencies(self) -> Dict:
        """Parse DEPENDENCIES section"""
        deps = {
            'required_before': [],
            'required_after': [],
            'optional': []
        }

        # Find DEPENDENCIES section
        dep_section = re.search(r'DEPENDENCIES:\s*\n((?:.*\n)*?)(?=\n[A-Z]+:|$)', self.header)
        if not dep_section:
            return deps

        dep_text = dep_section.group(1)

        # Parse "Requires:" lines
        for match in re.finditer(r'-\s*Requires:\s*(.+)', dep_text):
            dep_desc = match.group(1).lower()

            # Extract module name
            module_match = re.search(r'(\w+)\s+module', dep_desc)
            if module_match:
                module = module_match.group(1)
                if module not in deps['required_before']:
                    deps['required_before'].append(module)

        return deps

    def parse_usage_example(self) -> str:
        """Extract usage example from USAGE section"""
        # Find the USAGE section
        usage_section = re.search(r'USAGE:\s*\n((?:.*\n)*?)(?=\n[A-Z]+:|$)', self.header)
        if not usage_section:
            return ""

        usage_text = usage_section.group(1)

        # Extract lines after "In input.asc"
        example_lines = []
        in_example = False

        for line in usage_text.split('\n'):
            line = line.strip()

            if 'input.asc' in line.lower():
                in_example = True
                example_lines.append('# ' + line)
            elif in_example and line and not line.startswith('Include'):
                example_lines.append(line)

        return '\n  '.join(example_lines) if example_lines else ""

    def parse_notes(self) -> List[str]:
        """Extract important notes from NOTES section"""
        notes = []

        # Find NOTES section
        notes_section = re.search(r'NOTES:\s*\n((?:.*\n)*?)(?=\n[A-Z]+:|$)', self.header)
        if not notes_section:
            return notes

        notes_text = notes_section.group(1)

        # Extract each note (lines starting with -)
        for match in re.finditer(r'-\s*(.+)', notes_text):
            notes.append(match.group(1).strip())

        return notes

    def generate_index_yaml(self) -> str:
        """Generate INDEX.md content in YAML format"""
        metadata = self.parse_metadata()
        lifecycle = self.parse_lifecycle()
        inputs = self.parse_io_section('INPUTS')
        outputs = self.parse_io_section('OUTPUTS')
        parameters = self.parse_io_section('PARAMETERS')
        dependencies = self.parse_dependencies()
        usage = self.parse_usage_example()
        notes = self.parse_notes()

        yaml_lines = [
            f"# Component Index: {metadata['name']}",
            "",
            "```yaml",
            "component:",
            f"  name: {metadata['name']}",
            f"  category: {metadata['category']}",
            f"  dof: {metadata['dof']}",
            f"  description: {metadata['description']}",
            "",
            "lifecycle:",
            f"  def: {str(lifecycle['def']).lower()}",
            f"  init: {str(lifecycle['init']).lower()}",
            f"  exec: {str(lifecycle['exec']).lower()}",
            "",
        ]

        # Inputs
        if inputs:
            yaml_lines.append("inputs:")
            for inp in inputs:
                yaml_lines.extend([
                    f"  - index: {inp['index']}",
                    f"    name: {inp['name']}",
                    f"    type: {inp['type']}",
                    f"    unit: {inp['unit']}",
                    f"    description: {inp['description']}",
                    f"    source: {inp['source']}",
                    ""
                ])
        else:
            yaml_lines.append("inputs: []")
            yaml_lines.append("")

        # Outputs
        if outputs:
            yaml_lines.append("outputs:")
            for out in outputs:
                yaml_lines.extend([
                    f"  - index: {out['index']}",
                    f"    name: {out['name']}",
                    f"    type: {out['type']}",
                    f"    unit: {out['unit']}",
                    f"    description: {out['description']}",
                    f"    scope: {out['scope']}",
                    ""
                ])
        else:
            yaml_lines.append("outputs: []")
            yaml_lines.append("")

        # Parameters
        if parameters:
            yaml_lines.append("parameters:")
            for param in parameters:
                yaml_lines.extend([
                    f"  - index: {param['index']}",
                    f"    name: {param['name']}",
                    f"    type: {param['type']}",
                    f"    unit: {param['unit']}",
                    f"    description: {param['description']}",
                    "    required: true",
                    ""
                ])
        else:
            yaml_lines.append("parameters: []")
            yaml_lines.append("")

        # Dependencies
        yaml_lines.extend([
            "dependencies:",
            f"  required_before: {dependencies['required_before']}",
            f"  required_after: {dependencies['required_after']}",
            f"  optional: {dependencies['optional']}",
            "",
        ])

        # Usage example
        if usage:
            yaml_lines.extend([
                "usage_example: |",
                f"  {usage}",
                "",
            ])

        # Notes
        if notes:
            yaml_lines.append("notes:")
            for note in notes:
                yaml_lines.append(f"  - {note}")
        else:
            yaml_lines.append("notes: []")

        yaml_lines.append("```")

        return '\n'.join(yaml_lines)


def find_all_components(components_dir: str) -> List[str]:
    """Find all component .cpp files"""
    components = []

    for root, dirs, files in os.walk(components_dir):
        for file in files:
            if file.endswith('.cpp') and not file.startswith('test_'):
                components.append(os.path.join(root, file))

    return sorted(components)


def main():
    """Generate INDEX.md files for all components"""
    # Find components directory
    script_dir = Path(__file__).parent
    components_dir = script_dir.parent / 'components'

    if not components_dir.exists():
        print(f"Error: Components directory not found: {components_dir}")
        return

    # Find all component files
    component_files = find_all_components(str(components_dir))

    print(f"Found {len(component_files)} component files")
    print("Generating INDEX.md files...\n")

    generated = 0
    errors = 0

    for filepath in component_files:
        try:
            # Parse component
            parser = ComponentParser(filepath)

            # Generate YAML
            yaml_content = parser.generate_index_yaml()

            # Write INDEX.md in same directory as component
            component_dir = Path(filepath).parent
            index_file = component_dir / 'INDEX.md'

            # Append to INDEX.md (or create if doesn't exist)
            if index_file.exists():
                # Append with separator
                with open(index_file, 'a') as f:
                    f.write('\n\n---\n\n')
                    f.write(yaml_content)
            else:
                # Create new file
                index_file.write_text(yaml_content)

            print(f"✓ {parser.parse_metadata()['name']}")
            generated += 1

        except Exception as e:
            print(f"✗ {filepath}: {e}")
            errors += 1

    print(f"\nGenerated {generated} INDEX entries")
    if errors > 0:
        print(f"Errors: {errors}")


if __name__ == '__main__':
    main()
