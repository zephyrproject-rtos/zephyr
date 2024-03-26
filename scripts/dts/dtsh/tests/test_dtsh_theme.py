# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""Unit tests for the dtsh.rich.theme module."""

# Relax pylint a bit for unit tests.
# pylint: disable=missing-function-docstring


import os

import pytest

from rich.color import Color

from dtsh.rich.theme import DTShTheme

from .dtsh_uthelpers import DTShTests


def test_dtshtheme_load_theme_file() -> None:
    # 1. Initialize configuration with bundled defaults.
    theme = DTShTheme(DTShTests.get_resource_path("theme", "test.ini"))

    assert not theme.styles["test.default"].bold
    # See Color.ANSI_COLOR_NAMES for valid color names.
    assert theme.styles["test.red"].color == Color.parse("red")

    assert theme.styles["test.interpolation"].color == Color.parse("red")

    # 2. Load user's specific configuration (overrides defaults).
    theme.load_theme_file(DTShTests.get_resource_path("theme", "override.ini"))
    assert theme.styles["test.default"].color == Color.parse("green")

    with pytest.raises(DTShTheme.Error):
        theme.load_theme_file("not/a/styles/file.ini")


def test_dtshtheme_defaults() -> None:
    # All these constants MUST have suitable values in the bundled theme.ini.
    theme_ini = os.path.abspath(
        os.path.join(
            os.path.dirname(__file__), "..", "src", "dtsh", "rich", "theme.ini"
        )
    )
    assert os.path.isfile(theme_ini)
    theme_defaults = DTShTheme(theme_ini)

    assert theme_defaults.styles[DTShTheme.STYLE_DEFAULT]
    assert theme_defaults.styles[DTShTheme.STYLE_WARNING]
    assert theme_defaults.styles[DTShTheme.STYLE_ERROR]
    assert theme_defaults.styles[DTShTheme.STYLE_DISABLED]

    assert theme_defaults.styles[DTShTheme.STYLE_FS_FILE]
    assert theme_defaults.styles[DTShTheme.STYLE_FS_DIR]

    assert theme_defaults.styles[DTShTheme.STYLE_LINK_LOCAL]
    assert theme_defaults.styles[DTShTheme.STYLE_LINK_WWW]

    assert theme_defaults.styles[DTShTheme.STYLE_LIST_HEADER]
    assert theme_defaults.styles[DTShTheme.STYLE_TREE_HEADER]

    assert theme_defaults.styles[DTShTheme.STYLE_DT_PATH_BRANCH]
    assert theme_defaults.styles[DTShTheme.STYLE_DT_PATH_NODE]

    assert theme_defaults.styles[DTShTheme.STYLE_DT_NODE_NAME]
    assert theme_defaults.styles[DTShTheme.STYLE_DT_UNIT_NAME]
    assert theme_defaults.styles[DTShTheme.STYLE_DT_UNIT_ADDR]
    assert theme_defaults.styles[DTShTheme.STYLE_DT_DEVICE_LABEL]
    assert theme_defaults.styles[DTShTheme.STYLE_DT_NODE_LABEL]
    assert theme_defaults.styles[DTShTheme.STYLE_DT_COMPAT_STR]
    assert theme_defaults.styles[DTShTheme.STYLE_DT_BINDING_COMPAT]
    assert theme_defaults.styles[DTShTheme.STYLE_DT_BINDING_DESC]
    assert theme_defaults.styles[DTShTheme.STYLE_DT_CB_ORDER]
    assert theme_defaults.styles[DTShTheme.STYLE_DT_IS_CHILD_BINDING]
    assert theme_defaults.styles[DTShTheme.STYLE_DT_ALIAS]
    assert theme_defaults.styles[DTShTheme.STYLE_DT_CHOSEN]
    assert theme_defaults.styles[DTShTheme.STYLE_DT_BUS]
    assert theme_defaults.styles[DTShTheme.STYLE_DT_ON_BUS]
    assert theme_defaults.styles[DTShTheme.STYLE_DT_IRQ_NUMBER]
    assert theme_defaults.styles[DTShTheme.STYLE_DT_IRQ_PRIORITY]
    assert theme_defaults.styles[DTShTheme.STYLE_DT_REG_ADDR]
    assert theme_defaults.styles[DTShTheme.STYLE_DT_REG_SIZE]
    assert theme_defaults.styles[DTShTheme.STYLE_DT_ORDINAL]
    assert theme_defaults.styles[DTShTheme.STYLE_DT_DEP_ON]
    assert theme_defaults.styles[DTShTheme.STYLE_DT_DEP_FAILED]
    assert theme_defaults.styles[DTShTheme.STYLE_DT_REQ_BY]
    assert theme_defaults.styles[DTShTheme.STYLE_DT_VENDOR_NAME]
    assert theme_defaults.styles[DTShTheme.STYLE_DT_VENDOR_PREFIX]
