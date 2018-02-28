# Copyright (c) 2018 Bobby Noelte.
#
# SPDX-License-Identifier: Apache-2.0

import inspect
from pathlib import Path

class Error(Exception):
    pass

class ErrorMixin(object):
    __slots__ = []

    ##
    # @brief Get code generation error exception
    #
    # @note only for 'raise codegen.Error(msg)' in snippet
    #
    # @param msg exception message
    # @param frame_index [optional] call frame index
    # @param snippet_lineno [optional] line number within snippet
    # @return code generation exception object
    #
    def _get_error_exception(self, msg, frame_index = 0,
                             snippet_lineno = 0):
        if frame_index >= 0:
            # There are frames to get data from
            frame_index += 1
            frame = inspect.currentframe()
            try:
                while frame_index > 0:
                    frame = frame.f_back
                    frame_index -= 1
                (filename, snippet_lineno, function, code_context, index) = \
                    inspect.getframeinfo(frame)
            except:
                pass
            finally:
                del frame
        input_lineno = self._snippet_offset + self._get_snippet_lineno(snippet_lineno)
        error_msg = "{} #{} ({}, line {}): {}".format(
            Path(self._snippet_file).name,
            input_lineno, self._get_snippet_id(),
            snippet_lineno, msg)
        listing = self._list_snippet()
        if listing:
            error_msg = listing + '\n' + error_msg
        else:
            listing = self._list_lines()
            if listing:
                error_msg= listing + '\n' + error_msg
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
    # @param snippet_lineno [optional] line number within snippet
    #
    def error(self, msg = 'Error raised by codegen generator.',
              frame_index = 0, snippet_lineno = 0):
        frame_index += 1
        raise self._get_error_exception(msg, frame_index, snippet_lineno)
