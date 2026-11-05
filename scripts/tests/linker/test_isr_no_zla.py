# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Keep C alignment extensions optional for scatter and ICF placement."""

import importlib
import os
import shlex
import shutil
import subprocess
from io import StringIO
from pathlib import Path

import pytest
import test_iar_isr as iar_tests

cmake = iar_tests.cmake
parser_factory = iar_tests.parser_factory
record = iar_tests.record

ROOT = Path(__file__).resolve().parents[3]


@pytest.mark.parametrize("capability", [None, 0])
@pytest.mark.parametrize("writable", [False, True])
def test_no_zero_length_arrays_in_strict_c(monkeypatch, tmp_path, capability, writable):
    monkeypatch.syspath_prepend(str(ROOT / "scripts/build"))
    layout = importlib.import_module("isr_table_layout")
    table = layout.Table("entries", "entries", 64, 0)
    marker = StringIO()
    table.write_alignment_source(marker, writable=writable)
    compiler = shlex.split(os.environ.get("ISR_NO_ZLA_CC", "cc"))
    if not shutil.which(compiler[0]):
        pytest.skip("A C compiler is required")
    source = tmp_path / "alignment.c"
    source.write_text(
        "#include <stdint.h>\n"
        + ("" if capability is None else f"#define TOOLCHAIN_HAS_ZLA {capability}\n")
        + marker.getvalue()
        + "int ordinary_c_object = 1;\n"
    )
    # Attribute macros are deliberately undefined: the inactive branch must
    # require neither GNU array syntax nor compiler-specific declarations.
    subprocess.run(
        compiler
        + [
            "-std=c11",
            "-pedantic-errors",
            "-Werror",
            "-c",
            str(source),
            "-o",
            str(tmp_path / "alignment.o"),
        ],
        check=True,
        capture_output=True,
        text=True,
    )
    preprocessed = subprocess.check_output(compiler + ["-E", "-P", str(source)], text=True)
    assert table.alignment_symbol not in preprocessed
    assert table.alignment_section not in preprocessed


@pytest.mark.parametrize("backend", ["iar", "armlink"])
@pytest.mark.parametrize(
    "shared,switch", [(False, False), (True, False), (False, True), (True, True)]
)
def test_backend_alignment_does_not_require_marker_bytes(
    cmake, parser_factory, tmp_path, backend, shared, switch
):
    parser = parser_factory(shared, switch)
    check_placement(cmake, tmp_path, backend, parser.tables.values())


@pytest.mark.parametrize("backend", ["iar", "armlink"])
def test_empty_table_retains_start_and_end(cmake, monkeypatch, tmp_path, backend):
    monkeypatch.syspath_prepend(str(ROOT / "scripts/build"))
    layout = importlib.import_module("isr_table_layout")
    check_placement(cmake, tmp_path, backend, [layout.Table("entries", "entries", 64, 0)])


def check_placement(cmake, directory, backend, tables):
    tables = list(tables)
    sections = []
    settings = StringIO()
    for table in tables:
        sections.append(
            record(
                NAME=table.section,
                VMA="RAM",
                LMA="FLASH",
                ALIGN=table.alignment,
                NOINPUT="TRUE",
                NOSYMBOLS="TRUE",
            )
        )
        table.write_cmake(settings)
    source = directory / "run.cmake"
    output = directory / "layout"
    generator = (
        "iar/config_file_script.cmake" if backend == "iar" else "armlink/scatter_script.cmake"
    )
    source.write_text(
        'set(MEMORY_REGIONS "{NAME\\;FLASH\\;START\\;0x1000\\;SIZE\\;0x10000}"\n'
        ' "{NAME\\;RAM\\;START\\;0x20000000\\;SIZE\\;0x10000}")\n'
        + "set(SECTIONS "
        + "\n".join(sections)
        + ")\n"
        + settings.getvalue()
        + '\nset(PASS LINKER_ZEPHYR_PREBUILT)\n'
        + f'set(OUT_FILE "{output}.script")\n'
        + f'set(STEERING_FILE "{output}.steer")\n'
        + f'set(STEERING_C "{output}.c")\n'
        + f'set(KEEP_FILE "{output}.keep")\n'
        + f'include("{ROOT}/cmake/linker/{generator}")\n'
    )
    subprocess.run([cmake, "-P", str(source)], check=True, capture_output=True, text=True)
    text = Path(f"{output}.script").read_text()
    for table in tables:
        # The alignment setting reserves zero bytes even when its optional
        # C input is absent. Real entry extents retain the target ABI stride.
        if backend == "iar":
            assert (
                f"define block {table.section} with fixed order, alignment={table.alignment}"
            ) in text
            assert (
                f"block {table.section}_100 with size = 0, alignment = {table.alignment}"
            ) in text
            assert f"{table.symbol} = ADDR({table.section}_100_input);" in text
            assert f"{table.end_symbol} = ADDR(" in text
        else:
            assert f"ALIGN {table.alignment} EMPTY 0x0" in text
            assert f"{table.section}_0_100 (ImageBase({table.section}) + 0) " in text
            assert f"ScatterAssert(ImageLength({table.section}_0_100) <= 0)" in text
            steering = Path(f"{output}.steer").read_text()
            assert f"RESOLVE {table.symbol} AS " in steering
            assert f"RESOLVE {table.end_symbol} AS " in steering
        for item in table.inputs:
            assert item.name in text
            assert item.offset % item.alignment == 0
        assert sum(item.size for item in table.inputs) == table.size
