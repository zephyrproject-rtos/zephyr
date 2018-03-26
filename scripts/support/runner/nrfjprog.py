# Copyright (c) 2017 Linaro Limited.
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for flashing with nrfjprog.'''

import sys

from .core import ZephyrBinaryRunner, RunnerCaps


class NrfJprogBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for nrfjprog.'''

    def __init__(self, hex_, family, softreset, erase=False, debug=False):
        super(NrfJprogBinaryRunner, self).__init__(debug=debug)
        self.hex_ = hex_
        self.family = family
        self.softreset = softreset
        self.erase = erase

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
        parser.add_argument('--softreset', required=False,
                            action='store_true',
                            help='use reset instead of pinreset')
        parser.add_argument('--erase', action='store_true',
                            help='if given, mass erase flash before loading')

    @classmethod
    def create_from_args(cls, args):
        return NrfJprogBinaryRunner(args.kernel_hex, args.nrf_family,
                                    args.softreset, erase=args.erase,
                                    debug=args.verbose)

    def get_board_snr_from_user(self):
        snrs = self.check_output(['nrfjprog', '--ids'])
        snrs = snrs.decode(sys.getdefaultencoding()).strip().splitlines()

        if len(snrs) == 0:
            raise RuntimeError('"nrfjprog --ids" did not find a board; Is the board connected?')
        elif len(snrs) == 1:
            board_snr = snrs[0]
            if board_snr == '0':
                raise RuntimeError('"nrfjprog --ids" returned 0; is a debugger already connected?')
            return board_snr

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
        commands = []
        board_snr = self.get_board_snr_from_user()
        program_cmd = ['nrfjprog', '--program', self.hex_, '-f', self.family,
                       '--snr', board_snr]

        print('Flashing file: {}'.format(self.hex_))

        if self.erase:
            commands.extend([
                ['nrfjprog', '--eraseall', '-f', self.family,
                 '--snr', board_snr],
                program_cmd
                ])
        else:
            if self.family == 'NRF51':
                commands.append(program_cmd + ['--sectorerase'])
            else:
                commands.append(program_cmd + ['--sectoranduicrerase'])

        if self.family == 'NRF52' and self.softreset == False:
            commands.extend([
                # Enable pin reset
                ['nrfjprog', '--pinresetenable', '-f', self.family,
                 '--snr', board_snr],
            ])

        if self.softreset:
            commands.append(['nrfjprog', '--reset', '-f', self.family,
                             '--snr', board_snr])
        else:
            commands.append(['nrfjprog', '--pinreset', '-f', self.family,
                             '--snr', board_snr])

        for cmd in commands:
            self.check_call(cmd)

        print('Board with serial number {} flashed successfully.'.format(
                  board_snr))
