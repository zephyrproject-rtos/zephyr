# Copyright (c) 2017 Linaro Limited.
# Copyright (c) 2019 Nordic Semiconductor ASA.
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for flashing with nrfjprog.'''

import shlex
import subprocess
import sys
from re import fullmatch, escape

from runners.core import ZephyrBinaryRunner, RunnerCaps, BuildConfiguration

try:
    from intelhex import IntelHex
except ImportError:
    IntelHex = None

# Helper function for inspecting hex files.
# has_region returns True if hex file has any contents in a specific region
# region_filter is a callable that takes an address as argument and
# returns True if that address is in the region in question
def has_region(regions, hex_file):
    if IntelHex is None:
        raise RuntimeError('one or more Python dependencies were missing; '
                           "see the getting started guide for details on "
                           "how to fix")

    try:
        ih = IntelHex(hex_file)
        return any((len(ih[rs:re]) > 0) for (rs, re) in regions)
    except FileNotFoundError:
        return False

# https://infocenter.nordicsemi.com/index.jsp?topic=%2Fug_nrf_cltools%2FUG%2Fcltools%2Fnrf_nrfjprogexe_return_codes.html&cp=9_1_3_1
UnavailableOperationBecauseProtectionError = 16

class NrfJprogBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for nrfjprog.'''

    def __init__(self, cfg, family, softreset, snr, erase=False,
                 tool_opt=[], force=False, recover=False):
        super().__init__(cfg)
        self.hex_ = cfg.hex_file
        self.family = family
        self.softreset = softreset
        self.snr = snr
        self.erase = bool(erase)
        self.force = force
        self.recover = bool(recover)

        self.tool_opt = []
        for opts in [shlex.split(opt) for opt in tool_opt]:
            self.tool_opt += opts

    @classmethod
    def name(cls):
        return 'nrfjprog'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'}, erase=True)

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--nrf-family',
                            choices=['NRF51', 'NRF52', 'NRF53', 'NRF91'],
                            help='''MCU family; still accepted for
                            compatibility only''')
        parser.add_argument('--softreset', required=False,
                            action='store_true',
                            help='use reset instead of pinreset')
        parser.add_argument('--snr', required=False,
                            help="""Serial number of board to use.
                            '*' matches one or more characters/digits.""")
        parser.add_argument('--tool-opt', default=[], action='append',
                            help='''Additional options for nrfjprog,
                            e.g. "--recover"''')
        parser.add_argument('--force', required=False,
                            action='store_true',
                            help='Flash even if the result cannot be guaranteed.')
        parser.add_argument('--recover', required=False,
                            action='store_true',
                            help='''erase all user available non-volatile
                            memory and disable read back protection before
                            flashing (erases flash for both cores on nRF53)''')

    @classmethod
    def do_create(cls, cfg, args):
        return NrfJprogBinaryRunner(cfg, args.nrf_family, args.softreset,
                                    args.snr, erase=args.erase,
                                    tool_opt=args.tool_opt, force=args.force,
                                    recover=args.recover)

    def ensure_snr(self):
        if not self.snr or "*" in self.snr:
            self.snr = self.get_board_snr(self.snr or "*")
        self.snr = self.snr.lstrip("0")

    def get_boards(self):
        snrs = self.check_output(['nrfjprog', '--ids'])
        snrs = snrs.decode(sys.getdefaultencoding()).strip().splitlines()
        if not snrs:
            raise RuntimeError('"nrfjprog --ids" did not find a board; '
                               'is the board connected?')
        return snrs

    @staticmethod
    def verify_snr(snr):
        if snr == '0':
            raise RuntimeError('"nrfjprog --ids" returned 0; '
                                'is a debugger already connected?')

    def get_board_snr(self, glob):
        # Use nrfjprog --ids to discover connected boards.
        #
        # If there's exactly one board connected, it's safe to assume
        # the user wants that one. Otherwise, bail unless there are
        # multiple boards and we are connected to a terminal, in which
        # case use print() and input() to ask what the user wants.

        re_glob = escape(glob).replace(r"\*", ".+")
        snrs = [snr for snr in self.get_boards() if fullmatch(re_glob, snr)]

        if len(snrs) == 0:
            raise RuntimeError(
                'There are no boards connected{}.'.format(
                        f" matching '{glob}'" if glob != "*" else ""))
        elif len(snrs) == 1:
            board_snr = snrs[0]
            self.verify_snr(board_snr)
            print("Using board {}".format(board_snr))
            return board_snr
        elif not sys.stdin.isatty():
            raise RuntimeError(
                f'refusing to guess which of {len(snrs)} '
                'connected boards to use. (Interactive prompts '
                'disabled since standard input is not a terminal.) '
                'Please specify a serial number on the command line.')

        snrs = sorted(snrs)
        print('There are multiple boards connected{}.'.format(
                        f" matching '{glob}'" if glob != "*" else ""))
        for i, snr in enumerate(snrs, 1):
            print('{}. {}'.format(i, snr))

        p = 'Please select one with desired serial number (1-{}): '.format(
                len(snrs))
        while True:
            try:
                value = input(p)
            except EOFError:
                sys.exit(0)
            try:
                value = int(value)
            except ValueError:
                continue
            if 1 <= value <= len(snrs):
                break

        return snrs[value - 1]

    def ensure_family(self):
        # Ensure self.family is set.

        if self.family is not None:
            return

        if self.build_conf.get('CONFIG_SOC_SERIES_NRF51X', False):
            self.family = 'NRF51'
        elif self.build_conf.get('CONFIG_SOC_SERIES_NRF52X', False):
            self.family = 'NRF52'
        elif self.build_conf.get('CONFIG_SOC_SERIES_NRF53X', False):
            self.family = 'NRF53'
        elif self.build_conf.get('CONFIG_SOC_SERIES_NRF91X', False):
            self.family = 'NRF91'
        else:
            raise RuntimeError(f'unknown nRF; update {__file__}')

    def check_force_uicr(self):
        # On SoCs without --sectoranduicrerase, we want to fail by
        # default if the application contains UICR data and we're not sure
        # that the flash will succeed.

        # A map from SoCs which need this check to their UICR address
        # ranges. If self.family isn't in here, do nothing.
        uicr_ranges = {
            'NRF53': ((0x00FF8000, 0x00FF8800),
                      (0x01FF8000, 0x01FF8800)),
            'NRF91': ((0x00FF8000, 0x00FF8800),),
        }

        if self.family not in uicr_ranges:
            return

        uicr = uicr_ranges[self.family]

        if not self.uicr_data_ok and has_region(uicr, self.hex_):
            # Hex file has UICR contents, and that's not OK.
            raise RuntimeError(
                'The hex file contains data placed in the UICR, which '
                'needs a full erase before reprogramming. Run west '
                'flash again with --force, --erase, or --recover.')

    @property
    def uicr_data_ok(self):
        # True if it's OK to try to flash even with UICR data
        # in the image; False otherwise.

        return self.force or self.erase or self.recover

    def recover_target(self):
        if self.family == 'NRF53':
            self.logger.info(
                'Recovering and erasing flash memory for both the network '
                'and application cores.')
        else:
            self.logger.info('Recovering and erasing all flash memory.')

        if self.family == 'NRF53':
            self.check_call(['nrfjprog', '--recover', '-f', self.family,
                             '--coprocessor', 'CP_NETWORK',
                             '--snr', self.snr])

        self.check_call(['nrfjprog', '--recover',  '-f', self.family,
                         '--snr', self.snr])

    def program_hex(self):
        # Get the nrfjprog command use to actually program self.hex_.
        self.logger.info('Flashing file: {}'.format(self.hex_))

        # What type of erase argument should we pass to nrfjprog?
        if self.erase:
            erase_arg = '--chiperase'
        else:
            if self.family == 'NRF52':
                erase_arg = '--sectoranduicrerase'
            else:
                erase_arg = '--sectorerase'

        if self.family == 'NRF53':
            if self.build_conf.get('CONFIG_SOC_NRF5340_CPUAPP', False):
                coprocessor = 'CP_APPLICATION'
            elif self.build_conf.get('CONFIG_SOC_NRF5340_CPUNET', False):
                coprocessor = 'CP_NETWORK'
            else:
                # When it's time to update this file, it would probably be best
                # to handle this by adding common 'SOC_NRF53X_CPUAPP'
                # and 'SOC_NRF53X_CPUNET' options, so we don't have to
                # maintain a list of SoCs in this file too.
                raise RuntimeError(f'unknown nRF53; update {__file__}')
            coprocessor_args = ['--coprocessor', coprocessor]
        else:
            coprocessor_args = []

        # It's important for tool_opt to come last, so it can override
        # any options that we set here.
        try:
            self.check_call(['nrfjprog', '--program', self.hex_, erase_arg,
                             '-f', self.family, '--snr', self.snr] +
                            coprocessor_args + self.tool_opt)
        except subprocess.CalledProcessError as cpe:
            if cpe.returncode == UnavailableOperationBecauseProtectionError:
                if self.family == 'NRF53':
                    family_help = (
                        '  Note: your target is an nRF53; all flash memory '
                        'for both the network and application cores will be '
                        'erased prior to reflashing.')
                else:
                    family_help = (
                        '  Note: this will recover and erase all flash memory '
                        'prior to reflashing.')
                self.logger.error(
                    'Flashing failed because the target '
                    'must be recovered.\n'
                    '  To fix, run "west flash --recover" instead.\n' +
                    family_help)
            raise

    def reset_target(self):
        if self.family == 'NRF52' and not self.softreset:
            self.check_call(['nrfjprog', '--pinresetenable', '-f', self.family,
                             '--snr', self.snr])  # Enable pin reset

        if self.softreset:
            self.check_call(['nrfjprog', '--reset', '-f', self.family,
                             '--snr', self.snr])
        else:
            self.check_call(['nrfjprog', '--pinreset', '-f', self.family,
                             '--snr', self.snr])

    def do_run(self, command, **kwargs):
        self.require('nrfjprog')
        self.build_conf = BuildConfiguration(self.cfg.build_dir)

        self.ensure_output('hex')
        self.ensure_snr()
        self.ensure_family()
        self.check_force_uicr()

        if self.recover:
            self.recover_target()
        self.program_hex()
        self.reset_target()

        self.logger.info(f'Board with serial number {self.snr} '
                         'flashed successfully.')
