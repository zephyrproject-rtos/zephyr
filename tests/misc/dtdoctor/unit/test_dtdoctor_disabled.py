#!/usr/bin/env python3

# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Tests for the disabled-node diagnosis."""

import dtdoctor_analyzer
from conftest import DTS_DISABLED_FULL, DTS_DISABLED_UNREFERENCED, dts_line_of, ord_symbol


def diagnose(edt):
    return dtdoctor_analyzer.handle_disabled_node(edt.label2node["foo_dev"])


def test_headline_reports_file_and_line(make_edt):
    edt, dts_path = make_edt(DTS_DISABLED_FULL)
    lineno = dts_line_of(dts_path, 'status = "disabled"')
    assert diagnose(edt)[0] == f"'foo_dev: /foo-device' is disabled in {dts_path}:{lineno}"


def test_lists_dependent_nodes(make_edt):
    edt, _ = make_edt(DTS_DISABLED_FULL)
    lines = diagnose(edt)
    assert "The following nodes depend on it:" in lines
    assert " - /consumer-a" in lines


def test_no_dependents_when_unreferenced(make_edt):
    edt, _ = make_edt(DTS_DISABLED_UNREFERENCED)
    assert not any("depend on it" in line for line in diagnose(edt))


def test_chosen_reference_reported(make_edt):
    edt, _ = make_edt(DTS_DISABLED_FULL)
    assert any("chosen" in line and "'zephyr,console'" in line for line in diagnose(edt))


def test_alias_reference_reported(make_edt):
    edt, _ = make_edt(DTS_DISABLED_FULL)
    assert any("aliases" in line and "'my-foo'" in line for line in diagnose(edt))


def test_remediation_hint(make_edt):
    edt, _ = make_edt(DTS_DISABLED_FULL)
    assert any("setting its 'status' property to 'okay'" in line for line in diagnose(edt))


def test_main_end_to_end_disabled(make_edt, make_pickle, run_analyzer):
    edt, dts_path = make_edt(DTS_DISABLED_FULL)
    rc, out, _ = run_analyzer(make_pickle(edt), ord_symbol(edt, "foo_dev"))
    assert rc == 0
    assert "DT Doctor" in out
    assert "is disabled in" in out
    assert str(dts_path) in out
