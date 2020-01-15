# Copyright (c) 2017 Linaro Limited.
# Copyright (c) 2019 Nordic Semiconductor ASA.
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for flashing with nrfjprog.'''

import os
import shlex
import sys
from intelhex import IntelHex

from runners.core import ZephyrBinaryRunner, RunnerCaps


class NrfJprogBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for nrfjprog.'''

    def __init__(self, cfg, family, softreset, snr, erase=False,
        tool_opt=[]):
        super(NrfJprogBinaryRunner, self).__init__(cfg)
        self.hex_ = cfg.hex_file
        self.family = family
        self.softreset = softreset
        self.snr = snr
        self.erase = erase

        self.tool_opt = []
        for opts in [shlex.split(opt) for opt in tool_opt]:
            self.tool_opt += opts

    @classmethod
    def name(cls):
        return 'nrfjprog'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'})

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--nrf-family', required=True,
                            choices=['NRF51', 'NRF52', 'NRF53', 'NRF91'],
                            help='family of nRF MCU')
        parser.add_argument('--softreset', required=False,
                            action='store_true',
                            help='use reset instead of pinreset')
        parser.add_argument('--erase', action='store_true',
                            help='if given, mass erase flash before loading')
        parser.add_argument('--snr', required=False,
                            help='serial number of board to use')
        parser.add_argument('--tool-opt', default=[], action='append',
                            help='''Additional options for nrfjprog,
                            e.g. "--recover"''')

    @classmethod
    def create(cls, cfg, args):
        ret = NrfJprogBinaryRunner(cfg, args.nrf_family, args.softreset,
                                   args.snr, erase=args.erase,
                                   tool_opt=args.tool_opt)
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

        if self.family == 'NRF53':
            # self.hex_ can contain code for both application core and network
            # core. Code being flashed to the network core needs to have a
            # specific argument provided to nrfjprog. If network code is found,
            # generate two new hex files, one for each core, and flash them
            # with the correct '--coprocessor' argument.
            ih = IntelHex(self.hex_)
            net_hex = IntelHex()
            app_hex = IntelHex()
            for s in ih.segments():
                if s[0] >= 0x01000000:
                    net_hex.merge(ih[s[0]:s[1]])
                else:
                    app_hex.merge(ih[s[0]:s[1]])

            if len(net_hex) > 0:
                wd = os.path.dirname(self.hex_)
                net_hex_file = os.path.join(wd, 'GENERATED_CP_NETWORK_'
                                            + os.path.basename(self.hex_))
                self.logger.info("Generating CP_NETWORK hex file {}"
                                 .format(net_hex_file))
                net_hex.write_hex_file(net_hex_file)

                program_net_cmd = ['nrfjprog', '--coprocessor', 'CP_NETWORK',
                                   '--program', net_hex_file, '-f',
                                   self.family, '--snr', board_snr,
                                   '--sectorerase']
                commands.extend([program_net_cmd])
                self.logger.info('Flashing file: {}'.format(net_hex_file))

                if len(app_hex) > 0:
                    app_hex_file = os.path.join(wd, 'GENERATED_CP_APPLICATION_'
                                                + os.path.basename(self.hex_))
                    self.logger.info("Generating CP_APPLICATION hex file {}"
                                     .format(net_hex_file))
                    app_hex.write_hex_file(app_hex_file)
                    self.hex_ = app_hex_file

        program_cmd = ['nrfjprog', '--program', self.hex_, '-f', self.family,
                       '--snr', board_snr] + self.tool_opt

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
