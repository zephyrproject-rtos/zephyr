#
# Copyright (c) 2018 Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

from .directive import DTDirective

##
# @brief Generate device tree information based on heuristics.
#
# Generates in EDTS:
# - bus/master : device id of bus master for a bus device
# - parent-device : device id of parent device
class DTHeuristics(DTDirective):

    def __init__(self, extractors):
        super().__init__(extractors)

    ##
    # @brief Insert device parent-child relation into EDTS
    #
    def _edts_insert_device_parent_child_relation(self, node_address):
        # Check for a parent device this device is subordinated
        parent_device_id = None
        parent_node_address = ''
        for comp in node_address.split('/')[1:-1]:
            parent_node_address += '/' + comp
            compatibles = self.dts().get_compats(parent_node_address)
            if compatibles:
                # there is a parent device,
                if 'simple-bus' in compatibles:
                    # only use the ones that have a minimum control on the child
                    continue
                if ('pin-controller' in parent_node_address) and \
                    (self.dts().reduced[node_address]['props'] \
                        .get('compatible', None) is None):
                    # sub nodes of pin controllers should have their own compatible
                    # otherwise it is a pin configuration node (not a device)
                    continue
                parent_device_id = self.edts_device_id(parent_node_address)
        if parent_device_id:
            device_id = self.edts_device_id(node_address)
            self.edts().insert_device_property(device_id, 'parent-device',
                                               parent_device_id)
            if not parent_device_id in self.edts()['devices']:
                # parent not in edts - insert now
                self.edts().insert_device_property(parent_device_id,
                                                   'child-devices/0',
                                                   device_id)
                return
            child_devices = self.edts()['devices'][parent_device_id].get('child-devices', None)
            if child_devices is None:
                child_devices = {}
                self.edts()['devices'][parent_device_id]['child-devices'] = child_devices
            if not device_id in child_devices.values():
                child_devices[str(len(child_devices))] = device_id

    ##
    # @brief Generate device tree information based on heuristics.
    #
    # Device tree properties may have to be deduced by heuristics
    # as the property definitions are not always consistent across
    # different node types.
    #
    # @param node_address Address of node issuing the directive.
    # @param prop Directive property name
    #
    def extract(self, node_address, prop):
        compat = ''

        # Check aliases
        if node_address in self.dts().aliases:
            for i, alias in enumerate(self.dts().aliases[node_address]):
                # extract alias node info
                self.edts_insert_device_property(node_address,
                                                 'alias/{}'.format(i), alias)

        # Process compatible related work
        compatibles = self.dts().reduced[node_address]['props'].get('compatible', [])
        if not isinstance(compatibles, list):
            compatibles = [compatibles,]

        # Check for <device>-device that is connected to a bus
        for compat in compatibles:
            # get device type list
            try:
                device_types = self.bindings()[compat]['type']

                if not isinstance(device_types, list):
                    device_types = [device_types, ]
                    if not isinstance(device_types, list):
                        device_types = [device_types, ]

                # inject device type in database
                for j, device_type in enumerate(device_types):
                    self.edts().insert_device_type(compat, device_type)
                    self.edts_insert_device_property(node_address,
                                                     'device-type/{}'.format(j),
                                                     device_type)
            except KeyError:
                pass

            # Proceed with bus processing
            if 'parent' not in self.bindings()[compat]:
                continue

            bus_master_device_type = self.bindings()[compat]['parent']['bus']

            # get parent bindings
            parent_node_address = self.dts().get_parent_address(node_address)
            parent_binding = self.bindings()[self.dts().reduced[parent_node_address] \
                                          ['props']['compatible']]

            if bus_master_device_type != parent_binding['child']['bus']:
                continue

            # generate EDTS
            self.edts_insert_device_property(node_address, 'bus/master',
                                             parent_node_address)

        # Check for a parent device this device is subordinated
        self._edts_insert_device_parent_child_relation(node_address)
