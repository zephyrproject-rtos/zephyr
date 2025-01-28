# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""Unit tests for the dtsh.builtins.cat module."""

# Relax pylint a bit for unit tests.
# pylint: disable=missing-function-docstring


from dtsh.io import DTShOutput
from dtsh.shell import DTSh
from dtsh.builtins.cat import DTShBuiltinCat

from .dtsh_uthelpers import DTShTests

_stdout = DTShOutput()


def test_dtsh_builtin_cat() -> None:
    cmd = DTShBuiltinCat()
    DTShTests.check_cmd_meta(cmd, "cat")


def test_dtsh_builtin_cat_flags() -> None:
    cmd = DTShBuiltinCat()
    sh = DTSh(DTShTests.get_sample_dtmodel(), [cmd])
    DTShTests.check_cmd_flags(cmd, sh, _stdout)


def test_dtsh_builtin_cat_execute() -> None:
    out = DTShOutput()
    cmd = DTShBuiltinCat()
    sh = DTSh(DTShTests.get_sample_dtmodel(), [cmd])

    DTShTests.check_cmd_execute(cmd, sh, out)
