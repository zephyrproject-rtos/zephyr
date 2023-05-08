# Copyright (c) 2023 Peter Johanson <peter@peterjohanson.com>
#
# SPDX-License-Identifier: Apache-2.0

'''UF2 runner (flash only) for UF2 compatible bootloaders.'''

from pathlib import Path
from shutil import copy

from runners.core import ZephyrBinaryRunner, RunnerCaps

try:
    import psutil  # pylint: disable=unused-import
    MISSING_PSUTIL = False
except ImportError:
    # This can happen when building the documentation for the
    # runners package if psutil is not on sys.path. This is fine
    # to ignore in that case.
    MISSING_PSUTIL = True

class UF2BinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for copying to UF2 USB-MSC mounts.'''

    def __init__(self, cfg, board_id=None):
        super().__init__(cfg)
        self.board_id = board_id

    @classmethod
    def name(cls):
        return 'uf2'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'})

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--board-id', dest='board_id',
                            help='Board-ID value to match from INFO_UF2.TXT')

    @classmethod
    def do_create(cls, cfg, args):
        return UF2BinaryRunner(cfg, board_id=args.board_id)

    @staticmethod
    def get_uf2_info_path(part) -> Path:
        return Path(part.mountpoint) / "INFO_UF2.TXT"

    @staticmethod
    def is_uf2_partition(part):
        try:
            return ((part.fstype in ['vfat', 'FAT']) and
                    UF2BinaryRunner.get_uf2_info_path(part).is_file())
        except PermissionError:
            return False

    @staticmethod
    def get_uf2_info(part):
        lines = UF2BinaryRunner.get_uf2_info_path(part).read_text().splitlines()

        lines = lines[1:] # Skip the first summary line

        def split_uf2_info(line: str):
            k, _, val = line.partition(':')
            return k.strip(), val.strip()

        return {k: v for k, v in (split_uf2_info(line) for line in lines) if k and v}

    def match_board_id(self, part):
        info = self.get_uf2_info(part)

        return info.get('Board-ID') == self.board_id

    def get_uf2_partitions(self):
        parts = [part for part in psutil.disk_partitions() if self.is_uf2_partition(part)]

        if (self.board_id is not None) and parts:
            parts = [part for part in parts if self.match_board_id(part)]
            if not parts:
                self.logger.warning("Discovered UF2 partitions don't match Board-ID '%s'",
                                    self.board_id)

        return parts

    def copy_uf2_to_partition(self, part):
        self.ensure_output('uf2')

        copy(self.cfg.uf2_file, part.mountpoint)

    def do_run(self, command, **kwargs):
        if MISSING_PSUTIL:
            raise RuntimeError(
                'could not import psutil; something may be wrong with the '
                'python environment')

        partitions = self.get_uf2_partitions()
        if not partitions:
            raise RuntimeError('No matching UF2 partitions found')

        if len(partitions) > 1:
            raise RuntimeError('More than one matching UF2 partitions found')

        part = partitions[0]
        self.logger.info("Copying UF2 file to '%s'", part.mountpoint)
        self.copy_uf2_to_partition(part)
