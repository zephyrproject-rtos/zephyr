# Copyright 2018 (c) Foundries.io.
#
# SPDX-License-Identifier: Apache-2.0

'''Common definitions for building Zephyr applications.

This provides some default settings and convenience wrappers for
building Zephyr applications needed by multiple commands.

See build.py for the build command itself.
'''

import logging
import os
import sys
from pathlib import Path

from west.commands import Verbosity
from west.configuration import Configuration
from west.util import WestNotFound, escapes_directory, west_topdir

import zcmake

# Explicit, flat name: avoids colliding with scripts/pylib/build_helpers/
# domains.py (which grabs `logging.getLogger('build_helpers')` at import
# time).
BUILD_HELPERS_LOGGER = 'zephyr_build_helpers'
_logger = logging.getLogger(BUILD_HELPERS_LOGGER)


class WestLogFormatter(logging.Formatter):

    def __init__(self):
        # Match the format used by west's own LogFormatter
        # (west/app/main.py) so bridged records render identically to
        # records emitted by west itself.
        super().__init__(fmt='%(name)s: %(levelname)s: %(message)s')


class WestLogHandler(logging.Handler):
    '''Forwards Python logging records to a WestCommand's logging methods.

    Module-level ``logging`` calls bypass west's verbosity controls and
    colorized output. Routing them through the command's ``inf``/``wrn``/
    ``err``/``dbg`` methods keeps formatting consistent and makes records
    obey ``west -v`` / ``west -vv``.

    Not provided by west itself, but could land in the future,
    see https://github.com/zephyrproject-rtos/west/issues/952
    '''

    def __init__(self, command, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self._command = command
        self.setFormatter(WestLogFormatter())

    def emit(self, record):
        fmt = self.format(record)
        lvl = record.levelno
        if lvl > logging.CRITICAL:
            self._command.die(fmt)
        elif lvl >= logging.ERROR:
            self._command.err(fmt)
        elif lvl >= logging.WARNING:
            self._command.wrn(fmt)
        elif lvl >= logging.INFO:
            self._command.inf(fmt)
        elif lvl >= logging.DEBUG:
            self._command.dbg(fmt)
        else:
            self._command.dbg(fmt, level=Verbosity.DBG_EXTREME)


def forward_logging_to_west(command, logger_names):
    '''Forward records from the given Python loggers to ``command``'s
    logging methods, so module-level debug output is visible under
    ``west -v`` / ``west -vv``.

    ``logger_names`` may be a single logger name or an iterable of names.

    This simply adds a custom handler to each logger. If you forget to
    forward to any West command, then Python's ``logging.lastResort``
    should still print warnings and higher to stderr while discarding
    lower priorities, see the
    https://docs.python.org/3/library/logging.html HOWTO for details.

    While not tested, you could in theory forward to different West
    commands at the same time. Forwarding multiple times to the _same_
    West command will print an error and be ignored.
    '''
    if isinstance(logger_names, str):
        logger_names = [logger_names]
    for name in logger_names:
        logger = logging.getLogger(name)
        _fwd_logger(logger, command)


def _fwd_logger(logger, west_cmd):

    # Note logger.handlers are only the _directly_ attached handlers,
    # ignoring propagation up
    for handler in logger.handlers:
        match handler:
            case WestLogHandler():
                prev_cmd = handler._command
                hdlr_print = f"WestLogHandler({prev_cmd.name})"
                if prev_cmd == west_cmd:
                    west_cmd.err(
                        f"logger('{logger.name}') was already forwarded to {hdlr_print}!"
                    )
                    return
            case _:
                hdlr_print = f"{handler}"

        west_cmd.dbg(f"logger('{logger.name}') already had handler: {hdlr_print}")

    # Drop DEBUG records at the logger unless the user asked for them,
    # so they don't reach attached handlers (ours or anyone else's).
    logger.setLevel(1 if west_cmd.verbosity >= Verbosity.DBG else logging.INFO)

    logger.addHandler(WestLogHandler(west_cmd))
    # Use immediately.  This starts with "logger %(name)s" thanks
    # to WestLogHandler.__init__() above
    logger.debug(f"forwarded to WestLogHandler({west_cmd.name}). propagate={logger.propagate}")


# Domains.py must be imported from the pylib directory, since
# twister also uses the implementation
script_dir = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
sys.path.insert(0, os.path.join(script_dir, "pylib/build_helpers/"))
from domains import Domains  # noqa: E402

DEFAULT_BUILD_DIR = 'build'
'''Name of the default Zephyr build directory.'''

DEFAULT_CMAKE_GENERATOR = 'Ninja'
'''Name of the default CMake generator.'''

FIND_BUILD_DIR_DESCRIPTION = f'''\
If the build directory is not given, the default is {DEFAULT_BUILD_DIR}/ unless the
build.dir-fmt configuration variable is set. The current directory is
checked after that. If either is a Zephyr build directory, it is used.
'''

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

def find_build_dir(dir, guess=False, *, config=None, **kwargs):
    '''Heuristic for finding a build directory.
    If `dir` is specified, this directory is returned as the build directory.
    Otherwise, the default build directory is determined according to the
    following priorities:
    1. Resolved `build.dir-fmt` configuration option (all {args} are resolvable).
       Return this directory, if it is an already existing build directory.
    2. The current working directory, if it is an existing build directory.
    3. Resolved `build.dir-fmt` configuration option, no matter if it is an
       already existing build directory.
    4. DEFAULT_BUILD_DIR

    `config` is a west.configuration.Configuration object. When None, one is
    instantiated from the current west workspace; if we are not inside a
    workspace, `build.dir-fmt` lookup is skipped.
    '''

    build_dir = dir
    cwd = os.getcwd()
    if config is None:
        try:
            config = Configuration(topdir=west_topdir())
        except WestNotFound:
            config = None
    dir_fmt = config.get('build.dir-fmt', default=None) if config is not None else None
    if dir_fmt:
        _logger.debug('config dir-fmt: %s', dir_fmt)
        dir_fmt = _resolve_build_dir(dir_fmt, guess, cwd, **kwargs)
    if not build_dir and is_zephyr_build(dir_fmt):
        build_dir = dir_fmt
    if not build_dir and is_zephyr_build(cwd):
        build_dir = cwd
    if not build_dir and dir_fmt:
        build_dir = dir_fmt
    if not build_dir:
        build_dir = DEFAULT_BUILD_DIR
    _logger.debug('build dir: %s', build_dir)
    return os.path.abspath(build_dir)

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
    if not path:
        return False

    try:
        cache = zcmake.CMakeCache.from_build_dir(path)
    except FileNotFoundError:
        cache = {}

    if 'ZEPHYR_BASE' in cache or 'ZEPHYR_TOOLCHAIN_VARIANT' in cache:
        _logger.debug('%s is a zephyr build directory', path)
        return True

    _logger.debug('%s is NOT a valid zephyr build directory', path)
    return False


def load_domains(path):
    '''Load domains from a domains.yaml.

    If domains.yaml is not found, then a single 'app' domain referring to the
    top-level build folder is created and returned.
    '''
    domains_file = Path(path) / 'domains.yaml'

    if not domains_file.is_file():
        default_domains = {
            "default": "app",
            "build_dir": path,
            "domains": [
                {
                    "name": "app",
                    "build_dir": path
                }
            ],
            "flash_order": ["app"]
        }
        return Domains(default_domains)

    return Domains.from_file(domains_file)
