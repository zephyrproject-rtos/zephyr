# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import logging
import os.path
import platform
import shlex

_WINDOWS = platform.system() == 'Windows'

logger = logging.getLogger(__name__)


def log_command(logger: logging.Logger, msg: str, args: list, level: int = logging.DEBUG):
    """
    Platform-independent helper for logging subprocess invocations.

    Will log a command string that can be copy/pasted into a POSIX
    shell on POSIX platforms. This is not available on Windows, so
    the entire args array is logged instead.

    :param logger: logging.Logger to use
    :param msg: message to associate with the command
    :param args: argument list as passed to subprocess module
    :param level: log level
    """
    msg = f'{msg}: %s'
    if _WINDOWS:
        logger.log(level, msg, str(args))
    else:
        logger.log(level, msg, shlex.join(args))


def normalize_filename(filename: str) -> str:
    filename = os.path.expanduser(os.path.expandvars(filename))
    filename = os.path.normpath(os.path.abspath(filename))
    return filename
