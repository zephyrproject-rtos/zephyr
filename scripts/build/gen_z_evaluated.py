#!/usr/bin/env python3
#
# Copyright (c) 2018 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

"""
Script to bridge the gap between ld-syntax expressions and IAR Ilink capabilities

This works by leveraging the multiple linker passes; for PASS i
the zephyr_linker_XXXX() tags things with @IAR_EVALUATE@ that needs special
treatment (e.g. MPU_ALIGN(region_size)). This is picked up by
config_file_script.cmake during @-variable expansion, and puts it into a file
(iar_evaluate_PASS.json).

This script reads iar_evaluate_PASSi.json and the corresponding
zephyr_PASSi.elf file to evaluate the required expressions. The results
are put into a .cmake file that defines the required @IAR_EVALUATED_@ variables.
This file is then zephyr_linker_include_generated() into PASSi+1.

The grand plan is thus: open the elf file to gather all symbol values.
Iterate over all of the elements in the json, evaluate the expressions and if
it succeeds put the resulting @IAR_EVALUATED@ thig into the cmake.
"""

import argparse
import json
import math
import re

import elftools.common.exceptions
from elftools.elf.elffile import ELFFile
from elftools.elf.sections import SymbolTableSection


def cmake_list_append(list, props):
    tokens = [rf"{key}\;{value}" for key, value in props.items()]
    stuff = r"\;".join(tokens)
    return """list(APPEND """ + list + """ \"{""" + stuff + """}\")\n"""


def zephyr_linker_include_var(var, value):
    props = {"""VAR""": var, """VALUE""": value}
    return cmake_list_append("VARIABLES", props)


def parse_elf_file(file):
    symbols = {}
    with open(file, 'rb') as f:
        try:
            elffile = ELFFile(f)
        except elftools.common.exceptions.ELFError as e:
            exit(f"Error: {args.elf}: {e}")

        symbol_tbls = [s for s in elffile.iter_sections() if isinstance(s, SymbolTableSection)]

        for section in symbol_tbls:
            for symbol in section.iter_symbols():
                if symbol['st_shndx'] == "SHN_UNDEF":
                    continue
                if symbol['st_info']['bind'] != 'STB_GLOBAL':
                    continue

                symbols[symbol.name] = symbol['st_value']
        return symbols


def parse_input_file(name):
    expressions = {}
    with open(name, 'rb') as f:
        all_exprs = json.load(f)

        for expr in all_exprs:
            if expr['name'] not in expressions:
                expressions[expr['name']] = expr['expression']
    return expressions


# define some ld-expression functions:
# There are a bunch of them that require info about sections and segments
# let's ignore them for now...
def MAX(a, b):
    return max(a, b)


def MIN(a, b):
    return min(a, b)


def LOG2CEIL(x):
    if x <= 0:
        raise ValueError("LOG2CEIL is not defined for non-positive values")
    return math.ceil(math.log2(x))


def evaluate_expressions(expressions, symbols):
    variables = {}
    for name, expr in expressions.items():
        # Replace symbol names with their values
        for symbol, value in symbols.items():
            expr = re.sub(r'\b' + symbol + r'\b', str(value), expr)
        value = eval(expr)
        variables[name] = value
    return variables


def generate_cmake_output(variables, output):
    with open(output, 'w') as f:
        for name, value in variables.items():
            f.write(zephyr_linker_include_var(name, value))


def parse_args():
    global args
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False,
    )
    parser.add_argument("-e", "--elf", required=True, default=None, help="ELF file")
    parser.add_argument("-i", "--input", required=True, help="input json expressions file")
    parser.add_argument("-o", "--output", required=True, help="Output cmake file file")
    parser.add_argument("-v", "--verbose", action="count", default=0, help="Verbose Output")

    args = parser.parse_args()


def main():
    parse_args()

    symbols = parse_elf_file(args.elf)
    if args.verbose:
        print(f"Symbols: {symbols}")
    expressions = parse_input_file(args.input)
    variables = evaluate_expressions(expressions, symbols)
    generate_cmake_output(variables, args.output)


if __name__ == '__main__':
    main()
