#
# Copyright (c) 2018 Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

from extract.globals import *
from extract.edts import *
from extract.directive import DTDirective
from extract.default import default

##
# @brief Generate device tree information based on heuristics.
#
# Generates in EDTS:
# - bus/master : device id of bus master for a bus device
# - parent-device : device id of parent device
class DTHeuristics(DTDirective):

    def __init__(self):
        pass

    ##
    # @brief Generate device tree information based on heuristics.
    #
    # Device tree properties may have to be deduced by heuristics
    # as the property definitions are not always consistent across
    # different node types.
    #
    # @param node_address Address of node owning the
    #                     compatible definition.
    # @param yaml YAML definition for the owning node.
    # @param prop compatible property name
    # @param def_label Define label string of node owning the
    #                  compatible definition.
    #
    def extract(self, node_address, yaml, prop, def_label):
        compatible = reduced[node_address]['props']['compatible']
        if not isinstance(compatible, list):
            compatible = [compatible]

        # Check for <device>-device that is connected to a bus
        for compat in compatible:
            for device_type in yaml[compat].get('type', []):
                if not device_type.endswith('-device'):
                    continue

                bus_master_device_type = device_type.replace('-device', '')

                # get parent
                parent_node_address = ''
                for comp in node_address.split('/')[1:-1]:
                    parent_node_address += '/' + comp

                # get parent yaml
                parent_yaml = yaml[reduced[parent_node_address] \
                                          ['props']['compatible']]

                if bus_master_device_type not in parent_yaml['type']:
                    continue

                # generate EDTS
                edts_insert_device_property(node_address, 'bus/master',
                    parent_node_address)

        # Check for a parent device this device is subordinated
        edts_insert_device_parent_device_property(node_address)

##
# @brief Management information for heuristics.
heuristics = DTHeuristics()
