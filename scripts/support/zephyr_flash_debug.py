#! /usr/bin/env python3

# Copyright (c) 2017 Linaro Limited.
#
# SPDX-License-Identifier: Apache-2.0

"""Zephyr flash/debug script

This script is a transparent replacement for legacy Zephyr flash and
debug scripts which have now been removed. It will be refactored over
time as the rest of the build system is taught to use it.
"""

from os import path
import sys

from runner.core import ZephyrBinaryRunner, get_env_bool_or


# TODO: Stop using environment variables.
#
# Migrate the build system so we can use an argparse.ArgumentParser and
# per-flasher subparsers, so invoking the script becomes something like:
#
#   python zephyr_flash_debug.py openocd --openocd-bin=/openocd/path ...
#
# For now, maintain compatibility.
def run(shell_script_full, command, debug):
    shell_script = path.basename(shell_script_full)
    try:
        runner = ZephyrBinaryRunner.create_for_shell_script(shell_script,
                                                            command,
                                                            debug)
    except ValueError:
        print('Unrecognized shell script {}'.format(shell_script_full),
              file=sys.stderr)
        raise

    runner.run(command)


if __name__ == '__main__':
    commands = {'flash', 'debug', 'debugserver'}
    debug = True
    try:
        debug = get_env_bool_or('KBUILD_VERBOSE', False)
        if len(sys.argv) != 3 or sys.argv[1] not in commands:
            raise ValueError('usage: {} <{}> path-to-script'.format(
                sys.argv[0], '|'.join(commands)))
        run(sys.argv[2], sys.argv[1], debug)
    except Exception as e:
        if debug:
            raise
        else:
            print('Error: {}'.format(e), file=sys.stderr)
            print('Re-run with KBUILD_VERBOSE=1 for a stack trace.',
                  file=sys.stderr)
            sys.exit(1)
