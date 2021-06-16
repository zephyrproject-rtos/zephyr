# Copyright (c) 2017 Linaro Limited.
# Copyright (c) 2020 Gerson Fernando Budke <nandojve@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0

'''bossac-specific runner (flash only) for Atmel SAM microcontrollers.'''

import pathlib
import pickle
import platform
import subprocess
import sys

from runners.core import ZephyrBinaryRunner, RunnerCaps

# This is needed to load edt.pickle files.
try:
    from devicetree import edtlib  # pylint: disable=unused-import
    MISSING_EDTLIB = False
except ImportError:
    # This can happen when building the documentation for the
    # runners package if edtlib is not on sys.path. This is fine
    # to ignore in that case.
    MISSING_EDTLIB = True

if platform.system() == 'Darwin':
    DEFAULT_BOSSAC_PORT = None
else:
    DEFAULT_BOSSAC_PORT = '/dev/ttyACM0'
DEFAULT_BOSSAC_SPEED = '115200'

class BossacBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for bossac.'''

    def __init__(self, cfg, bossac='bossac', port=DEFAULT_BOSSAC_PORT,
                 speed=DEFAULT_BOSSAC_SPEED):
        super().__init__(cfg)
        self.bossac = bossac
        self.port = port
        self.speed = speed

    @classmethod
    def name(cls):
        return 'bossac'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'})

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--bossac', default='bossac',
                            help='path to bossac, default is bossac')
        parser.add_argument('--bossac-port', default=DEFAULT_BOSSAC_PORT,
                            help='serial port to use, default is ' +
                            str(DEFAULT_BOSSAC_PORT))
        parser.add_argument('--speed', default=DEFAULT_BOSSAC_SPEED,
                            help='serial port speed to use, default is ' +
                            DEFAULT_BOSSAC_SPEED)

    @classmethod
    def do_create(cls, cfg, args):
        return BossacBinaryRunner(cfg, bossac=args.bossac,
                                  port=args.bossac_port, speed=args.speed)

    def read_help(self):
        """Run bossac --help and return the output as a list of lines"""
        self.require(self.bossac)
        try:
            # BOSSA > 1.9.1 returns OK
            out = self.check_output([self.bossac, '--help']).decode()
        except subprocess.CalledProcessError as ex:
            # BOSSA <= 1.9.1 returns an error
            out = ex.output.decode()

        return out.split('\n')

    def supports(self, flag):
        """Check if bossac supports a flag by searching the help"""
        for line in self.read_help():
            if flag in line:
                return True
        return False

    def is_extended_samba_protocol(self):
        ext_samba_versions = ['CONFIG_BOOTLOADER_BOSSA_ARDUINO',
                              'CONFIG_BOOTLOADER_BOSSA_ADAFRUIT_UF2']

        for x in ext_samba_versions:
            if self.build_conf.getboolean(x):
                return True
        return False

    def is_partition_enabled(self):
        return self.build_conf.getboolean('CONFIG_USE_DT_CODE_PARTITION')

    def get_chosen_code_partition_node(self):
        # Get the EDT Node corresponding to the zephyr,code-partition
        # chosen DT node

        # Ensure the build directory has a compiled DTS file
        # where we expect it to be.
        b = pathlib.Path(self.cfg.build_dir)
        edt_pickle = b / 'zephyr' / 'edt.pickle'
        if not edt_pickle.is_file():
            error_msg = "can't load devicetree; expected to find:" \
	                + str(edt_pickle)

            raise RuntimeError(error_msg)

        # Load the devicetree.
        with open(edt_pickle, 'rb') as f:
            edt = pickle.load(f)

        return edt.chosen_node('zephyr,code-partition')

    def get_board_name(self):
        if 'CONFIG_BOARD' not in self.build_conf:
            return '<board>'

        return self.build_conf['CONFIG_BOARD']

    def get_dts_img_offset(self):
        if self.build_conf.getboolean('CONFIG_BOOTLOADER_BOSSA_LEGACY'):
            return 0

        if self.build_conf.getboolean('CONFIG_HAS_FLASH_LOAD_OFFSET'):
            return self.build_conf['CONFIG_FLASH_LOAD_OFFSET']

        return 0

    def get_image_offset(self, supports_offset):
        """Validates and returns the flash offset"""

        dts_img_offset = self.get_dts_img_offset()

        if int(str(dts_img_offset), 16) > 0:
            if not supports_offset:
                old_sdk = 'This version of BOSSA does not support the' \
                          ' --offset flag. Please upgrade to a newer Zephyr' \
                          ' SDK version >= 0.12.0.'
                raise RuntimeError(old_sdk)

            return dts_img_offset

        return None

    def set_serial_config(self):
        if platform.system() == 'Linux' or platform.system() == 'Darwin':
            self.require('stty')

            # GNU coreutils uses a capital F flag for 'file'
            flag = '-F' if platform.system() == 'Linux' else '-f'

            if self.is_extended_samba_protocol():
                self.speed = '1200'

            cmd_stty = ['stty', flag, self.port, 'raw', 'ispeed', self.speed,
                        'ospeed', self.speed, 'cs8', '-cstopb', 'ignpar',
                        'eol', '255', 'eof', '255']
            self.check_call(cmd_stty)

    def make_bossac_cmd(self):
        self.ensure_output('bin')
        cmd_flash = [self.bossac, '-p', self.port, '-R', '-e', '-w', '-v',
                     '-b', self.cfg.bin_file]

        dt_chosen_code_partition_nd = self.get_chosen_code_partition_node()

        if self.is_partition_enabled():
            if dt_chosen_code_partition_nd is None:
                error_msg = 'The device tree zephyr,code-partition chosen' \
                            ' node must be defined.'

                raise RuntimeError(error_msg)

            offset = self.get_image_offset(self.supports('--offset'))

            if offset is not None and int(str(offset), 16) > 0:
                cmd_flash += ['-o', '%s' % offset]

        elif dt_chosen_code_partition_nd is not None:
            error_msg = 'There is no CONFIG_USE_DT_CODE_PARTITION Kconfig' \
                        ' defined at ' + self.get_board_name() + \
                        '_defconfig file.\n This means that' \
                        ' zephyr,code-partition device tree node should not' \
                        ' be defined. Check Zephyr SAM-BA documentation.'

            raise RuntimeError(error_msg)

        return cmd_flash

    def get_darwin_serial_device_list(self):
        """
        Get a list of candidate serial ports on Darwin by querying the IOKit
        registry.
        """
        import plistlib

        ioreg_out = self.check_output(['ioreg', '-r', '-c', 'IOSerialBSDClient',
                                       '-k', 'IOCalloutDevice', '-a'])
        serial_ports = plistlib.loads(ioreg_out, fmt=plistlib.FMT_XML)

        return [port["IOCalloutDevice"] for port in serial_ports]

    def get_darwin_user_port_choice(self):
        """
        Ask the user to select the serial port from a set of candidate ports
        retrieved from IOKit on Darwin.

        Modelled on get_board_snr() in the nrfjprog runner.
        """
        devices = self.get_darwin_serial_device_list()

        if len(devices) == 0:
            raise RuntimeError('No candidate serial ports were found!')
        elif len(devices) == 1:
            print('Using only serial device on the system: ' + devices[0])
            return devices[0]
        elif not sys.stdin.isatty():
            raise RuntimeError('Refusing to guess which serial port to use: '
                               f'there are {len(devices)} available. '
                               '(Interactive prompts disabled since standard '
                               'input is not a terminal - please specify a '
                               'port using --bossac-port instead)')

        print('There are multiple serial ports available on this system:')

        for i, device in enumerate(devices, 1):
            print(f'    {i}. {device}')

        p = f'Please select one (1-{len(devices)}, or EOF to exit): '

        while True:
            try:
                value = input(p)
            except EOFError:
                sys.exit(0)
            try:
                value = int(value)
            except ValueError:
                continue
            if 1 <= value <= len(devices):
                break

        return devices[value - 1]

    def do_run(self, command, **kwargs):
        if MISSING_EDTLIB:
            self.logger.warning(
                'could not import edtlib; something may be wrong with the '
                'python environment')

        if platform.system() == 'Windows':
            msg = 'CAUTION: BOSSAC runner not support on Windows!'
            raise RuntimeError(msg)
        elif platform.system() == 'Darwin' and self.port is None:
            self.port = self.get_darwin_user_port_choice()

        self.require(self.bossac)
        self.set_serial_config()
        self.check_call(self.make_bossac_cmd())
