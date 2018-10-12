# Copyright 2018 Open Source Foundries Limited.
#
# SPDX-License-Identifier: Apache-2.0

'''Miscellaneous utilities used by west
'''

import os
import shlex
import textwrap


def quote_sh_list(cmd):
    '''Transform a command from list into shell string form.'''
    fmt = ' '.join('{}' for _ in cmd)
    args = [shlex.quote(s) for s in cmd]
    return fmt.format(*args)


def wrap(text, indent):
    '''Convenience routine for wrapping text to a consistent indent.'''
    return textwrap.wrap(text, initial_indent=indent,
                         subsequent_indent=indent)


class WestNotFound(RuntimeError):
    '''Neither the current directory nor any parent has a West installation.'''


def west_dir(start=None):
    '''Returns the absolute path of the west/ top level directory.

    Starts the search from the start directory, and goes to its
    parents. If the start directory is not specified, the current
    directory is used.

    Raises WestNotFound if no west top-level directory is found.
    '''
    return os.path.join(west_topdir(start), 'west')


def west_topdir(start=None):
    '''
    Like west_dir(), but returns the path to the parent directory of the west/
    directory instead, where project repositories are stored
    '''
    # If you change this function, make sure to update the bootstrap
    # script's find_west_topdir().

    if start is None:
        cur_dir = os.getcwd()
    else:
        cur_dir = start

    while True:
        if os.path.isfile(os.path.join(cur_dir, 'west', '.west_topdir')):
            return cur_dir

        parent_dir = os.path.dirname(cur_dir)
        if cur_dir == parent_dir:
            # At the root
            raise WestNotFound('Could not find a West installation '
                               'in this or any parent directory')
        cur_dir = parent_dir


def in_multirepo_install(start=None):
    '''Returns True iff the path ``start`` is in a multi-repo installation.

    If start is not given, it defaults to the current working directory.

    This is equivalent to checking if west_dir() raises an exception
    when given the same start kwarg.
    '''
    try:
        west_topdir(start)
        result = True
    except WestNotFound:
        result = False
    return result
