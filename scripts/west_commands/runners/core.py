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
import shlex
import shutil
import signal
import subprocess
import re
from typing import Dict, List, NamedTuple, NoReturn, Optional, Set, Type, \
    Union

# Turn on to enable just logging the commands that would be run (at
# info rather than debug level), without actually running them. This
# can break runners that are expecting output or if one command
# depends on another, so it's just for debugging.
_DRY_RUN = False

_logger = logging.getLogger('runners')


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
        opt_value = re.compile('^(?P<option>CONFIG_[A-Za-z0-9_]+)=(?P<value>.*)$')
        not_set = re.compile('^# (?P<option>CONFIG_[A-Za-z0-9_]+) is not set$')

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

class MissingProgram(FileNotFoundError):
    '''FileNotFoundError subclass for missing program dependencies.

    No significant changes from the parent FileNotFoundError; this is
    useful for explicitly signaling that the file in question is a
    program that some class requires to proceed.

    The filename attribute contains the missing program.'''

    def __init__(self, program):
        super().__init__(errno.ENOENT, os.strerror(errno.ENOENT), program)


class RunnerCaps:
    '''This class represents a runner class's capabilities.

    Each capability is represented as an attribute with the same
    name. Flag attributes are True or False.

    Available capabilities:

    - commands: set of supported commands; default is {'flash',
      'debug', 'debugserver', 'attach'}.

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
    '''

    def __init__(self,
                 commands: Set[str] = {'flash', 'debug',
                                       'debugserver', 'attach'},
                 flash_addr: bool = False,
                 erase: bool = False):
        self.commands = commands
        self.flash_addr = bool(flash_addr)
        self.erase = bool(erase)

    def __str__(self):
        return (f'RunnerCaps(commands={self.commands}, '
                f'flash_addr={self.flash_addr}, '
                f'erase={self.erase}'
                ')')


def _missing_cap(cls: Type['ZephyrBinaryRunner'], option: str) -> NoReturn:
    # Helper function that's called when an option was given on the
    # command line that corresponds to a missing capability in the
    # runner class cls.

    raise ValueError(f"{cls.name()} doesn't support {option} option")


class RunnerConfig(NamedTuple):
    '''Runner execution-time configuration.

    This is a common object shared by all runners. Individual runners
    can register specific configuration options using their
    do_add_parser() hooks.
    '''
    build_dir: str              # application build directory
    board_dir: str              # board definition directory
    elf_file: Optional[str]     # zephyr.elf path, or None
    hex_file: Optional[str]     # zephyr.hex path, or None
    bin_file: Optional[str]     # zephyr.bin path, or None
    gdb: Optional[str] = None   # path to a usable gdb
    openocd: Optional[str] = None  # path to a usable openocd
    openocd_search: Optional[str] = None  # add this to openocd search path


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
    'flash'). Boards (like 'nrf52dk_nrf52832') declare which runner(s)
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
        return ZephyrBinaryRunner.__subclasses__()

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

        if caps.flash_addr:
            parser.add_argument('--dt-flash', default='n', choices=_YN_CHOICES,
                                action=_DTFlashAction,
                                help='''If 'yes', try to use flash address
                                information from devicetree when flash
                                addresses are unknown (e.g. when flashing a .bin)''')
        else:
            parser.add_argument('--dt-flash', help=argparse.SUPPRESS)

        parser.add_argument('--erase', '--no-erase', nargs=0,
                            action=_ToggleAction,
                            help=("mass erase flash before loading, or don't"
                                  if caps.erase else argparse.SUPPRESS))

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
        if args.dt_flash and not caps.flash_addr:
            _missing_cap(cls, '--dt-flash')
        if args.erase and not caps.erase:
            _missing_cap(cls, '--erase')

        ret = cls.do_create(cfg, args)
        if args.erase:
            ret.logger.info('mass erase requested')
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
    def thread_info_enabled(self) -> bool:
        '''Returns True if self.build_conf has
        CONFIG_DEBUG_THREAD_INFO enabled. This supports the
        CONFIG_OPENOCD_SUPPORT fallback as well for now.
        '''
        return (self.build_conf.getboolean('CONFIG_DEBUG_THREAD_INFO') or
                self.build_conf.getboolean('CONFIG_OPENOCD_SUPPORT'))

    @staticmethod
    def require(program: str) -> str:
        '''Require that a program is installed before proceeding.

        :param program: name of the program that is required,
                        or path to a program binary.

        If ``program`` is an absolute path to an existing program
        binary, this call succeeds. Otherwise, try to find the program
        by name on the system PATH.

        If the program can be found, its path is returned.
        Otherwise, raises MissingProgram.'''
        ret = shutil.which(program)
        if ret is None:
            raise MissingProgram(program)
        return ret

    def run_server_and_client(self, server, client):
        '''Run a server that ignores SIGINT, and a client that handles it.

        This routine portably:

        - creates a Popen object for the ``server`` command which ignores
          SIGINT
        - runs ``client`` in a subprocess while temporarily ignoring SIGINT
        - cleans up the server after the client exits.

        It's useful to e.g. open a GDB server and client.'''
        server_proc = self.popen_ignore_int(server)
        try:
            self.run_client(client)
        finally:
            server_proc.terminate()
            server_proc.wait()

    def run_client(self, client):
        '''Run a client that handles SIGINT.'''
        previous = signal.signal(signal.SIGINT, signal.SIG_IGN)
        try:
            self.check_call(client)
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

    def popen_ignore_int(self, cmd: List[str]) -> subprocess.Popen:
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

        return subprocess.Popen(cmd, creationflags=cflags, preexec_fn=preexec)

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

        if output_type in ('elf', 'hex', 'bin'):
            err += f' Try enabling CONFIG_BUILD_OUTPUT_{output_type.upper()}.'

        # RuntimeError avoids a stack trace saved in run_common.
        raise RuntimeError(err)
