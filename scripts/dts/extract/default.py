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
# @brief Manage directives in a default way.
#
class DTDefault(DTDirective):

    @staticmethod
    def _extract_enum(node_path, prop, prop_values, label):
        cell_yaml = get_binding(node_path)['properties'][prop]
        if 'enum' in cell_yaml:
            if prop_values in cell_yaml['enum']:
                if isinstance(cell_yaml['enum'], list):
                    value = cell_yaml['enum'].index(prop_values)
                if isinstance(cell_yaml['enum'], dict):
                    value = cell_yaml['enum'][prop_values]
                label = label + "_ENUM"
                return {label:value}
            else:
                print("ERROR")
        return {}


    ##
    # @brief Extract directives in a default way
    #
    # @param node_path Path to node owning the clockxxx definition.
    # @param prop property name
    # @param prop type (string, boolean, etc)
    # @param def_label Define label string of node owning the directive.
    #
    def extract(self, node_path, prop, prop_type, def_label):
        prop_def = {}
        prop_alias = {}

        if prop_type == 'boolean':
            if prop in reduced[node_path]['props']:
                prop_values = 1
            else:
                prop_values = 0
        else:
            prop_values = reduced[node_path]['props'][prop]

        if prop_type in {"string-array", "array", "uint8-array"}:
            if not isinstance(prop_values, list):
                prop_values = [prop_values]

        if prop_type == "uint8-array":
            prop_name = str_to_label(prop)
            label = def_label + '_' + prop_name
            prop_value = ''.join(['{ ',
                                  ', '.join(["0x%02x" % b for b in prop_values]),
                                  ' }'])
            prop_def[label] = prop_value
            add_compat_alias(node_path, prop_name, label, prop_alias)
        elif isinstance(prop_values, list):
            for i, prop_value in enumerate(prop_values):
                prop_name = str_to_label(prop)
                label = def_label + '_' + prop_name
                if isinstance(prop_value, str):
                    prop_value = "\"" + prop_value + "\""
                prop_def[label + '_' + str(i)] = prop_value
                add_compat_alias(node_path,
                        prop_name + '_' + str(i),
                        label + '_' + str(i),
                        prop_alias)
        else:
            prop_name = str_to_label(prop)
            label = def_label + '_' + prop_name

            if prop_values == 'parent-label':
                prop_values = find_parent_prop(node_path, 'label')

            prop_def = self._extract_enum(node_path, prop, prop_values, label)
            if prop_def:
                add_compat_alias(node_path, prop_name + "_ENUM" , label + "_ENUM", prop_alias)
            if isinstance(prop_values, str):
                prop_values = "\"" + prop_values + "\""
            prop_def[label] = prop_values
            add_compat_alias(node_path, prop_name, label, prop_alias)

            # generate defs for node aliases
            if node_path in aliases:
                add_prop_aliases(
                    node_path,
                    lambda alias: str_to_label(alias) + '_' + prop_name,
                    label,
                    prop_alias)

        insert_defs(node_path, prop_def, prop_alias)

##
# @brief Management information for directives handled by default.
default = DTDefault()
