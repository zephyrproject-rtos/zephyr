# Copyright 2018 Open Source Foundries Limited.
#
# SPDX-License-Identifier: Apache-2.0

'''Zephyr RTOS meta-tool (west) main module
'''


import argparse
from functools import partial
import os
import sys
from subprocess import CalledProcessError

from . import log
from .cmd import CommandContextError
from .cmd.flash import Flash
from .cmd.debug import Debug, DebugServer
from .util import quote_sh_list


COMMANDS = (Flash(), Debug(), DebugServer())
'''Supported top-level commands.'''


class InvalidWestContext(RuntimeError):
    pass


def command_handler(command, known_args, unknown_args):
    command.run(known_args, unknown_args)


def validate_context(args, unknown):
    '''Validate the run-time context expected by west.'''
    if args.zephyr_base:
        os.environ['ZEPHYR_BASE'] = args.zephyr_base
    else:
        if 'ZEPHYR_BASE' not in os.environ:
            log.wrn('--zephyr-base missing and no ZEPHYR_BASE',
                    'in the environment')
        else:
            args.zephyr_base = os.environ['ZEPHYR_BASE']


def parse_args(argv):
    west_parser = argparse.ArgumentParser(
        prog='west', description='The Zephyr RTOS meta-tool.',
        epilog='Run "west <command> -h" for help on each command.')
    west_parser.add_argument('-z', '--zephyr-base', default=None,
                             help='''Path to the Zephyr base directory. If not
                             given, ZEPHYR_BASE must be defined in the
                             environment, and will be used instead.''')
    west_parser.add_argument('-v', '--verbose', default=0, action='count',
                             help='''Display verbose output. May be given
                             multiple times to increase verbosity.''')
    subparser_gen = west_parser.add_subparsers(title='commands',
                                               dest='command')

    for command in COMMANDS:
        parser = command.add_parser(subparser_gen)
        parser.set_defaults(handler=partial(command_handler, command))

    args, unknown = west_parser.parse_known_args(args=argv)

    # Set up logging verbosity before doing anything else, so
    # e.g. verbose messages related to argument handling errors
    # work properly.
    log.set_verbosity(args.verbose)

    try:
        validate_context(args, unknown)
    except InvalidWestContext as iwc:
        log.err(*iwc.args, fatal=True)
        west_parser.print_usage(file=sys.stderr)
        sys.exit(1)

    if 'handler' not in args:
        log.err('you must specify a command', fatal=True)
        west_parser.print_usage(file=sys.stderr)
        sys.exit(1)

    return args, unknown


def main(argv=None):
    if argv is None:
        argv = sys.argv[1:]
    args, unknown = parse_args(argv)

    for_stack_trace = 'run as "west -v ... {} ..." for a stack trace'.format(
        args.command)
    try:
        args.handler(args, unknown)
    except KeyboardInterrupt:
        sys.exit(0)
    except CalledProcessError as cpe:
        log.err('command exited with status {}: {}'.format(
            cpe.args[0], quote_sh_list(cpe.args[1])))
        if args.verbose:
            raise
        else:
            log.inf(for_stack_trace)
    except CommandContextError as cce:
        log.die('command', args.command, 'cannot be run in this context:',
                *cce.args)
    except Exception as exc:
        log.err(*exc.args, fatal=True)
        if args.verbose:
            raise
        else:
            log.inf(for_stack_trace)
