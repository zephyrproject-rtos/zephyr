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
    '''Zephyr directive for generating documentation with the shell commands needed
    to manage (build, flash, etc.) an application.

    For example, to generate commands to build samples/hello_world for
    qemu_x86:

    .. zephyr-app-commands::
       :zephyr-app: samples/hello_world
       :board: qemu_x86
       :goals: build

    Directive options:

    - :app: if set, the commands will change directories to this path to the
      application.

    - :zephyr-app: like :app:, but includes instructions from the Zephyr base
      directory. Cannot be given with :app:

    - :board: if set, the application build will target the given board

    - :goals: a whitespace-separated list of what to do with the app (in
      'build', 'flash', 'debug', 'debugserver'). Commands to accomplish these
      tasks will be generated in the right order.

    '''
    has_content = False
    required_arguments = 0
    optional_arguments = 0
    final_argument_whitespace = False
    option_spec = {
        'app': directives.unchanged,
        'zephyr-app': directives.unchanged,
        'board': directives.unchanged,
        'goals': directives.unchanged_required
    }

    def run(self):
        # Parse directive options.
        app = self.options.get('app', None)
        zephyr_app = self.options.get('zephyr-app', None)
        board = self.options.get('board', None)
        goals = self.options.get('goals').split()

        if app and zephyr_app:
            raise self.error('Both app and zephyr-app options were given.')

        # Build the command content as a list, then convert to string.
        content = []
        if zephyr_app:
            content.append('$ cd $ZEPHYR_BASE/{}'.format(zephyr_app))
        elif app:
            content.append('$ cd {}'.format(app))
        content.extend([
            '$ mkdir build && cd build',
            '$ cmake -GNinja{} ..'.format(' -DBOARD={}'.format(board)
                                          if board else '')])
        if 'build' in goals:
            content.append('$ ninja')
        if 'flash' in goals:
            content.append('$ ninja flash')
        if 'debug' in goals:
            content.append('$ ninja debug')
        if 'debugserver' in goals:
            content.append('$ ninja debugserver')
        content = '\n'.join(content)

        # Create the nodes.
        literal = nodes.literal_block(content, content)
        self.add_name(literal)
        literal['language'] = 'console'
        return [literal]


def setup(app):
    app.add_directive('zephyr-app-commands', ZephyrAppCommandsDirective)

    return {
        'version': '1.0',
        'parallel_read_safe': True,
        'parallel_write_safe': True
    }
