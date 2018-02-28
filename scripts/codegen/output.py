# Copyright (c) 2018 Bobby Noelte.
#
# SPDX-License-Identifier: Apache-2.0


class OutputMixin(object):
    __slots__ = []

    def msg(self, s):
        self.prout("Message: "+s)

    def out(self, sOut='', dedent=False, trimblanklines=False):
        self._out(sOut, dedent, trimblanklines)

    def outl(self, sOut='', **kw):
        """ The cog.outl function.
        """
        self._out(sOut, **kw)
        self._out('\n')
