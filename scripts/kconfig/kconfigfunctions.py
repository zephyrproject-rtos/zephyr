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

if not doc_mode:
    DTS_POST_CPP = os.environ["DTS_POST_CPP"]
    BINDINGS_DIRS = os.environ.get("DTS_ROOT_BINDINGS")
    DTC_FLAGS = os.environ.get("EXTRA_DTC_FLAGS")

    if DTC_FLAGS is not None and "-Wno-simple_bus_reg" in DTC_FLAGS:
        edt_warn_flag = False
    else:
        edt_warn_flag = True

    # if a board port doesn't use DTS than these might not be set
    if os.path.isfile(DTS_POST_CPP) and BINDINGS_DIRS is not None:
        edt = edtlib.EDT(DTS_POST_CPP, BINDINGS_DIRS.split("?"),
                warn_reg_unit_address_mismatch=edt_warn_flag)
    else:
        edt = None


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


def dt_chosen_enabled(kconf, _, chosen):
    """
    This function returns "y" if /chosen contains a property named 'chosen'
    that points to an enabled node, and "n" otherwise
    """
    if doc_mode or edt is None:
        return "n"

    node = edt.chosen_node(chosen)
    return "y" if node and node.enabled else "n"


def dt_chosen_path(kconf, _, chosen):
    """
    This function takes a /chosen node property and returns the path
    to the node in the property value, or the empty string.
    """
    if doc_mode or edt is None:
        return "n"

    node = edt.chosen_node(chosen)

    return node.path if node else ""


def dt_nodelabel_enabled(kconf, _, label):
    """
    This function takes a 'label' and returns "y" if we find an "enabled"
    node that has a 'nodelabel' of 'label' in the EDT otherwise we return "n"
    """
    if doc_mode or edt is None:
        return "n"

    node = edt.label2node.get(label)

    return "y" if node and node.enabled else "n"


def dt_alias_enabled(kconf, _, alias):
    """
    This function takes an /aliases property 'alias' and returns "y" if we find
    that alias is defined and points to an "enabled" node in the EDT;
    otherwise, we return "n"
    """
    if doc_mode or edt is None:
        return "n"

    node = edt.alias2node.get(alias)

    return "y" if node and node.enabled else "n"


def _node_reg_addr(node, index, unit):
    if not node:
        return 0

    if not node.regs:
        return 0

    if int(index) >= len(node.regs):
        return 0

    if node.regs[int(index)].addr is None:
        return 0

    return node.regs[int(index)].addr >> _dt_units_to_scale(unit)


def _node_reg_size(node, index, unit):
    if not node:
        return 0

    if not node.regs:
        return 0

    if int(index) >= len(node.regs):
        return 0

    if node.regs[int(index)].size is None:
        return 0

    return node.regs[int(index)].size >> _dt_units_to_scale(unit)


def _node_int_prop(node, prop):
    if not node:
        return 0

    if prop not in node.props:
        return 0

    if node.props[prop].type != "int":
        return 0

    return node.props[prop].val


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


def dt_node_int_prop(kconf, name, path, prop):
    """
    This function takes a 'path' and property name ('prop') looks for an EDT
    node at that path. If it finds an EDT node, it will look to see if that
    node has a property called 'prop' and if that 'prop' is an integer type
    will return the value of the property 'prop' as either a string int or
    string hex value, if not we return 0.
    """

    if doc_mode or edt is None:
        return "0"

    try:
        node = edt.get_node(path)
    except edtlib.EDTError:
        return "0"

    if name == "dt_node_int_prop_int":
        return str(_node_int_prop(node, prop))
    if name == "dt_node_int_prop_hex":
        return hex(_node_int_prop(node, prop))


def dt_compat_enabled(kconf, _, compat):
    """
    This function takes a 'compat' and returns "y" if we find an "enabled"
    compatible node in the EDT otherwise we return "n"
    """
    if doc_mode or edt is None:
        return "n"

    return "y" if compat in edt.compat2enabled else "n"


def dt_compat_on_bus(kconf, _, compat, bus):
    """
    This function takes a 'compat' and returns "y" if we find an "enabled"
    compatible node in the EDT which is on bus 'bus'. It returns "n" otherwise.
    """
    if doc_mode or edt is None:
        return "n"

    for node in edt.compat2enabled[compat]:
        if node.on_bus is not None and node.on_bus == bus:
            return "y"

    return "n"


def dt_nodelabel_has_compat(kconf, _, label, compat):
    """
    This function takes a 'label' and returns "y" if an "enabled" node with
    such label can be found in the EDT and that node is compatible with the
    provided 'compat', otherwise it returns "n".
    """
    if doc_mode or edt is None:
        return "n"

    for node in edt.compat2enabled[compat]:
        if label in node.labels:
            return "y"

    return "n"


def dt_nodelabel_path(kconf, _, label):
    """
    This function takes a node label (not a label property) and
    returns the path to the node which has that label, or an empty
    string if there is no such node.
    """
    if doc_mode or edt is None:
        return ""

    node = edt.label2node.get(label)

    return node.path if node else ""


def shields_list_contains(kconf, _, shield):
    """
    Return "n" if cmake environment variable 'SHIELD_AS_LIST' doesn't exist.
    Return "y" if 'shield' is present list obtained after 'SHIELD_AS_LIST'
    has been split using ";" as a separator and "n" otherwise.
    """
    try:
        list = os.environ['SHIELD_AS_LIST']
    except KeyError:
        return "n"

    return "y" if shield in list.split(";") else "n"


functions = {
        "dt_compat_enabled": (dt_compat_enabled, 1, 1),
        "dt_compat_on_bus": (dt_compat_on_bus, 2, 2),
        "dt_chosen_label": (dt_chosen_label, 1, 1),
        "dt_chosen_enabled": (dt_chosen_enabled, 1, 1),
        "dt_chosen_path": (dt_chosen_path, 1, 1),
        "dt_nodelabel_enabled": (dt_nodelabel_enabled, 1, 1),
        "dt_alias_enabled": (dt_alias_enabled, 1, 1),
        "dt_chosen_reg_addr_int": (dt_chosen_reg, 1, 3),
        "dt_chosen_reg_addr_hex": (dt_chosen_reg, 1, 3),
        "dt_chosen_reg_size_int": (dt_chosen_reg, 1, 3),
        "dt_chosen_reg_size_hex": (dt_chosen_reg, 1, 3),
        "dt_node_reg_addr_int": (dt_node_reg, 1, 3),
        "dt_node_reg_addr_hex": (dt_node_reg, 1, 3),
        "dt_node_reg_size_int": (dt_node_reg, 1, 3),
        "dt_node_reg_size_hex": (dt_node_reg, 1, 3),
        "dt_node_has_bool_prop": (dt_node_has_bool_prop, 2, 2),
        "dt_node_int_prop_int": (dt_node_int_prop, 2, 2),
        "dt_node_int_prop_hex": (dt_node_int_prop, 2, 2),
        "dt_nodelabel_has_compat": (dt_nodelabel_has_compat, 2, 2),
        "dt_nodelabel_path": (dt_nodelabel_path, 1, 1),
        "shields_list_contains": (shields_list_contains, 1, 1),
}
