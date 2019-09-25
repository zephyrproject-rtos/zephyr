# Copyright (c) 2017 Linaro Limited.
# Copyright (c) 2019 Nordic Semiconductor ASA.
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for flashing with nrfjprog.'''

import os
import sys

from runners.core import ZephyrBinaryRunner, RunnerCaps


class NrfJprogBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for nrfjprog.'''

    def __init__(self, cfg, family, softreset, snr, erase=False):
        super(NrfJprogBinaryRunner, self).__init__(cfg)
        self.hex_ = cfg.hex_file
        self.family = family
        self.softreset = softreset
        self.snr = snr
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
                            choices=['NRF51', 'NRF52', 'NRF91'],
                            help='family of nRF MCU')
        parser.add_argument('--softreset', required=False,
                            action='store_true',
                            help='use reset instead of pinreset')
        parser.add_argument('--erase', action='store_true',
                            help='if given, mass erase flash before loading')
        parser.add_argument('--snr', required=False,
                            help='serial number of board to use')

    @classmethod
    def create(cls, cfg, args):
        ret = NrfJprogBinaryRunner(cfg, args.nrf_family, args.softreset,
                                   args.snr, erase=args.erase)
        ret.ensure_snr()
        return ret

    def ensure_snr(self):
        if not self.snr:
            self.snr = self.get_board_snr_from_user()

    def get_board_snr_from_user(self):
        snrs = self.check_output(['nrfjprog', '--ids'])
        snrs = snrs.decode(sys.getdefaultencoding()).strip().splitlines()

        if not snrs:
            raise RuntimeError('"nrfjprog --ids" did not find a board; '
                               'is the board connected?')

        if len(snrs) == 1:
            board_snr = snrs[0]
            if board_snr == '0':
                raise RuntimeError('"nrfjprog --ids" returned 0; '
                                   'is a debugger already connected?')
            return board_snr

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
        self.require('nrfjprog')

        commands = []
        if self.snr is None:
            raise ValueError("self.snr must not be None")
        else:
            board_snr = self.snr.lstrip("0")

        if not os.path.isfile(self.hex_):
            raise ValueError('Cannot flash; hex file ({}) does not exist. '.
                             format(self.hex_) +
                             'Try enabling CONFIG_BUILD_OUTPUT_HEX.')

        program_cmd = ['nrfjprog', '--program', self.hex_, '-f', self.family,
                       '--snr', board_snr]
        self.logger.info('Flashing file: {}'.format(self.hex_))
        if self.erase:
            commands.extend([
                ['nrfjprog',
                 '--eraseall',
                 '-f', self.family,
                 '--snr', board_snr],
                program_cmd
            ])
        else:
            if self.family == 'NRF52':
                commands.append(program_cmd + ['--sectoranduicrerase'])
            else:
                commands.append(program_cmd + ['--sectorerase'])

        if self.family == 'NRF52' and not self.softreset:
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

        self.logger.info('Board with serial number {} flashed successfully.'.
                         format(board_snr))
