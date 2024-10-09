# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""Unit tests for the dtsh.builtins.tree module."""

# Relax pylint a bit for unit tests.
# pylint: disable=missing-function-docstring

from dtsh.io import DTShOutput
from dtsh.shell import DTSh
from dtsh.builtins.tree import DTShBuiltinTree

from .dtsh_uthelpers import DTShTests

_stdout = DTShOutput()


def test_dtsh_builtin_tree() -> None:
    cmd = DTShBuiltinTree()
    DTShTests.check_cmd_meta(cmd, "tree")


def test_dtsh_builtin_tree_flags() -> None:
    cmd = DTShBuiltinTree()
    sh = DTSh(DTShTests.get_sample_dtmodel(), [cmd])
    DTShTests.check_cmd_flags(cmd, sh, _stdout)


def test_dtsh_builtin_tree_arg_orderby() -> None:
    cmd = DTShBuiltinTree()
    sh = DTSh(DTShTests.get_sample_dtmodel(), [cmd])
    DTShTests.check_cmd_arg_order_by(cmd, sh, _stdout)


def test_dtsh_builtin_tree_arg_longfmt() -> None:
    cmd = DTShBuiltinTree()
    sh = DTSh(DTShTests.get_sample_dtmodel(), [cmd])
    DTShTests.check_cmd_arg_longfmt(cmd, sh, _stdout)


def test_dtsh_builtin_tree_arg_fixed_depth() -> None:
    cmd = DTShBuiltinTree()
    sh = DTSh(DTShTests.get_sample_dtmodel(), [cmd])
    DTShTests.check_cmd_arg_fixed_depth(cmd, sh, _stdout)


def test_dtsh_builtin_tree_param() -> None:
    cmd = DTShBuiltinTree()
    sh = DTSh(DTShTests.get_sample_dtmodel(), [cmd])
    DTShTests.check_cmd_param_dtpaths(cmd, sh, _stdout)


def test_dtsh_builtin_tree_execute() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    cmd = DTShBuiltinTree()
    sh = DTSh(dtmodel, [cmd])

    DTShTests.check_cmd_execute(cmd, sh, _stdout)
