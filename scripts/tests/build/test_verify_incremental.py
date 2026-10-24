# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

import subprocess
import sys
from pathlib import Path

import pytest


@pytest.mark.parametrize("failure", ["none", "header", "nested"])
def test_incremental_cleanup(tmp_path, failure):
    source = tmp_path / "source"
    source.mkdir()
    build = tmp_path / "build"
    include = tmp_path / "include"
    include.mkdir()
    header = include / "dependency.h"
    original = "int dependency_initial(void);\n"
    header.write_text(original)
    zephyr_include = tmp_path / "zephyr_include"
    zephyr_include.mkdir()
    if sys.platform != "win32":
        alias = build / "zephyr/misc/generated/syscalls_links/include"
        alias.parent.mkdir(parents=True)
        alias.symlink_to(zephyr_include, target_is_directory=True)

    (source / "CMakeLists.txt").write_text(
        "cmake_minimum_required(VERSION 3.20)\n"
        "project(incremental_cleanup NONE)\n"
        "add_custom_target(syscall_list_h_target\n"
        "  COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/generate.cmake)\n"
    )
    trigger = build / "zephyr/misc/generated/syscalls_subdirs.trigger"
    (source / "generate.cmake").write_text(
        f'file(READ "{header.as_posix()}" content)\n'
        'if(content MATCHES "dependency_edited")\n'
        '  set(code "K_SYSCALL_DEPENDENCY_EDITED")\n'
        'else()\n'
        '  set(code "K_SYSCALL_DEPENDENCY_INITIAL")\n'
        'endif()\n'
        f'if("{failure}" STREQUAL "header" AND content MATCHES "dependency_edited")\n'
        '  message(FATAL_ERROR "Controlled failure after header edit")\n'
        'endif()\n'
        f'if(EXISTS "{(include / "nested[1]/added.h").as_posix()}")\n'
        f'  if("{failure}" STREQUAL "nested")\n'
        '    message(FATAL_ERROR "Controlled failure after nested header creation")\n'
        '  endif()\n'
        f'  file(READ "{(include / "nested[1]/added.h").as_posix()}" added_content)\n'
        '  if(added_content MATCHES "dependency_nested_edited")\n'
        '    string(APPEND code " K_SYSCALL_DEPENDENCY_NESTED_EDITED")\n'
        '  else()\n'
        '    string(APPEND code " K_SYSCALL_DEPENDENCY_ADDED")\n'
        '  endif()\n'
        'endif()\n'
        f'file(WRITE "{(build / "zephyr/include/generated/zephyr/syscall_list.h").as_posix()}" '
        '"${code}\\n")\n'
        f'if(NOT EXISTS "{trigger.as_posix()}")\n'
        f'  file(WRITE "{trigger.as_posix()}" "")\n'
        'endif()\n'
    )
    (build / "zephyr/include/generated/zephyr").mkdir(parents=True)
    (build / "zephyr/misc/generated").mkdir(parents=True, exist_ok=True)
    subprocess.run(["cmake", "-S", str(source), "-B", str(build)], check=True)
    script = (
        Path(__file__).parents[3]
        / "tests/application_development/syscall_dependencies/verify_incremental.py"
    )
    result = subprocess.run(
        [sys.executable, str(script), str(build), str(include), str(zephyr_include)],
        capture_output=True,
        text=True,
    )
    if failure == "none":
        assert result.returncode == 0, result.stdout + result.stderr
    else:
        assert result.returncode != 0
        assert "Controlled failure" in result.stdout + result.stderr
    assert header.read_text() == original
    assert not (include / "nested[1]").exists()
