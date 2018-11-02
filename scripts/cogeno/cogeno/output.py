# Copyright (c) 2018 Bobby Noelte.
#
# SPDX-License-Identifier: Apache-2.0


class OutputMixin(object):
    __slots__ = []

    def _out(self, output='', dedent=False, trimblanklines=False):
        if trimblanklines and ('\n' in output):
            lines = output.split('\n')
            if lines[0].strip() == '':
                del lines[0]
            if lines and lines[-1].strip() == '':
                del lines[-1]
            output = '\n'.join(lines)+'\n'
        if dedent:
            output = reindentBlock(output)

        if self._context.script_is_python():
            self._context._outstring += output
        return output

    def msg(self, s):
        return self.prout("Message: "+s)

    def out(self, sOut='', dedent=False, trimblanklines=False):
        return self._out(sOut, dedent, trimblanklines)

    def outl(self, sOut='', **kw):
        """ The cog.outl function.
        """
        self._out(sOut, **kw)
        self._out('\n')
