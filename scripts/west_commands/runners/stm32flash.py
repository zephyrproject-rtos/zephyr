# Copyright (c) 2019 Thomas Kupper <thomas.kupper@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for flashing with stm32flash.'''

import platform
from os import path

from runners.core import RunnerCaps, ZephyrBinaryRunner

DEFAULT_DEVICE = '/dev/ttyUSB0'
if platform.system() == 'Darwin':
    DEFAULT_DEVICE = '/dev/tty.SLAB_USBtoUART'

class Stm32flashBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for stm32flash.'''

    def __init__(self, cfg, device, action='write', baud=57600,
                 force_binary=False, start_addr=0, exec_addr=None,
                 serial_mode='8e1', reset=False, verify=False):
        super().__init__(cfg)

        self.device = device
        self.action = action
        self.baud = baud
        self.force_binary = force_binary
        self.start_addr = start_addr
        self.exec_addr = exec_addr
        self.serial_mode = serial_mode
        self.reset = reset
        self.verify = verify

    @classmethod
    def name(cls):
        return 'stm32flash'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'}, reset=True)

    @classmethod
    def do_add_parser(cls, parser):

        # required argument(s)
        # none for now

        # optional argument(s)
        parser.add_argument('--device', default=DEFAULT_DEVICE, required=False,
                            help='serial port to flash, default \'' + DEFAULT_DEVICE + '\'')

        parser.add_argument('--action', default='write', required=False,
                            choices=['erase', 'info', 'start', 'write'],
                            help='erase / get device info / start execution / write flash')

        parser.add_argument('--baud-rate', default='57600', required=False,
                            choices=['1200', '1800', '2400', '4800', '9600', '19200',
                            '38400', '57600', '115200', '230400', '256000', '460800',
                            '500000', '576000', '921600', '1000000', '1500000', '2000000'],
                            help='serial baud rate, default \'57600\'')

        parser.add_argument('--force-binary', required=False, action='store_true',
                            help='force the binary parser')

        parser.add_argument('--start-addr', default=0, required=False,
                            help='specify start address for write operation, default \'0\'')

        parser.add_argument('--execution-addr', default=None, required=False,
                            help='start execution at specified address, default \'0\' \
                            which means start of flash')

        parser.add_argument('--serial-mode', default='8e1', required=False,
                            help='serial port mode, default \'8e1\'')

        parser.add_argument('--verify', default=False, required=False, action='store_true',
                            help='verify writes, default False')

        parser.set_defaults(reset=False)

    @classmethod
    def do_create(cls, cfg, args):
        return Stm32flashBinaryRunner(cfg, device=args.device, action=args.action,
            baud=args.baud_rate, force_binary=args.force_binary,
            start_addr=args.start_addr, exec_addr=args.execution_addr,
            serial_mode=args.serial_mode, reset=args.reset, verify=args.verify)

    def do_run(self, command, **kwargs):
        self.require('stm32flash')
        self.ensure_output('bin')

        bin_name = self.cfg.bin_file
        bin_size = path.getsize(bin_name)

        cmd_flash = ['stm32flash', '-b', self.baud,
            '-m', self.serial_mode]

        action = self.action.lower()

        if action == 'info':
            # show device information and exit
            msg_text = "get device info from {}".format(self.device)

        elif action == 'erase':
            # erase flash
            #size_aligned = (int(bin_size)  >> 12) + 1 << 12
            size_aligned = (int(bin_size) & 0xfffff000) + 4096
            msg_text = "erase {} bit starting at {}".format(size_aligned, self.start_addr)
            cmd_flash.extend([
            '-S', str(self.start_addr) + ":" + str(size_aligned), '-o'])

        elif action == 'start':
            # start execution
            msg_text = "start code execution at {}".format(self.exec_addr)
            if self.exec_addr:
                if self.exec_addr == 0 or self.exec_addr.lower() == '0x0':
                    msg_text += " (flash start)"
            else:
                self.exec_addr = 0
            cmd_flash.extend([
            '-g', str(self.exec_addr)])

        elif action == 'write':
            # flash binary file
            msg_text = "write {} bytes starting at {}".format(bin_size, self.start_addr)
            cmd_flash.extend([
            '-S', str(self.start_addr) + ":" + str(bin_size),
            '-w', bin_name])

            if self.exec_addr:
                cmd_flash.extend(['-g', self.exec_addr])

            if self.force_binary:
                cmd_flash.extend(['-f'])

            if self.reset:
                cmd_flash.extend(['-R'])

            if self.verify:
                cmd_flash.extend(['-v'])

        else:
            msg_text = "invalid action \'{}\' passed!".format(action)
            self.logger.error('Invalid action \'{}\' passed!'.format(action))
            return -1

        cmd_flash.extend([self.device])
        self.logger.info("Board: " + msg_text)
        self.check_call(cmd_flash)
        self.logger.info('Board: finished \'{}\' .'.format(action))
