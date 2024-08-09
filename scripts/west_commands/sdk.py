# Copyright (c) 2024 TOKITA Hiroshi
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import hashlib
import os
import platform
import re
import requests
import semver
import shutil
import subprocess
import tarfile
import tempfile
import textwrap
import zcmake
from zipfile import ZipFile

from west.commands import WestCommand

if platform.system() == "Windows":
    from py7zr import SevenZipFile


class Sdk(WestCommand):
    def __init__(self):
        super().__init__(
            "sdk",
            "manage Zephyr SDK",
            "List and Install Zephyr SDK",
        )

    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(
            self.name,
            help=self.help,
            description=self.description,
            formatter_class=argparse.RawDescriptionHelpFormatter,
            epilog=textwrap.dedent(
                """
            Listing SDKs:

                Run 'west sdk' or 'west sdk list' to list installed SDKs.
                See 'west sdk list --help' for details.


            Installing SDK:

                Run 'west sdk install' to install Zephyr SDK.
                See 'west sdk install --help' for details.
            """
            ),
        )

        subparsers_gen = parser.add_subparsers(
            metavar="<subcommand>",
            dest="subcommand",
            help="select a subcommand. If omitted, treat it as the 'list' selected.",
        )

        subparsers_gen.add_parser(
            "list",
            help="list installed Zephyr SDKs",
            formatter_class=argparse.RawDescriptionHelpFormatter,
            epilog=textwrap.dedent(
                """
            Listing SDKs:

                Run 'west sdk' or 'west sdk list' command information about available SDKs is displayed.
            """
            ),
        )

        install_args_parser = subparsers_gen.add_parser(
            "install",
            help="install Zephyr SDK",
            formatter_class=argparse.RawDescriptionHelpFormatter,
            epilog=textwrap.dedent(
                """
            Installing SDK:

                Run 'west sdk install' to install Zephyr SDK.

                Set --version option to install the specified version of the SDK.
                If not specified, the install version detects from "${ZEPHYR_BASE}/SDK_VERSION file.
                SDKs older than 0.14.1 are not supported.

                You can specify the installation directory with --install-dir.
                If the specified version of the SDK is already installed,
                the already installed SDK will be used regardless of the settings of
                --install-dir.

                Typically, Zephyr SDK archives contain only a directory named zephyr-sdk-<version>
                at the top level.
                Extract the archive to the home directory if not specified --install-dir.
                In this case, SDK will install into ${HOME}/zephyr-sdk-<version>.
                If specified --install-dir, The `zephyr-sdk-<version>` will rename and relocate
                the specified location.

                --interactive, --toolchains, --no-toolchains and --no-hosttools options
                specify the behavior of the installer. Please see the description of each option.

                --personal-access-token specifies the GitHub personal access token.
                This helps to relax the limits on the number of REST API calls.

                --api-url specifies the REST API endpoint for GitHub releases information
                when installing the SDK from a different GitHub repository.
            """
            ),
        )

        install_args_parser.add_argument(
            "-v",
            "--version",
            default=None,
            nargs="?",
            metavar="SDK_VER",
            help="specifies the install version of Zephyr SDK. "
            "If not specified, the install version detects from "
            "${ZEPHYR_BASE}/SDK_VERSION file.",
        )
        install_args_parser.add_argument(
            "-d",
            "--install-dir",
            default=None,
            metavar="DIR",
            help="specifies the install destination directory.",
        )
        install_args_parser.add_argument(
            "-i",
            "--interactive",
            action="store_true",
            help="launches installer in interactive mode. "
            "--toolchains, --no-toolchains and --no-hosttools will be ignored if this option enabled.",
        )
        install_args_parser.add_argument(
            "-t",
            "--toolchains",
            metavar="TC",
            nargs="+",
            help="specifies the installation of toolchains. "
            "If this option is not given, all toolchains will be installed.",
        )
        install_args_parser.add_argument(
            "-T",
            "--no-toolchains",
            action="store_true",
            help="do not install toolchains. "
            "--toolchains will be ignored if this option is enabled.",
        )
        install_args_parser.add_argument(
            "-H",
            "--no-hosttools",
            action="store_true",
            help="do not install host-tools.",
        )
        install_args_parser.add_argument(
            "--personal-access-token", help="specify GitHub personal access token."
        )
        install_args_parser.add_argument(
            "--api-url",
            default="https://api.github.com/repos/zephyrproject-rtos/sdk-ng/releases",
            help="specifies the GitHub releases API endpoint for finding SDKs.",
        )

        return parser

    def os_arch_name(self):
        system = platform.system()
        machine = platform.machine()

        if system == "Linux":
            osname = "linux"
        elif system == "Darwin":
            osname = "macos"
        elif system == "Windows":
            osname = "windows"
        else:
            self.die(f"Unsupported system: {system}")

        if machine in ["aarch64", "arm64"]:
            arch = "aarch64"
        elif machine in ["x86_64", "AMD64"]:
            arch = "x86_64"
        else:
            self.die(f"Unsupported machine: {machine}")

        return (osname, arch)

    def detect_version(self, args):
        if args.version:
            version = args.version
        else:
            if os.environ["ZEPHYR_BASE"]:
                zephyr_base = os.environ["ZEPHYR_BASE"]
            else:
                zephyr_base = os.path.join(os.path.dirname(__file__), "..", "..")

            if not os.path.exists(os.path.join(zephyr_base, "SDK_VERSION")):
                self.die(f'{os.path.join(zephyr_base, "SDK_VERSION")} not exists.')

            with open(os.path.join(zephyr_base, "SDK_VERSION")) as f:
                version = f.readlines()[0].strip()
                self.inf(
                    f'Found \'{os.path.join(zephyr_base, "SDK_VERSION")}\', '
                    + f"installing version {version}."
                )

        try:
            semver.Version.parse(version)
        except Exception:
            self.die(f"Invalid version format: {version}")

        if semver.compare(version, "0.14.1") < 0:
            self.die(f"Versions older than v0.14.1 are not supported.")

        return version

    def fetch_releases(self, url, req_headers):
        """fetch releases data via GitHub REST API"""

        releases = []
        page = 1

        while True:
            params = {"page": page, "per_page": 100}
            resp = requests.get(url, headers=req_headers, params=params)
            if resp.status_code != 200:
                raise Exception(f"Failed to fetch: {resp.status_code}, {resp.text}")

            data = resp.json()
            if not data:
                break

            releases.extend(data)
            page += 1

        return releases

    def minimal_sdk_filename(self, release):
        (osname, arch) = self.os_arch_name()
        version = re.sub(r"^v", "", release["tag_name"])

        if semver.compare(version, "0.16.0") < 0:
            if osname == "windows":
                ext = ".zip"
            else:
                ext = ".tar.gz"
        else:
            if osname == "windows":
                ext = ".7z"
            else:
                ext = ".tar.xz"

        return "zephyr-sdk-" + version + "_" + osname + "-" + arch + "_minimal" + ext

    def minimal_sdk_sha256(self, sha256_list, release):
        name = self.minimal_sdk_filename(release)
        tuples = [(re.split(r"\s+", t)) for t in sha256_list.splitlines()]
        hashtable = {t[1]: t[0] for t in tuples}

        return hashtable[name]

    def minimal_sdk_url(self, release):
        name = self.minimal_sdk_filename(release)
        assets = release.get("assets", [])
        minimal_sdk_asset = next(filter(lambda x: x["name"] == name, assets))

        return minimal_sdk_asset["browser_download_url"]

    def sha256_sum_url(self, release):
        assets = release.get("assets", [])
        minimal_sdk_asset = next(filter(lambda x: x["name"] == "sha256.sum", assets))

        return minimal_sdk_asset["browser_download_url"]

    def download_and_extract(self, base_dir, dir_name, target_release, req_headers):
        self.inf("Fetching sha256...")
        sha256_url = self.sha256_sum_url(target_release)
        resp = requests.get(sha256_url, headers=req_headers, stream=True)
        if resp.status_code != 200:
            raise Exception(f"Failed to download {sha256_url}: {resp.status_code}")

        sha256 = self.minimal_sdk_sha256(resp.content.decode("UTF-8"), target_release)

        archive_url = self.minimal_sdk_url(target_release)
        self.inf(f"Downloading {archive_url}...")
        resp = requests.get(archive_url, headers=req_headers, stream=True)
        if resp.status_code != 200:
            raise Exception(f"Failed to download {archive_url}: {resp.status_code}")

        try:
            if not os.path.exists(base_dir):
                os.makedirs(base_dir)

            with tempfile.TemporaryDirectory(dir=base_dir) as tempdir:
                # download archive file
                filename = os.path.join(tempdir, re.sub(r"^.*/", "", archive_url))
                file = open(filename, mode="wb")
                total_length = int(resp.headers["Content-Length"])
                count = 0

                for chunk in resp.iter_content(chunk_size=8192):
                    file.write(chunk)
                    count = count + len(chunk)
                    self.inf(f"\r {count}/{total_length}", end="")
                self.inf()
                self.inf(f"Downloaded: {file.name}")
                file.close()

                # check sha256 hash
                with open(file.name, "rb") as sha256file:
                    digest = hashlib.sha256(sha256file.read()).hexdigest()
                    if sha256 != digest:
                        raise Exception(f"sha256 mismatched: {sha256}:{digest}")

                # extract archive file
                self.inf(f"Extract: {file.name}")
                if file.name.endswith(".tar.xz"):
                    with tarfile.open(file.name, mode="r:xz") as archive:
                        archive.extractall(path=tempdir)
                elif file.name.endswith(".tar.gz"):
                    with tarfile.open(file.name, mode="r:gz") as archive:
                        archive.extractall(path=tempdir)
                elif file.name.endswith(".7z"):
                    with SevenZipFile(file.name, mode="r") as archive:
                        archive.extractall(path=tempdir)
                elif file.name.endswith(".zip"):
                    with ZipFile(file.name, mode="r") as archive:
                        archive.extractall(path=tempdir)
                else:
                    raise Exception("Unknown file extension.")

                # We expect that only the zephyr-sdk-x.y.z directory will be present in the archive.
                extracted_dirs = [
                    d
                    for d in os.listdir(tempdir)
                    if os.path.isdir(os.path.join(tempdir, d))
                ]
                if len(extracted_dirs) != 1:
                    raise Exception("Unexpected archive format")

                # move to destination dir
                if dir_name:
                    dest_dir = os.path.join(base_dir, dir_name)
                else:
                    dest_dir = os.path.join(base_dir, extracted_dirs[0])

                if not os.path.exists(os.path.dirname(dest_dir)):
                    os.makedirs(os.path.dirname(dest_dir))

                self.inf(
                    f"Move: {os.path.join(tempdir, extracted_dirs[0])} to {dest_dir}."
                )
                shutil.move(os.path.join(tempdir, extracted_dirs[0]), dest_dir)

                return dest_dir
        except PermissionError as pe:
            self.die(pe)

    def run_setup(self, args, sdk_dir):
        if "Windows" == platform.system():
            setup = os.path.join(sdk_dir, "setup.cmd")
            optsep = "/"
        else:
            setup = os.path.join(sdk_dir, "setup.sh")
            optsep = "-"

        # Associate installed SDK so that it can be found.
        cmds_cmake_pkg = [setup, optsep + "c"]
        self.dbg("Run: ", cmds_cmake_pkg)
        result = subprocess.run(cmds_cmake_pkg)
        if result.returncode != 0:
            self.die(f"command \"{' '.join(cmds_cmake_pkg)}\" failed")

        cmds = [setup]

        if not args.interactive and not args.no_toolchains:
            if not args.toolchains:
                cmds.extend([optsep + "t", "all"])
            else:
                for tc in args.toolchains:
                    cmds.extend([optsep + "t", tc])

        if not args.interactive and not args.no_hosttools:
            cmds.extend([optsep + "h"])

        if args.interactive or len(cmds) != 1:
            self.dbg("Run: ", cmds)
            result = subprocess.run(cmds)
            if result.returncode != 0:
                self.die(f"command \"{' '.join(cmds)}\" failed")

    def install_sdk(self, args, user_args):
        version = self.detect_version(args)
        (osname, arch) = self.os_arch_name()

        if args.personal_access_token:
            req_headers = {
                "Authorization": "Bearer " + args.personal_access_token,
            }
        else:
            req_headers = {}

        self.inf("Fetching Zephyr SDK list...")
        releases = self.fetch_releases(args.api_url, req_headers)
        self.dbg("releases: ", "\n".join([x["tag_name"] for x in releases]))

        # checking version
        def check_semver(version):
            try:
                semver.Version.parse(version)
                return True
            except Exception:
                return False

        available_versions = [
            re.sub(r"^v", "", x["tag_name"])
            for x in releases
            if check_semver(re.sub(r"^v", "", x["tag_name"]))
        ]

        if not version in available_versions:
            self.die(
                f"Unavailable SDK version: {version}."
                + "Please select from the list below:\n"
                + "\n".join(available_versions)
            )

        target_release = [x for x in releases if x["tag_name"] == "v" + version][0]

        # checking toolchains parameters
        assets = target_release["assets"]
        self.dbg("assets: ", "\n".join([x["browser_download_url"] for x in assets]))

        prefix = f"toolchain_{osname}-{arch}_"
        available_toolchains = [
            re.sub(r"\..*", "", x["name"].replace(prefix, ""))
            for x in assets
            if x["name"].startswith(prefix)
        ]

        if args.toolchains:
            for tc in args.toolchains:
                if not tc in available_toolchains:
                    self.die(
                        f"toolchain {tc} is not available.\n"
                        + "Please select from the list below:\n"
                        + "\n".join(available_toolchains)
                    )

        installed_info = [v for (k, v) in self.fetch_sdk_info().items() if k == version]
        if len(installed_info) == 0:
            if args.install_dir:
                base_dir = os.path.dirname(args.install_dir)
                dir_name = os.path.basename(args.install_dir)
            else:
                base_dir = os.path.expanduser("~")
                dir_name = None

            sdk_dir = self.download_and_extract(
                base_dir, dir_name, target_release, req_headers
            )
        else:
            sdk_dir = installed_info[0]["path"]
            self.inf(
                f"Zephyr SDK version {version} is already installed at {sdk_dir}. Using it."
            )

        self.run_setup(args, sdk_dir)

    def fetch_sdk_info(self):
        sorted_sdks = []
        try:
            cmds = [
                "-P",
                os.path.join(os.path.dirname(__file__), "sdk", "listsdk.cmake"),
            ]

            output = zcmake.run_cmake(cmds, capture_output=True)
            if output:
                # remove '-- ' leader
                sorted_sdks = [l[3:] for l in output]
            else:
                sorted_sdks = []

        except Exception as e:
            self.die(e)

        sdk_info = {}
        for sdk in reversed(sorted_sdks):
            entry = {}

            ver = None
            if os.path.exists(os.path.join(sdk, "sdk_version")):
                with open(os.path.join(sdk, "sdk_version")) as f:
                    ver = f.readline().strip()
            else:
                continue

            entry["path"] = sdk

            if os.path.exists(os.path.join(sdk, "sysroots")):
                entry["hosttools"] = "installed"

            # Identify toolchain directory by the existence of <toolchain>/bin/<toolchain>-gcc
            if "Windows" == platform.system():
                gcc_postfix = "-gcc.exe"
            else:
                gcc_postfix = "-gcc"

            toolchains = [
                tc
                for tc in os.listdir(sdk)
                if os.path.exists(os.path.join(sdk, tc, "bin", tc + gcc_postfix))
            ]

            tcarray = []
            if len(toolchains) > 0:
                for tc in toolchains:
                    tcarray.append(tc.strip())
                entry["toolchains"] = tcarray

            if ver:
                sdk_info[ver] = entry

        return sdk_info

    def list_sdk(self):
        sdk_info = self.fetch_sdk_info()

        if len(sdk_info) == 0:
            self.die("No Zephyr SDK installed.")

        for k, v in sdk_info.items():
            self.inf(k + ":")
            self.inf("  path: " + v["path"])
            if "hosttools" in v:
                self.inf("  hosttools: " + v["hosttools"])
            if "toolchains" in v:
                self.inf("  toolchains:")
                for tc in v["toolchains"]:
                    self.inf("    - " + tc)

            self.inf()

    def do_run(self, args, user_args):
        self.dbg("args: ", args)
        if args.subcommand == "install":
            self.install_sdk(args, user_args)
        elif args.subcommand == "list":
            self.list_sdk()
