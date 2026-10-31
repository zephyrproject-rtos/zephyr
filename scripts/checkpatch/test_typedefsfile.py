# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Regression tests for loading the bundled checkpatch typedefs file."""

import subprocess
from pathlib import Path

import pytest

ZEPHYR_BASE = Path(__file__).resolve().parents[2]
CHECKPATCH = ZEPHYR_BASE / "scripts" / "checkpatch.pl"
# FILE is only known to checkpatch through scripts/checkpatch/typedefsfile.
# Without it, the '*' below is reported as "need consistent spacing".
PATCH = (
    "diff --git a/foo.h b/foo.h\n"
    "--- a/foo.h\n"
    "+++ b/foo.h\n"
    "@@ -1,1 +1,2 @@\n"
    " int a;\n"
    "+void f(FILE *const fp, int ch);\n"
)


@pytest.mark.parametrize("in_tree", [True, False], ids=["zephyr_base", "outside_tree"])
def test_bundled_typedefs_are_loaded(tmp_path, monkeypatch, in_tree):
    # Keep a user's ~/.checkpatch.conf out of the run.
    monkeypatch.setenv("HOME", str(tmp_path))
    result = subprocess.run(
        [CHECKPATCH, "--no-tree", "--show-types", "-"],
        input=PATCH,
        text=True,
        capture_output=True,
        check=False,
        cwd=ZEPHYR_BASE if in_tree else tmp_path,
    )
    assert "lines checked" in result.stdout, result.stderr
    assert "No additional types" not in result.stderr
    assert "SPACING" not in result.stdout
