# Copyright (c) 2023 Nordic Semiconductor ASA.
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for flashing with nrfutil.'''

import json
import os
from pathlib import Path
import sys
import subprocess

from runners.nrf_common import NrfBinaryRunner

class NrfUtilBinaryRunner(NrfBinaryRunner):
    '''Runner front-end for nrfutil.'''

    def __init__(self, cfg, family, softreset, dev_id, erase=False,
                 reset=True, tool_opt=[], force=False, recover=False):

        super().__init__(cfg, family, softreset, dev_id, erase, reset,
                         tool_opt, force, recover)
        self._ops = []
        self._op_id = 1

    @classmethod
    def name(cls):
        return 'nrfutil'

    @classmethod
    def tool_opt_help(cls) -> str:
        return 'Additional options for nrfutil, e.g. "--log-level"'

    @classmethod
    def do_create(cls, cfg, args):
        return NrfUtilBinaryRunner(cfg, args.nrf_family, args.softreset,
                                   args.dev_id, erase=args.erase,
                                   reset=args.reset,
                                   tool_opt=args.tool_opt, force=args.force,
                                   recover=args.recover)

    def _exec(self, args):
        try:
            out = self.check_output(['nrfutil', '--json', 'device'] + args)
        except subprocess.CalledProcessError as e:
            # https://docs.python.org/3/reference/compound_stmts.html#except-clause
            cpe = e
            out = cpe.stdout
        else:
            cpe = None
        finally:
            # https://github.com/ndjson/ndjson-spec
            out = [json.loads(s) for s in
                   out.decode(sys.getdefaultencoding()).split('\n') if len(s)]
            self.logger.debug(f'output: {out}')
            if cpe:
                if 'execute-batch' in args:
                    for o in out:
                        if o['type'] == 'batch_end' and o['data']['error']:
                            cpe.returncode = o['data']['error']['code']
                raise cpe

        return out

    def do_get_boards(self):
        out = self._exec(['list'])
        devs = []
        for o in out:
            if o['type'] == 'task_end':
                devs = o['data']['data']['devices']
        snrs = [dev['serialNumber'] for dev in devs]

        self.logger.debug(f'Found boards: {snrs}')
        return snrs

    def do_require(self):
        self.require('nrfutil')

    def _insert_op(self, op):
        op['operationId'] = f'{self._op_id}'
        self._op_id += 1
        self._ops.append(op)

    def _exec_batch(self):
        # prepare the dictionary and convert to JSON
        batch = json.dumps({'family': f'{self.family}',
                            'operations': [op for op in self._ops]},
                            indent=4) + '\n'

        hex_dir = Path(self.hex_).parent
        json_file = os.fspath(hex_dir / f'generated_nrfutil_batch.json')

        with open(json_file, "w") as f:
            f.write(batch)

        # reset first in case an exception is thrown
        self._ops = []
        self._op_id = 1
        self.logger.debug(f'Executing batch in: {json_file}')
        self._exec(['x-execute-batch', '--batch-path', f'{json_file}',
                    '--serial-number', f'{self.dev_id}'])

    def do_exec_op(self, op, force=False):
        self.logger.debug(f'Executing op: {op}')
        if force:
            if len(self._ops) != 0:
                raise RuntimeError(f'Forced exec with {len(self._ops)} ops')
            self._insert_op(op)
            self._exec_batch()
            return True
        # Defer by default
        return False

    def flush_ops(self, force=True):
        if not force:
            return
        while self.ops:
            self._insert_op(self.ops.popleft())
        self._exec_batch()
