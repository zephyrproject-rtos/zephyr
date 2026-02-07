#!/usr/bin/env python3
# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

"""
Probe Settings Validator

This script validates probe_settings.yaml files against the defined JSON schema.
"""

import argparse
import json
import os
import sys
from pathlib import Path
from typing import Any

# Set UTF-8 encoding for Windows compatibility
if os.name == 'nt':  # Windows
    import codecs

    sys.stdout = codecs.getwriter('utf-8')(sys.stdout.buffer, 'strict')
    sys.stderr = codecs.getwriter('utf-8')(sys.stderr.buffer, 'strict')

try:
    import yaml
    from jsonschema import Draft7Validator
except ImportError as e:
    print(f"Error: Required package not found: {e}")
    print("Please install required packages:")
    print("pip install pyyaml jsonschema")
    sys.exit(1)


class ProbeSettingsValidator:
    """Validator for probe settings configuration files."""

    def __init__(self, schema_path: Path):
        """Initialize validator with schema file."""
        self.schema_path = schema_path
        self.schema = self._load_schema()
        self.validator = Draft7Validator(self.schema)

    def _load_schema(self) -> dict[str, Any]:
        """Load JSON schema from file."""
        try:
            with open(self.schema_path, encoding='utf-8') as f:
                return json.load(f)
        except FileNotFoundError:
            raise FileNotFoundError(
                f"Schema file not found: {self.schema_path}"
            ) from FileNotFoundError
        except json.JSONDecodeError as e:
            raise ValueError(f"Invalid JSON schema: {e}") from json.JSONDecodeError

    def _load_yaml(self, yaml_path: Path) -> dict[str, Any]:
        """Load YAML configuration file."""
        try:
            with open(yaml_path, encoding='utf-8') as f:
                return yaml.safe_load(f)
        except FileNotFoundError:
            raise FileNotFoundError(f"YAML file not found: {yaml_path}") from FileNotFoundError
        except yaml.YAMLError as e:
            raise ValueError(f"Invalid YAML file: {e}") from yaml.YAMLError

    def validate_file(self, yaml_path: Path) -> tuple[bool, list[str]]:
        """
        Validate a probe settings YAML file.

        Returns:
            tuple: (is_valid, list_of_errors)
        """
        try:
            config = self._load_yaml(yaml_path)
            errors = []

            # Validate against schema
            for error in self.validator.iter_errors(config):
                error_path = " -> ".join(str(p) for p in error.absolute_path)
                if error_path:
                    error_msg = f"Path '{error_path}': {error.message}"
                else:
                    error_msg = f"Root: {error.message}"
                errors.append(error_msg)

            # Additional custom validations
            custom_errors = self._custom_validations(config)
            errors.extend(custom_errors)

            return len(errors) == 0, errors

        except (FileNotFoundError, ValueError) as e:
            return False, [str(e)]

    def _custom_validations(self, config: dict[str, Any]) -> list[str]:
        """Perform additional custom validations."""
        errors = []

        if 'routes' in config:
            # Check for duplicate route IDs
            route_ids = [route.get('id') for route in config['routes']]
            if len(route_ids) != len(set(route_ids)):
                errors.append("Duplicate route IDs found")

            # Check for duplicate route names
            route_names = [route.get('name') for route in config['routes']]
            if len(route_names) != len(set(route_names)):
                errors.append("Duplicate route names found")

            # Validate channel configurations
            for i, route in enumerate(config['routes']):
                channels = route['channels']
                if route.get('type') != 'single' and channels.get('channels_n') == channels.get(
                    'channels_p'
                ):
                    errors.append(
                        f"Route {i}: diff mode channels_p and channels_n should not equal"
                    )

        return errors


def print_success(message: str):
    """Print success message with cross-platform compatibility."""
    try:
        print(f"[PASS] {message}")
    except UnicodeEncodeError:
        print(f"[PASS] {message}")


def print_error(message: str):
    """Print error message with cross-platform compatibility."""
    try:
        print(f"[FAIL] {message}")
    except UnicodeEncodeError:
        print(f"[FAIL] {message}")


def main():
    """Main function for command-line usage."""
    parser = argparse.ArgumentParser(
        description="Validate probe_settings.yaml files against schema", allow_abbrev=False
    )
    parser.add_argument("yaml_file", type=Path, help="Path to probe_settings.yaml file to validate")
    parser.add_argument(
        "--schema",
        type=Path,
        default=Path(__file__).parent / "probe_settings_schema.json",
        help="Path to JSON schema file (default: probe_settings_schema.json in same directory)",
    )
    parser.add_argument("--verbose", "-v", action="store_true", help="Enable verbose output")

    args = parser.parse_args()

    try:
        validator = ProbeSettingsValidator(args.schema)
        is_valid, errors = validator.validate_file(args.yaml_file)

        if is_valid:
            print_success(f"{args.yaml_file} is valid")
            if args.verbose:
                print("All validation checks passed successfully.")
            return 0
        else:
            print_error(f"{args.yaml_file} is invalid:")
            for error in errors:
                print(f"  - {error}")
            return 1

    except Exception as e:
        print_error(f"Validation error: {e}")
        return 1


if __name__ == "__main__":
    sys.exit(main())
