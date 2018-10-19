#
# Copyright (c) 2018 Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

##
# @brief DTS aliases
#
# Methods for aliases.
#
class DTSAliasesMixin(object):
    __slots__ = []

    def _init_aliases(self, root):
        if 'children' in root:
            if 'aliases' in root['children']:
                for k, v in root['children']['aliases']['props'].items():
                    self.aliases[v].append(k)

        # Treat alternate names as aliases
        for k in self.reduced.keys():
            if self.reduced[k].get('alt_name', None) is not None:
                self.aliases[k].append(self.reduced[k]['alt_name'])
