# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""Unit tests for the dtsh.builtins.find module."""

# Relax pylint a bit for unit tests.
# pylint: disable=missing-function-docstring


from typing import List

from dtsh.io import DTShOutput
from dtsh.shell import DTSh
from dtsh.shellutils import DTSH_ARG_NODE_CRITERIA, DTShArgCriterion
from dtsh.builtins.find import DTShBuiltinFind

from .dtsh_uthelpers import DTShTests

_stdout = DTShOutput()


def test_dtsh_builtin_find() -> None:
    cmd = DTShBuiltinFind()
    DTShTests.check_cmd_meta(cmd, "find")


def test_dtsh_builtin_find_flags() -> None:
    cmd = DTShBuiltinFind()
    sh = DTSh(DTShTests.get_sample_dtmodel(), [cmd])
    DTShTests.check_cmd_flags(cmd, sh, _stdout)


def test_dtsh_builtin_find_arg_orderby() -> None:
    cmd = DTShBuiltinFind()
    sh = DTSh(DTShTests.get_sample_dtmodel(), [cmd])
    DTShTests.check_cmd_arg_order_by(cmd, sh, _stdout)


def test_dtsh_builtin_find_arg_longfmt() -> None:
    cmd = DTShBuiltinFind()
    sh = DTSh(DTShTests.get_sample_dtmodel(), [cmd])
    DTShTests.check_cmd_arg_longfmt(cmd, sh, _stdout)


def test_dtsh_builtin_find_arg_criteria() -> None:
    cmd = DTShBuiltinFind()
    sh = DTSh(DTShTests.get_sample_dtmodel(), [cmd])

    args: List[DTShArgCriterion] = [
        cmd.option(
            f"-{opt.sh}" if opt.shortname else f"--{opt.longname}"  # type: ignore
        )
        for opt in DTSH_ARG_NODE_CRITERIA
    ]

    for arg in args:
        assert not arg.isset
        assert not arg.get_criterion()

    argv: List[str] = []
    for opt in args:
        argv.append(
            f"-{opt.shortname}" if opt.shortname else f"--{opt.longname}"
        )
        # "*" is valid for both text-based and int-based criteria.
        argv.append("*")

    cmd.execute(argv, sh, _stdout)

    for arg in args:
        assert arg.isset
        assert arg.get_criterion()


def test_dtsh_builtin_find_param() -> None:
    cmd = DTShBuiltinFind()
    sh = DTSh(DTShTests.get_sample_dtmodel(), [cmd])
    DTShTests.check_cmd_param_dtpaths(cmd, sh, _stdout)


def test_dtsh_builtin_find_execute() -> None:
    cmd = DTShBuiltinFind()
    sh = DTSh(DTShTests.get_sample_dtmodel(), [cmd])

    DTShTests.check_cmd_execute(cmd, sh, _stdout)
