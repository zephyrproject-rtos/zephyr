# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""Unit tests for the dtsh.builtins.pwd module."""

# Relax pylint a bit for unit tests.
# pylint: disable=missing-function-docstring


from dtsh.io import DTShOutput
from dtsh.shell import DTSh
from dtsh.builtins.pwd import DTShBuiltinPwd

from .dtsh_uthelpers import DTShTests


def test_dtsh_builtin_pwd() -> None:
    cmd = DTShBuiltinPwd()
    DTShTests.check_cmd_meta(cmd, "pwd")


def test_dtsh_builtin_pwd_execute() -> None:
    out = DTShOutput()
    cmd = DTShBuiltinPwd()
    sh = DTSh(DTShTests.get_sample_dtmodel(), [cmd])

    DTShTests.check_cmd_execute(cmd, sh, out)
