# Copyright (c) 2017 Linaro Limited.
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for flashing with dfu-util.'''

import os
import sys
import time

from .core import ZephyrBinaryRunner, get_env_or_bail


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
    def handles_command(cls, command):
        return command == 'flash'

    def create_from_env(command, debug):
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

        return DfuUtilBinaryRunner(pid, alt, img, dfuse=dfuse, exe=exe,
                                   debug=debug)

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
