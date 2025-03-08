# Copyright (c) 2022 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

'''
Common code used when logging that is needed by multiple modules.
'''

import logging
import os
import platform
import shlex

_WINDOWS = platform.system() == 'Windows'


logger = logging.getLogger("twister")
logger.setLevel(logging.DEBUG)

def log_command(logger, msg, args):
    '''Platform-independent helper for logging subprocess invocations.
    Will log a command string that can be copy/pasted into a POSIX
    shell on POSIX platforms. This is not available on Windows, so
    the entire args array is logged instead.

    :param logger: logging.Logger to use
    :param msg: message to associate with the command
    :param args: argument list as passed to subprocess module
    '''
    msg = f'{msg}: %s'
    if _WINDOWS:
        logger.debug(msg, str(args))
    else:
        logger.debug(msg, shlex.join(args))

def setup_logging(outdir, log_file, log_level, timestamps):
    # create file handler which logs even debug messages
    if log_file:
        file_handler = logging.FileHandler(log_file)
    else:
        file_handler = logging.FileHandler(os.path.join(outdir, "twister.log"))

    file_handler.setLevel(logging.DEBUG)

    # create console handler with a higher log level
    console_handler = logging.StreamHandler()
    console_handler.setLevel(getattr(logging, log_level))

    # create formatter and add it to the handlers
    if timestamps:
        formatter = logging.Formatter("%(asctime)s - %(levelname)s - %(message)s")
    else:
        formatter = logging.Formatter("%(levelname)-7s - %(message)s")

    formatter_file = logging.Formatter(
        "%(asctime)s - %(name)s - %(levelname)s - %(message)s"
    )
    console_handler.setFormatter(formatter)
    file_handler.setFormatter(formatter_file)

    # add the handlers to logger
    logger.addHandler(console_handler)
    logger.addHandler(file_handler)
