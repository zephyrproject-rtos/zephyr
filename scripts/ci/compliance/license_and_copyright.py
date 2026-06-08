# Copyright (c) 2018,2020 Intel Corporation
# Copyright (c) 2022 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

import os
from collections.abc import Iterable
from pathlib import Path

import _compliance_context as _ctx
from reuse.project import Project
from reuse.report import ProjectSubsetReport


class LicenseAndCopyrightCheck(_ctx.ComplianceTest):
    """
    Verify that every file touched by the patch set has correct SPDX headers and uses allowed
    license.
    """

    name = "LicenseAndCopyrightCheck"
    doc = "Check SPDX headers and copyright lines with the reuse Python API."

    def _report_violations(
        self,
        paths: Iterable[Path],
        title: str,
        severity: str,
        desc: str | None = None,
    ) -> None:
        for p in paths:
            rel_path = os.path.relpath(str(p), _ctx.GIT_TOP)
            self.fmtd_failure(severity, title, rel_path, desc=desc or "", line=1)

    def run(self) -> None:
        changed_files = _ctx.get_files(filter="d")
        if not changed_files:
            return

        # Only scan text files for now, in the future we may want to leverage REUSE standard's
        # ability to also associate license/copyright info with binary files.
        filtered_files = []
        for file in changed_files:
            full_path = _ctx.GIT_TOP / file
            mime_type = _ctx.magic.from_file(os.fspath(full_path), mime=True)
            if mime_type.startswith("text/"):
                filtered_files.append(file)
        changed_files = filtered_files

        project = Project.from_directory(_ctx.GIT_TOP)
        report = ProjectSubsetReport.generate(project, changed_files, multiprocessing=False)

        self._report_violations(
            report.files_without_licenses,
            "License missing",
            "warning",
            "File has no SPDX-License-Identifier header, consider adding one.",
        )

        self._report_violations(
            report.files_without_copyright,
            "Copyright missing",
            "warning",
            "File has no SPDX-FileCopyrightText header, consider adding one.",
        )

        for lic_id, paths in getattr(report, "missing_licenses", {}).items():
            self._report_violations(
                paths,
                "License may not be allowed",
                "warning",
                (
                    f"License file for '{lic_id}' not found in /LICENSES. Please check "
                    "https://docs.zephyrproject.org/latest/contribute/guidelines.html#components-using-other-licenses."
                ),
            )
