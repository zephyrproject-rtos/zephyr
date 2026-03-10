# Copyright (c) 2024 Basalte bv
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import os
import platform
import subprocess
import sys
import textwrap
from itertools import chain
from pathlib import Path, PureWindowsPath

from west.commands import WestCommand
from west.util import quote_sh_list

from zephyr_ext_common import ZEPHYR_BASE

sys.path.append(os.fspath(Path(__file__).parent.parent))
import zephyr_module


def in_venv() -> bool:
    return sys.prefix != sys.base_prefix


class Packages(WestCommand):
    def __init__(self):
        super().__init__(
            "packages",
            "manage packages for Zephyr",
            "List and Install packages for Zephyr and modules",
            accepts_unknown_args=True,
        )

    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(
            self.name,
            help=self.help,
            description=self.description,
            formatter_class=argparse.RawDescriptionHelpFormatter,
            epilog=textwrap.dedent(
                """
            Listing packages:

                Run 'west packages <manager>' to list all dependencies
                available from a given package manager, already
                installed and not. These can be filtered by module,
                see 'west packages <manager> --help' for details.
            """
            ),
        )

        parser.add_argument(
            "-m",
            "--module",
            action="append",
            default=[],
            dest="modules",
            metavar="<module>",
            help="Zephyr module to run the 'packages' command for. "
            "Use 'zephyr' if the 'packages' command should run for Zephyr itself. "
            "Option can be passed multiple times. "
            "If this option is not given, the 'packages' command will run for Zephyr "
            "and all modules.",
        )

        subparsers_gen = parser.add_subparsers(
            metavar="<manager>",
            dest="manager",
            help="select a manager.",
            required=True,
        )

        pip_parser = subparsers_gen.add_parser(
            "pip",
            help="manage pip packages",
            formatter_class=argparse.RawDescriptionHelpFormatter,
            epilog=textwrap.dedent(
                """
            Manage pip packages:

                Run 'west packages pip' to print all requirement files needed by
                Zephyr and modules.

                The output is compatible with the requirements file format itself.
            """
            ),
        )

        pip_parser.add_argument(
            "--install",
            action="store_true",
            help="Install pip requirements instead of listing them. "
            "A single 'pip install' command is built and executed. "
            "Additional pip arguments can be passed after a -- separator "
            "from the original 'west packages pip --install' command. For example pass "
            "'--dry-run' to pip not to actually install anything, but print what would be.",
        )

        pip_parser.add_argument(
            "--ignore-venv-check",
            action="store_true",
            help="Ignore the virtual environment check. "
            "This is useful when running 'west packages pip --install' "
            "in a CI environment where the virtual environment is not set up.",
        )

        return parser

    def do_run(self, args, unknown):
        if len(unknown) > 0 and unknown[0] != "--":
            self.die(
                f'Unknown argument "{unknown[0]}"; '
                'arguments for the manager should be passed after "--"'
            )

        # Store the zephyr modules for easier access
        self.zephyr_modules = zephyr_module.parse_modules(
            ZEPHYR_BASE, self.manifest, require_yaml_validation=False
        )

        if args.modules:
            # Check for unknown module names
            module_names = [m.meta.get("name") for m in self.zephyr_modules]
            module_names.append("zephyr")
            for m in args.modules:
                if m not in module_names:
                    self.die(f'Unknown zephyr module "{m}"')

        if args.manager == "pip":
            return self.do_run_pip(args, unknown[1:])

        # Unreachable but print an error message if an implementation is missing.
        self.die(f'Unsupported package manager: "{args.manager}"')

    def do_run_pip(self, args, manager_args):
        requirements = []

        if not args.modules or "zephyr" in args.modules:
            requirements.append(ZEPHYR_BASE / "scripts/requirements.txt")

        for module in self.zephyr_modules:
            module_name = module.meta.get("name")
            if args.modules and module_name not in args.modules:
                if args.install:
                    self.dbg(f"Skipping module {module_name}")
                continue

            # Get the optional pip section from the package managers
            pip = module.meta.get("package-managers", {}).get("pip")
            if pip is None:
                if args.install:
                    self.dbg(f"Nothing to install for {module_name}")
                continue

            # Add requirements files
            requirements += [Path(module.project) / r for r in pip.get("requirement-files", [])]

        if args.install:
            if not in_venv() and not args.ignore_venv_check:
                self.die("Running pip install outside of a virtual environment")

            if len(requirements) > 0:
                cmd = [sys.executable, "-m", "pip", "install"]
                cmd += chain.from_iterable([("-r", str(r)) for r in requirements])
                cmd += manager_args
                self.dbg(quote_sh_list(cmd))

                # Use os.execv to execute a new program, replacing the current west process,
                # this unloads all python modules first and allows for pip to update packages safely
                if platform.system() != 'Windows':
                    os.execv(cmd[0], cmd)

                # Only reachable on Windows systems
                # Windows does not really support os.execv:
                # https://github.com/python/cpython/issues/63323
                # https://github.com/python/cpython/issues/101191
                # Warn the users about permission errors as those reported in:
                # https://github.com/zephyrproject-rtos/zephyr/issues/100296
                cmdscript = (
                    PureWindowsPath(__file__).parents[1] / "utils" / "west-packages-pip-install.cmd"
                )
                self.wrn(
                    "Updating packages on Windows with 'west packages pip --install', that are "
                    "currently in use by west, results in permission errors. Leaving your "
                    "environment with conflicting package versions. Recommended is to start with "
                    "a new environment in that case.\n\n"
                    "To avoid this using powershell run the following command instead:\n"
                    f"{sys.executable} -m pip install @((west packages pip) -split ' ')\n\n"
                    "Using cmd.exe execute the helper script:\n"
                    f"cmd /c {cmdscript}\n\n"
                    "Running 'west packages pip --install -- --dry-run' can provide information "
                    "without actually updating the environment."
                )
                subprocess.check_call(cmd)
            else:
                self.inf("Nothing to install")
            return

        if len(manager_args) > 0:
            self.die(f'west packages pip does not support unknown arguments: "{manager_args}"')

        self.inf("\n".join([f"-r {r}" for r in requirements]))
