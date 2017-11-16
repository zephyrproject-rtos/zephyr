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
import os
import platform
import pprint
import shlex
import signal
import subprocess
import sys


def get_env_or_bail(env_var):
    '''Get an environment variable, or raise an error.

    In case of KeyError, an error message is printed, along with the
    environment, and the exception is re-raised.
    '''
    try:
        return os.environ[env_var]
    except KeyError:
        print('Variable {} not in environment:'.format(
                  env_var), file=sys.stderr)
        pprint.pprint(dict(os.environ), stream=sys.stderr)
        raise


def get_env_bool_or(env_var, default_value):
    '''Get an environment variable as a boolean, or return a default value.

    Get an environment variable, interpret it as a base ten
    integer, and convert that to a boolean.

    In case the environment variable is not defined, return default_value.
    '''
    try:
        return bool(int(os.environ[env_var]))
    except KeyError:
        return default_value


def get_env_strip_or(env_var, to_strip, default_value):
    '''Get and clean up an environment variable, or return a default value.

    Get the value of env_var from the environment. If it is
    defined, return that value with to_strip stripped off. If it
    is undefined, return default_value (without any stripping).
    '''
    value = os.environ.get(env_var, None)
    if value is not None:
        return value.strip(to_strip)
    else:
        return default_value


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


class RunnerCaps:
    '''This class represents a runner class's capabilities.

    The most basic capability is the set of supported commands,
    available in the commands field. This defaults to all three
    commands.'''

    def __init__(self, commands={'flash', 'debug', 'debugserver'}):
        self.commands = commands


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
    name, and declares commands it can handle. Zephyr boards declare
    compatible runner(s) by name to the build system, which can then
    call into the create_runner() method below to make a concrete
    runner instance for use executing a command.

    If your board can use an existing runner, all you have to do is
    give its name to the build system. How to do that is out of the
    scope of this documentation, but use the existing boards as a
    starting point.

    If you want to define and use your own runner:

    1. Define a ZephyrBinaryRunner subclass, and implement its
       abstract methods. Override any methods you need to, especially
       capabilities().

    2. Make sure the Python module defining your runner class is
       imported by this package's __init__.py (otherwise,
       create_runner() won't work).

    3. Give your runner's name to the Zephyr build system in your
       board's build files.

    Runners use a variety of target-specific tools and configuration
    values, the user interface to which is abstracted by this
    class. Each runner subclass should take any values it needs to
    execute one of these commands in its constructor.  The actual
    command execution is handled in the run() method.

    At present, the Zephyr build system uses environment variables to
    control runner behavior.  To support this, a create_runner()
    method is defined below.  This method takes a runner name, and
    iterates over defined ZephyrBinaryRunner subclasses to find the
    runner class. It then checks that it supports the command, then
    instantiates and returns a runner with configuration determined by
    the environment.

    To support this, subclasses currently must define a static method:
    create_from_env(). This is called by create_runner() to create a
    concrete runner instance.

    The environment-based factories are for legacy use *only*; the
    build system is moving away from use of environment variables. The
    user must be able to construct and use a runner using only the
    constructor and run() method.

    '''

    def __init__(self, debug=False):
        self.debug = debug

    @staticmethod
    def create_runner(runner_name, command, debug):
        for cls in ZephyrBinaryRunner.__subclasses__():
            if cls.name() == runner_name:
                break
        else:
            raise ValueError('no runner named {} is known'.format(runner_name))

        caps = cls.capabilities()
        if command not in caps.commands:
            raise ValueError('runner {} does not implement command {}'.format(
                runner_name, command))

        return cls.create_from_env(command, debug)

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

        This implementation returns the default capabilities, which
        includes support for all three commands, but no other special
        powers.

        Subclasses should override appropriately if needed.'''
        return RunnerCaps()

    @staticmethod
    @abc.abstractmethod
    def create_from_env(command, debug):
        '''Create new runner instance from environment variables.'''

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
        if self.debug:
            print(quote_sh_list(cmd))
        subprocess.check_call(cmd)

    def check_output(self, cmd):
        '''Subclass subprocess.check_output() wrapper.

        Subclasses should use this command to run command in a
        subprocess and check that it executed correctly, rather than
        using subprocess directly, to keep accurate debug logs.
        '''
        if self.debug:
            print(quote_sh_list(cmd))
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

        if self.debug:
            print(quote_sh_list(cmd))

        return subprocess.Popen(cmd, creationflags=cflags, preexec_fn=preexec)
