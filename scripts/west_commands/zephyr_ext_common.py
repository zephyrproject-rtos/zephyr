# Copyright (c) 2018 Foundries.io
#
# SPDX-License-Identifier: Apache-2.0

'''Helpers shared by multiple west extension command modules.

Note that common helpers used by the flash and debug extension
commands are in run_common -- that's for common code used by
commands which specifically execute runners.'''

import os
from pathlib import Path
import re

from west import log
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
            log.err(msg)
            log.die('refusing to proceed without --force due to above error')


def load_dot_config(path):
    '''Load a Kconfig .config output file in 'path' and return a dict
    from symbols set in that file to their values.

    This parser:

    - respects the usual "# CONFIG_FOO is not set" --> CONFIG_FOO=n
      semantics
    - converts decimal or hexadecimal integer literals to ints
    - strips quotes around strings
    '''

    # You might think it would be easier to create a new
    # kconfiglib.Kconfig object and use its load_config() method to
    # parse the file instead.
    #
    # Beware, traveler: that way lies madness.
    #
    # Zephyr's Kconfig files rely heavily on environment variables to
    # manage user-configurable directory locations, and recreating
    # that state turns out to be a surprisingly fragile process. It's
    # not just 'srctree'; there are various others and they change
    # randomly as features get merged.
    #
    # Nor will pickling the Kconfig object come to your aid. It's a
    # deeply recursive data structure which requires a very high
    # system recursion limit (100K frames seems to do it) and contains
    # references to un-pickle-able values in its attributes.
    #
    # It's easier and in fact more robust to rely on the fact that the
    # .config file contents are just a strictly formatted Makefile
    # fragment with some extra "is not set" semantics.

    opt_value = re.compile('^(?P<option>CONFIG_[A-Za-z0-9_]+)=(?P<value>.*)$')
    not_set = re.compile('^# (?P<option>CONFIG_[A-Za-z0-9_]+) is not set$')

    with open(path, 'r') as f:
        lines = f.readlines()

    ret = {}
    for line in lines:
        match = opt_value.match(line)
        if match:
            value = match.group('value')
            if value.startswith('"') and value.endswith('"'):
                value = value[1:-1]
            else:
                try:
                    base = 16 if value.startswith('0x') else 10
                    value = int(value, base=base)
                except ValueError:
                    pass

            ret[match.group('option')] = value

        match = not_set.match(line)
        if match:
            ret[match.group('option')] = 'n'

    return ret
