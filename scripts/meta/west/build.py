# Copyright 2018 (c) Foundries.io.
#
# SPDX-License-Identifier: Apache-2.0

'''Common definitions for building Zephyr applications.

This provides some default settings and convenience wrappers for
building Zephyr applications needed by multiple commands.

See west.cmd.build for the build command itself.
'''

from west import cmake
from west import log

DEFAULT_BUILD_DIR = 'build'
'''Name of the default Zephyr build directory.'''

DEFAULT_CMAKE_GENERATOR = 'Ninja'
'''Name of the default CMake generator.'''


def is_zephyr_build(path):
    '''Return true if and only if `path` appears to be a valid Zephyr
    build directory.

    "Valid" means the given path is a directory which contains a CMake
    cache with a 'ZEPHYR_TOOLCHAIN_VARIANT' key.
    '''
    try:
        cache = cmake.CMakeCache.from_build_dir(path)
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
