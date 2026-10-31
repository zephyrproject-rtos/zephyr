# SPDX-FileCopyrightText: Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

"""Check that relocation metadata does not become File API source entries."""

import json
import os
import subprocess
from pathlib import Path

import pytest

ZEPHYR_BASE = Path(__file__).resolve().parents[2]


@pytest.mark.parametrize("file_api", [False, True])
def test_relocation_metadata(tmp_path, file_api):
    source = tmp_path / "source"
    build = tmp_path / "build"
    source.mkdir()
    for name in ("first.c", "second.c"):
        (source / name).write_text("int unused;\n", encoding="utf-8")

    (source / "CMakeLists.txt").write_text(
        """
cmake_minimum_required(VERSION 3.28)
project(relocation_metadata NONE)
list(APPEND CMAKE_MODULE_PATH "${ZEPHYR_BASE}/cmake/modules")
include(extensions)
add_custom_target(code_data_relocation_target)
add_custom_target(compiler)
zephyr_code_relocate(FILES first.c second.c LOCATION ITCM_TEXT)
zephyr_code_relocate(FILES "${CMAKE_CURRENT_SOURCE_DIR}/first.c"
  LOCATION RAM_DATA NOCOPY NOKEEP FILTER ".*data")
zephyr_code_relocate(
  FILES "$<IF:$<BOOL:1>,${CMAKE_CURRENT_SOURCE_DIR}/second.c,unused.c>"
  LOCATION SRAM2_TEXT)
add_library(relocated_library INTERFACE first.c "${CMAKE_CURRENT_SOURCE_DIR}/second.c")
zephyr_code_relocate(LIBRARY relocated_library LOCATION RAM_TEXT)
set(directives "$<TARGET_PROPERTY:code_data_relocation_target,RELOCATION_DIRECTIVES>")
file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/directives.txt"
  CONTENT "$<TARGET_GENEX_EVAL:code_data_relocation_target,${directives}>")
""",
        encoding="utf-8",
    )
    if file_api:
        query = build / ".cmake/api/v1/query"
        query.mkdir(parents=True)
        (query / "codemodel-v2").touch()

    result = subprocess.run(
        [
            "cmake",
            "-G",
            "Ninja",
            "-S",
            str(source),
            "-B",
            str(build),
            f"-DZEPHYR_BASE={ZEPHYR_BASE.as_posix()}",
        ],
        capture_output=True,
        text=True,
        env={**os.environ, "CMAKE_COLOR_DIAGNOSTICS": "OFF"},
        timeout=120,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    # File API generation errors can be emitted even when CMake returns zero.
    assert "CMake Error" not in result.stderr, result.stderr
    prefix = source.as_posix()
    assert (build / "directives.txt").read_text(encoding="utf-8") == (
        f"\nITCM_TEXT:COPY:{prefix}/first.c;{prefix}/second.c,"
        f"\nRAM_DATA:NOCOPY;NOKEEP:{prefix}/first.c,.*data"
        f"\nSRAM2_TEXT:COPY:{prefix}/second.c,"
        f"\nRAM_TEXT:COPY:{prefix}/second.c;{prefix}/first.c,"
    )
    if file_api:
        replies = list(
            (build / ".cmake/api/v1/reply").glob("target-code_data_relocation_target-*.json")
        )
        assert len(replies) == 1
        target = json.loads(replies[0].read_text(encoding="utf-8"))
        assert not target.get("sources"), target
