# Copyright (c) 2024 Basalte bv
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import os
import platform
import shutil
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
            "",
            description="List and Install packages for Zephyr and modules",
            accepts_unknown_args=True,
        )

    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(
            self.name,
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

        uv_parser = subparsers_gen.add_parser(
            "uv",
            help="manage packages with uv",
            formatter_class=argparse.RawDescriptionHelpFormatter,
            epilog=textwrap.dedent(
                """
            Manage packages with uv:

                Run 'west packages uv' to print all requirement and override
                files needed by Zephyr and modules, formatted for use with
                'uv pip install'.

                Requirement files are taken from the 'pip' package-managers
                section, and override files from the 'uv' package-managers
                section, of a module's 'zephyr/module.yml'. Override files
                are emitted as '--override <file>' entries.
            """
            ),
        )

        uv_parser.add_argument(
            "--install",
            action="store_true",
            help="Install packages using uv instead of listing them. "
            "A single 'uv pip install' command is built and executed. "
            "Additional uv arguments can be passed after a -- separator "
            "from the original 'west packages uv --install' command. For example pass "
            "'--dry-run' to uv not to actually install anything, but print what would be.",
        )

        uv_parser.add_argument(
            "--ignore-venv-check",
            action="store_true",
            help="Ignore the virtual environment check. "
            "This is useful when running 'west packages uv --install' "
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

        if args.manager == "uv":
            return self.do_run_uv(args, unknown[1:])

        # Unreachable but print an error message if an implementation is missing.
        self.die(f'Unsupported package manager: "{args.manager}"')

    def collect_files(self, args):
        requirements = []
        overrides = []

        if not args.modules or "zephyr" in args.modules:
            requirements.append(ZEPHYR_BASE / "scripts/requirements.txt")

        for module in self.zephyr_modules:
            module_requirements, module_overrides = self.collect_module_files(module, args)
            requirements += module_requirements
            overrides += module_overrides

        return requirements, overrides

    def collect_module_files(self, module, args):
        module_name = module.meta.get("name")
        if args.modules and module_name not in args.modules:
            if args.install:
                self.dbg(f"Skipping module {module_name}")
            return [], []

        # Get the optional pip and uv sections from the package managers
        package_managers = module.meta.get("package-managers", {})
        pip = package_managers.get("pip")
        uv = package_managers.get("uv")
        if pip is None and uv is None:
            if args.install:
                self.dbg(f"Nothing to install for {module_name}")
            return [], []

        # Requirements files are declared under the pip section
        requirements = []
        if pip is not None:
            requirements = [Path(module.project) / r for r in pip.get("requirement-files", [])]

        # Override files are declared under the uv section
        overrides = []
        if uv is not None:
            overrides = [Path(module.project) / o for o in uv.get("override-files", [])]

        return requirements, overrides

    def do_install(self, base_cmd, requirement_args, extra_args, manager, cmdscript_name):
        if shutil.which(base_cmd[0]) is None:
            self.die(f'"{base_cmd[0]}" not found; is it installed and on PATH?')

        cmd = list(base_cmd)
        cmd += requirement_args
        cmd += extra_args
        self.dbg(quote_sh_list(cmd))

        # Use os.execvp to execute a new program, replacing the current west process,
        # this unloads all python modules first and allows the manager to update packages safely
        if platform.system() != 'Windows':
            os.execvp(cmd[0], cmd)

        # Only reachable on Windows systems
        # Windows does not really support os.execv:
        # https://github.com/python/cpython/issues/63323
        # https://github.com/python/cpython/issues/101191
        # Warn the users about permission errors as those reported in:
        # https://github.com/zephyrproject-rtos/zephyr/issues/100296
        cmdscript = PureWindowsPath(__file__).parents[1] / "utils" / cmdscript_name
        self.wrn(
            f"Updating packages on Windows with 'west packages {manager} --install', that are "
            "currently in use by west, results in permission errors. Leaving your "
            "environment with conflicting package versions. Recommended is to start with "
            "a new environment in that case.\n\n"
            "To avoid this using powershell run the following command instead:\n"
            f"{quote_sh_list(base_cmd)} @((west packages {manager}) -split ' ')\n\n"
            "Using cmd.exe execute the helper script:\n"
            f"cmd /c {cmdscript}\n\n"
            f"Running 'west packages {manager} --install -- --dry-run' can provide information "
            "without actually updating the environment."
        )
        subprocess.check_call(cmd)

    def do_run_pip(self, args, pip_args):
        requirements, _ = self.collect_files(args)

        if args.install:
            if not in_venv() and not args.ignore_venv_check:
                self.die("Running pip install outside of a virtual environment")

            if len(requirements) > 0:
                requirement_args = list(chain.from_iterable(("-r", str(r)) for r in requirements))
                self.do_install(
                    [sys.executable, "-m", "pip", "install"],
                    requirement_args,
                    pip_args,
                    "pip",
                    "west-packages-pip-install.cmd",
                )
            else:
                self.inf("Nothing to install")
            return

        self.inf("\n".join([f"-r {r}" for r in requirements]))

    def do_run_uv(self, args, uv_args):
        requirements, overrides = self.collect_files(args)

        if args.install:
            if len(requirements) > 0:
                requirement_args = list(chain.from_iterable(("-r", str(r)) for r in requirements))
                requirement_args += list(
                    chain.from_iterable(("--override", str(o)) for o in overrides)
                )
                self.do_install(
                    ["uv", "pip", "install"],
                    requirement_args,
                    uv_args,
                    "uv",
                    "west-packages-uv-install.cmd",
                )
            else:
                self.inf("Nothing to install")
            return

        lines = [f"-r {r}" for r in requirements] + [f"--override {o}" for o in overrides]
        self.inf("\n".join(lines))
