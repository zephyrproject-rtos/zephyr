#! /usr/bin/env python3

# Copyright (c) 2017 Linaro Limited.
#
# SPDX-License-Identifier: Apache-2.0

"""Zephyr board flashing script

This script is a transparent replacement for an existing Zephyr flash
script. If it can invoke the flashing tools natively, it will do so; otherwise,
it delegates to the shell script passed as second argument."""

import abc
from os import path
import os
import sys
import subprocess


def get_env_bool_or(env_var, default_value):
    try:
        return bool(int(os.environ[env_var]))
    except KeyError:
        return default_value


def check_call(cmd, debug):
    if debug:
        print(' '.join(cmd))
    subprocess.check_call(cmd)


class ZephyrBinaryFlasher(abc.ABC):
    '''Abstract superclass for flasher objects.'''

    def __init__(self, debug=False):
        self.debug = debug

    @staticmethod
    def create_for_shell_script(shell_script, debug):
        '''Factory for using as a drop-in replacement to a shell script.

        Get flasher instance to use in place of shell_script, deriving
        flasher configuration from the environment.'''
        for sub_cls in ZephyrBinaryFlasher.__subclasses__():
            if sub_cls.replaces_shell_script(shell_script):
                return sub_cls.create_from_env(debug)
        raise ValueError('no flasher replaces script {}'.format(shell_script))

    @staticmethod
    @abc.abstractmethod
    def replaces_shell_script(shell_script):
        '''Check if this flasher class replaces FLASH_SCRIPT=shell_script.'''

    @staticmethod
    @abc.abstractmethod
    def create_from_env(debug):
        '''Create new flasher instance from environment variables.

        This class must be able to replace the current FLASH_SCRIPT. The
        environment variables expected by that script are used to build
        the flasher in a backwards-compatible manner.'''

    @abc.abstractmethod
    def flash(self, **kwargs):
        '''Flash the board.'''


# TODO: Stop using environment variables.
#
# Migrate the build system so we can use an argparse.ArgumentParser and
# per-flasher subparsers, so invoking the script becomes something like:
#
#   python zephyr_flash_debug.py openocd --openocd-bin=/openocd/path ...
#
# For now, maintain compatibility.
def flash(shell_script_full, debug):
    shell_script = path.basename(shell_script_full)
    try:
        flasher = ZephyrBinaryFlasher.create_for_shell_script(shell_script,
                                                              debug)
    except ValueError:
        # Can't create a flasher; fall back on shell script.
        check_call([shell_script_full, 'flash'], debug)
        return

    flasher.flash()


if __name__ == '__main__':
    debug = True
    try:
        debug = get_env_bool_or('KBUILD_VERBOSE', False)
        if len(sys.argv) != 3 or sys.argv[1] != 'flash':
            raise ValueError('usage: {} flash path-to-script'.format(
                sys.argv[0]))
        flash(sys.argv[2], debug)
    except Exception as e:
        if debug:
            raise
        else:
            print('Error: {}'.format(e), file=sys.stderr)
            print('Re-run with KBUILD_VERBOSE=1 for a stack trace.',
                  file=sys.stderr)
            sys.exit(1)
