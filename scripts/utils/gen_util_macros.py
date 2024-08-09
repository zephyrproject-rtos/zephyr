"""
Utility script to generate headers for the following macros
- Z_LISTIFY
- Z_UTIL_INC
- Z_UTIL_DEC
- Z_UTIL_X2
- Z_IS_EQ

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
from typing import Callable, List

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


def gen_util_internal_is_eq(limit: int):
    with open("util_internal_is_eq.h", "w") as file:
        write_hidden_start(file)
        write_internal_use_check(file)

        file.write("#ifndef ZEPHYR_INCLUDE_SYS_UTIL_INTERNAL_IS_EQ_H_\n")
        file.write("#define ZEPHYR_INCLUDE_SYS_UTIL_INTERNAL_IS_EQ_H_\n")
        file.write("\n")

        for i in range(0, limit + 1):
            file.write(f"#define Z_IS_{i}_EQ_{i}(...) \\,\n")

        file.write("\n")
        file.write("#endif /* ZEPHYR_INCLUDE_SYS_UTIL_INTERNAL_IS_EQ_H_ */\n")
        file.write("\n")
        write_hidden_stop(file)


def gen_util_internal_util_inc(limit: int):
    with open("util_internal_util_inc.h", "w") as file:
        write_hidden_start(file)
        write_internal_use_check(file)

        file.write("#ifndef ZEPHYR_INCLUDE_SYS_UTIL_INTERNAL_UTIL_INC_H_\n")
        file.write("#define ZEPHYR_INCLUDE_SYS_UTIL_INTERNAL_UTIL_INC_H_\n")
        file.write("\n")

        for i in range(0, limit + 2):
            file.write(f"#define Z_UTIL_INC_{i} {i + 1}\n")

        file.write("\n")
        file.write("#endif /* ZEPHYR_INCLUDE_SYS_UTIL_INTERNAL_UTIL_INC_H_ */\n")
        file.write("\n")
        write_hidden_stop(file)


def gen_util_internal_util_dec(limit: int):
    with open("util_internal_util_dec.h", "w") as file:
        write_hidden_start(file)
        write_internal_use_check(file)

        file.write("#ifndef ZEPHYR_INCLUDE_SYS_UTIL_INTERNAL_UTIL_DEC_H_\n")
        file.write("#define ZEPHYR_INCLUDE_SYS_UTIL_INTERNAL_UTIL_DEC_H_\n")
        file.write("\n")

        file.write(f"#define Z_UTIL_DEC_0 0\n")
        for i in range(1, limit + 2):
            file.write(f"#define Z_UTIL_DEC_{i} {i - 1}\n")

        file.write("\n")
        file.write("#endif /* ZEPHYR_INCLUDE_SYS_UTIL_INTERNAL_UTIL_DEC_H_ */\n")
        file.write("\n")
        write_hidden_stop(file)


def gen_util_internal_util_x2(limit: int):
    with open("util_internal_util_x2.h", "w") as file:
        write_hidden_start(file)
        write_internal_use_check(file)

        file.write("#ifndef ZEPHYR_INCLUDE_SYS_UTIL_INTERNAL_UTIL_X2_H_\n")
        file.write("#define ZEPHYR_INCLUDE_SYS_UTIL_INTERNAL_UTIL_X2_H_\n")
        file.write("\n")

        for i in range(0, limit + 1):
            file.write(f"#define Z_UTIL_X2_{i} {i *2}\n")

        file.write("\n")
        file.write("#endif /* ZEPHYR_INCLUDE_SYS_UTIL_INTERNAL_UTIL_X2_H_ */\n")
        file.write("\n")
        write_hidden_stop(file)


def gen_util_internal_number_to_bits(limit: int):
    with open("util_internal_number_to_bits.h", "w") as file:
        write_hidden_start(file)
        write_internal_use_check(file, "internal_math")

        file.write("#ifndef ZEPHYR_INCLUDE_SYS_UTIL_INTERNAL_NUMBER_TO_BITS_H_\n")
        file.write("#define ZEPHYR_INCLUDE_SYS_UTIL_INTERNAL_NUMBER_TO_BITS_H_\n")
        file.write("\n")

        next_power_of_2: Callable[[int], int] = lambda x: 1 << (x - 1).bit_length()
        _limit = next_power_of_2(limit)
        _n_bits = (_limit - 1).bit_length()

        file.write("#define Z_DO_NUMBER_TO_BITS(n) Z_NUMBER_TO_BITS_##n\n")
        file.write("#define Z_NUMBER_TO_BITS(n) Z_DO_NUMBER_TO_BITS(n)\n")
        file.write("\n")

        _macro_args: List[str] = ['_'+str(i) for i in range(_n_bits)[::-1]]
        file.write(f"#define Z_DO_BITS_TO_NUMBER({', '.join(_macro_args)}) "
                   f"Z_BITS_TO_NUMBER_##{'##'.join(_macro_args)}\n")
        file.write("#define Z_BITS_TO_NUMBER(b) Z_DO_BITS_TO_NUMBER b\n")
        file.write("\n")
        file.write(f"#define Z_DO_REVERSE_BITS({', '.join(_macro_args)}) "
                   f"({', '.join(_macro_args[::-1])})\n")
        file.write("#define Z_REVERSE_BITS(b) Z_DO_REVERSE_BITS b\n")
        file.write("\n")

        file.write("#define Z_GET_BIT_N(N, ...) Z_GET_BIT_##N(__VA_ARGS__)\n")
        _macro_args = []
        for i in range(_n_bits)[::-1]:
            _macro_args += [f'_{i}']
            file.write(f"#define Z_GET_BIT_{i}({', '.join(_macro_args)}, ...) _{i}\n")
        file.write("\n")

        for i in range(_limit):
            bits = bin(i)[2:].rjust(_n_bits, '0')
            file.write(f"#define Z_NUMBER_TO_BITS_{i} ({', '.join(bits)})\n")
            file.write(f"#define Z_BITS_TO_NUMBER_{bits} {i}\n")

        file.write("\n")
        file.write("#endif /* ZEPHYR_INCLUDE_SYS_UTIL_INTERNAL_NUMBER_TO_BITS_H_ */\n")
        file.write("\n")
        write_hidden_stop(file)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(allow_abbrev=False)
    parser.add_argument(
        "-l", "--limit", type=int, required=True, help="Limit of macros"
    )
    args = parser.parse_args()

    gen_util_listify(args.limit)
    gen_util_internal_is_eq(args.limit)
    gen_util_internal_util_inc(args.limit)
    gen_util_internal_util_dec(args.limit)
    gen_util_internal_util_x2(args.limit)
    gen_util_internal_number_to_bits(args.limit)
