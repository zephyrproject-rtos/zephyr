# Copyright (c) 2022-2024 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for flashing with the Intel ADSP boards.'''

import argparse
import hashlib
import os
import random
import re
import shutil
import sys

from zephyr_ext_common import ZEPHYR_BASE

from runners.core import RunnerCaps, ZephyrBinaryRunner

DEFAULT_CAVSTOOL='soc/intel/intel_adsp/tools/cavstool_client.py'

class SignParamError(argparse.Action):
    'User-friendly feedback when trying to sign with west flash'
    def __call__(self, parser, namespace, values, option_string=None):
        parser.error(f'Cannot use "west flash {option_string} ..." any more. ' +
                     '"west sign" is now called from CMake, see "west sign -h"')

class IntelAdspBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for the intel ADSP boards.'''

    def __init__(self,
                 cfg,
                 remote_host,
                 pty,
                 tool_opt,
                 ):
        super().__init__(cfg)

        self.remote_host = remote_host
        self.bin_fw = os.path.join(cfg.build_dir, 'zephyr', 'zephyr.ri')

        self.cavstool = os.path.join(ZEPHYR_BASE, DEFAULT_CAVSTOOL)
        self.platform = os.path.basename(cfg.board_dir)
        self.pty = pty

        self.tool_opt_args = tool_opt

    @classmethod
    def name(cls):
        return 'intel_adsp'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'}, tool_opt=True)

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--remote-host',
                            help='hostname of the remote targeting ADSP board')
        parser.add_argument('--pty', nargs='?', const="remote-host", type=str,
                            help=''''Capture the output of cavstool.py running on --remote-host \
                            and stream it remotely to west's standard output.''')

        for old_sign_param in [ '--rimage-tool', '--config-dir', '--default-key', '--key']:
            parser.add_argument(old_sign_param, action=SignParamError,
                            help='do not use, "west sign" is now called from CMake, see "west sign -h"')

    @classmethod
    def tool_opt_help(cls) -> str:
        return """Additional options for run/request service tool,
        e.g. '--lock' """

    @classmethod
    def do_create(cls, cfg, args):
        return IntelAdspBinaryRunner(cfg,
                                    remote_host=args.remote_host,
                                    pty=args.pty,
                                    tool_opt=args.tool_opt,
                                    )

    def do_run(self, command, **kwargs):
        self.logger.info('Starting Intel ADSP runner')

        if re.search("adsp", self.platform):
            self.require(self.cavstool)
            self.flash(**kwargs)
        else:
            self.logger.error("No suitable platform for running")
            sys.exit(1)

    def flash(self, **kwargs):
        'Generate a hash string for appending to the sending ri file'
        hash_object = hashlib.md5(self.bin_fw.encode())
        random_str = f"{random.getrandbits(64)}".encode()
        hash_object.update(random_str)
        send_bin_fw = str(self.bin_fw + "." + hash_object.hexdigest())
        shutil.copy(self.bin_fw, send_bin_fw)

        # Copy the zephyr to target remote ADSP host and run
        self.run_cmd = ([f'{self.cavstool}','-s', f'{self.remote_host}', f'{send_bin_fw}'])

        # Add the extra tool options to run/request service tool
        if self.tool_opt_args:
            self.run_cmd = self.run_cmd + self.tool_opt_args

        self.logger.debug(f"rcmd: {self.run_cmd}")

        self.check_call(self.run_cmd)

        # If the self.pty is assigned, the log will output to stdout
        # directly. That means you don't have to execute the command:
        #
        #   cavstool_client.py -s {host}:{port} -l
        #
        # to get the result later separately.
        if self.pty is not None:
            if self.pty == 'remote-host':
                self.log_cmd = ([f'{self.cavstool}','-s', f'{self.remote_host}', '-l'])
            else:
                self.log_cmd = ([f'{self.cavstool}','-s', f'{self.pty}', '-l'])

            self.logger.debug(f"rcmd: {self.log_cmd}")

            self.check_call(self.log_cmd)
