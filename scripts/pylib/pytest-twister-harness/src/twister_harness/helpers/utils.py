# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
from __future__ import annotations

import logging
import re

from pathlib import Path


logger = logging.getLogger(__name__)


def find_in_config(config_file: Path | str, config_key: str) -> str:
    """Find key in config file"""
    re_key = re.compile(rf'{config_key}=(.+)')
    with open(config_file) as f:
        lines = f.readlines()
    for line in lines:
        if m := re_key.match(line):
            logger.debug('Found matching key: %s' % line.strip())
            return m.group(1).strip('"\'')
    logger.debug('Not found key: %s' % config_key)
    return ''


def match_lines(output_lines: list[str], searched_lines: list[str]) -> None:
    """Check all lines exist in the output"""
    __tracebackhide__ = True  # pylint: disable=unused-variable
    missing_lines = [sl for sl in searched_lines if not any(sl in line for line in output_lines)]
    if missing_lines:
        raise AssertionError(f"Missing lines: {missing_lines}")


def match_no_lines(output_lines: list[str], searched_lines: list[str]) -> None:
    """Check lines not found in the output"""
    __tracebackhide__ = True  # pylint: disable=unused-variable
    found_lines = [sl for sl in searched_lines if any(sl in line for line in output_lines)]
    if found_lines:
        raise AssertionError(f"Found lines that should not be present: {found_lines}")
