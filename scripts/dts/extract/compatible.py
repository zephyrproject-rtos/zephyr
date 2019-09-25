#
# Copyright (c) 2018 Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

# NOTE: This file is part of the old device tree scripts, which will be removed
# later. They are kept to generate some legacy #defines via the
# --deprecated-only flag.
#
# The new scripts are gen_defines.py, edtlib.py, and dtlib.py.

from extract.globals import *
from extract.directive import DTDirective

##
# @brief Manage compatible directives.
#
# Handles:
# - compatible
#
class DTCompatible(DTDirective):
    ##
    # @brief Extract compatible
    #
    # @param node_path Path to node owning the
    #                  compatible definition.
    # @param prop compatible property name
    # @param def_label Define label string of node owning the
    #                  compatible definition.
    #
    def extract(self, node_path, prop, def_label):

        # compatible definition
        binding = get_binding(node_path)
        compatible = reduced[node_path]['props'][prop]
        if not isinstance(compatible, list):
            compatible = [compatible, ]

        for comp in compatible:
            # Generate #define
            insert_defs(node_path,
                        {'DT_COMPAT_' + str_to_label(comp): '1'},
                        {})

            # Generate #define for BUS a "sensor" might be on, for example
            # #define DT_ST_LPS22HB_PRESS_BUS_SPI 1
            if 'parent' in binding:
                compat_def = 'DT_' + str_to_label(comp) + '_BUS_' + \
                    binding['parent']['bus'].upper()
                insert_defs(node_path, {compat_def: '1'}, {})

        # Generate defines of the form:
        # #define DT_<COMPAT>_<INSTANCE ID> 1
        for compat, instance_id in reduced[node_path]['instance_id'].items():
            compat_instance = 'DT_' + str_to_label(compat) + '_' + str(instance_id)

            insert_defs(node_path, {compat_instance: '1'}, {})
            deprecated_main.append(compat_instance)

            # Generate defines of the form:
            # #define DT_<COMPAT>_<INSTANCE ID>_BUS_<BUS> 1
            if 'parent' in binding:
                bus = binding['parent']['bus']
                insert_defs(node_path,
                            {compat_instance + '_BUS_' + bus.upper(): '1'},
                            {})
                deprecated_main.append(compat_instance + '_BUS_' + bus.upper())


##
# @brief Management information for compatible.
compatible = DTCompatible()
