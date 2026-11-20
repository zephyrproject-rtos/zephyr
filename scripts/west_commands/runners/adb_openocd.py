# SPDX-FileCopyrightText: Copyright (c) 2026 Canonical Limited
# SPDX-License-Identifier: Apache-2.0

"""Runner for flashing through an OpenOCD instance running on an ADB device.

Some boards pair an MCU with a Linux-capable processor that drives the MCU's
SWD lines (e.g. through GPIOs) and ships its own OpenOCD. This runner copies
the image to that processor with ``adb push`` and runs OpenOCD there with
``adb shell``.
"""

import argparse
import posixpath
import shlex
import subprocess

from runners.core import RunnerCaps, RunnerConfig, ZephyrBinaryRunner

DEFAULT_ADB = 'adb'
DEFAULT_OPENOCD = '/opt/openocd/bin/openocd'
DEFAULT_SEARCH = ['/opt/openocd', '/opt/openocd/share/openocd/scripts']
DEFAULT_CONFIG = ['openocd_gpiod.cfg']
DEFAULT_REMOTE_DIR = '/tmp/zephyr-adb-openocd'


class AdbOpenOcdBinaryRunner(ZephyrBinaryRunner):
    """Runner front-end for OpenOCD running on an ADB-connected device."""

    def __init__(
        self,
        cfg: RunnerConfig,
        adb: str = DEFAULT_ADB,
        dev_id: str | None = None,
        openocd: str = DEFAULT_OPENOCD,
        search: list[str] | None = None,
        config: list[str] | None = None,
        pre_init: list[str] | None = None,
        post_reset: list[str] | None = None,
        remote_dir: str = DEFAULT_REMOTE_DIR,
        verify: bool = True,
    ):
        super().__init__(cfg)
        self.adb = adb
        self.dev_id = dev_id
        self.openocd = openocd
        self.search = search if search is not None else DEFAULT_SEARCH
        self.config = config if config is not None else DEFAULT_CONFIG
        self.pre_init = pre_init or []
        self.post_reset = post_reset or []
        self.remote_dir = remote_dir
        self.verify = verify

    @classmethod
    def name(cls):
        return 'adb-openocd'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'}, dev_id=True)

    @classmethod
    def dev_id_help(cls) -> str:
        return 'ADB serial number of the device running OpenOCD'

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--adb', default=DEFAULT_ADB, help='path to the adb tool on the host')
        parser.add_argument(
            '--remote-openocd',
            default=DEFAULT_OPENOCD,
            help=f'path to openocd on the device, default {DEFAULT_OPENOCD}',
        )
        parser.add_argument(
            '--remote-search',
            action='append',
            help='OpenOCD script search directory on the device; may be given multiple times',
        )
        parser.add_argument(
            '--remote-config',
            action='append',
            help='OpenOCD configuration file on the device; may be given multiple times',
        )
        parser.add_argument(
            '--cmd-pre-init',
            action='append',
            help='OpenOCD command to run before "init"; may be given multiple times',
        )
        parser.add_argument(
            '--cmd-post-reset',
            action='append',
            help='OpenOCD command to run after the final reset; may be given multiple times',
        )
        parser.add_argument(
            '--remote-dir',
            default=DEFAULT_REMOTE_DIR,
            help=f'directory on the device to copy the image to, default {DEFAULT_REMOTE_DIR}',
        )
        parser.add_argument(
            '--verify',
            default=True,
            action=argparse.BooleanOptionalAction,
            help='verify the image after flashing (default: true)',
        )

    @classmethod
    def do_create(cls, cfg, args):
        return AdbOpenOcdBinaryRunner(
            cfg,
            adb=args.adb,
            dev_id=args.dev_id,
            openocd=args.remote_openocd,
            search=args.remote_search,
            config=args.remote_config,
            pre_init=args.cmd_pre_init,
            post_reset=args.cmd_post_reset,
            remote_dir=args.remote_dir,
            verify=args.verify,
        )

    def _adb(self, *args: str) -> list[str]:
        cmd = [self.adb]
        if self.dev_id:
            cmd += ['-s', self.dev_id]
        return cmd + list(args)

    def do_run(self, command, **kwargs):
        self.require(self.adb)
        self.ensure_output('elf')

        remote_elf = posixpath.join(self.remote_dir, 'zephyr.elf')

        openocd_cmd = [self.openocd]
        for path in self.search:
            openocd_cmd += ['-s', path]
        for config in self.config:
            openocd_cmd += ['-f', config]
        for cmd in self.pre_init:
            openocd_cmd += ['-c', cmd]
        openocd_cmd += [
            '-c',
            'init',
            '-c',
            'reset halt',
            '-c',
            f'flash write_image erase "{remote_elf}"',
        ]
        if self.verify:
            openocd_cmd += ['-c', f'verify_image "{remote_elf}"']
        openocd_cmd += ['-c', 'reset run']
        for cmd in self.post_reset:
            openocd_cmd += ['-c', cmd]
        openocd_cmd += ['-c', 'shutdown']

        self.check_call(self._adb('shell', 'mkdir', '-p', shlex.quote(self.remote_dir)))
        self.check_call(self._adb('push', self.cfg.elf_file, remote_elf))
        try:
            # adb recommends double quoting to prevent a multi-word command's
            # quotes from getting swallowed by the shell. See:
            # https://developer.android.com/tools/adb#shellcommands
            self.check_call(self._adb('shell', shlex.join(openocd_cmd)))
        finally:
            try:
                self.check_call(self._adb('shell', 'rm', '-f', shlex.quote(remote_elf)))
            except subprocess.CalledProcessError as e:
                self.logger.warning(f'failed to remove {remote_elf} on device: {e}')
