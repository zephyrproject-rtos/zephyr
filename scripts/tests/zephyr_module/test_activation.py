# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0

"""Tests for module activation in scripts/zephyr_module.py."""

from pathlib import Path

import zephyr_module

META = {"name": "hal_tdk", "name-sanitized": "hal_tdk"}


def opts(active_default_y):
    return "\n".join(
        zephyr_module.kconfig_module_opts("hal_tdk", "hal_tdk", [], False, active_default_y)
    )


def block(snippet, symbol):
    """The properties of one config entry."""
    body = snippet.split(f"config {symbol}\n", 1)[1]
    return body.split("\nconfig ", 1)[0]


def test_availability_is_a_fact_in_either_mode():
    """A checked out module is available whatever the activation mode is."""
    for active_default_y in (True, False):
        assert "default y" in block(opts(active_default_y), "ZEPHYR_HAL_TDK_MODULE")


def test_activation_requires_availability():
    """Kconfig only reports a module missing if being active depends on it."""
    for active_default_y in (True, False):
        assert "depends on ZEPHYR_HAL_TDK_MODULE" in block(
            opts(active_default_y), "ZEPHYR_HAL_TDK_MODULE_ACTIVE"
        )


def test_all_activation_makes_every_module_active():
    assert "default y" in block(opts(True), "ZEPHYR_HAL_TDK_MODULE_ACTIVE")


def test_strict_activation_leaves_activation_to_kconfig():
    """Without a default the module is active only where something selects it."""
    assert "default y" not in block(opts(False), "ZEPHYR_HAL_TDK_MODULE_ACTIVE")


def test_strict_activation_still_sources_the_module_kconfig(tmp_path):
    """A module has to be visible to Kconfig for anything to select it."""
    glue = tmp_path / "Kconfig"
    glue.write_text("config TDK_ROOT\n\tbool\n")

    snippet = zephyr_module.kconfig_snippet(META, Path(tmp_path), glue, active_default_y=False)

    assert glue.resolve().as_posix() in snippet
    assert "default y" not in block(snippet, "ZEPHYR_HAL_TDK_MODULE_ACTIVE")
