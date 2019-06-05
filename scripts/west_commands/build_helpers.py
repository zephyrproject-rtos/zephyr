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
from west.configuration import config
from west.util import escapes_directory

DEFAULT_BUILD_DIR = 'build'
'''Name of the default Zephyr build directory.'''

DEFAULT_CMAKE_GENERATOR = 'Ninja'
'''Name of the default CMake generator.'''

FIND_BUILD_DIR_DESCRIPTION = '''\
If not given, the default build directory ({}/ unless the
build.dir-fmt configuration variable is set) and the current directory are
checked, in that order. If one is a Zephyr build directory, it is used.
'''.format(DEFAULT_BUILD_DIR)

def _resolve_build_dir(fmt, cwd, **kwargs):
    # Remove any None values, we do not want 'None' as a string
    kwargs = {k: v for k, v in kwargs.items() if v is not None}
    # Check if source_dir is below cwd first
    source_dir = kwargs.get('source_dir')
    if source_dir:
        if escapes_directory(cwd, source_dir):
            kwargs['source_dir'] = os.path.relpath(source_dir, cwd)
        else:
            # no meaningful relative path possible
            kwargs['source_dir'] = ''

    return fmt.format(**kwargs)

def find_build_dir(dir, **kwargs):
    '''Heuristic for finding a build directory.

    The default build directory is computed by reading the build.dir-fmt
    configuration option, defaulting to DEFAULT_BUILD_DIR if not set. It might
    be None if the build.dir-fmt configuration option is set but cannot be
    resolved.
    If the given argument is truthy, it is returned. Otherwise, if
    the default build folder is a build directory, it is returned.
    Next, if the current working directory is a build directory, it is
    returned. Finally, the default build directory is returned (may be None).
    '''

    if dir:
        build_dir = dir
    else:
        cwd = os.getcwd()
        default = config.get('build', 'dir-fmt', fallback=DEFAULT_BUILD_DIR)
        try:
            default = _resolve_build_dir(default, cwd, **kwargs)
            log.dbg('config dir-fmt: {}'.format(default),
                    level=log.VERBOSE_EXTREME)
        except KeyError:
            default = None
        if default and is_zephyr_build(default):
            build_dir = default
        elif is_zephyr_build(cwd):
            build_dir = cwd
        else:
            build_dir = default
    log.dbg('build dir: {}'.format(build_dir), level=log.VERBOSE_EXTREME)
    if build_dir:
        return os.path.abspath(build_dir)
    else:
        return None

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
