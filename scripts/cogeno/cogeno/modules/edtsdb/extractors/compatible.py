#
# Copyright (c) 2018 Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

from .directive import DTDirective

##
# @brief Manage compatible directives.
#
# Handles:
# - compatible
#
# Generates in EDTS:
# - compatible/<index> : compatible
class DTCompatible(DTDirective):

    def __init__(self, extractors):
        super().__init__(extractors)

    ##
    # @brief Extract compatible
    #
    # @param node_address Address of node issuing the directive.
    # @param prop Directive property name
    #
    def extract(self, node_address, prop):

        # compatible definition
        compatible = self.dts().reduced[node_address]['props'][prop]
        if not isinstance(compatible, list):
            compatible = [compatible, ]

        # generate EDTS
        device_id = self.edts_device_id(node_address)
        for i, comp in enumerate(compatible):
            self.edts().insert_device_property(device_id,
                                              'compatible/{}'.format(i),
                                               comp)
