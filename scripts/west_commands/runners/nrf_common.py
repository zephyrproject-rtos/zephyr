# Copyright (c) 2017 Linaro Limited.
# Copyright (c) 2023 Nordic Semiconductor ASA.
#
# SPDX-License-Identifier: Apache-2.0

'''Runner base class for flashing with nrf tools.'''

import abc
from collections import deque
import os
from pathlib import Path
import shlex
import subprocess
import sys
from re import fullmatch, escape

from runners.core import ZephyrBinaryRunner, RunnerCaps

try:
    from intelhex import IntelHex
except ImportError:
    IntelHex = None

ErrNotAvailableBecauseProtection = 24
ErrVerify = 25

class NrfBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end base class for nrf tools.'''

    def __init__(self, cfg, family, softreset, dev_id, erase=False,
                 reset=True, tool_opt=[], force=False, recover=False):
        super().__init__(cfg)
        self.hex_ = cfg.hex_file
        if family and not family.endswith('_FAMILY'):
            family = f'{family}_FAMILY'
        self.family = family
        self.softreset = softreset
        self.dev_id = dev_id
        self.erase = bool(erase)
        self.reset = bool(reset)
        self.force = force
        self.recover = bool(recover)

        self.tool_opt = []
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
                            choices=['NRF51', 'NRF52', 'NRF53', 'NRF91'],
                            help='''MCU family; still accepted for
                            compatibility only''')
        parser.add_argument('--softreset', required=False,
                            action='store_true',
                            help='use reset instead of pinreset')
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
            self.family = 'NRF51_FAMILY'
        elif self.build_conf.getboolean('CONFIG_SOC_SERIES_NRF52X'):
            self.family = 'NRF52_FAMILY'
        elif self.build_conf.getboolean('CONFIG_SOC_SERIES_NRF53X'):
            self.family = 'NRF53_FAMILY'
        elif self.build_conf.getboolean('CONFIG_SOC_SERIES_NRF91X'):
            self.family = 'NRF91_FAMILY'
        else:
            raise RuntimeError(f'unknown nRF; update {__file__}')

    def hex_refers_region(self, region_start, region_end):
        for segment_start, _ in self.hex_contents.segments():
            if region_start <= segment_start <= region_end:
                return True
        return False

    def hex_has_uicr_content(self):
        # A map from SoCs which need this check to their UICR address
        # ranges. If self.family isn't in here, do nothing.
        uicr_ranges = {
            'NRF53': ((0x00FF8000, 0x00FF8800),
                      (0x01FF8000, 0x01FF8800)),
            'NRF91': ((0x00FF8000, 0x00FF8800),),
        }

        if self.family not in uicr_ranges:
            return

        for region_start, region_end in uicr_ranges[self.family]:
            if self.hex_refers_region(region_start, region_end):
                return True

    def flush(self, force=False):
        try:
            self.flush_ops(force=force)
        except subprocess.CalledProcessError as cpe:
            if cpe.returncode == ErrNotAvailableBecauseProtection:
                if self.family == 'NRF53_FAMILY':
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
            if cpe.returncode == ErrVerify:
                # If there are data in  the UICR region it is likely that the
                # verify failed du to the UICR not been erased before, so giving
                # a warning here will hopefully enhance UX.
                if self.hex_has_uicr_content():
                    self.logger.warning(
                        'The hex file contains data placed in the UICR, which '
                        'may require a full erase before reprogramming. Run '
                        'west flash again with --erase, or --recover.')
            raise


    def recover_target(self):
        if self.family == 'NRF53_FAMILY':
            self.logger.info(
                'Recovering and erasing flash memory for both the network '
                'and application cores.')
        else:
            self.logger.info('Recovering and erasing all flash memory.')

        # The network core needs to be recovered first due to the fact that
        # recovering it erases the flash of *both* cores. Since a recover
        # operation unlocks the core and then flashes a small image that keeps
        # the debug access port open, recovering the network core last would
        # result in that small image being deleted from the app core.
        if self.family == 'NRF53_FAMILY':
            self.exec_op('recover', core='NRFDL_DEVICE_CORE_NETWORK')

        self.exec_op('recover')

    def program_hex(self):
        # Get the command use to actually program self.hex_.
        self.logger.info('Flashing file: {}'.format(self.hex_))

        # What type of erase argument should we pass to the tool?
        if self.erase:
            erase_arg = 'ERASE_ALL'
        else:
            if self.family == 'NRF52_FAMILY':
                erase_arg = 'ERASE_PAGES_INCLUDING_UICR'
            else:
                erase_arg = 'ERASE_PAGES'

        xip_ranges = {
            'NRF52_FAMILY': (0x12000000, 0x19FFFFFF),
            'NRF53_FAMILY': (0x10000000, 0x1FFFFFFF),
        }
        qspi_erase_opt = None
        if self.family in xip_ranges:
            xip_start, xip_end = xip_ranges[self.family]
            if self.hex_refers_region(xip_start, xip_end):
                qspi_erase_opt = 'ERASE_ALL'

        # What tool commands do we need to flash this target?
        if self.family == 'NRF53_FAMILY':
            # nRF53 requires special treatment due to the extra coprocessor.
            self.program_hex_nrf53(erase_arg, qspi_erase_opt)
        else:
            self.op_program(self.hex_, erase_arg, qspi_erase_opt, defer=True)

        self.flush(force=False)

    def program_hex_nrf53(self, erase_arg, qspi_erase_opt):
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
        # Kind of hacky, but it works, and the tools are not capable of
        # flashing to both cores at once. If self.hex_ only affects
        # one core's flash, then we skip the extra work to save time.
        # ************************************************************

        # Address range of the network coprocessor's flash. From nRF5340 OPS.
        # We should get this from DTS instead if multiple values are possible,
        # but this is fine for now.
        net_flash_start = 0x01000000
        net_flash_end   = 0x0103FFFF

        # If there is nothing in the hex file for the network core,
        # only the application core is programmed.
        if not self.hex_refers_region(net_flash_start, net_flash_end):
            self.op_program(self.hex_, erase_arg, qspi_erase_opt, defer=True,
                            core='NRFDL_DEVICE_CORE_APPLICATION')
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

            self.op_program(net_hex_file, erase_arg, None, defer=True,
                            core='NRFDL_DEVICE_CORE_NETWORK')
            self.op_program(app_hex_file, erase_arg, qspi_erase_opt, defer=True,
                            core='NRFDL_DEVICE_CORE_APPLICATION')
        # Otherwise, only the network core is programmed.
        else:
            self.op_program(self.hex_, erase_arg, None, defer=True,
                            core='NRFDL_DEVICE_CORE_NETWORK')

    def reset_target(self):
        if self.family == 'NRF52_FAMILY' and not self.softreset:
            self.exec_op('pinreset-enable')

        if self.softreset:
            self.exec_op('reset', option="RESET_SYSTEM")
        else:
            self.exec_op('reset', option="RESET_PIN")

    @abc.abstractmethod
    def do_require(self):
        ''' Ensure the tool is installed '''

    def op_program(self, hex_file, erase, qspi_erase, defer=False, core=None):
        args = {'firmware': {'file': hex_file, 'format': 'NRFDL_FW_INTEL_HEX'},
                'chip_erase_mode': erase, 'verify': 'VERIFY_READ'}
        if qspi_erase:
            args['qspi_erase_mode'] = qspi_erase
        self.exec_op('program', defer, core, **args)

    def exec_op(self, op, defer=False, core=None, **kwargs):
        _op = f'{op}'
        op = {'operation': {'type': _op}}
        if core:
            op['core'] = core
        op['operation'].update(kwargs)
        self.logger.debug(f'defer: {defer} op: {op}')
        if defer or not self.do_exec_op(op, force=False):
            self.ops.append(op)

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
