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
        edt = edtlib.EDT(DTS_POST_CPP, BINDINGS_DIRS.split(";"))
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
    to a EDT device.  If it finds a EDT device, it will look to see if that
    device has a "label" property and return the value of that "label", if not
    we return an empty string.
    """
    if doc_mode or edt is None:
        return ""

    dev = edt.chosen_dev(chosen)
    if not dev:
        return ""

    if "label" not in dev.props:
        return ""

    return dev.props["label"].val


def _dev_reg_addr(dev, index, unit):
    if not dev:
        return "0x0"

    if not dev.regs:
        return "0x0"

    if int(index) >= len(dev.regs):
        return "0x0"

    return hex(dev.regs[int(index)].addr >> _dt_units_to_scale(unit))


def _dev_reg_size(dev, index, unit):
    if not dev:
        return "0"

    if not dev.regs:
        return "0"

    if int(index) >= len(dev.regs):
        return "0"

    return str(dev.regs[int(index)].size >> _dt_units_to_scale(unit))


def dt_chosen_reg_addr(kconf, _, chosen, index=0, unit=None):
    """
    This function takes a 'chosen' property and treats that property as a path
    to a EDT device.  If it finds a EDT device, it will look to see if that
    device has a register at the give 'index' and return the address value of
    that reg, if not we return 0.

    The function will divide the value based on 'unit':
        None        No division
        'k' or 'K'  divide by 1024 (1 << 10)
        'm' or 'M'  divide by 1,048,576 (1 << 20)
        'g' or 'G'  divide by 1,073,741,824 (1 << 30)
    """
    if doc_mode or edt is None:
        return "0x0"

    dev = edt.chosen_dev(chosen)

    return _dev_reg_addr(dev, index, unit)


def dt_chosen_reg_size(kconf, _, chosen, index=0, unit=None):
    """
    This function takes a 'chosen' property and treats that property as a path
    to a EDT device.  If it finds a EDT device, it will look to see if that
    device has a register at the give 'index' and return the size value of
    that reg, if not we return 0.

    The function will divide the value based on 'unit':
        None        No division
        'k' or 'K'  divide by 1024 (1 << 10)
        'm' or 'M'  divide by 1,048,576 (1 << 20)
        'g' or 'G'  divide by 1,073,741,824 (1 << 30)
    """
    if doc_mode or edt is None:
        return "0"

    dev = edt.chosen_dev(chosen)

    return _dev_reg_size(dev, index, unit)


def dt_node_reg_addr(kconf, _, path, index=0, unit=None):
    """
    This function takes a 'path' and looks for a EDT device at that path.
    If it finds a EDT device, it will look to see if that device has a
    register at the give 'index' and return the address value of that reg, if
    not we return 0.

    The function will divide the value based on 'unit':
        None        No division
        'k' or 'K'  divide by 1024 (1 << 10)
        'm' or 'M'  divide by 1,048,576 (1 << 20)
        'g' or 'G'  divide by 1,073,741,824 (1 << 30)
    """
    if doc_mode or edt is None:
        return "0"

    try:
        dev = edt.get_dev(path)
    except edtlib.EDTError:
        return "0"

    return _dev_reg_addr(dev, index, unit)


def dt_node_reg_size(kconf, _, path, index=0, unit=None):
    """
    This function takes a 'path' and looks for a EDT device at that path.
    If it finds a EDT device, it will look to see if that device has a
    register at the give 'index' and return the size value of that reg, if
    not we return 0.

    The function will divide the value based on 'unit':
        None        No division
        'k' or 'K'  divide by 1024 (1 << 10)
        'm' or 'M'  divide by 1,048,576 (1 << 20)
        'g' or 'G'  divide by 1,073,741,824 (1 << 30)
    """
    if doc_mode or edt is None:
        return "0"

    try:
        dev = edt.get_dev(path)
    except edtlib.EDTError:
        return "0"

    return _dev_reg_size(dev, index, unit)


def dt_node_has_bool_prop(kconf, _, path, prop):
    """
    This function takes a 'path' and looks for a EDT device at that path.
    If it finds a EDT device, it will look to see if that device has a
    boolean property by the name of 'prop'.  If the 'prop' exists it will
    return "y" otherwise we return "n".
    """
    if doc_mode or edt is None:
        return "n"

    try:
        dev = edt.get_dev(path)
    except edtlib.EDTError:
        return "n"

    if prop not in dev.props:
        return "n"

    if dev.props[prop].type != "boolean":
        return "n"

    if dev.props[prop].val:
        return "y"

    return "n"


def dt_compat_enabled(kconf, _, compat):
    """
    This function takes a 'compat' and returns "y" if we find an "enabled"
    compatible device in the EDT otherwise we return "n"
    """
    if doc_mode or edt is None:
        return "n"

    for dev in edt.devices:
        if compat in dev.compats and dev.enabled:
            return "y"

    return "n"


functions = {
        "dt_int_val": (dt_int_val, 1, 2),
        "dt_hex_val": (dt_hex_val, 1, 2),
        "dt_str_val": (dt_str_val, 1, 1),
        "dt_compat_enabled": (dt_compat_enabled, 1, 1),
        "dt_chosen_label": (dt_chosen_label, 1, 1),
        "dt_chosen_reg_addr": (dt_chosen_reg_addr, 1, 3),
        "dt_chosen_reg_size": (dt_chosen_reg_size, 1, 3),
        "dt_node_reg_addr": (dt_node_reg_addr, 1, 3),
        "dt_node_reg_size": (dt_node_reg_size, 1, 3),
        "dt_node_has_bool_prop": (dt_node_has_bool_prop, 2, 2),
}
