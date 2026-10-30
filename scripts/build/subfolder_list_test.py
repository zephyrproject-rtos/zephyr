# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

import subprocess
import sys
from pathlib import Path

import pytest


@pytest.mark.parametrize("create_links", [False, True], ids=["paths", "links"])
def test_multiple_directories(tmp_path, create_links):
    roots = [tmp_path / name / "include" for name in ("first", "second")]
    for root in roots:
        (root / "nested").mkdir(parents=True)

    output = tmp_path / "directories.txt"
    command = [
        sys.executable,
        str(Path(__file__).with_name("subfolder_list.py")),
        "--out-file",
        str(output),
    ]
    if create_links:
        command.extend(["--create-links", str(tmp_path / "links")])

    # Reusing the link directory must work when a scan root is removed.
    for scan_roots in (roots, roots, roots[1:]):
        directories = [argument for root in scan_roots for argument in ("--directory", str(root))]
        subprocess.run(command + directories, check=True)
        actual = [Path(line).resolve() for line in output.read_text().splitlines()]
        expected = [path for root in scan_roots for path in (root, root / "nested")]
        assert actual == expected
