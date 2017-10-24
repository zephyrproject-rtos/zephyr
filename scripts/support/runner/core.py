#! /usr/bin/env python3

# Copyright (c) 2017 Linaro Limited.
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


class ZephyrBinaryRunner(abc.ABC):
    '''Abstract superclass for binary runners (flashers, debuggers).

    With some exceptions, boards supported by Zephyr must provide
    generic means to be flashed (have a Zephyr firmware binary
    permanently installed on the device for running) and debugged
    (have a breakpoint debugger and program loader on a host
    workstation attached to a running target). This is supported by
    three top-level commands managed by the Zephyr build system:

    - 'flash': flash a previously configured binary to the board,
      start execution on the target, then return.

    - 'debug': connect to the board via a debugging protocol, then
      drop the user into a debugger interface with symbol tables
      loaded from the current binary, and block until it exits.

    - 'debugserver': connect via a board-specific debugging protocol,
      then reset and halt the target. Ensure the user is now able to
      connect to a debug server with symbol tables loaded from the
      binary.

    Runner functionality relies on a variety of target-specific tools
    and configuration values, the user interface to which is
    abstracted by this class. Each runner subclass should take any
    values it needs to execute one of these commands in its
    constructor.  The actual command execution is handled in the run()
    method.

    This functionality is also replacing the legacy Zephyr runners,
    which are shell scripts.

    At present, the Zephyr build system uses a variety of
    tool-specific environment variables to control runner behavior.
    To support a transition to ZephyrBinaryRunner and subclasses, this
    class provides a create_for_shell_script() static factory method.
    This method iterates over ZephyrBinaryRUnner subclasses,
    determines which (if any) can provide equivalent functionality to
    the shell-based runner, and returns a subclass instance with its
    configuration determined from the environment.

    To support this, subclasess currently must provide a pair of
    static methods, replaces_shell_script() and create_from_env(). The
    first allows the runner subclass to declare which commands and
    scripts it can replace. The second is called by
    create_for_shell_script() to create a concrete runner instance.

    The environment-based factories are for legacy use *only*; the
    user must be able to construct and use a runner using only the
    constructor and run() method.'''

    def __init__(self, debug=False):
        self.debug = debug

    @staticmethod
    def create_for_shell_script(shell_script, command, debug):
        '''Factory for using as a drop-in replacement to a shell script.

        Command is one of 'flash', 'debug', 'debugserver'.

        Get runner instance to use in place of shell_script, deriving
        configuration from the environment.'''
        for sub_cls in ZephyrBinaryRunner.__subclasses__():
            if sub_cls.replaces_shell_script(shell_script, command):
                return sub_cls.create_from_env(command, debug)
        raise ValueError('cannot implement script {} command {}'.format(
                             shell_script, command))

    @staticmethod
    @abc.abstractmethod
    def replaces_shell_script(shell_script, command):
        '''Check if this class replaces shell_script for the given command.'''

    @staticmethod
    @abc.abstractmethod
    def create_from_env(command, debug):
        '''Create new flasher instance from environment variables.

        This class must be able to replace the current shell script
        (FLASH_SCRIPT or DEBUG_SCRIPT, depending on command). The
        environment variables expected by that script are used to build
        the flasher in a backwards-compatible manner.'''

    @abc.abstractmethod
    def run(self, command, **kwargs):
        '''Run a command ('flash', 'debug', 'debugserver').

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
