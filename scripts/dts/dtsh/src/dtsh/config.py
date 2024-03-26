# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""Devicetree shell configuration and data.

User-specific configuration and data are stored into a
platform-dependent directory.

DTSh is configured by simple INI files named "dtsh.ini":

- the bundled configuration file which sets the default configuration
- an optional user's configuration file which customizes the defaults

The command history file (if GNU readline support is enabled)
is loaded from and stored into the same directory.

Unit tests and examples: tests/test_dtsh_config.py
"""


from typing import Optional

import configparser
import codecs
import enum
import os
import re
import shutil
import sys


class ActionableType(enum.Enum):
    """Control the rendering of actionable text elements (aka links).

    When acted on (clicked) actionable UIs elements should open local
    files in the system default text editor (e.g. YAML binding files),
    or open external URLs in the system default browser (e.g. links to
    the Zephyr project documentation or Devicetree Specification).

    Valid values:

    - "none": won't create any link
    - "default": the text itself (e.g. "nordic,nrf-twi") is made actionable
    - "sparse": an additional actionable view (e.g. "[↗]") is appended
        to the original text

    For sparse links, the additional view content is configured
    by the DTSh option "wchar.actionable".
    """

    NONE = "none"
    """No actions."""

    LINK = "link"
    """Make linked text itself actionable."""

    ALT = "alt"
    """Append additional actionable view."""


class DTShConfig:
    """Devicetree shell application data and configuration."""

    class Error(BaseException):
        """Error loading configuration file."""

    @classmethod
    def getinstance(cls) -> "DTShConfig":
        """Access the preferences configuration instance."""
        return _dtshconf

    # RE for ASCII escape sequences that may appear in Python strings.
    # See:
    # - DTShConfig.getstr()
    # - "unicode_escape doesn't work in general" (https://stackoverflow.com/a/24519338)
    _RE_ESCAPE_SEQ: re.Pattern[str] = re.compile(
        r"""
    ( \\U........
    | \\u....
    | \\x..
    | \\[0-7]{1,3}
    | \\N\{[^}]+\}
    | \\[\\'"abfnrtv]
    )""",
        re.UNICODE | re.VERBOSE,
    )

    # Parsed configuration.
    _cfg: configparser.ConfigParser

    # Path to the per-user DTSh configuration and data directory
    _app_dir: str

    def __init__(self, path: Optional[str] = None) -> None:
        """Initialize DTSh configuration.

        If a configuration path is explicitly set,
        only this configuration file is loaded.

        Otherwise, proceed to default configuration initialization:

        - 1st, load bundled default configuration file
        - then, load user's configuration file to customize defaults

        Args:
            path: Path to configuration file,
              or None for default configuration initialization.
        """
        self._init_app_dir()

        self._cfg = configparser.ConfigParser(
            # Despite its name, extended interpolation seems more intuitive
            # to us than basic interpolation.
            interpolation=configparser.ExtendedInterpolation()
        )

        if path:
            # If explicitly specified, load only this one.
            self.load_ini_file(path)
        else:
            # Load defaults from bundled configuration file.
            path = os.path.join(os.path.dirname(__file__), "dtsh.ini")
            self.load_ini_file(path)
            # Load user's configuration file if any.
            path = self.get_user_file("dtsh.ini")
            if os.path.isfile(path):
                self.load_ini_file(path)

    @property
    def app_dir(self) -> str:
        r"""Path to the per-user DTSh configuration and data directory.

        Location is platform-dependent:

        - POSIX: "$XDG_CONFIG_HOME/dtsh", or "~/.config/dtsh" if XDG_CONFIG_HOME
          is not set (Freedesktop XDG Base Directory Specification)
        - Windows: "%LOCALAPPDATA%\DTSh", or "~\AppData\Local\DTSh"
          if LOCALAPPDATA is unset (unlikely)
        - macOS: "~/Library/DTSh"

        Returns:
            The path to the user's directory for DTSh application data
            and configuration files. The directory is not granted to exist.
        """
        return self._app_dir

    @property
    def wchar_ellipsis(self) -> str:
        """Ellipsis."""
        return self.getstr("wchar.ellipsis")

    @property
    def wchar_arrow_ne(self) -> str:
        """North-East Arrow."""
        return self.getstr("wchar.arrow_ne")

    @property
    def wchar_arrow_nw(self) -> str:
        """North-West Arrow."""
        return self.getstr("wchar.arrow_nw")

    @property
    def wchar_arrow_right(self) -> str:
        """Rightwards Arrow."""
        return self.getstr("wchar.arrow_right")

    @property
    def wchar_arrow_right_hook(self) -> str:
        """Rightwards Arrow."""
        return self.getstr("wchar.arrow_right_hook")

    @property
    def wchar_dash(self) -> str:
        """Tiret."""
        return self.getstr("wchar.dash")

    @property
    def wchar_link(self) -> str:
        """Tiret."""
        return self.getstr("wchar.link")

    @property
    def prompt_default(self) -> str:
        """Default ANSI prompt for DTSh sessions."""
        return self.getstr("prompt.default")

    @property
    def prompt_alt(self) -> str:
        """Alternate ANSI prompt for DTSh sessions."""
        return self.getstr("prompt.alt")

    @property
    def prompt_sparse(self) -> bool:
        """Whether to append an empty line after DTSh command outputs."""
        return self.getbool("prompt.sparse")

    @property
    def pref_redir2_maxwidth(self) -> int:
        """Maximum width in number characters for command output redirection."""
        return self.getint("pref.redir2_maxwidth")

    @property
    def pref_always_longfmt(self) -> bool:
        """Whether to assume the flag "use long listing format" is always set."""
        return self.getbool("pref.always_longfmt")

    @property
    def pref_fs_hide_dotted(self) -> bool:
        """Whether to hide files and directories whose name starts with "."."""
        return self.getbool("pref.fs.hide_dotted")

    @property
    def pref_fs_no_spaces(self) -> bool:
        """Whether to forbid spaces in redirection file paths."""
        return self.getbool("pref.fs.no_spaces")

    @property
    def pref_fs_no_overwrite(self) -> bool:
        """Whether to forbid redirection to overwrite existing files."""
        return self.getbool("pref.fs.no_overwrite")

    @property
    def pref_sizes_si(self) -> bool:
        """Whether to print sizes with SI units (bytes, kB, MB)."""
        return self.getbool("pref.sizes_si")

    @property
    def pref_hex_upper(self) -> bool:
        """Whether to print hexadicimal digits upper case."""
        return self.getbool("pref.hex_upper")

    @property
    def pref_list_headers(self) -> bool:
        """Whether to show the headers in list views."""
        return self.getbool("pref.list.headers")

    @property
    def pref_list_placeholder(self) -> str:
        """Placeholder for missing values in list views."""
        return self.getstr("pref.list.place_holder")

    @property
    def pref_list_fmt(self) -> str:
        """Default format string for node fields in list views."""
        return self.getstr("pref.list.fmt")

    @property
    def pref_list_actionable_type(self) -> ActionableType:
        """Actionable type for list views."""
        return self.get_actionable_type("pref.list.actionable_type")

    @property
    def pref_list_multi(self) -> bool:
        """Whether to allow multiple-line cells in list views."""
        return self.getbool("pref.list.multi")

    @property
    def pref_tree_headers(self) -> bool:
        """Whether to show the headers in tree views."""
        return self.getbool("pref.tree.headers")

    @property
    def pref_tree_placeholder(self) -> str:
        """Placeholder for missing values in tree views."""
        return self.getstr("pref.tree.place_holder")

    @property
    def pref_tree_fmt(self) -> str:
        """Default format string for node fields in tree views."""
        return self.getstr("pref.tree.fmt")

    @property
    def pref_tree_actionable_type(self) -> ActionableType:
        """Actionable type for tree anchors."""
        return self.get_actionable_type("pref.tree.actionable_type")

    @property
    def pref_2Sided_actionable_type(self) -> ActionableType:
        """Actionable type for the left list view of a 2-sided."""
        return self.get_actionable_type("pref.2sided.actionable_type")

    @property
    def pref_tree_cb_anchor(self) -> str:
        """Symbol to anchor child-bindings to their parent in tree-views."""
        return self.getstr("pref.tree.cb_anchor")

    @property
    def pref_actionable_type(self) -> ActionableType:
        """Default rendering for actionable texts."""
        actionable_type = self.getstr("pref.actionable_type")
        try:
            return ActionableType(actionable_type)
        except ValueError:
            print(
                f"Invalid actionable type: {actionable_type}", file=sys.stderr
            )
        return ActionableType.LINK

    @property
    def pref_actionable_text(self) -> str:
        """Alternate actionable view."""
        return self.getstr("pref.actionable_wchar")

    @property
    def pref_svg_theme(self) -> str:
        """CSS theme for command output redirection to SVG."""
        return self.getstr("pref.svg.theme")

    @property
    def pref_svg_font_family(self) -> str:
        """Font family for command output redirection to SVG."""
        return self.getstr("pref.svg.font_family")

    @property
    def pref_svg_font_ratio(self) -> float:
        """Font aspect ratio for command output redirection to SVG."""
        return self.getfloat("pref.svg.font_ratio")

    @property
    def pref_html_theme(self) -> str:
        """CSS theme for command output redirection to HTML."""
        return self.getstr("pref.html.theme")

    @property
    def pref_html_font_family(self) -> str:
        """Font family for command output redirection to HTML."""
        return self.getstr("pref.html.font_family")

    def init_user_files(self) -> int:
        """Initialize per-user configuration files."""
        if os.path.isdir(self._app_dir):
            src_dir = os.path.dirname(os.path.abspath(__file__))

            try:
                dst = self.get_user_file("dtsh.ini")
                if os.path.exists(dst):
                    print(f"File exists, skipped: {dst}")
                else:
                    shutil.copyfile(os.path.join(src_dir, "dtsh.ini"), dst)
                    print(f"User preferences: {dst}")

                dst = self.get_user_file("theme.ini")
                if os.path.exists(dst):
                    print(f"File exists, skipped: {dst}")
                else:
                    shutil.copyfile(
                        os.path.join(src_dir, "rich", "theme.ini"), dst
                    )
                    print(f"User theme: {dst}")

                return 0

            except (OSError, OSError) as e:
                print(f"Failed to create file: {dst}", file=sys.stderr)
                print(f"Cause: {e}", file=sys.stderr)

        # Per-user configuration files don't exist (-ENOENT).
        return -2

    def get_user_file(self, *paths: str) -> str:
        """Get path to a use file within the DTSh application directory.

        Args:
            paths: Relative path to the resource.
        """
        return os.path.join(self._app_dir, *paths)

    def getbool(self, option: str, fallback: bool = False) -> bool:
        """Access a configuration option's value as a boolean.

        Boolean:
        - True: '1', 'yes', 'true', and 'on'
        - False: '0', 'no', 'false', and 'off'

        Args:
            option: The option's name.
            fallback: If set, represents the fall-back value
              for an undefined option or an invalid value.
              Defaults to False.

        Returns:
            The option's value as a boolean.
        """
        try:
            return self._cfg.getboolean("dtsh", option)
        except (configparser.Error, ValueError) as e:
            print(f"configuration error: {option}: {e}", file=sys.stderr)
        return fallback

    def getint(self, option: str, fallback: int = 0) -> int:
        """Access a configuration option's value as a integer.

        Integers:

        - base-2, -8, -10 and -16 are supported
        - if not base 10, the actual base is determined
          by the prefix "0b/0B" (base-2), "0o/0O" (base-8),
          or "0x/0X" (base-16)

        Args:
            option: The option's name.
            fallback: If set, represents the fall-back value
              for an undefined option or an invalid value.
              Defaults to 0.

        Returns:
            The option's value as an integer.
        """
        try:
            return int(self._cfg.get("dtsh", option), base=0)
        except (configparser.Error, ValueError) as e:
            print(f"configuration error: {option}: {e}", file=sys.stderr)
        return fallback

    def getfloat(self, option: str, fallback: float = 0) -> float:
        """Access a configuration option's value as float.

        Args:
            option: The option's name.
            fallback: If set, represents the fall-back value
              for an undefined option or an invalid value.
              Defaults to 0.

        Returns:
            The option's value as a float.
        """
        try:
            return float(self._cfg.get("dtsh", option))
        except (configparser.Error, ValueError) as e:
            print(f"configuration error: {option}: {e}", file=sys.stderr)
        return fallback

    def getstr(self, option: str, fallback: str = "") -> str:
        r"""Access a configuration option's value as wide string.

        Wide strings may contain:
        - actual Unicode characters (e.g. "↗")
        - UTF-8 character literals (e.g. "\u276d")
        - other ASCII escape sequences (e.g. "\t")

        Double-quotes are optional, excepted when the string value
        ends with trailing spaces (e.g. "❯ ").

        Args:
            option: The option's name.
            fallback: If set, represents the fall-back value
              for an undefined option or an invalid value.
              Defaults to an empty string.

        Returns:
            The option's value as a wide string.
        """
        # To support what we named "wide strings", relying on Python UTF-8
        # codecs won't work:
        #
        #     "00î00".encode('utf-8').decode('unicode_escape')
        #     '00Ã®00'
        #
        # Reason (see "unicode_escape doesn't work in general" bellow):
        # The unicode_escape codec, despite its name, turns out to assume
        # that all non-ASCII bytes are in the Latin-1 (ISO-8859-1) encoding.
        #
        # The best approach seems to be:
        # - to not encode values, Python 3 strings are already Unicode
        # - decode only escape sequences
        #
        # Adapted from @rspeer answer "unicode_escape doesn't work in general"
        # at https://stackoverflow.com/a/24519338.
        try:
            # Quoted strings permit to define empty strings and strings
            # that end with spaces: strip leading and trailing spaces.
            val = self._cfg.get("dtsh", option).strip('"')
            # Multi-line strings permit to define long strings: replace
            # newlines with spaces.
            val = val.replace("\n", " ")
            return str(
                DTShConfig._RE_ESCAPE_SEQ.sub(
                    lambda match: codecs.decode(
                        match.group(0), "unicode-escape"
                    ),
                    val,
                )
            )
        except configparser.Error as e:
            print(f"Configuration error: {option}: {e}", file=sys.stderr)
        return fallback

    def get_actionable_type(self, pref: str) -> ActionableType:
        """Access a preference as actionable type."""
        actionable_type = self.getstr(pref)
        try:
            return ActionableType(actionable_type)
        except ValueError:
            print(
                f"{pref}: invalid actionable type '{actionable_type}'",
                file=sys.stderr,
            )
        # Fall-back, we don't want to fault.
        return ActionableType.LINK

    def load_ini_file(self, path: str) -> None:
        """Load options from configuration file (INI format).

        Overrides already loaded values with the same keys.

        Args:
            path: Path to a configuration file.

        Raises:
            DTShConfig.Error: Failed to load configuration file.
        """
        try:
            f = open(  # pylint: disable=consider-using-with
                path, "r", encoding="utf-8"
            )
            self._cfg.read_file(f)

        except (OSError, configparser.Error) as e:
            raise DTShConfig.Error(str(e)) from e

    def _init_app_dir(self) -> None:
        if sys.platform == "darwin":
            self._app_dir = self._init_app_dir_darwin()
        elif os.name == "nt":
            self._app_dir = self._init_app_dir_nt()
        else:
            self._app_dir = self._init_app_dir_posix()

        if not os.path.isdir(self._app_dir):
            try:
                os.makedirs(self._app_dir, mode=0o750)
            except OSError as e:
                print(
                    f"Failed to create directory: {self._app_dir}",
                    file=sys.stderr,
                )
                print(f"Cause: {e}", file=sys.stderr)

    def _init_app_dir_darwin(self) -> str:
        return os.path.abspath(
            os.path.join(os.path.expanduser("~"), "Library", "DTSh")
        )

    def _init_app_dir_nt(self) -> str:
        local_app_data = os.environ.get(
            "LOCALAPPDATA",
            os.path.join(os.path.expanduser("~"), "AppData", "Local"),
        )
        return os.path.abspath(os.path.join(local_app_data, "DTSh"))

    def _init_app_dir_posix(self) -> str:
        xdg_cfg_home = os.environ.get(
            "XDG_CONFIG_HOME",
            os.path.join(os.path.expanduser("~"), ".config"),
        )
        return os.path.abspath(os.path.join(xdg_cfg_home, "dtsh"))


_dtshconf = DTShConfig()
