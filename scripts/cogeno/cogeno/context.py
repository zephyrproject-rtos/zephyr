# Copyright (c) 2018 Bobby Noelte.
#
# SPDX-License-Identifier: Apache-2.0


from pathlib import Path

##
# Make relative import work also with __main__
if __package__ is None or __package__ == '':
    # use current directory visibility
    from options import Options
else:
    # use current package visibility
    from .options import Options

##
# @brief Context for code generation
#
class Context(object):

    _suffix_replacements = {
        ".h.in" : ".h",
        ".h.py" : ".h",
        ".h.cogeno" : ".h",
        ".h.jinja2" : ".h",
        ".c.in" : ".c",
        ".c.py" : ".c",
        ".c.cogeno" : ".c",
        ".c.jinja2" : ".c",
        ".html.in" : ".html",
        ".html.py" : ".html",
        ".html.cogeno" : ".html",
        ".html.jinja2" : ".html",
    }

    #
    # @brief Get template file from options
    #
    def _options_template_file(self):
        if self._options.input_file is None:
            # No input file given
            self.error("No input file given (expected cogeno template).\n" +
                        "'{}'.".format(self._options),
                        frame_index = -2)
        input_file = self._options.input_file
        try:
            template_file = Path(input_file).resolve()
        except FileNotFoundError:
            # Python 3.4/3.5 will throw this exception
            # Python >= 3.6 will not throw this exception
            template_file = Path(input_file)
        if not template_file.is_file():
            # Input does not exist
            self.error("Input file '{}' does not exist."
                        .format(input_file),
                        frame_index = -2)
        return str(template_file)

    #
    # @brief Get output file from options
    #
    def _options_output_file(self):
        if self._options.sOutputName is None:
            return '-'
        if self._options.sOutputName == '-':
            return self._options.sOutputName
        output_name = self._options.sOutputName
        try:
            output_file = Path(output_name).resolve()
        except FileNotFoundError:
            # Python 3.4/3.5 will throw this exception
            # Python >= 3.6 will not throw this exception
            output_file = Path(output_name)
        if not output_file.parent.is_dir():
            # directory does not exist
            self.error("Output directory '{}' does not exist."
                        .format(output_file.parent),
                        frame_index = -2)
        # Check for some common output endings that stem from
        # input file name copy
        output_file = str(output_file)
        for suffix in self._suffix_replacements.keys():
            if output_file.endswith(suffix) \
               and self._template_file.endswith(suffix):
                output_file = output_file.replace(suffix,
                                                  self._suffix_replacements[suffix])
                break
        return output_file

    #
    # @brief Get log file from options
    #
    def _options_log_file(self):
        if self._options.log_file is None:
            return '-'
        if self._options.log_file == '-':
            return self._options.log_file
        log_name = self._options.log_file
        try:
            log_file = Path(log_name).resolve()
        except FileNotFoundError:
            # Python 3.4/3.5 will throw this exception
            # Python >= 3.6 will not throw this exception
            log_file = Path(log_name)
        if not log_file.parent.is_dir():
            # directory does not exist
            self.error("Log directory '{}' does not exist."
                        .format(log_file.parent),
                        frame_index = -2)
        log_file = str(log_file)
        return log_file

    #
    # @brief Get lock file from options
    #
    def _options_lock_file(self):
        if self._options.lock_file is None:
            return None
        lock_name = self._options.lock_file
        try:
            lock_file = Path(lock_name).resolve()
        except FileNotFoundError:
            # Python 3.4/3.5 will throw this exception
            # Python >= 3.6 will not throw this exception
            lock_file = Path(lock_name)
        if not lock_file.parent.is_dir():
            # directory does not exist
            self.error("Lock directory '{}' does not exist."
                        .format(lock_file.parent),
                        frame_index = -2)
        lock_file = str(lock_file)
        return lock_file

    def __init__(self,
                generator,
                parent_context = None,
                generation_globals = None,
                options = None,
                eval_begin = None,
                eval_end = None,
                eval_adjust = None,
                delete_code = None,
                template_file = None,
                template = None,
                template_source_type = None,
                script_type = None,
                template_tabsize = None,
                templates_paths = None,
                modules_paths = None,
                jinja2_environment = None,
                output_file = None,
                log_file = None,
                lock_file = None):
        # The code generator that will use this context
        self._generator = generator
        # Only the top level template does not have a parent
        self._parent = parent_context
        # Code generation usually works on the same global namespace
        self._globals = generation_globals
        # Options usually from the top level template
        self._options = options
        # The output of this context
        self._outstring = ''
        # The path/name of the output file
        self._output_file = output_file
        # The path/name of the log file
        self._log_file = log_file
        # The path/name of the lock file (concurrent processes of cogeno)
        self._lock_file = lock_file
        # the current evaluation begin and end offset in the template
        self._eval_begin = eval_begin
        self._eval_end = eval_end
        # value to adjust line numbers reported by script compilation
        # and execution
        self._eval_adjust = eval_adjust
        # Delete the generator code from the output
        self._delete_code = delete_code
        # The path of the template file or name of template
        self._template_file = template_file
        # The template this context works on
        self._template = template
        # The template source type ('snippet', "file", 'string', None)
        self._template_source_type = template_source_type
        # The template script type ('inline', "python", 'jinja2', None)
        self._script_type = script_type
        # The tabsize in thew template code
        self._template_tabsize = template_tabsize
        # Paths to search template files
        self._templates_paths = []
        # Paths to search Python modules
        self._modules_paths = []
        # Jinja2 environment for Jinja2 script execution
        self._jinja2_environment = jinja2_environment

        # init unknown generation _globals to parent scope if possible
        if self._globals is None:
            if self._parent is None:
                self._globals = {}
                # list of include files that are guarded against inclusion
                self._globals['_guard_include'] = []
                # global flag for code generation
                self._globals['_generate_code'] = True
            else:
                self._globals = self._parent._globals
        # init unknown options to parent scope if possible
        if self._options is None:
            if self._parent is None:
                # use default options
                self._options = Options()
            else:
                self._options = self._parent._options
        # --
        # from now on we can use the options to read values that are not given
        # --

        # init unknown template
        if self._template_file is None:
            if self._parent is None:
                self._template_file = self._options_template_file()
                self._template_source_type = 'file'
            else:
                self._parent._template_file
                self._template_source_type = self._parent._template_source_type
        # init unknown output file to parent scope if possible
        if self._output_file is None:
            if self._parent is None:
                self._output_file = self._options_output_file()
            else:
                self._output_file = self._parent._output_file
        # init unknown lock file to parent scope if possible
        if self._log_file is None:
            if self._parent is None:
                self._log_file = self._options_log_file()
            else:
                self._log_file = self._parent._log_file
        # init unknown lock file to parent scope if possible
        if self._lock_file is None:
            if self._parent is None:
                self._lock_file = self._options_lock_file()
            else:
                self._lock_file = self._parent._lock_file
        # init unknown eval_adjust value
        if self._eval_adjust is None:
            self._eval_adjust = 0
        # init unknown delete code value
        if self._delete_code is None:
            if self._parent is None:
                # use default options
                self._delete_code = self._options.delete_code
            else:
                self._delete_code = self._parent._delete_code
        # init unknown tabsize to parent scope if possible
        if self._template_tabsize is None:
            if self._parent is None:
                # use default options
                self._template_tabsize = 8
            else:
                self._template_tabsize = self._parent._template_tabsize
        # init modules paths
        if modules_paths is None:
            if self._parent is None:
                modules_paths = self._options.modules_paths
            else:
                modules_paths = self._parent._modules_paths
        if modules_paths:
            if not isinstance(modules_paths, list):
                modules_paths = [modules_paths,]
            for path in modules_paths:
                self._modules_paths.append(path)
        self._modules_paths.append(Path(generator.cogeno_path(), 'modules'))
        # init templates paths
        if templates_paths is None:
            if self._parent is None:
                templates_paths = self._options.templates_paths
            else:
                templates_paths = self._parent._templates_paths
        if templates_paths:
            if not isinstance(templates_paths, list):
                templates_paths = [templates_paths,]
            for path in templates_paths:
                self._templates_paths.append(path)
        self._templates_paths.append(Path(generator.cogeno_path(), 'templates'))
        # init jinja2 environment
        # Jinja2 environment will only be created if there is a Jinja2 use.
        if self._jinja2_environment is None:
            if not self._parent is None:
                self._jinja2_environment = self._parent._jinja2_environment

    def __str__(self):
        sb = []
        for key in self.__dict__:
            sb.append("{key}='{value}'".format(key=key, value=self.__dict__[key]))
        return ', '.join(sb)

    def __repr__(self):
        return self.__str__()

    def parent(self):
        return self._parent

    def generation_globals(self):
        return self._globals

    def script_is_inline(self):
        return self._script_type == 'inline'

    def script_is_python(self):
        return self._script_type == 'python'

    def script_is_jinja2(self):
        return self._script_type == 'jinja2'

    def script_type(self):
        return self._script_type

    ##
    # @brief Template is a snippet.
    #
    # Snippets are parts of the template of
    # the parent context.
    #
    # @return True in case the template is a snippet,
    #         False otherwise.
    def template_is_snippet(self):
        return self._template_source_type == 'snippet'

    def template_is_file(self):
        return self._template_source_type == 'file'

    def template_is_string(self):
        return self._template_source_type == 'string'

    ##
    # @brief Add line
    def out(self, line):
        self._outstring += line

    ##
    # @brief Add line with newline
    def outl(self, line):
        self._outstring += line + '\n'

