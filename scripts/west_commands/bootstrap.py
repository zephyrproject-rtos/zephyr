# Copyright (c) 2024 Google LLC
# SPDX-License-Identifier: Apache-2.0

import sys
import os
import subprocess
from zephyr_ext_common import ZEPHYR_BASE
from pathlib import Path
import requests
import tempfile
import tarfile
import zipfile
import gzip
import magic
import lzma

sys.path.append(os.fspath(Path(__file__).parent.parent))

from west import log
from west.commands import WestCommand
from run_common import add_parser_common
from typing import NamedTuple, List, Union, Set, Dict
from collections import namedtuple

import zephyr_module
import venv
import pathlib
import glob
import platform
import shutil
import stat
import re


class color:
    PURPLE = "\033[95m"
    CYAN = "\033[96m"
    DARKCYAN = "\033[36m"
    BLUE = "\033[94m"
    GREEN = "\033[92m"
    YELLOW = "\033[93m"
    RED = "\033[91m"
    BOLD = "\033[1m"
    UNDERLINE = "\033[4m"
    END = "\033[0m"


_ARCH_MAPPING = {
    "x86_64": "amd64",
    "aarch64": "arm64",
}


def _get_global_defitions() -> Dict[str, str]:
    defs = {}
    current_os = platform.system()
    if current_os == "Windows":
        defs["os"] = "windows"
    elif current_os == "Darwin":
        defs["os"] = "mac"
    elif current_os == "Linux":
        defs["os"] = "linux"
    else:
        raise RuntimeError("Unknown OS: " + current_os)

    arch = platform.machine()
    defs["arch"] = _ARCH_MAPPING.get(arch, arch)
    defs["platform"] = f"{defs['os']}-{defs['arch']}"
    return defs


_GLOBAL_DEFINITIONS: Dict[str, str] = _get_global_defitions()


def _is_virtual_env(path: pathlib.Path) -> bool:
    """
    Checks if the given path points to a virtual environment.

    Args:
        path: A pathlib.Path object representing the path to check.

    Returns:
        True if the path is a virtual environment, False otherwise.
    """

    if not path.exists() or not path.is_dir():
        return False

    bin_dir = path / "bin" if os.name == "posix" else path / "Scripts"
    lib_dir = path / "lib"

    # Check for essential directories and potentially an activation script
    return (
        bin_dir.exists()
        and lib_dir.exists()
        and any(
            [
                (bin_dir / "activate").exists(),
                (bin_dir / "activate.bat").exists(),
                (bin_dir / "activate.ps1").exists(),
                (bin_dir / "activate.fish").exists(),
            ]
        )
    )


def _rm_dir(path_to_delete: pathlib.Path) -> None:
    """Delete a directory recursively.

    On Windows if a file can't be deleted, mark it as writable then delete.
    """

    def make_writable_and_delete(_func, path, _exc_info):
        os.chmod(path, stat.S_IWRITE)
        os.unlink(path)

    on_rm_error = None
    if platform.system() == "Windows":
        on_rm_error = make_writable_and_delete
    shutil.rmtree(path_to_delete, onerror=on_rm_error)


def _create_venv(destination_dir: pathlib.Path) -> None:
    if destination_dir.exists():
        _rm_dir(destination_dir)
    venv.create(destination_dir, symlinks=True, with_pip=True)


class Requirements(NamedTuple):
    requirements: List[pathlib.Path]
    constraints: List[pathlib.Path]
    module_dirs: List[pathlib.Path]
    modules: List[str]

    def __add__(self, other):
        if not isinstance(other, Requirements):
            return NotImplemented

        merged_requirements = list(set(self.requirements + other.requirements))
        merged_constraints = list(set(self.constraints + other.constraints))
        merged_module_dirs = list(set(self.module_dirs + other.module_dirs))
        merged_modules = list(set(self.modules + other.modules))

        return Requirements(
            requirements=merged_requirements,
            constraints=merged_constraints,
            module_dirs=merged_module_dirs,
            modules=merged_modules,
        )


def _check_packages_installed(bin_requirements: List[str]) -> None:
    """
    Checks if the given binary packages are installed. Uses apt on Linux
    and brew on macOS. Other systems automatically pass.

    Args:
        bin_requirements: A list of strings representing the package names.

    Returns:
        True if all packages are installed (or on an unsupported system), False otherwise.
    """

    system = platform.system()

    result = True

    if system == "Linux":
        command = ["apt-get", "install", "--dry-run"] + bin_requirements

        def verifier(process: subprocess.CompletedProcess) -> bool | None:
            for line in process.stdout.splitlines():
                matches = re.match(
                    pattern=r"(\d+) upgraded, (\d+) newly installed, .*",
                    string=line.strip(),
                )
                if not matches:
                    continue
                return matches.group(1) == "0" and matches.group(2) == "0"
            return None

    elif system == "Darwin":
        command = ["brew", "list"]

        def verifier(process: subprocess.CompletedProcess) -> bool | None:
            installed_packages: Set[str] = set(process.stdout.splitlines())
            return all(package in installed_packages for package in bin_requirements)

    else:
        command = []

    if command:
        process = subprocess.run(
            command,
            capture_output=True,
            text=True,
            check=False,
        )
        status = verifier(process=process)
    else:
        status = None
    if status is None:
        log.inf("  * Failed to verify binary dependencies")
        return
    elif status is True:
        log.inf("  * OK")
        return

    log.wrn("Binary dependencies are missing.")
    log.wrn("Install them with the following command:")
    if system == "Linux":
        log.wrn("  $ sudo apt-get install " + " ".join(bin_requirements))
    elif system == "Darwin":
        log.wrn("  $ brew install " + " ".join(bin_requirements))


class UrlEntry(NamedTuple):
    url: str
    paths: List[str]


def _print_banner():
    print(
        """
 /$$$$$$$$                     /$$                          
|_____ $$                     | $$                          
     /$$/   /$$$$$$   /$$$$$$ | $$$$$$$  /$$   /$$  /$$$$$$ 
    /$$/   /$$__  $$ /$$__  $$| $$__  $$| $$  | $$ /$$__  $$
   /$$/   | $$$$$$$$| $$  \ $$| $$  \ $$| $$  | $$| $$  \__/
  /$$/    | $$_____/| $$  | $$| $$  | $$| $$  | $$| $$      
 /$$$$$$$$|  $$$$$$$| $$$$$$$/| $$  | $$|  $$$$$$$| $$      
|________/ \_______/| $$____/ |__/  |__/ \____  $$|__/      
                    | $$                 /$$  | $$          
                    | $$                |  $$$$$$/          
                    |__/                 \______/           
"""
    )


class Bootstrap(WestCommand):
    """
    Bootstrap your west environment
    """

    def __init__(self):
        super().__init__(
            name="bootstrap",
            help="Manage Zephyr dependencies and environment",
            description="",
        )

    def do_add_parser(self, parser_adder):
        return add_parser_common(self, parser_adder)

    def _get_py_requirements(
        self, project_dir: pathlib.Path, requirements: list[dict]
    ) -> Requirements:
        module_dirs: List[pathlib.Path] = []
        requirement_files: List[pathlib.Path] = []
        constraint_files: List[pathlib.Path] = []
        module_names: List[str] = []
        for requirement in requirements:
            if requirement["type"] == "directory":
                search_pattern = str(project_dir / requirement["pattern"])
                py_files = glob.glob(search_pattern, recursive=True)
                for p in py_files:
                    dir_path = pathlib.Path(p)
                    if not (
                        (dir_path / "setup.py").is_file()
                        or (dir_path / "pyproject.toml").is_file()
                    ):
                        continue
                    module_dirs.append(dir_path)
            elif requirement["type"] == "module":
                module_names.append(requirement["pattern"])
            elif requirement["type"] == "constraint":
                constraint_files.append(project_dir / requirement["pattern"])
            elif requirement["type"] == "requirements":
                requirement_files.append(project_dir / requirement["pattern"])
            else:
                raise RuntimeError(f'Invalid requirement type: {requirement["type"]}')

        assert all(p.is_dir() for p in module_dirs)
        assert all(p.is_file() for p in requirement_files)
        assert all(p.is_file() for p in constraint_files)
        assert all(
            re.match(pattern=r"^\w+$", string=str(p)) is not None for p in module_names
        )
        return Requirements(
            requirements=requirement_files,
            constraints=constraint_files,
            module_dirs=module_dirs,
            modules=module_names,
        )

    def do_run(self, args, remainder) -> None:
        """
        Run the command
        """

        _print_banner()

        log.inf("- Checking virtual environment...")
        # Check if the manifest contains venv management
        if self.manifest.venv is None:
            log.inf("  Virtual environment not found in manifest")
            return

        # We have to know where the topdir is
        assert self.topdir is not None

        # Set the venv path
        self.venv_path = pathlib.Path(self.topdir) / self.manifest.venv.name

        # If the venv is not created yet, create it
        if not _is_virtual_env(path=self.venv_path):
            log.inf("  * Creating virtual environment:", self.venv_path)
            _create_venv(destination_dir=self.venv_path)
        else:
            log.inf("  * Virtual environment already exists:", self.venv_path)

        # Get the python requirements for the manifest project
        py_requirements = self._get_py_requirements(
            project_dir=pathlib.Path(self.manifest.projects[0].abspath),
            requirements=[
                {
                    "pattern": r.pattern,
                    "type": r.type,
                }
                for r in self.manifest.venv.py_requirements
            ],
        )

        # Get the binary requirements
        definitions: Dict[str, str] = self.manifest.venv.definitions

        self.env = os.environ.copy()

        # Iterate over the modules and get all their requirements
        for module in zephyr_module.parse_modules(ZEPHYR_BASE, self.manifest):
            module_path = pathlib.Path(self.topdir) / module.project
            self.env[f'ZEPHYR_{module.meta["name"].upper()}_MODULE_DIR'] = str(
                module_path
            )
        #     if "venv" not in module.meta:
        #         continue
        #     module_venv = module.meta["venv"]
        #     if "py-requirements" in module_venv:
        #         py_requirements += self._get_py_requirements(
        #             project_dir=pathlib.Path(module.project),
        #             requirements=module_venv["py-requirements"],
        #         )
        #     if "bin-requirements" in module_venv:
        #         if bin_requirement_key in module_venv["bin-requirements"]:
        #             bin_requirements += module_venv["bin-requirements"][
        #                 bin_requirement_key
        #             ]

        log.inf("- Installing Python dependencies...")
        self._install_python_deps(
            deps=py_requirements.module_dirs + py_requirements.modules,
            constraints=py_requirements.constraints,
            is_requirements=False,
        )
        self._install_python_deps(
            deps=py_requirements.requirements,
            constraints=py_requirements.constraints,
            is_requirements=True,
        )

        self.extra_paths: List[pathlib.Path] = []
        if self.manifest.venv.bin_requirements:
            log.inf("- Checking binary dependencies...")
            self._check_bin_requirements(global_defs=definitions)

        self.modify_activate_script(hermetic=False)

        print("")
        print(
            color.BOLD
            + "\U0001FA81\U0001FA81 Bootstrapping complete! \U0001FA81\U0001FA81"
            + color.END
        )
        print("\nEnter the virtual environment by running:\n")
        if os.name == "posix":
            print(f"  $ source {str(self.venv_path / 'bin' / 'activate')}")
        else:
            print(f"  $ {str(self.venv_path / 'Scripts' / 'activate.bat')}")

    @staticmethod
    def _replace_variables(input_str: str, defs: Dict[str, str]) -> str:
        while True:
            placeholders = re.findall(r"\$\{(.*?)\}", input_str)
            if not placeholders:
                return input_str

            missing_keys = [key for key in placeholders if key not in defs]
            if missing_keys:
                raise KeyError(
                    f"Missing keys in '{input_str}': {', '.join(missing_keys)}"
                )

            key = placeholders[0]
            input_str = re.sub(r"\$\{" + key + r"\}", defs[key], input_str)

    def _install_bin_requirement(
        self,
        name: str,
        install_path: pathlib.Path,
        url: str,
        local_defs: Dict[str, str],
    ) -> None:
        url = Bootstrap._replace_variables(input_str=url, defs=local_defs)
        message_format = "  * [bin] %s... %s"
        bootstrap_version_file = install_path / ".west_bootstrap_version"

        # Check if .west_bootstrap_version exists and matches the URL
        if bootstrap_version_file.exists():
            existing_url = bootstrap_version_file.read_text().strip()
            if existing_url == url:
                log.inf(message_format % (name, "OK"))
                return

        # Create a temporary directory
        log.inf(message_format % (name, "installing"))
        with tempfile.TemporaryDirectory() as temp_dir:
            temp_dir_path = pathlib.Path(temp_dir)
            # Download the file to the temporary directory
            response = requests.get(url, stream=True, timeout=600)
            response.raise_for_status()  # Raise an exception for bad status codes

            # Get the filename from the Content-Disposition header, if available
            filename: None | str = None
            if "Content-Disposition" in response.headers:
                disposition = response.headers["Content-Disposition"]
                filenames = re.findall('filename="?([^"]+)"?', disposition)
                if filenames:
                    filename = filenames[0]

            # If filename is still None, use the last part of the URL as a fallback
            if filename is None:
                filename = url.split("/")[-1]

            temp_file_path = temp_dir_path / filename
            with open(temp_file_path, "wb") as f:
                for chunk in response.iter_content(chunk_size=8192):
                    f.write(chunk)

            # Extract the file based on its type
            if temp_file_path.suffix == ".gz":
                if temp_file_path.name[:-3].endswith(
                    ".tar"
                ):  # Check for .tar.gz or .tar.xz
                    with tarfile.open(temp_file_path, "r:gz") as tar:
                        tar.extractall(install_path)
                else:
                    with gzip.open(temp_file_path, "rb") as f_in:
                        with open(temp_file_path.with_suffix(""), "wb") as f_out:
                            f_out.write(f_in.read())
            elif temp_file_path.suffix == ".tar":
                with tarfile.open(temp_file_path, "r:") as tar:
                    tar.extractall(install_path)
            elif temp_file_path.suffix == ".zip":
                with zipfile.ZipFile(temp_file_path, "r") as zip_ref:
                    zip_ref.extractall(install_path)
            elif temp_file_path.suffix == ".xz":
                if temp_file_path.name[:-3].endswith(".tar"):  # Check for .tar.xz
                    with lzma.open(temp_file_path, "rb") as f_xz:
                        with tarfile.open(fileobj=f_xz, mode="r|") as tar:
                            tar.extractall(install_path)
                else:
                    raise ValueError(
                        "Unsupported .xz file format (only .tar.xz is supported)"
                    )
            else:
                raise ValueError("Unsupported file format: " + str(temp_file_path))

        # Create .west_bootstrap_version with the URL
        bootstrap_version_file.write_text(url)
        log.inf("          DONE")

    def _check_bin_requirements(self, global_defs: Dict[str, str]) -> None:
        for name, value in self.manifest.venv.bin_requirements.items():
            local_defs = global_defs.copy()
            local_defs.update(value.get("definitions", {}))
            local_defs.update(_GLOBAL_DEFINITIONS)
            install_path = self.venv_path / "bin_requirements" / name
            install_path.mkdir(parents=True, exist_ok=True)
            url_map: Dict[str, Dict][str, Union[str, List[str]]] = value["urls"]

            # first look for exact match to the current platform
            if _GLOBAL_DEFINITIONS["platform"] in url_map:
                url_key = _GLOBAL_DEFINITIONS["platform"]
            else:
                matching_url_keys: List[str] = []
                for key in url_map.keys():
                    pattern = Bootstrap._replace_variables(
                        input_str=key, defs=local_defs
                    ).replace("*", ".*")

                    match = re.match(
                        pattern=pattern, string=_GLOBAL_DEFINITIONS["platform"]
                    )
                    if match:
                        matching_url_keys.append(key)

                if len(matching_url_keys) == 0:
                    raise RuntimeError("Failed to find platform URL")
                if len(matching_url_keys) > 1:
                    raise RuntimeError(
                        "Multiple url keys matched: " + str(matching_url_keys)
                    )
                url_key = matching_url_keys[0]

            url_entry: UrlEntry = UrlEntry(
                url=url_map[url_key]["url"],
                paths=url_map[url_key].get("paths", ["."]),
            )
            self.extra_paths += [
                install_path
                / Bootstrap._replace_variables(input_str=path, defs=local_defs)
                for path in url_entry.paths
            ]
            self._install_bin_requirement(
                name=name,
                install_path=install_path,
                url=url_entry.url,
                local_defs=local_defs,
            )

    def _install_python_deps(
        self,
        deps: List[Union[pathlib.Path, str]],
        constraints: List[pathlib.Path],
        is_requirements: bool,
    ) -> None:
        if not deps:
            return
        python_executable = (
            self.venv_path / "bin" / "python3"
            if os.name == "posix"
            else self.venv_path / "Scripts" / "python.exe"
        )

        pattern_installing = r"Collecting (\S+).*"
        pattern_satisfied = r"Requirement already satisfied: (\S+).*"
        command = [str(python_executable), "-m", "pip", "install"]
        message_format = "  * [Py] %s... %s"
        deps_array = [str(v) for v in deps]
        if is_requirements:
            command.append("-r")
        command += deps_array
        for c in constraints:
            command += ["-c" + str(c)]
        process = subprocess.Popen(
            command,
            env=self.env,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )

        # Iterate over stdout lines as they are produced
        assert process.stdout is not None
        for line in process.stdout:
            # Search for matches in each line
            line = line.strip()
            installing_match = re.search(pattern_installing, line)
            if installing_match:
                print(message_format % (installing_match.group(1), "installing"))
                continue
            satisfied_match = re.search(pattern_satisfied, line)
            if satisfied_match:
                print(message_format % (satisfied_match.group(1), "OK"))

        process.wait()

    def modify_activate_script(self, hermetic: bool):
        """
        Modifies the activate script to add/prepend extra paths to PATH,
        replacing existing PATH setting if found. Also moves 'hash -r'
        to the end if it exists.

        Args:
            hermetic: If True, override PATH with extra_paths; otherwise, prepend them.
        """

        current_os = platform.system()
        if current_os == "Windows":
            activate_script_path = self.venv_path / "Scripts" / "activate.bat"
            path_separator = ";"
            path_variable = "%PATH%"
            set_command = "set"
            path_pattern = rf"{set_command} PATH=(.*)"
        else:
            activate_script_path = self.venv_path / "bin" / "activate"
            path_separator = ":"
            path_variable = "$PATH"
            set_command = "export"
            path_pattern = rf"{set_command} PATH=(.*)"

        with open(activate_script_path, "r", encoding="utf-8") as f:
            lines = f.readlines()

        path_line_found = False
        clear_hash_command_found = False
        hash_r_line_index = None  # To store the index of 'hash -r' if found

        extra_paths = [str(path) for path in self.extra_paths]

        for i, line in enumerate(lines):
            if re.match(path_pattern, line):
                path_line_found = True
                if hermetic:
                    new_path = path_separator.join(extra_paths)
                else:
                    new_path = path_separator.join(extra_paths + [path_variable])
                lines[i] = f"{set_command} PATH={new_path}\n"
            elif re.match(r"hash -r", line):
                clear_hash_command_found = True
                hash_r_line_index = i

            if clear_hash_command_found and path_line_found:
                break

        if not path_line_found:
            if hermetic:
                new_path = path_separator.join(extra_paths)
            else:
                new_path = path_separator.join(extra_paths + [path_variable])
            lines.append(f"\n{set_command} PATH={new_path}\n")

        if clear_hash_command_found:
            if hash_r_line_index is not None:
                # Remove the 'hash -r' line from its original position
                del lines[hash_r_line_index]
            # Add 'hash -r' to the end
            lines.append("\nhash -r 2> /dev/null\n")

        with open(activate_script_path, "w", encoding="utf-8") as f:
            f.writelines(lines)

        self.add_execute_permissions()

    def add_execute_permissions(self):
        """Adds execute permissions to all files in the directories specified in self.extra_paths."""

        for path in self.extra_paths:
            if not path.is_dir():
                continue  # Skip if it's not a directory

            for file in path.iterdir():
                if not file.is_file():
                    continue

                if magic.from_file(file, mime=True).startswith("text/"):
                    continue

                # Make the file executable for the owner
                file.chmod(file.stat().st_mode | 0o111)  # Add execute permissions
