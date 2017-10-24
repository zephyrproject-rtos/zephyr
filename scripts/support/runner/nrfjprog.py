# Copyright (c) 2017 Linaro Limited.
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for flashing with nrfjprog.'''

from os import path
import sys

from .core import ZephyrBinaryRunner, get_env_or_bail


class NrfJprogBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for nrfjprog.'''

    def __init__(self, hex_, family, board, debug=False):
        super(NrfJprogBinaryRunner, self).__init__(debug=debug)
        self.hex_ = hex_
        self.family = family
        self.board = board

    def replaces_shell_script(shell_script, command):
        return command == 'flash' and shell_script == 'nrf_flash.sh'

    def create_from_env(command, debug):
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

        return NrfJprogBinaryRunner(hex_, family, board, debug=debug)

    def get_board_snr_from_user(self):
        snrs = self.check_output(['nrfjprog', '--ids'])
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

    def run(self, command, **kwargs):
        if command != 'flash':
            raise ValueError('only flash is supported')

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
        commands.append(['nrfjprog',
                         '--pinreset',
                         '-f', self.family,
                         '--snr', board_snr])

        for cmd in commands:
            self.check_call(cmd)

        print('{} Serial Number {} flashed with success.'.format(
                  self.board, board_snr))
