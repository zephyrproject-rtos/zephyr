#
# Copyright (c) 2018 Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

from .globals import *
import edtsdatabase

edts = edtsdatabase.EDTSDatabase()

##
# @brief Get EDTS device id associated to node address.
#
# @return ETDS device id
def edts_device_id(node_address):
	return node_address

def edts_insert_chosen(chosen, node_address):
    edts.insert_chosen(chosen, edts_device_id(node_address))

##
# @brief Insert device property into EDTS
#
def edts_insert_device_property(node_address, property_path, property_value):
    device_id = edts_device_id(node_address)
    edts.insert_device_property(device_id, property_path, property_value)

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
