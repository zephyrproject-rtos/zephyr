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
import sys
from pathlib import Path
from west import log
from west.configuration import config
from west.util import escapes_directory

# Domains.py must be imported from the pylib directory, since
# twister also uses the implementation
script_dir = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
sys.path.insert(0, os.path.join(script_dir, "pylib/build_helpers/"))
from domains import Domains

DEFAULT_BUILD_DIR = 'build'
'''Name of the default Zephyr build directory.'''

DEFAULT_CMAKE_GENERATOR = 'Ninja'
'''Name of the default CMake generator.'''

FIND_BUILD_DIR_DESCRIPTION = '''\
If the build directory is not given, the default is {}/ unless the
build.dir-fmt configuration variable is set. The current directory is
checked after that. If either is a Zephyr build directory, it is used.
'''.format(DEFAULT_BUILD_DIR)

def _resolve_build_dir(fmt, guess, cwd, **kwargs):
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

    try:
        return fmt.format(**kwargs)
    except KeyError:
        if not guess:
            return None

    # Guess the build folder by iterating through all sub-folders from the
    # root of the format string and trying to resolve. If resolving fails,
    # proceed to iterate over subfolders only if there is a single folder
    # present on each iteration.
    parts = Path(fmt).parts
    b = Path('.')
    for p in parts:
        # default to cwd in the first iteration
        curr = b
        b = b.joinpath(p)
        try:
            # if fmt is an absolute path, the first iteration will always
            # resolve '/'
            b = Path(str(b).format(**kwargs))
        except KeyError:
            # Missing key, check sub-folders and match if a single one exists
            while True:
                if not curr.exists():
                    return None
                dirs = [f for f in curr.iterdir() if f.is_dir()]
                if len(dirs) != 1:
                    return None
                curr = dirs[0]
                if is_zephyr_build(str(curr)):
                    return str(curr)
    return str(b)

def find_build_dir(dir, guess=False, **kwargs):
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
        default = _resolve_build_dir(default, guess, cwd, **kwargs)
        log.dbg('config dir-fmt: {}'.format(default), level=log.VERBOSE_EXTREME)
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
    cache with a 'ZEPHYR_BASE' or 'ZEPHYR_TOOLCHAIN_VARIANT' variable.

    (The check for ZEPHYR_BASE introduced sometime after Zephyr 2.4 to
    fix https://github.com/zephyrproject-rtos/zephyr/issues/28876; we
    keep support for the second variable around for compatibility with
    versions 2.2 and earlier, which didn't have ZEPHYR_BASE in cache.
    The cached ZEPHYR_BASE was added in
    https://github.com/zephyrproject-rtos/zephyr/pull/23054.)
    '''
    try:
        cache = zcmake.CMakeCache.from_build_dir(path)
    except FileNotFoundError:
        cache = {}

    if 'ZEPHYR_BASE' in cache or 'ZEPHYR_TOOLCHAIN_VARIANT' in cache:
        log.dbg(f'{path} is a zephyr build directory',
                level=log.VERBOSE_EXTREME)
        return True

    log.dbg(f'{path} is NOT a valid zephyr build directory',
            level=log.VERBOSE_EXTREME)
    return False


def load_domains(path):
    '''Load domains from a domains.yaml.

    If domains.yaml is not found, then a single 'app' domain referring to the
    top-level build folder is created and returned.
    '''
    domains_file = Path(path) / 'domains.yaml'

    if not domains_file.is_file():
        return Domains.from_yaml(f'''\
default: app
build_dir: {path}
domains:
  - name: app
    build_dir: {path}
flash_order:
  - app
''')

    return Domains.from_file(domains_file)
