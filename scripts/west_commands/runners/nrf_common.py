# Copyright (c) 2017 Linaro Limited.
# Copyright (c) 2023 Nordic Semiconductor ASA.
#
# SPDX-License-Identifier: Apache-2.0

'''Runner base class for flashing with nrf tools.'''

import abc
import contextlib
import functools
import os
import shlex
import subprocess
import sys
from collections import deque
from pathlib import Path
from re import escape, fullmatch

from zephyr_ext_common import ZEPHYR_BASE

sys.path.append(os.fspath(Path(__file__).parent.parent.parent))
import zephyr_module

from runners.core import RunnerCaps, ZephyrBinaryRunner

try:
    from intelhex import IntelHex
except ImportError:
    IntelHex = None

ErrNotAvailableBecauseProtection = 24
ErrVerify = 25

UICR_RANGES = {
    'nrf53': {
        'Application': (0x00FF8000, 0x00FF8800),
        'Network': (0x01FF8000, 0x01FF8800),
    },
    'nrf54h': {
        'Application': (0x0FFF8000, 0x0FFF8800),
        'Network': (0x0FFFA000, 0x0FFFA800),
    },
    'nrf54l': {
        'Application': (0x00FFD000, 0x00FFDA00),
    },
    'nrf91': {
        'Application': (0x00FF8000, 0x00FF8800),
    },
    'nrf92': {
        'Application': (0x0FFF8000, 0x0FFF8800),
        'Network': (0x0FFFA000, 0x0FFFA800),
    },
}

# Relative to the root of the hal_nordic module
SUIT_STARTER_PATH = Path('zephyr/blobs/suit/bin/suit_manifest_starter.hex')

@functools.cache
def _get_suit_starter():
    path = None
    modules = zephyr_module.parse_modules(ZEPHYR_BASE)
    for m in modules:
        if 'hal_nordic' in m.meta.get('name'):
            path = Path(m.project)
            break

    if not path:
        raise RuntimeError("hal_nordic project missing in the manifest")

    suit_starter = path / SUIT_STARTER_PATH
    if not suit_starter.exists():
        raise RuntimeError("Unable to find suit manifest starter file, "
                           "please make sure to run \'west blobs fetch "
                           "hal_nordic\'")

    return str(suit_starter.resolve())

class NrfBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end base class for nrf tools.'''

    def __init__(self, cfg, family, softreset, pinreset, dev_id, erase=False,
                 reset=True, tool_opt=None, force=False, recover=False):
        super().__init__(cfg)
        self.hex_ = cfg.hex_file
        # The old --nrf-family options takes upper-case family names
        self.family = family.lower() if family else None
        self.softreset = softreset
        self.pinreset = pinreset
        self.dev_id = dev_id
        self.erase = bool(erase)
        self.reset = bool(reset)
        self.force = force
        self.recover = bool(recover)

        # Only applicable for nrfutil
        self.suit_starter = False

        self.tool_opt = []
        if tool_opt is not None:
            for opts in [shlex.split(opt) for opt in tool_opt]:
                self.tool_opt += opts

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'}, dev_id=True, erase=True,
                          reset=True, tool_opt=True)

    @classmethod
    def dev_id_help(cls) -> str:
        return '''Device identifier. Use it to select the J-Link Serial Number
                  of the device connected over USB. '*' matches one or more
                  characters/digits'''

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--nrf-family',
                            choices=['NRF51', 'NRF52', 'NRF53', 'NRF54L',
                                     'NRF54H', 'NRF91', 'NRF92'],
                            help='''MCU family; still accepted for
                            compatibility only''')
        # Not using a mutual exclusive group for softreset and pinreset due to
        # the way dump_runner_option_help() works in run_common.py
        parser.add_argument('--softreset', required=False,
                            action='store_true',
                            help='use softreset instead of pinreset')
        parser.add_argument('--pinreset', required=False,
                            action='store_true',
                            help='use pinreset instead of softreset')
        parser.add_argument('--snr', required=False, dest='dev_id',
                            help='obsolete synonym for -i/--dev-id')
        parser.add_argument('--force', required=False,
                            action='store_true',
                            help='Flash even if the result cannot be guaranteed.')
        parser.add_argument('--recover', required=False,
                            action='store_true',
                            help='''erase all user available non-volatile
                            memory and disable read back protection before
                            flashing (erases flash for both cores on nRF53)''')

        parser.set_defaults(reset=True)

    @classmethod
    def args_from_previous_runner(cls, previous_runner, args):
        # Propagate the chosen device ID to next runner
        if args.dev_id is None:
            args.dev_id = previous_runner.dev_id

    def ensure_snr(self):
        if not self.dev_id or "*" in self.dev_id:
            self.dev_id = self.get_board_snr(self.dev_id or "*")
        self.dev_id = self.dev_id.lstrip("0")

    @abc.abstractmethod
    def do_get_boards(self):
        ''' Return an array of Segger SNRs '''

    def get_boards(self):
        snrs = self.do_get_boards()
        if not snrs:
            raise RuntimeError('Unable to find a board; '
                               'is the board connected?')
        return snrs

    @staticmethod
    def verify_snr(snr):
        if snr == '0':
            raise RuntimeError('The Segger SNR obtained is 0; '
                                'is a debugger already connected?')

    def get_board_snr(self, glob):
        # Use nrfjprog or nrfutil to discover connected boards.
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
            print(f"Using board {board_snr}")
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
            print(f'{i}. {snr}')

        p = f'Please select one with desired serial number (1-{len(snrs)}): '
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
            self.family = 'nrf51'
        elif self.build_conf.getboolean('CONFIG_SOC_SERIES_NRF52X'):
            self.family = 'nrf52'
        elif self.build_conf.getboolean('CONFIG_SOC_SERIES_NRF53X'):
            self.family = 'nrf53'
        elif self.build_conf.getboolean('CONFIG_SOC_SERIES_NRF54LX'):
            self.family = 'nrf54l'
        elif self.build_conf.getboolean('CONFIG_SOC_SERIES_NRF54HX'):
            self.family = 'nrf54h'
        elif self.build_conf.getboolean('CONFIG_SOC_SERIES_NRF91X'):
            self.family = 'nrf91'
        elif self.build_conf.getboolean('CONFIG_SOC_SERIES_NRF92X'):
            self.family = 'nrf92'
        else:
            raise RuntimeError(f'unknown nRF; update {__file__}')

    def hex_refers_region(self, region_start, region_end):
        for segment_start, _ in self.hex_contents.segments():
            if region_start <= segment_start <= region_end:
                return True
        return False

    def hex_get_uicrs(self):
        hex_uicrs = {}

        if self.family in UICR_RANGES:
            for uicr_core, uicr_range in UICR_RANGES[self.family].items():
                if self.hex_refers_region(*uicr_range):
                    hex_uicrs[uicr_core] = uicr_range

        return hex_uicrs

    def flush(self, force=False):
        try:
            self.flush_ops(force=force)
        except subprocess.CalledProcessError as cpe:
            if cpe.returncode == ErrNotAvailableBecauseProtection:
                if self.family == 'nrf53':
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
            if cpe.returncode == ErrVerify and self.hex_get_uicrs():
                # If there is data in the UICR region it is likely that the
                # verify failed due to the UICR not been erased before, so giving
                # a warning here will hopefully enhance UX.
                self.logger.warning(
                    'The hex file contains data placed in the UICR, which '
                    'may require a full erase before reprogramming. Run '
                    'west flash again with --erase, or --recover.'
                )
            raise


    def recover_target(self):
        if self.family in ('nrf53', 'nrf54h', 'nrf92'):
            self.logger.info(
                'Recovering and erasing flash memory for both the network '
                'and application cores.')
        else:
            self.logger.info('Recovering and erasing all flash memory.')

        # The network core of the nRF53 needs to be recovered first due to the
        # fact that recovering it erases the flash of *both* cores. Since a
        # recover operation unlocks the core and then flashes a small image that
        # keeps the debug access port open, recovering the network core last
        # would result in that small image being deleted from the app core.
        # In the case of the 54H, the order is indifferent.
        if self.family in ('nrf53', 'nrf54h', 'nrf92'):
            self.exec_op('recover', core='Network')

        self.exec_op('recover')

    def _get_core(self):
        if self.family in ('nrf54h', 'nrf92'):
            if (self.build_conf.getboolean('CONFIG_SOC_NRF54H20_CPUAPP') or
                self.build_conf.getboolean('CONFIG_SOC_NRF9280_CPUAPP')):
                return 'Application'
            if (self.build_conf.getboolean('CONFIG_SOC_NRF54H20_CPURAD') or
                self.build_conf.getboolean('CONFIG_SOC_NRF9280_CPURAD')):
                return 'Network'
            raise RuntimeError(f'Core not found for family: {self.family}')

        if self.family in ('nrf53'):
            if self.build_conf.getboolean('CONFIG_SOC_NRF5340_CPUAPP'):
                return 'Application'
            if self.build_conf.getboolean('CONFIG_SOC_NRF5340_CPUNET'):
                return 'Network'
            raise RuntimeError(f'Core not found for family: {self.family}')

        return None

    def program_hex(self):
        # Get the command use to actually program self.hex_.
        self.logger.info(f'Flashing file: {self.hex_}')

        # What type of erase/core arguments should we pass to the tool?
        core = self._get_core()

        if self.family in ('nrf54h', 'nrf92'):
            erase_arg = 'ERASE_NONE'

            generated_uicr = self.build_conf.getboolean('CONFIG_NRF_REGTOOL_GENERATE_UICR')

            if generated_uicr and not self.hex_get_uicrs().get(core):
                raise RuntimeError(
                    f"Expected a UICR to be contained in: {self.hex_}\n"
                    "Please ensure that the correct version of nrf-regtool is "
                    "installed, then run 'west build --cmake' to try again."
                )

            if self.erase:
                self.exec_op('erase', core='Application', kind='all')
                self.exec_op('erase', core='Network', kind='all')

            # Manage SUIT artifacts.
            # This logic should be executed only once per build.
            # Use sysbuild board qualifiers to select the context,
            # with which the artifacts will be programmed.
            if self.build_conf.get('CONFIG_BOARD_QUALIFIERS') == self.sysbuild_conf.get(
                'SB_CONFIG_BOARD_QUALIFIERS'
            ):
                mpi_hex_dir = Path(os.path.join(self.cfg.build_dir, 'zephyr'))

                # Handle Manifest Provisioning Information
                if self.sysbuild_conf.getboolean('SB_CONFIG_SUIT_MPI_GENERATE'):
                    app_mpi_hex_file = os.fspath(
                        mpi_hex_dir / self.sysbuild_conf.get('SB_CONFIG_SUIT_MPI_APP_AREA_PATH'))
                    rad_mpi_hex_file = os.fspath(
                        mpi_hex_dir / self.sysbuild_conf.get('SB_CONFIG_SUIT_MPI_RAD_AREA_PATH')
                    )
                    if os.path.exists(app_mpi_hex_file):
                        self.op_program(
                            app_mpi_hex_file,
                            'ERASE_NONE',
                            None,
                            defer=True,
                            core='Application',
                        )
                    if os.path.exists(rad_mpi_hex_file):
                        self.op_program(
                            rad_mpi_hex_file,
                            'ERASE_NONE',
                            None,
                            defer=True,
                            core='Network',
                        )

                # Handle SUIT root manifest if application manifests are not used.
                # If an application firmware is built, the root envelope is merged
                # with other application manifests as well as the output HEX file.
                if core != 'Application' and self.sysbuild_conf.get('SB_CONFIG_SUIT_ENVELOPE'):
                    app_root_envelope_hex_file = os.fspath(
                        mpi_hex_dir / 'suit_installed_envelopes_application_merged.hex'
                    )
                    if os.path.exists(app_root_envelope_hex_file):
                        self.op_program(
                            app_root_envelope_hex_file,
                            'ERASE_NONE',
                            None,
                            defer=True,
                            core='Application',
                        )

            if not self.erase and generated_uicr:
                self.exec_op('erase', core=core, kind='uicr')
        else:
            if self.erase:
                erase_arg = 'ERASE_ALL'
            else:
                erase_arg = 'ERASE_RANGES_TOUCHED_BY_FIRMWARE'

        xip_ranges = {
            'nrf52': (0x12000000, 0x19FFFFFF),
            'nrf53': (0x10000000, 0x1FFFFFFF),
        }
        ext_mem_erase_opt = None
        if self.family in xip_ranges:
            xip_start, xip_end = xip_ranges[self.family]
            if self.hex_refers_region(xip_start, xip_end):
                ext_mem_erase_opt = 'ERASE_ALL'

        self.op_program(self.hex_, erase_arg, ext_mem_erase_opt, defer=True, core=core)
        self.flush(force=False)


    def reset_target(self):
        # Default to soft reset on nRF52 only, because ICs in these series can
        # reconfigure the reset pin as a regular GPIO
        default = "RESET_SYSTEM" if self.family == 'nrf52' else "RESET_PIN"
        kind = ("RESET_SYSTEM" if self.softreset else "RESET_PIN" if
               self.pinreset else default)

        if self.family == 'nrf52' and kind == "RESET_PIN":
            # Write to the UICR enabling nRESET in the corresponding pin
            self.exec_op('pinreset-enable')

        self.logger.debug(f'Reset kind: {kind}')
        self.exec_op('reset', kind=kind)

    @abc.abstractmethod
    def do_require(self):
        ''' Ensure the tool is installed '''

    def _check_suit_starter(self, op):
        op = op['operation']
        if op['type'] not in ('erase', 'recover', 'program'):
            return None
        if op['type'] == 'program' and op['options']['chip_erase_mode'] != "ERASE_UICR":
            return None

        file = _get_suit_starter()
        self.logger.debug(f'suit starter: {file}')

        return file

    def op_program(self, hex_file, erase, ext_mem_erase, defer=False, core=None):
        args = self._op_program(hex_file, erase, ext_mem_erase)
        self.exec_op('program', defer, core, **args)

    def _op_program(self, hex_file, erase, ext_mem_erase):
        args = {'firmware': {'file': hex_file},
                'options': {'chip_erase_mode': erase, 'verify': 'VERIFY_READ'}}
        if ext_mem_erase:
            args['options']['ext_mem_erase_mode'] = ext_mem_erase

        return args

    def exec_op(self, op, defer=False, core=None, **kwargs):

        def _exec_op(op, defer=False, core=None, **kwargs):
            _op = f'{op}'
            op = {'operation': {'type': _op}}
            if core:
                op['core'] = core
            op['operation'].update(kwargs)
            self.logger.debug(f'defer: {defer} op: {op}')
            if defer or not self.do_exec_op(op, force=False):
                self.ops.append(op)
            return op

        _op = _exec_op(op, defer, core, **kwargs)
        # Check if the suit manifest starter needs programming
        if self.suit_starter and self.family == 'nrf54h':
            file = self._check_suit_starter(_op)
            if file:
                args = self._op_program(file, 'ERASE_NONE', None)
                _exec_op('program', defer, core, **args)

    @abc.abstractmethod
    def do_exec_op(self, op, force=False):
        ''' Execute an operation. Return True if executed, False if not.
            Throws subprocess.CalledProcessError with the appropriate
            returncode if a failure arises.'''

    def flush_ops(self, force=True):
        ''' Execute any remaining ops in the self.ops array.
            Throws subprocess.CalledProcessError with the appropriate
            returncode if a failure arises.
            Subclasses can override this method for special handling of
            queued ops.'''
        self.logger.debug('Flushing ops')
        while self.ops:
            self.do_exec_op(self.ops.popleft(), force)

    def do_run(self, command, **kwargs):
        self.do_require()

        if self.softreset and self.pinreset:
            raise RuntimeError('Options --softreset and --pinreset are mutually '
                               'exclusive.')

        self.ensure_output('hex')
        if IntelHex is None:
            raise RuntimeError('Python dependency intelhex was missing; '
                               'see the getting started guide for details on '
                               'how to fix')
        self.hex_contents = IntelHex()
        with contextlib.suppress(FileNotFoundError):
            self.hex_contents.loadfile(self.hex_, format='hex')

        self.ensure_snr()
        self.ensure_family()

        self.ops = deque()

        if self.recover:
            self.recover_target()
        self.program_hex()
        if self.reset:
            self.reset_target()
        # All done, now flush any outstanding ops
        self.flush(force=True)

        self.logger.info(f'Board with serial number {self.dev_id} '
                         'flashed successfully.')
