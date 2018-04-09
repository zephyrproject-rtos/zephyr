#! /usr/bin/env python3

# Copyright (c) 2018 Open Source Foundries Limited.
#
# SPDX-License-Identifier: Apache-2.0

"""Zephyr runner wrapper script

This helper script is a user-friendly front-end to Zephyr's "runner"
Python package. The runner abstracts operations like flashing and
debugging with cross-platform interfaces wrapping tools like pyocd,
OpenOCD, etc.

The Zephyr build system stores parameters needed by this script in the
CMake cache file.  The build system flash, debug, and debugserver
targets then invoke the script, which creates a runner using the
cached parameters.

Users can also invoke the script directly to dynamically modify
arguments without changing the cache, use a different cache file, etc.
"""


import argparse
from collections import OrderedDict
from functools import partial
from os import path
import re
import shutil
import subprocess
import sys
import textwrap

from runner.core import ZephyrBinaryRunner, quote_sh_list


COMMANDS = ('flash', 'debug', 'debugserver')
COMMAND_HELP = {
    'flash': 'flash and run a binary onto a board',
    'debug': 'connect debugger and drop into a debug session',
    'debugserver': 'connect debugger and accept debug session connections',
    }
DEFAULT_CACHE = 'CMakeCache.txt'
PROGRAM = path.basename(sys.argv[0])
INDENT = ' ' * 2  # Don't change this, or the argparse indent won't match up.


class UnsupportedCommandError(RuntimeError):
    pass


class CMakeCacheEntry:
    '''Represents a CMake cache entry.

    This class understands the type system in a CMakeCache.txt, and
    converts the following cache types to Python types:

    Cache Type    Python type
    ----------    -------------------------------------------
    FILEPATH      str
    PATH          str
    STRING        str OR list of str (if ';' is in the value)
    BOOL          bool
    INTERNAL      str OR list of str (if ';' is in the value)
    ----------    -------------------------------------------
    '''

    # Regular expression for a cache entry.
    #
    # CMake variable names can include escape characters, allowing a
    # wider set of names than is easy to match with a regular
    # expresion. To be permissive here, use a non-greedy match up to
    # the first colon (':'). This breaks if the variable name has a
    # colon inside, but it's good enough.
    CACHE_ENTRY = re.compile(
        r'''(?P<name>.*?)                               # name
         :(?P<type>FILEPATH|PATH|STRING|BOOL|INTERNAL)  # type
         =(?P<value>.*)                                 # value
        ''', re.X)

    @classmethod
    def _to_bool(cls, val):
        # Convert a CMake BOOL string into a Python bool.
        #
        #   "True if the constant is 1, ON, YES, TRUE, Y, or a
        #   non-zero number. False if the constant is 0, OFF, NO,
        #   FALSE, N, IGNORE, NOTFOUND, the empty string, or ends in
        #   the suffix -NOTFOUND. Named boolean constants are
        #   case-insensitive. If the argument is not one of these
        #   constants, it is treated as a variable."
        #
        # https://cmake.org/cmake/help/v3.0/command/if.html
        val = val.upper()
        if val in ('ON', 'YES', 'TRUE', 'Y'):
            return True
        elif val in ('OFF', 'NO', 'FALSE', 'N', 'IGNORE', 'NOTFOUND', ''):
            return False
        elif val.endswith('-NOTFOUND'):
            return False
        else:
            try:
                v = int(val)
                return v != 0
            except ValueError as exc:
                raise ValueError('invalid bool {}'.format(val)) from exc

    @classmethod
    def from_line(cls, line, line_no):
        # Comments can only occur at the beginning of a line.
        # (The value of an entry could contain a comment character).
        if line.startswith('//') or line.startswith('#'):
            return None

        # Whitespace-only lines do not contain cache entries.
        if not line.strip():
            return None

        m = cls.CACHE_ENTRY.match(line)
        if not m:
            return None

        name, type_, value = (m.group(g) for g in ('name', 'type', 'value'))
        if type_ == 'BOOL':
            try:
                value = cls._to_bool(value)
            except ValueError as exc:
                args = exc.args + ('on line {}: {}'.format(line_no, line),)
                raise ValueError(args) from exc
        elif type_ == 'STRING' or type_ == 'INTERNAL':
            # If the value is a CMake list (i.e. is a string which
            # contains a ';'), convert to a Python list.
            if ';' in value:
                value = value.split(';')

        return CMakeCacheEntry(name, value)

    def __init__(self, name, value):
        self.name = name
        self.value = value

    def __str__(self):
        fmt = 'CMakeCacheEntry(name={}, value={})'
        return fmt.format(self.name, self.value)


class CMakeCache:
    '''Parses and represents a CMake cache file.'''

    def __init__(self, cache_file):
        self.load(cache_file)

    def load(self, cache_file):
        entries = []
        with open(cache_file, 'r') as cache:
            for line_no, line in enumerate(cache):
                entry = CMakeCacheEntry.from_line(line, line_no)
                if entry:
                    entries.append(entry)
        self._entries = OrderedDict((e.name, e) for e in entries)

    def get(self, name, default=None):
        entry = self._entries.get(name)
        if entry is not None:
            return entry.value
        else:
            return default

    def get_list(self, name, default=None):
        if default is None:
            default = []
        entry = self._entries.get(name)
        if entry is not None:
            value = entry.value
            if isinstance(value, list):
                return value
            elif isinstance(value, str):
                return [value]
            else:
                msg = 'invalid value {} type {}'
                raise RuntimeError(msg.format(value, type(value)))
        else:
            return default

    def __getitem__(self, name):
        return self._entries[name].value

    def __setitem__(self, name, entry):
        if not isinstance(entry, CMakeCacheEntry):
            msg = 'improper type {} for value {}, expecting CMakeCacheEntry'
            raise TypeError(msg.format(type(entry), entry))
        self._entries[name] = entry

    def __delitem__(self, name):
        del self._entries[name]

    def __iter__(self):
        return iter(self._entries.values())


def run_cmake_build(directory, caller=None):
    '''Ensure the CMake build in a directory is up to date.'''
    if caller is None:
        caller = subprocess.check_call
    caller([shutil.which('cmake'), '--build', directory])


def get_runner_cls(runner):
    for cls in ZephyrBinaryRunner.get_runners():
        if cls.name() == runner:
            return cls
    raise ValueError('unknown runner "{}"'.format(runner))


def make_c_identifier(string):
    '''Make a C identifier from a string in the same way CMake does.
    '''
    # the behavior of CMake's string(MAKE_C_IDENTIFER ...)  is not
    # precisely documented. This behavior matches the test case
    # that introduced the function:
    #
    # https://gitlab.kitware.com/cmake/cmake/commit/0ab50aea4c4d7099b339fb38b4459d0debbdbd85
    ret = []

    alpha_under = re.compile('[A-Za-z_]')
    alpha_num_under = re.compile('[A-Za-z0-9_]')

    if not alpha_under.match(string):
        ret.append('_')
    for c in string:
        if alpha_num_under.match(c):
            ret.append(c)
        else:
            ret.append('_')

    return ''.join(ret)


def get_runner_args(cache, runner):
    args_var = 'ZEPHYR_RUNNER_ARGS_{}'.format(make_c_identifier(runner))
    return cache.get_list(args_var)


def command_handler(command, args, runner_args):
    # Ensure build artifacts are up to date.
    run_cmake_build(path.dirname(args.cmake_cache))

    cache = CMakeCache(args.cmake_cache)
    board = cache['CACHED_BOARD']

    if args.runner is not None:
        runner = args.runner
    elif command == 'flash':
        runner = cache.get('ZEPHYR_BOARD_FLASH_RUNNER')
    else:
        runner = cache.get('ZEPHYR_BOARD_DEBUG_RUNNER')

    if runner is None:
        msg = (
            '"{}" is not supported with board {}. '
            'Please check the documentation for alternate instructions.')
        raise UnsupportedCommandError(msg.format(command, board))

    common_args = cache.get_list('ZEPHYR_RUNNER_ARGS_COMMON')
    cached_runner_args = get_runner_args(cache, runner)

    runner_cls = get_runner_cls(runner)
    if command not in runner_cls.capabilities().commands:
        msg = (
            'Runner "{}" does not support "{}" with board {}. '
            'Please try another runner, or check the documentation for '
            'alternate instructions.')
        raise UnsupportedCommandError(msg.format(runner, command, board))

    # Construct the final command line arguments, create a
    # runner-specific parser to handle them, and run the command.
    # TODO: the presence of [command] here is not necessary; clean
    #       up the runner core so it goes away and then remove it.
    assert isinstance(runner_args, list), runner_args
    final_runner_args = (common_args + cached_runner_args +
                         runner_args + [command])

    parser = argparse.ArgumentParser(prog=runner)
    runner_cls.add_parser(parser)
    parsed_args = parser.parse_args(args=final_runner_args)
    parsed_args.verbose = args.verbose
    runner = runner_cls.create_from_args(parsed_args)
    runner.run(command)


def print_common_opt_info(cache, initial_indent, subsequent_indent):
    common = cache.get('ZEPHYR_RUNNER_ARGS_COMMON')
    if common is None:
        return

    print('{}Common option settings:'.format(initial_indent))
    for arg in common:
        print('{}{}'.format(subsequent_indent, arg))
    print()


def print_runner_opt_info(cache, runner, initial_indent, subsequent_indent):
    runner_args = get_runner_args(cache, runner)
    if not runner_args:
        return

    print('{}Runner-specific option settings:'.format(initial_indent))
    for arg in runner_args:
        print('{}{}'.format(subsequent_indent, arg))
    print()


def print_runner_info(cache, runner, indent):
    cls = get_runner_cls(runner)
    available = (cache and runner in cache.get_list('ZEPHYR_RUNNERS'))

    print('Runner {}'.format(runner))
    print()
    print('Available: {}'.format('yes' if available else 'no'))
    print()
    if available:
        print_common_opt_info(cache, '', indent)
        print_runner_opt_info(cache, runner, '', indent)

    # Construct and print the usage text
    dummy_parser = argparse.ArgumentParser(prog='')
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
        formatter.start_section('{} options'.format(runner))
        formatter.add_text(group.description)
        formatter.add_arguments(actions)
        formatter.end_section()
    print(formatter.format_help())


def wrap(text, indent):
    return textwrap.wrap(text, initial_indent=indent, subsequent_indent=indent)


def info_handler(args, unknown):
    if unknown:
        raise RuntimeError('unrecognized arguments:', quote_sh_list(unknown))

    # If the cache is a file, try to ensure build artifacts are up to
    # date. If that doesn't work, still try to print information on a
    # best-effort basis.
    if path.isfile(args.cmake_cache):
        run_cmake_build(path.dirname(args.cmake_cache), caller=subprocess.call)

    try:
        cache = CMakeCache(args.cmake_cache)
    except Exception:
        msg = 'Warning: cannot load cache {}, output will be limited'
        print(msg.format(args.cmake_cache), file=sys.stderr)
        cache = None

    if args.runner:
        # Just information on one runner was requested.
        print_runner_info(cache, args.runner, INDENT)
        return

    all_cls = ZephyrBinaryRunner.get_runners()
    all_runners = ', '.join(r.name() for r in all_cls)
    print('All Zephyr runners:')
    for line in wrap(all_runners, INDENT):
        print(line)

    if cache is None:
        return

    available = cache.get_list('ZEPHYR_RUNNERS')
    available_cls = {}
    for cls in all_cls:
        if cls.name() in available:
            available_cls[cls.name()] = cls
    flash = cache.get('ZEPHYR_BOARD_FLASH_RUNNER')
    debug = cache.get('ZEPHYR_BOARD_DEBUG_RUNNER')
    board = cache['CACHED_BOARD']  # must be present

    print()
    print('CMake Cache with runner configuration:')
    print('{}{}'.format(INDENT, args.cmake_cache))
    print()
    print('Default flash runner (BOARD_FLASH_RUNNER):')
    print('{}{}'.format(INDENT, flash))
    print()
    print('Default debug runner (BOARD_DEBUG_RUNNER):')
    print('{}{}'.format(INDENT, debug))
    print()
    print_common_opt_info(cache, '', INDENT)
    if available:
        msg = 'Available runners for {} (use "{}" for more on any runner):'
        for line in wrap(msg.format(board, PROGRAM + ' info RUNNER'), ''):
            print(line)
        for runner in available:
            print('{}{}:'.format(INDENT, runner))
            print('{}Capabilities:'.format(INDENT * 2))
            print('{}{}'.format(INDENT * 3,
                                available_cls[runner].capabilities()))
            print_runner_opt_info(cache, runner, INDENT * 2, INDENT * 3)
    else:
        msg = ('No runners available for {}. '
               'Consult the documentation for instructions on how to run '
               'binaries on this target.').format(board)
        for line in wrap(msg, ''):
            print(line)


def print_fatal(exception, text=None, wrap=True):
    fatal = 'Fatal error: '
    if text is not None:
        text = '{}{}'.format(fatal, text)
    else:
        text = '{}{}'.format(fatal, ' '.join(exception.args))
    if wrap:
        for line in textwrap.wrap(text, initial_indent='',
                                  subsequent_indent=' ' * len(fatal)):
            print(line, file=sys.stderr)
    else:
        print(text, file=sys.stderr)


def main():
    # Top-level parser and subparsers creator.
    top_desc = '''Run (flash or debug) Zephyr binaries on hardware.'''
    top_epilog = textwrap.dedent('''\
    This script is usually run from a Zephyr build directory, and
    takes a command, like so:

      zephyr_run.py flash [optional arguments to flash runner]
      zephyr_run.py debug [optional arguments to debug runner]
      zephyr_run.py debugserver [optional arguments to debug runner]

    For more information on the runners themselves, use:

      zephyr_run.py info

    If running this script from outside a build directory, you must
    give a --cmake-cache.''')
    top_parser = argparse.ArgumentParser(
        formatter_class=argparse.RawDescriptionHelpFormatter,
        description=top_desc, epilog=top_epilog)
    top_parser.add_argument('-v', '--verbose', action='store_true',
                            help='if given, enable verbose output')
    top_parser.add_argument('-f', '--cmake-cache', default=DEFAULT_CACHE,
                            help='''path to CMake cache file containing runner
                            configuration (this is generated by the Zephyr
                            build system when compiling binaries);
                            default: {}'''.format(DEFAULT_CACHE))
    top_parser.add_argument('-r', '--runner',
                            help='''if given, overrides the default runner
                            set in CMAKE_CACHE''')
    parsers = top_parser.add_subparsers(title='commands', dest='command')

    # Sub-parsers for each command.
    for cmd in COMMANDS:
        parser = parsers.add_parser(
            cmd,
            description='''Any additional arguments (after "{}") are
            passed directly to the runner. Use "{} info" for information
            on available and default runners, and "{} info RUNNER" for
            information and usage on a particular runner.'''.format(
                cmd, PROGRAM, PROGRAM),
            help=COMMAND_HELP[cmd])
        parser.set_defaults(handler=partial(command_handler, cmd))

    # Sub-parser for the 'info' command.
    parser = parsers.add_parser('info',
                                help='''get runner information''',
                                description='''Print information about all
                                runners, and configuration information for
                                available runners.''')
    parser.add_argument('runner', nargs='?',
                        help='''if given, only print information about
                        this runner''')
    parser.set_defaults(handler=info_handler)

    # Parse arguments. Known arguments are handled by this script. Any
    # unknown arguments are collected into a list and passed on to the
    # runner.
    args, unknown = top_parser.parse_known_args()

    if 'handler' not in args:
        print('Fatal error: you must specify a command', file=sys.stderr)
        print()
        top_parser.print_help(file=sys.stderr)
        sys.exit(1)

    # Run command handler.
    for_stack_trace = ('Re-run as "{} --verbose ... {} ..." '
                       'for a stack trace.').format(PROGRAM, args.command)
    try:
        args.handler(args, unknown)
    except KeyboardInterrupt:
        sys.exit(0)
    except subprocess.CalledProcessError as s:
        msg = 'command exited with status {}: {}'
        text = msg.format(s.args[0], quote_sh_list(s.args[1]))
        print_fatal(s, text=text, wrap=False)
        if args.verbose:
            raise
        else:
            print(for_stack_trace, file=sys.stderr)
    except UnsupportedCommandError as u:
        print_fatal(u)
        sys.exit(1)
    except Exception as e:
        if e.args:
            print_fatal(e)
        else:
            print('Fatal error occurred.', file=sys.stderr)
        top_parser.print_usage(file=sys.stderr)
        print(file=sys.stderr)
        if args.verbose:
            raise
        else:
            print(for_stack_trace, file=sys.stderr)


if __name__ == '__main__':
    main()
