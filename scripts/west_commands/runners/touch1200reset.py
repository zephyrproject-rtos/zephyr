# Copyright (c) 2026 Maksim Salau <maksim.salau@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for resetting a board using 'Touch 1200bps' mechanism.'''

from time import sleep

from serial import Serial
from serial.serialutil import SerialException
from serial.tools.list_ports import comports

from runners.core import RunnerCaps, ZephyrBinaryRunner


class Touch1200ResetRunner(ZephyrBinaryRunner):
    '''Runner front-end for touch1200reset.'''

    def __init__(self, cfg, device):
        super().__init__(cfg)

        self.device = device

    @classmethod
    def name(cls):
        return 'touch1200reset'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'reset'})

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument(
            'device',
            default='(auto)',
            nargs='?',
            help='serial port device, default: \'%(default)s\'',
        )

    @classmethod
    def do_create(cls, cfg, args):
        return Touch1200ResetRunner(cfg, device=args.device)

    def detect_device(self):
        vid = self.build_conf.get('CONFIG_CDC_ACM_SERIAL_VID')
        pid = self.build_conf.get('CONFIG_CDC_ACM_SERIAL_PID')

        self.logger.info("Searching for a USB device %04x:%04x", vid, pid)
        devices = [p.device for p in comports() if p.vid == vid and p.pid == pid]

        if len(devices) == 0:
            raise RuntimeError('No matching devices found')

        self.logger.info("Matching devices: %s", ", ".join(devices))

        if len(devices) > 1:
            raise RuntimeError(f'Several matching devices found: {devices}')

        self.device = devices[0]

    def do_run(self, _command, **kwargs):
        if self.device == '(auto)':
            self.detect_device()

        self.logger.info('Device: %s', self.device)

        try:
            with Serial(self.device, 1200):
                sleep(0.1)
        except SerialException as e:
            raise RuntimeError(e) from e
