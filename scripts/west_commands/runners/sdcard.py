# Copyright (c) 2026 Analog Devices, Inc.
#
# SPDX-License-Identifier: Apache-2.0

'''Runner (flash only) for boards that boot an image from a mounted SD card.'''

import os
from pathlib import Path
from shutil import copyfile

from runners.core import RunnerCaps, ZephyrBinaryRunner


class SdcardBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for copying an image to a mounted SD card.'''

    def __init__(self, cfg, mountpoint=None, dest_name=None, dry_run=False):
        super().__init__(cfg)
        self.mountpoint = mountpoint
        self.dest_name = dest_name
        self.dry_run = dry_run

    @classmethod
    def name(cls):
        return 'sdcard'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'}, file=True, dry_run=True)

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument(
            '--mountpoint', help='mount point of the SD card partition the board boots from'
        )
        parser.add_argument(
            '--dest-name', help='file name on the SD card (default: name of the image)'
        )

    @classmethod
    def do_create(cls, cfg, args):
        return SdcardBinaryRunner(
            cfg, mountpoint=args.mountpoint, dest_name=args.dest_name, dry_run=args.dry_run
        )

    def do_run(self, command, **kwargs):
        if not self.mountpoint:
            raise RuntimeError('--mountpoint is required')
        if not os.path.ismount(self.mountpoint):
            raise RuntimeError(f"'{self.mountpoint}' is not a mount point; is the SD card mounted?")

        if self.cfg.file:
            src = self.cfg.file
        else:
            self.ensure_output('bin')
            src = self.cfg.bin_file

        dest = Path(self.mountpoint) / (self.dest_name or Path(src).name)
        self.logger.info("Copying '%s' to '%s'", src, dest)
        if self.dry_run:
            return

        copyfile(src, dest)
        if hasattr(os, 'sync'):
            os.sync()
        self.logger.info("Copied to '%s'", dest)
