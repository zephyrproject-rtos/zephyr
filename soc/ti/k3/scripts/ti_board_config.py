#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 Texas Instruments Incorporated

"""TI K3 Board Configuration Binary Generator and Blob Creator.

Converts YAML board configurations to binary format for TI K3 SYSFW.
Supports both single config mode and combined blob mode.
"""

import argparse
import os
import struct
import sys

try:
    import yaml
    from jsonschema import ValidationError, validate
except ImportError as e:
    print(f"ERROR: Required Python package missing: {e}", file=sys.stderr)
    print("Install: pip install pyyaml jsonschema", file=sys.stderr)
    sys.exit(1)

# Board config type identifiers per TI SYSFW specification
BOARDCFG = 0xB
BOARDCFG_RM = 0xC
BOARDCFG_SEC = 0xD
BOARDCFG_PM = 0xE

# Descriptor format: type(u16), offset(u16), size(u16), devgrp(u8), reserved(u8)
BOARDCFG_DESC_FMT = '<HHHBB'

# Config type names for display
BOARDCFG_TYPE_NAMES = {
    BOARDCFG: "BOARDCFG",
    BOARDCFG_RM: "BOARDCFG_RM",
    BOARDCFG_SEC: "BOARDCFG_SEC",
    BOARDCFG_PM: "BOARDCFG_PM",
}


class TIBoardConfigGenerator:
    """Generates TI K3 board configuration binaries from YAML."""

    def __init__(self, schema_file: str, config_file: str):
        self.schema_file = schema_file
        self.config_file = config_file
        self.schema: dict[str, any] = {}
        self.config: dict[str, any] = {}

    def load_files(self) -> None:
        """Load and parse YAML schema and config files."""
        try:
            with open(self.schema_file) as f:
                self.schema = yaml.safe_load(f)
        except Exception as e:
            print(f"ERROR: Failed to load schema {self.schema_file}: {e}", file=sys.stderr)
            sys.exit(1)

        try:
            with open(self.config_file) as f:
                self.config = yaml.safe_load(f)
        except Exception as e:
            print(f"ERROR: Failed to load config {self.config_file}: {e}", file=sys.stderr)
            sys.exit(1)

    def validate_config(self) -> bool:
        """Validate configuration against schema."""
        try:
            validate(instance=self.config, schema=self.schema)
            return True
        except ValidationError as e:
            print("ERROR: Configuration validation failed:", file=sys.stderr)
            print(f"  {e.message}", file=sys.stderr)
            if e.path:
                print(f"  At: {' -> '.join(str(p) for p in e.path)}", file=sys.stderr)
            return False
        except Exception as e:
            print(f"ERROR: Validation error: {e}", file=sys.stderr)
            return False

    def _resolve_ref(self, ref: str) -> dict[str, any]:
        """Resolve a JSON schema $ref like '#/definitions/u8'."""
        if not ref.startswith("#/"):
            raise ValueError(f"Unsupported $ref: {ref}")

        path_parts = ref[2:].split('/')
        current = self.schema

        for part in path_parts:
            if part in current:
                current = current[part]
            else:
                raise ValueError(f"Cannot resolve $ref: {ref}")

        return current

    def _get_type_size(self, type_def: dict[str, any]) -> int:
        """Determine byte size (1, 2, or 4) from type definition."""
        max_val = type_def.get('maximum', 0xFFFFFFFF)
        if max_val <= 0xFF:
            return 1  # u8
        elif max_val <= 0xFFFF:
            return 2  # u16
        else:
            return 4  # u32

    def _serialize_value(self, value: any, type_schema: dict[str, any]) -> bytes:
        """Serialize a single integer value to little-endian bytes."""
        if '$ref' in type_schema:
            ref_schema = self._resolve_ref(type_schema['$ref'])
            size = self._get_type_size(ref_schema)
        elif type_schema.get('type') == 'integer':
            size = self._get_type_size(type_schema)
        else:
            raise ValueError(f"Unsupported type schema: {type_schema}")

        if not isinstance(value, int):
            raise TypeError(f"Expected int, got {type(value).__name__}")

        return value.to_bytes(size, byteorder='little')

    def _serialize_object(self, obj: dict[str, any], obj_schema: dict[str, any]) -> bytes:
        """Serialize an object (dictionary) to bytes depth-first."""
        result = bytearray()
        for prop_name, prop_schema in obj_schema.get('properties', {}).items():
            if prop_name in obj:
                result += self._serialize_any(obj[prop_name], prop_schema)
        return bytes(result)

    def _serialize_array(self, arr: list, arr_schema: dict[str, any]) -> bytes:
        """Serialize an array to bytes element-by-element."""
        result = bytearray()
        items_schema = arr_schema.get('items', {})
        for item in arr:
            result += self._serialize_any(item, items_schema)
        return bytes(result)

    def _serialize_any(self, value: any, value_schema: dict[str, any]) -> bytes:
        """Serialize any value to bytes based on its schema type."""
        if '$ref' in value_schema:
            return self._serialize_value(value, value_schema)

        value_type = value_schema.get('type')
        if value_type == 'object':
            return self._serialize_object(value, value_schema)
        elif value_type == 'array':
            return self._serialize_array(value, value_schema)
        elif value_type == 'integer':
            return self._serialize_value(value, value_schema)
        else:
            raise ValueError(f"Unsupported type: {value_type}")

    def generate(self) -> bytes:
        """Generate board configuration binary from loaded YAML."""
        result = bytearray()
        for key in self.config:
            if key in self.schema.get('properties', {}):
                key_schema = self.schema['properties'][key]
                result += self._serialize_any(self.config[key], key_schema)
        return bytes(result)


def create_combined_blob(
    config_binaries: list[tuple[bytes, int, str]],
    output_file: str,
    sw_rev: int,
    devgrp: int,
    verbose: bool,
) -> None:
    """Create combined board config blob from multiple binaries.

    Blob structure follows TI SYSFW specification:
        - Header: num_elems (u8) + sw_rev (u8)
        - Descriptors: array of (type, offset, size, devgrp, reserved)
        - Data: concatenated config binaries
    """
    num_elems = len(config_binaries)
    header_size = 2
    desc_size = struct.calcsize(BOARDCFG_DESC_FMT)
    offset = header_size + (num_elems * desc_size)

    descriptors = bytearray()
    config_data = bytearray()

    for binary_data, config_type, _ in config_binaries:
        size = len(binary_data)
        desc = struct.pack(BOARDCFG_DESC_FMT, config_type, offset, size, devgrp, 0)
        descriptors += desc
        config_data += binary_data
        offset += size

    try:
        with open(output_file, 'wb') as f:
            f.write(struct.pack('<BB', num_elems, sw_rev))
            f.write(descriptors)
            f.write(config_data)

        total_size = header_size + len(descriptors) + len(config_data)
        if verbose:
            print(f"\nCombined blob created: {output_file}")
            print(f"  Number of configs: {num_elems}")
            print(f"  Total size: {total_size} bytes")
            for _, config_type, name in config_binaries:
                type_name = BOARDCFG_TYPE_NAMES.get(config_type, f"UNKNOWN(0x{config_type:X})")
                print(f"  - {type_name}: {name}.yaml")
        else:
            print(f"Generated: {output_file} ({total_size} bytes)")

    except OSError as e:
        print(f"ERROR: Failed to write output file: {e}", file=sys.stderr)
        sys.exit(1)


def validate_file_exists(filepath: str, description: str) -> None:
    """Validate that a file exists, exit with error if not."""
    if not os.path.exists(filepath):
        print(f"ERROR: {description} not found: {filepath}", file=sys.stderr)
        sys.exit(1)


def collect_configs_to_process(args) -> list[tuple[str, int, str]]:
    """Collect configuration files to process from arguments."""
    configs = []
    if args.board_cfg:
        configs.append((args.board_cfg, BOARDCFG, 'board-cfg'))
    if args.sec_cfg:
        configs.append((args.sec_cfg, BOARDCFG_SEC, 'sec-cfg'))
    if args.pm_cfg:
        configs.append((args.pm_cfg, BOARDCFG_PM, 'pm-cfg'))
    if args.rm_cfg:
        configs.append((args.rm_cfg, BOARDCFG_RM, 'rm-cfg'))
    return configs


def validate_configs_exist(configs: list[tuple[str, int, str]]) -> None:
    """Validate that all config files exist."""
    for yaml_path, _, name in configs:
        validate_file_exists(yaml_path, f"{name} file")


def convert_config_to_binary(schema_file: str, yaml_path: str, name: str, verbose: bool) -> bytes:
    """Convert a single YAML config to binary format."""
    if verbose:
        print(f"Converting {name}: {yaml_path}")

    generator = TIBoardConfigGenerator(schema_file, yaml_path)
    generator.load_files()

    if not generator.validate_config():
        print(f"ERROR: Validation failed for {name}", file=sys.stderr)
        sys.exit(1)

    binary_data = generator.generate()

    if verbose:
        print(f"  Generated {len(binary_data)} bytes")

    return binary_data


def process_single_mode(args) -> None:
    """Process single config YAML to binary mode."""
    validate_file_exists(args.config, "Config file")

    generator = TIBoardConfigGenerator(args.schema, args.config)
    generator.load_files()

    if not generator.validate_config():
        sys.exit(1)

    binary_data = generator.generate()

    with open(args.output, 'wb') as f:
        f.write(binary_data)

    print(f"Generated: {args.output} ({len(binary_data)} bytes)")


def process_combined_mode(args) -> None:
    """Process multiple YAMLs into combined blob mode."""
    configs_to_process = collect_configs_to_process(args)
    validate_configs_exist(configs_to_process)

    config_binaries = []
    for yaml_path, config_type, name in configs_to_process:
        binary_data = convert_config_to_binary(args.schema, yaml_path, name, args.verbose)
        config_binaries.append((binary_data, config_type, name))

    create_combined_blob(config_binaries, args.output, args.sw_rev, args.devgrp, args.verbose)


def validate_mode_selection(parser, single_mode: bool, combined_mode: bool) -> None:
    """Validate that exactly one mode is selected."""
    if single_mode and combined_mode:
        parser.error("Cannot use both single mode (--config) and combined mode")
    if not single_mode and not combined_mode:
        parser.error("Must specify either --config or at least one --board-cfg/--sec-cfg/etc")


def main():
    """Main entry point."""
    parser = argparse.ArgumentParser(
        description="TI K3 Board Configuration Binary Generator",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Mode 1: Single Binary
  %(prog)s --schema schema.yaml --config board-cfg.yaml --output board-cfg.bin

Mode 2: Combined Blob (converts YAMLs and creates blob)
  # AM62L: board + security
  %(prog)s --schema schema.yaml --board-cfg board-cfg.yaml --sec-cfg sec-cfg.yaml -o combined.bin

  # AM62x: all 4 configs
  %(prog)s --schema schema.yaml --board-cfg board-cfg.yaml --sec-cfg sec-cfg.yaml \\
           --pm-cfg pm-cfg.yaml --rm-cfg rm-cfg.yaml -o combined.bin
        """,
        allow_abbrev=False,
    )

    parser.add_argument('--schema', required=True, help='YAML schema file')
    parser.add_argument('--config', help='Single config YAML (mode 1)')
    parser.add_argument('--board-cfg', help='board-cfg.yaml (mode 2)')
    parser.add_argument('--sec-cfg', help='sec-cfg.yaml (mode 2)')
    parser.add_argument('--pm-cfg', help='pm-cfg.yaml (mode 2)')
    parser.add_argument('--rm-cfg', help='rm-cfg.yaml (mode 2)')
    parser.add_argument('--sw-rev', type=int, default=1, help='Software revision (default: 1)')
    parser.add_argument('--devgrp', type=int, default=0, help='Device group (default: 0)')
    parser.add_argument('-o', '--output', required=True, help='Output binary file')
    parser.add_argument('-v', '--verbose', action='store_true', help='Verbose output')

    args = parser.parse_args()

    validate_file_exists(args.schema, "Schema file")

    single_mode = args.config is not None
    combined_mode = any([args.board_cfg, args.sec_cfg, args.pm_cfg, args.rm_cfg])

    validate_mode_selection(parser, single_mode, combined_mode)

    if single_mode:
        process_single_mode(args)
    else:
        process_combined_mode(args)


if __name__ == '__main__':
    main()
