# Copyright (c) 2018 Bobby Noelte.
#
# SPDX-License-Identifier: Apache-2.0

from pathlib import Path
import io

from .error import Error

class IncludeMixin(object):
    __slots__ = []

    def out_include(self, include_file):
        try:
            input_file = Path(include_file).resolve()
        except FileNotFoundError:
            # Python 3.4/3.5 will throw this exception
            # Python >= 3.6 will not throw this exception
            input_file = Path(include_file)
        if not input_file.is_file():
            # don't resolve upfront
            input_file = Path(include_file)
            # try to find the file in the templates directory
            expanded_file_path = self.zephyr_path().joinpath(
                                                "scripts/codegen/templates")
            if 'templates' in input_file.parts:
                templates_seen = False
            else:
                # assume the path starts after templates
                templates_seen = True
            # append the remaining part behind templates
            for part in input_file.parts:
                if templates_seen:
                    expanded_file_path = expanded_file_path.joinpath(part)
                elif part is 'templates':
                    templates_seen = True
            if expanded_file_path.is_file():
                input_file = expanded_file_path
        if not input_file.is_file():
            raise self._get_error_exception(
                "Include file '{}' does not exist or is no file.".
                format(input_file), 1)
        if str(input_file) in self.generator_globals['_guard_include']:
            self.log('------- include guarded {} - multiple inclusion of include file.'.
                     format(str(input_file)))
        else:
            output_fd = io.StringIO()
            # delete inline code in included files
            delete_code = self.processor.options.bDeleteCode
            self.processor.options.bDeleteCode = True
            self.log('------- include start {}'.format(input_file))
            self.processor.process_file(str(input_file), output_fd,
					globals=self.generator_globals)
            self.log(output_fd.getvalue())
            self.log('------- include   end {}'.format(input_file))
            self.processor.options.bDeleteCode = delete_code
            self.out(output_fd.getvalue())

    def guard_include(self):
        if self._snippet_file in self.generator_globals['_guard_include']:
            # This should never happen
            raise self._get_error_exception(
                "Multiple inclusion of guarded include file '{}'.".
                format(self._snippet_file), 1)
        self.log('------- include guard {}'.format(self._snippet_file))
        self.generator_globals['_guard_include'].append(self._snippet_file)

