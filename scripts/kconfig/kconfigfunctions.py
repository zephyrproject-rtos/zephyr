#
# Copyright (c) 2018-2019 Linaro
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import os

# Types we support
# 'string', 'int', 'hex', 'bool'

doc_mode = os.environ.get('KCONFIG_DOC_MODE') == "1"

# The env var 'GENERATED_DTS_BOARD_CONF' must be set
GENERATED_DTS_BOARD_CONF = os.environ['GENERATED_DTS_BOARD_CONF']

dt_defines = {}
if (not doc_mode) and os.path.isfile(GENERATED_DTS_BOARD_CONF):
    with open(GENERATED_DTS_BOARD_CONF, 'r', encoding='utf-8') as fd:
        for line in fd:
            if '=' in line:
                define, val = line.split('=')
                dt_defines[define] = val.strip()

def _dt_units_to_scale(unit):
    if not unit:
        return 0
    if unit in {'k', 'K'}:
        return 10
    if unit in {'m', 'M'}:
        return 20
    if unit in {'g', 'G'}:
        return 30

def dt_int_val(kconf, _, name, unit=None):
    """
    This function looks up 'name' in the DTS generated "conf" style database
    (generated_dts_board.conf in <build_dir>/zephyr/include/generated/)
    and if it's found it will return the value as an decimal integer.  The
    function will divide the value based on 'unit':
        None        No division
        'k' or 'K'  divide by 1024 (1 << 10)
        'm' or 'M'  divide by 1,048,576 (1 << 20)
        'g' or 'G'  divide by 1,073,741,824 (1 << 30)
    """
    if doc_mode or name not in dt_defines:
        return "0"

    d = dt_defines[name]
    if d.startswith(('0x', '0X')):
        d = int(d, 16)
    else:
        d = int(d)
    d >>= _dt_units_to_scale(unit)

    return str(d)

def dt_hex_val(kconf, _, name, unit=None):
    """
    This function looks up 'name' in the DTS generated "conf" style database
    (generated_dts_board.conf in <build_dir>/zephyr/include/generated/)
    and if it's found it will return the value as an hex integer.  The
    function will divide the value based on 'unit':
        None        No division
        'k' or 'K'  divide by 1024 (1 << 10)
        'm' or 'M'  divide by 1,048,576 (1 << 20)
        'g' or 'G'  divide by 1,073,741,824 (1 << 30)
    """
    if doc_mode or name not in dt_defines:
        return "0x0"

    d = dt_defines[name]
    if d.startswith(('0x', '0X')):
        d = int(d, 16)
    else:
        d = int(d)
    d >>= _dt_units_to_scale(unit)

    return hex(d)

def dt_str_val(kconf, _, name):
    """
    This function looks up 'name' in the DTS generated "conf" style database
    (generated_dts_board.conf in <build_dir>/zephyr/include/generated/)
    and if it's found it will return the value as string.  If it's not found we
    return an empty string.
    """
    if doc_mode or name not in dt_defines:
        return ""

    return dt_defines[name].strip('"')

functions = {
        "dt_int_val": (dt_int_val, 1, 2),
        "dt_hex_val": (dt_hex_val, 1, 2),
        "dt_str_val": (dt_str_val, 1, 1),
}
