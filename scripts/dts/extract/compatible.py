#
# Copyright (c) 2018 Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

from extract.globals import *
from extract.edts import *
from extract.directive import DTDirective

##
# @brief Manage compatible directives.
#
# Handles:
# - compatible
#
# Generates in EDTS:
# - compatible/<index> : compatible
class DTCompatible(DTDirective):

    def __init__(self):
        pass

    ##
    # @brief Extract compatible
    #
    # @param node_address Address of node owning the
    #                     compatible definition.
    # @param yaml YAML definition for the owning node.
    # @param prop compatible property name
    # @param def_label Define label string of node owning the
    #                  compatible definition.
    #
    def extract(self, node_address, yaml, prop, def_label):

        # compatible definition
        compatible = reduced[node_address]['props'][prop]
        if not isinstance(compatible, list):
            compatible = [compatible, ]

        # generate EDTS
        device_id = edts_device_id(node_address)
        for i, comp in enumerate(compatible):
            edts_insert_device_property(device_id, 'compatible/{}'.format(i),
					comp)

##
# @brief Management information for compatible.
compatible = DTCompatible()
