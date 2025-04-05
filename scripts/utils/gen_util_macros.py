"""
Utility script to generate headers for the following macros
- Z_LISTIFY

.. note::
    The script will simply create the header files in the current working directory,
    they should be copied manually to $ZEPHYR_BASE/include/zephyr/sys/ accordingly.

Usage::

    python $ZEPHYR_BASE/scripts/utils/gen_util_macros.py -l 4095

Copyright (c) 2023, Meta
SPDX-License-Identifier: Apache-2.0
"""

import argparse
from io import TextIOWrapper

def write_hidden_start(file: TextIOWrapper):
    file.write("/**\n")
    file.write(" * @cond INTERNAL_HIDDEN\n")
    file.write(" */\n")


def write_hidden_stop(file: TextIOWrapper):
    file.write("/**\n")
    file.write(" * INTERNAL_HIDDEN @endcond\n")
    file.write(" */\n")


def write_internal_use_check(file: TextIOWrapper, name: str = "internal"):
    file.write("\n")
    file.write(f"#ifndef ZEPHYR_INCLUDE_SYS_UTIL_{name.upper()}_H_\n")
    file.write(f"#error \"This header should not be used directly, please include util_{name.lower()}.h instead\"\n")
    file.write(f"#endif /* ZEPHYR_INCLUDE_SYS_UTIL_{name.upper()}_H_ */\n")
    file.write("\n")


def gen_util_listify(limit:int):
    with open("util_listify.h", "w") as file:
        write_hidden_start(file)
        write_internal_use_check(file, "loops")

        file.write("#ifndef ZEPHYR_INCLUDE_SYS_UTIL_LISTIFY_H_\n")
        file.write("#define ZEPHYR_INCLUDE_SYS_UTIL_LISTIFY_H_\n")
        file.write("\n")

        file.write("/* Set of UTIL_LISTIFY particles */\n")
        file.write("#define Z_UTIL_LISTIFY_0(F, sep, ...)\n\n")
        file.write("#define Z_UTIL_LISTIFY_1(F, sep, ...) \\\n")
        file.write("	F(0, __VA_ARGS__)\n\n")

        for i in range(2, limit + 3):
            file.write(f"#define Z_UTIL_LISTIFY_{i}(F, sep, ...) \\\n")
            file.write(f"	Z_UTIL_LISTIFY_{i - 1}(F, sep, __VA_ARGS__) __DEBRACKET sep \\\n")
            file.write(f"	F({i - 1}, __VA_ARGS__)\n")

        file.write("\n")
        file.write("#endif /* ZEPHYR_INCLUDE_SYS_UTIL_LISTIFY_H_ */\n")
        file.write("\n")
        write_hidden_stop(file)

def gen_util_expr_bits(limit:int):
    with open("util_expr_bits.h", "w") as file:
        write_hidden_start(file)
        file.write("\n")
        file.write("#ifndef ZEPHYR_INCLUDE_SYS_UTIL_EXPR_NUM_H_\n")
        file.write("#define ZEPHYR_INCLUDE_SYS_UTIL_EXPR_NUM_H_\n")
        file.write("\n")

        for i in range(0, limit + 2):
            file.write(f"#define Z_EXPR_BITS_{i}U " + ", ".join(f"{i:032b}") + "\n")

        file.write("\n")

        for i in range(0, (limit + 2) * 2):
            file.write(f"#define Z_EXPR_BIN_TO_DEC_0B{i:032b} " + f"{i}" + "\n")

        file.write("\n")
        file.write("#endif /* ZEPHYR_INCLUDE_SYS_UTIL_EXPR_NUM_H_ */\n")
        file.write("\n")
        write_hidden_stop(file)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(allow_abbrev=False)
    parser.add_argument(
        "-l", "--limit", type=int, required=True, help="Limit of macros"
    )
    args = parser.parse_args()

    gen_util_listify(args.limit)
    gen_util_expr_bits(args.limit)
