"""
Warnings filter extension
#########################

Copyright (c) 2021 Nordic Semiconductor ASA
SPDX-License-Identifier: Apache-2.0

Introduction
============

This Sphinx plugin can be used to filter out warnings that are known to be false
positives. The warnings are filtered out based on a set of regular expressions
given via an configuration file. The format of the configuration file is a
plain-text file where each line consists of a regular expression. Any lines
starting with ``#`` will be ignored.

Configuration options
=====================

- ``warnings_filter_config``: Configuration file.
- ``warnings_filter_silent``: Silent flag. If True, warning is hidden. If False
  the warning is converted to an information message and displayed.
"""

import logging
import re
from typing import Dict, Any, List

from sphinx.application import Sphinx
from sphinx.util.logging import NAMESPACE


__version__ = "0.1.0"


class WarningsFilter(logging.Filter):
    """Warnings filter.

    Args:
        expressions: List of regular expressions.
        silent: If true, warning is hidden, otherwise it is shown as INFO.
        name: Filter name.
    """
    def __init__(self, expressions: List[str], silent: bool, name: str = "") -> None:
        super().__init__(name)

        self._expressions = expressions
        self._silent = silent

    def filter(self, record: logging.LogRecord) -> bool:
        if record.levelno != logging.WARNING:
            return True

        for expression in self._expressions:
            if re.match(expression, record.msg):
                if self._silent:
                    return False
                else:
                    record.levelno = logging.INFO
                    record.msg = f"Filtered warning: {record.msg}"
                    return True

        return True


def configure(app: Sphinx) -> None:
    """Entry point.

    Args:
        app: Sphinx application instance.
    """

    # load expressions from configuration file
    with open(app.config.warnings_filter_config) as f:
        expressions = list()
        for line in f.readlines():
            if not line.startswith("#"):
                expressions.append(line.rstrip())

    # install warnings filter to all the Sphinx logger handlers
    filter = WarningsFilter(expressions, app.config.warnings_filter_silent)
    logger = logging.getLogger(NAMESPACE)
    for handler in logger.handlers:
        handler.filters.insert(0, filter)


def setup(app: Sphinx) -> Dict[str, Any]:
    app.add_config_value("warnings_filter_config", "", "")
    app.add_config_value("warnings_filter_silent", True, "")

    app.connect("builder-inited", configure)

    return {
        "version": __version__,
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
