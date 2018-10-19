#
# Copyright (c) 2018 Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

from .directive import DTDirective

##
# @brief Manage <device>-controller directives.
#
# Handles:
# - <device>-controller
#
# Generates in EDTS:
# - <device>-controller : True
class DTController(DTDirective):

    def __init__(self, extractors):
        super().__init__(extractors)

    ##
    # @brief Extract <device>-controller
    #
    # @param node_address Address of node issuing the directive.
    # @param prop Directive property name
    #
    def extract(self, node_address, prop):

        # generate EDTS
        device_id = self.edts_device_id(node_address)
        self.edts().insert_device_property(device_id, prop, True)
