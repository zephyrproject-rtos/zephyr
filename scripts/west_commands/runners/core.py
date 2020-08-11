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

    def __init__(self, build_dir):
        self.build_dir = build_dir
        self.options = {}
        self._init()

    def __contains__(self, item):
        return item in self.options

    def __getitem__(self, item):
        return self.options[item]

    def get(self, option, *args):
        return self.options.get(option, *args)

    def _init(self):
        self._parse(os.path.join(self.build_dir, 'zephyr', '.config'))

    def _parse(self, filename):
        with open(filename, 'r') as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith('#'):
                    continue
                option, value = line.split('=', 1)
                self.options[option] = self._parse_value(value)

    @staticmethod
    def _parse_value(value):
        if value.startswith('"') or value.startswith("'"):
            return value.split()
        try:
            return int(value, 0)
        except ValueError:
            return value


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
                 commands={'flash', 'debug', 'debugserver', 'attach'},
                 flash_addr=False, erase=False):
        self.commands = commands
        self.flash_addr = bool(flash_addr)
        self.erase = bool(erase)

    def __str__(self):
        return (f'RunnerCaps(commands={self.commands}, '
                f'flash_addr={self.flash_addr}, '
                f'erase={self.erase}'
                ')')


def _missing_cap(cls, option):
    # Helper function that's called when an option was given on the
    # command line that corresponds to a missing capability.
    #
    # 'cls' is a ZephyrBinaryRunner subclass; 'option' is an option
    # that can't be supported due to missing capability.

    raise ValueError(f"{cls.name()} doesn't support {option} option")


class RunnerConfig:
    '''Runner execution-time configuration.

    This is a common object shared by all runners. Individual runners
    can register specific configuration options using their
    do_add_parser() hooks.

    This class's __slots__ contains exactly the configuration variables.
    '''

    __slots__ = ['build_dir', 'board_dir', 'elf_file', 'hex_file',
                 'bin_file', 'gdb', 'openocd', 'openocd_search']

    # TODO: revisit whether we can get rid of some of these.  Having
    # tool-specific configuration options here is a layering
    # violation, but it's very convenient to have a single place to
    # store the locations of tools (like gdb and openocd) that are
    # needed by multiple ZephyrBinaryRunner subclasses.
    def __init__(self, build_dir, board_dir,
                 elf_file, hex_file, bin_file,
                 gdb=None, openocd=None, openocd_search=None):
        self.build_dir = build_dir
        '''Zephyr application build directory'''

        self.board_dir = board_dir
        '''Zephyr board directory'''

        self.elf_file = elf_file
        '''Path to the elf file that the runner should operate on'''

        self.hex_file = hex_file
        '''Path to the hex file that the runner should operate on'''

        self.bin_file = bin_file
        '''Path to the bin file that the runner should operate on'''

        self.gdb = gdb
        ''''Path to GDB compatible with the target, may be None.'''

        self.openocd = openocd
        '''Path to OpenOCD to use for this target, may be None.'''

        self.openocd_search = openocd_search
        '''directory to add to OpenOCD search path, may be None.'''


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
    directory so to create and use runners with this class's create()
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

    def __init__(self, cfg):
        '''Initialize core runner state.

        ``cfg`` is a RunnerConfig instance.'''
        self.cfg = cfg
        '''RunnerConfig for this instance.'''

        self.logger = logging.getLogger('runners.{}'.format(self.name()))
        '''logging.Logger for this instance.'''

    @staticmethod
    def get_runners():
        '''Get a list of all currently defined runner classes.'''
        return ZephyrBinaryRunner.__subclasses__()

    @classmethod
    @abc.abstractmethod
    def name(cls):
        '''Return this runner's user-visible name.

        When choosing a name, pick something short and lowercase,
        based on the name of the tool (like openocd, jlink, etc.) or
        the target architecture/board (like xtensa etc.).'''

    @classmethod
    def capabilities(cls):
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
        # Common options that depend on runner capabilities. If a
        # capability is not supported, the option string or strings
        # are added anyway, to prevent an individual runner class from
        # using them to mean something else.
        caps = cls.capabilities()

        if caps.flash_addr:
            parser.add_argument('--dt-flash', default='n', choices=_YN_CHOICES,
                                action=_DTFlashAction,
                                help='''If 'yes', use configuration generated
                                by device tree (DT) to compute flash
                                addresses.''')
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
    def create(cls, cfg, args):
        '''Create an instance from command-line arguments.

        - ``cfg``: RunnerConfig instance (pass to superclass __init__)
        - ``args``: runner-specific argument namespace parsed from
          execution environment, as specified by ``add_parser()``.'''
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
    def do_create(cls, cfg, args):
        '''Hook for instance creation from command line arguments.'''

    @classmethod
    def get_flash_address(cls, args, build_conf, default=0x0):
        '''Helper method for extracting a flash address.

        If args.dt_flash is true, get the address from the
        BoardConfiguration, build_conf. (If
        CONFIG_HAS_FLASH_LOAD_OFFSET is n in that configuration, it
        returns CONFIG_FLASH_BASE_ADDRESS. Otherwise, it returns
        CONFIG_FLASH_BASE_ADDRESS + CONFIG_FLASH_LOAD_OFFSET.)

        Otherwise (when args.dt_flash is False), the default value is
        returned.'''
        if args.dt_flash:
            if build_conf['CONFIG_HAS_FLASH_LOAD_OFFSET']:
                return (build_conf['CONFIG_FLASH_BASE_ADDRESS'] +
                        build_conf['CONFIG_FLASH_LOAD_OFFSET'])
            else:
                return build_conf['CONFIG_FLASH_BASE_ADDRESS']
        else:
            return default

    def run(self, command, **kwargs):
        '''Runs command ('flash', 'debug', 'debugserver', 'attach').

        This is the main entry point to this runner.'''
        caps = self.capabilities()
        if command not in caps.commands:
            raise ValueError('runner {} does not implement command {}'.format(
                self.name(), command))
        self.do_run(command, **kwargs)

    @abc.abstractmethod
    def do_run(self, command, **kwargs):
        '''Concrete runner; run() delegates to this. Implement in subclasses.

        In case of an unsupported command, raise a ValueError.'''

    @staticmethod
    def require(program):
        '''Require that a program is installed before proceeding.

        :param program: name of the program that is required,
                        or path to a program binary.

        If ``program`` is an absolute path to an existing program
        binary, this call succeeds. Otherwise, try to find the program
        by name on the system PATH.

        On error, raises MissingProgram.'''
        if shutil.which(program) is None:
            raise MissingProgram(program)

    def run_server_and_client(self, server, client):
        '''Run a server that ignores SIGINT, and a client that handles it.

        This routine portably:

        - creates a Popen object for the ``server`` command which ignores
          SIGINT
        - runs ``client`` in a subprocess while temporarily ignoring SIGINT
        - cleans up the server after the client exits.

        It's useful to e.g. open a GDB server and client.'''
        server_proc = self.popen_ignore_int(server)
        previous = signal.signal(signal.SIGINT, signal.SIG_IGN)
        try:
            self.check_call(client)
        finally:
            signal.signal(signal.SIGINT, previous)
            server_proc.terminate()
            server_proc.wait()

    def _log_cmd(self, cmd):
        escaped = ' '.join(shlex.quote(s) for s in cmd)
        if not _DRY_RUN:
            self.logger.debug(escaped)
        else:
            self.logger.info(escaped)

    def call(self, cmd):
        '''Subclass subprocess.call() wrapper.

        Subclasses should use this method to run command in a
        subprocess and get its return code, rather than
        using subprocess directly, to keep accurate debug logs.
        '''
        self._log_cmd(cmd)
        if _DRY_RUN:
            return 0
        return subprocess.call(cmd)

    def check_call(self, cmd):
        '''Subclass subprocess.check_call() wrapper.

        Subclasses should use this method to run command in a
        subprocess and check that it executed correctly, rather than
        using subprocess directly, to keep accurate debug logs.
        '''
        self._log_cmd(cmd)
        if _DRY_RUN:
            return
        subprocess.check_call(cmd)

    def check_output(self, cmd):
        '''Subclass subprocess.check_output() wrapper.

        Subclasses should use this method to run command in a
        subprocess and check that it executed correctly, rather than
        using subprocess directly, to keep accurate debug logs.
        '''
        self._log_cmd(cmd)
        if _DRY_RUN:
            return b''
        return subprocess.check_output(cmd)

    def popen_ignore_int(self, cmd):
        '''Spawn a child command, ensuring it ignores SIGINT.

        The returned subprocess.Popen object must be manually terminated.'''
        cflags = 0
        preexec = None
        system = platform.system()

        if system == 'Windows':
            cflags |= subprocess.CREATE_NEW_PROCESS_GROUP
        elif system in {'Linux', 'Darwin'}:
            preexec = os.setsid

        self._log_cmd(cmd)
        if _DRY_RUN:
            return _DebugDummyPopen()

        return subprocess.Popen(cmd, creationflags=cflags, preexec_fn=preexec)
