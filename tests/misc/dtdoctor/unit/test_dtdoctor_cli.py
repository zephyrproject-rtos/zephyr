#!/usr/bin/env python3

# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Tests for the analyzer CLI symbol/ordinal resolution and exit codes."""

from conftest import DTS_DISABLED_FULL, ord_symbol


def test_non_device_symbol_silent_rc1(run_analyzer):
    # The wrapper only ever passes __device_dts_ord_* symbols; anything else is
    # rejected silently, before the pickle is even opened
    rc, out, err = run_analyzer("does-not-matter.pickle", "some_other_symbol")
    assert rc == 1
    assert out == ""
    assert err == ""


def test_symbol_embedded_in_text_accepted(make_edt, make_pickle, run_analyzer):
    edt, _ = make_edt(DTS_DISABLED_FULL)
    symbol = ord_symbol(edt, "foo_dev")
    rc, out, _ = run_analyzer(make_pickle(edt), f"`{symbol}' referenced in section .text")
    assert rc == 0
    assert "DT Doctor" in out


def test_unknown_ordinal_reports_error(make_edt, make_pickle, run_analyzer):
    edt, _ = make_edt(DTS_DISABLED_FULL)
    missing = max(n.dep_ordinal for n in edt.nodes) + 1000
    rc, out, err = run_analyzer(make_pickle(edt), f"__device_dts_ord_{missing}")
    assert rc == 1
    assert out == ""
    assert f"Ordinal {missing} not found" in err
