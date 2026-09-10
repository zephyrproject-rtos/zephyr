# Copyright (c) 2018 Foundries.io
#
# SPDX-License-Identifier: Apache-2.0

'''Helpers shared by multiple west extension command modules.

Note that common helpers used by the flash and debug extension
commands are in run_common -- that's for common code used by
commands which specifically execute runners.'''

import os
import shlex
from pathlib import Path

from west.commands import WestCommand

# This relies on this file being zephyr/scripts/foo/bar.py.
# If you move this file, you'll break it, so be careful.
THIS_ZEPHYR = Path(__file__).parent.parent.parent
ZEPHYR_BASE = Path(os.environ.get('ZEPHYR_BASE', THIS_ZEPHYR))

# FIXME we need a nicer way to handle imports from scripts and cmake than this.
ZEPHYR_SCRIPTS = ZEPHYR_BASE / 'scripts'
ZEPHYR_CMAKE = ZEPHYR_BASE / 'cmake'

class Forceable(WestCommand):
    '''WestCommand subclass for commands with a --force option.'''

    @staticmethod
    def add_force_arg(parser):
        '''Add a -f / --force option to the parser.'''
        parser.add_argument('-f', '--force', action='store_true',
                            help='ignore any errors and try to proceed')

    def check_force(self, cond, msg):
        '''Abort if the command needs to be forced and hasn't been.

        The "forced" predicate must be in self.args.forced.

        If cond and self.args.force are both False, scream and die
        with message msg. Otherwise, return. That is, "cond" is a
        condition which means everything is OK; if it's False, only
        self.args.force being True can allow execution to proceed.
        '''
        if not (cond or self.args.force):
            self.err(msg)
            self.die('refusing to proceed without --force due to above error')

    def config_get_words(self, section_key, fallback=None):
        unparsed = self.config.get(section_key)
        self.dbg(f'west config {section_key}={unparsed}')
        return fallback if unparsed is None else shlex.split(unparsed)

    def config_get(self, section_key, fallback=None):
        words = self.config_get_words(section_key)
        if words is None:
            return fallback
        if len(words) != 1:
            self.die(f'Single word expected for: {section_key}={words}. Use quotes?')
        return words[0]
