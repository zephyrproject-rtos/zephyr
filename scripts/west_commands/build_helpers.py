# Copyright 2018 (c) Foundries.io.
#
# SPDX-License-Identifier: Apache-2.0

'''Common definitions for building Zephyr applications.

This provides some default settings and convenience wrappers for
building Zephyr applications needed by multiple commands.

See build.py for the build command itself.
'''

import zcmake
import os
from west import log

DEFAULT_BUILD_DIR = 'build'
'''Name of the default Zephyr build directory.'''

DEFAULT_CMAKE_GENERATOR = 'Ninja'
'''Name of the default CMake generator.'''

BUILD_DIR_DESCRIPTION = '''\
Build directory. If missing and run in a Zephyr build directory, it is
used; otherwise, it's "{}".'''.format(
    DEFAULT_BUILD_DIR)


def find_build_dir(dir):
    '''Heuristic for finding a build directory.

    If the given argument is truthy, it is returned. Otherwise, if
    the current working directory is a build directory, it is
    returned. Otherwise, DEFAULT_BUILD_DIR is returned.'''
    if dir:
        build_dir = dir
    else:
        cwd = os.getcwd()
        if is_zephyr_build(cwd):
            build_dir = cwd
        else:
            build_dir = DEFAULT_BUILD_DIR
    return os.path.abspath(build_dir)


def is_zephyr_build(path):
    '''Return true if and only if `path` appears to be a valid Zephyr
    build directory.

    "Valid" means the given path is a directory which contains a CMake
    cache with a 'ZEPHYR_TOOLCHAIN_VARIANT' key.
    '''
    try:
        cache = zcmake.CMakeCache.from_build_dir(path)
    except FileNotFoundError:
        cache = {}

    if 'ZEPHYR_TOOLCHAIN_VARIANT' in cache:
        log.dbg('{} is a zephyr build directory'.format(path),
                level=log.VERBOSE_EXTREME)
        return True
    else:
        log.dbg('{} is NOT a valid zephyr build directory'.format(path),
                level=log.VERBOSE_EXTREME)
        return False
