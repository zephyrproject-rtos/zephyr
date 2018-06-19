# Copyright (c) 2017 Linaro Limited.
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for flashing with nrfjprog.'''

import sys

from .. import log
from .core import ZephyrBinaryRunner, RunnerCaps


class NrfJprogBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for nrfjprog.'''

    def __init__(self, cfg, family, softreset, snr):
        super(NrfJprogBinaryRunner, self).__init__(cfg)
        self.hex_ = cfg.kernel_hex
        self.family = family
        self.softreset = softreset
        self.snr = snr

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
        parser.add_argument('--snr', required=False,
                            help='serial number of board to use')

    @classmethod
    def create(cls, cfg, args):
        return NrfJprogBinaryRunner(cfg, args.nrf_family, args.softreset, args.snr)

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

        log.dbg("Refusing the temptation to guess a board",
                level=log.VERBOSE_EXTREME)

        # Use of print() here is advised. We don't want to lose
        # this information in a separate log -- this is
        # interactive and requires a terminal.
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
        if (self.snr is None):
            board_snr = self.get_board_snr_from_user()
        else:
            board_snr = self.snr.lstrip("0")

        print('Flashing file: {}'.format(self.hex_))
        commands = [
            ['nrfjprog', '--eraseall', '-f', self.family, '--snr', board_snr],
            ['nrfjprog', '--program', self.hex_, '-f', self.family, '--snr',
             board_snr],
        ]
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

        log.inf('Board with serial number {} flashed successfully.'.format(
                  board_snr))
