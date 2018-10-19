#
# Copyright (c) 2018 Bobby Noelte
#
# SPDX-License-Identifier: Apache-2.0
#

##
# @brief DTS compatibles
#
# Methods for compatibles.
#
class DTSCompatiblesMixin(object):
    __slots__ = []

    def _init_compatibles(self, d, name = '/'):
        if 'props' in d:
            compat = d['props'].get('compatible')
            enabled = d['props'].get('status')

        if enabled == "disabled":
            return

        if compat is not None:
            self.compatibles[name] = compat

        if name != '/':
            name += '/'

        if isinstance(d, dict):
            if d['children']:
                for k, v in d['children'].items():
                    self._init_compatibles(v, name + k)
