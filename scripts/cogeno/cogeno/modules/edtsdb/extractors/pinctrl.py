#
# Copyright (c) 2018 Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

from .directive import DTDirective

##
# @brief Manage pinctrl related directives.
#
# Handles:
# - pinctrl-x
# - pinctrl-names
#
# Generates in EDTS:
# - pinctrl/<index>/name           : name of the pinctrl
# - pinctrl/<index>/pinconf/<pinconf-index>/pin-controller : device_id of
#                                                            pin controller
# - pinctrl/<index>/pinconf/<pinconf-index>/bias-disable        : pinconf value
# - pinctrl/<index>/pinconf/<pinconf-index>/bias-high-impedance : ..
# - pinctrl/<index>/pinconf/<pinconf-index>/bias-bus-hold       : ..
#
class DTPinCtrl(DTDirective):

    ##
    # @brief Boolean type properties for pin configuration by pinctrl.
    pinconf_bool_props = [
        "bias-disable", "bias-high-impedance", "bias-bus-hold",
        "drive-push-pull", "drive-open-drain", "drive-open-source",
        "input-enable", "input-disable", "input-schmitt-enable",
        "input-schmitt-disable", "low-power-enable", "low-power-disable",
        "output-disable", "output-enable", "output-low","output-high"]
    ##
    # @brief Boolean or value type properties for pin configuration by pinctrl.
    pinconf_bool_or_value_props = [
        "bias-pull-up", "bias-pull-down", "bias-pull-pin-default"]
    ##
    # @brief List type properties for pin configuration by pinctrl.
    pinconf_list_props = [
        "pinmux", "pins", "group", "drive-strength", "input-debounce",
        "power-source", "slew-rate", "speed"]

    def __init__(self, extractors):
        super().__init__(extractors)
        # Ignore properties that are covered by the extraction of:
        extractors.ignore_properties.extend([ \
            # - 'pinctrl-x' for pin controller clients
            'pinctrl-names',])

    ##
    # @brief Extract pinctrl information.
    #
    # @param node_address Address of node owning the pinctrl definition.
    # @param yaml YAML definition for the owning node.
    # @param prop pinctrl-x key
    # @param def_label Define label string of client node owning the pinctrl
    #                  definition.
    #
    def extract(self, node_address, prop):

        node = self.dts().reduced[node_address]

        # Get pinctrl index from pinctrl-<index> directive
        pinctrl_index = int(prop.split('-')[1])
        # Pinctrl definition
        pinctrl = node['props'][prop]
        # Name of this pinctrl state. Use directive if there is no name.
        pinctrl_names = node['props'].get('pinctrl-names', [])
        if not isinstance(pinctrl_names, list):
            pinctrl_names = [pinctrl_names, ]
        if pinctrl_index >= len(pinctrl_names):
            pinctrl_name = prop
        else:
            pinctrl_name = pinctrl_names[pinctrl_index]

        pin_config_handles = []
        if not isinstance(pinctrl, list):
            pin_config_handles.append(pinctrl)
        else:
            pin_config_handles = list(pinctrl)

        # generate EDTS pinctrl
        pinctrl_client_device_id = self.edts_device_id(node_address)
        self.edts().insert_device_property(pinctrl_client_device_id,
            'pinctrl/{}/name'.format(pinctrl_index), pinctrl_name)

        pinconf_index = 0
        for pin_config_handle in pin_config_handles:
            pin_config_node_address = self.dts().phandles[pin_config_handle]
            pin_controller_node_address = \
                    '/'.join(pin_config_node_address.split('/')[:-1])
            pin_config_subnode_prefix = \
                            '/'.join(pin_config_node_address.split('/')[-1:])
            pin_controller_device_id = self.edts_device_id(pin_controller_node_address)
            for subnode_address in self.dts().reduced:
                if pin_config_subnode_prefix in subnode_address \
                    and pin_config_node_address != subnode_address:
                    # Found a subnode underneath the pin configuration node
                    # Create pinconf defines and EDTS
                    self.edts().insert_device_property(pinctrl_client_device_id,
                        'pinctrl/{}/pinconf/{}/name' \
                            .format(pinctrl_index, pinconf_index),
                        subnode_address.split('/')[-1])
                    self.edts().insert_device_property(pinctrl_client_device_id,
                        'pinctrl/{}/pinconf/{}/pin-controller' \
                            .format(pinctrl_index, pinconf_index),
                        pin_controller_device_id)
                    pinconf_props = self.dts().reduced[subnode_address]['props'].keys()
                    for pinconf_prop in pinconf_props:
                        pinconf_value = self.dts().reduced[subnode_address]['props'][pinconf_prop]
                        if pinconf_prop in self.pinconf_bool_props:
                            pinconf_value = 1 if pinconf_value else 0
                        elif pinconf_prop in self.pinconf_bool_or_value_props:
                            if isinstance(pinconf_value, bool):
                                pinconf_value = 1 if pinconf_value else 0
                        elif pinconf_prop in self.pinconf_list_props:
                            if not isinstance(pinconf_value, list):
                                pinconf_value = [pinconf_value, ]
                        else:
                            # should become an exception
                            print("pinctrl.py: Pin control pin configuration",
                                  "'{}'".format(pinconf_prop),
                                  "not supported - ignored.")
                        # generate EDTS pinconf value
                        self.edts().insert_device_property(pinctrl_client_device_id,
                            'pinctrl/{}/pinconf/{}/{}'.format(pinctrl_index,
                                                              pinconf_index,
                                                              pinconf_prop),
                            pinconf_value)
                    pinconf_index += 1
