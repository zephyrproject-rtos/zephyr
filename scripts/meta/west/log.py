# Copyright 2018 Open Source Foundries Limited.
#
# SPDX-License-Identifier: Apache-2.0

'''Logging module for west

Provides common methods for logging messages to display to the user.'''

import colorama
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


def inf(*args, colorize=False):
    '''Print an informational message.

    colorize (default: False):
      If True, the message is printed in bright green if stdout is a terminal.
    '''
    # This approach colorizes any sep= and end= text too, as expected.
    #
    # colorama automatically strips the ANSI escapes when stdout isn't a
    # terminal (by wrapping sys.stdout).
    if colorize:
        print(colorama.Fore.LIGHTGREEN_EX, end='')

    print(*args)

    if colorize:
        # The final flush=True avoids issues with unrelated output from
        # commands (usually Git) becoming green, due to the final attribute
        # reset ANSI escape getting line-buffered.
        print(colorama.Style.RESET_ALL, end='', flush=True)


def wrn(*args):
    '''Print a warning.'''
    print(colorama.Fore.LIGHTRED_EX + 'WARNING: ', end='', file=sys.stderr)
    print(*args, file=sys.stderr)
    print(colorama.Style.RESET_ALL, end='', file=sys.stderr, flush=True)


def err(*args, fatal=False):
    '''Print an error.'''
    print(colorama.Fore.LIGHTRED_EX
              + ('FATAL ERROR: ' if fatal else 'ERROR: '),
          end='', file=sys.stderr)
    print(*args, file=sys.stderr)
    print(colorama.Style.RESET_ALL, end='', file=sys.stderr, flush=True)


def die(*args, exit_code=1):
    '''Print a fatal error, and abort with the given exit code.'''
    err(*args, fatal=True)
    sys.exit(exit_code)
