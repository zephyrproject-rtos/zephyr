#
# Copyright (c) 2018 Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

##
# @brief DTS phandles
#
# Methods for phandles.
#
class DTSPHandlesMixin(object):
    __slots__ = []

    def _init_phandles(self, root, name = '/'):
        if 'props' in root:
            handle = root['props'].get('phandle')
            enabled = root['props'].get('status')

        if enabled == "disabled":
            return

        if handle is not None:
            self.phandles[handle] = name

        if name != '/':
            name += '/'

        if isinstance(root, dict):
            if root['children']:
                for k, v in root['children'].items():
                    self._init_phandles(v, name + k)
