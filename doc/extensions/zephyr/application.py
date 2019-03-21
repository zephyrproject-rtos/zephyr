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
      The default is 'cmake'.

    \:app:
      if set, the commands will change directories to this path to the
      application.

    \:zephyr-app:
      like \:app:, but includes instructions from the Zephyr base
      directory. Cannot be given with \:app:.

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

    \:conf:
      if set, the application build will use the given configuration
      file.  If multiple conf files are provided, enclose the
      space-separated list of files with quotes, e.g., "a.conf b.conf".

    \:gen-args:
      if set, additional arguments to the CMake invocation

    \:build-args:
      if set, additional arguments to the build invocation

    \:build-dir:
      if set, the application build directory will *APPEND* this
      (relative, Unix-separated) path to the standard build directory. This is
      mostly useful for distinguishing builds for one application within a
      single page.

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

    '''
    has_content = False
    required_arguments = 0
    optional_arguments = 0
    final_argument_whitespace = False
    option_spec = {
        'tool': directives.unchanged,
        'app': directives.unchanged,
        'zephyr-app': directives.unchanged,
        'generator': directives.unchanged,
        'host-os': directives.unchanged,
        'board': directives.unchanged,
        'shield': directives.unchanged,
        'conf': directives.unchanged,
        'gen-args': directives.unchanged,
        'build-args': directives.unchanged,
        'build-dir': directives.unchanged,
        'goals': directives.unchanged_required,
        'maybe-skip-config': directives.flag,
        'compact': directives.flag
    }

    TOOLS = ['cmake', 'west', 'all']
    GENERATORS = ['make', 'ninja']
    HOST_OS = ['unix', 'win', 'all']

    def run(self):
        # Re-run on the current document if this directive's source changes.
        self.state.document.settings.env.note_dependency(__file__)

        # Parse directive options.  Don't use os.path.sep or os.path.join here!
        # That would break if building the docs on Windows.
        tool = self.options.get('tool', 'cmake').lower()
        app = self.options.get('app', None)
        zephyr_app = self.options.get('zephyr-app', None)
        generator = self.options.get('generator', 'ninja').lower()
        host_os = self.options.get('host-os', 'all').lower()
        board = self.options.get('board', None)
        shield = self.options.get('shield', None)
        conf = self.options.get('conf', None)
        gen_args = self.options.get('gen-args', None)
        build_args = self.options.get('build-args', None)
        build_dir_append = self.options.get('build-dir', '').strip('/')
        goals = self.options.get('goals').split()
        skip_config = 'maybe-skip-config' in self.options
        compact = 'compact' in self.options

        if tool not in self.TOOLS:
            raise self.error('Unknown tool {}; choose from: {}'.format(
                tool, self.TOOLS))

        if app and zephyr_app:
            raise self.error('Both app and zephyr-app options were given.')

        if generator not in self.GENERATORS:
            raise self.error('Unknown generator {}; choose from: {}'.format(
                generator, self.GENERATORS))

        if host_os not in self.HOST_OS:
            raise self.error('Unknown host-os {}; choose from: {}'.format(
                host_os, self.HOST_OS))

        if compact and skip_config:
            raise self.error('Both compact and maybe-skip-config options were given.')

        # Allow build directories which are nested.
        build_dir = ('build' + '/' + build_dir_append).rstrip('/')
        num_slashes = build_dir.count('/')
        source_dir = '/'.join(['..' for i in range(num_slashes + 1)])

        # Create host_os array
        host_os = [host_os] if host_os != "all" else [v for v in self.HOST_OS
                                                        if v != 'all']
        # Create tools array
        tools = [tool] if tool != "all" else [v for v in self.TOOLS
                                                if v != 'all']
        # Build the command content as a list, then convert to string.
        content = []
        cd_to = zephyr_app or app
        tool_comment = None
        if len(tools) > 1:
            tool_comment = 'Using {}:'

        run_config = {
            'host_os': host_os,
            'cd_to': cd_to,
            'board': board,
            'shield': shield,
            'conf': conf,
            'gen_args': gen_args,
            'source_dir': source_dir,
            'build_args': build_args,
            'build_dir': build_dir,
            'zephyr_app': zephyr_app,
            'goals': goals,
            'compact': compact,
            'skip_config': skip_config,
            'generator': generator
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
                                    'CMake and {}'.format( generator)))
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
        literal['language'] = 'console'
        return literal


    def _generate_west(self, **kwargs):
        content = []
        board = kwargs['board']
        goals = kwargs['goals']
        cd_to = kwargs['cd_to']
        build_dir = kwargs['build_dir']
        kwargs['board'] = None
        cmake_args = self._cmake_args(**kwargs)
        cmake_args = ' --{}'.format(cmake_args) if cmake_args != '' else ''
        # ignore zephyr_app since west needs to run within
        # the installation. Instead rely on relative path.
        src = ' {}'.format(cd_to) if cd_to else ''
        dst = ' -d {}'.format(build_dir) if build_dir != 'build' else ''

        goal_args = ' -b {}{}{}{}'.format(board, dst, src, cmake_args)
        if 'build' in goals:
            content.append('west build{}'.format(goal_args))
            # No longer need to specify additional args, they are in the
            # CMake cache
            goal_args = '{}'.format(dst)

        if 'sign' in goals:
            content.append('west sign{}'.format(goal_args))

        for goal in goals:
            if goal == 'build' or goal == 'sign':
                continue
            elif goal == 'flash':
                content.append('west flash{}'.format(goal_args))
            elif goal == 'debug':
                content.append('west debug{}'.format(goal_args))
            elif goal == 'debugserver':
                content.append('west debugserver{}'.format(goal_args))
            elif goal == 'attach':
                content.append('west attach{}'.format(goal_args))
            else:
                content.append('west build -t {}{}'.format(goal, goal_args))

        return content

    def _mkdir(self, mkdir, build_dir, host_os, skip_config):
        content = []
        if skip_config:
            content.append("# If you already made a build directory ({}) and ran cmake, just 'cd {}' instead.".format(build_dir, build_dir))  # noqa: E501
        if host_os == "unix":
            content.append('{} {} && cd {}'.format(mkdir, build_dir, build_dir))
        elif host_os == "win":
            build_dir = build_dir.replace('/','\\')
            content.append('mkdir {} & cd {}'.format(build_dir, build_dir))
        return content

    def _cmake_args(self, **kwargs):
        board = kwargs['board']
        shield = kwargs['shield']
        conf = kwargs['conf']
        gen_args = kwargs['gen_args']
        board_arg = ' -DBOARD={}'.format(board) if board else ''
        shield_arg = ' -DSHIELD={}'.format(shield) if shield else ''
        conf_arg = ' -DCONF_FILE={}'.format(conf) if conf else ''
        gen_args = ' {}'.format(gen_args) if gen_args else ''

        return '{}{}{}{}'.format(board_arg, shield_arg, conf_arg, gen_args)

    def _generate_make(self, **kwargs):
        build_args = kwargs['build_args']
        source_dir = kwargs['source_dir']
        goals = kwargs['goals']
        compact = kwargs['compact']

        build_args = ' {}'.format(build_args) if build_args else ''
        cmake_args = self._cmake_args(**kwargs)

        content = []
        content.extend(['cmake{} {}'.format(cmake_args, source_dir)])
        if not compact:
            content.extend(['',
                            '# Now run make on the generated build system:'])
        if 'build' in goals:
            content.append('make{}'.format(build_args))
        for goal in goals:
            if goal == 'build':
                continue
            content.append('make {}'.format(goal))
        return content

    def _generate_ninja(self, **kwargs):
        build_args = kwargs['build_args']
        source_dir = kwargs['source_dir']
        goals = kwargs['goals']
        compact = kwargs['compact']

        build_args = ' {}'.format(build_args) if build_args else ''
        cmake_args = self._cmake_args(**kwargs)

        content = []
        content.extend(['cmake -GNinja{} {}'.format(cmake_args, source_dir)])
        if not compact:
            content.extend(['',
                            '# Now run ninja on the generated build system:'])
        if 'build' in goals:
            content.append('ninja{}'.format(build_args))
        for goal in goals:
            if goal == 'build':
                continue
            content.append('ninja {}'.format(goal))
        return content

    def _generate_cmake(self, **kwargs):
        host_os = kwargs['host_os']
        cd_to = kwargs['cd_to']
        zephyr_app = kwargs['zephyr_app']
        host_os = kwargs['host_os']
        build_dir = kwargs['build_dir']
        skip_config = kwargs['skip_config']
        compact = kwargs['compact']
        generator = kwargs['generator']

        num_slashes = build_dir.count('/')
        mkdir = 'mkdir' if num_slashes == 0 else 'mkdir -p'
        content = []
        os_comment = None
        if len(host_os) > 1:
            os_comment = '# On {}'

        for host in host_os:
            if host == "unix":
                if os_comment:
                    content.append(os_comment.format('Linux/macOS'))
                if cd_to:
                    prefix = '$ZEPHYR_BASE/' if zephyr_app else ''
                    content.append('cd {}{}'.format(prefix, cd_to))
            elif host == "win":
                if os_comment:
                    content.append(os_comment.format('Windows'))
                if cd_to:
                    prefix = '%ZEPHYR_BASE%\\' if zephyr_app else ''
                    backslashified = cd_to.replace('/', '\\')
                    content.append('cd {}{}'.format(prefix, backslashified))
            content.extend(self._mkdir(mkdir, build_dir, host, skip_config))
            if not compact:
                content.append('')

        if not compact:
            content.append('# Use cmake to configure a {}-based build system:'.format(generator.capitalize()))  # noqa: E501
        if generator == 'make':
            content.extend(self._generate_make(**kwargs))
        elif generator == 'ninja':
            content.extend(self._generate_ninja(**kwargs))
        return content


def setup(app):
    app.add_directive('zephyr-app-commands', ZephyrAppCommandsDirective)

    return {
        'version': '1.0',
        'parallel_read_safe': True,
        'parallel_write_safe': True
    }
