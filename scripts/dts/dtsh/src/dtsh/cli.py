# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""Devicetree shell CLI.

Run the devicetree shell without West.
"""

from typing import cast, Optional, List

import argparse
import os
import sys

from dtsh.config import DTShConfig
from dtsh.shell import DTShError
from dtsh.rich.theme import DTShTheme
from dtsh.rich.session import DTShRichSession


class DTShCliArgv:
    """Command line arguments parser."""

    _parser: argparse.ArgumentParser
    _argv: argparse.Namespace

    def __init__(self) -> None:
        self._parser = argparse.ArgumentParser(
            prog="dtsh",
            description="shell-like interface with Devicetree",
            # See e.g. https://github.com/zephyrproject-rtos/zephyr/issues/53495
            allow_abbrev=False,
        )

        grp_open_dts = self._parser.add_argument_group("open a DTS file")
        grp_open_dts.add_argument(
            "-b",
            "--bindings",
            help="directory to search for binding files",
            action="append",
            metavar="DIR",
        )
        grp_open_dts.add_argument(
            "dts", help="path to the DTS file", nargs="?", metavar="DTS"
        )

        grp_user_files = self._parser.add_argument_group("user files")
        grp_user_files.add_argument(
            "-u",
            "--user-files",
            help="initialize per-user configuration files and exit",
            action="store_true",
        )
        grp_user_files.add_argument(
            "--preferences",
            help="load additional preferences file",
            nargs=1,
            metavar="FILE",
        )
        grp_user_files.add_argument(
            "--theme",
            help="load additional styles file",
            nargs=1,
            metavar="FILE",
        )

        self._argv = self._parser.parse_args()

    @property
    def binding_dirs(self) -> Optional[List[str]]:
        """Directories to search for binding files."""
        if self._argv.bindings:
            return cast(List[str], self._argv.bindings)
        return None

    @property
    def dts(self) -> str:
        """Path to the Devicetree source file."""
        if self._argv.dts:
            return str(self._argv.dts)
        return os.path.join(os.path.abspath("build"), "zephyr", "zephyr.dts")

    @property
    def user_files(self) -> bool:
        """Initialize user files and exit."""
        return bool(self._argv.user_files)

    @property
    def preferences(self) -> Optional[str]:
        """Additional preferences file."""
        return (
            str(self._argv.preferences[0]) if self._argv.preferences else None
        )

    @property
    def theme(self) -> Optional[str]:
        """Additional styles file."""
        return str(self._argv.theme[0]) if self._argv.theme else None


def _load_preference_file(path: str) -> None:
    try:
        DTShConfig.getinstance().load_ini_file(path)
    except DTShConfig.Error as e:
        print(e, file=sys.stderr)
        print(f"Failed to load preferences file: {path}", file=sys.stderr)
        sys.exit(-22)


def _load_theme_file(path: str) -> None:
    try:
        DTShTheme.getinstance().load_theme_file(path)
    except DTShConfig.Error as e:
        print(e, file=sys.stderr)
        print(f"Failed to load styles file: {path}", file=sys.stderr)
        sys.exit(-22)


def run() -> None:
    """Open a devicetree shell session and run its interactive loop."""
    argv = DTShCliArgv()

    if argv.user_files:
        # Initialize per-user configuration files and exit.
        ret = DTShConfig.getinstance().init_user_files()
        sys.exit(ret)

    if argv.preferences:
        # Load additional preference file.
        _load_preference_file(argv.preferences)

    if argv.theme:
        # Load additional styles file.
        _load_theme_file(argv.theme)

    session = None
    try:
        session = DTShRichSession.create(argv.dts, argv.binding_dirs)
    except DTShError as e:
        print(e.msg, file=sys.stderr)
        print("Failed to initialize devicetree", file=sys.stderr)
        sys.exit(-22)

    if session:
        session.run()


if __name__ == "__main__":
    run()
