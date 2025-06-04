# Copyright (c) 2025 Pete Johanson
#
# SPDX-License-Identifier: Apache-2.0
#
# pylint: disable=duplicate-code

'''Runner for rfp.'''

import os
import platform
import re
from pathlib import Path

from runners.core import RunnerCaps, ZephyrBinaryRunner

if platform.system() in ('Darwin', 'Windows'):
    DEFAULT_RFP_PORT = None
else:
    DEFAULT_RFP_PORT = '/dev/ttyACM0'

RFP_CLI_EXE = 'rfp-cli'


def to_num(number):
    dev_match = re.search(r"^\d*\+dev", number)
    dev_version = dev_match is not None

    num_match = re.search(r"^\d*", number)
    num = int(num_match.group(0))

    if dev_version:
        num += 1

    return num


class RfpBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for rfp.'''

    def __init__(
        self,
        cfg,
        rfp_cli='rfp-cli',
        device=None,
        erase=False,
        verify=False,
        port=DEFAULT_RFP_PORT,
        tool=None,
        interface=None,
        rpd_file=None,
        speed=None,
        dt_flash=True,
    ):
        super().__init__(cfg)
        self.file = cfg.file
        self.file_type = cfg.file_type
        self.hex_name = cfg.hex_file
        self.bin_name = cfg.bin_file
        self.elf_name = cfg.elf_file
        self.mot_name = cfg.mot_file
        self.dt_flash = dt_flash
        self.rfp_cmd = [rfp_cli]
        self.verify = verify
        self.erase = erase
        self.port = port
        self.tool = tool
        self.interface = interface
        self.device = device
        self.rpd_file = rpd_file
        self.speed = speed

    @classmethod
    def name(cls):
        return 'rfp'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'}, erase=True)

    @classmethod
    def do_add_parser(cls, parser):
        # Find the default efp-cli executable
        cls.default_rfp()

        parser.add_argument(
            '--rfp-cli', default=RFP_CLI_EXE, help='path to rfp-cli, default is rfp-cli'
        )
        parser.add_argument(
            '--port',
            default=DEFAULT_RFP_PORT,
            help='serial port to use, default is ' + str(DEFAULT_RFP_PORT),
        )
        parser.add_argument(
            '--tool',
            help='emulator hardware to use (e2, e2l, jlink) when port is not set',
        )
        parser.add_argument(
            '--interface',
            help='selects the communications interface (uart, swd)',
        )
        parser.add_argument(
            '--rpd-file',
            help='path to renesas partition data zephyr.rpd',
        )
        parser.add_argument('--device', help='Specify the device type to pass to rfp-cli')
        parser.add_argument('--verify', action='store_true', help='if given, verify after flash')
        parser.add_argument('--speed', help='Specify the serial port speed')

    @classmethod
    def do_create(cls, cfg, args):
        return RfpBinaryRunner(
            cfg,
            rfp_cli=args.rfp_cli,
            device=args.device,
            port=args.port,
            tool=args.tool,
            interface=args.interface,
            rpd_file=args.rpd_file,
            erase=args.erase,
            speed=args.speed,
            verify=args.verify,
        )

    @staticmethod
    def default_rfp():
        global RFP_CLI_EXE

        if platform.system() == 'Windows':
            try:
                import winreg

                registry = winreg.ConnectRegistry(None, winreg.HKEY_LOCAL_MACHINE)
                key = winreg.OpenKey(registry, r"SOFTWARE\Classes\rpjfile\shell\Open\command")
                val = winreg.QueryValue(key, None)
                match = re.match(r'"(.*?)".*', val)[1]
                RFP_CLI_EXE = str(Path(match).parent / 'rfp-cli.exe')
            except Exception:
                RFP_CLI_EXE = 'rfp-cli.exe'
        else:
            RFP_CLI_EXE = 'rfp-cli'

    def do_run(self, command, **kwargs):
        if command == 'flash':
            if self.rpd_file is not None:
                self.do_partition(**kwargs)

            self.do_flash(**kwargs)
        else:
            self.logger.error("Unsuppported command")

    def do_flash(self, **kwargs):
        self.require(self.rfp_cmd[0])

        flash_file = None
        load_image = ['-run']

        if self.erase:
            load_image += ['-erase']
        else:
            load_image += ['-noerase']

        if self.verify:
            load_image += ['-v']

        if self.hex_name is not None and os.path.isfile(self.hex_name):
            flash_file = self.hex_name
            load_image += ['-file', flash_file]
        elif self.mot_name is not None and os.path.isfile(self.mot_name):
            flash_file = self.mot_name
            load_image += ['-file', flash_file]

        elif self.elf_name is not None and os.path.isfile(self.elf_name):
            flash_file = self.elf_name
            load_image += ['-file', flash_file]
        elif self.bin_name is not None and os.path.isfile(self.bin_name):
            if self.dt_flash:
                flash_addr = self.flash_address_from_build_conf(self.build_conf)
            else:
                flash_addr = 0
            flash_file = self.bin_name
            load_image += ['-bin', f'0x{flash_addr:x}', flash_file]
        else:
            err = (
                "Cannot flash; no valid firmware file found.\n"
                f"Checked: user file ({self.file}), hex ({self.hex_name}), "
                f"mot ({self.mot_name}), bin ({self.bin_name}), elf ({self.elf_name})"
            )
            raise ValueError(err)

        self.logger.info(f'Flashing file: {flash_file}')

        if self.tool is None:
            connection = ['-port', self.port]
        else:
            connection = ['-tool', self.tool]

        if self.interface:
            connection += ['-interface', self.interface]

        if self.speed:
            connection += ['-s', self.speed]

        device = ['-device', self.device]

        cmd = self.rfp_cmd + connection + device + ['-p'] + load_image
        self.check_call(cmd)
