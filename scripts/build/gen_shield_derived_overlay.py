#!/usr/bin/env python3
#
# Copyright 2025 TOKITA Hiroshi
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import os
import re
import sys

import yaml

L_SHIELD_YML = "shield.yml"
L_SHIELD = "shield"
L_NAME = "name"
L_OPTIONS = "options"
L_VARIANTS = "variants"
L_DEFAULT = "default"
L_TYPE = "type"
L_INT = "int"
L_BOOLEAN = "boolean"
L_STRING = "string"
L___INDEX = "__index"


def load_options_schema(shield_dir, shield_name):
    """
    Load the options schema for the specified shield

    Args:
        shield_dir (str): The directory containing the shield.yml file.
        shield_name (str): The name of the shield or its variant.

    Returns:
        dict: The parsed options schema or a default schema.
    """

    shield_path = os.path.join(shield_dir, L_SHIELD_YML)
    INDEX_DEF_DEFAULT = {L_TYPE: L_INT, L_DEFAULT: 0}

    if not os.path.exists(shield_path):
        return {L___INDEX: INDEX_DEF_DEFAULT}

    with open(shield_path) as file:
        schema = yaml.safe_load(file)

    opt_schema = schema.get(L_SHIELD, {}).get(L_OPTIONS, {L___INDEX: INDEX_DEF_DEFAULT})

    if schema[L_SHIELD][L_NAME] == shield_name:
        return opt_schema

    variant_name = shield_name[len(schema[L_SHIELD].get(L_NAME, "")) + 1 :]
    for variant in schema[L_SHIELD].get(L_VARIANTS, []):
        if variant[L_NAME] == variant_name:
            for k, v in variant.get(L_OPTIONS, {}).items():
                opt_schema[k] = opt_schema.get(k, {}) | v
            break

    return opt_schema


def gen_opt_def_list(opt_name, opt_value, def_stem, opts_schema):
    """
    Generate a list of macro definitions (#define statements) for a shield option.

    Args:
        opt_name (str): The option name.
        opt_value (any): The opt_value of the option.
        def_stem (str): The common part name for the macro definitions.
        opts_schema (dict): The schema describing the shield options.

    Returns:
        list: A list of strings representing C macro definitions.

    Raises:
        ValueError: If the option schema is invalid or missing required fields.
    """

    opt_name_upper = re.sub(r"[^a-zA-Z0-9]", "_", opt_name).upper()
    opt = opts_schema.get(opt_name, None)

    if opt and L_TYPE in opt and opt[L_TYPE] == L_INT:
        try:
            if str(opt_value).lower().startswith("0x"):
                num = int(str(opt_value), 16)
            else:
                num = int(str(opt_value))
            return [
                f"#define {def_stem}_{opt_name_upper}_EXISTS 1",
                f"#define {def_stem}_{opt_name_upper} {num}",
                f"#define {def_stem}_{opt_name_upper}_HEX {hex(num)[2:]}",
            ]
        except ValueError as ve:
            raise ValueError(f"Invalid format '{str(opt_value)}' for option '{opt_name}'.") from ve
    elif opt and L_TYPE in opt and opt[L_TYPE] == L_BOOLEAN:
        return [
            f"#define {def_stem}_{opt_name_upper}_EXISTS 1",
            f"#define {def_stem}_{opt_name_upper} 1",
        ]
    elif opt and L_TYPE in opt and opt[L_TYPE] == L_STRING:
        token = re.sub(r"[^a-zA-Z0-9]", "_", str(opt_value))
        return [
            f"#define {def_stem}_{opt_name_upper}_EXISTS 1",
            f"#define {def_stem}_{opt_name_upper} \"{opt_value}\"",
            f"#define {def_stem}_{opt_name_upper}_STRING_UNQUOTED {str(opt_value)}",
            f"#define {def_stem}_{opt_name_upper}_STRING_TOKEN {token}",
            f"#define {def_stem}_{opt_name_upper}_STRING_UPPER_TOKEN {token.upper()}",
        ]

    value_or_1 = opt_value if opt_value else 1

    return [
        f"#define {def_stem}_{opt_name_upper}_EXISTS 1",
        f"#define {def_stem}_{opt_name_upper} {value_or_1}",
    ]


def generate_defs(shield_dir, shield_name, shield_optstr):
    """
    Generate C macro definitions for shield options.

    Args:
        shield_dir (str): The directory containing the shield.yml file.
        shield_name (str): The name of the shield.
        shield_optstr (str): A string of shield options.

    Returns:
        str: The generated C macro definitions as a single string.
    """

    derived_name = re.sub(r"[^a-zA-Z0-9]", "_", shield_name + shield_optstr)
    opts_schema = load_options_schema(shield_dir, shield_name)
    opt_str = shield_optstr
    definitions = []
    defaults = []

    for name, opt in opts_schema.items():
        def_stem = f"SHIELD_OPTION_DEFAULT_{shield_name}".upper()
        default_value = opt.get(L_DEFAULT) if opt.get(L_TYPE) != L_BOOLEAN else 0
        defaults.extend(gen_opt_def_list(name, default_value, def_stem, opts_schema))

    if shield_optstr.startswith("@"):
        idx_match = re.match(r"^@([0-9]*)(.*)$", shield_optstr)
        if idx_match:
            idx_str = idx_match.group(1)
            opt_str = idx_match.group(2)
            def_stem = f"SHIELD_OPTION_{derived_name}".upper()
            definitions.extend(gen_opt_def_list(L___INDEX, idx_str, def_stem, opts_schema))

    for opt in opt_str.split(":"):
        if not opt:
            continue
        opt_match = re.match(r"^([a-zA-Z_][a-zA-Z0-9-_]+)(=.*)?$", opt)
        if opt_match:
            opt_name = opt_match.group(1)
            opt_value = opt_match.group(2)[1:] if opt_match.group(2) else None
            def_stem = f"SHIELD_OPTION_{derived_name}".upper()
            definitions.extend(gen_opt_def_list(opt_name, opt_value, def_stem, opts_schema))

    return '\n'.join(definitions + ["\n"] + defaults)


def generate_derived_overlay(shield_overlay, shield_optstr, derived_overlay):
    """
    Generate a derived overlay file for a shield with parameterized options.

    Args:
        shield_overlay (str): The input shield overlay file path.
        shield_optstr (str): A colon-separated string of shield options.
        derived_overlay (str): The output file path for the derived overlay.

    Returns:
        None
    """

    shield_overlay_abs = os.path.abspath(shield_overlay)
    shield_dir = os.path.dirname(shield_overlay_abs)
    shield_name = os.path.splitext(os.path.basename(shield_overlay_abs))[0]
    derived_name = re.sub(r"[^a-zA-Z0-9]", "_", shield_name + shield_optstr)
    definitions_str = generate_defs(shield_dir, shield_name, shield_optstr)

    overlay_content = (
        f"/* auto-generated by gen_shield_derived_overlay.py, don't edit */"
        "\n"
        "\n"
        f"#include <shield_utils.h>"
        "\n"
        "\n"
        f"{definitions_str}"
        "\n"
        "\n"
        f"#define SHIELD_BASE_NAME {shield_name.upper()}"
        "\n"
        f"#define SHIELD_BASE_NAME_EXISTS 1"
        "\n"
        f"#define SHIELD_DERIVED_NAME {derived_name.upper()}"
        "\n"
        f"#define SHIELD_DERIVED_NAME_EXISTS 1"
        "\n"
        "\n"
        f"#include <{shield_overlay_abs}>"
        "\n"
        "\n"
        f"#undef SHIELD_DERIVED_NAME_EXISTS"
        "\n"
        f"#undef SHIELD_DERIVED_NAME"
        "\n"
        f"#undef SHIELD_BASE_NAME_EXISTS"
        "\n"
        f"#undef SHIELD_BASE_NAME"
        "\n"
    )

    os.makedirs(os.path.dirname(derived_overlay), exist_ok=True)
    with open(derived_overlay, "w") as file:
        file.write(overlay_content)

    print(f"Derived overlay file generated: {derived_overlay}", file=sys.stderr)


def main():
    """
    Main function to parse command-line arguments and generate derived overlays.
    """

    parser = argparse.ArgumentParser(
        allow_abbrev=False, description="Process shield files and generate derived overlays."
    )

    parser.add_argument("shield_overlay", type=str, help="The filename of the shield overlay.")
    parser.add_argument(
        "--shield-options",
        type=str,
        required=True,
        help="Optional shield options as a colon-separated string (e.g., option1:option2).",
    )
    parser.add_argument(
        "--derived-overlay",
        type=str,
        required=True,
        help="The filename of the generated derived overlay.",
    )

    args = parser.parse_args()

    generate_derived_overlay(args.shield_overlay, args.shield_options, args.derived_overlay)


if __name__ == "__main__":
    main()
