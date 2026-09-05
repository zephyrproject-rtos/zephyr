# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Pytest configuration for the DT Doctor integration test."""

import json
import pickle
import shlex
import sys
from pathlib import Path

import pytest

ZEPHYR_BASE = Path(__file__).parents[4]
sys.path.insert(0, str(ZEPHYR_BASE / "scripts" / "dts" / "python-devicetree" / "src"))


def pytest_addoption(parser):
    parser.addoption(
        "--build-dir",
        action="store",
        required=True,
        help="Path to the build directory of the test application",
    )
    parser.addoption(
        "--cc",
        action="store",
        required=True,
        help="Path to the C compiler the application was built with",
    )


@pytest.fixture(scope="session")
def build_dir(request):
    return Path(request.config.getoption("--build-dir"))


@pytest.fixture(scope="session")
def cc(request):
    return request.config.getoption("--cc")


@pytest.fixture(scope="session")
def edt_pickle(build_dir):
    return build_dir / "zephyr" / "edt.pickle"


@pytest.fixture(scope="session")
def edt(edt_pickle):
    from devicetree import edtlib  # noqa: F401 (needed to unpickle the EDT)

    with open(edt_pickle, "rb") as f:
        return pickle.load(f)


@pytest.fixture(scope="session")
def compile_cmd(build_dir):
    """The application's real compile command for the source file with the given suffix.

    Returns (argv, cwd, source_path), straight from the build's compile_commands.json.
    """
    with open(build_dir / "compile_commands.json", encoding="utf-8") as f:
        entries = json.load(f)

    def _get(suffix: str):
        entry = next(e for e in entries if e["file"].endswith(suffix))
        return shlex.split(entry["command"]), Path(entry["directory"]), entry["file"]

    return _get


def retarget(argv, source_file, new_source, new_obj):
    """Point a compile command at another source file and output object.

    Dependency-generation flags are dropped so the replay cannot touch the
    application build's own .obj/.d files.
    """
    result = []
    skip_next = False
    for tok in argv:
        if skip_next:
            skip_next = False
            continue
        if tok in ("-MD", "-MMD"):
            continue
        if tok in ("-MT", "-MF", "-MQ"):
            skip_next = True
            continue
        result.append(tok)

    for i, tok in enumerate(result):
        if tok == source_file:
            result[i] = str(new_source)
        elif tok == "-o":
            result[i + 1] = str(new_obj)
    return result
