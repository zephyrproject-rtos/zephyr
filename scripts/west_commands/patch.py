# Copyright (c) 2024 Tenstorrent AI ULC
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import hashlib
import os
import re
import shlex
import subprocess
import textwrap
import urllib.request
from pathlib import Path

import pykwalify.core
import yaml
from west.commands import WestCommand

try:
    from yaml import CSafeDumper as SafeDumper
    from yaml import CSafeLoader as SafeLoader
except ImportError:
    from yaml import SafeDumper, SafeLoader

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

            Fetching Patches:

                Run "west patch gh-fetch" to fetch patches from Github.
                See "west patch gh-fetch --help" for details.

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
        parser.add_argument(
            "-m",
            "--module",
            action="append",
            dest="modules",
            metavar="DIR",
            type=Path,
            help="Zephyr module directory to run the 'patch' command for. "
            "Option can be passed multiple times. "
            "If this option is not given, the 'patch' command will run for Zephyr "
            "and all modules.",
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

        gh_fetch_arg_parser = subparsers.add_parser(
            "gh-fetch",
            help="Fetch patch from Github",
            formatter_class=argparse.RawDescriptionHelpFormatter,
            epilog=textwrap.dedent(
                """
            Fetching Patches from Github:

                Run "west patch gh-fetch" to fetch a PR from Github and store it as a patch.
                The meta data is generated and appended to the provided patches.yml file.

                If no patches.yml file exists, it will be created.
            """
            ),
        )
        gh_fetch_arg_parser.add_argument(
            "-o",
            "--owner",
            action="store",
            default="zephyrproject-rtos",
            help="Github repository owner",
        )
        gh_fetch_arg_parser.add_argument(
            "-r",
            "--repo",
            action="store",
            default="zephyr",
            help="Github repository",
        )
        gh_fetch_arg_parser.add_argument(
            "-pr",
            "--pull-request",
            metavar="ID",
            action="store",
            required=True,
            type=int,
            help="Github Pull Request ID",
        )
        gh_fetch_arg_parser.add_argument(
            "-m",
            "--module",
            metavar="DIR",
            action="store",
            required=True,
            type=Path,
            help="Module path",
        )
        gh_fetch_arg_parser.add_argument(
            "-s",
            "--split-commits",
            action="store_true",
            help="Create patch files for each commit instead of a single patch for the entire PR",
        )
        gh_fetch_arg_parser.add_argument(
            '-t',
            '--token',
            metavar='FILE',
            dest='tokenfile',
            help='File containing GitHub token (alternatively, use GITHUB_TOKEN env variable)',
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

    def load_yml(self, args, allow_missing):
        if not os.path.isfile(args.patch_yml):
            if not allow_missing:
                self.inf(f"no patches to apply: {args.patch_yml} not found")
                return None

            # Return the schema defaults
            return pykwalify.core.Core(source_data={}, schema_data=patches_schema).validate()

        try:
            with open(args.patch_yml) as f:
                yml = yaml.load(f, Loader=SafeLoader)
            return pykwalify.core.Core(source_data=yml, schema_data=patches_schema).validate()
        except (yaml.YAMLError, pykwalify.errors.SchemaError) as e:
            self.die(f"ERROR: Malformed yaml {args.patch_yml}: {e}")

    def do_run(self, args, _):
        self.filter_args(args)

        west_config = Path(args.west_workspace) / ".west" / "config"
        if not os.path.isfile(west_config):
            self.die(f"{args.west_workspace} is not a valid west workspace")

        yml = self.load_yml(args, args.subcommand in ["gh-fetch"])
        if yml is None:
            return

        if not args.subcommand:
            args.subcommand = "list"

        method = {
            "apply": self.apply,
            "clean": self.clean,
            "list": self.list,
            "gh-fetch": self.gh_fetch,
        }

        method[args.subcommand](args, yml, args.modules)

    def apply(self, args, yml, mods=None):
        patches = yml.get("patches", [])
        if not patches:
            return

        patch_count = 0
        failed_patch = None
        patched_mods = set()

        for patch_info in patches:
            mod = Path(patch_info["module"])
            if mods and mod not in mods:
                continue

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

            mod_path = Path(args.west_workspace) / mod
            patched_mods.add(mod)

            self.dbg(f"patching {mod}... ", end="")
            apply_cmd += patch_path
            apply_cmd_list.extend([patch_path])
            proc = subprocess.run(apply_cmd_list, cwd=mod_path)
            if proc.returncode:
                self.dbg("FAIL")
                self.err(proc.stderr)
                failed_patch = pth
                break
            self.dbg("OK")

        if not failed_patch:
            self.inf(f"{patch_count} patches applied successfully \\o/")
            return

        if args.roll_back:
            self.clean(args, yml, patched_mods)

        self.die(f"failed to apply patch {failed_patch}")

    def clean(self, args, yml, mods=None):
        clean_cmd = yml["clean-command"]
        checkout_cmd = yml["checkout-command"]

        if not clean_cmd and not checkout_cmd:
            self.dbg("no clean or checkout commands specified")
            return

        clean_cmd_list = shlex.split(clean_cmd)
        checkout_cmd_list = shlex.split(checkout_cmd)

        for mod, mod_path in Patch.get_mod_paths(args, yml).items():
            if mods and mod not in mods:
                continue
            try:
                if checkout_cmd:
                    self.dbg(f"Running '{checkout_cmd}' in {mod}.. ", end="")
                    proc = subprocess.run(checkout_cmd_list, capture_output=True, cwd=mod_path)
                    if proc.returncode:
                        self.dbg("FAIL")
                        self.err(f"{checkout_cmd} failed for {mod}\n{proc.stderr}")
                    else:
                        self.dbg("OK")

                if clean_cmd:
                    self.dbg(f"Running '{clean_cmd}' in {mod}.. ", end="")
                    proc = subprocess.run(clean_cmd_list, capture_output=True, cwd=mod_path)
                    if proc.returncode:
                        self.dbg("FAIL")
                        self.err(f"{clean_cmd} failed for {mod}\n{proc.stderr}")
                    else:
                        self.dbg("OK")

            except Exception as e:
                # If this fails for some reason, just log it and continue
                self.err(f"failed to clean up {mod}: {e}")

    def list(self, args, yml, mods=None):
        patches = yml.get("patches", [])
        if not patches:
            return

        for patch_info in patches:
            if mods and Path(patch_info["module"]) not in mods:
                continue
            self.inf(patch_info)

    def gh_fetch(self, args, yml, mods=None):
        if mods:
            self.die(
                "Module filters are not available for the gh-fetch subcommand, "
                "pass a single -m/--module argument after the subcommand."
            )

        try:
            from github import Auth, Github
        except ImportError:
            self.die("PyGithub not found; can be installed with 'pip install PyGithub'")

        gh = Github(auth=Auth.Token(args.tokenfile) if args.tokenfile else None)
        pr = gh.get_repo(f"{args.owner}/{args.repo}").get_pull(args.pull_request)
        args.patch_base.mkdir(parents=True, exist_ok=True)

        if args.split_commits:
            for cm in pr.get_commits():
                subject = cm.commit.message.splitlines()[0]
                filename = "-".join(filter(None, re.split("[^a-zA-Z0-9]+", subject))) + ".patch"

                # No patch URL is provided by the API, but appending .patch to the HTML works too
                urllib.request.urlretrieve(f"{cm.html_url}.patch", args.patch_base / filename)

                patch_info = {
                    "path": filename,
                    "sha256sum": self.get_file_sha256sum(args.patch_base / filename),
                    "module": str(args.module),
                    "author": cm.commit.author.name or "Hidden",
                    "email": cm.commit.author.email or "hidden@github.com",
                    "date": cm.commit.author.date.strftime("%Y-%m-%d"),
                    "upstreamable": True,
                    "merge-pr": pr.html_url,
                    "merge-status": pr.merged,
                }

                yml.setdefault("patches", []).append(patch_info)
        else:
            filename = "-".join(filter(None, re.split("[^a-zA-Z0-9]+", pr.title))) + ".patch"
            urllib.request.urlretrieve(pr.patch_url, args.patch_base / filename)

            patch_info = {
                "path": filename,
                "sha256sum": self.get_file_sha256sum(args.patch_base / filename),
                "module": str(args.module),
                "author": pr.user.name or "Hidden",
                "email": pr.user.email or "hidden@github.com",
                "date": pr.created_at.strftime("%Y-%m-%d"),
                "upstreamable": True,
                "merge-pr": pr.html_url,
                "merge-status": pr.merged,
            }

            yml.setdefault("patches", []).append(patch_info)

        args.patch_yml.parent.mkdir(parents=True, exist_ok=True)
        with open(args.patch_yml, "w") as f:
            yaml.dump(yml, f, Dumper=SafeDumper)

    @staticmethod
    def get_file_sha256sum(filename: Path) -> str:
        with open(filename, "rb") as fp:
            digest = hashlib.file_digest(fp, "sha256")

        return digest.hexdigest()

    @staticmethod
    def get_mod_paths(args, yml):
        patches = yml.get("patches", [])
        if not patches:
            return {}

        mod_paths = {}
        for patch_info in patches:
            mod = Path(patch_info["module"])
            mod_path = os.path.realpath(Path(args.west_workspace) / mod)
            mod_paths[mod] = mod_path

        return mod_paths
