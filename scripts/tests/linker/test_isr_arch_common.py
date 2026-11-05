# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Exercise architecture-common ISR policy with each backend and link stage."""

import re
import subprocess
from pathlib import Path

import pytest
import test_iar_isr as iar_tests

cmake = iar_tests.cmake
ROOT = Path(__file__).resolve().parents[3]


def collect(cmake, directory, backend, local, code, variant=None):
    source, build = directory / "source", directory / "build"
    source.mkdir(parents=True)
    (source / "CMakeLists.txt").write_text(f'''
cmake_minimum_required(VERSION 3.28)
project(arch_common_isr_fixture NONE)
set(ZEPHYR_BASE "{ROOT}")
list(APPEND CMAKE_MODULE_PATH "{ROOT}/cmake/modules")
include(extensions)
add_custom_target(linker)
set(CONFIG_GEN_ISR_TABLES TRUE)
set(CONFIG_GEN_SW_ISR_TABLE TRUE)
set(CONFIG_GEN_SW_ISR_TABLE_ARRAY TRUE)
set(CONFIG_GEN_IRQ_VECTOR_TABLE TRUE)
set(CONFIG_ISR_TABLES_LOCAL_DECLARATION {"TRUE" if local else "FALSE"})
set(CONFIG_IRQ_VECTOR_TABLE_JUMP_BY_CODE {"TRUE" if code else "FALSE"})
set(LINKER {backend})
set(ZEPHYR_TOOLCHAIN_VARIANT {variant or backend})
zephyr_linker_memory(NAME FLASH START 0x1000 SIZE 0x100000 FLAGS rx)
zephyr_linker_memory(NAME RAM START 0x20000000 SIZE 0x100000 FLAGS rw)
{'' if local else 'zephyr_linker_memory(NAME IDT_LIST START 0xffff8000 SIZE 0x800 FLAGS rw)'}
zephyr_linker_group(NAME ROM_REGION LMA FLASH)
zephyr_linker_group(NAME RAM_REGION VMA RAM LMA FLASH)
zephyr_linker_group(NAME RODATA_REGION GROUP ROM_REGION)
zephyr_linker_group(NAME DATA_REGION GROUP RAM_REGION)
file(WRITE "${{CMAKE_BINARY_DIR}}/calls.log" "")
foreach(helper add_library add_dependencies target_link_libraries zephyr_link_libraries
    zephyr_library_link_libraries zephyr_compile_definitions)
  macro(${{helper}})
    file(APPEND "${{CMAKE_BINARY_DIR}}/calls.log" "${{ARGV}}\\n")
  endmacro()
endforeach()
foreach(helper zephyr_library zephyr_library_sources zephyr_library_sources_ifdef
    zephyr_library_include_directories zephyr_library_property zephyr_cc_option
    zephyr_linker_sources zephyr_linker_sources_ifdef add_subdirectory_ifdef)
  macro(${{helper}})
  endmacro()
endforeach()
include("{ROOT}/arch/common/CMakeLists.txt")
zephyr_linker_generate_linker_settings_file("${{CMAKE_BINARY_DIR}}/settings.cmake")
''')
    subprocess.run(
        [cmake, "-S", str(source), "-B", str(build)], check=True, capture_output=True, text=True
    )
    return build, (build / "settings.cmake").read_text().replace(r"\;", ";")


def render(cmake, build, backend, phase):
    generator = {
        "ld": "ld/ld_script.cmake",
        "iar": "iar/config_file_script.cmake",
        "armlink": "armlink/scatter_script.cmake",
    }[backend]
    output = build / (phase or "pre0")
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


@pytest.mark.parametrize("backend", ["ld", "iar", "armlink"])
@pytest.mark.parametrize("local", [False, True])
@pytest.mark.parametrize("code", [False, True])
def test_common_isr_metadata_and_discovery(cmake, tmp_path, backend, local, code):
    build, settings = collect(cmake, tmp_path, backend, local, code)
    declarations = re.findall(r"\{([^{}]+)\}", settings)
    metadata = [record for record in declarations if "NAME;.intList;" in record]
    assert len(metadata) == 1
    inputs = [record for record in declarations if "SECTION;.intList;" in record]
    assert len(inputs) == 2
    assert sum("FIRST;TRUE" in record for record in inputs) == 1
    assert all(("KEEP;TRUE" in record) is not (local and backend == "iar") for record in inputs)
    calls = (build / "calls.log").read_text()
    assert ("isr_tables_discovery;OBJECT;isr_tables.c" in calls) is local
    assert ("_sw_isr_table" in calls) is not local
    assert ("Z_ISR_LOCAL_METADATA_NONALLOC=1" in calls) is (local and backend == "armlink")
    for phase in ("", "LINKER_ZEPHYR_PREBUILT", "LINKER_ZEPHYR_FINAL"):
        text = render(cmake, build, backend, phase)
        if local:
            assert (".irq.*" in text) is (not phase)
            assert (".isr_pre0.entries" in text or "isr_pre0_entries" in text) is (
                not phase and (backend != "ld" or not code)
            )
        elif backend == "iar":
            assert "section .irq_info" in text and "section .intList" in text
            assert "GROUP;RODATA_REGION" in metadata[0]


def test_legacy_iar_storage_uses_toolchain_variant(cmake, tmp_path):
    _, settings = collect(cmake, tmp_path, "ld", False, False, variant="iar")
    metadata = next(
        record for record in re.findall(r"\{([^{}]+)\}", settings) if "NAME;.intList;" in record
    )
    assert "GROUP;RODATA_REGION" in metadata
    assert "VMA;IDT_LIST" not in metadata


@pytest.mark.parametrize("backend,code", [("ld", True), ("iar", False), ("armlink", False)])
def test_discovery_policy_ignores_cmake_cache(cmake, tmp_path, backend, code):
    build, _ = collect(cmake, tmp_path, backend, True, code)
    before = (build / "settings.cmake").read_bytes()
    calls = (build / "calls.log").read_bytes()
    subprocess.run(
        [
            cmake,
            "-S",
            str(tmp_path / "source"),
            "-B",
            str(build),
            "-Drefs_discard_pass=PASS;UNSELECTED_PHASE",
            "-Dvector_sections=cache_vector_section",
            "-Dcollect_vector_entries=FALSE",
        ],
        check=True,
        capture_output=True,
        text=True,
    )
    assert (build / "settings.cmake").read_bytes() == before
    assert (build / "calls.log").read_bytes() == calls
