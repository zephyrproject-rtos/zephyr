# Copyright (c) 2023 Nordic Semiconductor ASA.
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for flashing with nrfutil.'''

import json
import os
import subprocess
import sys
from pathlib import Path

from runners.core import _DRY_RUN
from runners.nrf_common import NrfBinaryRunner


class NrfUtilBinaryRunner(NrfBinaryRunner):
    '''Runner front-end for nrfutil.'''

    def __init__(self, cfg, family, softreset, dev_id, erase=False,
                 reset=True, tool_opt=None, force=False, recover=False,
                 suit_starter=False):

        super().__init__(cfg, family, softreset, dev_id, erase, reset,
                         tool_opt, force, recover)

        self.suit_starter = suit_starter

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
                                   recover=args.recover,
                                   suit_starter=args.suit_manifest_starter)

    @classmethod
    def do_add_parser(cls, parser):
        super().do_add_parser(parser)
        parser.add_argument('--suit-manifest-starter', required=False,
                            action='store_true',
                            help='Use the SUIT manifest starter file')

    def _exec(self, args):
        jout_all = []

        cmd = ['nrfutil', '--json', 'device'] + args
        self._log_cmd(cmd)

        if _DRY_RUN:
            return {}

        with subprocess.Popen(cmd, stdout=subprocess.PIPE) as p:
            for line in iter(p.stdout.readline, b''):
                # https://github.com/ndjson/ndjson-spec
                jout = json.loads(line.decode(sys.getdefaultencoding()))
                jout_all.append(jout)

                if 'x-execute-batch' in args:
                    if jout['type'] == 'batch_update':
                        pld = jout['data']['data']
                        if (
                            pld['type'] == 'task_progress' and
                            pld['data']['progress']['progressPercentage'] == 0
                        ):
                            self.logger.info(pld['data']['progress']['description'])
                    elif jout['type'] == 'batch_end' and jout['data']['error']:
                        raise subprocess.CalledProcessError(
                            jout['data']['error']['code'], cmd
                        )

        return jout_all

    def do_get_boards(self):
        out = self._exec(['list'])
        devs = []
        for o in out:
            if o['type'] == 'task_end':
                devs = o['data']['data']['devices']
        snrs = [dev['serialNumber'] for dev in devs if dev['traits']['jlink']]

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
        json_file = os.fspath(hex_dir / 'generated_nrfutil_batch.json')

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
