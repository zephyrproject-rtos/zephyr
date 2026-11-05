# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Generate scatter/steering fixtures without requiring a licensed linker."""

import os
import shutil
import subprocess
from pathlib import Path

import pytest

ZEPHYR_BASE = Path(__file__).resolve().parents[3]


@pytest.fixture
def cmake():
    executable = os.environ.get("CMAKE", shutil.which("cmake"))
    if not executable:
        pytest.skip("CMake is unavailable")
    version = subprocess.check_output([executable, "--version"], text=True).split()[2]
    if tuple(map(int, version.split(".")[:2])) < (3, 28):
        pytest.skip("Scatter generation requires CMake 3.28 or newer")
    return executable


def generate(cmake, directory, settings, phase="LINKER_ZEPHYR_PREBUILT"):
    output = directory / "layout"
    script = directory / "run.cmake"
    script.write_text(
        'set(MEMORY_REGIONS "{NAME\\;FLASH\\;START\\;0x1000\\;SIZE\\;0x10000}"\n'
        ' "{NAME\\;RAM\\;START\\;0x20000000\\;SIZE\\;0x10000}")\n'
        + settings
        + f'\nset(PASS {phase})\n'
        + f'set(OUT_FILE "{output}.sct")\n'
        + f'set(STEERING_FILE "{output}.steer")\n'
        + f'set(STEERING_C "{output}.c")\n'
        + f'set(KEEP_FILE "{output}.keep")\n'
        + f'include("{ZEPHYR_BASE}/cmake/linker/armlink/scatter_script.cmake")\n'
    )
    subprocess.run([cmake, "-P", str(script)], check=True, capture_output=True, text=True)
    return {
        suffix: Path(f"{output}.{suffix}").read_text() for suffix in ("sct", "steer", "keep", "c")
    }


def record(**fields):
    return '"{' + r"\;".join(str(value) for pair in fields.items() for value in pair) + '}"'


def test_array_and_switch_alias(cmake, tmp_path):
    settings = (
        "set(SECTIONS "
        + record(NAME="sw_isr_table", VMA="RAM", NOINPUT="TRUE", SUBALIGN=8, ENDALIGN=8)
        + ")\nset(SECTION_SETTINGS\n"
    )
    for index in (1, 0):
        settings += (
            record(
                SECTION="sw_isr_table",
                PRIO=index,
                OFFSET=index * 16,
                ALIGN=8,
                INPUT=f".isr.entry.{index}",
                KEEP="TRUE",
                MAX_SIZE=16,
                SYMBOLS="_sw_isr_table" if index == 0 else "__isr_uart_entry",
            )
            + "\n"
        )
    settings += ")"
    result = generate(cmake, tmp_path, settings)
    assert result["sct"].index("*.o(.isr.entry.0)") < result["sct"].index("*.o(.isr.entry.1)")
    assert "sw_isr_table_16_1 (ImageBase(sw_isr_table) + 16) ALIGN 8" in result["sct"]
    assert "ScatterAssert(ImageLength(sw_isr_table_16_1) <= 16)" in result["sct"]
    assert "RESOLVE __isr_uart_entry AS Image$$sw_isr_table_16_1$$Base" in result["steer"]
    assert "RESOLVE __sw_isr_table_end AS Image$$sw_isr_table_end$$Limit" in result["steer"]
    assert '--keep="*.o(.isr.entry.1)"' in result["keep"]


def test_shared_capacity_and_padding(cmake, tmp_path):
    settings = (
        "set(SECTIONS "
        + record(NAME="shared_sw_isr_table", VMA="RAM", NOINPUT="TRUE", SUBALIGN=8)
        + ")\nset(SECTION_SETTINGS "
        + record(
            SECTION="shared_sw_isr_table",
            OFFSET=0,
            INPUT=".isr.shared.0",
            ALIGN=8,
            MIN_SIZE=40,
            MAX_SIZE=40,
            KEEP="TRUE",
            SYMBOLS="z_shared_sw_isr_table",
        )
        + ")"
    )
    result = generate(cmake, tmp_path, settings)
    assert "ImageLength(shared_sw_isr_table_0_100) <= 40" in result["sct"]
    assert "ImageBase(shared_sw_isr_table_0_100) + 40" in result["sct"]
    assert (
        "RESOLVE z_shared_sw_isr_table AS Image$$shared_sw_isr_table_0_100$$Base" in result["steer"]
    )
    assert (
        "RESOLVE __shared_sw_isr_table_end AS Image$$shared_sw_isr_table_end$$Limit"
        in result["steer"]
    )
    assert result["keep"].count(".isr.shared.0") == 1


@pytest.mark.parametrize(
    "phase,selected",
    [
        ("LINKER_ISR_DISCOVERY", "placeholder"),
        ("LINKER_ZEPHYR_PREBUILT", "generated"),
        ("LINKER_ZEPHYR_FINAL", "generated"),
    ],
)
def test_stage_selection(cmake, tmp_path, phase, selected):
    settings = (
        "set(SECTIONS "
        + record(NAME="irq_vector_table", VMA="FLASH", NOINPUT="TRUE")
        + ")\nset(SECTION_SETTINGS\n"
    )
    for name, passes in (
        ("placeholder", "LINKER_ISR_DISCOVERY"),
        ("generated", r"LINKER_ZEPHYR_PREBUILT\;LINKER_ZEPHYR_FINAL"),
    ):
        settings += (
            record(
                SECTION="irq_vector_table",
                OFFSET=0,
                INPUT=f".isr.vector.{name}",
                KEEP="TRUE",
                SYMBOLS="_irq_vector_table",
                PASS=passes,
            )
            + "\n"
        )
    result = generate(cmake, tmp_path, settings + ")", phase)
    rejected = "generated" if selected == "placeholder" else "placeholder"
    assert f".isr.vector.{selected}" in result["sct"]
    assert f".isr.vector.{selected}" in result["keep"]
    assert f".isr.vector.{rejected}" not in result["sct"]
    assert f".isr.vector.{rejected}" not in result["keep"]
    assert result["steer"].count("RESOLVE _irq_vector_table AS ") == 1


def test_per_pass_steering_dependencies(cmake, tmp_path):
    source = tmp_path / "source"
    source.mkdir()
    (source / "empty.c").write_text("void empty(void) {}\n")
    (source / "manifest.cmake").write_text("# Generated input\n")
    cmake_source = f'''
cmake_minimum_required(VERSION 3.28)
project(scatter_fixture C)
set(CONFIG_ISR_TABLES_LOCAL_DECLARATION TRUE)
set(ZEPHYR_BASE "{ZEPHYR_BASE}")
add_library(zephyr_interface INTERFACE)
add_library(arch__arm__core__cortex_m OBJECT empty.c)
add_library(offsets OBJECT empty.c)
add_custom_target(linker)
set_property(TARGET linker PROPERTY ENTRY empty)
set(OFFSETS_LIB offsets)
function(zephyr_linker_generate_linker_settings_file output)
  file(WRITE ${{output}} [==[
set(MEMORY_REGIONS
  "{{NAME\\;FLASH\\;START\\;0x1000\\;SIZE\\;0x10000}}" CACHE INTERNAL "")
]==])
endfunction()
include("{ZEPHYR_BASE}/cmake/linker/armlink/target.cmake")
foreach(stage pre0 pre1 final)
  configure_linker_script(${{stage}}.cmd LINKER_${{stage}}
    ${{CMAKE_CURRENT_SOURCE_DIR}}/manifest.cmake
  )
  add_executable(elf_${{stage}} empty.c)
  toolchain_ld_link_elf(
    TARGET_ELF elf_${{stage}}
    OUTPUT_MAP ${{CMAKE_CURRENT_BINARY_DIR}}/${{stage}}.map
    LINKER_SCRIPT ${{CMAKE_CURRENT_BINARY_DIR}}/${{stage}}.cmd
  )
  get_target_property(libraries elf_${{stage}} LINK_LIBRARIES)
  file(WRITE ${{CMAKE_CURRENT_BINARY_DIR}}/${{stage}}.libraries "${{libraries}}")
endforeach()
add_custom_target(scatter_files DEPENDS pre0.cmd pre1.cmd final.cmd)
'''
    (source / "CMakeLists.txt").write_text(cmake_source)
    build = tmp_path / "build"
    subprocess.run(
        [cmake, "-S", str(source), "-B", str(build), "-G", "Ninja"],
        check=True,
        capture_output=True,
        text=True,
    )
    subprocess.run(
        [cmake, "--build", str(build), "--target", "scatter_files"],
        check=True,
        capture_output=True,
        text=True,
    )
    targets = set()
    for stage in ("pre0", "pre1", "final"):
        libraries = (build / f"{stage}.libraries").read_text()
        assert f"--edit={build}/{stage}.steer" in libraries
        assert f"--via={build}/{stage}.keep" in libraries
        target = next(item for item in libraries.split(";") if "armlink_steering_" in item)
        targets.add(target)
        assert (build / f"{stage}.steer").is_file()
        assert (build / f"{stage}.keep").is_file()
    assert len(targets) == 3
    ninja = (build / "build.ninja").read_text()
    assert str(source / "manifest.cmake") in ninja
    assert str(ZEPHYR_BASE / "scripts/build/armlink_symbol_export.py") in ninja
    post_build = [
        line
        for line in ninja.splitlines()
        if "POST_BUILD =" in line and "armlink_symbol_export.py" in line
    ]
    assert len(post_build) == 3
    for line in post_build:
        assert line.index("armlink_symbol_export.py") < line.index("isr_metadata_finalize.py")


@pytest.mark.parametrize("backend", ["ld", "armlink"])
@pytest.mark.parametrize(
    "phase", ["LINKER_ISR_DISCOVERY", "LINKER_ZEPHYR_PREBUILT", "LINKER_ZEPHYR_FINAL"]
)
def test_cortex_m_exception_and_irq_layout(cmake, tmp_path, backend, phase):
    source = tmp_path / "source"
    source.mkdir()
    # Use the architecture's actual section declarations, not a fixture copy.
    (source / "CMakeLists.txt").write_text(
        f'''
cmake_minimum_required(VERSION 3.28)
project(cortex_m_vectors NONE)
list(APPEND CMAKE_MODULE_PATH "{ZEPHYR_BASE}/cmake/modules")
include(extensions)
add_custom_target(linker)
set(CONFIG_ISR_TABLES_LOCAL_DECLARATION y)
set(CONFIG_ROM_START_OFFSET 128)
set(CONFIG_NUM_IRQS 2)
set(CONFIG_GEN_IRQ_START_VECTOR 0)
set(CONFIG_GEN_IRQ_VECTOR_TABLE y)
set(VECTOR_ALIGN 128)
set(VECTOR_TABLE_MIN_SIZE 256)
zephyr_linker_section(NAME .rom_start ADDRESS 0x1000 VMA FLASH NOINPUT)
include("{ZEPHYR_BASE}/cmake/linker_script/arm/cortex-m-vectors.cmake")
get_target_property(sections linker SECTIONS)
get_target_property(settings linker SECTION_SETTINGS)
file(WRITE ${{CMAKE_CURRENT_BINARY_DIR}}/architecture.cmake
  "set(SECTIONS [==[${{sections}}]==])\\nset(SECTION_SETTINGS [==[${{settings}}]==])\\n"
)
'''
    )
    build = tmp_path / "build"
    subprocess.run(
        [cmake, "-S", str(source), "-B", str(build)],
        check=True,
        capture_output=True,
        text=True,
    )
    settings = (build / "architecture.cmake").read_text()
    settings += "\nlist(APPEND SECTION_SETTINGS\n"
    # Two explicit generated vector objects plus separate public start/end anchors.
    for index, fields in enumerate(
        [
            dict(OFFSET=192, SYMBOLS="_irq_vector_table"),
            dict(OFFSET=192, INPUT=".irq.vector.0", SYMBOLS="vector0", MIN_SIZE=4, MAX_SIZE=4),
            dict(OFFSET=196, INPUT=".irq.vector.1", SYMBOLS="vector1", MIN_SIZE=4, MAX_SIZE=4),
            dict(OFFSET=200, SYMBOLS="_irq_vector_table_end"),
        ]
    ):
        settings += (
            record(
                SECTION=".rom_start",
                PRIO=100 + index,
                ALIGN=4,
                KEEP="TRUE",
                PASS=r"LINKER_ZEPHYR_PREBUILT\;LINKER_ZEPHYR_FINAL",
                **fields,
            )
            + "\n"
        )
    settings += ")\n"
    if backend == "armlink":
        result = generate(cmake, tmp_path, settings, phase)
        output = result["sct"]
        assert "RESOLVE _vector_start AS Image$$rom_start_128_50$$Base" in result["steer"]
        assert "RESOLVE _vector_end AS Image$$rom_start_200_2147483647$$Base" in result["steer"]
        assert "ImageLength(rom_start_128_50) <= 64" in output
        assert "ImageBase(rom_start_200_2147483647) + 184" in output
        if phase != "LINKER_ISR_DISCOVERY":
            assert "rom_start_192_100" in output
            assert "rom_start_192_101" in output
    else:
        script = tmp_path / "gnu.cmake"
        output_file = tmp_path / "gnu.ld"
        script.write_text(
            'set(MEMORY_REGIONS "{NAME\\;FLASH\\;START\\;0x1000\\;SIZE\\;0x10000}")\n'
            'set(FORMAT elf32-littlearm)\nset(ENTRY z_arm_reset)\n'
            + settings
            + f'\nset(PASS {phase})\nset(OUT_FILE "{output_file}")\n'
            + f'include("{ZEPHYR_BASE}/cmake/linker/ld/ld_script.cmake")\n'
        )
        subprocess.run([cmake, "-P", str(script)], check=True, capture_output=True, text=True)
        output = output_file.read_text()
        assert "_vector_start = .;" in output
        assert "_vector_end = .;" in output
        assert ". = 192; /* OFFSET */" in output
        assert ". = 200; /* OFFSET */" in output
    assert ".exc_vector_table*" in output
    assert ".gnu.linkonce.irq_vector_table" not in output
    if phase == "LINKER_ISR_DISCOVERY":
        assert ".isr_pre0.vectors" in output
        assert ".irq.vector.0" not in output
    else:
        assert ".isr_pre0.vectors" not in output
        assert output.index(".exc_vector_table*") < output.index(".irq.vector.0")
        assert output.index(".irq.vector.0") < output.index(".irq.vector.1")


@pytest.mark.parametrize("backend", ["ld", "armlink"])
@pytest.mark.parametrize(
    "phase", ["LINKER_ISR_DISCOVERY", "LINKER_ZEPHYR_PREBUILT", "LINKER_ZEPHYR_FINAL"]
)
def test_cortex_m_soc_owned_vectors(cmake, tmp_path, backend, phase):
    source = tmp_path / "source"
    source.mkdir()
    (source / "CMakeLists.txt").write_text(
        f'''
cmake_minimum_required(VERSION 3.28)
project(cortex_m_manual_vectors NONE)
list(APPEND CMAKE_MODULE_PATH "{ZEPHYR_BASE}/cmake/modules")
include(extensions)
add_custom_target(linker)
set(CONFIG_ISR_TABLES_LOCAL_DECLARATION y)
set(CONFIG_GEN_IRQ_VECTOR_TABLE n)
set(CONFIG_ROM_START_OFFSET 128)
set(CONFIG_NUM_IRQS 6368)
set(VECTOR_ALIGN 128)
set(vector_table_min_size_arg MIN_SIZE 256)
zephyr_linker_section(NAME .rom_start ADDRESS 0x1000 VMA FLASH NOINPUT)
include("{ZEPHYR_BASE}/cmake/linker_script/arm/cortex-m-vectors.cmake")
get_target_property(sections linker SECTIONS)
get_target_property(settings linker SECTION_SETTINGS)
file(WRITE ${{CMAKE_CURRENT_BINARY_DIR}}/architecture.cmake
  "set(SECTIONS [==[${{sections}}]==])\\nset(SECTION_SETTINGS [==[${{settings}}]==])\\n"
)
'''
    )
    build = tmp_path / "build"
    subprocess.run(
        [cmake, "-S", str(source), "-B", str(build)], check=True, capture_output=True, text=True
    )
    settings = (build / "architecture.cmake").read_text()
    if backend == "armlink":
        result = generate(cmake, tmp_path, settings, phase)
        output = result["sct"]
        assert "RESOLVE _vector_start AS Image$$rom_start_128_50$$Base" in result["steer"]
        assert "RESOLVE _vector_end AS Image$$rom_start_128_50$$Limit" in result["steer"]
        assert '--keep="*.o(.isr_vector_table*)"' in result["keep"]
    else:
        output_file = tmp_path / "manual.ld"
        script = tmp_path / "manual.cmake"
        script.write_text(
            'set(MEMORY_REGIONS "{NAME\\;FLASH\\;START\\;0x1000\\;SIZE\\;0x10000}")\n'
            'set(FORMAT elf32-littlearm)\nset(ENTRY z_arm_reset)\n'
            + settings
            + f'\nset(PASS {phase})\nset(OUT_FILE "{output_file}")\n'
            + f'include("{ZEPHYR_BASE}/cmake/linker/ld/ld_script.cmake")\n'
        )
        subprocess.run([cmake, "-P", str(script)], check=True, capture_output=True, text=True)
        output = output_file.read_text()
        assert "_vector_start = .;" in output
        assert "_vector_end = .;" in output
        assert "KEEP(*(.isr_vector_table*))" in output
    assert ".exc_vector_table*" in output
    assert ".isr_vector_table*" in output
    assert ".isr_pre0.vectors" not in output
    assert ".gnu.linkonce.irq_vector_table" not in output
    assert ".irq.vector" not in output


def test_explicit_symbol_without_automatic_alias(cmake, tmp_path):
    settings = (
        "set(SECTIONS "
        + record(NAME="isr_entries", VMA="FLASH", NOINPUT="TRUE", NOSYMBOLS="TRUE")
        + ")\nset(SECTION_SETTINGS "
        + record(
            SECTION="isr_entries",
            OFFSET=0,
            ALIGN=4,
            SYMBOLS="__isr_entries_start",
        )
        + ")"
    )
    result = generate(cmake, tmp_path, settings)
    assert result["steer"].count("RESOLVE __isr_entries_start AS ") == 1
    assert "RESOLVE __isr_entries_start AS Image$$isr_entries_0_100$$Base" in result["steer"]
    assert "RESOLVE __isr_entries_size AS " not in result["steer"]


@pytest.mark.parametrize("phase,active", [("", True), ("LINKER_ZEPHYR_FINAL", False)])
def test_info_is_rejected_only_when_active(cmake, tmp_path, phase, active):
    script = tmp_path / "info.cmake"
    script.write_text(
        'set(MEMORY_REGIONS "{NAME\\;FLASH\\;START\\;0x1000\\;SIZE\\;0x10000}")\n'
        + "set(SECTIONS "
        + record(
            NAME="metadata",
            VMA="FLASH",
            TYPE="INFO",
            NOINPUT="TRUE",
            PASS=r"NOT\;LINKER_ZEPHYR_PREBUILT\;LINKER_ZEPHYR_FINAL",
        )
        + ")\n"
        + f'set(PASS "{phase}")\nset(OUT_FILE "{tmp_path}/info.sct")\n'
        + f'include("{ZEPHYR_BASE}/cmake/linker/armlink/scatter_script.cmake")\n'
    )
    result = subprocess.run([cmake, "-P", str(script)], capture_output=True, text=True)
    if active:
        assert result.returncode != 0
        assert "does not support non-allocated INFO" in result.stderr
    else:
        assert result.returncode == 0, result.stderr


def test_unrelated_system_info_is_not_silently_loaded(cmake, tmp_path):
    settings = (
        'set(SECTIONS '
        + record(NAME="debug_records", ADDRESS=0, TYPE="INFO", NOINPUT="TRUE", NOSYMBOLS="TRUE")
        + ')\n'
    )
    with pytest.raises(subprocess.CalledProcessError) as error:
        generate(cmake, tmp_path, settings, phase="")
    assert "INFO requires the ISR metadata finalizer" in error.value.stderr


def test_discovery_requires_an_independent_address_page(cmake, tmp_path):
    settings = (
        'set(MEMORY_REGIONS ' + record(NAME="ALL", START=0, SIZE=4294967296) + ')\n'
        'set(SECTIONS '
        + record(NAME="intList", ADDRESS=0, TYPE="INFO", NOINPUT="TRUE", NOSYMBOLS="TRUE")
        + ')\n'
    )
    with pytest.raises(subprocess.CalledProcessError) as error:
        generate(cmake, tmp_path, settings, phase="")
    assert "No independent virtual region" in error.value.stderr


def test_bss_extent_uses_region_names_not_zi_symbol_suffixes(cmake, tmp_path):
    settings = (
        "set(SECTIONS "
        + record(NAME="bss", VMA="RAM", TYPE="BSS", NOINIT="TRUE", NOINPUT="TRUE", ENDALIGN=8)
        + ")\n"
        "set(SECTION_SETTINGS " + record(SECTION="bss", INPUT=".bss*", ALIGN=8) + ")\n"
    )
    result = generate(cmake, tmp_path, settings)
    assert "ImageLimit(bss_end) - ImageBase(bss)" in result["sct"]
    assert "ImageLimit(bss_end$$ZI)" not in result["sct"]
    assert "ImageBase(bss$$ZI)" not in result["sct"]
    assert "Image$$bss$$ZI$$Base" in result["steer"]


def test_zero_byte_alignment_marker_uses_valid_region_alignment(cmake, tmp_path):
    settings = (
        "set(SECTIONS "
        + record(NAME="sw_isr_table", VMA="FLASH", NOINPUT="TRUE")
        + ")\nset(SECTION_SETTINGS "
        + record(
            SECTION="sw_isr_table",
            OFFSET=16,
            ALIGN=1,
            INPUT=".isr.alignment",
            MIN_SIZE=0,
            MAX_SIZE=0,
            KEEP="TRUE",
        )
        + ")\n"
    )
    result = generate(cmake, tmp_path, settings)
    assert "(ImageBase(sw_isr_table) + 16) ALIGN 4" in result["sct"]
    assert "ALIGN 1" not in result["sct"]
    assert "ImageLength(sw_isr_table_16_100) <= 0" in result["sct"]


def test_symbol_only_setting_marks_current_boundary(cmake, tmp_path):
    settings = (
        "set(SECTIONS "
        + record(NAME="device_api_area", VMA="FLASH", NOINPUT="TRUE")
        + ")\nset(SECTION_SETTINGS "
        + record(
            SECTION="device_api_area",
            INPUT=".uart.*",
            SORT="NAME",
            PRIO=10,
            SYMBOLS=r"uart_start\;uart_end",
            KEEP="TRUE",
        )
        + "\n"
        + record(SECTION="device_api_area", SYMBOLS="uart_ext_end", PRIO=11)
        + ")"
    )
    result = generate(cmake, tmp_path, settings)
    assert "device_api_area_11 +0" in result["sct"]
    assert "RESOLVE uart_ext_end AS Image$$device_api_area_11$$Base" in result["steer"]


def test_canonical_symbol_roots_have_no_runtime_storage(cmake, tmp_path):
    settings = "set(SECTIONS " + record(NAME="table", VMA="FLASH", NOINPUT="TRUE") + ")"
    result = generate(cmake, tmp_path, settings)
    assert '__asm__(".global Image$$table$$Base\\n");' in result["c"]
    assert "__armlink_symbol_steering" not in result["c"]
    assert "extern char" not in result["c"]
