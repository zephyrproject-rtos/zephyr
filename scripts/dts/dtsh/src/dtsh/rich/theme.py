# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""Devicetree shell theme (aka rich styles).

A theme is a collection of styles, each of which is consistently used
to represent a type of information: e.g. by default compatible strings
are always green, and things that behave like symbolic links (e.g. aliases)
are all in italic.

Theme files are simple INI files named "theme.ini":

- the bundled theme file which sets the default appearance
- an optional user's theme file which customizes the defaults

The user's theme file is located in the DTSh application data directory.

Unit tests and examples: tests/test_dtsh_theme.py
"""


from typing import Optional, Dict, Mapping

import configparser
import os
import sys

from rich.errors import StyleError, StyleSyntaxError
from rich.style import Style
from rich.theme import Theme

from dtsh.config import DTShConfig

_dtshconf: DTShConfig = DTShConfig.getinstance()


class DTShTheme:
    """Rich styles."""

    STYLE_DEFAULT = "dtsh.default"
    STYLE_ERROR = "dtsh.error"
    STYLE_WARNING = "dtsh.warning"
    STYLE_DISABLED = "dtsh.disabled"
    STYLE_APOLOGIES = "dtsh.apologies"

    STYLE_FS_FILE = "dtsh.fs.file"
    STYLE_FS_DIR = "dtsh.fs.dir"

    STYLE_LINK_LOCAL = "dtsh.link.local"
    STYLE_LINK_WWW = "dtsh.link.www"

    STYLE_LIST_HEADER = "dtsh.list.header"
    STYLE_TREE_HEADER = "dtsh.tree.header"

    STYLE_DT_PATH_BRANCH = "dtsh.dt.path_branch"
    STYLE_DT_PATH_NODE = "dtsh.dt.path_node"

    STYLE_DT_DESCRIPTION = "dtsh.dt.description"

    STYLE_DT_NODE_NAME = "dtsh.dt.node_name"
    STYLE_DT_UNIT_NAME = "dtsh.dt.unit_name"
    STYLE_DT_UNIT_ADDR = "dtsh.dt.unit_addr"
    STYLE_DT_DEVICE_LABEL = "dtsh.dt.device_label"
    STYLE_DT_NODE_LABEL = "dtsh.dt.node_label"
    STYLE_DT_COMPAT_STR = "dtsh.dt.compat_str"
    STYLE_DT_BINDING_COMPAT = "dtsh.dt.binding_compat"
    STYLE_DT_BINDING_DESC = "dtsh.dt.binding_desc"
    STYLE_DT_CB_ORDER = "dtsh.dt.cb_depth"
    STYLE_DT_CB_ANCHOR = "dtsh.dt.cb_anchor"
    STYLE_DT_IS_CHILD_BINDING = "dtsh.dt.is_child_binding"
    STYLE_DT_ALIAS = "dtsh.dt.alias"
    STYLE_DT_CHOSEN = "dtsh.dt.chosen"
    STYLE_DT_BUS = "dtsh.dt.bus"
    STYLE_DT_ON_BUS = "dtsh.dt.on_bus"
    STYLE_DT_IRQ_NUMBER = "dtsh.dt.irq_number"
    STYLE_DT_IRQ_PRIORITY = "dtsh.dt.irq_priority"
    STYLE_DT_STATUS_ENABLED = "dtsh.dt.status_enabled"
    STYLE_DT_STATUS_DISABLED = "dtsh.dt.status_disabled"
    STYLE_DT_REG_ADDR = "dtsh.dt.reg_addr"
    STYLE_DT_REG_SIZE = "dtsh.dt.reg_size"
    STYLE_DT_ORDINAL = "dtsh.dt.dep_ordinal"
    STYLE_DT_DEP_ON = "dtsh.dt.depend_on"
    STYLE_DT_DEP_FAILED = "dtsh.dt.depend_failed"
    STYLE_DT_REQ_BY = "dtsh.dt.required_by"
    STYLE_DT_VENDOR_NAME = "dtsh.dt.vendor_name"
    STYLE_DT_VENDOR_PREFIX = "dtsh.dt.vendor_prefix"

    STYLE_DT_PROPERTY = "dtsh.dt.property"
    STYLE_DTVALUE_BOOL = "dtsh.dtvalue.bool"
    STYLE_DTVALUE_TRUE = "dtsh.dtvalue.true"
    STYLE_DTVALUE_FALSE = "dtsh.dtvalue.false"
    STYLE_DTVALUE_INT = "dtsh.dtvalue.int"
    STYLE_DTVALUE_INT_ARRAY = "dtsh.dtvalue.int_array"
    STYLE_DTVALUE_STR = "dtsh.dtvalue.string"
    STYLE_DTVALUE_UINT8 = "dtsh.dtvalue.uint8"
    STYLE_DTVALUE_PHANDLE = "dtsh.dtvalue.phandle"
    STYLE_DTVALUE_PHANDLE_DATA = "dtsh.dtvalue.phandle_data"
    STYLE_DTVALUE_COMPOUND = "dtsh.dtvalue.compound"
    STYLE_YAML_BINDING = "dtsh.yaml.binding"
    STYLE_YAML_INCLUDE = "dtsh.yaml.include"
    STYLE_DTS_FILE = "dtsh.dts"

    STYLE_FORM_LABEL = "dtsh.form.label"
    STYLE_FORM_DEFAULT = "dtsh.form.default"

    STYLE_INF_ZEPHYR_BASE = "dtsh.inf.zephyr_base"
    STYLE_INF_ZEPHYR_KERNEL = "dtsh.inf.kernel"
    STYLE_INF_DEVICETREE = "dtsh.inf.devicetree"
    STYLE_INF_TOOLCHAIN = "dtsh.inf.toolchain"
    STYLE_INF_BINDINGS = "dtsh.inf.bindings"
    STYLE_INF_VENDORS = "dtsh.inf.vendors"
    STYLE_INF_APPLICATION = "dtsh.inf.application"
    STYLE_INF_BOARD = "dtsh.inf.board"
    STYLE_INF_BOARD_FILE = "dtsh.inf.board_file"
    STYLE_INF_SOC = "dtsh.inf.soc"
    STYLE_INF_SOC_SVD = "dtsh.inf.soc_svd"

    class Error(BaseException):
        """Error loading styles file."""

    @classmethod
    def getinstance(cls) -> "DTShTheme":
        """Access the rich styles configuration instance."""
        return _dtshtheme

    # Rich styles.
    _styles: Dict[str, Style]

    def __init__(self, path: Optional[str] = None) -> None:
        """Initialize DTSh theme.

        If a theme path is explicitly set,
        only this theme file is loaded.

        Otherwise, proceed to default theme initialization:

        - 1st, load bundled theme file
        - then, load user's theme file to override defaults

        Args:
            path: Path to theme file,
              or None for default theme initialization.
        """
        self._styles = {}

        if path:
            # If explicitly specified, load only this one: fault if invalid.
            self.load_theme_file(path, fail_early=True)
        else:
            # Load defaults from bundled styles file
            # (fault if invalid, should not happen).
            path = os.path.join(os.path.dirname(__file__), "theme.ini")
            self.load_theme_file(path, fail_early=True)

            # Load user's theme file if any,
            # don't fault if unreadable or invalid.
            path = _dtshconf.get_user_file("theme.ini")
            if path and os.path.isfile(path):
                self.load_theme_file(path, fail_early=False)

    @property
    def styles(self) -> Mapping[str, Style]:
        """The theme's rich styles."""
        return self._styles

    def load_theme_file(self, path: str, fail_early: bool = True) -> None:
        """Load a rich styles file.

        Args:
            path: Path to a styles file.
            fail_early: If set, fault when we can't open the file for reading,
              or its content is invalid. This is the default.

        Raises:
            DTShTheme.Error: Failed to load styles file.
        """
        try:
            self._styles.update(Theme.read(path, encoding="utf-8").styles)
        except (
            OSError,
            StyleError,
            StyleSyntaxError,
            configparser.Error,
        ) as e:
            if isinstance(e, OSError):
                msg = e.strerror
            elif isinstance(e, configparser.Error):
                msg = e.message
            else:
                msg = str(e)
            if fail_early:
                raise DTShTheme.Error(msg) from e
            print(f"Failed to load theme file: {msg}", file=sys.stderr)


_dtshtheme = DTShTheme()
