# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
from __future__ import annotations

import logging
import re

from pathlib import Path
from twister_harness import Shell, MCUmgr
from twister_harness.helpers.shell import ShellMCUbootCommandParsed


logger = logging.getLogger(__name__)


def find_in_config(config_file: Path | str, config_key: str) -> str:
    re_key = re.compile(rf'{config_key}=(.+)')
    with open(config_file) as f:
        lines = f.readlines()
    for line in lines:
        if m := re_key.match(line):
            logger.debug('Found matching key: %s' % line.strip())
            return m.group(1).strip('"\'')
    return ''


def match_lines(output_lines: list[str], searched_lines: list[str]) -> None:
    """Check all lines exist in the output"""
    for sl in searched_lines:
        assert any(sl in line for line in output_lines)


def match_no_lines(output_lines: list[str], searched_lines: list[str]) -> None:
    """Check lines not found in the output"""
    for sl in searched_lines:
        assert all(sl not in line for line in output_lines)


def check_with_shell_command(shell: Shell, version: str, swap_type: str | None = None) -> None:
    mcuboot_areas = ShellMCUbootCommandParsed.create_from_cmd_output(shell.exec_command('mcuboot'))
    assert mcuboot_areas.areas[0].version == version
    if swap_type:
        assert mcuboot_areas.areas[0].swap_type == swap_type


def check_with_mcumgr_command(mcumgr: MCUmgr, version: str) -> None:
    image_list = mcumgr.get_image_list()
    # version displayed by MCUmgr does not print +0 and changes + to '.' for non-zero values
    assert image_list[0].version == version.replace('+0', '').replace('+', '.')
