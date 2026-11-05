# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0
"""Keep ISR table alignment without changing input sizes or XIP placement."""

import importlib
import os
import shutil
import subprocess
from io import StringIO
from pathlib import Path

import pytest

ZEPHYR_BASE = Path(__file__).resolve().parents[3]


@pytest.fixture
def table(monkeypatch):
    monkeypatch.syspath_prepend(str(ZEPHYR_BASE / "scripts/build"))
    layout = importlib.import_module("isr_table_layout")
    return layout.Table(
        "sw_isr_table",
        "_sw_isr_table",
        64,
        16,
        [
            layout.InputSection(".entry.0", 0, 8, 4, "entry_0"),
            layout.InputSection(".entry.1", 8, 8, 4, "entry_1"),
        ],
    )


@pytest.fixture
def cmake():
    executable = os.environ.get("CMAKE", shutil.which("cmake"))
    if not executable:
        pytest.skip("CMake is unavailable")
    version = subprocess.check_output([executable, "--version"], text=True).split()[2]
    if tuple(map(int, version.split(".")[:2])) < (3, 28):
        pytest.skip("Linker generation requires CMake 3.28 or newer")
    return executable


@pytest.mark.parametrize("writable", [False, True])
def test_alignment_marker_has_no_table_bytes(table, writable):
    source = StringIO()
    table.write_alignment_source(source, writable=writable)
    text = source.getvalue()
    assert "[0] __aligned(64)" in text
    assert ("static const uint8_t" in text) is not writable
    assert table.alignment_section in text
    assert table.size == 16
    assert [item.offset for item in table.inputs] == [0, 8]


def test_gnu_template_keeps_marker_before_entries(table):
    script = StringIO()
    table.write_ld(script)
    text = script.getvalue()
    assert text.index(table.alignment_section) < text.index('_sw_isr_table = .;')
    assert text.count(table.alignment_section) == 1


@pytest.mark.parametrize("backend", ["gnu", "lld", "armlink"])
def test_cmake_alignment_attributes_and_marker(cmake, tmp_path, table, backend):
    layout = StringIO()
    table.write_cmake(layout)
    output = tmp_path / "layout"
    script = tmp_path / "run.cmake"
    generator = "armlink/scatter_script.cmake" if backend == "armlink" else "ld/ld_script.cmake"
    script.write_text(
        'set(FORMAT elf32-littlearm)\nset(ENTRY _start)\n'
        'set(MEMORY_REGIONS "{NAME\\;FLASH\\;START\\;0x1000\\;SIZE\\;0x10000}"\n'
        ' "{NAME\\;RAM\\;START\\;0x20000000\\;SIZE\\;0x10000}")\n'
        'set(SECTIONS "{NAME\\;sw_isr_table\\;VMA\\;RAM\\;LMA\\;FLASH'
        '\\;ALIGN_WITH_INPUT\\;TRUE\\;ALIGN\\;64\\;NOINPUT\\;TRUE}")\n'
        + layout.getvalue()
        + '\nset(PASS LINKER_ZEPHYR_PREBUILT)\n'
        + f'set(LINKER_IS_LLD {"TRUE" if backend == "lld" else "FALSE"})\n'
        + f'set(OUT_FILE "{output}.ld")\n'
        + f'set(STEERING_FILE "{output}.steer")\n'
        + f'set(STEERING_C "{output}.c")\n'
        + f'set(KEEP_FILE "{output}.keep")\n'
        + f'include("{ZEPHYR_BASE}/cmake/linker/{generator}")\n'
    )
    subprocess.run([cmake, "-P", str(script)], check=True, capture_output=True, text=True)
    text = Path(f"{output}.ld").read_text()
    assert text.count(table.alignment_section) == 1
    assert ".entry.0" in text and ".entry.1" in text
    if backend == "gnu":
        assert ": ALIGN_WITH_INPUT\n" in text
        assert "ALIGN_WITH_INPUT ALIGN(" not in text
    elif backend == "lld":
        assert ": ALIGN(64)\n" in text
        assert "ALIGN_WITH_INPUT" not in text
    else:
        keep = Path(f"{output}.keep").read_text()
        assert table.alignment_section in keep
        assert "RESOLVE _sw_isr_table AS Image$$sw_isr_table_0_100$$Base" in (
            Path(f"{output}.steer").read_text()
        )
        assert "sw_isr_table_0_100" in text
