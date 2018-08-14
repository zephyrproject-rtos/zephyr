#
# Copyright (c) 2018 Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

from extract.globals import *
from extract.directive import DTDirective

##
# @brief Manage directives in a default way.
#
class DTDefault(DTDirective):

    def __init__(self):
        pass

    ##
    # @brief Extract directives in a default way
    #
    # @param node_address Address of node owning the clockxxx definition.
    # @param yaml YAML definition for the owning node.
    # @param prop property name
    # @param names (unused)
    # @param def_label Define label string of node owning the directive.
    #
    def extract(self, node_address, yaml, prop, names, def_label):
        prop_def = {}
        prop_alias = {}
        prop_values = reduced[node_address]['props'][prop]

        if isinstance(prop_values, list):
            for i, prop_value in enumerate(prop_values):
                prop_name = convert_string_to_label(prop)
                label = def_label + '_' + prop_name
                if isinstance(prop_value, str):
                    prop_value = "\"" + prop_value + "\""
                prop_def[label + '_' + str(i)] = prop_value
        else:
            prop_name = convert_string_to_label(prop)
            label = def_label + '_' + prop_name

            if prop_values == 'parent-label':
                prop_values = find_parent_prop(node_address, 'label')

            if isinstance(prop_values, str):
                prop_values = "\"" + prop_values + "\""
            prop_def[label] = prop_values

            # generate defs for node aliases
            if node_address in aliases:
                for i in aliases[node_address]:
                    alias_label = convert_string_to_label(i)
                    alias = alias_label + '_' + prop_name
                    prop_alias[alias] = label

        insert_defs(node_address, prop_def, prop_alias)

##
# @brief Management information for directives handled by default.
default = DTDefault()
