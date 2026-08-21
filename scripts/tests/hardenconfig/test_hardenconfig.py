#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0
"""Tests for the hardenconfig target (scripts/kconfig/hardenconfig.py)."""

import os
import sys
import textwrap

import pytest

ZEPHYR_BASE = os.getenv("ZEPHYR_BASE")
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts", "kconfig"))

import hardenconfig  # noqa: E402
import kconfiglib  # noqa: E402


@pytest.mark.parametrize(
    ("value", "expected"),
    [
        (None, False),
        ("", False),
        ("0", False),
        ("n", False),
        # CMake spells a false cache variable this way
        ("OFF", False),
        ("FALSE", False),
        ("NO", False),
        ("NOTFOUND", False),
        ("1", True),
        ("y", True),
        ("ON", True),
        ("TRUE", True),
    ],
)
def test_env_flag(monkeypatch, value, expected):
    if value is None:
        monkeypatch.delenv("HARDENCONFIG_TEST_FLAG", raising=False)
    else:
        monkeypatch.setenv("HARDENCONFIG_TEST_FLAG", value)
    assert hardenconfig.env_flag("HARDENCONFIG_TEST_FLAG") is expected


MARKER_KCONFIG = """
    config EXPERIMENTAL
        bool

    config DEPRECATED
        bool

    config NOT_SECURE
        bool

    config GATE
        bool "gate"

    config ALWAYS_EXPERIMENTAL
        bool "always experimental"
        default y
        select EXPERIMENTAL

    config MAYBE_EXPERIMENTAL
        bool "maybe experimental"
        default y
        select EXPERIMENTAL if GATE

    config PLAIN
        bool "plain"
        default y
"""


@pytest.fixture
def marker_kconf(tmp_path):
    kconfig = tmp_path / "Kconfig"
    kconfig.write_text(textwrap.dedent(MARKER_KCONFIG))
    return kconfiglib.Kconfig(str(kconfig))


def test_marker_selects_honor_condition(marker_kconf):
    def flagged():
        report = hardenconfig.build_report(marker_kconf, {}, "strict")
        return {opt.name for opt in report.options if opt.origin == "marker"}

    assert "ALWAYS_EXPERIMENTAL" in flagged()
    assert "PLAIN" not in flagged()
    # the select is inactive while GATE is disabled
    assert "MAYBE_EXPERIMENTAL" not in flagged()

    marker_kconf.syms["GATE"].set_value(2)
    assert "MAYBE_EXPERIMENTAL" in flagged()
