# Copyright (c) 2018 Open Source Foundries Limited.
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

'''Common code used by commands which execute runners.
'''

import importlib.util
import re
import argparse
import logging
from collections import defaultdict
from os import close, getcwd, path, fspath
from pathlib import Path
from subprocess import CalledProcessError
import sys
import tempfile
import textwrap
import traceback

from dataclasses import dataclass
from west import log
from build_helpers import find_build_dir, is_zephyr_build, load_domains, \
    FIND_BUILD_DIR_DESCRIPTION
from west.commands import CommandError
from west.configuration import config
from runners.core import FileType
from runners.core import BuildConfiguration
import yaml

import zephyr_module
from zephyr_ext_common import ZEPHYR_BASE, ZEPHYR_SCRIPTS

# Runners depend on edtlib. Make sure the copy in the tree is
# available to them before trying to import any.
sys.path.insert(0, str(ZEPHYR_SCRIPTS / 'dts' / 'python-devicetree' / 'src'))

from runners import get_runner_cls, ZephyrBinaryRunner, MissingProgram
from runners.core import RunnerConfig
import zcmake

# Context-sensitive help indentation.
# Don't change this, or output from argparse won't match up.
INDENT = ' ' * 2

IGNORED_RUN_ONCE_PRIORITY = -1
SOC_FILE_RUN_ONCE_DEFAULT_PRIORITY = 0
BOARD_FILE_RUN_ONCE_DEFAULT_PRIORITY = 10

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

@dataclass
class UsedFlashCommand:
    command: str
    boards: list
    runners: list
    first: bool
    ran: bool = False

@dataclass
class ImagesFlashed:
    flashed: int = 0
    total: int = 0

@dataclass
class SocBoardFilesProcessing:
    filename: str
    board: bool = False
    priority: int = IGNORED_RUN_ONCE_PRIORITY
    yaml: object = None

def import_from_path(module_name, file_path):
    spec = importlib.util.spec_from_file_location(module_name, file_path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[module_name] = module
    spec.loader.exec_module(module)
    return module

def command_verb(command):
    return "flash" if command.name == "flash" else "debug"

def add_parser_common(command, parser_adder=None, parser=None):
    if parser_adder is not None:
        parser = parser_adder.add_parser(
            command.name,
            formatter_class=argparse.RawDescriptionHelpFormatter,
            help=command.help,
            description=command.description)

    # Remember to update west-completion.bash if you add or remove
    # flags

    group = parser.add_argument_group('general options',
                                      FIND_BUILD_DIR_DESCRIPTION)

    group.add_argument('-d', '--build-dir', metavar='DIR',
                       help='application build directory')
    # still supported for backwards compatibility, but questionably
    # useful now that we do everything with runners.yaml
    group.add_argument('-c', '--cmake-cache', metavar='FILE',
                       help=argparse.SUPPRESS)
    group.add_argument('-r', '--runner',
                       help='override default runner from --build-dir')
    group.add_argument('--skip-rebuild', action='store_true',
                       help='do not refresh cmake dependencies first')
    group.add_argument('--domain', action='append',
                       help='execute runner only for given domain')

    group = parser.add_argument_group(
        'runner configuration',
        textwrap.dedent(f'''\
        ===================================================================
          IMPORTANT:
          Individual runners support additional options not printed here.
        ===================================================================

        Run "west {command.name} --context" for runner-specific options.

        If a build directory is found, --context also prints per-runner
        settings found in that build directory's runners.yaml file.

        Use "west {command.name} --context -r RUNNER" to limit output to a
        specific RUNNER.

        Some runner settings also can be overridden with options like
        --hex-file. However, this depends on the runner: not all runners
        respect --elf-file / --hex-file / --bin-file, nor use gdb or openocd,
        etc.'''))
    group.add_argument('-H', '--context', action='store_true',
                       help='print runner- and build-specific help')
    # Options used to override RunnerConfig values in runners.yaml.
    # TODO: is this actually useful?
    group.add_argument('--board-dir', metavar='DIR', help='board directory')
    # FIXME: these are runner-specific and should be moved to where --context
    # can find them instead.
    group.add_argument('--gdb', help='path to GDB')
    group.add_argument('--openocd', help='path to openocd')
    group.add_argument(
        '--openocd-search', metavar='DIR', action='append',
        help='path to add to openocd search path, if applicable')

    return parser

def do_run_common(command, user_args, user_runner_args, domain_file=None):
    # This is the main routine for all the "west flash", "west debug",
    # etc. commands.

    # Holds a list of run once commands, this is useful for sysbuild images
    # whereby there are multiple images per board with flash commands that can
    # interfere with other images if they run one per time an image is flashed.
    used_cmds = []

    # Holds a set of processed board names for flash running information.
    processed_boards = set()

    # Holds a dictionary of board image flash counts, the first element is
    # number of images flashed so far and second element is total number of
    # images for a given board.
    board_image_count = defaultdict(ImagesFlashed)

    highest_priority = IGNORED_RUN_ONCE_PRIORITY
    highest_entry = None
    check_files = []

    if user_args.context:
        dump_context(command, user_args, user_runner_args)
        return

    # Import external module runners
    for module in zephyr_module.parse_modules(ZEPHYR_BASE, command.manifest):
        runners_ext = module.meta.get("runners", [])
        for runner in runners_ext:
            import_from_path(
                module.meta.get("name", "runners_ext"), Path(module.project) / runner["file"]
            )

    build_dir = get_build_dir(user_args)
    if not user_args.skip_rebuild:
        rebuild(command, build_dir, user_args)

    if domain_file is None:
        if user_args.domain is None:
            # No domains are passed down and no domains specified by the user.
            # So default domain will be used.
            domains = [load_domains(build_dir).get_default_domain()]
        else:
            # No domains are passed down, but user has specified domains to use.
            # Get the user specified domains.
            domains = load_domains(build_dir).get_domains(user_args.domain)
    else:
        domains = load_domains(build_dir).get_domains(user_args.domain,
                                                      default_flash_order=True)

    if len(domains) > 1:
        if len(user_runner_args) > 0:
            log.wrn("Specifying runner options for multiple domains is experimental.\n"
                    "If problems are experienced, please specify a single domain "
                    "using '--domain <domain>'")

        # Process all domains to load board names and populate flash runner
        # parameters.
        board_names = set()
        for d in domains:
            if d.build_dir is None:
                build_dir = get_build_dir(user_args)
            else:
                build_dir = d.build_dir

            cache = load_cmake_cache(build_dir, user_args)
            build_conf = BuildConfiguration(build_dir)
            board = build_conf.get('CONFIG_BOARD_TARGET')
            board_names.add(board)
            board_image_count[board].total += 1

            # Load board flash runner configuration (if it exists) and store
            # single-use commands in a dictionary so that they get executed
            # once per unique board name.
            for directory in cache.get_list('SOC_DIRECTORIES'):
                if directory not in processed_boards:
                    check_files.append(SocBoardFilesProcessing(Path(directory) / 'soc.yml'))
                    processed_boards.add(directory)

            for directory in cache.get_list('BOARD_DIRECTORIES'):
                if directory not in processed_boards:
                    check_files.append(SocBoardFilesProcessing(Path(directory) / 'board.yml', True))
                    processed_boards.add(directory)

        for check in check_files:
            try:
                with open(check.filename, 'r') as f:
                    check.yaml = yaml.safe_load(f.read())

                    if 'runners' not in check.yaml:
                        continue
                    elif check.board is False and 'run_once' not in check.yaml['runners']:
                        continue

                    if 'priority' in check.yaml['runners']:
                        check.priority = check.yaml['runners']['priority']
                    else:
                        check.priority = BOARD_FILE_RUN_ONCE_DEFAULT_PRIORITY if check.board is True else SOC_FILE_RUN_ONCE_DEFAULT_PRIORITY

                    if check.priority == highest_priority:
                        log.die("Duplicate flash run once configuration found with equal priorities")

                    elif check.priority > highest_priority:
                        highest_priority = check.priority
                        highest_entry = check

            except FileNotFoundError:
                continue

        if highest_entry is not None:
            group_type = 'boards' if highest_entry.board is True else 'qualifiers'

            for cmd in highest_entry.yaml['runners']['run_once']:
                for data in highest_entry.yaml['runners']['run_once'][cmd]:
                    for group in data['groups']:
                        run_first = bool(data['run'] == 'first')
                        if group_type == 'qualifiers':
                            targets = []
                            for target in group[group_type]:
                                # For SoC-based qualifiers, prepend to the beginning of the
                                # match to allow for matching any board name
                                targets.append('([^/]+)/' + target)
                        else:
                            targets = group[group_type]

                        used_cmds.append(UsedFlashCommand(cmd, targets, data['runners'], run_first))

    # Reduce entries to only those having matching board names (either exact or with regex) and
    # remove any entries with empty board lists
    for i, entry in enumerate(used_cmds):
        for l, match in enumerate(entry.boards):
            match_found = False

            # Check if there is a matching board for this regex
            for check in board_names:
                if re.match(fr'^{match}$', check) is not None:
                    match_found = True
                    break

            if not match_found:
                del entry.boards[l]

        if len(entry.boards) == 0:
            del used_cmds[i]

    prev_runner = None
    for d in domains:
        prev_runner = do_run_common_image(command, user_args, user_runner_args, used_cmds,
                                          board_image_count, d.build_dir, prev_runner)


def do_run_common_image(command, user_args, user_runner_args, used_cmds,
                        board_image_count, build_dir=None, prev_runner=None):
    global re
    command_name = command.name
    if build_dir is None:
        build_dir = get_build_dir(user_args)
    cache = load_cmake_cache(build_dir, user_args)
    build_conf = BuildConfiguration(build_dir)
    board = build_conf.get('CONFIG_BOARD_TARGET')

    if board_image_count is not None and board in board_image_count:
        board_image_count[board].flashed += 1

    # Load runners.yaml.
    yaml_path = runners_yaml_path(build_dir, board)
    runners_yaml = load_runners_yaml(yaml_path)

    # Get a concrete ZephyrBinaryRunner subclass to use based on
    # runners.yaml and command line arguments.
    runner_cls = use_runner_cls(command, board, user_args, runners_yaml,
                                cache)
    runner_name = runner_cls.name()

    # Set up runner logging to delegate to west.log commands.
    logger = logging.getLogger('runners')
    logger.setLevel(LOG_LEVEL)
    if not logger.hasHandlers():
        # Only add a runners log handler if none has been added already.
        logger.addHandler(WestLogHandler())

    # If the user passed -- to force the parent argument parser to stop
    # parsing, it will show up here, and needs to be filtered out.
    runner_args = [arg for arg in user_runner_args if arg != '--']

    # Check if there are any commands that should only be ran once per board
    # and if so, remove them for all but the first iteration of the flash
    # runner per unique board name.
    if len(used_cmds) > 0 and len(runner_args) > 0:
        i = len(runner_args) - 1
        while i >= 0:
            for cmd in used_cmds:
                if cmd.command == runner_args[i] and (runner_name in cmd.runners or 'all' in cmd.runners):
                    # Check if board is here
                    match_found = False

                    for match in cmd.boards:
                        # Check if there is a matching board for this regex
                        if re.match(fr'^{match}$', board) is not None:
                            match_found = True
                            break

                    if not match_found:
                        continue

                    # Check if this is a first or last run
                    if not cmd.first:
                        # For last run instances, we need to check that this really is the last
                        # image of all boards being flashed
                        for check in cmd.boards:
                            can_continue = False

                            for match in board_image_count:
                                if re.match(fr'^{check}$', match) is not None:
                                    if board_image_count[match].flashed == board_image_count[match].total:
                                        can_continue = True
                                        break

                        if not can_continue:
                            continue

                    if not cmd.ran:
                        cmd.ran = True
                    else:
                        runner_args.pop(i)

                    break

            i = i - 1

    # If flashing multiple images, the runner supports reset after flashing and
    # the board has enabled this functionality, check if the board should be
    # reset or not. If this is not specified in the board/soc file, leave it up to
    # the runner's default configuration to decide if a reset should occur.
    if runner_cls.capabilities().reset:
        if board_image_count is not None:
            reset = True

            for cmd in used_cmds:
                if cmd.command == '--reset' and (runner_name in cmd.runners or 'all' in cmd.runners):
                    # Check if board is here
                    match_found = False

                    for match in cmd.boards:
                        if re.match(fr'^{match}$', board) is not None:
                            match_found = True
                            break

                    if not match_found:
                        continue

                    # Check if this is a first or last run
                    if cmd.first and cmd.ran:
                        reset = False
                        break
                    elif not cmd.first and not cmd.ran:
                        # For last run instances, we need to check that this really is the last
                        # image of all boards being flashed
                        for check in cmd.boards:
                            can_continue = False

                            for match in board_image_count:
                                if re.match(fr'^{check}$', match) is not None:
                                    if board_image_count[match].flashed != board_image_count[match].total:
                                        reset = False
                                        break

            if reset:
                runner_args.append('--reset')
            else:
                runner_args.append('--no-reset')

    # Arguments in this order to allow specific to override general:
    #
    # - runner-specific runners.yaml arguments
    # - user-provided command line arguments
    final_argv = runners_yaml['args'][runner_name] + runner_args

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
    parser = argparse.ArgumentParser(prog=runner_name, allow_abbrev=False)
    add_parser_common(command, parser=parser)
    runner_cls.add_parser(parser)
    args, unknown = parser.parse_known_args(args=final_argv)
    if unknown:
        log.die(f'runner {runner_name} received unknown arguments: {unknown}')

    # Propagate useful args from previous domain invocations
    if prev_runner is not None:
        runner_cls.args_from_previous_runner(prev_runner, args)

    # Override args with any user_args. The latter must take
    # precedence, or e.g. --hex-file on the command line would be
    # ignored in favor of a board.cmake setting.
    for a, v in vars(user_args).items():
        if v is not None:
            setattr(args, a, v)

    # Create the RunnerConfig from runners.yaml and any command line
    # overrides.
    runner_config = get_runner_config(build_dir, yaml_path, runners_yaml, args)
    log.dbg(f'runner_config: {runner_config}', level=log.VERBOSE_VERY)

    # Use that RunnerConfig to create the ZephyrBinaryRunner instance
    # and call its run().
    try:
        runner = runner_cls.create(runner_config, args)
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
    return runner

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

def runners_yaml_path(build_dir, board):
    ret = Path(build_dir) / 'zephyr' / 'runners.yaml'
    if not ret.is_file():
        log.die(f'no runners.yaml found in {build_dir}/zephyr. '
        f"Either board {board} doesn't support west flash/debug/simulate,"
        ' or a pristine build is needed.')
    return ret

def load_runners_yaml(path):
    # Load runners.yaml and convert to Python object.

    try:
        with open(path, 'r') as f:
            content = yaml.safe_load(f.read())
    except FileNotFoundError:
        log.die(f'runners.yaml file not found: {path}')

    if not content.get('runners'):
        log.wrn(f'no pre-configured runners in {path}; '
                "this probably won't work")

    return content

def use_runner_cls(command, board, args, runners_yaml, cache):
    # Get the ZephyrBinaryRunner class from its name, and make sure it
    # supports the command. Print a message about the choice, and
    # return the class.

    runner = args.runner or runners_yaml.get(command.runner_key)
    if runner is None:
        log.die(f'no {command.name} runner available for board {board}. '
                "Check the board's documentation for instructions.")

    _banner(f'west {command.name}: using runner {runner}')

    available = runners_yaml.get('runners', [])
    if runner not in available:
        if 'BOARD_DIR' in cache:
            board_cmake = Path(cache['BOARD_DIR']) / 'board.cmake'
        else:
            board_cmake = 'board.cmake'
        log.err(f'board {board} does not support runner {runner}',
                fatal=True)
        log.inf(f'To fix, configure this runner in {board_cmake} and rebuild.')
        sys.exit(1)
    try:
        runner_cls = get_runner_cls(runner)
    except ValueError as e:
        log.die(e)
    if command.name not in runner_cls.capabilities().commands:
        log.die(f'runner {runner} does not support command {command.name}')

    return runner_cls

def get_runner_config(build_dir, yaml_path, runners_yaml, args=None):
    # Get a RunnerConfig object for the current run. yaml_config is
    # runners.yaml's config: map, and args are the command line arguments.
    yaml_config = runners_yaml['config']
    yaml_dir = yaml_path.parent
    if args is None:
        args = argparse.Namespace()

    def output_file(filetype):

        from_args = getattr(args, f'{filetype}_file', None)
        if from_args is not None:
            return from_args

        from_yaml = yaml_config.get(f'{filetype}_file')
        if from_yaml is not None:
            # Output paths in runners.yaml are relative to the
            # directory containing the runners.yaml file.
            return fspath(yaml_dir / from_yaml)

        return None

    def config(attr, default=None):
        return getattr(args, attr, None) or yaml_config.get(attr, default)

    def filetype(attr):
        ftype = str(getattr(args, attr, None)).lower()
        if ftype == "hex":
            return FileType.HEX
        elif ftype == "bin":
            return FileType.BIN
        elif ftype == "elf":
            return FileType.ELF
        elif getattr(args, attr, None) is not None:
            err = 'unknown --file-type ({}). Please use hex, bin or elf'
            raise ValueError(err.format(ftype))

        # file-type not provided, try to get from filename
        file = getattr(args, "file", None)
        if file is not None:
            ext = Path(file).suffix
            if ext == ".hex":
                return FileType.HEX
            if ext == ".bin":
                return FileType.BIN
            if ext == ".elf":
                return FileType.ELF

        # we couldn't get the file-type, set to
        # OTHER and let the runner deal with it
        return FileType.OTHER

    return RunnerConfig(build_dir,
                        yaml_config['board_dir'],
                        output_file('elf'),
                        output_file('exe'),
                        output_file('hex'),
                        output_file('bin'),
                        output_file('uf2'),
                        config('file'),
                        filetype('file_type'),
                        config('gdb'),
                        config('openocd'),
                        config('openocd_search', []),
                        config('rtt_address'))

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
        runners_yaml = None
    else:
        build_conf = BuildConfiguration(build_dir)
        board = build_conf.get('CONFIG_BOARD_TARGET')
        yaml_path = runners_yaml_path(build_dir, board)
        runners_yaml = load_runners_yaml(yaml_path)

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

    if runners_yaml is None:
        dump_context_no_config(command, cls)
    else:
        log.inf(f'build configuration:', colorize=True)
        log.inf(f'{INDENT}build directory: {build_dir}')
        log.inf(f'{INDENT}board: {board}')
        log.inf(f'{INDENT}runners.yaml: {yaml_path}')
        if cls:
            dump_runner_context(command, cls, runners_yaml)
        else:
            dump_all_runner_context(command, runners_yaml, board, build_dir)

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
        # This does the right thing with a None argument.
        dump_runner_context(command, cls, None)

def dump_runner_context(command, cls, runners_yaml, indent=''):
    dump_runner_caps(cls, indent)
    dump_runner_option_help(cls, indent)

    if runners_yaml is None:
        return

    if cls.name() in runners_yaml['runners']:
        dump_runner_args(cls.name(), runners_yaml, indent)
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

    dummy_parser = argparse.ArgumentParser(prog='', add_help=False, allow_abbrev=False)
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

def dump_runner_args(group, runners_yaml, indent=''):
    msg = f'{indent}{group} arguments from runners.yaml:'
    args = runners_yaml['args'][group]
    if args:
        log.inf(msg, colorize=True)
        for arg in args:
            log.inf(f'{indent}{INDENT}{arg}')
    else:
        log.inf(f'{msg} (none)', colorize=True)

def dump_all_runner_context(command, runners_yaml, board, build_dir):
    all_cls = {cls.name(): cls for cls in ZephyrBinaryRunner.get_runners() if
               command.name in cls.capabilities().commands}
    available = runners_yaml['runners']
    available_cls = {r: all_cls[r] for r in available if r in all_cls}
    default_runner = runners_yaml[command.runner_key]
    yaml_path = runners_yaml_path(build_dir, board)
    runners_yaml = load_runners_yaml(yaml_path)

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
    log.inf('common runner configuration:', colorize=True)
    runner_config = get_runner_config(build_dir, yaml_path, runners_yaml)
    for field, value in zip(runner_config._fields, runner_config):
        log.inf(f'{INDENT}- {field}: {value}')
    log.inf('runner-specific context:', colorize=True)
    for cls in available_cls.values():
        dump_runner_context(command, cls, runners_yaml, INDENT)

    if len(available) > 1:
        log.inf()
        log.inf('Note: use -r RUNNER to limit information to one runner.')

def dump_wrapped_lines(text, indent):
    for line in textwrap.wrap(text, initial_indent=indent,
                              subsequent_indent=indent,
                              break_on_hyphens=False,
                              break_long_words=False):
        log.inf(line)
