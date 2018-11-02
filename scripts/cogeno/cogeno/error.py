# Copyright (c) 2018 Bobby Noelte.
#
# SPDX-License-Identifier: Apache-2.0

import inspect
from pathlib import Path

##
# Make relative import work also with __main__
if __package__ is None or __package__ == '':
    # use current directory visibility
    from context import Context
else:
    # use current package visibility
    from .context import Context

class Error(Exception):
    pass

class ErrorMixin(object):
    __slots__ = []

    ##
    # @brief list snippet
    def _list_snippet(self):
        template = self._context.parent()._template
        if not template:
            return None
        listing = ""
        snippet_lineno = 0 - self._context._eval_adjust
        template_file = self._context._template_file
        eval_begin = self._context.parent()._eval_begin
        eval_end = self._context.parent()._eval_end
        for i, line in enumerate(template.splitlines()):
            if i < eval_begin:
                continue
            if i >= eval_end:
                break
            if snippet_lineno < 0:
                snippet_lineno += 1
                continue
            if snippet_lineno >= 0:
                listing += "\n"
            listing += "{} line {} = #{}: {}".format(
                template_file,
                snippet_lineno - self._context._eval_adjust,
                i - self._context._eval_adjust,
                line)
            snippet_lineno += 1
        return listing

    ##
    # @brief Get code generation error exception
    #
    # @note only for 'raise cogen.Error(msg)' in template
    #
    # @param msg exception message
    # @param frame_index [optional] call frame index
    # @param lineno [optional] line number within template
    # @return code generation exception object
    #
    def _get_error_exception(self, msg, frame_index = 0,
                             lineno = 0):

        if self._context.script_is_python():
            if frame_index >= 0:
                # There are frames to get data from
                frame_index += 1
                frame = inspect.currentframe()
                try:
                    while frame_index > 0:
                        frame = frame.f_back
                        frame_index -= 1
                    (filename, lineno, function, code_context, index) = \
                        inspect.getframeinfo(frame)
                except:
                    pass
                finally:
                    del frame
            if self._context.template_is_snippet():
                lineno = int(lineno)
                template_lineno = self._context.parent()._eval_begin \
                                  + lineno + self._context._eval_adjust
                error_msg = "{} line {} = #{}: {}".format(
                    self._context._template_file, lineno,
                    template_lineno, msg)
                listing = self._list_snippet()
                if listing:
                    error_msg = listing + '\n' + error_msg
            else:
                error_msg = msg

        else:
            error_msg = msg

        return Error(error_msg)

    ##
    # @brief Raise Error exception.
    #
    # Extra information is added that maps the python snippet
    # line seen by the Python interpreter to the line of the file
    # that inlines the python snippet.
    #
    # @param msg [optional] exception message
    # @param frame_index [optional] call frame index
    # @param lineno [optional] line number within template
    #
    def error(self, msg = 'Error raised by cogen generator.',
              frame_index = 0, lineno = 0):
        frame_index += 1
        raise self._get_error_exception(msg, frame_index, lineno)
