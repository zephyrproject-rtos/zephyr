# Copyright (c) 2021, Telink Semiconductor
#
# SPDX-License-Identifier: Apache-2.0

import os
import time
import subprocess

from runners.core import ZephyrBinaryRunner, RunnerCaps

class SpiBurnBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for SPI_burn.'''

    def __init__(self, cfg, addr, spiburn, iceman, erase=False):
        super().__init__(cfg)

        self.bin = cfg.bin_file
        self.spiburn = spiburn
        self.iceman = iceman
        self.addr = addr
        self.erase = bool(erase)

    @classmethod
    def name(cls):
        return 'spi_burn'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'}, erase=True)

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--addr', default='0x0',
                            help='start flash address to write')
        parser.add_argument('--telink-tools-path', help='path to Telink flash tools')

    @classmethod
    def do_create(cls, cfg, args):

        if args.telink_tools_path:
            spiburn = f'{args.telink_tools_path}/flash/bin/SPI_burn'
            iceman = f'{args.telink_tools_path}/ice/ICEman'
        else:
            # If telink_tools_path arg is not specified then pass to tools shall be specified in PATH
            spiburn = 'SPI_burn'
            iceman  = 'ICEman'

        return SpiBurnBinaryRunner(cfg, args.addr, spiburn, iceman, args.erase)

    def do_run(self, command, **kwargs):

        self.require(self.spiburn)

        # Find path to ICEman with require call
        self.iceman_path = self.require(self.iceman)

        if command == "flash":
            self._flash()
        else:
            self.logger.error(f'{command} not supported!')

    def _flash(self):

        origin_dir = os.getcwd()
        iceman_dir = os.path.dirname(self.iceman_path)

        try:

            # ICEman tool from Andestech is required to be run from its root dir,
            # due to the tool looking for its config files in currently active dir.
            # In attempt to run it from other dirs it returns with an error that
            # required config file is missing

            # Go into ICEman dir
            os.chdir(iceman_dir)

            # RUN ice in a background
            cmd_ice_run = ["./ICEman", '-Z', 'v5', '-l', 'aice_sdp.cfg']

            ice_process = subprocess.Popen(cmd_ice_run)

            # Wait for initialization
            time.sleep(1)

            # Compose flash command
            cmd_flash = [self.spiburn, '--addr', str(self.addr), '--image', self.bin]

            if self.erase:
                cmd_flash += ["--erase-all"]

            # Run SPI burn flash tool
            self.check_call(cmd_flash)

        finally:

            # Kill ICEman
            ice_process.terminate()

            # Restore origin dir
            os.chdir(origin_dir)
