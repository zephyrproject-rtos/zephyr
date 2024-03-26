# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""Unit tests for the dtsh.config module."""

# Relax pylint a bit for unit tests.
# pylint: disable=missing-function-docstring
# pylint: disable=import-outside-toplevel


import os

import pytest

from dtsh.config import DTShConfig, ActionableType

from .dtsh_uthelpers import DTShTests


def test_dtshconfig_load_ini_file() -> None:
    # 1. Initialize configuration with bundled defaults.
    cfg = DTShConfig(DTShTests.get_resource_path("ini", "test.ini"))
    assert cfg.getstr("test.string") == "a string"
    assert cfg.getstr("not.an.option") == ""
    # 2. Load user's specific configuration (overrides defaults).
    cfg.load_ini_file(DTShTests.get_resource_path("ini", "override.ini"))
    assert cfg.getstr("test.string") == "overridden"
    assert cfg.getstr("test.new") == "new"

    with pytest.raises(DTShConfig.Error):
        cfg.load_ini_file("not/a/config/file.ini")


def test_dtshconfig_getbool() -> None:
    cfg = DTShConfig(DTShTests.get_resource_path("ini", "test.ini"))

    assert cfg.getbool("test.true", False)
    assert not cfg.getbool("test.false", True)
    # getbool() is fail-safe.
    assert not cfg.getbool("test.bool.inval")
    assert cfg.getbool("undefined", True)


def test_dtshconfig_getint() -> None:
    cfg = DTShConfig(DTShTests.get_resource_path("ini", "test.ini"))

    assert cfg.getint("test.int") == 255
    assert cfg.getint("test.hex") == 0xFF
    # getint() is fail-safe.
    assert cfg.getint("test.int.inval") == 0
    assert cfg.getint("test.int.inval", -1) == -1


def test_dtshconfig_getstr() -> None:
    cfg = DTShConfig(DTShTests.get_resource_path("ini", "test.ini"))

    assert cfg.getstr("test.string") == "a string"
    assert cfg.getstr("test.string.quoted") == "quoted string "
    assert cfg.getstr("test.string.quotes") == 'a"b'
    assert cfg.getstr("test.string.unicode") == "❯"
    assert cfg.getstr("test.string.literal") == "\u276F"
    assert cfg.getstr("test.string.mixed") == "\u276F ❯"
    # getstr() is fail-safe.
    assert cfg.getstr("undefined") == ""
    assert cfg.getstr("undefined", "any") == "any"
    # Empty values map to empty strings.
    assert cfg.getstr("test.novalue") == ""


def test_dtshconfig_interpolation() -> None:
    cfg = DTShConfig(DTShTests.get_resource_path("ini", "test.ini"))
    assert cfg.getstr("test.interpolation") == "hello world"


def test_dtshconfig_defaults() -> None:
    dtsh_ini = os.path.abspath(
        os.path.join(os.path.dirname(__file__), "..", "src", "dtsh", "dtsh.ini")
    )
    assert os.path.isfile(dtsh_ini)
    cfg_defaults = DTShConfig(dtsh_ini)

    # Wide characters.
    assert "…" == cfg_defaults.wchar_ellipsis
    assert "↗" == cfg_defaults.wchar_arrow_ne
    assert "↖" == cfg_defaults.wchar_arrow_nw
    assert "→" == cfg_defaults.wchar_arrow_right
    assert "↳" == cfg_defaults.wchar_arrow_right_hook
    assert "—" == cfg_defaults.wchar_dash

    # Prompt.
    assert cfg_defaults.prompt_default
    assert cfg_defaults.prompt_alt
    assert cfg_defaults.prompt_sparse

    # General preferences.
    assert 255 == cfg_defaults.pref_redir2_maxwidth
    assert not cfg_defaults.pref_always_longfmt
    assert cfg_defaults.pref_sizes_si
    assert not cfg_defaults.pref_hex_upper
    assert cfg_defaults.pref_fs_hide_dotted
    assert cfg_defaults.pref_fs_no_spaces
    assert cfg_defaults.pref_fs_no_overwrite
    assert ActionableType.LINK == cfg_defaults.pref_actionable_type

    # List views.
    assert cfg_defaults.pref_list_headers
    assert not cfg_defaults.pref_list_placeholder
    assert cfg_defaults.pref_list_fmt
    assert ActionableType.LINK == ActionableType(
        cfg_defaults.pref_list_actionable_type
    )

    # Tree views.
    assert cfg_defaults.pref_tree_headers
    assert cfg_defaults.pref_tree_placeholder
    assert cfg_defaults.pref_tree_fmt
    assert ActionableType.NONE == ActionableType(
        cfg_defaults.pref_tree_actionable_type
    )
    assert ActionableType.LINK == cfg_defaults.pref_2Sided_actionable_type
    # 2-sided views child marker.
    assert cfg_defaults.pref_tree_cb_anchor

    # Actionable type.
    assert ActionableType.LINK == cfg_defaults.pref_actionable_type
    assert cfg_defaults.pref_actionable_text

    # HTML.
    assert "html" == cfg_defaults.pref_html_theme
    assert "Courier New" == cfg_defaults.pref_html_font_family

    # SVG.
    assert "svg" == cfg_defaults.pref_svg_theme
    assert "Courier New" == cfg_defaults.pref_svg_font_family
    assert 0.6 == cfg_defaults.pref_svg_font_ratio
