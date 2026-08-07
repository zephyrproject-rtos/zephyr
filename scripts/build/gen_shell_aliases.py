#!/usr/bin/env python3
#
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""
gen_aliases.py - Generate a C include file from a shell alias definition file.

Usage:
    python3 gen_shell_aliases.py [--max-command-len N] <input_file>
    [<output_file>]

Input file format:
    # This is a comment
    alias=command
    alias="command with spaces"

Output file format (suitable for inclusion in a struct initializer):
    {"alias", "command"},
    {"alias", "command with spaces"},
"""

import argparse
import configparser
import os
import sys


def escape_c_string(s: str) -> str:
    """Escape a Python string for safe embedding in a C string literal."""
    return (
        s.replace("\\", "\\\\")
        .replace('"', '\\"')
        .replace("\n", "\\n")
        .replace("\r", "\\r")
        .replace("\t", "\\t")
    )


def parse_aliases(input_path: str) -> list[tuple[str, str]]:
    """Parse alias definitions from an INI-style file using configparser."""
    config = configparser.ConfigParser(
        delimiters=("=",),
        comment_prefixes=("#",),
        inline_comment_prefixes=("#",),
        interpolation=None,
    )
    # Preserve key case (configparser lowercases by default)
    config.optionxform = str

    # configparser requires a section header; inject a dummy one
    with open(input_path) as f:
        content = "[aliases]\n" + f.read()

    try:
        config.read_string(content, source=input_path)
    except configparser.Error as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        sys.exit(1)

    entries: list[tuple[str, str]] = []
    for key, value in config.items("aliases"):
        # Strip surrounding quotes that configparser preserves
        if len(value) >= 2 and value[0] == '"' and value[-1] == '"':
            value = value[1:-1]
        entries.append((key, value))

    return entries


def validate_aliases(entries: list[tuple[str, str]], max_command_len: int | None) -> None:
    """Reject aliases that can never fit into the shell command buffer."""
    if max_command_len is None:
        return

    for alias, cmd in entries:
        if len(cmd) >= max_command_len:
            print(
                f"ERROR: alias {alias!r} expands to {len(cmd)} bytes, "
                f"but CONFIG_SHELL_CMD_BUFF_SIZE is {max_command_len}",
                file=sys.stderr,
            )
            sys.exit(1)


def generate(input_path: str, output_path: str | None, max_command_len: int | None) -> None:
    entries = parse_aliases(input_path)
    validate_aliases(entries, max_command_len)

    input_basename = os.path.basename(input_path)

    def write(out):
        out.write(f"/* Auto-generated from {input_basename} — do not edit */\n\n")
        for alias, cmd in entries:
            c_alias = escape_c_string(alias)
            c_cmd = escape_c_string(cmd)
            out.write(f'{{"{c_alias}", "{c_cmd}"}},\n')

    if output_path is None:
        write(sys.stdout)
    else:
        with open(output_path, "w", encoding="utf-8") as out:
            write(out)
        print(
            f"Generated {len(entries)} alias(es) → {output_path}",
            file=sys.stderr,
        )


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(allow_abbrev=False)
    parser.add_argument("input_file")
    parser.add_argument("output_file", nargs="?")
    parser.add_argument("--max-command-len", type=int)
    return parser.parse_args()


def main() -> None:
    args = parse_args()

    input_path = args.input_file
    output_path = args.output_file

    if not os.path.isfile(input_path):
        print(f"ERROR: input file not found: {input_path!r}", file=sys.stderr)
        sys.exit(1)

    generate(input_path, output_path, args.max_command_len)


if __name__ == "__main__":
    main()
