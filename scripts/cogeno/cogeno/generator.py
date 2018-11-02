# Copyright 2004-2016, Ned Batchelder.
#           http://nedbatchelder.com/code/cog
# Copyright (c) 2018 Bobby Noelte.
#
# SPDX-License-Identifier: MIT

import sys
import os
import re
import imp
import inspect

from pathlib import Path

##
# Make relative import work also with __main__
if __package__ is None or __package__ == '':
    # use current directory visibility
    from whiteutils import *
    from context import Context
    from options import Options, OptionsMixin
    from lock import LockMixin
    from generic import GenericMixin
    from inlinegen import InlineGenMixin
    from pygen import PyGenMixin
    from jinja2gen import Jinja2GenMixin
    from guard import GuardMixin
    from config import ConfigMixin
    from cmake import CMakeMixin
    from paths import PathsMixin
    from edts import EDTSMixin
    from include import IncludeMixin
    from log import LogMixin
    from error import ErrorMixin, Error
    from output import OutputMixin
    from importmodule import ImportMixin
    from redirectable import RedirectableMixin
else:
    # use current package visibility
    from .whiteutils import *
    from .context import Context
    from .options import Options, OptionsMixin
    from .lock import LockMixin
    from .generic import GenericMixin
    from .inlinegen import InlineGenMixin
    from .pygen import PyGenMixin
    from .jinja2gen import Jinja2GenMixin
    from .guard import GuardMixin
    from .config import ConfigMixin
    from .cmake import CMakeMixin
    from .paths import PathsMixin
    from .edts import EDTSMixin
    from .include import IncludeMixin
    from .log import LogMixin
    from .error import ErrorMixin, Error
    from .output import OutputMixin
    from .importmodule import ImportMixin
    from .redirectable import RedirectableMixin

class CodeGenerator(OptionsMixin, LockMixin, GenericMixin, InlineGenMixin,
                    PyGenMixin, Jinja2GenMixin, ConfigMixin,
                    CMakeMixin, PathsMixin, EDTSMixin, GuardMixin,
                    IncludeMixin, LogMixin, ErrorMixin, OutputMixin,
                    ImportMixin, RedirectableMixin):

    # The cogen module
    cogeno_module = None

    # Stack of module module states
    cogeno_module_states = []

    ##
    # @brief Magic mumbo-jumbo so that imported Python modules
    #        can say "import cogeno" and get our state.
    #
    # Make us the module.
    @classmethod
    def _init_cogeno_module(cls):
        if cls.cogeno_module is None:
            cls.cogeno_module = imp.new_module('cogeno')
            cls.cogeno_module.path = []
        sys.modules['cogeno'] = cls.cogeno_module

    ##
    # @brief Save our state to the "cogeno" module.
    #
    # Prepare to restore the current state before saving
    # to the module
    def _set_cogeno_module_state(self):
        restore_state = {}
        module_states = self.cogeno_module_states
        module = self.cogeno_module
        # Code generator state
        restore_state['_context'] = getattr(module, '_context', None)
        module._context = self._context
        # Paths
        restore_state['module_path'] = module.path[:]
        restore_state['sys_path'] = sys.path[:]
        # CodeGenerator methods that are relevant to templates
        # Look for the Mixin classes.
        for base_cls in inspect.getmro(CodeGenerator):
            if "Mixin" in base_cls.__name__:
                for member_name, member_value in inspect.getmembers(base_cls):
                    if member_name.startswith('_'):
                        continue
                    if inspect.isroutine(member_value):
                        restore_state[member_name] = \
                            getattr(module, member_name, None)
                        setattr(module, member_name,
                            getattr(self, member_name))
        module_states.append(restore_state)

    def _restore_cogeno_module_state(self):
        module_states = self.cogeno_module_states
        module = self.cogeno_module
        restore_state = module_states.pop()
        # Paths
        sys.path = restore_state['sys_path']
        module.path = restore_state['module_path']
        # Code generator state
        module._context = restore_state['_context']
        # CodeGenerator methods that are relevant to templates
        # Look for the Mixin classes.
        for base_cls in inspect.getmro(CodeGenerator):
            if "Mixin" in base_cls.__name__:
                for member_name, member_value in inspect.getmembers(base_cls):
                    if member_name.startswith('_'):
                        continue
                    if inspect.isroutine(member_value):
                        setattr(module, member_name, restore_state[member_name])


    def __init__(self):
        # the actual generation context
        self._context = None
        # Create the cogeno module if not available
        self._init_cogeno_module()

    ##
    # @brief evaluate context
    #
    # Inserts the context outstring in the current context
    #
    def _evaluate_context(self, context):
        if context.parent() != self._context:
            # This should never happen
            self.error("Context '{}' with wrong parent '{}' for evaluation (expected '{}')."
                       .format(context, context.parent(), self._context),
                       frame_index = -2, lineno = 0)

        # make the new context the actual one
        self._context = context

        if self._context.parent() is None:
            # we are at toplevel context
            # Assure the modules and templates paths
            # from context are inserted to module
            for path in self._context._modules_paths:
                self.cogeno_module.path.extend(str(path))
                sys.path.extend(str(path))
            for path in self._context._templates_paths:
                pass
            # Assure the module does have our state
            self._set_cogeno_module_state()

        if self._context.template_is_file():
            template_file = self.find_file_path(context._template_file,
                                                self.templates_paths())
            if template_file is None:
                self.error("File {} not found".format(context._template_file),
                           frame_index = -2, snippet_lineno = 0)
            context._template_file = str(template_file)
            if context.script_is_jinja2():
                # Jinja2 uses its own file loader
                pass
            else:
                # Get whole file as a string
                with template_file.open(mode = "r", encoding="utf-8") as template_fd:
                    context._template = template_fd.read()

        if self._context.script_type() is None:
            # Do some heuristics to find out the template script type
            # - a cogeno file with source code and inline code generation
            # - a ninja template
            # - a pure cogeno python template
            if "@code{.cogeno" in context._template:
                # we found a cogeno marker
                self._context._script_type = "inline"
            elif "{%" in context._template:
                self._context._script_type = "jinja2"
            elif "cogeno" in context._template:
                self._context._script_type = "python"
            elif context._template_file.endswith('.in'):
                self._context._script_type = "inline"
            elif context._template_file.endswith('.py'):
                self._context._script_type = "python"
            elif context._template_file.endswith('.jinja2'):
                self._context._script_type = "jinja2"
            else:
                raise TypeError("File {} expected to be a cogeno template (template script type unknown)"
                                .format(context._template_file))

        if self._context._eval_begin is None:
            self._context._eval_begin = 0

        if self._context.script_is_inline():
            self._inline_evaluate()
        elif self._context.script_is_python():
            self._python_evaluate()
        elif self._context.script_is_jinja2():
            self._jinja2_evaluate()
        else:
            # This should never happen
            self.error("Context '{}' with unknown script type '{}' for evaluation."
                       .format(context, context.script_type()),
                       frame_index = -2)

        # switch back context
        self._context = self._context.parent()

        if self._context is None:
            # The context we evaluated is a top level context
            if context._output_file == '-':
                sys.stdout.write(context._outstring)
            else:
                with Path(context._output_file).open(mode = 'w', encoding = 'utf-8') as output_fd:
                    output_fd.write(context._outstring)
                self.log('s{}: write {}'
                    .format(len(self.cogeno_module_states), context._output_file))
            self._restore_cogeno_module_state()
        else:
            self._context.out(context._outstring)
            # Within Jinja2 context we have to return the string
            # Just do it always
            return context._outstring


