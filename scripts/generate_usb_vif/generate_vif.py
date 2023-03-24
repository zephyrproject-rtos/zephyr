#!/usr/bin/env python3

# Copyright (c) 2022 The Chromium OS Authors
# SPDX-License-Identifier: Apache-2.0

"""This file contains a Python script which parses through Zephyr device tree using
EDT.pickle generated at build and generates a XML file containing USB VIF policies"""

import argparse
import inspect
import os
import pickle
import sys
import xml.etree.ElementTree as ET

import constants

SCRIPTS_DIR = os.path.join(os.path.dirname(__file__), "..")
sys.path.insert(0, os.path.join(SCRIPTS_DIR, 'dts', 'python-devicetree', 'src'))

def main():
    global edtlib

    args = parse_args()
    with open(args.edt_pickle, 'rb') as f:
        edt = pickle.load(f)
    edtlib = inspect.getmodule(edt)

    xml_root = get_root()
    add_elements_to_xml(xml_root, constants.VIF_SPEC_ELEMENTS)
    add_element_to_xml(xml_root, constants.MODEL_PART_NUMBER, args.board)
    for node in edt.compat2nodes[args.compatible]:
        xml_ele = add_element_to_xml(xml_root, constants.COMPONENT)
        parse_and_add_node_to_xml(xml_ele, node)
    tree = ET.ElementTree(xml_root)
    tree.write(args.vif_out, xml_declaration=True,
               encoding=constants.XML_ENCODING)


def is_vif_element(name):
    if name in constants.VIF_ELEMENTS:
        return True
    return False


def get_vif_element_name(name):
    return constants.XML_ELEMENT_NAME_PREFIX + ":" + constants.DT_VIF_ELEMENTS.get(
        name, name)


def get_root():
    xml_root = ET.Element(get_vif_element_name(constants.XML_ROOT_ELEMENT_NAME))
    add_attributes_to_xml_element(xml_root, constants.XML_NAMESPACE_ATTRIBUTES)
    return xml_root


def add_attributes_to_xml_element(xml_ele, attributes):
    for key, value in attributes.items():
        xml_ele.set(key, value)


def add_element_to_xml(xml_ele, name, text=None, attributes=None):
    if is_vif_element(name):
        new_xml_ele = ET.SubElement(xml_ele, get_vif_element_name(name))
        if text:
            new_xml_ele.text = str(text)
        if attributes:
            add_attributes_to_xml_element(new_xml_ele, attributes)
        return new_xml_ele
    return xml_ele


def add_elements_to_xml(xml_ele, elements):
    for element_name in elements:
        text = elements[element_name].get(constants.TEXT, None)
        attributes = elements[element_name].get(constants.ATTRIBUTES, None)
        new_xml_ele = add_element_to_xml(xml_ele, element_name, text, attributes)
        if constants.CHILD in elements[element_name]:
            add_elements_to_xml(new_xml_ele, elements[element_name][constants.CHILD])


def is_simple_datatype(value):
    if isinstance(value, (str, int, bool)):
        return True
    return False


def get_pdo_type(pdo_value):
    return pdo_value >> 30


def get_xml_bool_value(value):
    if value:
        return constants.TRUE
    return constants.FALSE


def parse_and_add_sink_pdos_to_xml(xml_ele, sink_pdos):
    new_xml_ele = add_element_to_xml(xml_ele, constants.SINK_PDOS)
    pdos_info = dict()
    snk_max_power = 0
    for pdo_value in sink_pdos:
        power_mv = parse_and_add_sink_pdo_to_xml(new_xml_ele, pdo_value, pdos_info)
        if power_mv > snk_max_power:
            snk_max_power = power_mv
    add_element_to_xml(xml_ele, constants.NUM_SINK_PDOS, None,
                       {constants.VALUE: str(len(sink_pdos))})
    add_element_to_xml(xml_ele, constants.EPR_SUPPORTED_AS_SINK,
                       attributes={constants.VALUE: constants.FALSE})
    add_element_to_xml(xml_ele, constants.NO_USB_SUSPEND_MAY_BE_SET,
                       attributes={constants.VALUE: constants.TRUE})
    add_element_to_xml(xml_ele, constants.HIGHER_CAPABILITY_SET, attributes={
        constants.VALUE: get_xml_bool_value(pdos_info.get(constants.HIGHER_CAPABILITY_SET, 0))})
    add_element_to_xml(xml_ele, constants.FR_SWAP_REQD_TYPE_C_CURRENT_AS_INITIAL_SOURCE,
                       "FR_Swap not supported", attributes={constants.VALUE: str(
            pdos_info.get(constants.FR_SWAP_REQD_TYPE_C_CURRENT_AS_INITIAL_SOURCE, 0))})
    add_element_to_xml(xml_ele, constants.PD_POWER_AS_SINK, f'{snk_max_power} mW',
                       {constants.VALUE: str(snk_max_power)})


def parse_and_add_sink_pdo_to_xml(xml_ele, pdo_value, pdos_info):
    power_mw = 0
    xml_ele = add_element_to_xml(xml_ele, constants.SINK_PDO)
    pdo_type = get_pdo_type(pdo_value)
    if pdo_type == constants.PDO_TYPE_FIXED:
        current = pdo_value & 0x3ff
        current_ma = current * 10
        voltage = (pdo_value >> 10) & 0x3ff
        voltage_mv = voltage * 50
        power_mw = (current_ma * voltage_mv) // 1000
        pdos_info[constants.HIGHER_CAPABILITY_SET] = pdo_value & (1 << 28)
        pdos_info[constants.FR_SWAP_REQD_TYPE_C_CURRENT_AS_INITIAL_SOURCE] = pdo_value & (3 << 23)
        add_element_to_xml(xml_ele, constants.SINK_PDO_VOLTAGE, f'{voltage_mv} mV',
                           {constants.VALUE: str(voltage)})
        add_element_to_xml(xml_ele, constants.SINK_PDO_OP_CURRENT,
                           f'{current_ma} mA',
                           {constants.VALUE: str(current)})
    elif pdo_type == constants.PDO_TYPE_BATTERY:
        max_voltage = (pdo_value >> 20) & 0x3ff
        max_voltage_mv = max_voltage * 50
        min_voltage = (pdo_value >> 10) & 0x3ff
        min_voltage_mv = min_voltage * 50
        power = pdo_value & 0x3ff
        power_mw = power * 250
        add_element_to_xml(xml_ele, constants.SINK_PDO_MIN_VOLTAGE,
                           f'{min_voltage_mv} mV',
                           {constants.VALUE: str(min_voltage)})
        add_element_to_xml(xml_ele, constants.SINK_PDO_MAX_VOLTAGE,
                           f'{max_voltage_mv} mV',
                           {constants.VALUE: str(max_voltage)})
        add_element_to_xml(xml_ele, constants.SINK_PDO_OP_POWER, f'{power_mw} mW',
                           {constants.VALUE: str(power)})
    elif pdo_type == constants.PDO_TYPE_VARIABLE:
        max_voltage = (pdo_value >> 20) & 0x3ff
        max_voltage_mv = max_voltage * 50
        min_voltage = (pdo_value >> 10) & 0x3ff
        min_voltage_mv = min_voltage * 50
        current = pdo_value & 0x3ff
        current_ma = current * 10
        power_mw = (current_ma * max_voltage_mv) // 1000
        add_element_to_xml(xml_ele, constants.SINK_PDO_MIN_VOLTAGE,
                           f'{min_voltage_mv} mV',
                           {constants.VALUE: str(min_voltage)})
        add_element_to_xml(xml_ele, constants.SINK_PDO_MAX_VOLTAGE,
                           f'{max_voltage_mv} mV',
                           {constants.VALUE: str(max_voltage)})
        add_element_to_xml(xml_ele, constants.SINK_PDO_OP_CURRENT,
                           f'{current_ma} mA',
                           {constants.VALUE: str(current)})
    elif pdo_type == constants.PDO_TYPE_AUGUMENTED:
        pps = (pdo_value >> 28) & 0x03
        if pps:
            raise ValueError(f'ERROR: Invalid PDO_TYPE {pdo_value}')
        pps_max_voltage = (pdo_value >> 17) & 0xff
        pps_max_voltage_mv = pps_max_voltage * 100
        pps_min_voltage = (pdo_value >> 8) & 0xff
        pps_min_voltage_mv = pps_min_voltage * 100
        pps_current = pdo_value & 0x7f
        pps_current_ma = pps_current * 50
        power_mw = (pps_current_ma * pps_max_voltage_mv) // 1000
        add_element_to_xml(xml_ele, constants.SINK_PDO_MIN_VOLTAGE,
                           f'{pps_min_voltage_mv} mV',
                           {constants.VALUE: str(pps_min_voltage)})
        add_element_to_xml(xml_ele, constants.SINK_PDO_MAX_VOLTAGE,
                           f'{pps_max_voltage_mv} mV',
                           {constants.VALUE: str(pps_max_voltage)})
        add_element_to_xml(xml_ele, constants.SINK_PDO_OP_CURRENT,
                           f'{pps_current_ma} mA',
                           {constants.VALUE: str(pps_current)})
    else:
        raise ValueError(f'ERROR: Invalid PDO_TYPE {pdo_value}')
    add_element_to_xml(xml_ele, constants.SINK_PDO_SUPPLY_TYPE,
                       constants.PDO_TYPES[pdo_type], {constants.VALUE: str(pdo_type)})
    return power_mw


def parse_and_add_controller_and_data_to_xml(xml_ele, cad):
    xml_ele = add_element_to_xml(xml_ele, cad.basename)
    for name in cad.data:
        add_element_to_xml(xml_ele, name, str(cad.data[name]))
    parse_and_add_node_to_xml(xml_ele, cad.controller)


def parse_and_add_array_to_xml(xml_ele, prop):
    for member in prop.val:
        if is_simple_datatype(member):
            add_element_to_xml(xml_ele, prop.name, str(member))
        elif isinstance(member, list):
            new_xml_ele = add_element_to_xml(xml_ele, prop.name)
            parse_and_add_array_to_xml(new_xml_ele, member)
        elif isinstance(member, edtlib.Node):
            new_xml_ele = add_element_to_xml(xml_ele, prop.name)
            parse_and_add_node_to_xml(new_xml_ele, member)
        elif isinstance(member, edtlib.ControllerAndData):
            new_xml_ele = add_element_to_xml(xml_ele, prop.name)
            parse_and_add_controller_and_data_to_xml(new_xml_ele, member)
        else:
            ValueError(
                f'Noticed undefined type : {str(type(member))}, with value {str(member)}')


def parse_and_add_node_to_xml(xml_ele, node):
    if not isinstance(node, edtlib.Node):
        return
    xml_ele = add_element_to_xml(xml_ele, node.name)
    for prop in node.props:
        if is_simple_datatype(node.props[prop].val):
            add_element_to_xml(xml_ele, node.props[prop].name,
                               str(node.props[prop].val))
        elif node.props[prop].name == constants.SINK_PDOS:
            parse_and_add_sink_pdos_to_xml(xml_ele, node.props[prop].val)
        elif isinstance(node.props[prop].val, list):
            parse_and_add_array_to_xml(xml_ele, node.props[prop])
        elif isinstance(node.props[prop].val, edtlib.Node):
            new_xml_ele = add_element_to_xml(xml_ele, node.props[prop].name)
            parse_and_add_node_to_xml(new_xml_ele, node.props[prop].val)
        elif isinstance(node.props[prop].val, edtlib.ControllerAndData):
            new_xml_ele = add_element_to_xml(xml_ele, node.props[prop].name)
            parse_and_add_controller_and_data_to_xml(new_xml_ele, node.props[prop].val)
        else:
            ValueError(
                f'Noticed undefined type : {str(type(node.props[prop].val))}, '
                f'with value {str(node.props[prop].val)}')
    for child in node.children:
        new_xml_ele = add_element_to_xml(xml_ele, child)
        parse_and_add_node_to_xml(new_xml_ele, node.children[child])


def parse_args():
    parser = argparse.ArgumentParser(allow_abbrev=False)
    parser.add_argument("--edt-pickle", required=True,
                        help="path to read the pickled edtlib.EDT object from")
    parser.add_argument("--compatible", required=True,
                        help="device tree compatible to be parsed")
    parser.add_argument("--vif-out", required=True,
                        help="path to write VIF policies to")
    parser.add_argument("--board", required=True, help="board name")
    return parser.parse_args()


if __name__ == "__main__":
    main()
