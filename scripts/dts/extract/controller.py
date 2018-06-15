#
# Copyright (c) 2018 Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

from extract.globals import *
from extract.edts import *
from extract.directive import DTDirective

##
# @brief Manage <device>-controller directives.
#
# Handles:
# - <device>-controller
#
# Generates in EDTS:
# - <device>-controller : True
class DTController(DTDirective):

    def __init__(self):
        pass

    ##
    # @brief Extract <device>-controller
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

        # generate EDTS
        device_id = edts_device_id(node_address)
        edts_insert_device_property(device_id, prop, True)

##
# @brief Management information for [device]-controller.
controller = DTController()
