#
# Copyright (c) 2018 Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

from .globals import *

##
# @brief Base class for device tree directives
#
class DTDirective(object):

    ##
    # @brief Get a label string for a list of label sub-strings.
    #
    # Label sub-strings are concatenated by '_'.
    #
    # @param label List of label sub-strings
    # @return label string
    #
    @staticmethod
    def get_label_string(label):
        return convert_string_to_label(
            '_'.join(x.strip() for x in label if x.strip()))


    ##
    # @brief Get label string for a node by address
    #
    # @param node_address Address of node
    # @return label string
    #
    @staticmethod
    def get_node_label_string(node_address):
        node = reduced[node_address]
        node_compat = get_compat(node_address)
        if node_compat is None:
            raise Exception(
                "No compatible property for node address {}."
                    .format(node_address))
        label = convert_string_to_label(node_compat.upper())
        if '@' in node_address:
            label += '_' + node_address.split('@')[-1].upper()
        elif 'reg' in node['props']:
            label += '_' + hex(node['props']['reg'][0])[2:].zfill(8)
        else:
            label += convert_string_to_label(node_address.upper())
        return label

    def __init__():
        pass

    ##
    # @brief Extract directive information.
    #
    # @param node_address Address of node issueuing the directive.
    # @param yaml YAML definition for the node.
    # @param prop Directive property name
    # @param names Names assigned to directive.
    # @param def_label Define label string of node owning the directive.
    #
    def extract(self, node_address, yaml, prop, names, def_label):
        pass
