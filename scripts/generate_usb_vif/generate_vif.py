#!/usr/bin/env python3

# Copyright (c) 2022 The Chromium OS Authors
# SPDX-License-Identifier: Apache-2.0

"""This file contains a Python script which parses through Zephyr device tree using
EDT.pickle generated at build and generates a XML file containing USB VIF policies"""

import argparse
import os
import pickle
import sys
import xml.etree.ElementTree as ET

import constants

SCRIPTS_DIR = os.path.join(os.path.dirname(__file__), "..")
sys.path.insert(0, os.path.join(SCRIPTS_DIR, 'dts', 'python-devicetree', 'src'))

from devicetree import edtlib


def is_vif_element(name):
    if (name in constants.VIF_ELEMENTS) or (name in constants.DT_VIF_ELEMENTS):
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
        new_xml_ele = add_element_to_xml(xml_ele, element_name, text,
                                         attributes)
        if constants.CHILD in elements[element_name]:
            add_elements_to_xml(new_xml_ele,
                                elements[element_name][constants.CHILD])


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


def parse_and_add_sink_pdo_to_xml(xml_ele, pdo_value):
    power_mw = 0
    xml_ele = add_element_to_xml(xml_ele, constants.SINK_PDO)
    pdo_type = get_pdo_type(pdo_value)
    if pdo_type == constants.PDO_TYPE_FIXED:
        current = pdo_value & 0x3ff
        current_ma = current * 10
        voltage = (pdo_value >> 10) & 0x3ff
        voltage_mv = voltage * 50
        power_mw = (current_ma * voltage_mv) // 1000
        add_element_to_xml(xml_ele, constants.SINK_PDO_VOLTAGE,
                           f'{voltage_mv} mV',
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
        add_element_to_xml(xml_ele, constants.SINK_PDO_OP_POWER,
                           f'{power_mw} mW',
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
                       constants.PDO_TYPES[pdo_type],
                       {constants.VALUE: str(pdo_type)})
    return power_mw


def parse_and_add_sink_pdos_to_xml(xml_ele, sink_pdos):
    new_xml_ele = add_element_to_xml(xml_ele, constants.SINK_PDOS)
    snk_max_power = 0
    for pdo_value in sink_pdos:
        power_mv = parse_and_add_sink_pdo_to_xml(new_xml_ele, pdo_value)
        if power_mv > snk_max_power:
            snk_max_power = power_mv
    add_element_to_xml(xml_ele, constants.PD_POWER_AS_SINK,
                       f'{snk_max_power} mW',
                       {constants.VALUE: str(snk_max_power)})


def parse_and_add_component_to_xml(xml_ele, component_node):
    for name in component_node.props:
        prop = component_node.props[name]
        if name == constants.SINK_PDOS:
            parse_and_add_sink_pdos_to_xml(xml_ele, prop.val)
        elif name == constants.FR_SWAP_REQD_TYPE_C_CURRENT_AS_INITIAL_SOURCE:
            add_element_to_xml(xml_ele, name,
                               constants.FR_SWAP_REQD_TYPE_C_CURRENT_AS_INITIAL_SOURCE_DICT[
                                   prop.val], {constants.VALUE: str(prop.val)})
        else:
            add_element_to_xml(xml_ele, name, None,
                               {constants.VALUE: str(prop.val).lower()})


def parse_and_add_components_to_xml(xml_ele, component_nodes):
    for component in component_nodes.val:
        new_xml_ele = add_element_to_xml(xml_ele, component_nodes.name)
        parse_and_add_component_to_xml(new_xml_ele, component)


def parse_and_add_vif_to_xml(xml_ele, vif_node):
    for name in vif_node.props:
        prop = vif_node.props[name]
        if name == constants.VIF_PRODUCT_TYPE:
            add_element_to_xml(xml_ele, name,
                               constants.VIF_PRODUCT_TYPE_DICT[prop.val],
                               {constants.VALUE: str(prop.val)})
        elif name == constants.CERTIFICATION_TYPE:
            add_element_to_xml(xml_ele, name,
                               constants.CERTIFICATION_TYPE_DICT[prop.val],
                               {constants.VALUE: str(prop.val)})
        elif is_simple_datatype(prop.val):
            add_element_to_xml(xml_ele, name, prop.val)
        elif name == constants.COMPONENT:
            parse_and_add_components_to_xml(xml_ele, prop)


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--edt-pickle", required=True,
                        help="path to read the pickled edtlib.EDT object from")
    parser.add_argument("--compatible", required=True,
                        help="device tree compatible to be parsed")
    parser.add_argument("--vif-out", required=True,
                        help="path to write VIF policies to")
    parser.add_argument("--board", required=True, help="board name")
    return parser.parse_args()


def main():
    args = parse_args()
    try:
        with open(args.edt_pickle, 'rb') as f:
            edt = pickle.load(f)
    except edtlib.EDTError as err:
        sys.exit(f"devicetree error: {err}")

    # NOTE: Assuming there will be only one VIF node at max per project
    for node in edt.compat2nodes[args.compatible]:
        xml_root = get_root()
        add_elements_to_xml(xml_root, constants.VIF_SPEC_ELEMENTS)
        parse_and_add_vif_to_xml(xml_root, node)
        tree = ET.ElementTree(xml_root)
        tree.write(args.vif_out, xml_declaration=True,
                   encoding=constants.XML_ENCODING)


if __name__ == "__main__":
    main()
