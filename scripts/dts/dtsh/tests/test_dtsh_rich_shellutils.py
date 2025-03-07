# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""Unit tests for the dtsh.rich.shellutils module."""

# Relax pylint a bit for unit tests.
# pylint: disable=missing-function-docstring


import pytest

from dtsh.shell import DTSh, DTShError
from dtsh.shellutils import (
    DTShFlagReverse,
    DTShFlagEnabledOnly,
    DTShArgOrderBy,
    DTSH_NODE_ORDER_BY,
)
from dtsh.rich.shellutils import (
    DTShNodeFmt,
    DTShFlagLongList,
    DTShArgLongFmt,
    DTShCommandLongFmt,
    DTSH_NODE_FMT_SPEC,
)
from dtsh.rich.modelview import SketchMV

from .dtsh_uthelpers import DTShTests


NODE_FMT_ALL = "".join(spec for spec in DTSH_NODE_FMT_SPEC)
NODE_COL_ALL = [fmt.col for fmt in DTSH_NODE_FMT_SPEC.values()]


def test_dtsh_node_fmt_spc() -> None:
    for key, spec in DTSH_NODE_FMT_SPEC.items():
        assert key == spec.key
        assert spec.brief
        assert spec.col
        assert spec.col.header


def test_dtsh_node_fmt() -> None:
    # Parse all valid format specifiers.
    assert NODE_COL_ALL == DTShNodeFmt.parse(NODE_FMT_ALL)

    # Client code expect DTShError on invalid format string.
    with pytest.raises(DTShError):
        DTShNodeFmt.parse("invalid format string")


def test_dtshflag_longlist() -> None:
    DTShTests.check_flag(DTShFlagLongList())


def test_dtsharg_longfmt() -> None:
    arg = DTShArgLongFmt()
    assert not arg.fmt

    DTShTests.check_arg(arg, NODE_FMT_ALL)
    assert NODE_COL_ALL == arg.fmt


def test_dtsharg_longfmt_autocomp() -> None:
    sh = DTSh(DTShTests.get_sample_dtmodel(), [])
    arg = DTShArgLongFmt()

    # Auto-complete with all available fields.
    autocomp_states = sorted(NODE_FMT_ALL, key=lambda x: x.lower())
    assert autocomp_states == [state.rlstr for state in arg.autocomp("", sh)]

    # Auto-complete with all fields expect those already set.
    autocomp_states.remove("n")
    autocomp_states.remove("a")
    assert autocomp_states == [state.rlstr for state in arg.autocomp("na", sh)]

    # No match
    assert [] == arg.autocomp("invalid_fmt", sh)


def test_dtsh_command_lonfmt() -> None:
    cmd = DTShCommandLongFmt("foo", "", [], None)
    # DTShFlagLongList
    assert cmd.option("-l")
    # DTShArgLongFmt
    assert cmd.option("--format")

    assert not cmd.with_flag(DTShFlagLongList)
    assert not cmd.with_arg(DTShArgLongFmt).isset
    assert not cmd.with_arg(DTShArgLongFmt).fmt

    cmd.parse_argv(["-l"])
    assert cmd.with_flag(DTShFlagLongList)
    # Default fields are not set on parsing the command line.
    assert not cmd.with_arg(DTShArgLongFmt).fmt
    # get_longformat() is intended to be used once the parser has finished,
    # and will answer "something" (fallback).
    assert 0 < len(cmd.get_longfmt(""))

    # Caller my also provide a default value when calling get_fields(),
    # typically using long lists without explicit format.
    assert 3 == len(cmd.get_longfmt("nac"))

    # Set all fields in format string.
    cmd.parse_argv(["--format", NODE_FMT_ALL])
    assert NODE_COL_ALL == cmd.with_arg(DTShArgLongFmt).fmt
    # A format is explicitly provided: default is ignored.
    assert NODE_COL_ALL == cmd.get_longfmt("")
    assert NODE_COL_ALL == cmd.get_longfmt("pd")

    with pytest.raises(DTShError):
        cmd.parse_argv(["--format", "invalid format string"])


def test_dtsh_command_lonfmt_sort() -> None:
    flag_reverse = DTShFlagReverse()
    arg_orderby = DTShArgOrderBy()
    cmd = DTShCommandLongFmt("mock", "", [flag_reverse, arg_orderby], None)

    assert not cmd.flag_reverse
    assert not cmd.arg_sorter

    cmd.parse_argv(["--order-by", "p"])
    assert not cmd.flag_reverse
    assert cmd.arg_sorter is DTSH_NODE_ORDER_BY["p"].sorter

    dt = DTShTests.get_sample_dtmodel()
    nodes = dt["/leds"].children
    expect_nodes = list(sorted(nodes))
    assert expect_nodes == cmd.sort(nodes)

    cmd.parse_argv(["-r", "--order-by", "p"])
    assert cmd.flag_reverse
    assert cmd.arg_sorter is DTSH_NODE_ORDER_BY["p"].sorter
    expect_nodes = list(reversed(expect_nodes))
    assert expect_nodes == cmd.sort(nodes)


def test_dtsh_command_lonfmt_prune() -> None:
    dt = DTShTests.get_sample_dtmodel()
    flag_enabled_only = DTShFlagEnabledOnly()
    cmd = DTShCommandLongFmt("foo", "", [flag_enabled_only], None)

    # Enabled.
    dt_i2c0 = dt.labeled_nodes["i2c0"]
    # Disabled.
    dt_i2c1 = dt.labeled_nodes["i2c1"]
    nodes = [dt_i2c0, dt_i2c1]

    assert not cmd.flag_enabled_only
    assert nodes == cmd.prune(nodes)

    cmd.parse_argv(["--enabled-only"])
    assert cmd.flag_enabled_only
    assert [dt_i2c0] == cmd.prune(nodes)


def test_dtsh_node_fmt_columns() -> None:
    # Crash-test: we should be able to render all nodes with all columns
    # for all layouts.
    sh = DTSh(DTShTests.get_sample_dtmodel(), [])
    for node in sh.dt:
        for col in NODE_COL_ALL:
            for layout in SketchMV.Layout:
                col.mk_view(node, SketchMV(layout))
