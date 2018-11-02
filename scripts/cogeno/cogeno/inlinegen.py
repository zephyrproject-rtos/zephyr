#
# Copyright (c) 2018 Bobby Noelte.
#
# SPDX-License-Identifier: Apache-2.0

import os
import re

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

class InlineGenMixin(object):
    __slots__ = []

    _inline_python_start_marker = "@code{.cogeno.py}"
    _inline_python_end_marker = "@endcode{.cogeno.py}"
    _inline_jinja2_start_marker = "@code{.cogeno.jinja2}"
    _inline_jinja2_end_marker = "@endcode{.cogeno.jinja2}"
    _inline_insert_marker = "@code{.cogeno.ins}@endcode"

    # code snippet markers and lines
    _inline_start_markers = (_inline_python_start_marker,
                             _inline_jinja2_start_marker)
    _inline_end_markers = (_inline_python_end_marker,
                           _inline_jinja2_end_marker)
    _inline_insert_markers = (_inline_insert_marker,)

    def _inline_start_marker(self, line):
        for marker in self._inline_start_markers:
            if marker in line:
                return marker
        return None

    def _inline_end_marker(self, line):
        for marker in self._inline_end_markers:
            if marker in line:
                return marker
        return None

    def _inline_insert_marker(self, line):
        for marker in self._inline_insert_markers:
            if marker in line:
                return marker
        return None

    def _inline_is_start_marker(self, line):
        if self._inline_start_marker(line) is not None:
            return True
        return False

    def _inline_is_end_marker(self, line):
        if self._inline_end_marker(line) is not None:
            return True
        return False

    def _inline_is_insert_marker(self, line):
        if self._inline_insert_marker(line) is not None:
            return True
        return False

    def _inline_sanitize_line(self, line):
        sanitized = line.expandtabs(self._context._template_tabsize).strip('\n')
        return sanitized

    @staticmethod
    def _inline_common_start(sa, sb):
        """ returns the longest common substring from the beginning of sa and sb """
        def _iter():
            for a, b in zip(sa, sb):
                if a == b:
                    yield a
                else:
                    return
        return ''.join(_iter())

    @staticmethod
    def _inline_common_prefix(lines):
        if not lines:
            return ''
        while len(lines) > 1:
            s1 = min(lines)
            s2 = max(lines)
            for i, c in enumerate(s1):
                if c != s2[i]:
                    return s1[:i]
            # We have a only prefix line (full s1)
            # Remove all only prefix lines
            # and try again if all lines start with space after the prefix
            super_prefix_lines = []
            for line in lines:
                if len(line) > len(s1):
                    if not line[len(s1)].isspace():
                        return s1
                    super_prefix_lines.append(line)
            lines = super_prefix_lines
        else:
            return ''


    ##
    # @brief Re-indent a code block.
    #
    # Take a block of text as a string or list of lines.
    # Remove any common prefix and whitespace indentation.
    # Re-indent using new_indent
    #
    # @param lines
    # @param common_prefix
    # @param new_indent
    # @return indented code block as a single string.
    def _inline_reindent_code(self, lines, common_prefix = '', new_indent=''):
        if not isinstance(lines, list):
            lines = lines.splitlines()
        prefix_len = len(common_prefix)
        code = ''
        for line in lines:
            if len(line) <= prefix_len:
                code += '\n'
            else:
                code += new_indent + line[prefix_len:] + '\n'
        return code

    ##
    # @brief Extract the executable script code from the snippet.
    #
    def _inline_snippet_code(self, fname, snippet, marker_lines):
        # If the markers and lines all have the same prefix
        # (end-of-line comment chars, for example),
        # then remove it from all the lines.
        lines = snippet.splitlines()
        common_prefix = self._inline_common_prefix(marker_lines + lines)
        if not common_prefix:
            # there may be a prefix error
            # just do some heuristics
            if fname.endswith(
                ('.h', '.hxx', '.c', '.cpp', '.cxx')):
                # assume C/C++ comments
                for line in (marker_lines + lines):
                    if line.strip().startswith('*'):
                        common_prefix = '*'
                        break
                    elif line.strip().startswith('//'):
                        common_prefix = '//'
                        break
            if common_prefix:
                # there should be a common prefix -> error
                common_lines = list() # lines with correct prefix
                template_eval_begin = self._context._eval_begin
                for lineno, line in enumerate(lines):
                    if not line.strip().startswith(common_prefix):
                        print("Cogeno: Common prefix '{}' may miss in cogeno snippet '{}' ({}) {}.".format(
                            common_prefix, fname, lineno, template_eval_begin + lineno))
                        line_start = lineno - 5
                        if line_start < 0:
                            line_start = 0
                        line_end = lineno + 5
                        if line_end > len(lines):
                            line_end = len(lines)
                        for snippet_lineno in range(line_start, line_end):
                            template_lineno = template_eval_begin + snippet_lineno
                            print("#{} ({}): {}".format(
                                template_lineno, snippet_lineno, lines[snippet_lineno]))
                    else:
                        common_lines.append(line)
                if len(common_lines) >= int(len(lines) / 2):
                    common_prefix = os.path.commonprefix(common_lines)
                    print("Cogeno: Assuming common prefix '{}' for cogeno snippet '{}' in '{}'.".format(
                          common_prefix, fname, self._context._template_file))
                else:
                    common_prefix = ''
        code = self._inline_reindent_code(lines, common_prefix)
        return code

    ##
    # @brief evaluate inline template
    #
    def _inline_evaluate(self):
        if not self._context.script_is_inline():
            # This should never happen
            self.error("Unexpected context '{}' for inline evaluation."
                       .format(self._context.script_type()),
                       frame_index = -2, lineno = 0)

        self.log('s{}: process inline template {}'
                 .format(len(self.cogeno_module_states),
                         self._context._template_file))

        state = 'text'
        lines = self._context._template.splitlines()
        snippet_context = None
        marker_lines = None
        for line_no, line in enumerate(lines):
            if line_no < self._context._eval_begin:
                # Someone exclude parts of the template
                continue
            if state == 'text':
                if self._inline_is_start_marker(line):
                    self._context._eval_begin = line_no + 1
                    self._context._eval_end = len(lines)
                    # prepare context for snippet processing
                    marker_lines = [self._inline_sanitize_line(line),]
                    if self._inline_start_marker(line) \
                        == self._inline_python_start_marker:
                        script_type = 'python'
                    else:
                        script_type = 'jinja2'
                    if self._inline_is_end_marker(line):
                        # within the start marker line is also and end marker
                        self._context._eval_begin = line_no
                        self._context._eval_end = line_no + 1
                        line = self._inline_sanitize_line(line)
                        # single line snippets contain the markers - remove them
                        start_marker = self._inline_start_marker(line)
                        end_marker = self._inline_end_marker(line)
                        begin = line.find(start_marker)
                        end = line.find(end_marker)
                        if begin > end:
                            self.error("Cogeno code markers inverted",
                                       frame_index = -2)
                        else:
                            template = line[begin + len(start_marker): end].strip()
                            snippet_context._template += template + '\n'
                        if self._context._delete_code:
                            # cut out template code
                            line = line[:begin] + line[end + len(end_marker):]
                        self._context.outl(line)
                        state = 'snippet_insert'
                    else:
                        snippet_context = Context(self, \
                            parent_context = self._context,
                            template_file = "{}#{}".format(self._context._template_file, line_no + 1),
                            template = '',
                            script_type = script_type,
                            template_source_type = 'snippet',
                            eval_begin = 0)
                        if not self._context._delete_code:
                             self._context.outl(line)
                        state = 'snippet_template'
                elif self._inline_is_end_marker(line):
                    self._context._eval_end = line_no + 1
                    self.error("Unexpected end marker in '{}'".format(line),
                                frame_index=-1, lineno=line_no)
                elif self._inline_is_insert_marker(line):
                    self._context._eval_end = line_no + 1
                    self.error("Unexpected insert marker in '{}'".format(line),
                                frame_index=-1, lineno=line_no)
                else:
                    self._context.outl(line)
            elif state == 'snippet_template':
                if self._inline_is_start_marker(line):
                    self._context._eval_end = line_no + 1
                    self.error("Unexpected start marker in '{}'".format(line),
                                frame_index=-1, lineno=line_no)
                elif self._inline_is_end_marker(line):
                    self._context._eval_end = line_no
                    if not self._context._delete_code:
                        self._context.outl(line)
                    marker_lines.append(self._inline_sanitize_line(line))
                    # We now have the template code.
                    # The following line maybe a trailing comment line
                    trailing_line = lines[line_no + 1]
                    common_start = self._inline_common_start(line,
                                                             trailing_line)
                    if (len(common_start) == 0) or common_start.isspace():
                        state = 'snippet_insert'
                    elif '@code' in trailing_line:
                        state = 'snippet_insert'
                    elif len(common_start) == (len(trailing_line) - 1):
                        state = 'snippet_template_trailing'
                    elif len(common_start) == len(trailing_line):
                        state = 'snippet_template_trailing'
                    else:
                        state = 'snippet_insert'
                elif self._inline_is_insert_marker(line):
                    self._context._eval_end = line_no + 1
                    self.error("Unexpected insert marker in '{}'".format(line),
                                frame_index=-1, lineno=line_no)
                else:
                    if not self._context._delete_code:
                        self._context.outl(line)
                    # expand tabs now as we may have to remove the common prefix
                    # which would change tab handling
                    line = self._inline_sanitize_line(line)
                    snippet_context._template += line + '\n'
            elif state == 'snippet_template_trailing':
                self._context.outl(line)
                state = 'snippet_insert'
            elif state == 'snippet_insert':
                if self._inline_is_start_marker(line):
                    self._context._eval_end = line_no + 1
                    self.error("Unexpected start marker in '{}'".format(line),
                                frame_index=-1, lineno=line_no)
                elif self._inline_is_end_marker(line):
                    self._context._eval_end = line_no + 1
                    self.error("Unexpected end marker in '{}'".format(line),
                                frame_index=-1, lineno=line_no)
                elif self._inline_is_insert_marker(line):
                    state = 'text'
                    code = self._inline_snippet_code( \
                                snippet_context._template_file,
                                snippet_context._template,
                                marker_lines)
                    if code and \
                       not self._context._options.bNoGenerate and \
                       self._context._globals['_generate_code']:
                        snippet_context._template = code
                        # let the devils work - evaluate context
                        # inserts the context output into the current context
                        self._evaluate_context(snippet_context)
                        # We need to make sure that the last line in the output
                        # ends with a newline, or it will be joined to the
                        # end-output line, ruining cogeno's idempotency.
                        if self._context._outstring \
                            and self._context._outstring[-1] != '\n':
                            self._context.outl('')
                        if not self._context._delete_code:
                            self._context.outl(line)
                    self._context._eval_begin = line_no + 1
                    self._context._eval_end = len(lines)
                else:
                    # we ignore the code in between end marker and insert marker
                    pass

        if snippet_context is None:
            # No snippet found - otherwise there would be a context
            if self.options.bWarnEmpty:
                self.warning("No inline code found in {}"
                             .format(self._context._template_file))
        elif state != 'text':
            # The snippet was not fully evaluated
            self.error("Missing '{}' or '{}' before end of template."
                        .format(self._inline_end_markers, self._inline_insert_markers),
                        frame_index = -2,
                        lineno = self._context._eval_begin)
