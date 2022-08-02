# Copyright (c) 2017 Linaro Limited.
# Copyright (c) 2019 Nordic Semiconductor ASA.
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for flashing with nrfjprog.'''

from functools import partial
import os
from pathlib import Path
import shlex
import subprocess
import sys
from re import fullmatch, escape

from runners.core import ZephyrBinaryRunner, RunnerCaps, depr_action

try:
    from intelhex import IntelHex
except ImportError:
    IntelHex = None

# https://infocenter.nordicsemi.com/index.jsp?topic=%2Fug_nrf_cltools%2FUG%2Fcltools%2Fnrf_nrfjprogexe_return_codes.html&cp=9_1_3_1
UnavailableOperationBecauseProtectionError = 16

class NrfJprogBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for nrfjprog.'''

    def __init__(self, cfg, family, softreset, dev_id, erase=False,
                 tool_opt=[], force=False, recover=False):
        super().__init__(cfg)
        self.hex_ = cfg.hex_file
        self.family = family
        self.softreset = softreset
        self.dev_id = dev_id
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
        return RunnerCaps(commands={'flash'}, dev_id=True, erase=True,
                          tool_opt=True)

    @classmethod
    def dev_id_help(cls) -> str:
        return '''Device identifier. Use it to select the J-Link Serial Number
                  of the device connected over USB. '*' matches one or more
                  characters/digits'''

    @classmethod
    def tool_opt_help(cls) -> str:
        return 'Additional options for nrfjprog, e.g. "--recover"'

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--nrf-family',
                            choices=['NRF51', 'NRF52', 'NRF53', 'NRF91'],
                            help='''MCU family; still accepted for
                            compatibility only''')
        parser.add_argument('--softreset', required=False,
                            action='store_true',
                            help='use reset instead of pinreset')
        parser.add_argument('--snr', required=False, dest='dev_id',
                            action=partial(depr_action,
                                           replacement='-i/--dev-id'),
                            help='Deprecated: use -i/--dev-id instead')
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
                                    args.dev_id, erase=args.erase,
                                    tool_opt=args.tool_opt, force=args.force,
                                    recover=args.recover)

    def ensure_snr(self):
        if not self.dev_id or "*" in self.dev_id:
            self.dev_id = self.get_board_snr(self.dev_id or "*")
        self.dev_id = self.dev_id.lstrip("0")

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

        if self.build_conf.getboolean('CONFIG_SOC_SERIES_NRF51X'):
            self.family = 'NRF51'
        elif self.build_conf.getboolean('CONFIG_SOC_SERIES_NRF52X'):
            self.family = 'NRF52'
        elif self.build_conf.getboolean('CONFIG_SOC_SERIES_NRF53X'):
            self.family = 'NRF53'
        elif self.build_conf.getboolean('CONFIG_SOC_SERIES_NRF91X'):
            self.family = 'NRF91'
        else:
            raise RuntimeError(f'unknown nRF; update {__file__}')

    def hex_refers_region(self, region_start, region_end):
        for segment_start, _ in self.hex_contents.segments():
            if region_start <= segment_start <= region_end:
                return True
        return False

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

        if self.uicr_data_ok or self.family not in uicr_ranges:
            return

        for region_start, region_end in uicr_ranges[self.family]:
            if self.hex_refers_region(region_start, region_end):
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
                             '--snr', self.dev_id])

        self.check_call(['nrfjprog', '--recover',  '-f', self.family,
                         '--snr', self.dev_id])

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

        xip_ranges = {
            'NRF52': (0x12000000, 0x19FFFFFF),
            'NRF53': (0x10000000, 0x1FFFFFFF),
        }
        qspi_erase_opt = []
        if self.family in xip_ranges:
            xip_start, xip_end = xip_ranges[self.family]
            if self.hex_refers_region(xip_start, xip_end):
                qspi_erase_opt = ['--qspisectorerase']

        # What nrfjprog commands do we need to flash this target?
        program_commands = []
        if self.family == 'NRF53':
            # nRF53 requires special treatment due to the extra coprocessor.
            self.program_hex_nrf53(erase_arg, qspi_erase_opt, program_commands)
        else:
            # It's important for tool_opt to come last, so it can override
            # any options that we set here.
            program_commands.append(['nrfjprog', '--program', self.hex_,
                                     erase_arg] +
                                    qspi_erase_opt +
                                    ['--verify', '-f', self.family,
                                     '--snr', self.dev_id] +
                                    self.tool_opt)

        try:
            for command in program_commands:
                self.check_call(command)
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

    def program_hex_nrf53(self, erase_arg, qspi_erase_opt, program_commands):
        # program_hex() helper for nRF53.

        # *********************** NOTE *******************************
        # self.hex_ can contain code for both the application core and
        # the network core.
        #
        # We can't assume, for example, that
        # CONFIG_SOC_NRF5340_CPUAPP=y means self.hex_ only contains
        # data for the app core's flash: the user can put arbitrary
        # addresses into one of the files in HEX_FILES_TO_MERGE.
        #
        # Therefore, on this family, we may need to generate two new
        # hex files, one for each core, and flash them individually
        # with the correct '--coprocessor' arguments.
        #
        # Kind of hacky, but it works, and nrfjprog is not capable of
        # flashing to both cores at once. If self.hex_ only affects
        # one core's flash, then we skip the extra work to save time.
        # ************************************************************

        def add_program_cmd(hex_file, coprocessor, qspi_erase_opt):
            program_commands.append(
                ['nrfjprog', '--program', hex_file, erase_arg] +
                qspi_erase_opt +
                ['--verify', '-f', 'NRF53', '--snr', self.dev_id,
                 '--coprocessor', coprocessor] +
                self.tool_opt)

        # Address range of the network coprocessor's flash. From nRF5340 OPS.
        # We should get this from DTS instead if multiple values are possible,
        # but this is fine for now.
        net_flash_start = 0x01000000
        net_flash_end   = 0x0103FFFF

        # If there is nothing in the hex file for the network core,
        # only the application core is programmed.
        if not self.hex_refers_region(net_flash_start, net_flash_end):
            add_program_cmd(self.hex_, 'CP_APPLICATION', qspi_erase_opt)
        # If there is some content that addresses a region beyond the network
        # core flash range, two hex files are generated and the two cores
        # are programmed one by one.
        elif self.hex_contents.minaddr() < net_flash_start or \
             self.hex_contents.maxaddr() > net_flash_end:

            net_hex, app_hex = IntelHex(), IntelHex()
            for start, end in self.hex_contents.segments():
                if net_flash_start <= start <= net_flash_end:
                    net_hex.merge(self.hex_contents[start:end])
                else:
                    app_hex.merge(self.hex_contents[start:end])

            hex_path = Path(self.hex_)
            hex_dir, hex_name = hex_path.parent, hex_path.name

            net_hex_file = os.fspath(
                hex_dir / f'GENERATED_CP_NETWORK_{hex_name}')
            app_hex_file = os.fspath(
                hex_dir / f'GENERATED_CP_APPLICATION_{hex_name}')

            self.logger.info(
                f'{self.hex_} targets both nRF53 coprocessors; '
                f'splitting it into: {net_hex_file} and {app_hex_file}')

            net_hex.write_hex_file(net_hex_file)
            app_hex.write_hex_file(app_hex_file)

            add_program_cmd(net_hex_file, 'CP_NETWORK', [])
            add_program_cmd(app_hex_file, 'CP_APPLICATION', qspi_erase_opt)
        # Otherwise, only the network core is programmed.
        else:
            add_program_cmd(self.hex_, 'CP_NETWORK', [])

    def reset_target(self):
        if self.family == 'NRF52' and not self.softreset:
            self.check_call(['nrfjprog', '--pinresetenable', '-f', self.family,
                             '--snr', self.dev_id])  # Enable pin reset

        if self.softreset:
            self.check_call(['nrfjprog', '--reset', '-f', self.family,
                             '--snr', self.dev_id])
        else:
            self.check_call(['nrfjprog', '--pinreset', '-f', self.family,
                             '--snr', self.dev_id])

    def do_run(self, command, **kwargs):
        self.require('nrfjprog')

        self.ensure_output('hex')
        if IntelHex is None:
            raise RuntimeError('one or more Python dependencies were missing; '
                               'see the getting started guide for details on '
                               'how to fix')
        self.hex_contents = IntelHex()
        try:
            self.hex_contents.loadfile(self.hex_, format='hex')
        except FileNotFoundError:
            pass

        self.ensure_snr()
        self.ensure_family()
        self.check_force_uicr()

        if self.recover:
            self.recover_target()
        self.program_hex()
        self.reset_target()

        self.logger.info(f'Board with serial number {self.dev_id} '
                         'flashed successfully.')
