# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Collect the production ISR CMake rules before rendering vendor scripts."""

import re
import subprocess
from pathlib import Path

import pytest
import test_iar_isr as iar_tests

cmake = iar_tests.cmake
ROOT = Path(__file__).resolve().parents[3]


@pytest.mark.parametrize("backend", ["iar", "armlink"])
@pytest.mark.parametrize("writable", [False, True])
@pytest.mark.parametrize("phase", ["", "LINKER_ZEPHYR_PREBUILT", "LINKER_ZEPHYR_FINAL"])
@pytest.mark.parametrize("tight", [False, True])
def test_discovery_is_outside_target_memory(cmake, tmp_path, backend, writable, phase, tight):
    source = tmp_path / "source"
    source.mkdir()
    build = tmp_path / "build"
    (source / "CMakeLists.txt").write_text(f'''
cmake_minimum_required(VERSION 3.28)
project(vendor_metadata_fixture NONE)
set(ZEPHYR_BASE "{ROOT}")
list(APPEND CMAKE_MODULE_PATH "{ROOT}/cmake/modules")
include(extensions)
add_custom_target(linker)
zephyr_linker_memory(NAME FLASH START 0x1000 SIZE {"64" if tight else "0x100000"} FLAGS rx)
zephyr_linker_memory(NAME RAM START 0x20000000 SIZE {"64" if tight else "0x100000"} FLAGS wx)
zephyr_linker_group(NAME ROM_REGION LMA FLASH)
zephyr_linker_group(NAME RAM_REGION VMA RAM LMA FLASH)
zephyr_linker_group(NAME RODATA_REGION GROUP ROM_REGION)
zephyr_linker_group(NAME DATA_REGION GROUP RAM_REGION)
# Suppress unrelated source/library construction, retaining the production
# zephyr_linker_section and zephyr_linker_section_configure implementations.
foreach(helper zephyr_library zephyr_library_sources zephyr_library_sources_ifdef
    zephyr_library_include_directories zephyr_library_property zephyr_cc_option
    zephyr_compile_definitions
    zephyr_linker_sources zephyr_linker_sources_ifdef zephyr_library_link_libraries
    add_subdirectory_ifdef add_library add_dependencies target_link_libraries)
  macro(${{helper}})
  endmacro()
endforeach()
set(CONFIG_GEN_ISR_TABLES TRUE)
set(CONFIG_GEN_SW_ISR_TABLE TRUE)
set(CONFIG_GEN_SW_ISR_TABLE_ARRAY TRUE)
set(CONFIG_ISR_TABLES_LOCAL_DECLARATION TRUE)
set(CONFIG_IAR_ZEPHYR_INIT TRUE)
set(CONFIG_SRAM_SW_ISR_TABLE {"TRUE" if writable else "FALSE"})
set(LINKER {backend})
include("{ROOT}/arch/common/CMakeLists.txt")
if({"TRUE" if tight else "FALSE"})
  zephyr_linker_section(NAME target_rom VMA FLASH NOINPUT MIN_SIZE 64)
  zephyr_linker_section_configure(SECTION target_rom KEEP INPUT ".fixture.rom")
  zephyr_linker_section(NAME target_ram VMA RAM NOINPUT TYPE BSS NOINIT MIN_SIZE 64)
  zephyr_linker_section_configure(SECTION target_ram KEEP INPUT ".fixture.bss")
  # Each typed discovery block is much larger than either full target region.
  foreach(section isr_declarations .isr_pre0.entries)
    zephyr_linker_section_configure(SECTION ${{section}} MIN_SIZE 0x20000
      PASS NOT LINKER_ZEPHYR_PREBUILT LINKER_ZEPHYR_FINAL
    )
  endforeach()
endif()
zephyr_linker_generate_linker_settings_file("${{CMAKE_BINARY_DIR}}/settings.cmake")
''')
    subprocess.run(
        [cmake, "-S", str(source), "-B", str(build)], check=True, capture_output=True, text=True
    )
    settings = (build / "settings.cmake").read_text()
    assert "IDT_LIST" not in settings
    assert "TYPE\\;BUILD_ONLY" in settings
    assert "GROUP\\;DATA_REGION\\;NAME\\;isr_declarations" not in settings
    assert "GROUP\\;RODATA_REGION\\;NAME\\;isr_declarations" not in settings
    generator = (
        "iar/config_file_script.cmake" if backend == "iar" else "armlink/scatter_script.cmake"
    )
    output = build / "layout.script"
    subprocess.run(
        [
            cmake,
            "-C",
            str(build / "settings.cmake"),
            f"-DPASS={phase}",
            f"-DOUT_FILE={output}",
            f"-DSTEERING_FILE={build}/layout.steer",
            f"-DSTEERING_C={build}/layout.c",
            f"-DKEEP_FILE={build}/layout.keep",
            "-P",
            str(ROOT / "cmake/linker" / generator),
        ],
        check=True,
        capture_output=True,
        text=True,
    )
    text = output.read_text()
    assert "IDT_LIST" not in text
    assert (".irq.*" in text) is (phase == "")
    assert (".isr.*" in text) is (phase == "")
    if backend == "iar":
        assert '".intList": place noload at address mem:0 { block intList };' in text
        assert "first  section .irq_info" in text
        region = re.search(
            r"define region __isr_discovery_region = mem:\[from (\d+) size (\d+)\];", text
        )
        assert (region is not None) is (phase == "")
        if region is not None:
            start, capacity = map(int, region.groups())
            size = 64 if tight else 0x100000
            assert capacity >= 0x40000
            assert start + capacity <= 1 << 32
            for address in (0x1000, 0x20000000):
                assert start + capacity <= address or start > address + size
            assert '"isr_declarations": place in __isr_discovery_region' in text
            assert '".isr_pre0.entries": place in __isr_discovery_region' in text
            assert '"isr_declarations_init"' not in text
            assert '".isr_pre0.entries_init"' not in text
            assert "initialize by copy" not in text
        if tight:
            assert "region FLASH = mem:[from 0x1000 size 64]" in text
            assert "region RAM = mem:[from 0x20000000 size 64]" in text
            assert '"target_rom": place in FLASH' in text
            assert '"target_ram": place in RAM' in text
            if phase == "":
                assert len(re.findall(r"minimum size\s*=\s*0x20000", text)) == 2
    else:
        first = re.search(r"(\w+)_discovery (\d+) OVERLAY NOCOMPRESS", text)
        assert first is not None
        first_name, start = first.group(1), int(first.group(2))
        assert start % 65536 == 0
        limits = [
            int(value) for value in re.findall(r"ScatterAssert\(ImageLimit\(\w+\) <= (\d+)\)", text)
        ]
        assert limits and len(set(limits)) == 1
        limit = limits[0]
        assert limit % 65536 == 0 and start < limit < 1 << 32
        size = 64 if tight else 0x100000
        for address in (0x1000, 0x20000000):
            assert limit <= address // 65536 * 65536 or start > address + size
        assert "intList_discovery" in text
        assert "*.o(.irq_info, +First)" in text
        assert "*.o(.intList)" in text
        if phase == "":
            assert first_name == "intList"
            assert "isr_declarations_discovery AlignExpr(ImageLimit(intList), 4)" in text
            previous = "isr_declarations_discovery_end" if tight else "isr_declarations"
            assert f"isr_pre0_entries_discovery AlignExpr(ImageLimit({previous}), 4)" in text
            if tight:
                assert text.count("< 0x20000") == 2
                assert text.count("+ 0x20000") == 2
        else:
            assert "isr_declarations_discovery" not in text
            assert "isr_pre0_entries_discovery" not in text
        if tight:
            assert "FLASH 0x1000 NOCOMPRESS 64" in text
            assert "RAM 0x20000000 NOCOMPRESS 64" in text


@pytest.mark.parametrize(
    "name,start,size,error",
    [
        ("ALL", "0", "4294967296", "No independent virtual region"),
        ("ALL", "4294967292", "8", "exceeds the ARM32 address space"),
        ("__isr_discovery_region", "0x1000", "256", "is reserved for ISR discovery"),
    ],
)
def test_iar_discovery_rejects_invalid_virtual_region(cmake, tmp_path, name, start, size, error):
    settings = (
        'set(MEMORY_REGIONS "{NAME\\;' + name + r'\;START\;' + start + r'\;SIZE\;' + size + '}")\n'
        'set(SECTIONS "{NAME\\;scratch\\;ADDRESS\\;0\\;TYPE\\;BUILD_ONLY'
        '\\;NOINPUT\\;TRUE\\;NOSYMBOLS\\;TRUE}")\n'
        'set(SECTION_SETTINGS "{SECTION\\;scratch\\;INPUT\\;.irq.*\\;KEEP\\;TRUE}")\n'
    )
    result = iar_tests.generate(cmake, tmp_path, settings, phase="", check=False)
    assert result.returncode != 0
    assert error in result.stderr
