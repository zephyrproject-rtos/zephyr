#
# Copyright (c) 2018 Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

##
# @brief DTS reduced
#
# Methods for reduced.
#
class DTSReducedMixin(object):
    __slots__ = []

    def _init_reduced(self, nodes, path = '/'):
        # compress nodes list to nodes w/ paths, add interrupt parent
        if 'props' in nodes:
            status = nodes['props'].get('status')

            if status == "disabled":
                return

        if isinstance(nodes, dict):
            self.reduced[path] = dict(nodes)
            self.reduced[path].pop('children', None)
            if path != '/':
                path += '/'
            if nodes['children']:
                for k, v in nodes['children'].items():
                    self._init_reduced(v, path + k)
