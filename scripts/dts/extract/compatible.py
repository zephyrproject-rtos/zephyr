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
    # @param yaml YAML definition for the owning node.
    # @param prop compatible property name
    # @param names (unused)
    # @param def_label Define label string of node owning the
    #                  compatible definition.
    #
    def extract(self, node_address, yaml, prop, names, def_label):

        # compatible definition
        compatible = reduced[node_address]['props'][prop]
        if not isinstance(compatible, list):
            compatible = [compatible, ]

        for i, comp in enumerate(compatible):
            # Generate #define's
            compat_label = convert_string_to_label(str(comp))
            compat_defs = 'CONFIG_DT_COMPAT_' + compat_label
            load_defs = {
                compat_defs: "",
            }
            insert_defs(node_address, load_defs, {})

##
# @brief Management information for compatible.
compatible = DTCompatible()
