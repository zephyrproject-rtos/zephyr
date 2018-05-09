# Copyright 2018 Open Source Foundries Limited.
#
# SPDX-License-Identifier: Apache-2.0

'''Logging module for west

Provides common methods for logging messages to display to the user.'''

import sys

VERBOSE_NONE = 0
'''Base verbosity level (zero), no verbose messages printed.'''

VERBOSE_NORMAL = 1
'''Base verbosity level, some verbose messages printed.'''

VERBOSE_VERY = 2
'''Very verbose output messages will be printed.'''

VERBOSE_EXTREME = 3
'''Extremely verbose output messages will be printed.'''

VERBOSE = VERBOSE_NONE
'''Global verbosity level. VERBOSE_NONE is the default.'''


def set_verbosity(value):
    '''Set the logging verbosity level.'''
    global VERBOSE
    VERBOSE = int(value)


def dbg(*args, level=VERBOSE_NORMAL):
    '''Print a verbose debug logging message.

    The message is only printed if level is at least the current
    verbosity level.'''
    if level > VERBOSE:
        return
    print(*args)


def inf(*args):
    '''Print an informational message.'''
    print(*args)


def wrn(*args):
    '''Print a warning.'''
    print('warning:', end=' ', file=sys.stderr, flush=False)
    print(*args, file=sys.stderr)


def err(*args, fatal=False):
    '''Print an error.'''
    if fatal:
        print('fatal', end=' ', file=sys.stderr, flush=False)
    print('error:', end=' ', file=sys.stderr, flush=False)
    print(*args, file=sys.stderr)


def die(*args, exit_code=1):
    '''Print a fatal error, and abort with the given exit code.'''
    print('fatal error:', end=' ', file=sys.stderr, flush=False)
    print(*args, file=sys.stderr)
    sys.exit(exit_code)
