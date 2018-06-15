#
# Copyright (c) 2018 Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

from extract.globals import *
from extract.directive import DTDirective

from extract.controller import controller
from extract.default import default

##
# @brief Generate device tree information based on heuristics.
#
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
    # @param names (unused)
    # @param[out] defs Property definitions for each node address
    # @param def_label Define label string of node owning the
    #                  compatible definition.
    #
    def extract(self, node_address, yaml, prop, names, defs, def_label):
        # Check for <device>-controllers
        compatible = reduced[node_address]['props']['compatible']
        if not isinstance(compatible, list):
            compatible = [compatible]
        for compat in compatible:
            if 'uart' in yaml[compat]['node_type']:
                controller.extract(node_address, yaml, 'serial-controller', {},
                                   defs, def_label)
                break
            if 'spi' in yaml[compat]['node_type']:
                controller.extract(node_address, yaml, 'spi-controller', {},
                                   defs, def_label)
                break
            if 'i2c' in yaml[compat]['node_type']:
                controller.extract(node_address, yaml, 'i2c-controller', {},
                                   defs, def_label)
                break
            if 'clock-provider' in yaml[compat]['node_type']:
                controller.extract(node_address, yaml, 'clock-controller', {},
                                   defs, def_label)
                break
            if 'can' in yaml[compat]['node_type']:
                controller.extract(node_address, yaml, 'can-controller', {},
                                   defs, def_label)
                break

##
# @brief Management information for heuristics.
heuristics = DTHeuristics()
