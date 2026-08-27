# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

"""Implementation for a configuration file reader."""

import logging
import os
import re
from pathlib import Path
from typing import Any

__tracebackhide__ = True

logger = logging.getLogger(__name__)


class ConfigReader:
    """Reads configuration from a config file."""

    def __init__(self, config_file: Path | str) -> None:
        """Initialize.

        :param config_file: path to a configuration file
        """
        assert os.path.exists(config_file), f"Path does not exist: {config_file}"
        assert os.path.isfile(config_file), f"It is not a file: {config_file}"
        self.config_file = config_file
        self.config: dict[str, str] = {}
        self.parse()

    def parse(self) -> dict[str, str]:
        """Parse a config file."""
        pattern = re.compile(r"^(?P<key>.+)=(?P<value>.+)$")
        with open(self.config_file) as file:
            for line in file:
                if match := pattern.match(line):
                    key, value = match.group("key"), match.group("value")
                    self.config[key] = value.strip("\"'")
        return self.config

    def read(self, config_key: str, default: Any = None, *, silent=False) -> str | None:
        """Find key in config file.

        :param config_key: key to read
        :param default: default value to return if key not found
        :param silent: do not raise an exception when key not found
        :raises ValueError: if key not found
        """
        try:
            value = self.config[config_key]
        except KeyError:
            if default is not None:
                return default
            logger.debug("Not found key: %s", config_key)
            if silent:
                return None
            raise ValueError(f"Could not find key: {config_key}") from None
        logger.debug("Found matching key: %s=%s", config_key, value)
        return value

    def read_int(self, config_key: str, default: int | None = None) -> int:
        """Find key in config file and return int.

        :param config_key: key to read
        :param default: default value to return if key not found
        """
        if default is not None and not isinstance(default, int):
            raise TypeError(f"default value must be type of int, but was {type(default)}")
        if default is not None:
            default = hex(default)  # type: ignore
        if value := self.read(config_key, default):
            try:
                return int(value)
            except ValueError:
                return int(value, 16)
        raise Exception("Non reachable code")  # pragma: no cover

    def read_bool(self, config_key: str, default: bool | None = None) -> bool:
        """Find key in config file and return bool.

        :param config_key: key to read
        :param default: default value to return if key not found
        """
        value = self.read(config_key, default)
        if isinstance(value, str):
            if value.lower() == "y":
                return True
            if value.lower() == "n":
                return False
        return bool(value)

    def read_hex(self, config_key: str, default: int | None = None) -> str:
        """Find key in config file and return hex.

        :param config_key: key to read
        :param default: default value to return if key not found
        :return: hex value as string
        """
        if default is not None and not isinstance(default, int):
            raise TypeError(f"default value must be type of int, but was {type(default)}")
        value = self.read_int(config_key, default)
        return hex(value)
