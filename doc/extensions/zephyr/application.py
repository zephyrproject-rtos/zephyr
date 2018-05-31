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
    '''
    This is a Zephyr directive for generating consistent documentation
    of the shell commands needed to manage (build, flash, etc.) an application.

    For example, to generate commands to build samples/hello_world for
    qemu_x86 use::

       .. zephyr-app-commands::
          :zephyr-app: samples/hello_world
          :board: qemu_x86
          :goals: build

    Directive options:

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
        'app': directives.unchanged,
        'zephyr-app': directives.unchanged,
        'generator': directives.unchanged,
        'host-os': directives.unchanged,
        'board': directives.unchanged,
        'conf': directives.unchanged,
        'gen-args': directives.unchanged,
        'build-args': directives.unchanged,
        'build-dir': directives.unchanged,
        'goals': directives.unchanged_required,
        'maybe-skip-config': directives.flag,
        'compact': directives.flag
    }

    GENERATORS = ['make', 'ninja']
    HOST_OS = ['unix', 'win', 'all']

    def run(self):
        # Re-run on the current document if this directive's source changes.
        self.state.document.settings.env.note_dependency(__file__)

        # Parse directive options.  Don't use os.path.sep or os.path.join here!
        # That would break if building the docs on Windows.
        app = self.options.get('app', None)
        zephyr_app = self.options.get('zephyr-app', None)
        generator = self.options.get('generator', 'ninja').lower()
        host_os = self.options.get('host-os', 'all').lower()
        board = self.options.get('board', None)
        conf = self.options.get('conf', None)
        gen_args = self.options.get('gen-args', None)
        build_args = self.options.get('build-args', None)
        build_dir_append = self.options.get('build-dir', '').strip('/')
        goals = self.options.get('goals').split()
        skip_config = 'maybe-skip-config' in self.options
        compact = 'compact' in self.options

        if app and zephyr_app:
            raise self.error('Both app and zephyr-app options were given.')

        if generator not in self.GENERATORS:
            raise self.error('Unknown generator {}; choose from: {}'.format(
                generator, self.GENERATORS))

        if host_os not in self.HOST_OS:
            raise self.error('Unknown host-os {}; choose from: {}'.format(
                generator, self.HOST_OS))

        if compact and skip_config:
            raise self.error('Both compact and maybe-skip-config options were given.')

        # Allow build directories which are nested.
        build_dir = ('build' + '/' + build_dir_append).rstrip('/')
        num_slashes = build_dir.count('/')
        source_dir = '/'.join(['..' for i in range(num_slashes + 1)])
        mkdir = 'mkdir' if num_slashes == 0 else 'mkdir -p'

        # Create host_os array
        host_os = [host_os] if host_os != "all" else self.HOST_OS

        run_config = {
            'board': board,
            'conf': conf,
            'gen_args': gen_args,
            'build_args': build_args,
            'source_dir': source_dir,
            'goals': goals,
            'compact': compact
            }

        # Build the command content as a list, then convert to string.
        content = []
        cd_to = zephyr_app or app
        comment = None
        if len(host_os) > 1:
            comment = '# On {}'

        for host in host_os:
            if cd_to:
                if host == "unix":
                    if comment:
                        content.append(comment.format('Linux/macOS'))
                    prefix = '$ZEPHYR_BASE/' if zephyr_app else ''
                    content.append('cd {}{}'.format(prefix, cd_to))
                elif host == "win":
                    if comment:
                        content.append(comment.format('Windows'))
                    prefix = '%ZEPHYR_BASE%\\' if zephyr_app else ''
                    backslashified = cd_to.replace('/', '\\')
                    content.append('cd {}{}'.format(prefix, backslashified))
            content.extend(self._mkdir(mkdir, build_dir, host,
                                       skip_config, compact))
            if not compact:
                content.append('')

        if not compact:
            content.append('# Use cmake to configure a {}-based build system:'.format(generator.capitalize()))  # noqa: E501
        if generator == 'make':
            content.extend(self._generate_make(**run_config))
        elif generator == 'ninja':
            content.extend(self._generate_ninja(**run_config))

        content = '\n'.join(content)

        # Create the nodes.
        literal = nodes.literal_block(content, content)
        self.add_name(literal)
        literal['language'] = 'console'
        return [literal]

    def _mkdir(self, mkdir, build_dir, host_os, skip_config, compact):
        content = []
        if skip_config:
            content.append("# If you already made a build directory ({}) and ran cmake, just 'cd {}' instead.".format(build_dir, build_dir))  # noqa: E501
        if host_os == "unix":
            content.append('{} {} && cd {}'.format(mkdir, build_dir, build_dir))
        elif host_os == "win":
            build_dir = build_dir.replace('/','\\')
            content.append('mkdir {} & cd {}'.format(build_dir, build_dir))
        return content

    def _generate_make(self, **kwargs):
        board = kwargs['board']
        conf = kwargs['conf']
        gen_args = kwargs['gen_args']
        build_args = kwargs['build_args']
        source_dir = kwargs['source_dir']
        goals = kwargs['goals']
        compact = kwargs['compact']

        board_arg = ' -DBOARD={}'.format(board) if board else ''
        conf_arg = ' -DCONF_FILE={}'.format(conf) if conf else ''
        gen_args = ' {}'.format(gen_args) if gen_args else ''
        build_args = ' {}'.format(build_args) if build_args else ''

        content = []
        content.extend([
            'cmake{}{}{} {}'.format(board_arg, conf_arg, gen_args,
                                       source_dir)])
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
        board = kwargs['board']
        conf = kwargs['conf']
        gen_args = kwargs['gen_args']
        build_args = kwargs['build_args']
        source_dir = kwargs['source_dir']
        goals = kwargs['goals']
        compact = kwargs['compact']

        board_arg = ' -DBOARD={}'.format(board) if board else ''
        conf_arg = ' -DCONF_FILE={}'.format(conf) if conf else ''
        gen_args = ' {}'.format(gen_args) if gen_args else ''
        build_args = ' {}'.format(build_args) if build_args else ''

        content = []
        content.extend([
            'cmake -GNinja{}{}{} {}'.format(board_arg, conf_arg, gen_args,
                                            source_dir)])
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


def setup(app):
    app.add_directive('zephyr-app-commands', ZephyrAppCommandsDirective)

    return {
        'version': '1.0',
        'parallel_read_safe': True,
        'parallel_write_safe': True
    }
