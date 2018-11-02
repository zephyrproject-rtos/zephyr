# Copyright (c) 2018 Nordic Semiconductor ASA
# Copyright (c) 2018 Bobby Noelte.
#
# SPDX-License-Identifier: Apache-2.0

import sys
from traceback import TracebackException

##
# Make relative import work also with __main__
if __package__ is None or __package__ == '':
    # use current directory visibility
    from error import Error
    from context import Context
else:
    # use current package visibility
    from .error import Error
    from .context import Context

class PyGenMixin(object):
    __slots__ = []

    ##
    # @brief evaluate
    #
    def _python_evaluate(self):
        if not self._context.script_is_python():
            # This should never happen
            self.error("Unexpected context '{}' for python evaluation."
                       .format(self._context.script_type()),
                       frame_index = -2, snippet_lineno = 0)

        self.log('s{}: process python template {}'
                 .format(len(self.cogeno_module_states),
                         self._context._template_file))

        # we add an extra line 'import cogeno'
        # account for that
        self._context._eval_adjust = -1

        # In Python 2.2, the last line has to end in a newline.
        eval_code = "import cogeno\n" + self._context._template + "\n"

        try:
            code = compile(eval_code, self._context._template_file, 'exec')
        except:
            exc_type, exc_value, exc_tb = sys.exc_info()
            exc_traceback = TracebackException(exc_type, exc_value, exc_tb)
            self.error(
                "compile exception '{}' within template {}".format(
                    exc_value, self._context._template_file),
                frame_index = -2,
                lineno = exc_traceback.lineno)

        # Make sure the "cogeno" module has our state.
        self._set_cogeno_module_state()

        self._context._outstring = ''
        try:
            eval(code, self._context.generation_globals())
        except:
            exc_type, exc_value, exc_tb = sys.exc_info()
            if exc_type is Error:
                # Exception raise by cogeno means
                raise
            # Not raised by cogeno means - add some info
            print("Cogeno: eval exception within cogeno snippet '{}' in '{}'.".format(
                  self._context._template_file, self._context.parent()._template_file))
            print(self._list_snippet())
            raise
        finally:
            self._restore_cogeno_module_state()

        # We need to make sure that the last line in the output
        # ends with a newline, or it will be joined to the
        # end-output line, ruining cogen's idempotency.
        if self._context._outstring and self._context._outstring[-1] != '\n':
            self._context._outstring += '\n'

        # end of evaluation - no extra offset anymore
        self._eval_adjust = 0
