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
        expected = [path.resolve() for root in scan_roots for path in (root, root / "nested")]
        assert actual == expected


def run_discovery(roots, output, links=None, excluded=()):
    command = [
        sys.executable,
        str(Path(__file__).with_name("subfolder_list.py")),
        "--out-file",
        str(output),
    ]
    for root in roots:
        command.extend(["--directory", str(root)])
    if links is not None:
        command.extend(["--create-links", str(links)])
    for directory in excluded:
        command.extend(["--exclude-directory", str(directory)])
    subprocess.run(command, check=True)
    return {Path(line).resolve() for line in output.read_text().splitlines() if line}


@pytest.mark.parametrize("create_links", [False, True], ids=["paths", "links"])
def test_missing_directory(tmp_path, create_links):
    present = tmp_path / "present"
    present.mkdir()
    missing = tmp_path / "missing"
    output = tmp_path / "directories.txt"
    links = tmp_path / "links" if create_links else None
    for _ in range(2):
        assert run_discovery([missing, present], output, links) == {present.resolve()}
    missing.mkdir()
    assert run_discovery([missing, present], output, links) == {
        missing.resolve(),
        present.resolve(),
    }


@pytest.mark.parametrize("create_links", [False, True], ids=["paths", "links"])
def test_excluded_directories(tmp_path, create_links):
    root = tmp_path / "app"
    api = root / "api"
    generated = root / "build" / "generated"
    generated_include = generated / "include"
    other = root / "build-other"
    for directory in [api, generated_include, root / ".git" / "refs", other / "api"]:
        directory.mkdir(parents=True)
    output = tmp_path / "directories.txt"
    links = tmp_path / "links" if create_links else None
    expected = {path.resolve() for path in [root, api, other, other / "api"]}
    for _ in range(2):
        assert run_discovery([root], output, links, [root / "build"]) == expected
    assert run_discovery([root, generated], output, links, [root / "build"]) == expected | {
        generated.resolve(),
        generated_include.resolve(),
    }


def test_links_inside_scan_root(tmp_path):
    root = tmp_path / "app"
    (root / "source").mkdir(parents=True)
    build = root / "build"
    build.mkdir()
    output = build / "directories.txt"
    expected = {path.resolve() for path in [root, root / "source", build]}
    for _ in range(2):
        assert run_discovery([root], output, build / "links") == expected
