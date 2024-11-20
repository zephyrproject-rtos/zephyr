# Copyright 2022 NXP
# SPDX-License-Identifier: Apache-2.0

'''Runner for Lauterbach TRACE32.'''

import argparse
import os
import platform
import subprocess
from pathlib import Path
from tempfile import TemporaryDirectory
from typing import List, Optional

from runners.core import BuildConfiguration, RunnerCaps, RunnerConfig, ZephyrBinaryRunner

DEFAULT_T32_CONFIG = Path('config.t32')


class TRACE32BinaryRunner(ZephyrBinaryRunner):
    '''
    Runner front-end for Lauterbach TRACE32.

    The runner is a wrapper around Lauterbach TRACE32 PowerView. It executes a Lauterbach Practice
    script (.cmm) after launching the debugger, which should be located at
    zephyr/boards/<board>/support/<command>.cmm, where <board> is the board directory and <command>
    is the name of the west runner command executed (e.g. flash or debug). Extra arguments can be
    passed to the startup script by using the command line option --startup-args.
    '''

    def __init__(self,
                 cfg: RunnerConfig,
                 t32_cfg: Path,
                 arch: str,
                 startup_args: list[str] | None = None,
                 timeout: int = 60) -> None:
        super().__init__(cfg)
        self.arch = arch
        self.t32_cfg = t32_cfg
        self.t32_exec: Path | None = None
        self.startup_dir = Path(cfg.board_dir) / 'support'
        self.startup_args = startup_args
        self.timeout = timeout

    @classmethod
    def name(cls) -> str:
        return 'trace32'

    @classmethod
    def capabilities(cls) -> RunnerCaps:
        return RunnerCaps(commands={'flash', 'debug'})

    @classmethod
    def do_add_parser(cls, parser: argparse.ArgumentParser) -> None:
        parser.add_argument('--arch',
                            default='auto',
                            choices=('auto', 'arm', 'riscv', 'xtensa'),
                            help='Target architecture. Set to "auto" to select the architecture '
                                 'based on CONFIG_ARCH value')
        parser.add_argument('--config',
                            default=DEFAULT_T32_CONFIG,
                            type=Path,
                            help='Override TRACE32 configuration file path. Can be a relative path '
                                 'to T32_DIR environment variable, or an absolute path')
        parser.add_argument('--startup-args',
                            nargs='*',
                            help='Arguments to pass to the start-up script')
        parser.add_argument('--timeout',
                            default=60,
                            type=int,
                            help='Timeout, in seconds, of the flash operation')

    @classmethod
    def do_create(cls, cfg: RunnerConfig, args: argparse.Namespace) -> 'TRACE32BinaryRunner':
        build_conf = BuildConfiguration(cfg.build_dir)
        if args.arch == 'auto':
            arch = build_conf.get('CONFIG_ARCH').replace('"', '')
            # there is a single binary for all ARM architectures
            arch = arch.replace('arm64', 'arm')
        else:
            arch = args.arch
        return TRACE32BinaryRunner(cfg, args.config, arch, startup_args=args.startup_args,
                                   timeout=args.timeout)

    def do_run(self, command, **kwargs) -> None:
        t32_dir = os.environ.get('T32_DIR')
        if not t32_dir:
            raise RuntimeError('T32_DIR environment variable undefined')

        if platform.system() == 'Windows':
            os_name = 'windows64'
            suffix = '.exe'
        elif platform.system() == 'Linux':
            os_name = 'pc_linux64'
            suffix = ''
        else:
            raise RuntimeError('Host OS not supported by this runner')

        self.t32_exec = Path(t32_dir) / 'bin' / os_name / f't32m{self.arch}{suffix}'
        if not self.t32_exec.exists():
            raise RuntimeError(f'Cannot find Lauterbach executable at {self.t32_exec}')

        if not self.t32_cfg.is_absolute():
            self.t32_cfg = Path(t32_dir) / self.t32_cfg
        if not self.t32_cfg.exists():
            raise RuntimeError(f'Cannot find Lauterbach configuration at {self.t32_cfg}')

        startup_script = self.startup_dir / f'{command}.cmm'
        if not startup_script.exists():
            raise RuntimeError(f'Cannot find start-up script at {startup_script}')

        if command == 'flash':
            self.flash(**kwargs)
        elif command == 'debug':
            self.debug(**kwargs)

    def flash(self, **kwargs) -> None:
        with TemporaryDirectory(suffix='t32') as tmp_dir:
            # use a temporary config file, based on the provided configuration,
            # to hide the TRACE32 software graphical interface
            cfg_content = f'{self.t32_cfg.read_text()}\n\nSCREEN=OFF\n'
            tmp_cfg = Path(tmp_dir) / DEFAULT_T32_CONFIG.name
            tmp_cfg.write_text(cfg_content)

            cmd = self.get_launch_command('flash', cfg=tmp_cfg)
            self.logger.info(f'Launching TRACE32: {" ".join(cmd)}')
            try:
                self.check_call(cmd, timeout=self.timeout)
                self.logger.info('Finished')
            except subprocess.TimeoutExpired:
                self.logger.error(f'Timed out after {self.timeout} seconds')

    def debug(self, **kwargs) -> None:
        cmd = self.get_launch_command('debug')
        self.logger.info(f'Launching TRACE32: {" ".join(cmd)}')
        self.check_call(cmd)

    def get_launch_command(self, command_name: str,
                           cfg: Path | None = None) -> list[str]:
        cmd = [
            str(self.t32_exec),
            '-c', str(cfg if cfg else self.t32_cfg),
            '-s', str(self.startup_dir / f'{command_name}.cmm')
        ]
        if self.startup_args:
            cmd.extend(self.startup_args)
        return cmd
