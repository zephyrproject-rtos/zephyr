# Copyright (c) 2018,2020 Intel Corporation
# Copyright (c) 2022 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

import platform
import subprocess
from pathlib import Path

import _compliance_context as _ctx
import unidiff


class ClangFormatCheck(_ctx.ComplianceTest):
    """
    Check if clang-format reports any issues
    """

    name = "ClangFormat"
    doc = _ctx.zephyr_doc_detail_builder("/contribute/guidelines.html#clang-format")

    def _process_patch_error(self, file: str, patch: unidiff.PatchedFile):
        for hunk in patch:
            # Strip the before and after context
            before = next(i for i, v in enumerate(hunk) if str(v).startswith(('-', '+')))
            after = next(i for i, v in enumerate(reversed(hunk)) if str(v).startswith(('-', '+')))
            msg = "".join([str(line) for line in hunk[before : -after or None]])

            # show the hunk at the last line
            self.fmtd_failure(
                "notice",
                "You may want to run clang-format on this change",
                file,
                line=hunk.source_start + hunk.source_length - after,
                desc=f'\r\n{msg}',
            )

    def run(self):
        exe = f"clang-format-diff.{'exe' if platform.system() == 'Windows' else 'py'}"

        for file in _ctx.get_files():
            if Path(file).suffix not in ['.c', '.h']:
                continue

            diff = subprocess.Popen(
                ('git', 'diff', '-U0', '--no-color', _ctx.COMMIT_RANGE, '--', file),
                stdout=subprocess.PIPE,
                cwd=_ctx.GIT_TOP,
            )
            try:
                subprocess.run(
                    (exe, '-p1'),
                    check=True,
                    stdin=diff.stdout,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.STDOUT,
                    cwd=_ctx.GIT_TOP,
                )

            except subprocess.CalledProcessError as ex:
                patchset = unidiff.PatchSet.from_string(ex.output, encoding="utf-8")
                for patch in patchset:
                    self._process_patch_error(file, patch)
