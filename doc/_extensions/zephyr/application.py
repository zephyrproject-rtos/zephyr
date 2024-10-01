# Copyright (c) 2017 Open Source Foundries Limited.
#
# SPDX-License-Identifier: Apache-2.0

'''Sphinx extensions related to managing Zephyr applications.'''

from docutils import nodes
from docutils.parsers.rst import Directive
from docutils.parsers.rst import directives


# TODO: extend and modify this for Windows.
#
# This could be as simple as generating a couple of sets of instructions, one
# for Unix environments, and another for Windows.
class ZephyrAppCommandsDirective(Directive):
    r'''
    This is a Zephyr directive for generating consistent documentation
    of the shell commands needed to manage (build, flash, etc.) an application.

    For example, to generate commands to build samples/hello_world for
    qemu_x86 use::

       .. zephyr-app-commands::
          :zephyr-app: samples/hello_world
          :board: qemu_x86
          :goals: build

    Directive options:

    \:tool:
      which tool to use. Valid options are currently 'cmake', 'west' and 'all'.
      The default is 'west'.

    \:app:
      path to the application to build.

    \:zephyr-app:
      path to the application to build, this is an app present in the upstream
      zephyr repository. Mutually exclusive with \:app:.

    \:cd-into:
      if set, build instructions are given from within the \:app: folder,
      instead of outside of it.

    \:generator:
      which build system to generate. Valid options are
      currently 'ninja' and 'make'. The default is 'ninja'. This option
      is not case sensitive.

    \:host-os:
      which host OS the instructions are for. Valid options are
      'unix', 'win' and 'all'. The default is 'all'.

    \:board:
      if set, the application build will target the given board.

    \:shield:
      if set, the application build will target the given shield.
      Multiple shields can be provided in a comma separated list.

    \:conf:
      if set, the application build will use the given configuration
      file.  If multiple conf files are provided, enclose the
      space-separated list of files with quotes, e.g., "a.conf b.conf".

    \:gen-args:
      if set, additional arguments to the CMake invocation

    \:build-args:
      if set, additional arguments to the build invocation

    \:snippets:
      if set, indicates the application should be compiled with the listed snippets.
      Multiple snippets can be provided in a comma separated list.

    \:build-dir:
      if set, the application build directory will *APPEND* this
      (relative, Unix-separated) path to the standard build directory. This is
      mostly useful for distinguishing builds for one application within a
      single page.

    \:build-dir-fmt:
      if set, assume that "west config build.dir-fmt" has been set to this
      path. Exclusive with 'build-dir' and depends on 'tool=west'.

    \:goals:
      a whitespace-separated list of what to do with the app (in
      'build', 'flash', 'debug', 'debugserver', 'run'). Commands to accomplish
      these tasks will be generated in the right order.

    \:maybe-skip-config:
      if set, this indicates the reader may have already
      created a build directory and changed there, and will tweak the text to
      note that doing so again is not necessary.

    \:compact:
      if set, the generated output is a single code block with no
      additional comment lines

    \:west-args:
      if set, additional arguments to the west invocation (ignored for CMake)

    \:flash-args:
      if set, additional arguments to the flash invocation

    '''
    has_content = False
    required_arguments = 0
    optional_arguments = 0
    final_argument_whitespace = False
    option_spec = {
        'tool': directives.unchanged,
        'app': directives.unchanged,
        'zephyr-app': directives.unchanged,
        'cd-into': directives.flag,
        'generator': directives.unchanged,
        'host-os': directives.unchanged,
        'board': directives.unchanged,
        'shield': directives.unchanged,
        'conf': directives.unchanged,
        'gen-args': directives.unchanged,
        'build-args': directives.unchanged,
        'snippets': directives.unchanged,
        'build-dir': directives.unchanged,
        'build-dir-fmt': directives.unchanged,
        'goals': directives.unchanged_required,
        'maybe-skip-config': directives.flag,
        'compact': directives.flag,
        'west-args': directives.unchanged,
        'flash-args': directives.unchanged,
    }

    TOOLS = ['cmake', 'west', 'all']
    GENERATORS = ['make', 'ninja']
    HOST_OS = ['unix', 'win', 'all']
    IN_TREE_STR = '# From the root of the zephyr repository'

    def run(self):
        # Re-run on the current document if this directive's source changes.
        self.state.document.settings.env.note_dependency(__file__)

        # Parse directive options.  Don't use os.path.sep or os.path.join here!
        # That would break if building the docs on Windows.
        tool = self.options.get('tool', 'west').lower()
        app = self.options.get('app', None)
        zephyr_app = self.options.get('zephyr-app', None)
        cd_into = 'cd-into' in self.options
        generator = self.options.get('generator', 'ninja').lower()
        host_os = self.options.get('host-os', 'all').lower()
        board = self.options.get('board', None)
        shield = self.options.get('shield', None)
        conf = self.options.get('conf', None)
        gen_args = self.options.get('gen-args', None)
        build_args = self.options.get('build-args', None)
        snippets = self.options.get('snippets', None)
        build_dir_append = self.options.get('build-dir', '').strip('/')
        build_dir_fmt = self.options.get('build-dir-fmt', None)
        goals = self.options.get('goals').split()
        skip_config = 'maybe-skip-config' in self.options
        compact = 'compact' in self.options
        west_args = self.options.get('west-args', None)
        flash_args = self.options.get('flash-args', None)

        if tool not in self.TOOLS:
            raise self.error('Unknown tool {}; choose from: {}'.format(
                tool, self.TOOLS))

        if app and zephyr_app:
            raise self.error('Both app and zephyr-app options were given.')

        if build_dir_append != '' and build_dir_fmt:
            raise self.error('Both build-dir and build-dir-fmt options were given.')

        if build_dir_fmt and tool != 'west':
            raise self.error('build-dir-fmt is only supported for the west build tool.')

        if generator not in self.GENERATORS:
            raise self.error('Unknown generator {}; choose from: {}'.format(
                generator, self.GENERATORS))

        if host_os not in self.HOST_OS:
            raise self.error('Unknown host-os {}; choose from: {}'.format(
                host_os, self.HOST_OS))

        if compact and skip_config:
            raise self.error('Both compact and maybe-skip-config options were given.')

        app = app or zephyr_app
        in_tree = self.IN_TREE_STR if zephyr_app else None
        # Allow build directories which are nested.
        build_dir = ('build' + '/' + build_dir_append).rstrip('/')

        # Create host_os array
        host_os = [host_os] if host_os != "all" else [v for v in self.HOST_OS
                                                        if v != 'all']
        # Create tools array
        tools = [tool] if tool != "all" else [v for v in self.TOOLS
                                                if v != 'all']

        # Create snippet array
        snippet_list = snippets.split(',') if snippets is not None else None

        # Create shields array
        shield_list = shield.split(',') if shield is not None else None

        # Build the command content as a list, then convert to string.
        content = []
        tool_comment = None
        if len(tools) > 1:
            tool_comment = 'Using {}:'

        run_config = {
            'host_os': host_os,
            'app': app,
            'in_tree': in_tree,
            'cd_into': cd_into,
            'board': board,
            'shield': shield_list,
            'conf': conf,
            'gen_args': gen_args,
            'build_args': build_args,
            'snippets': snippet_list,
            'build_dir': build_dir,
            'build_dir_fmt': build_dir_fmt,
            'goals': goals,
            'compact': compact,
            'skip_config': skip_config,
            'generator': generator,
            'west_args': west_args,
            'flash_args': flash_args,
            }

        if 'west' in tools:
            w = self._generate_west(**run_config)
            if tool_comment:
                paragraph = nodes.paragraph()
                paragraph += nodes.Text(tool_comment.format('west'))
                content.append(paragraph)
                content.append(self._lit_block(w))
            else:
                content.extend(w)

        if 'cmake' in tools:
            c = self._generate_cmake(**run_config)
            if tool_comment:
                paragraph = nodes.paragraph()
                paragraph += nodes.Text(tool_comment.format(
                    'CMake and {}'.format(generator)))
                content.append(paragraph)
                content.append(self._lit_block(c))
            else:
                content.extend(c)

        if not tool_comment:
            content = [self._lit_block(content)]

        return content

    def _lit_block(self, content):
        content = '\n'.join(content)

        # Create the nodes.
        literal = nodes.literal_block(content, content)
        self.add_name(literal)
        literal['language'] = 'shell'
        return literal


    def _generate_west(self, **kwargs):
        content = []
        generator = kwargs['generator']
        board = kwargs['board']
        app = kwargs['app']
        in_tree = kwargs['in_tree']
        goals = kwargs['goals']
        cd_into = kwargs['cd_into']
        build_dir = kwargs['build_dir']
        build_dir_fmt = kwargs['build_dir_fmt']
        compact = kwargs['compact']
        shield = kwargs['shield']
        snippets = kwargs['snippets']
        west_args = kwargs['west_args']
        flash_args = kwargs['flash_args']
        kwargs['board'] = None
        # west always defaults to ninja
        gen_arg = ' -G\'Unix Makefiles\'' if generator == 'make' else ''
        cmake_args = gen_arg + self._cmake_args(**kwargs)
        cmake_args = ' --{}'.format(cmake_args) if cmake_args != '' else ''
        west_args = ' {}'.format(west_args) if west_args else ''
        flash_args = ' {}'.format(flash_args) if flash_args else ''
        snippet_args = ''.join(f' -S {s}' for s in snippets) if snippets else ''
        shield_args = ''.join(f' --shield {s}' for s in shield) if shield else ''
        # ignore zephyr_app since west needs to run within
        # the installation. Instead rely on relative path.
        src = ' {}'.format(app) if app and not cd_into else ''

        if build_dir_fmt is None:
            dst = ' -d {}'.format(build_dir) if build_dir != 'build' else ''
            build_dst = dst
        else:
            app_name = app.split('/')[-1]
            build_dir_formatted = build_dir_fmt.format(app=app_name, board=board, source_dir=app)
            dst = ' -d {}'.format(build_dir_formatted)
            build_dst = ''

        if in_tree and not compact:
            content.append(in_tree)

        if cd_into and app:
            content.append('cd {}'.format(app))

        # We always have to run west build.
        #
        # FIXME: doing this unconditionally essentially ignores the
        # maybe-skip-config option if set.
        #
        # This whole script and its users from within the
        # documentation needs to be overhauled now that we're
        # defaulting to west.
        #
        # For now, this keeps the resulting commands working.
        content.append('west build -b {}{}{}{}{}{}{}'.
                       format(board, west_args, snippet_args, shield_args, build_dst, src, cmake_args))

        # If we're signing, we want to do that next, so that flashing
        # etc. commands can use the signed file which must be created
        # in this step.
        if 'sign' in goals:
            content.append('west sign{}'.format(dst))

        for goal in goals:
            if goal in {'build', 'sign'}:
                continue
            elif goal == 'flash':
                content.append('west flash{}{}'.format(flash_args, dst))
            elif goal == 'debug':
                content.append('west debug{}'.format(dst))
            elif goal == 'debugserver':
                content.append('west debugserver{}'.format(dst))
            elif goal == 'attach':
                content.append('west attach{}'.format(dst))
            else:
                content.append('west build -t {}{}'.format(goal, dst))

        return content

    @staticmethod
    def _mkdir(mkdir, build_dir, host_os, skip_config):
        content = []
        if skip_config:
            content.append("# If you already made a build directory ({}) and ran cmake, just 'cd {}' instead.".format(build_dir, build_dir))  # noqa: E501
        if host_os == 'all':
            content.append('mkdir {} && cd {}'.format(build_dir, build_dir))
        if host_os == "unix":
            content.append('{} {} && cd {}'.format(mkdir, build_dir, build_dir))
        elif host_os == "win":
            build_dir = build_dir.replace('/', '\\')
            content.append('mkdir {} & cd {}'.format(build_dir, build_dir))
        return content

    @staticmethod
    def _cmake_args(**kwargs):
        board = kwargs['board']
        conf = kwargs['conf']
        gen_args = kwargs['gen_args']
        board_arg = ' -DBOARD={}'.format(board) if board else ''
        conf_arg = ' -DCONF_FILE={}'.format(conf) if conf else ''
        gen_args = ' {}'.format(gen_args) if gen_args else ''

        return '{}{}{}'.format(board_arg, conf_arg, gen_args)

    def _cd_into(self, mkdir, **kwargs):
        app = kwargs['app']
        host_os = kwargs['host_os']
        compact = kwargs['compact']
        build_dir = kwargs['build_dir']
        skip_config = kwargs['skip_config']
        content = []
        os_comment = None
        if len(host_os) > 1:
            os_comment = '# On {}'
            num_slashes = build_dir.count('/')
            if not app and mkdir and num_slashes == 0:
                # When there's no app and a single level deep build dir,
                # simplify output
                content.extend(self._mkdir(mkdir, build_dir, 'all',
                               skip_config))
                if not compact:
                    content.append('')
                return content
        for host in host_os:
            if host == "unix":
                if os_comment:
                    content.append(os_comment.format('Linux/macOS'))
                if app:
                    content.append('cd {}'.format(app))
            elif host == "win":
                if os_comment:
                    content.append(os_comment.format('Windows'))
                if app:
                    backslashified = app.replace('/', '\\')
                    content.append('cd {}'.format(backslashified))
            if mkdir:
                content.extend(self._mkdir(mkdir, build_dir, host, skip_config))
            if not compact:
                content.append('')
        return content

    def _generate_cmake(self, **kwargs):
        generator = kwargs['generator']
        cd_into = kwargs['cd_into']
        app = kwargs['app']
        in_tree = kwargs['in_tree']
        build_dir = kwargs['build_dir']
        build_args = kwargs['build_args']
        snippets = kwargs['snippets']
        shield = kwargs['shield']
        skip_config = kwargs['skip_config']
        goals = kwargs['goals']
        compact = kwargs['compact']

        content = []

        if in_tree and not compact:
            content.append(in_tree)

        if cd_into:
            num_slashes = build_dir.count('/')
            mkdir = 'mkdir' if num_slashes == 0 else 'mkdir -p'
            content.extend(self._cd_into(mkdir, **kwargs))
            # Prepare cmake/ninja/make variables
            source_dir = ' ' + '/'.join(['..' for i in range(num_slashes + 1)])
            cmake_build_dir = ''
            tool_build_dir = ''
        else:
            source_dir = ' {}'.format(app) if app else ' .'
            cmake_build_dir = ' -B{}'.format(build_dir)
            tool_build_dir = ' -C{}'.format(build_dir)

        # Now generate the actual cmake and make/ninja commands
        gen_arg = ' -GNinja' if generator == 'ninja' else ''
        build_args = ' {}'.format(build_args) if build_args else ''
        snippet_args = ' -DSNIPPET="{}"'.format(';'.join(snippets)) if snippets else ''
        shield_args = ' -DSHIELD="{}"'.format(';'.join(shield)) if shield else ''
        cmake_args = self._cmake_args(**kwargs)

        if not compact:
            if not cd_into and skip_config:
                content.append("# If you already ran cmake with -B{}, you " \
                               "can skip this step and run {} directly.".
                               format(build_dir, generator))  # noqa: E501
            else:
                content.append('# Use cmake to configure a {}-based build' \
                               'system:'.format(generator.capitalize()))  # noqa: E501

        content.append('cmake{}{}{}{}{}{}'.format(cmake_build_dir, gen_arg,
                                              cmake_args, snippet_args, shield_args, source_dir))
        if not compact:
            content.extend(['',
                            '# Now run the build tool on the generated build system:'])

        if 'build' in goals:
            content.append('{}{}{}'.format(generator, tool_build_dir,
                                           build_args))
        for goal in goals:
            if goal == 'build':
                continue
            content.append('{}{} {}'.format(generator, tool_build_dir, goal))

        return content


def setup(app):
    app.add_directive('zephyr-app-commands', ZephyrAppCommandsDirective)

    return {
        'version': '1.0',
        'parallel_read_safe': True,
        'parallel_write_safe': True
    }
