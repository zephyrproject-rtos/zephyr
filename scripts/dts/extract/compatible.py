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
            # Generate #define
            insert_defs(node_address,
                        {'DT_COMPAT_' + str_to_label(comp): '1'},
                        {})

            # Generate #define for BUS a "sensor" might be on, for example
            # #define DT_ST_LPS22HB_PRESS_BUS_SPI 1
            if 'parent' in binding:
                compat_def = 'DT_' + str_to_label(comp) + '_BUS_' + \
                    binding['parent']['bus'].upper()
                insert_defs(node_address, {compat_def: '1'}, {})

        # Generate defines of the form:
        # #define DT_<COMPAT>_<INSTANCE ID> 1
        for compat, instance_id in reduced[node_address]['instance_id'].items():
            compat_instance = 'DT_' + str_to_label(compat) + '_' + str(instance_id)

            insert_defs(node_address, {compat_instance: '1'}, {})

            # Generate defines of the form:
            # #define DT_<COMPAT>_<INSTANCE ID>_BUS_<BUS> 1
            if 'parent' in binding:
                bus = binding['parent']['bus']
                insert_defs(node_address,
                            {compat_instance + '_BUS_' + bus.upper(): '1'},
                            {})


##
# @brief Management information for compatible.
compatible = DTCompatible()
