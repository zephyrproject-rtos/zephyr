# Copyright (c) 2025, Microchip Technology Inc.
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for flashing PIC devices with ipecmd.'''

import os
import platform
import sys
from pathlib import Path

from runners.core import RunnerCaps, ZephyrBinaryRunner

COMMON_LOCATIONS = ["/opt", "/usr/local", "/home", "C:\\Program Files"]


class IpecmdBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for Microchip's dspic33a_curiosity.'''

    def __init__(self, cfg, device, flash_tool):
        super().__init__(cfg)
        self.elf = cfg.elf_file
        self.ipecmd_cmd = None
        self.mplabx_base = None
        self.java_bin = None
        self.ipecmd_jar = None

        self.mplabx_base = self.find_mplabx_base()
        if not self.mplabx_base:
            print("Error: Could not locate mplabx base directory")
            sys.exit(1)

        version_path = self.find_latest_version_dir(self.mplabx_base)
        if not version_path:
            print("Error: No MPLAB X version directories found")
            sys.exit(1)
        if platform.system() == 'Linux':
            self.java_bin = self.find_java_bin(version_path)
            if not self.java_bin or not os.access(self.java_bin, os.X_OK):
                print("Error: Java executable not found or not executable")
                sys.exit(1)

            self.ipecmd_jar = self.find_ipecmd(version_path, "ipecmd.jar")
            if not self.ipecmd_jar:
                print(f"Error: ipecmd.jar not found in {version_path}/mplab_platform/mplab_ipe/")
                sys.exit(1)
            else:
                print(f'ipecmd: {self.ipecmd_jar}')
        elif platform.system() == 'Windows':
            self.ipecmd_exe = self.find_ipecmd(version_path, "ipecmd.exe")
            if not self.ipecmd_exe:
                print(f"Error: ipecmd.exe not found in {version_path}/mplab_platform/mplab_ipe/")
                sys.exit(1)
            else:
                print(f'ipecmd: {self.ipecmd_exe}')
        self.app_bin = cfg.bin_file
        print(f'bin file: {cfg.bin_file}')
        self.hex_file = cfg.hex_file
        print(f'hex file: {cfg.hex_file}')
        self.device = device
        self.flash_tool = flash_tool

    @classmethod
    def name(cls):
        return 'ipecmd'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'}, erase=True, reset=True)

    @classmethod
    def do_add_parser(cls, parser):
        # Required
        parser.add_argument('--device', required=True, help='soc')
        parser.add_argument('--flash-tool', required=True, help='hardware tool to program')

        parser.set_defaults(reset=True)

    @classmethod
    def do_create(cls, cfg, args):
        return IpecmdBinaryRunner(cfg, args.device, args.flash_tool)

    def do_run(self, command, **kwargs):
        print("***************Flashing*************")
        self.ensure_output('hex')

        self.logger.info(f'Flashing file: {self.hex_file}')
        self.logger.info(f'flash tool: {self.flash_tool}, Device: {self.device}')
        if self.hex_file is not None:
            if platform.system() == 'Linux':
                self.logger.info(f'flash cmd: {self.ipecmd_jar}')
                cmd = [
                    str(self.java_bin),
                    '-jar',
                    str(self.ipecmd_jar),
                    '-TP' + self.flash_tool,
                    '-P' + self.device,
                    '-M',
                    '-F' + self.hex_file,
                    '-OL',
                ]
            elif platform.system() == 'Windows':
                self.logger.info(f'flash cmd: {self.ipecmd_exe}')
                cmd = [
                    str(self.ipecmd_exe),
                    '-TP' + self.flash_tool,
                    '-P' + self.device,
                    '-M',
                    '-F' + self.hex_file,
                    '-OL',
                ]
            self.require(cmd[0])
            self.check_call(cmd)
        else:
            print("E: No HEX file found")

    def find_mplabx_base(self):
        for base in COMMON_LOCATIONS:
            for root, dirs, _ in os.walk(base):
                for d in dirs:
                    if d.startswith("mplabx") or d.startswith("MPLABX"):
                        return Path(root) / d
        return None

    def find_latest_version_dir(self, mplabx_base):
        versions = sorted([p for p in mplabx_base.glob("v*/") if p.is_dir()])
        return versions[-1] if versions else None

    def find_java_bin(self, version_path):
        if platform.system() == 'Linux':
            java_dirs = list(version_path.glob("sys/java/*/bin/java"))
        elif platform.system() == 'Windows':
            java_dirs = list(version_path.glob("sys/java/*/bin/java.exe"))
        else:
            self.logger.error("Platform not supported")
            sys.exit(1)
        return java_dirs[0] if java_dirs else None

    def find_ipecmd(self, version_path, tool):
        ipe_dir = version_path / "mplab_platform/mplab_ipe"
        for root, _, files in os.walk(ipe_dir):
            if tool in files:
                return Path(root) / tool
        return None
