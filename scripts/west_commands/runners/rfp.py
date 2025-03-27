# Copyright (c) 2025 Pete Johanson
#
# SPDX-License-Identifier: Apache-2.0
#
# pylint: disable=duplicate-code

'''Runner for rfp.'''

import platform
import re

from runners.core import RunnerCaps, ZephyrBinaryRunner

if platform.system() == 'Darwin':
    DEFAULT_RFP_PORT = None
else:
    DEFAULT_RFP_PORT = '/dev/ttyACM0'


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
        speed=None,
    ):
        super().__init__(cfg)

        self.rfp_cmd = [rfp_cli]
        self.verify = verify
        self.erase = erase
        self.port = port
        self.device = device
        self.speed = speed

    @classmethod
    def name(cls):
        return 'rfp'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'}, erase=True)

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument(
            '--rfp-cli', default='rfp-cli', help='path to rfp-cli, default is rfp-cli'
        )
        parser.add_argument(
            '--port',
            default=DEFAULT_RFP_PORT,
            help='serial port to use, default is ' + str(DEFAULT_RFP_PORT),
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
            erase=args.erase,
            speed=args.speed,
            verify=args.verify,
        )

    def do_run(self, command, **kwargs):
        if command == 'flash':
            self.do_flash(**kwargs)
        else:
            self.logger.error("Unsuppported command")

    def do_flash(self, **kwargs):
        self.ensure_output('hex')

        hex_name = self.cfg.hex_file

        self.logger.info(f'Flashing file: {hex_name}')

        load_image = ['-run']
        if self.erase:
            load_image += ['-erase']
        else:
            load_image += ['-noerase']

        if self.verify:
            load_image += ['-v']

        # Load image
        load_image += ['-p', '-file', hex_name]

        port = ['-port', self.port]
        if self.speed:
            port += ['-s', self.speed]
        device = ['-device', self.device]

        cmd = self.rfp_cmd + port + device + load_image
        self.check_call(cmd)
