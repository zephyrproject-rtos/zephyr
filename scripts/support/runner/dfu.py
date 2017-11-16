# Copyright (c) 2017 Linaro Limited.
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for flashing with dfu-util.'''

import sys
import time

from .core import ZephyrBinaryRunner, RunnerCaps


class DfuUtilBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for dfu-util.'''

    def __init__(self, pid, alt, img, dfuse=None, exe='dfu-util', debug=False):
        super(DfuUtilBinaryRunner, self).__init__(debug=debug)
        self.alt = alt
        self.img = img
        self.dfuse = dfuse
        self.cmd = [exe, '-d,{}'.format(pid)]
        try:
            self.list_pattern = ', alt={},'.format(int(self.alt))
        except ValueError:
            self.list_pattern = ', name="{}",'.format(self.alt)

    @classmethod
    def name(cls):
        return 'dfu-util'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'})

    @classmethod
    def do_add_parser(cls, parser):
        # Required:
        parser.add_argument("--pid", required=True,
                            help="USB VID:PID of the board")
        parser.add_argument("--alt", required=True,
                            help="interface alternate setting number or name")

        # Optional:
        parser.add_argument("--img",
                            help="binary to flash, default is --kernel-bin")
        parser.add_argument("--dfuse-addr", default=None,
                            help='''target address if the board is a DfuSe
                            device; ignored it not present''')
        parser.add_argument('--dfu-util', default='dfu-util',
                            help='dfu-util executable; defaults to "dfu-util"')

    @classmethod
    def create_from_args(cls, args):
        if args.img is None:
            args.img = args.kernel_bin
        return DfuUtilBinaryRunner(args.pid, args.alt, args.img,
                                   dfuse=args.dfuse_addr, exe=args.dfu_util,
                                   debug=args.verbose)

    def find_device(self):
        cmd = list(self.cmd) + ['-l']
        output = self.check_output(cmd)
        output = output.decode(sys.getdefaultencoding())
        return self.list_pattern in output

    def do_run(self, command, **kwargs):
        reset = 0
        if not self.find_device():
            reset = 1
            print('Please reset your board to switch to DFU mode...')
            while not self.find_device():
                time.sleep(0.1)

        cmd = list(self.cmd)
        if self.dfuse is not None:
            cmd.extend(['-s', '{}:leave'.format(self.dfuse)])
        cmd.extend(['-a', self.alt, '-D', self.img])
        self.check_call(cmd)
        if reset:
            print('Now reset your board again to switch back to runtime mode.')
