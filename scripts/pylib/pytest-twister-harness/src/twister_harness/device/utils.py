# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import contextlib
import logging
import os
import platform
import shlex
import signal
import subprocess
import time

import psutil

_WINDOWS = platform.system() == 'Windows'


def log_command(logger: logging.Logger, msg: str, args: list, level: int = logging.DEBUG) -> None:
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


def terminate_process(proc: subprocess.Popen) -> None:
    """
    Try to terminate provided process and all its subprocesses recursively.
    """
    with contextlib.suppress(ProcessLookupError, psutil.NoSuchProcess):
        for child in psutil.Process(proc.pid).children(recursive=True):
            with contextlib.suppress(ProcessLookupError, psutil.NoSuchProcess):
                os.kill(child.pid, signal.SIGTERM)
    proc.terminate()
    # sleep for a while before attempting to kill
    time.sleep(0.5)
    proc.kill()
