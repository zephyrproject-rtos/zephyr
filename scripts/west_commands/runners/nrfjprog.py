# Copyright (c) 2017 Linaro Limited.
# Copyright (c) 2023 Nordic Semiconductor ASA.
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for flashing with nrfjprog.'''

import subprocess
import sys

from runners.nrf_common import ErrNotAvailableBecauseProtection, ErrVerify, NrfBinaryRunner

# https://infocenter.nordicsemi.com/index.jsp?topic=%2Fug_nrf_cltools%2FUG%2Fcltools%2Fnrf_nrfjprogexe_return_codes.html&cp=9_1_3_1
UnavailableOperationBecauseProtectionError = 16
VerifyError = 55

class NrfJprogBinaryRunner(NrfBinaryRunner):
    '''Runner front-end for nrfjprog.'''

    def __init__(self, cfg, family, softreset, pinreset, dev_id, erase=False,
                 erase_mode=None, ext_erase_mode=None, reset=True, tool_opt=None,
                 force=False, recover=False, qspi_ini=None):

        super().__init__(cfg, family, softreset, pinreset, dev_id, erase,
                         erase_mode, ext_erase_mode, reset, tool_opt, force,
                         recover)

        self.qspi_ini = qspi_ini

    @classmethod
    def name(cls):
        return 'nrfjprog'

    @classmethod
    def capabilities(cls):
        return NrfBinaryRunner._capabilities()

    @classmethod
    def dev_id_help(cls) -> str:
        return NrfBinaryRunner._dev_id_help()

    @classmethod
    def tool_opt_help(cls) -> str:
        return 'Additional options for nrfjprog, e.g. "--clockspeed"'

    @classmethod
    def do_create(cls, cfg, args):
        return NrfJprogBinaryRunner(cfg, args.nrf_family, args.softreset,
                                    args.pinreset, args.dev_id, erase=args.erase,
                                    erase_mode=args.erase_mode,
                                    ext_erase_mode=args.ext_erase_mode,
                                    reset=args.reset, tool_opt=args.tool_opt,
                                    force=args.force, recover=args.recover,
                                    qspi_ini=args.qspi_ini)
    @classmethod
    def do_add_parser(cls, parser):
        super().do_add_parser(parser)
        parser.add_argument('--qspiini', required=False, dest='qspi_ini',
                            help='path to an .ini file with qspi configuration')

    def do_get_boards(self):
        snrs = self.check_output(['nrfjprog', '--ids'])
        return snrs.decode(sys.getdefaultencoding()).strip().splitlines()

    def do_require(self):
        self.require('nrfjprog')

    def do_exec_op(self, op, force=False):
        self.logger.debug(f'Executing op: {op}')
        # Translate the op

        families = {'nrf51': 'NRF51', 'nrf52': 'NRF52',
                    'nrf53': 'NRF53', 'nrf54l': 'NRF54L',
                    'nrf91': 'NRF91'}
        cores = {'Application': 'CP_APPLICATION',
                 'Network': 'CP_NETWORK'}

        core_opt = ['--coprocessor', cores[op['core']]] \
                   if op.get('core') else []

        cmd = ['nrfjprog']
        _op = op['operation']
        op_type = _op['type']
        # options that are an empty dict must use "in" instead of get()
        if op_type == 'pinreset-enable':
            cmd.append('--pinresetenable')
        elif op_type == 'program':
            cmd.append('--program')
            cmd.append(_op['firmware']['file'])
            opts = _op['options']
            erase = opts['chip_erase_mode']
            if erase == 'ERASE_ALL':
                cmd.append('--chiperase')
            elif erase == 'ERASE_RANGES_TOUCHED_BY_FIRMWARE':
                if self.family == 'nrf52':
                    cmd.append('--sectoranduicrerase')
                else:
                    cmd.append('--sectorerase')
            elif erase == 'ERASE_NONE':
                pass
            else:
                raise RuntimeError(f'Invalid erase mode: {erase}')

            if opts.get('ext_mem_erase_mode'):
                if opts['ext_mem_erase_mode'] == 'ERASE_RANGES_TOUCHED_BY_FIRMWARE':
                    cmd.append('--qspisectorerase')
                elif opts['ext_mem_erase_mode'] == 'ERASE_ALL':
                    cmd.append('--qspichiperase')

            if opts.get('verify'):
                # In the future there might be multiple verify modes
                cmd.append('--verify')
            if self.qspi_ini:
                cmd.append('--qspiini')
                cmd.append(self.qspi_ini)
        elif op_type == 'recover':
            cmd.append('--recover')
        elif op_type == 'reset':
            if _op['kind'] == 'RESET_SYSTEM':
                cmd.append('--reset')
            if _op['kind'] == 'RESET_PIN':
                cmd.append('--pinreset')
        elif op_type == 'erase':
            cmd.append(f'--erase{_op["kind"]}')
        else:
            raise RuntimeError(f'Invalid operation: {op_type}')

        try:
            self.check_call(cmd + ['-f', families[self.family]] + core_opt +
                            ['--snr', self.dev_id] + self.tool_opt)
        except subprocess.CalledProcessError as cpe:
            # Translate error codes
            if cpe.returncode == UnavailableOperationBecauseProtectionError:
                cpe.returncode = ErrNotAvailableBecauseProtection
            elif cpe.returncode == VerifyError:
                cpe.returncode = ErrVerify
            raise cpe
        return True
