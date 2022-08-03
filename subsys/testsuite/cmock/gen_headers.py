#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2022 Martin Schröder
import re
import argparse


def gen_headers(header, output, output_wrap):
    with open(header) as header_file:
        content = header_file.read()

    # Change static inline declarations to normal declarations
    pattern = re.compile(
        r"(?:__deprecated\s+|static\s+inline\s+)?(?:static\s+inline\s+|static\s+ALWAYS_INLINE\s+|__deprecated\s+)"
        r"((?:\w+[*\s]+)+\w+?\(.*?\))\n\{.+?\n\}",
        re.M | re.S,
    )
    content = pattern.sub(r"\1;", content)

    # Remove externs, but only function externs (there are extern variables as well)
    pattern = re.compile(r"extern\s+((?:\w+[*\s]+)+\w+?\(.*?\);)", re.M | re.S)
    content = pattern.sub(r"\1", content)

    # Delete anything that matches these patterns completely
    patterns = [
        r"/\*("  # group match for 'any number of'
        r"[^*]|"  # any number of non '*'
        r"[\r\n]|"  # any number of return or new lines
        r"(\*+([^*/]|[\r\n]))"  # any number of ('*' followed by not '*/') or (newlines)
        r")*"  # end group for any number of
        r"\*+/",  # ending with any number of '*' followed by slash '/'
    ]
    for pattern in patterns:
        pattern = re.compile(pattern, re.M | re.S)
        content = pattern.sub(r"", content)

    # Single line patterns
    patterns = [
        # Remove C++ comments
        r"//.*[\r\n]",
        # Remove syscalls because we will have duplicate declarations there
        r"#include <syscalls/\w+?.h>",
        # Remove attributes that mess up mock generation
        r"__syscall\s+",
        r"FUNC_NORETURN\s+",
        r"static\s+inline\s+",
        r"static\s+ALWAYS_INLINE\s+",
        r"__STATIC_INLINE\s+",
        r"inline\s+static\s+",
        r"__attribute_const__\s+",
        r"__deprecated\s+",
        r"__printf_like\(.*\)",
        r"BUILD_ASSERT\(.*\);",
    ]
    for pattern in patterns:
        pattern = re.compile(pattern)
        content = pattern.sub(r"", content)

    # Treat as always enabled because otherwise futex mocks fail to compile
    pattern = re.compile(r"ifdef CONFIG_USERSPACE\s+", re.M | re.S)
    content = pattern.sub("if 1\n", content)

    with open(output, "w") as output_file:
        output_file.write(content)

    # Prefix all function names with __wrap_ prefix
    pattern = re.compile(r"^\s*((?:\w+[*\s]+)+)(\w+?\s*\([^\\{}#]*?\)\s*;)", re.M)
    content = pattern.sub(r"\n\1__wrap_\2", content)

    with open(output_wrap, "w") as output_file:
        output_file.write(content)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-i", "--input", type=str, help="Input header file", required=True
    )
    parser.add_argument(
        "-o",
        "--output",
        type=str,
        help="Cleaned up header file to be included in the unit test",
        required=True,
    )
    parser.add_argument(
        "-w",
        "--wrap",
        type=str,
        help="Header with __wrap_ prefixes for mock generation",
        required=True,
    )
    args = parser.parse_args()

    gen_headers(args.input, args.output, args.wrap)
