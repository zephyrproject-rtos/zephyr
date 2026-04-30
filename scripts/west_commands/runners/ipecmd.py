# Copyright (c) 2025, Microchip Technology Inc.
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for flashing PIC devices with ipecmd.'''

import os
import platform
import re
import shutil
from pathlib import Path

from runners.core import RunnerCaps, ZephyrBinaryRunner

COMMON_LOCATIONS = ["/opt", "/usr/local", "/home", "/Applications", "C:\\Program Files"]


class IpecmdBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for Microchip's dspic33a_curiosity.'''

    def __init__(self, cfg, dev_id, flash_tool, ipecmd_path=None, java_path=None):
        super().__init__(cfg)
        self.hex_file = cfg.hex_file
        self.dev_id = dev_id
        self.flash_tool = flash_tool
        self.ipecmd_path = ipecmd_path
        self.java_path = java_path

    @classmethod
    def name(cls):
        return 'ipecmd'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'}, dev_id=True, erase=True, reset=True)

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--flash-tool', required=True, help='hardware tool to program')
        parser.add_argument(
            '--ipecmd-path',
            help='absolute path to ipecmd.jar (Linux) or ipecmd.exe (Windows); '
            'skips auto-discovery when provided',
        )
        parser.add_argument(
            '--java-path',
            help='absolute path to java binary (Linux only); skips PATH lookup when provided',
        )

        parser.set_defaults(reset=True)

    @classmethod
    def do_create(cls, cfg, args):
        return IpecmdBinaryRunner(
            cfg,
            args.dev_id,
            args.flash_tool,
            ipecmd_path=args.ipecmd_path,
            java_path=args.java_path,
        )

    def do_run(self, command, **kwargs):
        self.ensure_output('hex')

        self.logger.info(f'Flashing file: {self.hex_file}')
        self.logger.info(f'flash tool: {self.flash_tool}, Device: {self.dev_id}')

        if platform.system() == 'Linux':
            if self.java_path is not None:
                java_bin = self.java_path
            else:
                java_bin = self.find_java_bin()
                if java_bin is None:
                    self.logger.error(f'java not found in PATH: {os.environ.get("PATH", "")}')
                    raise RuntimeError('java not found')

            if self.ipecmd_path is not None:
                ipecmd_jar = Path(self.ipecmd_path)
            else:
                mplabx_base = self.find_mplabx_base()
                if mplabx_base is None:
                    self.logger.error(f'MPLABX installation not found in: {COMMON_LOCATIONS}')
                    raise RuntimeError('MPLABX not found')

                version_path = self.find_latest_version_dir(mplabx_base)
                if version_path is None:
                    self.logger.error(f'No versioned MPLABX directory found under: {mplabx_base}')
                    raise RuntimeError('MPLABX version directory not found')

                ipecmd_jar = self.find_ipecmd(version_path, 'ipecmd.jar')
                if ipecmd_jar is None:
                    ipe_dir = version_path / 'mplab_platform/mplab_ipe'
                    self.logger.error(f'ipecmd.jar not found under: {ipe_dir}')
                    raise RuntimeError('ipecmd.jar not found')

            self.logger.info(f'flash cmd: {ipecmd_jar}')
            cmd = [
                str(java_bin),
                '-jar',
                str(ipecmd_jar),
                '-TP' + self.flash_tool,
                '-P' + self.dev_id,
                '-M',
                '-F' + self.hex_file,
                '-OL',
            ]
        elif platform.system() == 'Windows':
            if self.ipecmd_path is not None:
                ipecmd_exe = Path(self.ipecmd_path)
            else:
                mplabx_base = self.find_mplabx_base()
                if mplabx_base is None:
                    self.logger.error(f'MPLABX installation not found in: {COMMON_LOCATIONS}')
                    raise RuntimeError('MPLABX not found')

                version_path = self.find_latest_version_dir(mplabx_base)
                if version_path is None:
                    self.logger.error(f'No versioned MPLABX directory found under: {mplabx_base}')
                    raise RuntimeError('MPLABX version directory not found')

                ipecmd_exe = self.find_ipecmd(version_path, 'ipecmd.exe')
                if ipecmd_exe is None:
                    ipe_dir = version_path / 'mplab_platform/mplab_ipe'
                    self.logger.error(f'ipecmd.exe not found under: {ipe_dir}')
                    raise RuntimeError('ipecmd.exe not found')

            self.logger.info(f'flash cmd: {ipecmd_exe}')
            cmd = [
                str(ipecmd_exe),
                '-TP' + self.flash_tool,
                '-P' + self.dev_id,
                '-M',
                '-F' + self.hex_file,
                '-OL',
            ]
        else:
            self.logger.error(f'Unsupported platform: {platform.system()}')
            raise RuntimeError('Unsupported platform')

        self.require(cmd[0])
        self.check_call(cmd)

    def find_mplabx_base(self):
        for base in COMMON_LOCATIONS:
            for root, dirs, _ in os.walk(base):
                for d in dirs:
                    if d.startswith("mplabx") or d.startswith("MPLABX"):
                        return Path(root) / d
        return None

    def find_latest_version_dir(self, mplabx_base):
        def version_key(p):
            return tuple(int(x) for x in re.findall(r'\d+', p.name))

        versions = sorted(
            [p for p in mplabx_base.glob("v*/") if p.is_dir()],
            key=version_key,
        )
        return versions[-1] if versions else None

    def find_java_bin(self):
        return shutil.which("java")

    def find_ipecmd(self, version_path, tool):
        ipe_dir = version_path / "mplab_platform/mplab_ipe"
        for root, _, files in os.walk(ipe_dir):
            if tool in files:
                return Path(root) / tool
        return None
