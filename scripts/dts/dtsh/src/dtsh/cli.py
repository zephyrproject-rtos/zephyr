# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""Devicetree shell CLI.

This module permits to open a DTSh session:
- above an existing command line arguments parser: we'll then assume
  that some external code is responsible for calling parse_args() on this parser,
  and providing the session with the parsed arguments (e.g. for integration
  with the WestCommand workflow)
- using the default arguments parser: the CLI is then responsible
  for parsing the command line
"""

from typing import cast, Optional, Union, List

import argparse
import os
import sys

from dtsh.config import DTShConfig
from dtsh.shell import DTShError
from dtsh.rich.theme import DTShTheme
from dtsh.rich.session import DTShRichSession


class DTShArgvParser(argparse.ArgumentParser):
    """Command line arguments parser."""

    @staticmethod
    def init(parser: argparse.ArgumentParser) -> None:
        """Add DTSh command line arguments to parser."""
        grp_open_dts = parser.add_argument_group("open a DTS file")
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

        grp_user_files = parser.add_argument_group("user files")
        grp_user_files.add_argument(
            "-u",
            "--user-files",
            help="initialize per-user configuration files and exit",
            action="store_true",
        )
        grp_user_files.add_argument(
            "--preferences",
            help="load additional preferences file",
            metavar="FILE",
        )
        grp_user_files.add_argument(
            "--theme",
            help="load additional styles file",
            metavar="FILE",
        )

        grp_session_ctrl = parser.add_argument_group("session control")
        grp_session_ctrl.add_argument(
            "-c",
            help="execute CMD at startup (may be repeated)",
            action="append",
            metavar="CMD",
        )
        grp_session_ctrl.add_argument(
            "-f",
            help="execute batch commands from FILE at startup",
            metavar="FILE",
        )
        grp_session_ctrl.add_argument(
            "-i",
            "--interactive",
            help="enter interactive loop after batch commands",
            action="store_true",
        )

    def __init__(self) -> None:
        """Initialize a default parser with DTSh arguments."""
        super().__init__(
            prog="dtsh",
            description="shell-like interface with Devicetree",
            # See e.g. https://github.com/zephyrproject-rtos/zephyr/issues/53495
            allow_abbrev=False,
        )
        DTShArgvParser.init(self)


class DTShCliArgs:
    """Parsed command line arguments."""

    _args: argparse.Namespace

    def __init__(self, args: argparse.Namespace) -> None:
        """Initialize arguments.

        Args:
            args: Parsed command line arguments as defined
              by ArgumentParser.parse_args().
        """
        self._args = args

    @property
    def binding_dirs(self) -> Optional[List[str]]:
        """Directories to search for binding files."""
        if self._args.bindings:
            return cast(List[str], self._args.bindings)
        return None

    @property
    def dts(self) -> str:
        """Path to the Devicetree source file."""
        if self._args.dts:
            return cast(str, self._args.dts)
        return os.path.join(os.path.abspath("build"), "zephyr", "zephyr.dts")

    @property
    def user_files(self) -> bool:
        """Initialize user files and exit."""
        return bool(self._args.user_files)

    @property
    def preferences(self) -> Optional[str]:
        """Additional preferences file."""
        if self._args.preferences:
            return cast(str, self._args.preferences)
        return None

    @property
    def theme(self) -> Optional[str]:
        """Additional styles file."""
        if self._args.theme:
            return cast(str, self._args.theme)
        return None

    @property
    def batch_source(self) -> Optional[Union[str, List[str]]]:
        """Batch command source, if defined."""
        if self._args.c:
            return cast(List[str], self._args.c)
        if self._args.f:
            return cast(str, self._args.f)
        return None

    @property
    def interactive(self) -> bool:
        """Is the interactive loop requested?"""
        if not (self._args.c or self._args.f):
            # no batch input, must be interactive
            return True
        return cast(bool, self._args.interactive)


class DTShCli:
    """Command line interface."""

    _parser: argparse.ArgumentParser

    @staticmethod
    def load_preference_file(path: str) -> None:
        """Load an additional preferences file above the default one.

        Raises:
            DTShError when loading the preferences file has failed.
        """
        try:
            DTShConfig.getinstance().load_ini_file(path)
        except DTShConfig.Error as e:
            raise DTShError(f"failed to load preferences file: {path}") from e

    @staticmethod
    def load_theme_file(path: str) -> None:
        """Load an additional theme file above the default one.

        Raises:
            DTShError when loading the theme file has failed.
        """
        try:
            DTShTheme.getinstance().load_theme_file(path)
        except DTShTheme.Error as e:
            raise DTShError(f"failed to load theme file: {path}") from e

    def __init__(
        self, parser: Optional[argparse.ArgumentParser] = None
    ) -> None:
        """Initialize CLI.

        Args:
            parser: If set, specify an existing parser to configure with
              DTSh command line arguments. If unset, the default parser is used.
        """
        if parser:
            self._parser = parser
        else:
            self._parser = DTShArgvParser()

    def run(self, args: Optional[argparse.Namespace] = None) -> None:
        """Run the command line interface.

        Args:
            args: Parsed arguments, e.g. when called from a West command.
              If unset, get the command line arguments with the default parser.
        """
        if args is None:
            args = self._parser.parse_args()
        if args.f and args.c:
            self._parser.error("-c and -f are mutually exclusive")

        cli_args = DTShCliArgs(args)

        if cli_args.user_files:
            # Initialize per-user configuration files and exit.
            ret = DTShConfig.getinstance().init_user_files()
            sys.exit(ret)

        if cli_args.preferences:
            # Load additional preference file.
            DTShCli.load_preference_file(cli_args.preferences)

        if cli_args.theme:
            # Load additional styles file.
            DTShCli.load_theme_file(cli_args.theme)

        if cli_args.batch_source:
            session = DTShRichSession.create_batch(
                cli_args.dts,
                cli_args.binding_dirs,
                cli_args.batch_source,
                cli_args.interactive,
            )
        else:
            session = DTShRichSession.create(
                cli_args.dts, cli_args.binding_dirs
            )

        session.run(cli_args.interactive)


def run() -> None:
    """Installed entry point."""
    cli = DTShCli()

    try:
        cli.run()
    except DTShError as e:
        print(f"dtsh: error: {e.msg}", file=sys.stderr)
        sys.exit(2)

    sys.exit(0)


if __name__ == "__main__":
    run()
