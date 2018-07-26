#
# Copyright (c) 2018 Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

from extract.globals import *
from extract.directive import DTDirective

##
# @brief Manage <device>-controller directives.
#
class DTController(DTDirective):

    def __init__(self):
        self._controllers = {}
        self._data = {}

    ##
    # @brief Extract <device>-controller
    #
    # <device>-controller information is added to the top node
    # - <top node>_<device>_CONTROLLER_<index>: <device_label>
    # - <top node>_<device>_CONTROLLER_COUNT: <index> + 1
    #
    # @param node_address Address of node owning the
    #                     <device>-controller definition.
    # @param yaml YAML definition for the owning node.
    # @param prop <device>-controller property name
    # @param names (unused)
    # @param def_label Define label string of node owning the
    #                  <device>-controller definition.
    #
    def extract(self, node_address, yaml, prop, names, def_label):

        # <device>-controller definition
        device = prop.replace('-controller', '')
        device_label = self.get_node_label_string(node_address)

        # top node
        top_node_def = {}
        top_node_address = node_top_address(node_address)
        top_node_label = convert_string_to_label(top_node_address)
        # assure top node in controllers
        if not top_node_address in self._controllers:
            self._controllers[top_node_address] = {}
        # assure device in top node
        if not device in self._controllers[top_node_address]:
            self._controllers[top_node_address][device] = {}

        # - Controller information is added to the top node
        if device_label in self._controllers[top_node_address][device]:
            # raise Exception(
            print("----------", node_address,
                "Device address {} used by multiple {} controllers."
                    .format(device_label, device))
            return
        controller_index = len(self._controllers[top_node_address][device])
        self._controllers[top_node_address][device][device_label] = controller_index
        controller_label = self.get_label_string([top_node_label, device,
                                    "CONTROLLER", str(controller_index)])
        top_node_def[controller_label] = device_label

        # top node controller count
        controller_label = self.get_label_string([top_node_label, device,
                                    "CONTROLLER", "COUNT"])
        top_node_def[controller_label] = str(len(self._controllers[top_node_address][device]))
        insert_defs(top_node_address, top_node_def, {})

##
# @brief Management information for [device]-controller.
controller = DTController()
