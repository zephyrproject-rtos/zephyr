#!/usr/bin/env python3

# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Tests for the enabled-node (no driver available) diagnosis."""

import dtdoctor_analyzer
from conftest import DTS_ENABLED, DTS_ENABLED_MULTI, DTS_ENABLED_NO_COMPAT, ord_symbol


def diagnose(edt, label="foo_dev"):
    return dtdoctor_analyzer.handle_enabled_node(edt.label2node[label])


def test_headline_names_node(make_edt, kconfig_env):
    kconfig_env("kconfig_basic")
    edt, _ = make_edt(DTS_ENABLED)
    assert diagnose(edt)[0] == (
        "'foo_dev: /foo-device' is enabled but no driver appears to be available for it.\n"
    )


def test_suggests_gating_kconfig_options(make_edt, kconfig_env):
    kconfig_env("kconfig_basic")
    edt, _ = make_edt(DTS_ENABLED)
    lines = diagnose(edt)
    assert "Try enabling these Kconfig options:\n" in lines
    assert " - CONFIG_DTD_SERIAL=y" in lines
    # The depending driver symbol itself is not suggested, only its gating options
    assert not any("CONFIG_DTD_UART" in line for line in lines)


def test_multi_compat_union(make_edt, kconfig_env):
    kconfig_env("kconfig_basic")
    edt, _ = make_edt(DTS_ENABLED_MULTI)
    lines = diagnose(edt)
    assert " - CONFIG_DTD_SERIAL=y" in lines
    assert " - CONFIG_DTD_SPECIAL_CORE=y" in lines


def test_no_compat_fallback(make_edt):
    edt, _ = make_edt(DTS_ENABLED_NO_COMPAT)
    lines = diagnose(edt, label="bare_dev")
    assert "Could not determine compatible; check driver Kconfig manually." in lines


def test_missing_zephyr_base_handled(make_edt, monkeypatch):
    monkeypatch.delenv("ZEPHYR_BASE", raising=False)
    edt, _ = make_edt(DTS_ENABLED)
    lines = diagnose(edt)
    assert any("check driver Kconfig manually" in line for line in lines)


def test_main_end_to_end_enabled(make_edt, make_pickle, run_analyzer, kconfig_env):
    kconfig_env("kconfig_basic")
    edt, _ = make_edt(DTS_ENABLED)
    rc, out, _ = run_analyzer(make_pickle(edt), ord_symbol(edt, "foo_dev"))
    assert rc == 0
    assert "DT Doctor" in out
    assert "is enabled but no driver" in out
    assert " - CONFIG_DTD_SERIAL=y" in out
