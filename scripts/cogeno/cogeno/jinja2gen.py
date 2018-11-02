#
# Copyright (c) 2018 Nordic Semiconductor ASA
# Copyright (c) 2018 Bobby Noelte.
#
# SPDX-License-Identifier: Apache-2.0

import inspect
from pathlib import Path
from jinja2 import (Environment, ChoiceLoader, FileSystemLoader, BaseLoader,
                    TemplateSyntaxError, TemplateAssertionError)

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

class Jinja2GenMixin(object):
    __slots__ = []

    class Jinja2SnippetLoader(BaseLoader):
        ##
        # Pool of known snippets
        snippet_templates = dict()

        def __init__(self, template_name = None, template_code = None):
            if not (template_name is None):
                self.snippet_templates[template_name] = template_code

        def get_source(self, environment, template):
            if not template in self.snippet_templates:
                raise TemplateNotFound(template)
            return self.snippet_templates[template], None, True

    ##
    # @brief Render a Ninja2 template.
    #
    # @param template_spec
    # @param data
    def render(self, template_spec, data = None):
        render_output = ""
        env = self._jinja2_environment()
        if data is None:
            data = self._jinja2_data()

        # Save proxy connection to the generator cogeno module
        cogeno_proxy_save = env.globals.get('cogeno', None)

        try:
            # Provide our generator API to Jinja2 templates
            env.globals.update(self._context.generator_globals())

            # Assure there is a cogeno proxy object to this generator
            # cogeno module available to the template
            env.globals.update(cogeno = self.cogeno_module.__dict__)

            if (template_spec in self.Jinja2SnippetLoader.snippet_templates):
                template_name = template_spec
                template = env.get_template(template_spec)
            elif Path(template_spec).is_file():
                template_name = template_spec
                template = env.get_template(template_spec)
            elif isinstance(template_spec, str):
                template_name = 'string'
                template = env.from_string(template_spec)
            else:
                raise self._get_error_exception("Unknown template spec {}".format(template_spec),
                                                frame_index = -1,
                                                snippet_lineno = self._snippet_lineno,
                                                snippet_adjust = 0)
            self.log('------- render start {}'.format(template_name))
            render_output = template.render(data=data)
            self.log(gen)
            self.log('------- render   end {}'.format(template_name))

        except (TemplateSyntaxError, TemplateAssertionError) as exc:
            raise self._get_error_exception("{}".format(exc.message),
                                            frame_index = -1,
                                            snippet_lineno = exc.lineno,
                                            snippet_adjust = 0)
        finally:
            # Restore proxy connection to generator
            env.globals.update(cogen = cogeno_proxy_save)

            # We need to make sure that the last line in the output
            # ends with a newline, or it will be joined to the
            # end-output line, ruining cogen's idempotency.
            if render_output and render_output[-1] != '\n':
                render_output += '\n'
            if not self._context.script_is_jinja2():
                # render was called from non jinja2 script context
                # we have to fill _outstring
                self._context.out(render_output)
            # Jinja2 context requires to return the render result
            # Do it always
            return render_output

    def _jinja2_environment(self):
        if self._context._jinja2_environment is None:
            # Prepare Jinja2 execution environment

            # Jinja2 templates paths for Jinja2 file loader
            templates = []
            for path in self.templates_paths():
                templates.append(str(path))

            loader = ChoiceLoader([
                FileSystemLoader(templates),
                self.Jinja2SnippetLoader()]
            )
            env = Environment(loader=loader,
                              extensions=['jinja2.ext.do',
                                          'jinja2.ext.loopcontrols'])
            self._context._jinja2_environment = env

        # Add path to the directory of the jinja2 template
        path = Path(self._context._template_file)
        if path.is_file():
            env.loader.loaders[0].searchpath.append(str(path.resolve().parent))

        return self._context._jinja2_environment

    def _jinja2_data(self):
        if not 'jinja2_data' in self._context._globals:
            # Prepare Jinja2 data
            data = {}
            data['devicetree'] = self.edts()._edts
            data['config'] = self.config_properties()
            data['runtime'] = {}
            data['runtime']['include_path'] = []
            for path in self.templates_paths():
                data['runtime']['include_path'].append(str(path))
            data['runtime']['input_file'] = self._context._template_file
            data['runtime']['output_name'] = self._context._output_file
            data['runtime']['log_file'] = self._context._log_file
            data['runtime']['defines'] = self._context._options.defines
            self._context._globals['jinja2_data'] = data
        else:
            data = self._context._globals['jinja2_data']
        return data

    ##
    # @brief evaluate jinja2 template
    #
    # write evaluation result to self._outstring
    #
    def _jinja2_evaluate(self):
        if not self._context.script_is_jinja2():
            # This should never happen
            self.error("Unexpected context '{}' for inline evaluation."
                       .format(self._context.script_type()),
                       frame_index = -2, snippet_lineno = 0)

        self.log('s{}: process jinja2 template {}'
                 .format(len(self.cogeno_module_states),
                         self._context._template_file))

        if not self._context.template_is_file():
            # Make the string/ snippet template known to the
            # Snippet loader
            self.Jinja2SnippetLoader.snippet_templates[self._context._template_file] = self._context._template

        render_output = self.render(self._context._template_file)
        self.out(render_output)
