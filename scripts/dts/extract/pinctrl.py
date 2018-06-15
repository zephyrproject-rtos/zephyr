#
# Copyright (c) 2018 Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

from extract.globals import *
from extract.edts import *
from extract.directive import DTDirective

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

    def __init__(self):
        pass

    ##
    # @brief Extract pinctrl information.
    #
    # @param node_address Address of node owning the pinctrl definition.
    # @param yaml YAML definition for the owning node.
    # @param prop pinctrl-x key
    # @param names Names assigned to pinctrl state pinctrl-<index>.
    # @param def_label Define label string of client node owning the pinctrl
    #                  definition.
    #
    def extract(self, node_address, yaml, prop, names, def_label):

        # Get pinctrl index from pinctrl-<index> directive
        pinctrl_index = int(prop.split('-')[1])
        # Pinctrl definition
        pinctrl = reduced[node_address]['props'][prop]
        # Name of this pinctrl state. Use directive if there is no name.
        if pinctrl_index >= len(names):
            pinctrl_name = prop
        else:
            pinctrl_name = names[pinctrl_index]

        pin_config_handles = []
        if not isinstance(pinctrl, list):
            pin_config_handles.append(pinctrl)
        else:
            pin_config_handles = list(pinctrl)

        # generate EDTS pinctrl
        pinctrl_client_device_id = edts_device_id(node_address)
        edts_insert_device_property(pinctrl_client_device_id,
            'pinctrl/{}/name'.format(pinctrl_index), pinctrl_name)

        client_prop_def = {}
        pinconf_index = 0
        for pin_config_handle in pin_config_handles:
            pin_config_node_address = phandles[pin_config_handle]
            pin_controller_node_address = \
                    '/'.join(pin_config_node_address.split('/')[:-1])
            pin_config_subnode_prefix = \
                            '/'.join(pin_config_node_address.split('/')[-1:])
            pin_controller_device_id = edts_device_id(pin_controller_node_address)
            for subnode_address in reduced:
                if pin_config_subnode_prefix in subnode_address \
                    and pin_config_node_address != subnode_address:
                    # Found a subnode underneath the pin configuration node
                    # Create pinconf defines and EDTS
                    edts_insert_device_property(pinctrl_client_device_id,
                        'pinctrl/{}/pinconf/{}/name' \
                            .format(pinctrl_index, pinconf_index),
                        subnode_address.split('/')[-1])
                    edts_insert_device_property(pinctrl_client_device_id,
                        'pinctrl/{}/pinconf/{}/pin-controller' \
                            .format(pinctrl_index, pinconf_index),
                        pin_controller_device_id)
                    pinconf_props = reduced[subnode_address]['props'].keys()
                    for pinconf_prop in pinconf_props:
                        pinconf_value = reduced[subnode_address]['props'][pinconf_prop]
                        if pinconf_prop in edts.pinconf_bool_props:
                            pinconf_value = 1 if pinconf_value else 0
                        elif pinconf_prop in edts.pinconf_bool_or_value_props:
                            if isinstance(pinconf_value, bool):
                                pinconf_value = 1 if pinconf_value else 0
                        elif pinconf_prop in edts.pinconf_list_props:
                            if not isinstance(pinconf_value, list):
                                pinconf_value = [pinconf_value, ]
                        else:
                            # generate defines

                            # use cell names if available
                            # (for a pinmux node these are: pin, function)
                            controller_compat = get_compat(pin_controller_node_address)
                            if controller_compat is None:
                                raise Exception("No binding or compatible missing for {}."
                                                .format(pin_controller_node_address))
                            controller_yaml = yaml.get(controller_compat, None)
                            if controller_yaml is None:
                                raise Exception("No binding for {}."
                                                .format(controller_compat))
                            if 'pinmux' in controller_compat:
                                cell_prefix = 'PINMUX'
                            else:
                                cell_prefix = 'PINCTRL'
                            cell_names = controller_yaml.get('#cells', None)
                            if cell_names is None:
                                # No cell names - use default names for pinctrl
                                cell_names = ['pin', 'function']
                            pin_label = self.get_label_string([def_label, \
                                        cell_prefix] + subnode_address.split('/')[-2:] \
                                        + [cell_names[0], str(pinconf_index)])
                            func_label = self.get_label_string([def_label, \
                                        cell_prefix] + subnode_address.split('/')[-2:] \
                                        + [cell_names[1], str(pinconf_index)])
                            client_prop_def[pin_label] = pinconf_prop
                            client_prop_def[func_label] = pinconf_value
                            continue
                        # generate EDTS pinconf value
                        edts_insert_device_property(pinctrl_client_device_id,
                            'pinctrl/{}/pinconf/{}/{}'.format(pinctrl_index,
                                                              pinconf_index,
                                                              pinconf_prop),
                            pinconf_value)
                    pinconf_index += 1
        # update property definitions of owning node
        insert_defs(node_address, client_prop_def, {})

##
# @brief Management information for pinctrl-[x].
pinctrl = DTPinCtrl()
