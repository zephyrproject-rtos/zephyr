#!/usr/bin/env python3

# Copyright (c) 2022 The Chromium OS Authors
# SPDX-License-Identifier: Apache-2.0

import argparse
import os
import sys
import xml.etree.ElementTree as ET

import constants

sys.path.insert(0, os.path.join(
    os.path.dirname(os.path.dirname(os.path.dirname(__file__))), 'dts',
    'python-devicetree', 'src'))

from devicetree import edtlib


def main():
  args = parse_args()
  try:
    edt = edtlib.EDT(args.dts, args.bindings_dirs)
  except edtlib.EDTError as e:
    sys.exit(f"devicetree error: {e}")
  root = get_root()
  add_elements_from_dict(constants.VIF_SPEC_ELEMENTS, root)
  add_xml_element(root, constants.MODEL_PART_NUMBER, args.board)
  xml_mem = add_xml_element(root, constants.COMPONENT)
  for node in edt.compat2nodes[args.compatible]:
    parse_node(xml_mem, node)
  tree = ET.ElementTree(root)
  tree.write(args.vif_out, xml_declaration=True,
             encoding=constants.XML_ENCODING)


def is_vif_element(name):
  if name in constants.VIF_ELEMENTS:
    return True
  return False


def get_vif_element_name(string):
  return constants.XML_ELEMENT_NAME_PREFIX + ":" + constants.DT_VIF_ELEMENTS_DICT.get(
      string, string)


def get_root():
  root = ET.Element(get_vif_element_name(constants.XML_ROOT_ELEMENT_NAME))
  add_attributes_from_dict(constants.XML_NAMESPACE_ATTRIBUTES, root)
  return root


def add_attributes_from_dict(dict, root):
  [root.set(key, value) for key, value in dict.items()]


def add_elements_from_dict(dict, root):
  for element_name in dict:
    text = dict[element_name].get(constants.TEXT, None)
    attributes = dict[element_name].get(constants.ATTRIBUTES, None)
    element = add_xml_element(root, element_name, text, attributes)
    if constants.CHILD in dict[element_name]:
      add_elements_from_dict(dict[element_name][constants.CHILD], element)


def add_xml_element(root, element_name, text=None, attributes=None):
  if is_vif_element(element_name):
    element = ET.SubElement(root, get_vif_element_name(element_name))
    if text:
      element.text = str(text)
    if attributes:
      add_attributes_from_dict(attributes, element)
    return element
  return root


def remove_special_characters(string):
  return ''.join(filter(str.isalnum, string))


def is_simple_data(data):
  if isinstance(data, (str, int, bool)):
    return True
  return False


def pdo_type(pdo_value):
  return pdo_value >> 30


def parse_sink_pdos(xml_mem, sink_pdos):
  add_xml_element(xml_mem, constants.NUM_SINK_PDOS, None,
                  {constants.VALUE: str(len(sink_pdos))})
  add_xml_element(xml_mem, constants.EPR_SUPPORTED_AS_SINK,
                  attributes={constants.VALUE: "false"})
  add_xml_element(xml_mem, constants.NO_USB_SUSPEND_MAY_BE_SET,
                  attributes={constants.VALUE: "true"})
  add_xml_element(xml_mem, constants.HIGHER_CAPABILITY_SET,
                  attributes={constants.VALUE: "false"})
  add_xml_element(xml_mem,
                  constants.FR_SWAP_REQD_TYPE_C_CURRENT_AS_INITIAL_SOURCE,
                  "FR_Swap not supported", attributes={constants.VALUE: "0"})
  xml_mem_1 = add_xml_element(xml_mem, constants.SINK_PDOS)
  snk_max_power = 0
  for pdo_value in sink_pdos:
    power_mv = parse_sink_pdo(xml_mem_1, pdo_value)
    if power_mv > snk_max_power:
      snk_max_power = power_mv
  add_xml_element(xml_mem, constants.PD_POWER_AS_SINK, f'{snk_max_power} mW',
                  {constants.VALUE: str(snk_max_power)})


def parse_sink_pdo(xml_mem, pdo_value):
  power_mw = 0
  xml_mem = add_xml_element(xml_mem, constants.SINK_PDO)
  type = pdo_type(pdo_value)
  add_xml_element(xml_mem, constants.SINK_PDO_SUPPLY_TYPE,
                  constants.PDO_TYPES[type], {constants.VALUE: str(type)})
  if type == 0:
    current = pdo_value & 0x3ff
    current_ma = current * 10
    voltage = (pdo_value >> 10) & 0x3ff
    voltage_mv = voltage * 50
    power_mw = (current_ma * voltage_mv) // 1000
    add_xml_element(xml_mem, constants.SINK_PDO_VOLTAGE, f'{voltage_mv} mV',
                    {constants.VALUE: str(voltage)})
    add_xml_element(xml_mem, constants.SINK_PDO_OP_CURRENT, f'{current_ma} mA',
                    {constants.VALUE: str(current)})
  elif type == 1:
    max_voltage = (pdo_value >> 20) & 0x3ff
    max_voltage_mv = max_voltage * 50
    min_voltage = (pdo_value >> 10) & 0x3ff
    min_voltage_mv = min_voltage * 50
    power = pdo_value & 0x3ff
    power_mw = power * 250
    add_xml_element(xml_mem, constants.SINK_PDO_MIN_VOLTAGE,
                    f'{min_voltage_mv} mV', {constants.VALUE: str(min_voltage)})
    add_xml_element(xml_mem, constants.SINK_PDO_MAX_VOLTAGE,
                    f'{max_voltage_mv} mV', {constants.VALUE: str(max_voltage)})
    add_xml_element(xml_mem, constants.SINK_PDO_OP_POWER, f'{power_mw} mW',
                    {constants.VALUE: str(power)})
  elif type == 2:
    max_voltage = (pdo_value >> 20) & 0x3ff
    max_voltage_mv = max_voltage * 50
    min_voltage = (pdo_value >> 10) & 0x3ff
    min_voltage_mv = min_voltage * 50
    current = pdo_value & 0x3ff
    current_ma = current * 10
    power_mw = (current_ma * max_voltage_mv) // 1000
    add_xml_element(xml_mem, constants.SINK_PDO_MIN_VOLTAGE,
                    f'{min_voltage_mv} mV', {constants.VALUE: str(min_voltage)})
    add_xml_element(xml_mem, constants.SINK_PDO_MAX_VOLTAGE,
                    f'{max_voltage_mv} mV', {constants.VALUE: str(max_voltage)})
    add_xml_element(xml_mem, constants.SINK_PDO_OP_CURRENT, f'{current_ma} mA',
                    {constants.VALUE: str(current)})
  elif type == 3:
    pps_max_voltage = (pdo_value >> 17) & 0xff
    pps_max_voltage_mv = pps_max_voltage * 100
    pps_min_voltage = (pdo_value >> 8) & 0xff
    pps_min_voltage_mv = pps_min_voltage * 100
    pps_current = pdo_value & 0x7f
    pps_current_ma = pps_current * 50
    power_mw = (pps_current_ma * pps_max_voltage_mv) // 1000
    add_xml_element(xml_mem, constants.SINK_PDO_MIN_VOLTAGE,
                    f'{pps_min_voltage_mv} mV',
                    {constants.VALUE: str(pps_min_voltage)})
    add_xml_element(xml_mem, constants.SINK_PDO_MAX_VOLTAGE,
                    f'{pps_max_voltage_mv} mV',
                    {constants.VALUE: str(pps_max_voltage)})
    add_xml_element(xml_mem, constants.SINK_PDO_OP_CURRENT,
                    f'{pps_current_ma} mA', {constants.VALUE: str(pps_current)})
  else:
    raise ValueError(f'ERROR: Invalid PDO_TYPE {pdo_value}')
  return power_mw


def parse_controller_and_data(xml_mem, cad):
  xml_mem = add_xml_element(xml_mem, cad.basename)
  for name in cad.data:
    add_xml_element(xml_mem, name, str(cad.data[name]))
  parse_node(xml_mem, cad.controller)


def parse_array(xml_mem, prop):
  for member in prop.val:
    if is_simple_data(member):
      add_xml_element(xml_mem, prop.name, str(member))
    elif isinstance(member, list):
      xml_mem_1 = add_xml_element(xml_mem, prop.name)
      parse_array(xml_mem_1, member)
    elif isinstance(member, edtlib.Node):
      xml_mem_1 = add_xml_element(xml_mem, prop.name)
      parse_node(xml_mem_1, member)
    elif isinstance(member, edtlib.ControllerAndData):
      xml_mem_1 = add_xml_element(xml_mem, prop.name)
      parse_controller_and_data(xml_mem_1, member)
    else:
      ValueError(
          f'Noticed undefined type : {str(type(member))}, with value {str(member)}')


def parse_node(xml_mem, node):
  if not isinstance(node, edtlib.Node):
    return
  xml_mem = add_xml_element(xml_mem, node.name)
  for prop in node.props:
    if is_simple_data(node.props[prop].val):
      add_xml_element(xml_mem, node.props[prop].name, str(node.props[prop].val))
    elif node.props[prop].name == constants.SINK_PDOS:
      parse_sink_pdos(xml_mem_1, node.props[prop].val)
    elif isinstance(node.props[prop].val, list):
      parse_array(xml_mem, node.props[prop])
    elif isinstance(node.props[prop].val, edtlib.Node):
      xml_mem_1 = add_xml_element(xml_mem, node.props[prop].name)
      parse_node(xml_mem_1, node.props[prop].val)
    elif isinstance(node.props[prop].val, edtlib.ControllerAndData):
      xml_mem_1 = add_xml_element(xml_mem, node.props[prop].name)
      parse_controller_and_data(xml_mem_1, node.props[prop].val)
    else:
      ValueError(
          f'Noticed undefined type : {str(type(node.props[prop].val))}, with value {str(node.props[prop].val)}')
  for child in node.children:
    xml_mem_1 = add_xml_element(xml_mem, child)
    parse_node(xml_mem_1, node.children[child])


def parse_args():
  parser = argparse.ArgumentParser()
  parser.add_argument("--dts", required=True,
                      help="Aggregated Zephyr project device tree file")
  parser.add_argument("--vif-out", required=True,
                      help="path to write VIF policies to")
  parser.add_argument("--bindings-dirs", nargs='+', required=True,
                      help="directory with bindings in YAML format, we allow multiple")
  parser.add_argument("--compatible", required=True,
                      help="device tree compatible to be parsed")
  parser.add_argument("--board", required=True, help="board name")
  return parser.parse_args()


if __name__ == "__main__":
  main()
