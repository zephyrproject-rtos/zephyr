# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import logging
import os
import sys
from pathlib import Path

from twister_harness.helper import normalize_filename

logger = logging.getLogger(__name__)


class LogFile:
    """Base class for logging files."""
    name = 'uninitialized'

    def __init__(self, filename: str | Path) -> None:
        self.default_encoding = sys.getdefaultencoding()
        self.filename = filename

    @staticmethod
    def get_log_filename(build_dir: Path | str, name: str) -> str:
        """
        :param build_dir: path to building directory.
        :param name: name of the logging file.
        :return: path to logging file
        """
        if not build_dir:
            filename = os.devnull
        else:
            name = name + '.log'
            filename = os.path.join(build_dir, name)
            filename = normalize_filename(filename=filename)
        return filename

    def handle(self, data: str | bytes) -> None:
        """Save information to logging file."""
        if data:
            data = data.decode(encoding=self.default_encoding) if isinstance(data, bytes) else data
            with open(file=self.filename, mode='a+', encoding=self.default_encoding) as log_file:
                log_file.write(data)  # type: ignore[arg-type]

    @classmethod
    def create(cls, build_dir: Path | str = '') -> LogFile:
        filename = cls.get_log_filename(build_dir=build_dir, name=cls.name)
        return cls(filename)


class BuildLogFile(LogFile):
    """Save logs from the building."""
    name = 'build'


class HandlerLogFile(LogFile):
    """Save output from a device."""
    name = 'handler'


class DeviceLogFile(LogFile):
    """Save errors during flashing onto device."""
    name = 'device'


class NullLogFile(LogFile):
    """Placeholder for no initialized log file"""
    def handle(self, data: str | bytes) -> None:
        """This method does nothing."""
