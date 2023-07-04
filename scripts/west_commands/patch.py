# Copyright (c) 2023, ithinx GmbH
# Copyright (c) 2023, tonies GmbH
#
# SPDX-License-Identifier: Apache-2.0

"""This command is used to apply formatted patches to projects in the manifest
file. Any project containing according userdata will be patched, e.g.:
```[name: "zephyr", "userdata": {"patch": {"directories": [patches/zephyr]}}]`
"""

from subprocess import CalledProcessError
from pathlib import Path
from west import log
from west.commands import WestCommand
from west.manifest import MANIFEST_PROJECT_INDEX


class Patch(WestCommand):
    def __init__(self):
        super().__init__(
            "patch",
            "Patch west subprojects",
            __doc__,
        )

    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(
            self.name, help=self.help, description=self.description)
        return parser

    def do_run(self, _args, _unknown):
        manifest_project_path = Path(
            self.manifest.projects[MANIFEST_PROJECT_INDEX].abspath)
        projects = self.manifest.projects
        for project in projects:
            log.banner(f"patching {project.name_and_path}):")
            if (
                project.userdata is None or "patch" not in project.userdata.keys()
                or "directories" not in project.userdata["patch"].keys()
            ):
                log.dbg(f"skipping {project.name}")
                continue

            log.dbg(f"trying to patch {project.name}")

            patches = []
            patch_dirs = project.userdata["patch"]["directories"]
            for patch_dir in patch_dirs:
                patches_path = Path(manifest_project_path / patch_dir)
                if not patches_path.exists():
                    log.wrn(f"patch directory {patches_path} not found")
                    continue
                patch_files = sorted(
                    filter(Path.is_file, patches_path.glob("*.patch")))
                if len(patch_files) == 0:
                    log.wrn(
                        f"no patches in found in directory {patches_path}.")
                    continue
                patches += patch_files

            if not patches:
                log.wrn("no patches found in any patch directory")
                continue

            log.small_banner(f"patching {project.name}")

            for patch in patches:
                if (
                    project.git(
                        f"apply --reverse --check {patch.resolve()}",
                        check=False,
                        capture_stdout=True,
                        capture_stderr=True,
                    ).returncode == 0
                ):
                    log.inf(
                        f"Patch {patch.relative_to(manifest_project_path)} to {project.name} already applied")
                    continue
                try:
                    log.inf(
                        f"Apply {patch.relative_to(manifest_project_path)} to {project.name}")
                    project.git(f"am {patch.resolve()}")
                except CalledProcessError:
                    log.die(
                        f"could not apply patch {patch.relative_to(manifest_project_path)} to {project.name}")
