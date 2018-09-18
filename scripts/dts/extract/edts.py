#
# Copyright (c) 2018 Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#
import pprint
from .globals import *
import edtsdatabase

edts = edtsdatabase.EDTSDatabase()

##
# @brief Get EDTS device id associated to node address.
#
# @return ETDS device id
def edts_device_id(node_address):
    return node_address


def edts_get_controller_type(controller_address):
    controller_type = ''
    controller_props = reduced[controller_address]['props']
    for prop in controller_props:
        if 'controller' in prop:
            controller_type = prop
            break
        if 'parent' in prop:
            controller_type = prop
            break
    if controller_type == '':
        raise Exception("Try to instanciate {} has a controller".format(controller_address))
    return controller_type

def edts_insert_chosen(chosen, node_address):
    edts.insert_chosen(chosen, edts_device_id(node_address))

##
# @brief Insert device property into EDTS
#
def edts_insert_device_property(node_address, property_path, property_value):
    device_id = edts_device_id(node_address)
    edts.insert_device_property(device_id, property_path, property_value)

##
# @brief Insert device type into EDTS
#
def edts_insert_device_type(compatible, device_type):
    edts.insert_device_type(compatible, device_type)

##
# @brief Insert device controller into EDTS
#
def edts_insert_device_controller(controller_address, node_address, line):
    device_id = edts_device_id(node_address)
    controller_id = edts_device_id(controller_address)
    controller_type = edts_get_controller_type(controller_address)
    edts.insert_device_controller(controller_type, controller_id,
                                  device_id, line)

##
# @brief Insert device parent-device property into EDTS
#
def edts_insert_device_parent_device_property(node_address):
    # Check for a parent device this device is subordinated
    parent_device_id = None
    parent_node_address = ''
    for comp in node_address.split('/')[1:-1]:
        parent_node_address += '/' + comp
        compatibles = reduced[parent_node_address]['props'] \
                .get('compatible', None)
        if compatibles:
            # there is a parent device,
            # only use the ones that have a minimum control on the child
            if 'simple-bus' not in compatibles:
                parent_device_id = edts_device_id(parent_node_address)
    if parent_device_id:
        edts_insert_device_property(edts_device_id(node_address),
                    'parent-device', parent_device_id)
