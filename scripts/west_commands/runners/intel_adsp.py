# Copyright (c) 2022 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for flashing with the Intel ADSP CAVS boards.'''

import os
import sys
import re
import hashlib
import random
import shutil
from runners.core import ZephyrBinaryRunner, RunnerCaps
from zephyr_ext_common import ZEPHYR_BASE

DEFAULT_CAVSTOOL='soc/xtensa/intel_adsp/tools/cavstool_client.py'
DEFAULT_SOF_MOD_DIR=os.path.join(ZEPHYR_BASE, '../modules/audio/sof')
DEFAULT_RIMAGE_TOOL=shutil.which('rimage')
DEFAULT_CONFIG_DIR=os.path.join(DEFAULT_SOF_MOD_DIR, 'rimage/config')
DEFAULT_KEY_DIR=os.path.join(DEFAULT_SOF_MOD_DIR, 'keys')


class IntelAdspBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for the intel ADSP boards.'''

    def __init__(self,
                 cfg,
                 remote_host,
                 rimage_tool,
                 config_dir,
                 default_key,
                 key,
                 pty,
                 tool_opt,
                 do_sign,
                 ):
        super().__init__(cfg)

        self.remote_host = remote_host
        self.rimage_tool = rimage_tool
        self.config_dir = config_dir
        self.bin_fw = os.path.join(cfg.build_dir, 'zephyr', 'zephyr.ri')

        self.cavstool = os.path.join(ZEPHYR_BASE, DEFAULT_CAVSTOOL)
        self.platform = os.path.basename(cfg.board_dir)
        self.pty = pty

        if key:
            self.key = key
        else:
            self.key = os.path.join(DEFAULT_KEY_DIR, default_key)

        self.tool_opt_args = tool_opt
        self.do_sign = do_sign

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
        parser.add_argument('--rimage-tool', default=DEFAULT_RIMAGE_TOOL,
                            help='path to the rimage tool')
        parser.add_argument('--config-dir', default=DEFAULT_CONFIG_DIR,
                            help='path to the toml config file')
        parser.add_argument('--default-key',
                            help='the default basename of the key store in board.cmake')
        parser.add_argument('--key',
                            help='specify where the signing key is')
        parser.add_argument('--pty', nargs='?', const="remote-host", type=str,
                            help=''''Capture the output of cavstool.py running on --remote-host \
                            and stream it remotely to west's standard output.''')

    @classmethod
    def tool_opt_help(cls) -> str:
        return """Additional options for run/request service tool,
        e.g. '--lock' """

    @classmethod
    def do_create(cls, cfg, args):
        # We now have `west flash` -> `west build` -> `west sign` so
        # `west flash` -> `west sign` is not needed anymore; it's also
        # slower because not concurrent. However, for backwards
        # compatibility keep signing here if some explicit rimage
        # --option was passed. Some of these options may differ from the
        # current `west sign` configuration; we take "precedence" by
        # running last.
        do_sign = (
            args.rimage_tool != DEFAULT_RIMAGE_TOOL or
            args.config_dir != DEFAULT_CONFIG_DIR or
            args.key is not None
        )
        return IntelAdspBinaryRunner(cfg,
                                    remote_host=args.remote_host,
                                    rimage_tool=args.rimage_tool,
                                    config_dir=args.config_dir,
                                    default_key=args.default_key,
                                    key=args.key,
                                    pty=args.pty,
                                    tool_opt=args.tool_opt,
                                    do_sign=do_sign,
                                    )

    def do_run(self, command, **kwargs):
        self.logger.info('Starting Intel ADSP runner')

        if self.do_sign:
            self.sign(**kwargs)

        if re.search("intel_adsp_cavs", self.platform):
            self.require(self.cavstool)
            self.flash(**kwargs)
        else:
            self.logger.error("No suitable platform for running")
            sys.exit(1)

    def sign(self, **kwargs):
        path_opt = ['-p', f'{self.rimage_tool}'] if self.rimage_tool else []
        sign_cmd = (
            ['west', 'sign', '-d', f'{self.cfg.build_dir}', '-t', 'rimage']
            + path_opt + ['-D', f'{self.config_dir}', '--', '-k', f'{self.key}']
        )
        self.logger.info(" ".join(sign_cmd))
        self.check_call(sign_cmd)

    def flash(self, **kwargs):
        # Generate a hash string for appending to the sending ri file
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
