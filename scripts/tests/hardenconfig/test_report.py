#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0
"""Tests for the hardenconfig report model and text rendering helpers."""

import os
import sys

ZEPHYR_BASE = os.getenv("ZEPHYR_BASE")
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts", "kconfig"))

import hardenconfig  # noqa: E402


def make_option(**kwargs):
    defaults = {
        "name": "FOO",
        "recommended": "n",
        "rationale": "reason",
        "result": "FAIL",
        "current": "y",
        "visible": True,
    }
    defaults.update(kwargs)
    return hardenconfig.Option(**defaults)


def test_report_failures_and_dict():
    report = hardenconfig.HardeningReport(
        profile="strict",
        options=[
            make_option(name="FAIL_VISIBLE"),
            make_option(name="HIDDEN", result="NA", visible=False),
            make_option(name="OK", result="PASS"),
            make_option(name="MISSING", result="NA", current=None),
        ],
    )

    assert [opt.name for opt in report.failures] == ["FAIL_VISIBLE"]

    as_dict = report.to_dict()
    assert as_dict["profile"] == "strict"
    assert as_dict["fail_count"] == 1
    assert len(as_dict["options"]) == 4
    assert as_dict["options"][0]["name"] == "FAIL_VISIBLE"
    assert as_dict["options"][0]["origin"] == "database"


def test_flexible_column_widths():
    # Wide terminal: both columns grow beyond their minimums.
    rationale, references = hardenconfig.flexible_column_widths(200)
    assert rationale > 24 and references > 14
    assert rationale + references == 200 - 80

    # Narrow terminal: minimum widths win.
    assert hardenconfig.flexible_column_widths(80) == (24, 14)


def test_render_text_filters_and_wraps(monkeypatch):
    monkeypatch.setattr(
        hardenconfig.shutil, "get_terminal_size", lambda fallback=None: os.terminal_size((120, 24))
    )
    report = hardenconfig.HardeningReport(
        profile="strict",
        options=[
            make_option(
                name="STACK_SENTINEL",
                rationale="word " * 40,
                references=[{"id": "CWE-121", "name": "Stack-based Buffer Overflow"}],
            ),
            make_option(name="BOOT_BANNER", result="PASS"),
        ],
    )

    text = hardenconfig.render_text(report, show_all=False)
    assert "CONFIG_STACK_SENTINEL" in text
    assert "CONFIG_BOOT_BANNER" not in text
    # resolved CWE name is rendered (possibly wrapped across lines)
    assert "CWE-121:" in text
    assert "Overflow" in text
    assert "1 option(s) deviate" in text
    # every rendered line respects the (mocked) terminal width
    assert all(len(line) <= 120 for line in text.splitlines())

    text_all = hardenconfig.render_text(report, show_all=True)
    assert "CONFIG_BOOT_BANNER" in text_all


def test_format_references():
    refs = [
        {"id": "CWE-121", "name": "Stack-based Buffer Overflow"},
        {"id": "CWE-999999", "name": None},
    ]
    assert hardenconfig.format_references(refs) == (
        "CWE-121: Stack-based Buffer Overflow\nCWE-999999"
    )
