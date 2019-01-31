# Copyright (c) 2018 Foundries.io
#
# SPDX-License-Identifier: Apache-2.0

'''Helpers shared by multiple west extension command modules.

Note that common helpers used by the flash and debug extension
commands are in run_common -- that's for common code used by
commands which specifically execute runners.'''

import os

from west import log
from west.build import DEFAULT_BUILD_DIR, is_zephyr_build
from west.commands import WestCommand

from runners.core import RunnerConfig

BUILD_DIR_DESCRIPTION = '''\
Explicitly sets the build directory.  If not given and the current
directory is a Zephyr build directory, it will be used; otherwise,
"{}" is assumed.'''.format(DEFAULT_BUILD_DIR)


def find_build_dir(dir):
    '''Heuristic for finding a build directory.

    If the given argument is truthy, it is returned. Otherwise, if
    the current working directory is a build directory, it is
    returned. Otherwise, west.build.DEFAULT_BUILD_DIR is returned.'''
    if dir:
        build_dir = dir
    else:
        cwd = os.getcwd()
        if is_zephyr_build(cwd):
            build_dir = cwd
        else:
            build_dir = DEFAULT_BUILD_DIR
    return os.path.abspath(build_dir)


class Forceable(WestCommand):
    '''WestCommand subclass for commands with a --force option.'''

    def add_force_arg(self, parser):
        '''Add a -f / --force option to the parser.'''
        parser.add_argument('-f', '--force', action='store_true',
                            help='Ignore any errors and try to proceed')

    def check_force(self, cond, msg):
        '''Abort if the command needs to be forced and hasn't been.

        The "forced" predicate must be in self.args.forced.

        If cond and self.args.force are both False, scream and die
        with message msg. Otherwise, return. That is, "cond" is a
        condition which means everything is OK; if it's False, only
        self.args.force being True can allow execution to proceed.
        '''
        if not (cond or self.args.force):
            log.err(msg)
            log.die('refusing to proceed without --force due to above error')


def cached_runner_config(build_dir, cache):
    '''Parse the RunnerConfig from a build directory and CMake Cache.'''
    board_dir = cache['ZEPHYR_RUNNER_CONFIG_BOARD_DIR']
    elf_file = cache.get('ZEPHYR_RUNNER_CONFIG_ELF_FILE',
                         cache['ZEPHYR_RUNNER_CONFIG_KERNEL_ELF'])
    hex_file = cache.get('ZEPHYR_RUNNER_CONFIG_HEX_FILE',
                         cache['ZEPHYR_RUNNER_CONFIG_KERNEL_HEX'])
    bin_file = cache.get('ZEPHYR_RUNNER_CONFIG_BIN_FILE',
                         cache['ZEPHYR_RUNNER_CONFIG_KERNEL_BIN'])
    gdb = cache.get('ZEPHYR_RUNNER_CONFIG_GDB')
    openocd = cache.get('ZEPHYR_RUNNER_CONFIG_OPENOCD')
    openocd_search = cache.get('ZEPHYR_RUNNER_CONFIG_OPENOCD_SEARCH')

    return RunnerConfig(build_dir, board_dir,
                        elf_file, hex_file, bin_file,
                        gdb=gdb, openocd=openocd,
                        openocd_search=openocd_search)
