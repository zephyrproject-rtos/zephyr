#!/usr/bin/env python3

# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Unit tests for find_kconfig_deps() against the fixture Kconfig trees."""

import dtdoctor_analyzer
import pytest

DT_HAS_FOO = "DT_HAS_VND_FOO_DEVICE_ENABLED"


@pytest.fixture
def load_kconf(kconfig_env):
    def _load(scenario):
        kconfig_env(scenario)
        kconf = dtdoctor_analyzer.setup_kconfig()
        assert kconf is not None
        return kconf

    return _load


def test_depends_on_suggests_gating_symbols(load_kconf):
    deps = dtdoctor_analyzer.find_kconfig_deps(load_kconf("kconfig_basic"), DT_HAS_FOO)
    assert deps == {"CONFIG_DTD_SERIAL"}


def test_only_dt_dep_yields_no_suggestions(load_kconf):
    # A driver whose only dependency is the DT_HAS symbol yields nothing: the
    # depending symbol itself is never suggested, only its gating options
    deps = dtdoctor_analyzer.find_kconfig_deps(
        load_kconf("kconfig_basic"), "DT_HAS_VND_BARE_DEVICE_ENABLED"
    )
    assert deps == set()


def test_select_condition_detected(load_kconf):
    deps = dtdoctor_analyzer.find_kconfig_deps(load_kconf("kconfig_select"), DT_HAS_FOO)
    assert "CONFIG_DTD_PLATFORM_SELECT" in deps


def test_imply_condition_detected(load_kconf):
    deps = dtdoctor_analyzer.find_kconfig_deps(load_kconf("kconfig_select"), DT_HAS_FOO)
    assert "CONFIG_DTD_PLATFORM_IMPLY" in deps


def test_no_substring_false_positive(load_kconf):
    # DT_HAS_VND_FOO_DEVICE_ENABLED must not match DT_HAS_VND_FOO_DEVICE_ENABLED_EXT
    deps = dtdoctor_analyzer.find_kconfig_deps(load_kconf("kconfig_substring"), DT_HAS_FOO)
    assert deps == set()
