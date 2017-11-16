# Copyright (c) 2017 Linaro Limited.
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for flashing with nrfjprog.'''

import sys

from .core import ZephyrBinaryRunner, RunnerCaps


class NrfJprogBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for nrfjprog.'''

    def __init__(self, hex_, family, debug=False):
        super(NrfJprogBinaryRunner, self).__init__(debug=debug)
        self.hex_ = hex_
        self.family = family

    @classmethod
    def name(cls):
        return 'nrfjprog'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'})

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--nrf-family', required=True,
                            choices=['NRF51', 'NRF52'],
                            help='family of nRF MCU')

    @classmethod
    def create_from_args(cls, args):
        return NrfJprogBinaryRunner(args.kernel_hex, args.nrf_family,
                                    debug=args.verbose)

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

    def do_run(self, command, **kwargs):
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

        print('Board with serial number {} flashed successfully.'.format(
                  board_snr))
