# Copyright (c) 2018,2020 Intel Corporation
# Copyright (c) 2022 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

import re

import _compliance_context as _ctx


class CMakeStyle(_ctx.ComplianceTest):
    """
    Checks cmake style added/modified files
    """

    name = "CMakeStyle"
    doc = _ctx.zephyr_doc_detail_builder("/contribute/style/cmake.html")

    def run(self):
        # Loop through added/modified files
        for fname in _ctx.get_files(filter="d"):
            if fname.endswith(".cmake") or fname.endswith("CMakeLists.txt"):
                self.check_style(fname)

    def check_style(self, fname):
        SPACE_BEFORE_OPEN_BRACKETS_CHECK = re.compile(r"^\s*if\s+\(")
        TAB_INDENTATION_CHECK = re.compile(r"^\t+")

        with open(fname, encoding="utf-8") as f:
            for line_num, line in enumerate(f.readlines(), start=1):
                if TAB_INDENTATION_CHECK.match(line):
                    self.fmtd_failure(
                        "error",
                        "CMakeStyle",
                        fname,
                        line_num,
                        "Use spaces instead of tabs for indentation",
                    )

                if SPACE_BEFORE_OPEN_BRACKETS_CHECK.match(line):
                    self.fmtd_failure(
                        "error",
                        "CMakeStyle",
                        fname,
                        line_num,
                        "Remove space before '(' in if() statements",
                    )
