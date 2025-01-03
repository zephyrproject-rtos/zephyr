# Copyright (c) 2024 Tenstorrent AI ULC
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import hashlib
import os
import shlex
import subprocess
import textwrap
from pathlib import Path

import pykwalify.core
import yaml
from west.commands import WestCommand

try:
    from yaml import CSafeLoader as SafeLoader
except ImportError:
    from yaml import SafeLoader

WEST_PATCH_SCHEMA_PATH = Path(__file__).parents[1] / "schemas" / "patch-schema.yml"
with open(WEST_PATCH_SCHEMA_PATH) as f:
    patches_schema = yaml.load(f, Loader=SafeLoader)

WEST_PATCH_BASE = Path("zephyr") / "patches"
WEST_PATCH_YAML = Path("zephyr") / "patches.yml"

_WEST_MANIFEST_DIR = Path("WEST_MANIFEST_DIR")
_WEST_TOPDIR = Path("WEST_TOPDIR")


class Patch(WestCommand):
    def __init__(self):
        super().__init__(
            "patch",
            "apply patches to the west workspace",
            "Apply patches to the west workspace",
            accepts_unknown_args=False,
        )

    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(
            self.name,
            help=self.help,
            formatter_class=argparse.RawDescriptionHelpFormatter,
            description=self.description,
            epilog=textwrap.dedent("""\
            Applying Patches:

                Run "west patch apply" to apply patches.
                See "west patch apply --help" for details.

            Cleaning Patches:

                Run "west patch clean" to clean patches.
                See "west patch clean --help" for details.

            Listing Patches:

                Run "west patch list" to list patches.
                See "west patch list --help" for details.

            YAML File Format:

            The patches.yml syntax is described in "scripts/schemas/patch-schema.yml".

            patches:
              - path: zephyr/kernel-pipe-fix-not-k-no-wait-and-ge-min-xfer-bytes.patch
                sha256sum: e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
                module: zephyr
                author: Kermit D. Frog
                email: itsnoteasy@being.gr
                date: 2020-04-20
                upstreamable: true
                merge-pr: https://github.com/zephyrproject-rtos/zephyr/pull/24486
                issue: https://github.com/zephyrproject-rtos/zephyr/issues/24485
                merge-status: true
                merge-commit: af926ae728c78affa89cbc1de811ab4211ed0f69
                merge-date: 2020-04-27
                apply-command: git apply
                comments: |
                  Songs about rainbows - why are there so many??
                custom:
                  possible-muppets-to-ask-for-clarification-with-the-above-question:
                    - Miss Piggy
                    - Gonzo
                    - Fozzie Bear
                    - Animal
            """),
        )

        parser.add_argument(
            "-b",
            "--patch-base",
            help="Directory containing patch files",
            metavar="DIR",
            default=_WEST_MANIFEST_DIR / WEST_PATCH_BASE,
            type=Path,
        )
        parser.add_argument(
            "-l",
            "--patch-yml",
            help="Path to patches.yml file",
            metavar="FILE",
            default=_WEST_MANIFEST_DIR / WEST_PATCH_YAML,
            type=Path,
        )
        parser.add_argument(
            "-w",
            "--west-workspace",
            help="West workspace",
            metavar="DIR",
            default=_WEST_TOPDIR,
            type=Path,
        )

        subparsers = parser.add_subparsers(
            dest="subcommand",
            metavar="<subcommand>",
            help="select a subcommand. If omitted treat it as 'list'",
        )

        apply_arg_parser = subparsers.add_parser(
            "apply",
            help="Apply patches",
            formatter_class=argparse.RawDescriptionHelpFormatter,
            epilog=textwrap.dedent(
                """
            Applying Patches:

                Run "west patch apply" to apply patches.
            """
            ),
        )
        apply_arg_parser.add_argument(
            "-r",
            "--roll-back",
            help="Roll back if any patch fails to apply",
            action="store_true",
            default=False,
        )

        subparsers.add_parser(
            "clean",
            help="Clean patches",
            formatter_class=argparse.RawDescriptionHelpFormatter,
            epilog=textwrap.dedent(
                """
            Cleaning Patches:

                Run "west patch clean" to clean patches.
            """
            ),
        )

        subparsers.add_parser(
            "list",
            help="List patches",
            formatter_class=argparse.RawDescriptionHelpFormatter,
            epilog=textwrap.dedent(
                """
            Listing Patches:

                Run "west patch list" to list patches.
            """
            ),
        )

        return parser

    def filter_args(self, args):
        try:
            manifest_path = self.config.get("manifest.path")
        except BaseException:
            self.die("could not retrieve manifest path from west configuration")

        topdir = Path(self.topdir)
        manifest_dir = topdir / manifest_path

        if args.patch_base.is_relative_to(_WEST_MANIFEST_DIR):
            args.patch_base = manifest_dir / args.patch_base.relative_to(_WEST_MANIFEST_DIR)
        if args.patch_yml.is_relative_to(_WEST_MANIFEST_DIR):
            args.patch_yml = manifest_dir / args.patch_yml.relative_to(_WEST_MANIFEST_DIR)
        if args.west_workspace.is_relative_to(_WEST_TOPDIR):
            args.west_workspace = topdir / args.west_workspace.relative_to(_WEST_TOPDIR)

    def do_run(self, args, _):
        self.filter_args(args)

        if not os.path.isfile(args.patch_yml):
            self.inf(f"no patches to apply: {args.patch_yml} not found")
            return

        west_config = Path(args.west_workspace) / ".west" / "config"
        if not os.path.isfile(west_config):
            self.die(f"{args.west_workspace} is not a valid west workspace")

        try:
            with open(args.patch_yml) as f:
                yml = yaml.load(f, Loader=SafeLoader)
            if not yml:
                self.inf(f"{args.patch_yml} is empty")
                return
            pykwalify.core.Core(source_data=yml, schema_data=patches_schema).validate()
        except (yaml.YAMLError, pykwalify.errors.SchemaError) as e:
            self.die(f"ERROR: Malformed yaml {args.patch_yml}: {e}")

        if not args.subcommand:
            args.subcommand = "list"

        method = {
            "apply": self.apply,
            "clean": self.clean,
            "list": self.list,
        }

        method[args.subcommand](args, yml)

    def apply(self, args, yml):
        patches = yml.get("patches", [])
        if not patches:
            return

        patch_count = 0
        failed_patch = None
        patched_mods = set()

        for patch_info in patches:
            pth = patch_info["path"]
            patch_path = os.path.realpath(Path(args.patch_base) / pth)

            apply_cmd = patch_info["apply-command"]
            apply_cmd_list = shlex.split(apply_cmd)

            self.dbg(f"reading patch file {pth}")
            patch_file_data = None

            try:
                with open(patch_path, "rb") as pf:
                    patch_file_data = pf.read()
            except Exception as e:
                self.err(f"failed to read {pth}: {e}")
                failed_patch = pth
                break

            self.dbg("checking patch integrity... ", end="")
            expect_sha256 = patch_info["sha256sum"]
            hasher = hashlib.sha256()
            hasher.update(patch_file_data)
            actual_sha256 = hasher.hexdigest()
            if actual_sha256 != expect_sha256:
                self.dbg("FAIL")
                self.err(
                    f"sha256 mismatch for {pth}:\n"
                    f"expect: {expect_sha256}\n"
                    f"actual: {actual_sha256}"
                )
                failed_patch = pth
                break
            self.dbg("OK")
            patch_count += 1
            patch_file_data = None

            mod = patch_info["module"]
            mod_path = Path(args.west_workspace) / mod
            patched_mods.add(mod)

            self.dbg(f"patching {mod}... ", end="")
            origdir = os.getcwd()
            os.chdir(mod_path)
            apply_cmd += patch_path
            apply_cmd_list.extend([patch_path])
            proc = subprocess.run(apply_cmd_list)
            if proc.returncode:
                self.dbg("FAIL")
                self.err(proc.stderr)
                failed_patch = pth
                break
            self.dbg("OK")
            os.chdir(origdir)

        if not failed_patch:
            self.inf(f"{patch_count} patches applied successfully \\o/")
            return

        if args.roll_back:
            self.clean(args, yml, patched_mods)

        self.die(f"failed to apply patch {pth}")

    def clean(self, args, yml, mods=None):
        clean_cmd = yml["clean-command"]
        checkout_cmd = yml["checkout-command"]

        if not clean_cmd and not checkout_cmd:
            self.dbg("no clean or checkout commands specified")
            return

        clean_cmd_list = shlex.split(clean_cmd)
        checkout_cmd_list = shlex.split(checkout_cmd)

        origdir = os.getcwd()
        for mod, mod_path in Patch.get_mod_paths(args, yml).items():
            if mods and mod not in mods:
                continue
            try:
                os.chdir(mod_path)

                if checkout_cmd:
                    self.dbg(f"Running '{checkout_cmd}' in {mod}.. ", end="")
                    proc = subprocess.run(checkout_cmd_list, capture_output=True)
                    if proc.returncode:
                        self.dbg("FAIL")
                        self.err(f"{checkout_cmd} failed for {mod}\n{proc.stderr}")
                    else:
                        self.dbg("OK")

                if clean_cmd:
                    self.dbg(f"Running '{clean_cmd}' in {mod}.. ", end="")
                    proc = subprocess.run(clean_cmd_list, capture_output=True)
                    if proc.returncode:
                        self.dbg("FAIL")
                        self.err(f"{clean_cmd} failed for {mod}\n{proc.stderr}")
                    else:
                        self.dbg("OK")

            except Exception as e:
                # If this fails for some reason, just log it and continue
                self.err(f"failed to clean up {mod}: {e}")

        os.chdir(origdir)

    def list(self, args, yml):
        patches = yml.get("patches", [])
        if not patches:
            return

        for patch_info in patches:
            self.inf(patch_info)

    @staticmethod
    def get_mod_paths(args, yml):
        patches = yml.get("patches", [])
        if not patches:
            return {}

        mod_paths = {}
        for patch_info in patches:
            mod = patch_info["module"]
            mod_path = os.path.realpath(Path(args.west_workspace) / mod)
            mod_paths[mod] = mod_path

        return mod_paths
