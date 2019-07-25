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

    ##
    # @brief Extract directive information.
    #
    # @param node_path Path to node issuing the directive.
    # @param prop Directive property name
    # @param def_label Define label string of node owning the directive.
    #
    def extract(self, node_path, prop, def_label):
        pass
