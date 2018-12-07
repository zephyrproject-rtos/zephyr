#
# Copyright (c) 2018 Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

from extract.globals import *
from extract.directive import DTDirective

##
# @brief Manage compatible directives.
#
# Handles:
# - compatible
#
class DTCompatible(DTDirective):

    def __init__(self):
        pass

    ##
    # @brief Extract compatible
    #
    # @param node_address Address of node owning the
    #                     compatible definition.
    # @param prop compatible property name
    # @param def_label Define label string of node owning the
    #                  compatible definition.
    #
    def extract(self, node_address, prop, def_label):

        # compatible definition
        binding = get_binding(node_address)
        compatible = reduced[node_address]['props'][prop]
        if not isinstance(compatible, list):
            compatible = [compatible, ]

        for i, comp in enumerate(compatible):
            # Generate #define's
            compat_label = convert_string_to_label(str(comp))
            compat_defs = 'DT_COMPAT_' + compat_label
            load_defs = {
                compat_defs: "1",
            }
            insert_defs(node_address, load_defs, {})
            # Generate #define for BUS a "sensor" might be on
            # for example:
            # #define DT_ST_LPS22HB_PRESS_BUS_SPI 1
            if 'parent' in binding:
                bus = binding['parent']['bus']
                compat_defs = 'DT_' + compat_label + '_BUS_' + bus.upper()
                load_defs = {
                    compat_defs: "1",
                }
                insert_defs(node_address, load_defs, {})


##
# @brief Management information for compatible.
compatible = DTCompatible()
