# Copyright (c) 2025 Thorsten Klein <thorsten.klein@bshg.com>
# Copyright (c) 2025 Bosch Hausgeräte GmbH
#
# SPDX-License-Identifier: Apache-2.0

"""Common code used by commands or tests"""

import contextlib
import os


@contextlib.contextmanager
def chdir(path):
    """
    Temporarily change the current working directory.
    This context manager changes the current working directory to `path`
    for the duration of the `with` block. After the block exits, the
    working directory is restored to its original value.
    """
    oldpwd = os.getcwd()
    os.chdir(path)
    try:
        yield
    finally:
        os.chdir(oldpwd)


@contextlib.contextmanager
def update_env(env: dict[str, str | None]):
    """
    Temporarily update the process environment variables.
    This context manager updates `os.environ` with the key-value pairs
    provided in the `env` dictionary for the duration of the `with` block.
    The existing environment is preserved and fully restored when the block
    exits. If the value is set to None, the environment variable is unset.
    """
    env_bak = dict(os.environ)
    env_vars = {}
    for k, v in env.items():
        # unset if value is None
        if v is None and k in os.environ:
            del os.environ[k]
        # set env variable to new value only if v is not None
        elif v is not None:
            env_vars[k] = v
    # apply the new environment
    os.environ.update(env_vars)
    try:
        yield
    finally:
        # reset to previous environment
        os.environ.clear()
        os.environ.update(env_bak)
