#! /usr/bin/env python3

# Copyright (c) 2017 Linaro Limited.
# Copyright (c) 2017 Open Source Foundries Limited.
#
# SPDX-License-Identifier: Apache-2.0

"""Zephyr binary runner core interfaces

This provides the core ZephyrBinaryRunner class meant for public use,
as well as some other helpers for concrete runner classes.
"""

import abc
import argparse
import errno
import logging
import os
import platform
import re
import selectors
import shlex
import shutil
import signal
import socket
import subprocess
import sys
from dataclasses import dataclass, field
from functools import partial
from enum import Enum
from inspect import isabstract
from typing import Dict, List, NamedTuple, NoReturn, Optional, Set, Type, \
    Union

try:
    from elftools.elf.elffile import ELFFile
    ELFTOOLS_MISSING = False
except ImportError:
    ELFTOOLS_MISSING = True


# Turn on to enable just logging the commands that would be run (at
# info rather than debug level), without actually running them. This
# can break runners that are expecting output or if one command
# depends on another, so it's just for debugging.
_DRY_RUN = False

_logger = logging.getLogger('runners')

# FIXME: I assume this code belongs somewhere else, but i couldn't figure out
# a good location for it, so i put it here for now
# We could potentially search for RTT blocks in hex or bin files as well,
# but since the magic string is "SEGGER RTT", i thought it might be better
# to avoid, at the risk of false positives.
def find_rtt_block(elf_file: str) -> Optional[int]:
    if ELFTOOLS_MISSING:
        raise RuntimeError('the Python dependency elftools was missing; '
                           'see the getting started guide for details on '
                           'how to fix')

    with open(elf_file, 'rb') as f:
        elffile = ELFFile(f)
        for sect in elffile.iter_sections('SHT_SYMTAB'):
            symbols = sect.get_symbol_by_name('_SEGGER_RTT')
            if symbols is None:
                continue
            for s in symbols:
                return s.entry.get('st_value')
    return None


class _DebugDummyPopen:

    def terminate(self):
        pass

    def wait(self):
        pass


MAX_PORT = 49151


class NetworkPortHelper:
    '''Helper class for dealing with local IP network ports.'''

    def get_unused_ports(self, starting_from):
        '''Find unused network ports, starting at given values.

        starting_from is an iterable of ports the caller would like to use.

        The return value is an iterable of ports, in the same order, using
        the given values if they were unused, or the next sequentially
        available unused port otherwise.

        Ports may be bound between this call's check and actual usage, so
        callers still need to handle errors involving returned ports.'''
        start = list(starting_from)
        used = self._used_now()
        ret = []

        for desired in start:
            port = desired
            while port in used:
                port += 1
                if port > MAX_PORT:
                    msg = "ports above {} are in use"
                    raise ValueError(msg.format(desired))
            used.add(port)
            ret.append(port)

        return ret

    def _used_now(self):
        handlers = {
            'Windows': self._used_now_windows,
            'Linux': self._used_now_linux,
            'Darwin': self._used_now_darwin,
        }
        handler = handlers[platform.system()]
        return handler()

    def _used_now_windows(self):
        cmd = ['netstat', '-a', '-n', '-p', 'tcp']
        return self._parser_windows(cmd)

    def _used_now_linux(self):
        cmd = ['ss', '-a', '-n', '-t']
        return self._parser_linux(cmd)

    def _used_now_darwin(self):
        cmd = ['netstat', '-a', '-n', '-p', 'tcp']
        return self._parser_darwin(cmd)

    @staticmethod
    def _parser_windows(cmd):
        out = subprocess.check_output(cmd).split(b'\r\n')
        used_bytes = [x.split()[1].rsplit(b':', 1)[1] for x in out
                      if x.startswith(b'  TCP')]
        return {int(b) for b in used_bytes}

    @staticmethod
    def _parser_linux(cmd):
        out = subprocess.check_output(cmd).splitlines()[1:]
        used_bytes = [s.split()[3].rsplit(b':', 1)[1] for s in out]
        return {int(b) for b in used_bytes}

    @staticmethod
    def _parser_darwin(cmd):
        out = subprocess.check_output(cmd).split(b'\n')
        used_bytes = [x.split()[3].rsplit(b':', 1)[1] for x in out
                      if x.startswith(b'tcp')]
        return {int(b) for b in used_bytes}


class BuildConfiguration:
    '''This helper class provides access to build-time configuration.

    Configuration options can be read as if the object were a dict,
    either object['CONFIG_FOO'] or object.get('CONFIG_FOO').

    Kconfig configuration values are available (parsed from .config).'''

    config_prefix = 'CONFIG'

    def __init__(self, build_dir: str):
        self.build_dir = build_dir
        self.options: Dict[str, Union[str, int]] = {}
        self.path = os.path.join(self.build_dir, 'zephyr', '.config')
        self._parse()

    def __contains__(self, item):
        return item in self.options

    def __getitem__(self, item):
        return self.options[item]

    def get(self, option, *args):
        return self.options.get(option, *args)

    def getboolean(self, option):
        '''If a boolean option is explicitly set to y or n,
        returns its value. Otherwise, falls back to False.
        '''
        return self.options.get(option, False)

    def _parse(self):
        filename = self.path

        opt_value = re.compile(f'^(?P<option>{self.config_prefix}_[A-Za-z0-9_]+)=(?P<value>.*)$')
        not_set = re.compile(f'^# (?P<option>{self.config_prefix}_[A-Za-z0-9_]+) is not set$')

        with open(filename, 'r') as f:
            for line in f:
                match = opt_value.match(line)
                if match:
                    value = match.group('value').rstrip()
                    if value.startswith('"') and value.endswith('"'):
                        # A string literal should have the quotes stripped,
                        # but otherwise be left as is.
                        value = value[1:-1]
                    elif value == 'y':
                        # The character 'y' is a boolean option
                        # that is set to True.
                        value = True
                    else:
                        # Neither a string nor 'y', so try to parse it
                        # as an integer.
                        try:
                            base = 16 if value.startswith('0x') else 10
                            self.options[match.group('option')] = int(value, base=base)
                            continue
                        except ValueError:
                            pass

                    self.options[match.group('option')] = value
                    continue

                match = not_set.match(line)
                if match:
                    # '# CONFIG_FOO is not set' means a boolean option is false.
                    self.options[match.group('option')] = False

class SysbuildConfiguration(BuildConfiguration):
    '''This helper class provides access to sysbuild-time configuration.

    Configuration options can be read as if the object were a dict,
    either object['SB_CONFIG_FOO'] or object.get('SB_CONFIG_FOO').

    Kconfig configuration values are available (parsed from .config).'''

    config_prefix = 'SB_CONFIG'

    def _parse(self):
        # If the build does not use sysbuild, skip parsing the file.
        if not os.path.exists(self.path):
            return
        super()._parse()

class MissingProgram(FileNotFoundError):
    '''FileNotFoundError subclass for missing program dependencies.

    No significant changes from the parent FileNotFoundError; this is
    useful for explicitly signaling that the file in question is a
    program that some class requires to proceed.

    The filename attribute contains the missing program.'''

    def __init__(self, program):
        super().__init__(errno.ENOENT, os.strerror(errno.ENOENT), program)


_RUNNERCAPS_COMMANDS = {'flash', 'debug', 'debugserver', 'attach', 'simulate', 'robot', 'rtt'}

@dataclass
class RunnerCaps:
    '''This class represents a runner class's capabilities.

    Each capability is represented as an attribute with the same
    name. Flag attributes are True or False.

    Available capabilities:

    - commands: set of supported commands; default is {'flash',
      'debug', 'debugserver', 'attach', 'simulate', 'robot', 'rtt'}.

    - dev_id: whether the runner supports device identifiers, in the form of an
      -i, --dev-id option. This is useful when the user has multiple debuggers
      connected to a single computer, in order to select which one will be used
      with the command provided.

    - flash_addr: whether the runner supports flashing to an
      arbitrary address. Default is False. If true, the runner
      must honor the --dt-flash option.

    - erase: whether the runner supports an --erase option, which
      does a mass-erase of the entire addressable flash on the target
      before flashing. On multi-core SoCs, this may only erase portions of
      flash specific the actual target core. (This option can be useful for
      things like clearing out old settings values or other subsystem state
      that may affect the behavior of the zephyr image. It is also sometimes
      needed by SoCs which have flash-like areas that can't be sector
      erased by the underlying tool before flashing; UICR on nRF SoCs
      is one example.)

    - reset: whether the runner supports a --reset option, which
      resets the device after a flash operation is complete.

    - extload: whether the runner supports a --extload option, which
      must be given one time and is passed on to the underlying tool
      that the runner wraps.

    - tool_opt: whether the runner supports a --tool-opt (-O) option, which
      can be given multiple times and is passed on to the underlying tool
      that the runner wraps.

    - file: whether the runner supports a --file option, which specifies
      exactly the file that should be used to flash, overriding any default
      discovered in the build directory.

    - hide_load_files: whether the elf/hex/bin file arguments should be hidden.

    - rtt: whether the runner supports SEGGER RTT. This adds a --rtt-address
      option.
    '''

    commands: Set[str] = field(default_factory=lambda: set(_RUNNERCAPS_COMMANDS))
    dev_id: bool = False
    flash_addr: bool = False
    erase: bool = False
    reset: bool = False
    extload: bool = False
    tool_opt: bool = False
    file: bool = False
    hide_load_files: bool = False
    rtt: bool = False  # This capability exists separately from the rtt command
                       # to allow other commands to use the rtt address

    def __post_init__(self):
        if not self.commands.issubset(_RUNNERCAPS_COMMANDS):
            raise ValueError(f'{self.commands=} contains invalid command')


def _missing_cap(cls: Type['ZephyrBinaryRunner'], option: str) -> NoReturn:
    # Helper function that's called when an option was given on the
    # command line that corresponds to a missing capability in the
    # runner class cls.

    raise ValueError(f"{cls.name()} doesn't support {option} option")


class FileType(Enum):
    OTHER = 0
    HEX = 1
    BIN = 2
    ELF = 3


class RunnerConfig(NamedTuple):
    '''Runner execution-time configuration.

    This is a common object shared by all runners. Individual runners
    can register specific configuration options using their
    do_add_parser() hooks.
    '''
    build_dir: str                  # application build directory
    board_dir: str                  # board definition directory
    elf_file: Optional[str]         # zephyr.elf path, or None
    exe_file: Optional[str]         # zephyr.exe path, or None
    hex_file: Optional[str]         # zephyr.hex path, or None
    bin_file: Optional[str]         # zephyr.bin path, or None
    uf2_file: Optional[str]         # zephyr.uf2 path, or None
    file: Optional[str]             # binary file path (provided by the user), or None
    file_type: Optional[FileType] = FileType.OTHER  # binary file type
    gdb: Optional[str] = None       # path to a usable gdb
    openocd: Optional[str] = None   # path to a usable openocd
    openocd_search: List[str] = []  # add these paths to the openocd search path
    rtt_address: Optional[int] = None # address of the rtt control block


_YN_CHOICES = ['Y', 'y', 'N', 'n', 'yes', 'no', 'YES', 'NO']


class _DTFlashAction(argparse.Action):

    def __call__(self, parser, namespace, values, option_string=None):
        if values.lower().startswith('y'):
            namespace.dt_flash = True
        else:
            namespace.dt_flash = False


class _ToggleAction(argparse.Action):

    def __call__(self, parser, args, ignored, option):
        setattr(args, self.dest, not option.startswith('--no-'))

class DeprecatedAction(argparse.Action):

    def __call__(self, parser, namespace, values, option_string=None):
        _logger.warning(f'Argument {self.option_strings[0]} is deprecated' +
                        (f' for your runner {self._cls.name()}'  if self._cls is not None else '') +
                        f', use {self._replacement} instead.')
        setattr(namespace, self.dest, values)

def depr_action(*args, cls=None, replacement=None, **kwargs):
    action = DeprecatedAction(*args, **kwargs)
    setattr(action, '_cls', cls)
    setattr(action, '_replacement', replacement)
    return action

class ZephyrBinaryRunner(abc.ABC):
    '''Abstract superclass for binary runners (flashers, debuggers).

    **Note**: this class's API has changed relatively rarely since it
    as added, but it is not considered a stable Zephyr API, and may change
    without notice.

    With some exceptions, boards supported by Zephyr must provide
    generic means to be flashed (have a Zephyr firmware binary
    permanently installed on the device for running) and debugged
    (have a breakpoint debugger and program loader on a host
    workstation attached to a running target).

    This is supported by four top-level commands managed by the
    Zephyr build system:

    - 'flash': flash a previously configured binary to the board,
      start execution on the target, then return.

    - 'debug': connect to the board via a debugging protocol, program
      the flash, then drop the user into a debugger interface with
      symbol tables loaded from the current binary, and block until it
      exits.

    - 'debugserver': connect via a board-specific debugging protocol,
      then reset and halt the target. Ensure the user is now able to
      connect to a debug server with symbol tables loaded from the
      binary.

    - 'attach': connect to the board via a debugging protocol, then drop
      the user into a debugger interface with symbol tables loaded from
      the current binary, and block until it exits. Unlike 'debug', this
      command does not program the flash.

    This class provides an API for these commands. Every subclass is
    called a 'runner' for short. Each runner has a name (like
    'pyocd'), and declares commands it can handle (like
    'flash'). Boards (like 'nrf52dk/nrf52832') declare which runner(s)
    are compatible with them to the Zephyr build system, along with
    information on how to configure the runner to work with the board.

    The build system will then place enough information in the build
    directory to create and use runners with this class's create()
    method, which provides a command line argument parsing API. You
    can also create runners by instantiating subclasses directly.

    In order to define your own runner, you need to:

    1. Define a ZephyrBinaryRunner subclass, and implement its
       abstract methods. You may need to override capabilities().

    2. Make sure the Python module defining your runner class is
       imported, e.g. by editing this package's __init__.py (otherwise,
       get_runners() won't work).

    3. Give your runner's name to the Zephyr build system in your
       board's board.cmake.

    Additional advice:

    - If you need to import any non-standard-library modules, make sure
      to catch ImportError and defer complaints about it to a RuntimeError
      if one is missing. This avoids affecting users that don't require your
      runner, while still making it clear what went wrong to users that do
      require it that don't have the necessary modules installed.

    - If you need to ask the user something (e.g. using input()), do it
      in your create() classmethod, not do_run(). That ensures your
      __init__() really has everything it needs to call do_run(), and also
      avoids calling input() when not instantiating within a command line
      application.

    - Use self.logger to log messages using the standard library's
      logging API; your logger is named "runner.<your-runner-name()>"

    For command-line invocation from the Zephyr build system, runners
    define their own argparse-based interface through the common
    add_parser() (and runner-specific do_add_parser() it delegates
    to), and provide a way to create instances of themselves from
    a RunnerConfig and parsed runner-specific arguments via create().

    Runners use a variety of host tools and configuration values, the
    user interface to which is abstracted by this class. Each runner
    subclass should take any values it needs to execute one of these
    commands in its constructor.  The actual command execution is
    handled in the run() method.'''

    def __init__(self, cfg: RunnerConfig):
        '''Initialize core runner state.'''

        self.cfg = cfg
        '''RunnerConfig for this instance.'''

        self.logger = logging.getLogger('runners.{}'.format(self.name()))
        '''logging.Logger for this instance.'''

    @staticmethod
    def get_runners() -> List[Type['ZephyrBinaryRunner']]:
        '''Get a list of all currently defined runner classes.'''
        def inheritors(klass):
            subclasses = set()
            work = [klass]
            while work:
                parent = work.pop()
                for child in parent.__subclasses__():
                    if child not in subclasses:
                        if not isabstract(child):
                            subclasses.add(child)
                        work.append(child)
            return subclasses

        return inheritors(ZephyrBinaryRunner)

    @classmethod
    @abc.abstractmethod
    def name(cls) -> str:
        '''Return this runner's user-visible name.

        When choosing a name, pick something short and lowercase,
        based on the name of the tool (like openocd, jlink, etc.) or
        the target architecture/board (like xtensa etc.).'''

    @classmethod
    def capabilities(cls) -> RunnerCaps:
        '''Returns a RunnerCaps representing this runner's capabilities.

        This implementation returns the default capabilities.

        Subclasses should override appropriately if needed.'''
        return RunnerCaps()

    @classmethod
    def add_parser(cls, parser):
        '''Adds a sub-command parser for this runner.

        The given object, parser, is a sub-command parser from the
        argparse module. For more details, refer to the documentation
        for argparse.ArgumentParser.add_subparsers().

        The lone common optional argument is:

        * --dt-flash (if the runner capabilities includes flash_addr)

        Runner-specific options are added through the do_add_parser()
        hook.'''
        # Unfortunately, the parser argument's type is not documented
        # in typeshed, so we can't type annotate much here.

        # Common options that depend on runner capabilities. If a
        # capability is not supported, the option string or strings
        # are added anyway, to prevent an individual runner class from
        # using them to mean something else.
        caps = cls.capabilities()

        if caps.dev_id:
            parser.add_argument('-i', '--dev-id',
                                dest='dev_id',
                                help=cls.dev_id_help())
        else:
            parser.add_argument('-i', '--dev-id', help=argparse.SUPPRESS)

        if caps.flash_addr:
            parser.add_argument('--dt-flash', default='n', choices=_YN_CHOICES,
                                action=_DTFlashAction,
                                help='''If 'yes', try to use flash address
                                information from devicetree when flash
                                addresses are unknown (e.g. when flashing a .bin)''')
        else:
            parser.add_argument('--dt-flash', help=argparse.SUPPRESS)

        if caps.file:
            parser.add_argument('-f', '--file',
                                dest='file',
                                help="path to binary file")
            parser.add_argument('-t', '--file-type',
                                dest='file_type',
                                help="type of binary file")
        else:
            parser.add_argument('-f', '--file', help=argparse.SUPPRESS)
            parser.add_argument('-t', '--file-type', help=argparse.SUPPRESS)

        if caps.hide_load_files:
            parser.add_argument('--elf-file', help=argparse.SUPPRESS)
            parser.add_argument('--hex-file', help=argparse.SUPPRESS)
            parser.add_argument('--bin-file', help=argparse.SUPPRESS)
        else:
            parser.add_argument('--elf-file',
                                metavar='FILE',
                                action=(partial(depr_action, cls=cls, replacement='-f/--file') if caps.file else None),
                                help='path to zephyr.elf' if not caps.file else 'Deprecated, use -f/--file instead.')
            parser.add_argument('--hex-file',
                                metavar='FILE',
                                action=(partial(depr_action, cls=cls, replacement='-f/--file') if caps.file else None),
                                help='path to zephyr.hex' if not caps.file else 'Deprecated, use -f/--file instead.')
            parser.add_argument('--bin-file',
                                metavar='FILE',
                                action=(partial(depr_action, cls=cls, replacement='-f/--file') if caps.file else None),
                                help='path to zephyr.bin' if not caps.file else 'Deprecated, use -f/--file instead.')

        parser.add_argument('--erase', '--no-erase', nargs=0,
                            action=_ToggleAction,
                            help=("mass erase flash before loading, or don't. "
                                  "Default action depends on each specific runner."
                                  if caps.erase else argparse.SUPPRESS))

        parser.add_argument('--reset', '--no-reset', nargs=0,
                            action=_ToggleAction,
                            help=("reset device after flashing, or don't. "
                                  "Default action depends on each specific runner."
                                  if caps.reset else argparse.SUPPRESS))

        parser.add_argument('--extload', dest='extload',
                            help=(cls.extload_help() if caps.extload
                                  else argparse.SUPPRESS))

        parser.add_argument('-O', '--tool-opt', dest='tool_opt',
                            default=[], action='append',
                            help=(cls.tool_opt_help() if caps.tool_opt
                                  else argparse.SUPPRESS))

        if caps.rtt:
            parser.add_argument('--rtt-address', dest='rtt_address',
                                type=lambda x: int(x, 0),
                                help="address of RTT control block. If not supplied, it will be autodetected if possible")
        else:
            parser.add_argument('--rtt-address', help=argparse.SUPPRESS)

        # Runner-specific options.
        cls.do_add_parser(parser)

    @classmethod
    @abc.abstractmethod
    def do_add_parser(cls, parser):
        '''Hook for adding runner-specific options.'''

    @classmethod
    def create(cls, cfg: RunnerConfig,
               args: argparse.Namespace) -> 'ZephyrBinaryRunner':
        '''Create an instance from command-line arguments.

        - ``cfg``: runner configuration (pass to superclass __init__)
        - ``args``: arguments parsed from execution environment, as
          specified by ``add_parser()``.'''
        caps = cls.capabilities()
        if args.dev_id and not caps.dev_id:
            _missing_cap(cls, '--dev-id')
        if args.dt_flash and not caps.flash_addr:
            _missing_cap(cls, '--dt-flash')
        if args.erase and not caps.erase:
            _missing_cap(cls, '--erase')
        if args.reset and not caps.reset:
            _missing_cap(cls, '--reset')
        if args.extload and not caps.extload:
            _missing_cap(cls, '--extload')
        if args.tool_opt and not caps.tool_opt:
            _missing_cap(cls, '--tool-opt')
        if args.file and not caps.file:
            _missing_cap(cls, '--file')
        if args.file_type and not args.file:
            raise ValueError("--file-type requires --file")
        if args.file_type and not caps.file:
            _missing_cap(cls, '--file-type')
        if args.rtt_address and not caps.rtt:
            _missing_cap(cls, '--rtt-address')

        ret = cls.do_create(cfg, args)
        if args.erase:
            ret.logger.info('mass erase requested')
        if args.reset:
            ret.logger.info('reset after flashing requested')
        return ret

    @classmethod
    @abc.abstractmethod
    def do_create(cls, cfg: RunnerConfig,
                  args: argparse.Namespace) -> 'ZephyrBinaryRunner':
        '''Hook for instance creation from command line arguments.'''

    @staticmethod
    def get_flash_address(args: argparse.Namespace,
                          build_conf: BuildConfiguration,
                          default: int = 0x0) -> int:
        '''Helper method for extracting a flash address.

        If args.dt_flash is true, returns the address obtained from
        ZephyrBinaryRunner.flash_address_from_build_conf(build_conf).

        Otherwise (when args.dt_flash is False), the default value is
        returned.'''
        if args.dt_flash:
            return ZephyrBinaryRunner.flash_address_from_build_conf(build_conf)
        else:
            return default

    @staticmethod
    def flash_address_from_build_conf(build_conf: BuildConfiguration):
        '''If CONFIG_HAS_FLASH_LOAD_OFFSET is n in build_conf,
        return the CONFIG_FLASH_BASE_ADDRESS value. Otherwise, return
        CONFIG_FLASH_BASE_ADDRESS + CONFIG_FLASH_LOAD_OFFSET.
        '''
        if build_conf.getboolean('CONFIG_HAS_FLASH_LOAD_OFFSET'):
            return (build_conf['CONFIG_FLASH_BASE_ADDRESS'] +
                    build_conf['CONFIG_FLASH_LOAD_OFFSET'])
        else:
            return build_conf['CONFIG_FLASH_BASE_ADDRESS']

    def run(self, command: str, **kwargs):
        '''Runs command ('flash', 'debug', 'debugserver', 'attach').

        This is the main entry point to this runner.'''
        caps = self.capabilities()
        if command not in caps.commands:
            raise ValueError('runner {} does not implement command {}'.format(
                self.name(), command))
        self.do_run(command, **kwargs)

    @abc.abstractmethod
    def do_run(self, command: str, **kwargs):
        '''Concrete runner; run() delegates to this. Implement in subclasses.

        In case of an unsupported command, raise a ValueError.'''

    @property
    def build_conf(self) -> BuildConfiguration:
        '''Get a BuildConfiguration for the build directory.'''
        if not hasattr(self, '_build_conf'):
            self._build_conf = BuildConfiguration(self.cfg.build_dir)
        return self._build_conf

    @property
    def sysbuild_conf(self) -> SysbuildConfiguration:
        '''Get a SysbuildConfiguration for the sysbuild directory.'''
        if not hasattr(self, '_sysbuild_conf'):
            self._sysbuild_conf = SysbuildConfiguration(os.path.dirname(self.cfg.build_dir))
        return self._sysbuild_conf

    @property
    def thread_info_enabled(self) -> bool:
        '''Returns True if self.build_conf has
        CONFIG_DEBUG_THREAD_INFO enabled.
        '''
        return self.build_conf.getboolean('CONFIG_DEBUG_THREAD_INFO')

    @classmethod
    def dev_id_help(cls) -> str:
        ''' Get the ArgParse help text for the --dev-id option.'''
        return '''Device identifier. Use it to select
                  which debugger, device, node or instance to
                  target when multiple ones are available or
                  connected.'''

    @classmethod
    def extload_help(cls) -> str:
        ''' Get the ArgParse help text for the --extload option.'''
        return '''External loader to be used by stm32cubeprogrammer
                  to program the targeted external memory.
                  The runner requires the external loader (*.stldr) filename.
                  This external loader (*.stldr) must be located within
                  STM32CubeProgrammer/bin/ExternalLoader directory.'''

    @classmethod
    def tool_opt_help(cls) -> str:
        ''' Get the ArgParse help text for the --tool-opt option.'''
        return '''Option to pass on to the underlying tool used
                  by this runner. This can be given multiple times;
                  the resulting arguments will be given to the tool
                  in the order they appear on the command line.'''

    @staticmethod
    def require(program: str, path: Optional[str] = None) -> str:
        '''Require that a program is installed before proceeding.

        :param program: name of the program that is required,
                        or path to a program binary.
        :param path:    PATH where to search for the program binary.
                        By default check on the system PATH.

        If ``program`` is an absolute path to an existing program
        binary, this call succeeds. Otherwise, try to find the program
        by name on the system PATH or in the given PATH, if provided.

        If the program can be found, its path is returned.
        Otherwise, raises MissingProgram.'''
        ret = shutil.which(program, path=path)
        if ret is None:
            raise MissingProgram(program)
        return ret

    def get_rtt_address(self) -> int | None:
        '''Helper method for extracting a the RTT control block address.

        If args.rtt_address was supplied, returns that.

        Otherwise, attempt to locate an rtt block in the elf file.
        If this is not found, None is returned'''
        if self.cfg.rtt_address is not None:
            return self.cfg.rtt_address
        elif self.cfg.elf_file is not None:
            return find_rtt_block(self.cfg.elf_file)
        return None

    def run_server_and_client(self, server, client, **kwargs):
        '''Run a server that ignores SIGINT, and a client that handles it.

        This routine portably:

        - creates a Popen object for the ``server`` command which ignores
          SIGINT
        - runs ``client`` in a subprocess while temporarily ignoring SIGINT
        - cleans up the server after the client exits.
        - the keyword arguments, if any, will be passed down to both server and
          client subprocess calls

        It's useful to e.g. open a GDB server and client.'''
        server_proc = self.popen_ignore_int(server, **kwargs)
        try:
            self.run_client(client, **kwargs)
        finally:
            server_proc.terminate()
            server_proc.wait()

    def run_client(self, client, **kwargs):
        '''Run a client that handles SIGINT.'''
        previous = signal.signal(signal.SIGINT, signal.SIG_IGN)
        try:
            self.check_call(client, **kwargs)
        finally:
            signal.signal(signal.SIGINT, previous)

    def _log_cmd(self, cmd: List[str]):
        escaped = ' '.join(shlex.quote(s) for s in cmd)
        if not _DRY_RUN:
            self.logger.debug(escaped)
        else:
            self.logger.info(escaped)

    def call(self, cmd: List[str], **kwargs) -> int:
        '''Subclass subprocess.call() wrapper.

        Subclasses should use this method to run command in a
        subprocess and get its return code, rather than
        using subprocess directly, to keep accurate debug logs.
        '''
        self._log_cmd(cmd)
        if _DRY_RUN:
            return 0
        return subprocess.call(cmd, **kwargs)

    def check_call(self, cmd: List[str], **kwargs):
        '''Subclass subprocess.check_call() wrapper.

        Subclasses should use this method to run command in a
        subprocess and check that it executed correctly, rather than
        using subprocess directly, to keep accurate debug logs.
        '''
        self._log_cmd(cmd)
        if _DRY_RUN:
            return
        subprocess.check_call(cmd, **kwargs)

    def check_output(self, cmd: List[str], **kwargs) -> bytes:
        '''Subclass subprocess.check_output() wrapper.

        Subclasses should use this method to run command in a
        subprocess and check that it executed correctly, rather than
        using subprocess directly, to keep accurate debug logs.
        '''
        self._log_cmd(cmd)
        if _DRY_RUN:
            return b''
        return subprocess.check_output(cmd, **kwargs)

    def popen_ignore_int(self, cmd: List[str], **kwargs) -> subprocess.Popen:
        '''Spawn a child command, ensuring it ignores SIGINT.

        The returned subprocess.Popen object must be manually terminated.'''
        cflags = 0
        preexec = None
        system = platform.system()

        if system == 'Windows':
            # We can't type check this line on Unix operating systems:
            # mypy thinks the subprocess module has no such attribute.
            cflags |= subprocess.CREATE_NEW_PROCESS_GROUP  # type: ignore
        elif system in {'Linux', 'Darwin'}:
            # We can't type check this on Windows for the same reason.
            preexec = os.setsid # type: ignore

        self._log_cmd(cmd)
        if _DRY_RUN:
            return _DebugDummyPopen()  # type: ignore

        return subprocess.Popen(cmd, creationflags=cflags, preexec_fn=preexec, **kwargs)

    def ensure_output(self, output_type: str) -> None:
        '''Ensure self.cfg has a particular output artifact.

        For example, ensure_output('bin') ensures that self.cfg.bin_file
        refers to an existing file. Errors out if it's missing or undefined.

        :param output_type: string naming the output type
        '''
        output_file = getattr(self.cfg, f'{output_type}_file', None)

        if output_file is None:
            err = f'{output_type} file location is unknown.'
        elif not os.path.isfile(output_file):
            err = f'{output_file} does not exist.'
        else:
            return

        if output_type in ('elf', 'hex', 'bin', 'uf2'):
            err += f' Try enabling CONFIG_BUILD_OUTPUT_{output_type.upper()}.'

        # RuntimeError avoids a stack trace saved in run_common.
        raise RuntimeError(err)

    def run_telnet_client(self, host: str, port: int) -> None:
        '''
        Run a telnet client for user interaction.
        '''
        # If a `nc` command is available, run it, as it will provide the best support for
        # CONFIG_SHELL_VT100_COMMANDS etc.
        if shutil.which('nc') is not None:
            client_cmd = ['nc', host, str(port)]
            self.run_client(client_cmd)
            return

        # Otherwise, use a pure python implementation. This will work well for logging,
        # but input is line based only.
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((host, port))
        sel = selectors.DefaultSelector()
        sel.register(sys.stdin, selectors.EVENT_READ)
        sel.register(sock, selectors.EVENT_READ)
        while True:
            events = sel.select()
            for key, _ in events:
                if key.fileobj == sys.stdin:
                    text = sys.stdin.readline()
                    if text:
                        sock.send(text.encode())

                elif key.fileobj == sock:
                    resp = sock.recv(2048)
                    if resp:
                        print(resp.decode())
