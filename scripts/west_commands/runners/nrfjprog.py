# Copyright (c) 2017 Linaro Limited.
# Copyright (c) 2023 Nordic Semiconductor ASA.
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for flashing with nrfjprog.'''

import subprocess
import sys

from runners.nrf_common import ErrNotAvailableBecauseProtection, ErrVerify, \
                               NrfBinaryRunner

# https://infocenter.nordicsemi.com/index.jsp?topic=%2Fug_nrf_cltools%2FUG%2Fcltools%2Fnrf_nrfjprogexe_return_codes.html&cp=9_1_3_1
UnavailableOperationBecauseProtectionError = 16
VerifyError = 55

class NrfJprogBinaryRunner(NrfBinaryRunner):
    '''Runner front-end for nrfjprog.'''

    @classmethod
    def name(cls):
        return 'nrfjprog'

    @classmethod
    def tool_opt_help(cls) -> str:
        return 'Additional options for nrfjprog, e.g. "--recover"'

    @classmethod
    def do_create(cls, cfg, args):
        return NrfJprogBinaryRunner(cfg, args.nrf_family, args.softreset,
                                    args.dev_id, erase=args.erase,
                                    reset=args.reset,
                                    tool_opt=args.tool_opt, force=args.force,
                                    recover=args.recover)

    def do_get_boards(self):
        snrs = self.check_output(['nrfjprog', '--ids'])
        return snrs.decode(sys.getdefaultencoding()).strip().splitlines()

    def do_require(self):
        self.require('nrfjprog')

    def do_exec_op(self, op, force=False):
        self.logger.debug(f'Executing op: {op}')
        # Translate the op

        families = {'NRF51_FAMILY': 'NRF51', 'NRF52_FAMILY': 'NRF52',
                    'NRF53_FAMILY': 'NRF53', 'NRF91_FAMILY': 'NRF91'}
        cores = {'NRFDL_DEVICE_CORE_APPLICATION': 'CP_APPLICATION',
                 'NRFDL_DEVICE_CORE_NETWORK': 'CP_NETWORK'}

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
            erase = _op['chip_erase_mode']
            if erase == 'ERASE_ALL':
                cmd.append('--chiperase')
            elif erase == 'ERASE_PAGES':
                cmd.append('--sectorerase')
            elif erase == 'ERASE_PAGES_INCLUDING_UICR':
                cmd.append('--sectoranduicrerase')
            else:
                raise RuntimeError(f'Invalid erase mode: {erase}')

            if _op.get('qspi_erase_mode'):
                # In the future there might be multiple QSPI erase modes
                cmd.append('--qspisectorerase')
            if _op.get('verify'):
                # In the future there might be multiple verify modes
                cmd.append('--verify')
        elif op_type == 'recover':
            cmd.append('--recover')
        elif op_type == 'reset':
            if _op['option'] == 'RESET_SYSTEM':
                cmd.append('--reset')
            if _op['option'] == 'RESET_PIN':
                cmd.append('--pinreset')
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
