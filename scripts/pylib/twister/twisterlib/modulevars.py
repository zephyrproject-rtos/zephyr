# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

"""Expansion of Zephyr module directory / name variables in path strings.

These mirror the CMake variables that the build system defines for every
Zephyr module, so the same ${ZEPHYR_<MODULE>_MODULE_DIR} spelling can be used
in both CMake and Twister test configuration contexts.
"""

import os
import re
from functools import cache

import zephyr_module
from twisterlib.constants import ZEPHYR_BASE

# Matches ${NAME} or $NAME references used when expanding path variables.
_VAR_REF_RE = re.compile(r'\$\{(?P<braced>\w+)\}|\$(?P<bare>\w+)')


@cache
def get_module_dir_vars() -> dict[str, str]:
    """Return a mapping of Zephyr module directory / name variables.

    For every discovered Zephyr module this provides the
    ZEPHYR_<MODULE>_MODULE_DIR and ZEPHYR_<MODULE>_MODULE_NAME entries,
    mirroring the CMake variables defined for each module in
    cmake/modules/zephyr_module.cmake. The module name is sanitized and
    upper-cased exactly as the build system does, so the same
    ${ZEPHYR_<MODULE>_MODULE_DIR} spelling works in both CMake and
    Twister contexts.
    """
    module_vars = {}
    for module in zephyr_module.parse_modules(ZEPHYR_BASE):
        name_upper = module.meta['name-sanitized'].upper()
        module_vars[f'ZEPHYR_{name_upper}_MODULE_DIR'] = os.path.normpath(module.project)
        module_vars[f'ZEPHYR_{name_upper}_MODULE_NAME'] = module.meta['name']
    return module_vars


def expand_zephyr_vars(path: str) -> str:
    """Expand Zephyr module directory / name variables in a path.

    Substitutes ZEPHYR_<MODULE>_MODULE_DIR and ZEPHYR_<MODULE>_MODULE_NAME,
    which are CMake variables not present in the process environment and are
    therefore not handled by os.path.expandvars. Unknown references are left
    untouched. Environment variables should be expanded with os.path.expandvars
    before calling this function.
    """
    if '$' not in path:
        return path

    module_vars = get_module_dir_vars()

    def replace(match: re.Match) -> str:
        name = match.group('braced') or match.group('bare')
        return module_vars.get(name, match.group(0))

    return _VAR_REF_RE.sub(replace, path)
