# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
from __future__ import annotations

import logging
import shlex

from subprocess import check_output
from pathlib import Path


logger = logging.getLogger(__name__)


def west_sign_with_imgtool(
        build_dir: Path,
        output_bin: Path | None = None,
        key_file: Path | None = None,
        version: str | None = None,
        timeout: int = 10
):
    """Wrapper method for `west sign -t imgtool` comamnd"""
    command = [
        'west', 'sign',
        '-t', 'imgtool',
        '--no-hex',
        '--build-dir', str(build_dir)
    ]
    if output_bin:
        command.extend(['--sbin', str(output_bin)])

    command_extra_args = []
    if key_file:
        command_extra_args.extend(['--key', str(key_file)])
    if version:
        command_extra_args.extend(['--version', version])

    if command_extra_args:
        command.append('--')
        command.extend(command_extra_args)

    logger.info(f"CMD: {shlex.join(command)}")
    output = check_output(command, text=True, timeout=timeout)
    logger.debug('OUT: %s' % output)
