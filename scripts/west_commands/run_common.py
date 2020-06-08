# Copyright (c) 2018 Open Source Foundries Limited.
#
# SPDX-License-Identifier: Apache-2.0

'''Common code used by commands which execute runners.
'''

import argparse
import logging
from os import close, getcwd, path
from pathlib import Path
from subprocess import CalledProcessError
import sys
import tempfile
import textwrap
import traceback

from west import log
from build_helpers import find_build_dir, is_zephyr_build, \
    FIND_BUILD_DIR_DESCRIPTION
from west.commands import CommandError
from west.configuration import config
import yaml

from runners import get_runner_cls, ZephyrBinaryRunner, MissingProgram
from runners.core import RunnerConfig
import zcmake

# Context-sensitive help indentation.
# Don't change this, or output from argparse won't match up.
INDENT = ' ' * 2

if log.VERBOSE >= log.VERBOSE_NORMAL:
    # Using level 1 allows sub-DEBUG levels of verbosity. The
    # west.log module decides whether or not to actually print the
    # message.
    #
    # https://docs.python.org/3.7/library/logging.html#logging-levels.
    LOG_LEVEL = 1
else:
    LOG_LEVEL = logging.INFO

def _banner(msg):
    log.inf('-- ' + msg, colorize=True)

class WestLogFormatter(logging.Formatter):

    def __init__(self):
        super().__init__(fmt='%(name)s: %(message)s')

class WestLogHandler(logging.Handler):

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.setFormatter(WestLogFormatter())
        self.setLevel(LOG_LEVEL)

    def emit(self, record):
        fmt = self.format(record)
        lvl = record.levelno
        if lvl > logging.CRITICAL:
            log.die(fmt)
        elif lvl >= logging.ERROR:
            log.err(fmt)
        elif lvl >= logging.WARNING:
            log.wrn(fmt)
        elif lvl >= logging.INFO:
            _banner(fmt)
        elif lvl >= logging.DEBUG:
            log.dbg(fmt)
        else:
            log.dbg(fmt, level=log.VERBOSE_EXTREME)

def command_verb(command):
    return "flash" if command.name == "flash" else "debug"

def add_parser_common(command, parser_adder=None, parser=None):
    if parser_adder is not None:
        parser = parser_adder.add_parser(
            command.name,
            formatter_class=argparse.RawDescriptionHelpFormatter,
            help=command.help,
            description=command.description,
            epilog=FIND_BUILD_DIR_DESCRIPTION)

    # Remember to update scripts/west-completion.bash if you add or remove
    # flags

    parser.add_argument('-H', '--context', action='store_true',
                        help='''Rebuild application and print context-sensitive
                        help; this may be combined with --runner to restrict
                        output to a given runner.''')

    group = parser.add_argument_group(title='General options')

    group.add_argument('-d', '--build-dir', metavar='DIR',
                       help='zephyr build directory')
    group.add_argument('-c', '--cmake-cache', metavar='FILE',
                       help='override the default CMake cache file')
    group.add_argument('-r', '--runner',
                       help=f'overrides the default {command.name} runner')
    group.add_argument('--skip-rebuild', action='store_true',
                       help='do not rebuild the application first')

    group = parser.add_argument_group(title='Common runner options')

    group.add_argument('--board-dir', metavar='DIR',
                       help='zephyr board directory')
    # FIXME: we should just have a single --file argument.
    group.add_argument('--elf-file', metavar='FILE', help='path to zephyr.elf')
    group.add_argument('--hex-file', metavar='FILE', help='path to zephyr.hex')
    group.add_argument('--bin-file', metavar='FILE', help='path to zephyr.bin')
    # FIXME: these are runner-specific. Remove them from this location.
    group.add_argument('--gdb', help='path to GDB, if applicable')
    group.add_argument('--openocd', help='path to OpenOCD, if applicable')
    group.add_argument(
        '--openocd-search', metavar='DIR',
        help='path to add to OpenOCD search path, if applicable')

    return parser

def desc_common(command_name):
    return textwrap.dedent(f'''\
    Only common "west {command_name}" options are listed here.
    To get help on available runner-specific options, run:

      west {command_name} --context -d BUILD_DIR
    ''')

def do_run_common(command, user_args, user_runner_args):
    # This is the main routine for all the "west flash", "west debug",
    # etc. commands.

    if user_args.context:
        dump_context(command, user_args, user_runner_args)
        return

    command_name = command.name
    build_dir = get_build_dir(user_args)
    cache = load_cmake_cache(build_dir, user_args)
    board = cache['CACHED_BOARD']
    if not user_args.skip_rebuild:
        rebuild(command, build_dir, user_args)

    # Load runners.yaml.
    runners_yaml = runners_yaml_path(cache)
    runner_config = load_runners_yaml(runners_yaml, user_args)

    # Get a concrete ZephyrBinaryRunner subclass to use based on
    # runners.yaml and command line arguments.
    runner_cls = use_runner_cls(command, board, user_args, runner_config,
                                cache)
    runner_name = runner_cls.name()

    # Set up runner logging to delegate to west.log commands.
    logger = logging.getLogger('runners')
    logger.setLevel(LOG_LEVEL)
    logger.addHandler(WestLogHandler())

    # If the user passed -- to force the parent argument parser to stop
    # parsing, it will show up here, and needs to be filtered out.
    runner_args = [arg for arg in user_runner_args if arg != '--']

    # Arguments are provided in this order to allow the specific to
    # override the general:
    #
    # - common runners.yaml arguments
    # - runner-specific runners.yaml arguments
    # - command line arguments
    final_argv = (runner_config['args']['common'] +
                  runner_config['args'][runner_name] +
                  runner_args)

    # 'user_args' contains parsed arguments which are:
    #
    # 1. provided on the command line, and
    # 2. handled by add_parser_common(), and
    # 3. *not* runner-specific
    #
    # 'final_argv' contains unparsed arguments from either:
    #
    # 1. runners.yaml, or
    # 2. the command line
    #
    # We next have to:
    #
    # - parse 'final_argv' now that we have all the command line
    #   arguments
    # - create a RunnerConfig using 'user_args' and the result
    #   of parsing 'final_argv'
    parser = argparse.ArgumentParser(prog=runner_name)
    add_parser_common(command, parser=parser)
    runner_cls.add_parser(parser)
    args, unknown = parser.parse_known_args(args=final_argv)
    if unknown:
        log.die(f'runner {runner_name} received unknown arguments: {unknown}')

    # Override args with any user_args. The latter must take
    # precedence, or e.g. --hex-file on the command line would be
    # ignored in favor of a board.cmake setting.
    for a, v in vars(user_args).items():
        if v is not None:
            setattr(args, a, v)

    # Create the RunnerConfig from the values assigned to common
    # arguments. This is a hacky way to go about this; probably
    # ZephyrBinaryRunner should define what it needs to make this
    # happen by itself. That would be a larger refactoring of the
    # runners package than there's time for right now, though.
    #
    # Use that RunnerConfig to create the ZephyrBinaryRunner instance
    # and call its run().
    runner = runner_cls.create(runner_cfg_from_args(args, build_dir), args)
    try:
        runner.run(command_name)
    except ValueError as ve:
        log.err(str(ve), fatal=True)
        dump_traceback()
        raise CommandError(1)
    except MissingProgram as e:
        log.die('required program', e.filename,
                'not found; install it or add its location to PATH')
    except RuntimeError as re:
        if not user_args.verbose:
            log.die(re)
        else:
            log.err('verbose mode enabled, dumping stack:', fatal=True)
            raise

def get_build_dir(args, die_if_none=True):
    # Get the build directory for the given argument list and environment.
    if args.build_dir:
        return args.build_dir

    guess = config.get('build', 'guess-dir', fallback='never')
    guess = guess == 'runners'
    dir = find_build_dir(None, guess)

    if dir and is_zephyr_build(dir):
        return dir
    elif die_if_none:
        msg = '--build-dir was not given, '
        if dir:
            msg = msg + 'and neither {} nor {} are zephyr build directories.'
        else:
            msg = msg + ('{} is not a build directory and the default build '
                         'directory cannot be determined. Check your '
                         'build.dir-fmt configuration option')
        log.die(msg.format(getcwd(), dir))
    else:
        return None

def load_cmake_cache(build_dir, args):
    cache_file = path.join(build_dir, args.cmake_cache or zcmake.DEFAULT_CACHE)
    try:
        return zcmake.CMakeCache(cache_file)
    except FileNotFoundError:
        log.die(f'no CMake cache found (expected one at {cache_file})')

def rebuild(command, build_dir, args):
    _banner(f'west {command.name}: rebuilding')
    try:
        zcmake.run_build(build_dir)
    except CalledProcessError:
        if args.build_dir:
            log.die(f're-build in {args.build_dir} failed')
        else:
            log.die(f're-build in {build_dir} failed (no --build-dir given)')

def runners_yaml_path(cache):
    try:
        return cache['ZEPHYR_RUNNERS_YAML']
    except KeyError:
        board = cache.get('CACHED_BOARD')
        log.die(f'either a pristine build is needed, or board {board} '
                "doesn't support west flash/debug "
                '(no ZEPHYR_RUNNERS_YAML in CMake cache)')

def load_runners_yaml(path, args):
    # Load runners.yaml using its location in the CMake cache,
    # allowing a command line override for the cache file location.

    try:
        with open(path, 'r') as f:
            config = yaml.safe_load(f.read())
    except FileNotFoundError:
        log.die(f'runners.yaml file not found: {path}')

    if not config.get('runners'):
        log.wrn(f'no pre-configured runners in {path}; '
                "this probably won't work")

    return config

def use_runner_cls(command, board, args, runner_config, cache):
    # Get the ZephyrBinaryRunner class from its name, and make sure it
    # supports the command. Print a message about the choice, and
    # return the class.

    runner = args.runner or runner_config.get(command.runner_key)
    if runner is None:
        log.die(f'no {command.name} runner available for board {board}. '
                "Check the board's documentation for instructions.")

    _banner(f'west {command.name}: using runner {runner}')

    available = runner_config.get('runners', [])
    if runner not in available:
        if 'BOARD_DIR' in cache:
            board_cmake = Path(cache['BOARD_DIR']) / 'board.cmake'
        else:
            board_cmake = 'board.cmake'
        log.err(f'board {board} does not support runner {runner}',
                fatal=True)
        log.inf(f'To fix, configure this runner in {board_cmake} and rebuild.')
        sys.exit(1)
    runner_cls = get_runner_cls(runner)
    if command.name not in runner_cls.capabilities().commands:
        log.die(f'runner {runner} does not support command {command.name}')

    return runner_cls

def runner_cfg_from_args(args, build_dir):
    return RunnerConfig(build_dir, args.board_dir, args.elf_file,
                        args.hex_file, args.bin_file,
                        gdb=args.gdb, openocd=args.openocd,
                        openocd_search=args.openocd_search)

def dump_traceback():
    # Save the current exception to a file and return its path.
    fd, name = tempfile.mkstemp(prefix='west-exc-', suffix='.txt')
    close(fd)        # traceback has no use for the fd
    with open(name, 'w') as f:
        traceback.print_exc(file=f)
    log.inf("An exception trace has been saved in", name)

#
# west {command} --context
#

def dump_context(command, args, unknown_args):
    build_dir = get_build_dir(args, die_if_none=False)
    if build_dir is None:
        log.wrn('no --build-dir given or found; output will be limited')
        runner_config = None
    else:
        cache = load_cmake_cache(build_dir, args)
        board = cache['CACHED_BOARD']
        runners_yaml = runners_yaml_path(cache)
        runner_config = load_runners_yaml(runners_yaml, args)

    # Re-build unless asked not to, to make sure the output is up to date.
    if build_dir and not args.skip_rebuild:
        rebuild(command, build_dir, args)

    if args.runner:
        try:
            cls = get_runner_cls(args.runner)
        except ValueError:
            log.die(f'invalid runner name {args.runner}; choices: ' +
                    ', '.join(cls.name() for cls in
                              ZephyrBinaryRunner.get_runners()))
    else:
        cls = None

    if runner_config is None:
        dump_context_no_config(command, cls)
    else:
        log.inf(f'build configuration:', colorize=True)
        log.inf(f'{INDENT}build directory: {build_dir}')
        log.inf(f'{INDENT}board: {board}')
        log.inf(f'{INDENT}runners.yaml: {runners_yaml}')
        if cls:
            dump_runner_context(command, cls, runner_config)
        else:
            dump_all_runner_context(command, runner_config, board, build_dir)

def dump_context_no_config(command, cls):
    if not cls:
        all_cls = {cls.name(): cls for cls in ZephyrBinaryRunner.get_runners()
                   if command.name in cls.capabilities().commands}
        log.inf('all Zephyr runners which support {}:'.format(command.name),
                colorize=True)
        dump_wrapped_lines(', '.join(all_cls.keys()), INDENT)
        log.inf()
        log.inf('Note: use -r RUNNER to limit information to one runner.')
    else:
        # This does the right thing with runner_config=None.
        dump_runner_context(command, cls, None)

def dump_runner_context(command, cls, runner_config, indent=''):
    dump_runner_caps(cls, indent)
    dump_runner_option_help(cls, indent)

    if runner_config is None:
        return

    if cls.name() in runner_config['runners']:
        dump_runner_args(cls.name(), runner_config, indent)
    else:
        log.wrn(f'support for runner {cls.name()} is not configured '
                f'in this build directory')

def dump_runner_caps(cls, indent=''):
    # Print RunnerCaps for the given runner class.

    log.inf(f'{indent}{cls.name()} capabilities:', colorize=True)
    log.inf(f'{indent}{INDENT}{cls.capabilities()}')

def dump_runner_option_help(cls, indent=''):
    # Print help text for class-specific command line options for the
    # given runner class.

    dummy_parser = argparse.ArgumentParser(prog='', add_help=False)
    cls.add_parser(dummy_parser)
    formatter = dummy_parser._get_formatter()
    for group in dummy_parser._action_groups:
        # Break the abstraction to filter out the 'flash', 'debug', etc.
        # TODO: come up with something cleaner (may require changes
        # in the runner core).
        actions = group._group_actions
        if len(actions) == 1 and actions[0].dest == 'command':
            # This is the lone positional argument. Skip it.
            continue
        formatter.start_section('REMOVE ME')
        formatter.add_text(group.description)
        formatter.add_arguments(actions)
        formatter.end_section()
    # Get the runner help, with the "REMOVE ME" string gone
    runner_help = f'\n{indent}'.join(formatter.format_help().splitlines()[1:])

    log.inf(f'{indent}{cls.name()} options:', colorize=True)
    log.inf(indent + runner_help)

def dump_runner_args(group, runner_config, indent=''):
    msg = f'{indent}{group} arguments from runners.yaml:'
    args = runner_config['args'][group]
    if args:
        log.inf(msg, colorize=True)
        for arg in args:
            log.inf(f'{indent}{INDENT}{arg}')
    else:
        log.inf(f'{msg} (none)', colorize=True)

def dump_all_runner_context(command, runner_config, board, build_dir):
    all_cls = {cls.name(): cls for cls in ZephyrBinaryRunner.get_runners() if
               command.name in cls.capabilities().commands}
    available = runner_config['runners']
    available_cls = {r: all_cls[r] for r in available if r in all_cls}
    default_runner = runner_config[command.runner_key]

    log.inf(f'zephyr runners which support "west {command.name}":',
            colorize=True)
    dump_wrapped_lines(', '.join(all_cls.keys()), INDENT)
    log.inf()
    dump_wrapped_lines('Note: not all may work with this board and build '
                       'directory. Available runners are listed below.',
                       INDENT)

    log.inf(f'available runners in runners.yaml:',
            colorize=True)
    dump_wrapped_lines(', '.join(available), INDENT)
    log.inf(f'default runner in runners.yaml:', colorize=True)
    log.inf(INDENT + default_runner)
    dump_runner_args('common', runner_config)
    log.inf('runner-specific context:', colorize=True)
    for cls in available_cls.values():
        dump_runner_context(command, cls, runner_config, INDENT)

    if len(available) > 1:
        log.inf()
        log.inf('Note: use -r RUNNER to limit information to one runner.')

def dump_wrapped_lines(text, indent):
    for line in textwrap.wrap(text, initial_indent=indent,
                              subsequent_indent=indent,
                              break_on_hyphens=False,
                              break_long_words=False):
        log.inf(line)
