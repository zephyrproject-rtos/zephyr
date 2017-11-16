#! /usr/bin/env python3

# Copyright (c) 2017 Linaro Limited.
# Copyright (c) 2017 Open Source Foundries Limited.
#
# SPDX-License-Identifier: Apache-2.0

"""Zephyr flash/debug script

This helper script is the build system's entry point to Zephyr's
"runner" Python package. This package provides ZephyrBinaryRunner,
which is a standard interface for flashing and debugging boards
supported by Zephyr, as well as backend-specific scripts for tools
such as OpenOCD, pyOCD, etc.
"""

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
def run(runner_name, command, debug):
    try:
        runner = ZephyrBinaryRunner.create_runner(runner_name, command, debug)
    except ValueError:
        print('runner {} is not available or does not support {}'.format(
                  runner_name, command),
              file=sys.stderr)
        raise

    runner.run(command)


if __name__ == '__main__':
    commands = {'flash', 'debug', 'debugserver'}
    debug = True
    try:
        debug = get_env_bool_or('VERBOSE', False)
        if len(sys.argv) != 3 or sys.argv[2] not in commands:
            raise ValueError('usage: {} <runner-name> <{}>'.format(
                sys.argv[0], '|'.join(commands)))
        run(sys.argv[1], sys.argv[2], debug)
    except Exception as e:
        if debug:
            raise
        else:
            print('Error: {}'.format(e), file=sys.stderr)
            print('Re-run with VERBOSE=1 for a stack trace.',
                  file=sys.stderr)
            sys.exit(1)
