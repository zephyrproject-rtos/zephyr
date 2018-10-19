#
# Copyright (c) 2018 Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

from .directive import DTDirective

##
# @brief Manage directives in a default way.
#
class DTDefault(DTDirective):

    def __init__(self, extractors):
        super().__init__(extractors)

    ##
    # @brief Extract directive information.
    #
    # @param edts Extended device tree database
    # @param dts Device tree specification
    # @param bindings Device tree bindings
    # @param node_address Address of node issuing the directive.
    # @param prop Directive property name
    #
    def extract(self, node_address, prop):

        prop_values = self.dts().reduced[node_address]['props'][prop]

        # generate EDTS
        self.edts_insert_device_property(node_address, prop, prop_values)
