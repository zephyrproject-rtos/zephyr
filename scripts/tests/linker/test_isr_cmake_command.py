# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Exercise GNU-format script commands through each backend's production macro."""

import hashlib
import re
import shlex
import shutil
import subprocess
from pathlib import Path

import pytest
import test_iar_isr as iar_tests

cmake = iar_tests.cmake
ROOT = Path(__file__).resolve().parents[3]
PASSES = ("", "CUSTOM;LINKER_ZEPHYR_PREBUILT", "LINKER_ZEPHYR_FINAL")


@pytest.mark.parametrize("backend", ["ld", "lld", "xt-ld"])
def test_cmake_script_command_contract(cmake, tmp_path, backend):
    ninja = shutil.which("ninja")
    if ninja is None:
        pytest.skip("Ninja is required for command and dependency checks")
    source = tmp_path / "source"
    source.mkdir()
    manifest = source / "generated layout.cmake"
    manifest.write_text("# Generated linker layout input\n")
    device_api = source / "device-api.ld"
    device_api.write_text("/* Generated device API input */\n")
    original = (ROOT / f"cmake/linker/{backend}/target.cmake").read_text()
    macro = re.search(r"macro\(configure_linker_script .*?endmacro\(\)", original, re.S)
    assert macro is not None
    include = "include(${ZEPHYR_BASE}/cmake/linker/ld/configure.cmake)"
    assert include in original
    (source / "CMakeLists.txt").write_text(
        f'''cmake_minimum_required(VERSION 3.28)
project(cmake_script_commands NONE)
set(ZEPHYR_BASE "{ROOT}")
list(APPEND CMAKE_MODULE_PATH "{ROOT}/cmake/modules")
include(extensions)
add_custom_target(linker)
set_property(TARGET linker PROPERTY FORMAT elf32-littlearm)
set_property(TARGET linker PROPERTY ENTRY _start)
set(CONFIG_CMAKE_LINKER_GENERATOR TRUE)
set(backend_args "unexpected-cache-argument" CACHE STRING "")
set(DEVICE_API_LD_TARGET "{device_api}")
zephyr_linker_memory(NAME RAM START 0x20000000 SIZE 0x10000 FLAGS rw)
zephyr_linker_section(NAME .fixture VMA RAM)
zephyr_linker_include_generated(CMAKE "{manifest}")
zephyr_linker_include_var(VAR CONTRACT_VALUE VALUE 7)
zephyr_linker_symbol(SYMBOL fixture_value EXPR "7")
'''
        + include
        + "\n"
        + macro.group(0)
        + f'''
configure_linker_script(pre0.cmd "" "{manifest}")
configure_linker_script(pre1.cmd "CUSTOM;LINKER_ZEPHYR_PREBUILT" "{manifest}")
configure_linker_script(final.cmd "LINKER_ZEPHYR_FINAL" "{manifest}")
add_custom_target(scripts ALL DEPENDS pre0.cmd pre1.cmd final.cmd)
'''
    )
    build = tmp_path / "build"
    subprocess.run(
        [cmake, "-S", str(source), "-B", str(build), "-G", "Ninja"],
        check=True,
        capture_output=True,
        text=True,
    )
    subprocess.run([cmake, "--build", str(build)], check=True, capture_output=True, text=True)
    commands = subprocess.check_output(
        [ninja, "-C", str(build), "-t", "commands", "scripts"], text=True
    ).splitlines()
    assert len(commands) == len(PASSES)
    for phase, output, command in zip(PASSES, ("pre0", "pre1", "final"), commands, strict=True):
        arguments = shlex.split(command)
        assert "unexpected-cache-argument" not in arguments
        digest = hashlib.sha256(phase.encode()).hexdigest()
        settings = build / f"include/generated/ld_script_settings_{digest}.cmake"
        assert str(settings) in arguments
        assert f"-DPASS={phase}" in arguments
        assert ("-DLINKER_IS_LLD=TRUE" in arguments) is (backend == "lld")
        contents = settings.read_text()
        assert str(manifest) in contents
        assert "CONTRACT_VALUE" in contents
        script = (build / f"{output}.cmd").read_text()
        assert re.search(r"fixture_value\s*=\s*7\s*;", script)
        query = subprocess.check_output(
            [ninja, "-C", str(build), "-t", "query", f"{output}.cmd"], text=True
        )
        for dependency in (
            manifest,
            device_api,
            settings,
            ROOT / "cmake/linker/ld/ld_script.cmake",
            ROOT / "cmake/linker/linker_script_common.cmake",
        ):
            path = dependency.relative_to(build) if dependency.is_relative_to(build) else dependency
            assert str(path) in query

    manifest.unlink()
    result = subprocess.run([cmake, "--build", str(build)], capture_output=True, text=True)
    assert result.returncode != 0
    assert manifest.name in result.stdout + result.stderr
