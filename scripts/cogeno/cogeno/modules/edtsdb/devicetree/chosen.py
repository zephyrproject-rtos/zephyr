#
# Copyright (c) 2018 Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

##
# @brief DTS chosen
#
# Methods for chosen.
#
class DTSChosenMixin(object):
    __slots__ = []

    def _init_chosen(self, root):
        if 'children' in root:
            if 'chosen' in root['children']:
                for k, v in root['children']['chosen']['props'].items():
                    self.chosen[k] = v
