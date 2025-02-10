# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""Unit tests for the dtsh.builtins.alias module."""

# Relax pylint a bit for unit tests.
# pylint: disable=missing-function-docstring


from dtsh.io import DTShOutput
from dtsh.shell import DTSh
from dtsh.builtins.alias import DTShBuiltinAlias

from .dtsh_uthelpers import DTShTests

_stdout = DTShOutput()


def test_dtsh_builtin_alias() -> None:
    cmd = DTShBuiltinAlias()
    DTShTests.check_cmd_meta(cmd, "alias")


def test_dtsh_builtin_alias_flags() -> None:
    cmd = DTShBuiltinAlias()
    sh = DTSh(DTShTests.get_sample_dtmodel(), [cmd])
    DTShTests.check_cmd_flags(cmd, sh, _stdout)


def test_dtsh_builtin_alias_arg_longfmt() -> None:
    cmd = DTShBuiltinAlias()
    sh = DTSh(DTShTests.get_sample_dtmodel(), [cmd])
    DTShTests.check_cmd_arg_longfmt(cmd, sh, _stdout)


def test_dtsh_builtin_alias_param() -> None:
    cmd = DTShBuiltinAlias()
    sh = DTSh(DTShTests.get_sample_dtmodel(), [cmd])
    DTShTests.check_cmd_param_alias(cmd, sh, _stdout)


def test_dtsh_builtin_alias_execute() -> None:
    dtmodel = DTShTests.get_sample_dtmodel()
    cmd = DTShBuiltinAlias()
    sh = DTSh(dtmodel, [cmd])

    DTShTests.check_cmd_execute(cmd, sh, _stdout)
