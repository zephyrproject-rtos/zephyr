# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Exercise ICF placement using the production ISR parser, without an IAR license."""

import importlib
import os
import shutil
import subprocess
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[3]


@pytest.fixture
def cmake():
    executable = os.environ.get("CMAKE", shutil.which("cmake"))
    if executable is None:
        pytest.skip("CMake is unavailable")
    version = subprocess.check_output([executable, "--version"], text=True).split()[2]
    if tuple(map(int, version.split(".")[:2])) < (3, 28):
        pytest.skip("ICF generation requires CMake 3.28 or newer")
    return executable


def record(**fields):
    return '"{' + r"\;".join(str(value) for pair in fields.items() for value in pair) + '}"'


def generate(cmake, directory, settings, phase="LINKER_ZEPHYR_PREBUILT", *, check=True):
    script = directory / "run.cmake"
    output = directory / "layout.icf"
    script.write_text(
        'set(MEMORY_REGIONS "{NAME\\;FLASH\\;START\\;0x1000\\;SIZE\\;0x10000}"\n'
        ' "{NAME\\;RAM\\;START\\;0x20000000\\;SIZE\\;0x10000}")\n'
        + settings
        + f'\nset(PASS "{phase}")\nset(OUT_FILE "{output}")\n'
        + f'set(STEERING_FILE "{directory}/layout.xcl")\n'
        + f'include("{ROOT}/cmake/linker/iar/config_file_script.cmake")\n'
    )
    result = subprocess.run([cmake, "-P", str(script)], capture_output=True, text=True, check=check)
    return output.read_text() if check else result


@pytest.fixture
def parser_factory(monkeypatch):
    monkeypatch.syspath_prepend(str(ROOT / "scripts/build"))
    monkeypatch.syspath_prepend(str(ROOT / "scripts/tests/isr_tables"))
    module = importlib.import_module("gen_isr_tables_parser_local")
    fixtures = importlib.import_module("test_isr_local_layout")

    def create(shared, switch):
        config = fixtures.Configuration(shared=shared, switch=switch, vector=True)
        registrations = [(17, 0, ".irq.first"), (19, 0, ".irq.single")]
        if shared:
            registrations.append((17, 0, ".irq.second"))
        parser = module.gen_isr_parser(
            fixtures.metadata(module, config, registrations, 4), config, None
        )
        parser.vector_cmake_placement = (".rom_start", 64)
        return parser

    return create


@pytest.mark.parametrize(
    "shared,switch", [(False, False), (True, False), (False, True), (True, True)]
)
def test_production_parser_slots_and_used_ends(cmake, tmp_path, parser_factory, shared, switch):
    parser = parser_factory(shared, switch)
    manifest = tmp_path / "isr_tables_layout.cmake"
    with manifest.open("w") as stream:
        parser.write_cmake(stream)
    tables = list(parser.tables.values())
    sections = [record(NAME=".rom_start", VMA="FLASH", NOINPUT="TRUE", NOSYMBOLS="TRUE")]
    sections.extend(
        record(NAME=table.section, VMA="FLASH", NOINPUT="TRUE", NOSYMBOLS="TRUE")
        for table in tables
        if table.section != "irq_vector_table"
    )
    settings = "set(SECTIONS " + "\n".join(sections) + ")\nset(SECTION_SETTINGS "
    settings += (
        record(
            SECTION=".rom_start",
            OFFSET=0,
            INPUT=".exc_vector_table",
            ALIGN=256,
            MIN_SIZE=64,
            MAX_SIZE=64,
            SYMBOLS="_vector_start",
            PRIO=50,
            KEEP="TRUE",
        )
        + ')\nset(INCLUDES "{CMAKE\\;'
        + str(manifest)
        + '}")\n'
    )
    output = generate(cmake, tmp_path, settings)
    assert "define image symbol __Vectors = ADDR(rom_start_50_input);" in output
    for table in tables:
        name = "rom_start" if table.section == "irq_vector_table" else table.section
        for index, item in enumerate(table.inputs, 101):
            block = f"{name}_{index}"
            assert f"block {block} with size = {item.size}," in output
            assert f"block {block}_input with fixed order, alignment = {item.alignment}" in output
            assert f"{item.symbol} = ADDR({block}_input);" in output
            assert f"{item.symbol}_end = (ADDR({block}_input) + SIZE({block}_input));" in output
            assert f"section {item.name}" in output
            assert f"maximum size = {item.size}" in output
            assert "keep {" in output
    # The third vector has OFFSET 72, but its slot is still four bytes.
    assert "block rom_start_103 with size = 4," in output
    assert "block rom_start_103 with size = 76," not in output


@pytest.mark.parametrize(
    "phase,expected",
    [
        ("", "discovery"),
        ("LINKER_ZEPHYR_PREBUILT", "generated"),
        ("LINKER_ZEPHYR_FINAL", "generated"),
    ],
)
def test_stage_exclusion(cmake, tmp_path, phase, expected):
    settings = 'set(SECTIONS ' + record(NAME="sw_isr_table", VMA="FLASH", NOINPUT="TRUE")
    settings += ")\nset(SECTION_SETTINGS "
    for name, passes in [
        ("discovery", r"NOT\;LINKER_ZEPHYR_PREBUILT\;LINKER_ZEPHYR_FINAL"),
        ("generated", r"LINKER_ZEPHYR_PREBUILT\;LINKER_ZEPHYR_FINAL"),
    ]:
        settings += (
            record(
                SECTION="sw_isr_table",
                INPUT=f".isr.{name}",
                KEEP="TRUE",
                SYMBOLS="_sw_isr_table",
                PASS=passes,
            )
            + "\n"
        )
    output = generate(cmake, tmp_path, settings + ")", phase)
    assert f"section .isr.{expected}" in output
    rejected = "generated" if expected == "discovery" else "discovery"
    assert f"section .isr.{rejected}" not in output


@pytest.mark.parametrize(
    "first,second,alignment,minimum,error",
    [
        (8, 0, 4, 0, "Non-monotonic offsets"),
        (2, 8, 4, 0, "Misaligned offset"),
        (0, 4, 4, 8, "Minimum input size exceeds its slot"),
    ],
)
def test_rejects_inconsistent_slot_records(
    cmake, tmp_path, first, second, alignment, minimum, error
):
    settings = 'set(SECTIONS ' + record(NAME="entries", VMA="FLASH", NOINPUT="TRUE")
    settings += ")\nset(SECTION_SETTINGS "
    settings += record(
        SECTION="entries", OFFSET=first, INPUT=".irq.first", ALIGN=alignment, MIN_SIZE=minimum
    )
    settings += record(SECTION="entries", OFFSET=second, SYMBOLS="table_end") + ")"
    result = generate(cmake, tmp_path, settings, check=False)
    assert result.returncode != 0
    assert error in result.stderr


def test_settings_dependencies_and_sram_initializers(cmake, tmp_path):
    source = tmp_path / "source"
    source.mkdir()
    manifest = source / "isr_tables_layout.cmake"
    manifest.write_text(
        "list(APPEND SECTION_SETTINGS "
        + record(
            SECTION="sw_isr_table",
            OFFSET=0,
            ALIGN=4,
            INPUT=".irq.real",
            KEEP="TRUE",
            MIN_SIZE=8,
            MAX_SIZE=8,
            SYMBOLS=r"entry\;entry_end",
        )
        + ")\n"
    )
    (source / "CMakeLists.txt").write_text(f'''
cmake_minimum_required(VERSION 3.28)
project(iar_stage_fixture NONE)
set(ZEPHYR_BASE "{ROOT}")
list(APPEND CMAKE_MODULE_PATH "{ROOT}/cmake/modules")
include(extensions)
add_custom_target(linker)
set(IAR_LINKER ilinkarm)
set(CONFIG_IAR_ZEPHYR_INIT TRUE)
include("{ROOT}/cmake/linker/iar/target.cmake")
zephyr_linker_memory(NAME FLASH START 0x1000 SIZE 0x10000 FLAGS rx)
zephyr_linker_memory(NAME RAM START 0x20000000 SIZE 0x10000 FLAGS rw)
zephyr_linker_group(NAME DATA VMA RAM LMA FLASH)
zephyr_linker_section(NAME sw_isr_table GROUP DATA NOINPUT NOSYMBOLS)
zephyr_linker_include_generated(CMAKE "{manifest}"
  PASS LINKER_ZEPHYR_PREBUILT LINKER_ZEPHYR_FINAL)
configure_linker_script(pre0.icf "" "{manifest}")
configure_linker_script(pre1.icf "CUSTOM;LINKER_ZEPHYR_PREBUILT" "{manifest}")
configure_linker_script(final.icf "LINKER_ZEPHYR_FINAL" "{manifest}")
add_custom_target(icf_files DEPENDS pre0.icf pre1.icf final.icf)
''')
    build = tmp_path / "build"
    subprocess.run(
        [cmake, "-S", str(source), "-B", str(build), "-G", "Ninja"],
        check=True,
        capture_output=True,
        text=True,
    )
    subprocess.run(
        [cmake, "--build", str(build), "--target", "icf_files"],
        check=True,
        capture_output=True,
        text=True,
    )
    assert len(list((build / "include/generated").glob("ld_script_settings_*.cmake"))) == 3
    assert ".irq.real" not in (build / "pre0.icf").read_text()
    for phase in ("pre1", "final"):
        output = (build / f"{phase}.icf").read_text()
        assert "initialize manually with copy friendly" in output
        assert "block sw_isr_table_100_input_init" in output
        assert (
            "entry_end = (ADDR(sw_isr_table_100_input) + SIZE(sw_isr_table_100_input));" in output
        )
        assert "keep {" in output
    ninja = (build / "build.ninja").read_text()
    assert str(manifest) in ninja
    assert str(ROOT / "cmake/linker/iar/config_file_script.cmake") in ninja
    assert str(ROOT / "cmake/linker/linker_script_common.cmake") in ninja


@pytest.mark.parametrize("phase,active", [("", True), ("LINKER_ZEPHYR_FINAL", False)])
def test_info_is_nonloaded_only_when_active(cmake, tmp_path, phase, active):
    settings = (
        "set(SECTIONS "
        + record(
            NAME="metadata",
            ADDRESS=0,
            TYPE="INFO",
            NOINPUT="TRUE",
            PASS=r"NOT\;LINKER_ZEPHYR_PREBUILT\;LINKER_ZEPHYR_FINAL",
        )
        + ")\nset(SECTION_SETTINGS "
        + record(SECTION="metadata", INPUT=".intList")
        + ")"
    )
    result = generate(cmake, tmp_path, settings, phase, check=False)
    assert result.returncode == 0, result.stderr
    output = (tmp_path / "layout.icf").read_text()
    assert ('place noload at address mem:0' in output) is active


def test_unmodified_named_input_sort_keeps_its_original_block(cmake, tmp_path):
    settings = "set(SECTIONS " + record(NAME="init", VMA="FLASH", NOINPUT="TRUE")
    settings += (
        ")\nset(SECTION_SETTINGS "
        + record(SECTION="init", INPUT=".z_init_*", SORT="NAME", KEEP="TRUE")
        + ")"
    )
    output = generate(cmake, tmp_path, settings)
    assert "block init_100 with alphabetical order, alignment=4" in output
    assert "init_100_input" not in output
