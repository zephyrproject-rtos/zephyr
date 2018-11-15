#
# Copyright (c) 2018-2019 Linaro
#
# SPDX-License-Identifier: Apache-2.0

import sys
import os
import re

# Types we support
# 'string', 'int', 'hex', 'bool'
conf_file = os.path.join(os.environ['PROJECT_BINARY_DIR'], "include/generated/generated_dts_board.conf")

regex = '='
defines = {}
if os.path.isfile(conf_file):
    for line in open(conf_file, 'r', encoding='utf-8'):
        if re.search(regex, line):
            c = line.split('=')
            defines[c[0]] = c[1].strip()

def dt_get_str_val(kconf, name, arg):
    return defines[name]

def _dt_units_to_scale(unit):
    if not unit:
        return 0
    if unit == 'k' or unit == 'K':
        return 10
    if unit == 'm' or unit == 'M':
        return 20
    if unit == 'g' or unit == 'g':
        return 30

def dt_get_int_val(kconf, name, arg, unit=None):
    try:
        d = defines[arg]
        if d.startswith('0x') or d.startswith('0x'):
            d = int(d, 16)
        else:
            d = int(d)
        d = d >> _dt_units_to_scale(unit)

    except:
        d = 0
    return str(d)

def dt_get_hex_val(kconf, name, arg, unit=None):
    try:
        d = defines[arg]
        if d.startswith('0x') or d.startswith('0x'):
            d = int(d, 16)
        else:
            d = int(d)
        d = d >> _dt_units_to_scale(unit)
        d = hex(d)

    except:
        d = 0x0
    return str(d)


functions = {
        "test_function": (dt_get_str_val, 1, 1),
        "dt_get_int_val": (dt_get_int_val, 1, 2),
        "dt_get_hex_val": (dt_get_hex_val, 1, 2),
}
