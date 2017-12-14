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
import os
import platform
import shlex
import signal
import subprocess


# Turn on to enable just printing the commands that would be run,
# without actually running them. This can break runners that are expecting
# output or if one command depends on another, so it's just for debugging.
DEBUG = False


class _DebugDummyPopen:

    def terminate(self):
        pass

    def wait(self):
        pass


def quote_sh_list(cmd):
    '''Transform a command from list into shell string form.'''
    fmt = ' '.join('{}' for _ in cmd)
    args = [shlex.quote(s) for s in cmd]
    return fmt.format(*args)


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

    def _parser_windows(self, cmd):
        out = subprocess.check_output(cmd).split(b'\r\n')
        used_bytes = [x.split()[1].rsplit(b':', 1)[1] for x in out
                      if x.startswith(b'  TCP')]
        return {int(b) for b in used_bytes}

    def _parser_linux(self, cmd):
        out = subprocess.check_output(cmd).splitlines()[1:]
        used_bytes = [s.split()[3].rsplit(b':', 1)[1] for s in out]
        return {int(b) for b in used_bytes}

    def _parser_darwin(self, cmd):
        out = subprocess.check_output(cmd).split(b'\n')
        used_bytes = [x.split()[3].rsplit(b':', 1)[1] for x in out
                      if x.startswith(b'tcp')]
        return {int(b) for b in used_bytes}


class BuildConfiguration:
    '''This helper class provides access to build-time configuration.

    Configuration options can be read as if the object were a dict,
    either object['CONFIG_FOO'] or object.get('CONFIG_FOO').

    Configuration values in .config and generated_dts_board.conf are
    available.'''

    def __init__(self, build_dir):
        self.build_dir = build_dir
        self.options = {}
        self._init()

    def __getitem__(self, item):
        return self.options[item]

    def get(self, option, *args):
        return self.options.get(option, *args)

    def _init(self):
        build_z = os.path.join(self.build_dir, 'zephyr')
        generated = os.path.join(build_z, 'include', 'generated')
        files = [os.path.join(build_z, '.config'),
                 os.path.join(generated, 'generated_dts_board.conf')]
        for f in files:
            self._parse(f)

    def _parse(self, filename):
        with open(filename, 'r') as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith('#'):
                    continue
                option, value = line.split('=', 1)
                self.options[option] = self._parse_value(value)

    def _parse_value(self, value):
        if value.startswith('"') or value.startswith("'"):
            return value.split()
        try:
            return int(value, 0)
        except ValueError:
            return value


class RunnerCaps:
    '''This class represents a runner class's capabilities.

    Each capability is represented as an attribute with the same
    name. Flag attributes are True or False.

    Available capabilities:

    - commands: set of supported commands; default is {'flash',
      'debug', 'debugserver'}.

    - flash_addr: whether the runner supports flashing to an
      arbitrary address. Default is False. If true, the runner
      must honor the --dt-flash option.
    '''

    def __init__(self,
                 commands={'flash', 'debug', 'debugserver'},
                 flash_addr=False):
        self.commands = commands
        self.flash_addr = bool(flash_addr)


_YN_CHOICES = ['Y', 'y', 'N', 'n', 'yes', 'no', 'YES', 'NO']


class _DTFlashAction(argparse.Action):

    def __call__(self, parser, namespace, values, option_string=None):
        if values.lower().startswith('y'):
            namespace.dt_flash = True
        else:
            namespace.dt_flash = False


class ZephyrBinaryRunner(abc.ABC):
    '''Abstract superclass for binary runners (flashers, debuggers).

    **Note**: these APIs are still evolving, and will change!

    With some exceptions, boards supported by Zephyr must provide
    generic means to be flashed (have a Zephyr firmware binary
    permanently installed on the device for running) and debugged
    (have a breakpoint debugger and program loader on a host
    workstation attached to a running target).

    This is supported by three top-level commands managed by the
    Zephyr build system:

    - 'flash': flash a previously configured binary to the board,
      start execution on the target, then return.

    - 'debug': connect to the board via a debugging protocol, then
      drop the user into a debugger interface with symbol tables
      loaded from the current binary, and block until it exits.

    - 'debugserver': connect via a board-specific debugging protocol,
      then reset and halt the target. Ensure the user is now able to
      connect to a debug server with symbol tables loaded from the
      binary.

    This class provides an API for these commands. Every runner has a
    name (like 'pyocd'), and declares commands it can handle (like
    'flash'). Zephyr boards (like 'nrf52_pca10040') declare compatible
    runner(s) by name to the build system, which makes concrete runner
    instances to execute commands via this class.

    If your board can use an existing runner, all you have to do is
    give its name to the build system. How to do that is out of the
    scope of this documentation, but use the existing boards as a
    starting point.

    If you want to define and use your own runner:

    1. Define a ZephyrBinaryRunner subclass, and implement its
       abstract methods. You may need to override capabilities().

    2. Make sure the Python module defining your runner class is
       imported, e.g. by editing this package's __init__.py (otherwise,
       get_runners() won't work).

    3. Give your runner's name to the Zephyr build system in your
       board's build files.

    For command-line invocation from the Zephyr build system, runners
    define their own argparse-based interface through the common
    add_parser() (and runner-specific do_add_parser() it delegates
    to), and provide a way to create instances of themselves from
    parsed arguments via create_from_args().

    Runners use a variety of target-specific tools and configuration
    values, the user interface to which is abstracted by this
    class. Each runner subclass should take any values it needs to
    execute one of these commands in its constructor.  The actual
    command execution is handled in the run() method.'''

    def __init__(self, debug=False):
        self.debug = debug

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
        the target architecture/board (like xtensa, em-starterkit,
        etc.).'''

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

        The standard (required) arguments are:

        * --board-dir
        * --kernel-elf, --kernel-hex, --kernel-bin

        The standard optional arguments are:

        * --gdb
        * --openocd, --openocd-search
        * --dt-flash (if the runner capabilities includes flash_addr)

        Runner-specific options are added through the do_add_parser()
        hook.

        The single positional argument is "command". This is currently
        restricted to values 'flash', 'debug', and 'debugserver'.'''
        # Required options.
        parser.add_argument('--board-dir', required=True,
                            help='Zephyr board directory')
        parser.add_argument('--kernel-elf', required=True,
                            help='path to kernel binary in .elf format')
        parser.add_argument('--kernel-hex', required=True,
                            help='path to kernel binary in .hex format')
        parser.add_argument('--kernel-bin', required=True,
                            help='path to kernel binary in .bin format')

        # Optional options.
        if cls.capabilities().flash_addr:
            parser.add_argument('--dt-flash', default='n', choices=_YN_CHOICES,
                                action=_DTFlashAction,
                                help='''If 'yes', use configuration
                                generated by device tree (DT) to compute flash
                                addresses.''')
        parser.add_argument('--gdb', default=None,
                            help='GDB compatible with the target')
        parser.add_argument('--openocd', default='openocd',
                            help='OpenOCD to use')
        parser.add_argument('--openocd-search', default=None,
                            help='directory to add to OpenOCD search path')

        # Runner-specific options.
        cls.do_add_parser(parser)

        # The lone positional argument. Note that argparse can't cope
        # with adding options after the first positional argument, so
        # this must come last.
        parser.add_argument('command',
                            choices=['flash', 'debug', 'debugserver'],
                            help='command to run (flash, debug, debugserver)')

    @classmethod
    @abc.abstractmethod
    def do_add_parser(cls, parser):
        '''Hook for adding runner-specific options.

        Subclasses **must not** add positional arguments. That is, when
        calling parser.add_argument(), make sure to begin the argument
        with '-' so it is interpreted as an option, rather than a
        positional argument.

        * OK: parser.add_argument('--my-option')
        * Not OK: parser.add_argument('my-argument').'''

    @classmethod
    @abc.abstractmethod
    def create_from_args(cls, args):
        '''Create an instance from command-line arguments.

        These will have been parsed from the command line according to
        the specification defined by add_parser().'''

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
        '''Runs command ('flash', 'debug', 'debugserver').

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

    def run_server_and_client(self, server, client):
        '''Run a server that ignores SIGINT, and a client that handles it.

        This routine portably:

        - creates a Popen object for the `server' command which ignores SIGINT
        - runs `client' in a subprocess while temporarily ignoring SIGINT
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

    def check_call(self, cmd):
        '''Subclass subprocess.check_call() wrapper.

        Subclasses should use this command to run command in a
        subprocess and check that it executed correctly, rather than
        using subprocess directly, to keep accurate debug logs.
        '''
        if DEBUG or self.debug:
            print(quote_sh_list(cmd))

        if DEBUG:
            return

        try:
            subprocess.check_call(cmd)
        except subprocess.CalledProcessError:
            print('Error running {}'.format(quote_sh_list(cmd)))
            raise

    def check_output(self, cmd):
        '''Subclass subprocess.check_output() wrapper.

        Subclasses should use this command to run command in a
        subprocess and check that it executed correctly, rather than
        using subprocess directly, to keep accurate debug logs.
        '''
        if self.debug:
            print(quote_sh_list(cmd))

        if DEBUG:
            return b''

        try:
            return subprocess.check_output(cmd)
        except subprocess.CalledProcessError:
            print('Error running {}'.format(quote_sh_list(cmd)))
            raise

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

        if DEBUG or self.debug:
            print(quote_sh_list(cmd))

        if DEBUG:
            return _DebugDummyPopen()

        return subprocess.Popen(cmd, creationflags=cflags, preexec_fn=preexec)
