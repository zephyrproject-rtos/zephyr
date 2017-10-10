#! /usr/bin/env python3

# Copyright (c) 2017 Linaro Limited.
#
# SPDX-License-Identifier: Apache-2.0

"""Zephyr board flashing script

This script is a transparent replacement for an existing Zephyr flash
script. If it can invoke the flashing tools natively, it will do so; otherwise,
it delegates to the shell script passed as second argument."""

import abc
from os import path
import os
import pprint
import sys
import subprocess
import time


def get_env_or_bail(env_var):
    try:
        return os.environ[env_var]
    except KeyError:
        print('Variable {} not in environment:'.format(
                  env_var), file=sys.stderr)
        pprint.pprint(dict(os.environ), stream=sys.stderr)
        raise


def get_env_bool_or(env_var, default_value):
    try:
        return bool(int(os.environ[env_var]))
    except KeyError:
        return default_value


def check_call(cmd, debug):
    if debug:
        print(' '.join(cmd))
    subprocess.check_call(cmd)


def check_output(cmd, debug):
    if debug:
        print(' '.join(cmd))
    return subprocess.check_output(cmd)


class ZephyrBinaryFlasher(abc.ABC):
    '''Abstract superclass for flasher objects.'''

    def __init__(self, debug=False):
        self.debug = debug

    @staticmethod
    def create_for_shell_script(shell_script, debug):
        '''Factory for using as a drop-in replacement to a shell script.

        Get flasher instance to use in place of shell_script, deriving
        flasher configuration from the environment.'''
        for sub_cls in ZephyrBinaryFlasher.__subclasses__():
            if sub_cls.replaces_shell_script(shell_script):
                return sub_cls.create_from_env(debug)
        raise ValueError('no flasher replaces script {}'.format(shell_script))

    @staticmethod
    @abc.abstractmethod
    def replaces_shell_script(shell_script):
        '''Check if this flasher class replaces FLASH_SCRIPT=shell_script.'''

    @staticmethod
    @abc.abstractmethod
    def create_from_env(debug):
        '''Create new flasher instance from environment variables.

        This class must be able to replace the current FLASH_SCRIPT. The
        environment variables expected by that script are used to build
        the flasher in a backwards-compatible manner.'''

    @abc.abstractmethod
    def flash(self, **kwargs):
        '''Flash the board.'''


class DfuUtilBinaryFlasher(ZephyrBinaryFlasher):
    '''Flasher front-end for dfu-util.'''

    def __init__(self, pid, alt, img, dfuse=None, exe='dfu-util', debug=False):
        super(DfuUtilBinaryFlasher, self).__init__(debug=debug)
        self.pid = pid
        self.alt = alt
        self.img = img
        self.dfuse = dfuse
        self.exe = exe
        try:
            self.list_pattern = ', alt={}'.format(int(self.alt))
        except ValueError:
            self.list_pattern = ', name={}'.format(self.alt)

    def replaces_shell_script(shell_script):
        return shell_script == 'dfuutil.sh'

    def create_from_env(debug):
        '''Create flasher from environment.

        Required:

        - DFUUTIL_PID: USB VID:PID of the board
        - DFUUTIL_ALT: interface alternate setting number or name
        - DFUUTIL_IMG: binary to flash

        Optional:

        - DFUUTIL_DFUSE_ADDR: target address if the board is a
          DfuSe device. Ignored if not present.
        - DFUUTIL: dfu-util executable, defaults to dfu-util.
        '''
        pid = get_env_or_bail('DFUUTIL_PID')
        alt = get_env_or_bail('DFUUTIL_ALT')
        img = get_env_or_bail('DFUUTIL_IMG')
        dfuse = os.environ.get('DFUUTIL_DFUSE_ADDR', None)
        exe = os.environ.get('DFUUTIL', 'dfu-util')

        return DfuUtilBinaryFlasher(pid, alt, img, dfuse=dfuse, exe=exe,
                                    debug=debug)

    def find_device(self):
        output = check_output([self.exe, '-l'], self.debug)
        output = output.decode(sys.getdefaultencoding())
        return self.list_pattern in output

    def flash(self, **kwargs):
        reset = 0
        if not self.find_device():
            reset = 1
            print('Please reset your board to switch to DFU mode...')
            while not self.find_device():
                time.sleep(0.1)

        cmd = [self.exe, '-d,{}'.format(self.pid)]
        if self.dfuse is not None:
            cmd.extend(['-s', '{}:leave'.format(self.dfuse)])
        cmd.extend(['-a', self.alt, '-D', self.img])
        check_call(cmd, self.debug)
        if reset:
            print('Now reset your board again to switch back to runtime mode.')


class PyOcdBinaryFlasher(ZephyrBinaryFlasher):
    '''Flasher front-end for pyocd-flashtool.'''

    def __init__(self, bin_name, target, flashtool='pyocd-flashtool',
                 board_id=None, daparg=None, debug=False):
        super(PyOcdBinaryFlasher, self).__init__(debug=debug)
        self.bin_name = bin_name
        self.target = target
        self.flashtool = flashtool
        self.board_id = board_id
        self.daparg = daparg

    def replaces_shell_script(shell_script):
        return shell_script == 'pyocd.sh'

    def create_from_env(debug):
        '''Create flasher from environment.

        Required:

        - O: build output directory
        - KERNEL_BIN_NAME: name of kernel binary
        - PYOCD_TARGET: target override

        Optional:

        - PYOCD_FLASHTOOL: flash tool path, defaults to pyocd-flashtool
        - PYOCD_BOARD_ID: ID of board to flash, default is to guess
        - PYOCD_DAPARG_ARG: arguments to pass to flashtool, default is none
        '''
        bin_name = path.join(get_env_or_bail('O'),
                             get_env_or_bail('KERNEL_BIN_NAME'))
        target = get_env_or_bail('PYOCD_TARGET')

        flashtool = os.environ.get('PYOCD_FLASHTOOL', 'pyocd-flashtool')
        board_id = os.environ.get('PYOCD_BOARD_ID', None)
        daparg = os.environ.get('PYOCD_DAPARG_ARG', None)

        return PyOcdBinaryFlasher(bin_name, target,
                                  flashtool=flashtool, board_id=board_id,
                                  daparg=daparg, debug=debug)

    def flash(self, **kwargs):
        daparg_args = []
        if self.daparg is not None:
            daparg_args = ['-da', self.daparg]

        board_args = []
        if self.board_id is not None:
            board_args = ['-b', self.board_id]

        cmd = ([self.flashtool] +
               daparg_args +
               ['-t', self.target] +
               board_args +
               [self.bin_name])

        print('Flashing Target Device')
        check_call(cmd, self.debug)


# TODO: Stop using environment variables.
#
# Migrate the build system so we can use an argparse.ArgumentParser and
# per-flasher subparsers, so invoking the script becomes something like:
#
#   python zephyr_flash_debug.py openocd --openocd-bin=/openocd/path ...
#
# For now, maintain compatibility.
def flash(shell_script_full, debug):
    shell_script = path.basename(shell_script_full)
    try:
        flasher = ZephyrBinaryFlasher.create_for_shell_script(shell_script,
                                                              debug)
    except ValueError:
        # Can't create a flasher; fall back on shell script.
        check_call([shell_script_full, 'flash'], debug)
        return

    flasher.flash()


if __name__ == '__main__':
    debug = True
    try:
        debug = get_env_bool_or('KBUILD_VERBOSE', False)
        if len(sys.argv) != 3 or sys.argv[1] != 'flash':
            raise ValueError('usage: {} flash path-to-script'.format(
                sys.argv[0]))
        flash(sys.argv[2], debug)
    except Exception as e:
        if debug:
            raise
        else:
            print('Error: {}'.format(e), file=sys.stderr)
            print('Re-run with KBUILD_VERBOSE=1 for a stack trace.',
                  file=sys.stderr)
            sys.exit(1)
