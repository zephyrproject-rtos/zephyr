# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""Unit tests for the dtsh.builtins.chosen module."""

# Relax pylint a bit for unit tests.
# pylint: disable=missing-function-docstring


from dtsh.io import DTShOutput
from dtsh.shell import DTSh
from dtsh.builtins.chosen import DTShBuiltinChosen

from .dtsh_uthelpers import DTShTests

_stdout = DTShOutput()


def test_dtsh_builtin_chosen() -> None:
    cmd = DTShBuiltinChosen()
    DTShTests.check_cmd_meta(cmd, "chosen")


def test_dtsh_builtin_chosen_flags() -> None:
    cmd = DTShBuiltinChosen()
    sh = DTSh(DTShTests.get_sample_dtmodel(), [cmd])
    DTShTests.check_cmd_flags(cmd, sh, _stdout)


def test_dtsh_builtin_chosen_arg_longfmt() -> None:
    cmd = DTShBuiltinChosen()
    sh = DTSh(DTShTests.get_sample_dtmodel(), [cmd])
    DTShTests.check_cmd_arg_longfmt(cmd, sh, _stdout)


def test_dtsh_builtin_chosen_param() -> None:
    cmd = DTShBuiltinChosen()
    sh = DTSh(DTShTests.get_sample_dtmodel(), [cmd])
    DTShTests.check_cmd_param_chosen(cmd, sh, _stdout)


def test_dtsh_builtin_chosen_execute() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    cmd = DTShBuiltinChosen()
    sh = DTSh(dtmodel, [cmd])

    DTShTests.check_cmd_execute(cmd, sh, _stdout)
