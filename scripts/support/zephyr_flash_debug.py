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

import argparse
import functools
import sys

from runner.core import ZephyrBinaryRunner


def print_runners_handler(args):
    for cls in ZephyrBinaryRunner.get_runners():
        print(cls.name())


def runner_handler(cls, args):
    runner = cls.create_from_args(args)
    # This relies on ZephyrBinaryRunner.add_parser() having command as
    # its single positional argument; see its docstring for details.
    runner.run(args.command)


def main():
    # Argument handling is split into a two-level structure, with
    # common options to the script first, then a sub-command (i.e.  a
    # runner name), then options and arguments for that sub-command
    # (like 'flash --some-option=value').
    #
    # For top-level help (including a list of runners), run:
    #
    # $ZEPHYR_BASE/.../SCRIPT.py -h
    #
    # For help on a particular RUNNER (like 'pyocd'), run:
    #
    # $ZEPHYR_BASE/.../SCRIPT.py RUNNER -h
    #
    # For verbose output, use:
    #
    # $ZEPHYR_BASE/.../SCRIPT.py [-v|--verbose] RUNNER [--runner-options]
    #
    # Note that --verbose comes *before* RUNNER, not after!
    #
    # Other commands (for now just the "runners" command, which prints
    # the available runners) are handled the same way:
    #
    # $ZEPHYR_BASE/.../SCRIPT.py runners
    top_parser = argparse.ArgumentParser()
    top_parser.add_argument('-v', '--verbose',
                            default=False, action='store_true',
                            help='If set, enable verbose output.')
    sub_parsers = top_parser.add_subparsers(dest='top_command')

    # Handlers for each subcommand.
    handlers = {}

    # The 'runners' command just prints the runners. It takes no arguments.
    sub_parsers.add_parser('runners')
    handlers['runners'] = print_runners_handler

    # Add a sub-command for each runner. (It's a bit hackish for runners
    # to know about argparse, but it's good enough for now.)
    for cls in ZephyrBinaryRunner.get_runners():
        if cls.name() in handlers:
            print('Runner {} name is already a top-level command'.format(
                      cls.name()),
                  file=sys.sterr)
            sys.exit(1)
        sub_parser = sub_parsers.add_parser(cls.name())
        cls.add_parser(sub_parser)
        handlers[cls.name()] = functools.partial(runner_handler, cls)

    args = top_parser.parse_args()
    if args.top_command is None:
        choices = ', '.join(handlers.keys())
        print('Missing command or runner; choices: {}'.format(choices),
              file=sys.stderr)
        sys.exit(1)
    try:
        handlers[args.top_command](args)
    except Exception as e:
        print('Error: {}'.format(e), file=sys.stderr)
        raise


if __name__ == '__main__':
    main()
