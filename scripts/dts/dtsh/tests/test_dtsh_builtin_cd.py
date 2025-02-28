# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""Unit tests for the dtsh.builtins.cd module."""

# Relax pylint a bit for unit tests.
# pylint: disable=missing-function-docstring


from dtsh.io import DTShOutput
from dtsh.shell import DTSh
from dtsh.builtins.cd import DTShBuiltinCd

from .dtsh_uthelpers import DTShTests

_stdout = DTShOutput()


def test_dtsh_builtin_cd() -> None:
    cmd = DTShBuiltinCd()
    DTShTests.check_cmd_meta(cmd, "cd")


def test_dtsh_builtin_cd_param() -> None:
    cmd = DTShBuiltinCd()
    sh = DTSh(DTShTests.get_sample_dtmodel(), [cmd])
    DTShTests.check_cmd_param_dtpath(cmd, sh, _stdout)


def test_dtsh_builtin_cd_execute() -> None:
    cmd = DTShBuiltinCd()
    sh = DTSh(DTShTests.get_sample_dtmodel(), [cmd])

    DTShTests.check_cmd_execute(cmd, sh, _stdout)
