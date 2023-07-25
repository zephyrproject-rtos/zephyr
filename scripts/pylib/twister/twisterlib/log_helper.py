# Copyright (c) 2022 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

'''
Common code used when logging that is needed by multiple modules.
'''

import platform
import shlex

_WINDOWS = (platform.system() == 'Windows')

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
