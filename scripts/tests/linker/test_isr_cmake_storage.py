# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Keep software ISR registration independent of its ROM/RAM placement."""

import re
import subprocess
from pathlib import Path

import pytest
import test_iar_isr as iar_tests

cmake = iar_tests.cmake
ROOT = Path(__file__).resolve().parents[3]


def collect(cmake, directory, writable, switch, shared, local):
    source, build = directory / "source", directory / "build"
    source.mkdir(parents=True)
    (source / "CMakeLists.txt").write_text(f'''
cmake_minimum_required(VERSION 3.28)
project(isr_storage_fixture NONE)
set(ZEPHYR_BASE "{ROOT}")
list(APPEND CMAKE_MODULE_PATH "{ROOT}/cmake/modules")
include(extensions)
add_custom_target(linker)
zephyr_linker_memory(NAME FLASH START 0x1000 SIZE 0x40000 FLAGS rx)
zephyr_linker_memory(NAME RAM START 0x20000000 SIZE 0x40000 FLAGS rw)
zephyr_linker_group(NAME ROM_REGION LMA FLASH)
zephyr_linker_group(NAME RAM_REGION VMA RAM LMA FLASH)
zephyr_linker_group(NAME RODATA_REGION GROUP ROM_REGION)
zephyr_linker_group(NAME DATA_REGION GROUP RAM_REGION)
# Unrelated iterable/init registrations are outside this section-policy test.
function(zephyr_iterable_section)
endfunction()
function(zephyr_linker_section_obj_level)
endfunction()
set(CONFIG_GEN_SW_ISR_TABLE TRUE)
set(CONFIG_SRAM_SW_ISR_TABLE {"TRUE" if writable else "FALSE"})
set(CONFIG_GEN_SW_ISR_TABLE_ARRAY {"FALSE" if switch else "TRUE"})
set(CONFIG_GEN_SW_ISR_TABLE_SWITCH {"TRUE" if switch else "FALSE"})
set(CONFIG_SHARED_INTERRUPTS {"TRUE" if shared else "FALSE"})
set(CONFIG_ISR_TABLES_LOCAL_DECLARATION {"TRUE" if local else "FALSE"})
set(CONFIG_ARCH_SW_ISR_TABLE_ALIGN 8)
set(XIP_ALIGN_WITH_INPUT ALIGN_WITH_INPUT)
include("{ROOT}/cmake/linker_script/common/common-rom.cmake")
include("{ROOT}/cmake/linker_script/common/common-ram.cmake")
zephyr_linker_generate_linker_settings_file("${{CMAKE_BINARY_DIR}}/settings.cmake")
''')
    subprocess.run(
        [cmake, "-S", str(source), "-B", str(build)], check=True, capture_output=True, text=True
    )
    return build, (build / "settings.cmake").read_text().replace(r"\;", ";")


def render(cmake, build, backend, phase):
    output = build / f"{backend}-{phase or 'pre0'}"
    generator = {
        "ld": "ld/ld_script.cmake",
        "iar": "iar/config_file_script.cmake",
        "armlink": "armlink/scatter_script.cmake",
    }[backend]
    subprocess.run(
        [
            cmake,
            "-C",
            str(build / "settings.cmake"),
            f"-DPASS={phase}",
            f"-DOUT_FILE={output}.script",
            f"-DSTEERING_FILE={output}.steer",
            f"-DSTEERING_C={output}.c",
            f"-DKEEP_FILE={output}.keep",
            "-P",
            str(ROOT / "cmake/linker" / generator),
        ],
        check=True,
        capture_output=True,
        text=True,
    )
    return Path(f"{output}.script").read_text()


@pytest.mark.parametrize("writable", [False, True])
@pytest.mark.parametrize("switch", [False, True])
@pytest.mark.parametrize("shared", [False, True])
@pytest.mark.parametrize("local", [False, True])
def test_software_table_storage_contract(cmake, tmp_path, writable, switch, shared, local):
    build, settings = collect(cmake, tmp_path, writable, switch, shared, local)
    name = "isr_entries" if local and switch else "sw_isr_table"
    declarations = [
        record
        for record in re.findall(r"\{([^{}]+)\}", settings)
        if f"NAME;{name};" in record + ";"
    ]
    assert len(declarations) == 1
    declaration = declarations[0]
    assert ("GROUP;DATA_REGION" in declaration) is writable
    assert ("KVMA;FLASH" in declaration) is not writable
    assert ("ALIGN_WITH_INPUT;TRUE" in declaration) is writable
    assert ("SUBALIGN;8" in declaration) is (writable and not local)
    assert ("NOINPUT;TRUE" in declaration) is (local or not writable)
    assert ("NOSYMBOLS;TRUE" in declaration) is (local and switch)
    assert (".gnu.linkonce.shared_sw_isr_table*" in settings) is (shared and not local)
    assert ("NAME;shared_sw_isr_table;" in settings) is (shared and local)
    assert (".isr_pre0.sw" in settings) is (local and not switch)
    for phase in ("", "LINKER_ZEPHYR_PREBUILT", "LINKER_ZEPHYR_FINAL"):
        script = render(cmake, build, "ld", phase)
        assert (".isr_pre0.sw" in script) is (local and not switch and not phase)
        assert (".isr_pre0.shared" in script) is (local and shared and not phase)
