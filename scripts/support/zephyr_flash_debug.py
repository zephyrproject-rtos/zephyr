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
import platform
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


class BossacBinaryFlasher(ZephyrBinaryFlasher):
    '''Flasher front-end for bossac.'''

    def __init__(self, bin_name, bossac='bossac', debug=False):
        super(BossacBinaryFlasher, self).__init__(debug=debug)
        self.bin_name = bin_name
        self.bossac = bossac

    def replaces_shell_script(shell_script):
        return shell_script == 'bossa-flash.sh'

    def create_from_env(debug):
        '''Create flasher from environment.

        Required:

        - O: build output directory
        - KERNEL_BIN_NAME: name of kernel binary

        Optional:

        - BOSSAC: path to bossac, default is bossac
        '''
        bin_name = path.join(get_env_or_bail('O'),
                             get_env_or_bail('KERNEL_BIN_NAME'))
        bossac = os.environ.get('BOSSAC', 'bossac')
        return BossacBinaryFlasher(bin_name, bossac=bossac, debug=debug)

    def flash(self, **kwargs):
        if platform.system() != 'Linux':
            msg = 'CAUTION: No flash tool for your host system found!'
            raise NotImplementedError(msg)

        cmd_stty = ['stty', '-F', '/dev/ttyACM0', 'raw', 'ispeed', '1200',
                    'ospeed', '1200', 'cs8', '-cstopb', 'ignpar', 'eol', '255',
                    'eof', '255']
        cmd_flash = [self.bossac, '-R', '-e', '-w', '-v', '-b', self.bin_name]

        check_call(cmd_stty, self.debug)
        check_call(cmd_flash, self.debug)


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


class Esp32BinaryFlasher(ZephyrBinaryFlasher):
    '''Flasher front-end for espidf.'''

    def __init__(self, elf, device, baud=921600, flash_size='detect',
                 flash_freq='40m', flash_mode='dio', espdif='espidf',
                 debug=False):
        super(Esp32BinaryFlasher, self).__init__(debug=debug)
        self.elf = elf
        self.device = device
        self.baud = baud
        self.flash_size = flash_size
        self.flash_freq = flash_freq
        self.flash_mode = flash_mode
        self.espdif = espdif

    def replaces_shell_script(shell_script):
        return shell_script == 'esp32.sh'

    def create_from_env(debug):
        '''Create flasher from environment.

        Required:

        - O: build output directory
        - KERNEL_ELF_NAME: name of kernel binary in ELF format

        Optional:

        - ESP_DEVICE: serial port to flash, default /dev/ttyUSB0
        - ESP_BAUD_RATE: serial baud rate, default 921600
        - ESP_FLASH_SIZE: flash size, default 'detect'
        - ESP_FLASH_FREQ: flash frequency, default '40m'
        - ESP_FLASH_MODE: flash mode, default 'dio'
        - ESP_TOOL: complete path to espdif, or set to 'espidf' to look for it
          in $ESP_IDF_PATH/components/esptool_py/esptool/esptool.py
        '''
        elf = path.join(get_env_or_bail('O'),
                        get_env_or_bail('KERNEL_ELF_NAME'))

        # TODO add sane device defaults on other platforms than Linux.
        device = os.environ.get('ESP_DEVICE', '/dev/ttyUSB0')
        baud = os.environ.get('ESP_BAUD_RATE', '921600')
        flash_size = os.environ.get('ESP_FLASH_SIZE', 'detect')
        flash_freq = os.environ.get('ESP_FLASH_FREQ', '40m')
        flash_mode = os.environ.get('ESP_FLASH_MODE', 'dio')
        espdif = os.environ.get('ESP_TOOL', 'espidf')

        if espdif == 'espdif':
            idf_path = get_env_or_bail('ESP_IDF_PATH')
            espdif = path.join(idf_path, 'components', 'esptool_py', 'esptool',
                               'esptool.py')

        return Esp32BinaryFlasher(elf, device, baud=baud,
                                  flash_size=flash_size, flash_freq=flash_freq,
                                  flash_mode=flash_mode, espdif=espdif,
                                  debug=debug)

    def flash(self, **kwargs):
        bin_name = path.splitext(self.elf)[0] + path.extsep + 'bin'
        cmd_convert = [self.espdif, '--chip', 'esp32', 'elf2image', self.elf]
        cmd_flash = [self.espdif, '--chip', 'esp32', '--port', self.device,
                     '--baud', self.baud, '--before', 'default_reset',
                     '--after', 'hard_reset', 'write_flash', '-u',
                     '--flash_mode', self.flash_mode,
                     '--flash_freq', self.flash_freq,
                     '--flash_size', self.flash_size,
                     '0x1000', bin_name]

        print("Converting ELF to BIN")
        check_call(cmd_convert, self.debug)

        print("Flashing ESP32 on {} ({}bps)".format(self.device, self.baud))
        check_call(cmd_flash, self.debug)


class Nios2Flasher(ZephyrBinaryFlasher):
    '''Flasher front-end for NIOS II.'''

    # From the original shell script:
    #
    #     "XXX [flash] only support[s] cases where the .elf is sent
    #      over the JTAG and the CPU directly boots from __start. CONFIG_XIP
    #      and CONFIG_INCLUDE_RESET_VECTOR must be disabled."

    def __init__(self, hex_, cpu_sof, zephyr_base, debug=False):
        super(Nios2Flasher, self).__init__(debug=debug)
        self.hex_ = hex_
        self.cpu_sof = cpu_sof
        self.zephyr_base = zephyr_base

    def replaces_shell_script(shell_script):
        return shell_script == 'nios2.sh'

    def create_from_env(debug):
        '''Create flasher from environment.

        Required:

        - O: build output directory
        - KERNEL_HEX_NAME: name of kernel binary in ELF format
        - NIOS2_CPU_SOF: location of the CPU .sof data
        - ZEPHYR_BASE: zephyr Git repository base directory
        '''
        hex_ = path.join(get_env_or_bail('O'),
                         get_env_or_bail('KERNEL_HEX_NAME'))
        cpu_sof = get_env_or_bail('NIOS2_CPU_SOF')
        zephyr_base = get_env_or_bail('ZEPHYR_BASE')

        return Nios2Flasher(hex_, cpu_sof, zephyr_base, debug=debug)

    def flash(self, **kwargs):
        cmd = [path.join(self.zephyr_base, 'scripts', 'support',
                         'quartus-flash.py'),
               '--sof', self.cpu_sof,
               '--kernel', self.hex_]

        check_call(cmd, self.debug)


class NrfJprogFlasher(ZephyrBinaryFlasher):
    '''Flasher front-end for nrfjprog.'''

    def __init__(self, hex_, family, board, debug=False):
        super(NrfJprogFlasher, self).__init__(debug=debug)
        self.hex_ = hex_
        self.family = family
        self.board = board

    def replaces_shell_script(shell_script):
        return shell_script == 'nrf_flash.sh'

    def create_from_env(debug):
        '''Create flasher from environment.

        Required:

        - O: build output directory
        - KERNEL_HEX_NAME: name of kernel binary in ELF format
        - NRF_FAMILY: e.g. NRF51 or NRF52
        - BOARD: Zephyr board name
        '''
        hex_ = path.join(get_env_or_bail('O'),
                         get_env_or_bail('KERNEL_HEX_NAME'))
        family = get_env_or_bail('NRF_FAMILY')
        board = get_env_or_bail('BOARD')

        return NrfJprogFlasher(hex_, family, board, debug=debug)

    def get_board_snr_from_user(self):
        snrs = check_output(['nrfjprog', '--ids'], self.debug)
        snrs = snrs.decode(sys.getdefaultencoding()).strip().splitlines()

        if len(snrs) == 1:
            return snrs[0]

        print('There are multiple boards connected.')
        for i, snr in enumerate(snrs, 1):
            print('{}. {}'.format(i, snr))

        p = 'Please select one with desired serial number (1-{}): '.format(
                len(snrs))
        while True:
            value = input(p)
            try:
                value = int(value)
            except ValueError:
                continue
            if 1 <= value <= len(snrs):
                break

        return snrs[value - 1]

    def flash(self, **kwargs):
        board_snr = self.get_board_snr_from_user()

        print('Flashing file: {}'.format(self.hex_))
        commands = [
            ['nrfjprog', '--eraseall', '-f', self.family, '--snr', board_snr],
            ['nrfjprog', '--program', self.hex_, '-f', self.family, '--snr',
             board_snr],
        ]
        if self.family == 'NRF52':
            commands.extend([
                # Set reset pin
                ['nrfjprog', '--memwr', '0x10001200', '--val', '0x00000015',
                 '-f', self.family, '--snr', board_snr],
                ['nrfjprog', '--memwr', '0x10001204', '--val', '0x00000015',
                 '-f', self.family, '--snr', board_snr],
                ['nrfjprog', '--reset', '-f', self.family, '--snr', board_snr],
            ])

        for cmd in commands:
            check_call(cmd, self.debug)

        print('{} Serial Number {} flashed with success.'.format(
                  self.board, board_snr))


class OpenOcdBinaryFlasher(ZephyrBinaryFlasher):
    '''Flasher front-end for openocd.'''

    def __init__(self, bin_name, zephyr_base, arch, board_name,
                 load_cmd, verify_cmd, openocd='openocd',
                 default_path=None, pre_cmd=None,
                 post_cmd=None, debug=False):
        super(OpenOcdBinaryFlasher, self).__init__(debug=debug)
        self.bin_name = bin_name
        self.zephyr_base = zephyr_base
        self.arch = arch
        self.board_name = board_name
        self.load_cmd = load_cmd
        self.verify_cmd = verify_cmd
        self.openocd = openocd
        self.default_path = default_path
        self.pre_cmd = pre_cmd
        self.post_cmd = post_cmd

    def replaces_shell_script(shell_script):
        return shell_script == 'openocd.sh'

    def create_from_env(debug):
        '''Create flasher from environment.

        Required:

        - O: build output directory
        - KERNEL_BIN_NAME: zephyr kernel binary
        - ZEPHYR_BASE: zephyr Git repository base directory
        - ARCH: board architecture
        - BOARD_NAME: zephyr name of board
        - OPENOCD_LOAD_CMD: command to load binary into flash
        - OPENOCD_VERIFY_CMD: command to verify flash executed correctly

        Optional:

        - OPENOCD: path to openocd, defaults to openocd
        - OPENOCD_DEFAULT_PATH: openocd search path to use
        - OPENOCD_PRE_CMD: command to run before any others
        - OPENOCD_POST_CMD: command to run after verifying flash write
        '''
        bin_name = path.join(get_env_or_bail('O'),
                             get_env_or_bail('KERNEL_BIN_NAME'))
        zephyr_base = get_env_or_bail('ZEPHYR_BASE')
        arch = get_env_or_bail('ARCH')
        board_name = get_env_or_bail('BOARD_NAME')
        load_cmd = get_env_or_bail('OPENOCD_LOAD_CMD').strip('"')
        verify_cmd = get_env_or_bail('OPENOCD_VERIFY_CMD').strip('"')

        openocd = os.environ.get('OPENOCD', 'openocd')
        default_path = os.environ.get('OPENOCD_DEFAULT_PATH', None)
        pre_cmd = os.environ.get('OPENOCD_PRE_CMD', None)
        post_cmd = os.environ.get('OPENOCD_POST_CMD', None)

        return OpenOcdBinaryFlasher(bin_name, zephyr_base, arch, board_name,
                                    load_cmd, verify_cmd, openocd=openocd,
                                    default_path=default_path, pre_cmd=pre_cmd,
                                    post_cmd=post_cmd, debug=debug)

    def flash(self, **kwargs):
        search_args = []
        if self.default_path is not None:
            search_args = ['-s', self.default_path]

        config = path.join(self.zephyr_base, 'boards', self.arch,
                           self.board_name, 'support', 'openocd.cfg')

        pre_cmd = []
        if self.pre_cmd is not None:
            pre_cmd = ['-c', self.pre_cmd]

        post_cmd = []
        if self.post_cmd is not None:
            post_cmd = ['-c', self.post_cmd]

        cmd = ([self.openocd] +
               search_args +
               ['-f', config,
                '-c', 'init',
                '-c', 'targets'] +
               pre_cmd +
               ['-c', 'reset halt',
                '-c', self.load_cmd,
                '-c', 'reset halt',
                '-c', self.verify_cmd] +
               post_cmd +
               ['-c', 'reset run',
                '-c', 'shutdown'])
        check_call(cmd, self.debug)


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
