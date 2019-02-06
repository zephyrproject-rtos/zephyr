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
        return str_to_label('_'.join(x.strip() for x in label if x.strip()))

    def __init__():
        pass

    ##
    # @brief Extract directive information.
    #
    # @param node_address Address of node issueuing the directive.
    # @param prop Directive property name
    # @param def_label Define label string of node owning the directive.
    #
    def extract(self, node_address, prop, def_label):
        pass
