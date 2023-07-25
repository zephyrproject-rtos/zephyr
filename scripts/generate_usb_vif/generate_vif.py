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

from constants import dt_constants
from constants import other_constants
from constants import pdo_constants
from constants import vif_element_constants
from constants import xml_constants

SCRIPTS_DIR = os.path.join(os.path.dirname(__file__), "..")
sys.path.insert(0, os.path.join(SCRIPTS_DIR, 'dts', 'python-devicetree', 'src'))


def get_value_attribute(data):
    if not isinstance(data, str):
        data = str(data)
    return {other_constants.VALUE: data}


def get_vif_element_name(name):
    return xml_constants.XML_ELEMENT_NAME_PREFIX + ":" + dt_constants.DT_VIF_ELEMENTS.get(
        name, name)


def get_root():
    xml_root = ET.Element(
        get_vif_element_name(xml_constants.XML_ROOT_ELEMENT_NAME))
    add_attributes_to_xml_element(xml_root,
                                  xml_constants.XML_NAMESPACE_ATTRIBUTES)
    return xml_root


def add_attributes_to_xml_element(xml_ele, attributes):
    for key, value in attributes.items():
        xml_ele.set(key, value)


def add_element_to_xml(xml_ele, name, text=None, attributes=None):
    new_xml_ele = ET.SubElement(xml_ele, get_vif_element_name(name))
    if text:
        new_xml_ele.text = str(text)
    if attributes:
        add_attributes_to_xml_element(new_xml_ele, attributes)
    return new_xml_ele


def add_elements_to_xml(xml_ele, elements):
    for element_name in elements:
        text = elements[element_name].get(other_constants.TEXT, None)
        attributes = elements[element_name].get(other_constants.ATTRIBUTES,
                                                None)
        new_xml_ele = add_element_to_xml(xml_ele, element_name, text,
                                         attributes)
        if other_constants.CHILD in elements[element_name]:
            add_elements_to_xml(new_xml_ele,
                                elements[element_name][other_constants.CHILD])


def add_vif_spec_from_source_to_xml(xml_ele, source_xml_ele):
    prefix = '{' + xml_constants.XML_VIF_NAMESPACE + '}'
    for child in source_xml_ele:
        element_name = child.tag[len(prefix):]
        if element_name in xml_constants.VIF_SPEC_ELEMENTS_FROM_SOURCE_XML:
            add_element_to_xml(xml_ele, element_name, child.text,
                               child.attrib)


def is_simple_datatype(value):
    if isinstance(value, (str, int, bool)):
        return True
    return False


def get_pdo_type(pdo_value):
    return pdo_value >> 30


def get_xml_bool_value(value):
    if value:
        return other_constants.TRUE
    return other_constants.FALSE


def parse_and_add_sink_pdo_to_xml(xml_ele, pdo_value, pdo_info):
    power_mw = 0
    xml_ele = add_element_to_xml(xml_ele, vif_element_constants.SINK_PDO)
    pdo_type = get_pdo_type(pdo_value)

    if pdo_type == pdo_constants.PDO_TYPE_FIXED:
        # As per USB PD specification Revision 3.1, Version 1.6, Table 6-16 Fixed supply PDO - Sink
        current = pdo_value & 0x3ff
        current_ma = current * 10
        voltage = (pdo_value >> 10) & 0x3ff
        voltage_mv = voltage * 50
        power_mw = (current_ma * voltage_mv) // 1000

        if pdo_value & (1 << 28):
            pdo_info[vif_element_constants.HIGHER_CAPABILITY_SET] = True
        pdo_info[
            vif_element_constants.FR_SWAP_REQD_TYPE_C_CURRENT_AS_INITIAL_SOURCE] = pdo_value & (
            3 << 23)
        add_element_to_xml(xml_ele, vif_element_constants.SINK_PDO_VOLTAGE,
                           f'{voltage_mv} mV',
                           get_value_attribute(voltage))
        add_element_to_xml(xml_ele, vif_element_constants.SINK_PDO_OP_CURRENT,
                           f'{current_ma} mA',
                           get_value_attribute(current))
    elif pdo_type == pdo_constants.PDO_TYPE_BATTERY:
        # As per USB PD specification Revision 3.1, Version 1.6, Table 6-18 Battery supply PDO - Sink
        max_voltage = (pdo_value >> 20) & 0x3ff
        max_voltage_mv = max_voltage * 50
        min_voltage = (pdo_value >> 10) & 0x3ff
        min_voltage_mv = min_voltage * 50
        power = pdo_value & 0x3ff
        power_mw = power * 250

        add_element_to_xml(xml_ele, vif_element_constants.SINK_PDO_MIN_VOLTAGE,
                           f'{min_voltage_mv} mV',
                           get_value_attribute(min_voltage))
        add_element_to_xml(xml_ele, vif_element_constants.SINK_PDO_MAX_VOLTAGE,
                           f'{max_voltage_mv} mV',
                           get_value_attribute(max_voltage))
        add_element_to_xml(xml_ele, vif_element_constants.SINK_PDO_OP_POWER,
                           f'{power_mw} mW',
                           get_value_attribute(power))
    elif pdo_type == pdo_constants.PDO_TYPE_VARIABLE:
        # As per USB PD specification Revision 3.1, Version 1.6, Table 6-17 Variable supply (non-Battery) PDO - Sink
        max_voltage = (pdo_value >> 20) & 0x3ff
        max_voltage_mv = max_voltage * 50
        min_voltage = (pdo_value >> 10) & 0x3ff
        min_voltage_mv = min_voltage * 50
        current = pdo_value & 0x3ff
        current_ma = current * 10
        power_mw = (current_ma * max_voltage_mv) // 1000

        add_element_to_xml(xml_ele, vif_element_constants.SINK_PDO_MIN_VOLTAGE,
                           f'{min_voltage_mv} mV',
                           get_value_attribute(min_voltage))
        add_element_to_xml(xml_ele, vif_element_constants.SINK_PDO_MAX_VOLTAGE,
                           f'{max_voltage_mv} mV',
                           get_value_attribute(max_voltage))
        add_element_to_xml(xml_ele, vif_element_constants.SINK_PDO_OP_CURRENT,
                           f'{current_ma} mA',
                           get_value_attribute(current))
    elif pdo_type == pdo_constants.PDO_TYPE_AUGUMENTED:
        # As per USB PD specification Revision 3.1, Version 1.6, Table 6-19 Programmable Power supply APDO - Sink
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

        add_element_to_xml(xml_ele, vif_element_constants.SINK_PDO_MIN_VOLTAGE,
                           f'{pps_min_voltage_mv} mV',
                           get_value_attribute(pps_min_voltage))
        add_element_to_xml(xml_ele, vif_element_constants.SINK_PDO_MAX_VOLTAGE,
                           f'{pps_max_voltage_mv} mV',
                           get_value_attribute(pps_max_voltage))
        add_element_to_xml(xml_ele, vif_element_constants.SINK_PDO_OP_CURRENT,
                           f'{pps_current_ma} mA',
                           get_value_attribute(pps_current))
    else:
        raise ValueError(f'ERROR: Invalid PDO_TYPE {pdo_value}')

    add_element_to_xml(xml_ele, vif_element_constants.SINK_PDO_SUPPLY_TYPE,
                       pdo_constants.PDO_TYPES[pdo_type],
                       get_value_attribute(pdo_type))
    return power_mw


def parse_and_add_sink_pdos_to_xml(xml_ele, sink_pdos):
    new_xml_ele = add_element_to_xml(xml_ele, dt_constants.SINK_PDOS)
    snk_max_power = 0
    pdo_info = dict()

    for pdo_value in sink_pdos:
        power_mv = parse_and_add_sink_pdo_to_xml(new_xml_ele, pdo_value,
                                                 pdo_info)
        if power_mv > snk_max_power:
            snk_max_power = power_mv

    add_element_to_xml(xml_ele, vif_element_constants.PD_POWER_AS_SINK,
                       f'{snk_max_power} mW',
                       get_value_attribute(snk_max_power))
    add_element_to_xml(xml_ele, vif_element_constants.HIGHER_CAPABILITY_SET,
                       None, get_value_attribute(get_xml_bool_value(
                        vif_element_constants.HIGHER_CAPABILITY_SET in
                            pdo_info)))
    fr_swap_required_type_c_current_as_initial_source = pdo_info[
        vif_element_constants.FR_SWAP_REQD_TYPE_C_CURRENT_AS_INITIAL_SOURCE]
    add_element_to_xml(xml_ele,
                       vif_element_constants.FR_SWAP_REQD_TYPE_C_CURRENT_AS_INITIAL_SOURCE,
                       other_constants.FR_SWAP_REQD_TYPE_C_CURRENT_AS_INITIAL_SOURCE_VALUES[
                           fr_swap_required_type_c_current_as_initial_source],
                       get_value_attribute(
                           fr_swap_required_type_c_current_as_initial_source))
    add_element_to_xml(xml_ele, vif_element_constants.NUM_SNK_PDOS, None,
                       get_value_attribute(len(sink_pdos)))


def parse_and_add_component_to_xml(xml_ele, node):
    add_element_to_xml(xml_ele, vif_element_constants.USB_PD_SUPPORT, None,
                       get_value_attribute(get_xml_bool_value(
                           not node.props[dt_constants.PD_DISABLE].val)))

    if not node.props[dt_constants.PD_DISABLE].val:
        power_role = node.props[dt_constants.POWER_ROLE].val
        add_element_to_xml(xml_ele, vif_element_constants.PD_PORT_TYPE,
                           other_constants.PD_PORT_TYPE_VALUES[power_role][1],
                           get_value_attribute(
                               other_constants.PD_PORT_TYPE_VALUES[
                                   power_role][0]))
        add_element_to_xml(xml_ele, vif_element_constants.TYPE_C_STATE_MACHINE,
                           other_constants.TYPE_C_STATE_MACHINE_VALUES[
                               power_role][1],
                           get_value_attribute(
                               other_constants.TYPE_C_STATE_MACHINE_VALUES[
                                   power_role][0]))

    if dt_constants.SINK_PDOS in node.props:
        parse_and_add_sink_pdos_to_xml(xml_ele,
                                       node.props[dt_constants.SINK_PDOS].val)


def get_source_xml_root(source_xml_file):
    return ET.parse(source_xml_file).getroot()


def parse_args():
    parser = argparse.ArgumentParser(allow_abbrev=False)
    parser.add_argument("--edt-pickle", required=True,
                        help="path to read the pickled edtlib.EDT object from")
    parser.add_argument("--compatible", required=True,
                        help="device tree compatible to be parsed")
    parser.add_argument("--vif-out", required=True,
                        help="path to write VIF policies to")
    parser.add_argument("--vif-source-xml", required=True,
                        help="XML file containing high level VIF specifications")
    return parser.parse_args()


def main():
    global edtlib

    args = parse_args()
    with open(args.edt_pickle, 'rb') as f:
        edt = pickle.load(f)
    edtlib = inspect.getmodule(edt)

    xml_root = get_root()
    source_xml_root = get_source_xml_root(args.vif_source_xml)
    add_elements_to_xml(xml_root, xml_constants.VIF_SPEC_ELEMENTS)
    add_vif_spec_from_source_to_xml(xml_root, source_xml_root)

    for node in edt.compat2nodes[args.compatible]:
        xml_ele = add_element_to_xml(xml_root, vif_element_constants.COMPONENT)
        parse_and_add_component_to_xml(xml_ele, node)

    tree = ET.ElementTree(xml_root)
    tree.write(args.vif_out, xml_declaration=True,
               encoding=xml_constants.XML_ENCODING)


if __name__ == "__main__":
    main()
