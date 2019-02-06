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
    # @param prop property name
    # @param prop type (string, boolean, etc)
    # @param def_label Define label string of node owning the directive.
    #
    def extract(self, node_address, prop, prop_type, def_label):
        prop_def = {}
        prop_alias = {}

        if prop_type == 'boolean':
            if prop in reduced[node_address]['props']:
                prop_values = 1
            else:
                prop_values = 0
        else:
            prop_values = reduced[node_address]['props'][prop]

        if isinstance(prop_values, list):
            for i, prop_value in enumerate(prop_values):
                prop_name = str_to_label(prop)
                label = def_label + '_' + prop_name
                if isinstance(prop_value, str):
                    prop_value = "\"" + prop_value + "\""
                prop_def[label + '_' + str(i)] = prop_value
                add_compat_alias(node_address,
                        prop_name + '_' + str(i),
                        label + '_' + str(i),
                        prop_alias)
        else:
            prop_name = str_to_label(prop)
            label = def_label + '_' + prop_name

            if prop_values == 'parent-label':
                prop_values = find_parent_prop(node_address, 'label')

            if isinstance(prop_values, str):
                prop_values = "\"" + prop_values + "\""
            prop_def[label] = prop_values
            add_compat_alias(node_address, prop_name, label, prop_alias)

            # generate defs for node aliases
            if node_address in aliases:
                add_prop_aliases(
                    node_address,
                    lambda alias: str_to_label(alias) + '_' + prop_name,
                    label,
                    prop_alias)

        insert_defs(node_address, prop_def, prop_alias)

##
# @brief Management information for directives handled by default.
default = DTDefault()
