# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""Unit tests for the dtsh.builtins.ls module."""

# Relax pylint a bit for unit tests.
# pylint: disable=missing-function-docstring


from dtsh.io import DTShOutput
from dtsh.shell import DTSh
from dtsh.builtins.ls import DTShBuiltinLs

from .dtsh_uthelpers import DTShTests

_stdout = DTShOutput()


def test_dtsh_builtin_ls() -> None:
    cmd = DTShBuiltinLs()
    DTShTests.check_cmd_meta(cmd, "ls")


def test_dtsh_builtin_ls_flags() -> None:
    cmd = DTShBuiltinLs()
    sh = DTSh(DTShTests.get_sample_dtmodel(), [cmd])
    DTShTests.check_cmd_flags(cmd, sh, _stdout)


def test_dtsh_builtin_ls_arg_orderby() -> None:
    cmd = DTShBuiltinLs()
    sh = DTSh(DTShTests.get_sample_dtmodel(), [cmd])
    DTShTests.check_cmd_arg_order_by(cmd, sh, _stdout)


def test_dtsh_builtin_ls_arg_longfmt() -> None:
    cmd = DTShBuiltinLs()
    sh = DTSh(DTShTests.get_sample_dtmodel(), [cmd])
    DTShTests.check_cmd_arg_longfmt(cmd, sh, _stdout)


def test_dtsh_builtin_ls_arg_fixed_depth() -> None:
    cmd = DTShBuiltinLs()
    sh = DTSh(DTShTests.get_sample_dtmodel(), [cmd])
    DTShTests.check_cmd_arg_fixed_depth(cmd, sh, _stdout)


def test_dtsh_builtin_ls_param() -> None:
    cmd = DTShBuiltinLs()
    sh = DTSh(DTShTests.get_sample_dtmodel(), [cmd])
    DTShTests.check_cmd_param_dtpaths(cmd, sh, _stdout)


def test_dtsh_builtin_ls_execute() -> None:
    out = DTShOutput()
    cmd = DTShBuiltinLs()
    sh = DTSh(DTShTests.get_sample_dtmodel(), [cmd])

    DTShTests.check_cmd_execute(cmd, sh, out)
