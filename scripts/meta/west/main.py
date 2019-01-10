#!/usr/bin/env python3

# Copyright 2018 Open Source Foundries Limited.
#
# SPDX-License-Identifier: Apache-2.0

'''Zephyr RTOS meta-tool (west) main module
'''


import argparse
import colorama
from functools import partial
import os
import sys
from subprocess import CalledProcessError, check_output, DEVNULL

from west import log
from west import config
from west.commands import CommandContextError
from west.commands.build import Build
from west.commands.flash import Flash
from west.commands.debug import Debug, DebugServer, Attach
from west.commands.project import List, Clone, Fetch, Pull, Rebase, Branch, \
                                  Checkout, Diff, Status, Update, ForAll, \
                                  WestUpdated
from west.manifest import Manifest
from west.util import quote_sh_list, in_multirepo_install, west_dir

IN_MULTIREPO_INSTALL = in_multirepo_install(os.path.dirname(__file__))

BUILD_FLASH_COMMANDS = [
    Build(),
    Flash(),
    Debug(),
    DebugServer(),
    Attach(),
]

PROJECT_COMMANDS = [
    List(),
    Clone(),
    Fetch(),
    Pull(),
    Rebase(),
    Branch(),
    Checkout(),
    Diff(),
    Status(),
    Update(),
    ForAll(),
]

# Built-in commands in this West. For compatibility with monorepo
# installations of West within the Zephyr tree, we only expose the
# project commands if this is a multirepo installation.
COMMANDS = BUILD_FLASH_COMMANDS

if IN_MULTIREPO_INSTALL:
    COMMANDS += PROJECT_COMMANDS


class InvalidWestContext(RuntimeError):
    pass


def command_handler(command, known_args, unknown_args):
    command.run(known_args, unknown_args)


def set_zephyr_base(args):
    '''Ensure ZEPHYR_BASE is set, emitting warnings if that's not
    possible, or if the user is pointing it somewhere different than
    what the manifest expects.'''
    zb_env = os.environ.get('ZEPHYR_BASE')

    if args.zephyr_base:
        # The command line --zephyr-base takes precedence over
        # everything else.
        zb = os.path.abspath(args.zephyr_base)
        zb_origin = 'command line'
    else:
        # If the user doesn't specify it concretely, use the project
        # with path 'zephyr' if that exists, or the ZEPHYR_BASE value
        # in the calling environment.
        #
        # At some point, we need a more flexible way to set environment
        # variables based on manifest contents, but this is good enough
        # to get started with and to ask for wider testing.
        manifest = Manifest.from_file()
        for project in manifest.projects:
            if project.path == 'zephyr':
                zb = project.abspath
                zb_origin = 'manifest file {}'.format(manifest.path)
                break
        else:
            if zb_env is None:
                log.wrn('no --zephyr-base given, ZEPHYR_BASE is unset,',
                        'and no manifest project has path "zephyr"')
                zb = None
                zb_origin = None
            else:
                zb = zb_env
                zb_origin = 'environment'

    if zb_env and os.path.abspath(zb) != os.path.abspath(zb_env):
        # The environment ZEPHYR_BASE takes precedence over either the
        # command line or the manifest, but in normal multi-repo
        # operation we shouldn't expect to need to set ZEPHYR_BASE to
        # point to some random place. In practice, this is probably
        # happening because zephyr-env.sh/cmd was run in some other
        # zephyr installation, and the user forgot about that.
        log.wrn('ZEPHYR_BASE={}'.format(zb_env),
                'in the calling environment, but has been set to',
                zb, 'instead by the', zb_origin)

    os.environ['ZEPHYR_BASE'] = zb

    log.dbg('ZEPHYR_BASE={} (origin: {})'.format(zb, zb_origin))


def print_version_info():
    # The bootstrapper will print its own version, as well as that of
    # the west repository itself, then exit. So if this file is being
    # asked to print the version, it's because it's being run
    # directly, and not via the bootstrapper.
    #
    # Rather than play tricks like invoking "pip show west" (which
    # assumes the bootstrapper was installed via pip, the common but
    # not universal case), refuse the temptation to make guesses and
    # print an honest answer.
    log.inf('West bootstrapper version: N/A, not run via bootstrapper')

    # The running west installation.
    if IN_MULTIREPO_INSTALL:
        try:
            desc = check_output(['git', 'describe', '--tags'],
                                stderr=DEVNULL,
                                cwd=os.path.dirname(__file__))
            west_version = desc.decode(sys.getdefaultencoding()).strip()
        except CalledProcessError:
            west_version = 'unknown'
    else:
        west_version = 'N/A, monorepo installation'
    west_src_west = os.path.dirname(__file__)
    print('West repository version: {} ({})'.
          format(west_version,
                 os.path.dirname(os.path.dirname(west_src_west))))


def parse_args(argv):
    # The prog='west' override avoids the absolute path of the main.py script
    # showing up when West is run via the wrapper
    west_parser = argparse.ArgumentParser(
        prog='west', description='The Zephyr RTOS meta-tool.',
        epilog='Run "west <command> -h" for help on each command.')

    # Remember to update scripts/west-completion.bash if you add or remove
    # flags

    west_parser.add_argument('-z', '--zephyr-base', default=None,
                             help='''Override the Zephyr base directory. The
                             default is the manifest project with path
                             "zephyr".''')

    west_parser.add_argument('-v', '--verbose', default=0, action='count',
                             help='''Display verbose output. May be given
                             multiple times to increase verbosity.''')

    west_parser.add_argument('-V', '--version', action='store_true')

    subparser_gen = west_parser.add_subparsers(title='commands',
                                               dest='command')

    for command in COMMANDS:
        parser = command.add_parser(subparser_gen)
        parser.set_defaults(handler=partial(command_handler, command))

    args, unknown = west_parser.parse_known_args(args=argv)

    if args.version:
        print_version_info()
        sys.exit(0)

    # Set up logging verbosity before doing anything else, so
    # e.g. verbose messages related to argument handling errors
    # work properly.
    log.set_verbosity(args.verbose)

    if IN_MULTIREPO_INSTALL:
        set_zephyr_base(args)

    if 'handler' not in args:
        if IN_MULTIREPO_INSTALL:
            log.err('west installation found (in {}), but no command given'.
                    format(west_dir()))
        else:
            log.err('no west command given')
        west_parser.print_help(file=sys.stderr)
        sys.exit(1)

    return args, unknown


def main(argv=None):
    # Makes ANSI color escapes work on Windows, and strips them when
    # stdout/stderr isn't a terminal
    colorama.init()

    if argv is None:
        argv = sys.argv[1:]
    args, unknown = parse_args(argv)

    if IN_MULTIREPO_INSTALL:
        # Read the configuration files
        config.read_config()

    for_stack_trace = 'run as "west -v ... {} ..." for a stack trace'.format(
        args.command)
    try:
        args.handler(args, unknown)
    except WestUpdated:
        # West has been automatically updated. Restart ourselves to run the
        # latest version, with the same arguments that we were given.
        os.execv(sys.executable, [sys.executable] + argv)
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


if __name__ == "__main__":
    main()
