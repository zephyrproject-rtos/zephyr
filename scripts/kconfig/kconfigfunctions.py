#
# Copyright (c) 2018-2019 Linaro
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import os
import sys

ZEPHYR_BASE = os.environ.get("ZEPHYR_BASE")
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts/dts"))

import edtlib

# Types we support
# 'string', 'int', 'hex', 'bool'

doc_mode = os.environ.get('KCONFIG_DOC_MODE') == "1"

dt_defines = {}
if not doc_mode:
    DTS_POST_CPP = os.environ["DTS_POST_CPP"]
    BINDINGS_DIRS = os.environ.get("DTS_ROOT_BINDINGS")

    # if a board port doesn't use DTS than these might not be set
    if os.path.isfile(DTS_POST_CPP) and BINDINGS_DIRS is not None:
        edt = edtlib.EDT(DTS_POST_CPP, BINDINGS_DIRS.split("?"))
    else:
        edt = None

    # The env var 'GENERATED_DTS_BOARD_CONF' must be set unless we are in
    # doc mode
    GENERATED_DTS_BOARD_CONF = os.environ['GENERATED_DTS_BOARD_CONF']
    if os.path.isfile(GENERATED_DTS_BOARD_CONF):
        with open(GENERATED_DTS_BOARD_CONF, 'r', encoding='utf-8') as fd:
            for line in fd:
                if '=' in line:
                    define, val = line.split('=', 1)
                    dt_defines[define] = val.strip()


def _warn(kconf, msg):
    print("{}:{}: WARNING: {}".format(kconf.filename, kconf.linenr, msg))


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

    _warn(kconf, "dt_int_val is deprecated.")

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

    _warn(kconf, "dt_hex_val is deprecated.")

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

    _warn(kconf, "dt_str_val is deprecated.")

    return dt_defines[name].strip('"')


def dt_chosen_label(kconf, _, chosen):
    """
    This function takes a 'chosen' property and treats that property as a path
    to an EDT node.  If it finds an EDT node, it will look to see if that node
    has a "label" property and return the value of that "label", if not we
    return an empty string.
    """
    if doc_mode or edt is None:
        return ""

    node = edt.chosen_node(chosen)
    if not node:
        return ""

    if "label" not in node.props:
        return ""

    return node.props["label"].val


def _node_reg_addr(node, index, unit):
    if not node:
        return 0

    if not node.regs:
        return 0

    if int(index) >= len(node.regs):
        return 0

    return node.regs[int(index)].addr >> _dt_units_to_scale(unit)


def _node_reg_size(node, index, unit):
    if not node:
        return 0

    if not node.regs:
        return 0

    if int(index) >= len(node.regs):
        return 0

    return node.regs[int(index)].size >> _dt_units_to_scale(unit)


def _dt_chosen_reg_addr(kconf, chosen, index=0, unit=None):
    """
    This function takes a 'chosen' property and treats that property as a path
    to an EDT node.  If it finds an EDT node, it will look to see if that
    nodnode has a register at the given 'index' and return the address value of
    that reg, if not we return 0.

    The function will divide the value based on 'unit':
        None        No division
        'k' or 'K'  divide by 1024 (1 << 10)
        'm' or 'M'  divide by 1,048,576 (1 << 20)
        'g' or 'G'  divide by 1,073,741,824 (1 << 30)
    """
    if doc_mode or edt is None:
        return 0

    node = edt.chosen_node(chosen)

    return _node_reg_addr(node, index, unit)


def _dt_chosen_reg_size(kconf, chosen, index=0, unit=None):
    """
    This function takes a 'chosen' property and treats that property as a path
    to an EDT node.  If it finds an EDT node, it will look to see if that node
    has a register at the given 'index' and return the size value of that reg,
    if not we return 0.

    The function will divide the value based on 'unit':
        None        No division
        'k' or 'K'  divide by 1024 (1 << 10)
        'm' or 'M'  divide by 1,048,576 (1 << 20)
        'g' or 'G'  divide by 1,073,741,824 (1 << 30)
    """
    if doc_mode or edt is None:
        return 0

    node = edt.chosen_node(chosen)

    return _node_reg_size(node, index, unit)


def dt_chosen_reg(kconf, name, chosen, index=0, unit=None):
    """
    This function just routes to the proper function and converts
    the result to either a string int or string hex value.
    """
    if name == "dt_chosen_reg_size_int":
        return str(_dt_chosen_reg_size(kconf, chosen, index, unit))
    if name == "dt_chosen_reg_size_hex":
        return hex(_dt_chosen_reg_size(kconf, chosen, index, unit))
    if name == "dt_chosen_reg_addr_int":
        return str(_dt_chosen_reg_addr(kconf, chosen, index, unit))
    if name == "dt_chosen_reg_addr_hex":
        return hex(_dt_chosen_reg_addr(kconf, chosen, index, unit))


def _dt_node_reg_addr(kconf, path, index=0, unit=None):
    """
    This function takes a 'path' and looks for an EDT node at that path. If it
    finds an EDT node, it will look to see if that node has a register at the
    given 'index' and return the address value of that reg, if not we return 0.

    The function will divide the value based on 'unit':
        None        No division
        'k' or 'K'  divide by 1024 (1 << 10)
        'm' or 'M'  divide by 1,048,576 (1 << 20)
        'g' or 'G'  divide by 1,073,741,824 (1 << 30)
    """
    if doc_mode or edt is None:
        return 0

    try:
        node = edt.get_node(path)
    except edtlib.EDTError:
        return 0

    return _node_reg_addr(node, index, unit)


def _dt_node_reg_size(kconf, path, index=0, unit=None):
    """
    This function takes a 'path' and looks for an EDT node at that path. If it
    finds an EDT node, it will look to see if that node has a register at the
    given 'index' and return the size value of that reg, if not we return 0.

    The function will divide the value based on 'unit':
        None        No division
        'k' or 'K'  divide by 1024 (1 << 10)
        'm' or 'M'  divide by 1,048,576 (1 << 20)
        'g' or 'G'  divide by 1,073,741,824 (1 << 30)
    """
    if doc_mode or edt is None:
        return 0

    try:
        node = edt.get_node(path)
    except edtlib.EDTError:
        return 0

    return _node_reg_size(node, index, unit)


def dt_node_reg(kconf, name, path, index=0, unit=None):
    """
    This function just routes to the proper function and converts
    the result to either a string int or string hex value.
    """
    if name == "dt_node_reg_size_int":
        return str(_dt_node_reg_size(kconf, path, index, unit))
    if name == "dt_node_reg_size_hex":
        return hex(_dt_node_reg_size(kconf, path, index, unit))
    if name == "dt_node_reg_addr_int":
        return str(_dt_node_reg_addr(kconf, path, index, unit))
    if name == "dt_node_reg_addr_hex":
        return hex(_dt_node_reg_addr(kconf, path, index, unit))


def dt_node_has_bool_prop(kconf, _, path, prop):
    """
    This function takes a 'path' and looks for an EDT node at that path. If it
    finds an EDT node, it will look to see if that node has a boolean property
    by the name of 'prop'.  If the 'prop' exists it will return "y" otherwise
    we return "n".
    """
    if doc_mode or edt is None:
        return "n"

    try:
        node = edt.get_node(path)
    except edtlib.EDTError:
        return "n"

    if prop not in node.props:
        return "n"

    if node.props[prop].type != "boolean":
        return "n"

    if node.props[prop].val:
        return "y"

    return "n"


def dt_compat_enabled(kconf, _, compat):
    """
    This function takes a 'compat' and returns "y" if we find an "enabled"
    compatible node in the EDT otherwise we return "n"
    """
    if doc_mode or edt is None:
        return "n"

    for node in edt.nodes:
        if compat in node.compats and node.enabled:
            return "y"

    return "n"


functions = {
        "dt_int_val": (dt_int_val, 1, 2),
        "dt_hex_val": (dt_hex_val, 1, 2),
        "dt_str_val": (dt_str_val, 1, 1),
        "dt_compat_enabled": (dt_compat_enabled, 1, 1),
        "dt_chosen_label": (dt_chosen_label, 1, 1),
        "dt_chosen_reg_addr_int": (dt_chosen_reg, 1, 4),
        "dt_chosen_reg_addr_hex": (dt_chosen_reg, 1, 4),
        "dt_chosen_reg_size_int": (dt_chosen_reg, 1, 4),
        "dt_chosen_reg_size_hex": (dt_chosen_reg, 1, 4),
        "dt_node_reg_addr_int": (dt_node_reg, 1, 4),
        "dt_node_reg_addr_hex": (dt_node_reg, 1, 4),
        "dt_node_reg_size_int": (dt_node_reg, 1, 4),
        "dt_node_reg_size_hex": (dt_node_reg, 1, 4),
        "dt_node_has_bool_prop": (dt_node_has_bool_prop, 2, 2),
}
