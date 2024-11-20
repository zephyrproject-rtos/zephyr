# Copyright (c) 2021, ATL-Electronics
# SPDX-License-Identifier: Apache-2.0

'''GigaDevice ISP tool (gd32isp) runner for serial boot ROM'''

from runners.core import RunnerCaps, ZephyrBinaryRunner

DEFAULT_GD32ISP_CLI   = 'GD32_ISP_Console'
DEFAULT_GD32ISP_PORT  = '/dev/ttyUSB0'
DEFAULT_GD32ISP_SPEED = '57600'
DEFAULT_GD32ISP_ADDR  = '0x08000000'

class Gd32ispBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for gd32isp.'''

    def __init__(self, cfg, device,
                 isp=DEFAULT_GD32ISP_CLI,
                 port=DEFAULT_GD32ISP_PORT,
                 speed=DEFAULT_GD32ISP_SPEED,
                 addr=DEFAULT_GD32ISP_ADDR):
        super().__init__(cfg)
        self.device = device
        self.isp = isp
        self.port = port
        self.speed = speed
        self.addr = addr

    @classmethod
    def name(cls):
        return 'gd32isp'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'})

    @classmethod
    def do_add_parser(cls, parser):
        # Required:
        parser.add_argument('--device', required=True,
                            help='device part number')

        # Optional:
        parser.add_argument('--isp', default=DEFAULT_GD32ISP_CLI,
                            help='path to gd32 isp console program')
        parser.add_argument('--port', default=DEFAULT_GD32ISP_PORT,
                            help='serial port to use, default is ' +
                            str(DEFAULT_GD32ISP_PORT))
        parser.add_argument('--speed', default=DEFAULT_GD32ISP_SPEED,
                            help='serial port speed to use, default is ' +
                            DEFAULT_GD32ISP_SPEED)
        parser.add_argument('--addr', default=DEFAULT_GD32ISP_ADDR,
                            help='flash address, default is ' +
                            DEFAULT_GD32ISP_ADDR)

    @classmethod
    def do_create(cls, cfg, args):
        return Gd32ispBinaryRunner(cfg,
                                   device=args.device,
                                   isp=args.isp,
                                   port=args.port,
                                   speed=args.speed,
                                   addr=args.addr)

    def do_run(self, command, **kwargs):
        self.require(self.isp)
        self.ensure_output('bin')

        cmd_flash = [self.isp,
                     '-c',
                     '--pn', self.port,
                     '--br', self.speed,
                     '--sb', '1',
                     '-i', self.device,
                     '-e',
                     '--all',
                     '-d',
                     '--a', self.addr,
                     '--fn', self.cfg.bin_file]

        self.check_call(cmd_flash)
