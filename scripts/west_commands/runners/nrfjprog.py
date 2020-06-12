# Copyright (c) 2017 Linaro Limited.
# Copyright (c) 2019 Nordic Semiconductor ASA.
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for flashing with nrfjprog.'''

import os
import shlex
import sys
try:
    from intelhex import IntelHex
except ImportError:
    IntelHex = None

from runners.core import ZephyrBinaryRunner, RunnerCaps

# Helper function for inspecting hex files.
# returns whether the hex file has any contents in a specific region
# region filter is a callable that takes an address as argument and
# returns True if that address is in the region in question
def has_region(region_filter, hex_file):
    if IntelHex is None:
        raise RuntimeError('intelhex missing; please "pip3 install intelhex"')
    return any(region_filter(addr) for addr in IntelHex(hex_file).addresses())


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
        return NrfJprogBinaryRunner(cfg, args.nrf_family, args.softreset,
                                    args.snr, erase=args.erase,
                                    tool_opt=args.tool_opt)

    def ensure_snr(self):
        if not self.snr:
            self.snr = self.get_board_snr()

    def get_board_snr(self):
        # Use nrfjprog --ids to discover connected boards.
        #
        # If there's exactly one board connected, it's safe to assume
        # the user wants that one. Otherwise, bail unless there are
        # multiple boards and we are connected to a terminal, in which
        # case use print() and input() to ask what the user wants.

        snrs = self.check_output(['nrfjprog', '--ids'])
        snrs = snrs.decode(sys.getdefaultencoding()).strip().splitlines()
        if not snrs:
            raise RuntimeError('"nrfjprog --ids" did not find a board; '
                               'is the board connected?')
        elif len(snrs) == 1:
            board_snr = snrs[0]
            if board_snr == '0':
                raise RuntimeError('"nrfjprog --ids" returned 0; '
                                   'is a debugger already connected?')
            return board_snr
        elif not sys.stdin.isatty():
            raise RuntimeError(
                f'refusing to guess which of {len(snrs)} '
                'connected boards to use. (Interactive prompts '
                'disabled since standard input is not a terminal.) '
                'Please specify a serial number on the command line.')

        snrs = sorted(snrs)
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

        self.ensure_snr()

        commands = []
        board_snr = self.snr.lstrip("0")

        if not os.path.isfile(self.hex_):
            raise ValueError('Cannot flash; hex file ({}) does not exist. '.
                             format(self.hex_) +
                             'Try enabling CONFIG_BUILD_OUTPUT_HEX.')

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
            is_uicr = {
                'NRF51': lambda addr: False,
                'NRF52': lambda addr: (0x10001000 <= addr < 0x10002000),
                'NRF53': lambda addr: (0x00FF8000 <= addr < 0x00FF9000) or (0x01FF8000 <= addr < 0x01FF8800),
                'NRF91': lambda addr: (0x00FF8000 <= addr < 0x00FF9000),
            }[self.family]

            if self.family == 'NRF52':
                commands.append(program_cmd + ['--sectoranduicrerase'])
            elif has_region(is_uicr, hex_file):
                # Hex file has UICR contents.
                commands.append(program_cmd + ['--chiperase'])
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
