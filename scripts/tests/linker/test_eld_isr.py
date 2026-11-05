# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Link production ISR placement fragments with ELD when the tools are available."""

import importlib
import os
import shutil
import subprocess
from io import StringIO
from pathlib import Path

import pytest
from elftools.elf.elffile import ELFFile

ROOT = Path(__file__).resolve().parents[3]


@pytest.fixture
def tools():
    clang = os.environ.get("CLANG", shutil.which("clang"))
    eld = os.environ.get("ELD", shutil.which("ld.eld"))
    if clang is None or eld is None:
        pytest.skip("clang and ELD are required for actual linker fixtures")
    return clang, eld


@pytest.fixture
def parser_factory(monkeypatch):
    monkeypatch.syspath_prepend(str(ROOT / "scripts/build"))
    monkeypatch.syspath_prepend(str(ROOT / "scripts/tests/isr_tables"))
    module = importlib.import_module("gen_isr_tables_parser_local")
    fixtures = importlib.import_module("test_isr_local_layout")

    def create(shared, switch):
        config = fixtures.Configuration(shared=shared, switch=switch)
        records = [(17, 0, ".irq.first"), (19, 0, ".irq.single")]
        if shared:
            records.append((17, 0, ".irq.second"))
        return module.gen_isr_parser(fixtures.metadata(module, config, records), config, None)

    return create


def compile_object(clang, directory, source):
    c_file = directory / "entries.c"
    obj = directory / "entries.o"
    c_file.write_text(source)
    subprocess.run(
        [
            clang,
            "--target=aarch64-none-elf",
            "-ffreestanding",
            "-fno-pic",
            "-fno-asynchronous-unwind-tables",
            "-ffunction-sections",
            "-fdata-sections",
            "-c",
            str(c_file),
            "-o",
            str(obj),
        ],
        check=True,
        capture_output=True,
        text=True,
    )
    return obj


@pytest.mark.parametrize(
    "shared,switch", [(False, False), (True, False), (False, True), (True, True)]
)
def test_production_placements_link(tools, tmp_path, parser_factory, shared, switch):
    clang, eld = tools
    parser = parser_factory(shared, switch)
    script = tmp_path / "layout.ld"
    contents = StringIO()
    contents.write("ENTRY(_start)\nSECTIONS { . = 0x10000; .text : { *(.text*) }\n")
    source = (
        "typedef unsigned char uint8_t;\n"
        "#define __aligned(x) __attribute__((aligned(x)))\n"
        "#define __used __attribute__((used))\n#define __noasan\n"
        "#define Z_GENERIC_SECTION_LITERAL(x) __attribute__((section(x)))\n"
        "void _start(void) {}\n"
    )
    for table in parser.tables.values():
        marker = StringIO()
        table.write_alignment_source(marker, writable=False)
        source += marker.getvalue()
        contents.write(f"{table.section} : {{\n")
        table.write_ld(contents)
        contents.write("}\n")
        for item in table.inputs:
            source += (
                f"static const uint8_t {item.symbol}_storage[{item.size}] "
                f"__attribute__((used, aligned({item.alignment}), section(\"{item.name}\")))"
                " = {0};\n"
            )
    contents.write("/DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) } }\n")
    script.write_text(contents.getvalue())
    obj = compile_object(clang, tmp_path, source)
    elf_file = tmp_path / "linked.elf"
    subprocess.run(
        [eld, "--gc-sections", "-T", str(script), str(obj), "-o", str(elf_file)],
        check=True,
        capture_output=True,
        text=True,
    )
    with elf_file.open("rb") as stream:
        elf = ELFFile(stream)
        symbols = {
            symbol.name: symbol["st_value"]
            for symbol in elf.get_section_by_name(".symtab").iter_symbols()
        }
        for table in parser.tables.values():
            assert symbols[table.symbol] % table.alignment == 0
            assert symbols[table.end_symbol] - symbols[table.symbol] == table.size
            for item in table.inputs:
                assert symbols[item.symbol] == symbols[table.symbol] + item.offset
                assert symbols[item.symbol + "_end"] - symbols[item.symbol] == item.size


@pytest.mark.parametrize("invalid", ["missing", "short", "duplicate"])
def test_rejects_bad_input_extent(tools, tmp_path, monkeypatch, invalid):
    monkeypatch.syspath_prepend(str(ROOT / "scripts/build"))
    layout = importlib.import_module("isr_table_layout")
    table = layout.Table(
        "sw_isr_table",
        "_sw_isr_table",
        8,
        16,
        [layout.InputSection(".irq.first", 0, 16, 8, "first")],
    )
    fragment = StringIO()
    table.write_ld(fragment)
    script = tmp_path / "layout.ld"
    script.write_text(
        "ENTRY(_start)\nSECTIONS { . = 0x10000; .text : { *(.text*) }\n"
        "sw_isr_table : {\n" + fragment.getvalue() + "} }\n"
    )
    source = "void _start(void) {}\n"
    if invalid != "missing":
        size = 8 if invalid == "short" else 16
        source += (
            f"const char entry[{size}] __attribute__((used, aligned(8), "
            "section(\".irq.first\"))) = {0};\n"
        )
    if invalid == "duplicate":
        source += (
            'const char duplicate[16] __attribute__((used, aligned(8), '
            'section(".irq.first"))) = {0};\n'
        )
    clang, eld = tools
    obj = compile_object(clang, tmp_path, source)
    result = subprocess.run(
        [eld, "-T", str(script), str(obj), "-o", str(tmp_path / "bad.elf")],
        capture_output=True,
        text=True,
    )
    assert result.returncode != 0
    assert "ISR input size" in result.stderr


def test_info_metadata_keeps_its_relocation_roots(tools, tmp_path):
    clang, eld = tools
    source = (
        "void _start(void) {}\n"
        "static void retained(void) { __asm__ volatile(\"\"); }\n"
        "void (*const metadata[1024])(void) __attribute__((used, section(\".intList\")))"
        " = {retained};\n"
    )
    obj = compile_object(clang, tmp_path, source)
    script = tmp_path / "info.ld"
    script.write_text(
        "ENTRY(_start)\nSECTIONS { . = 0x10000; .text : { *(.text*) }\n"
        ".intList 0 (INFO) : { KEEP(*(.intList)) } }\n"
    )
    elf_file = tmp_path / "info.elf"
    subprocess.run(
        [eld, "--gc-sections", "-T", str(script), str(obj), "-o", str(elf_file)],
        check=True,
        capture_output=True,
        text=True,
    )
    with elf_file.open("rb") as stream:
        elf = ELFFile(stream)
        metadata = elf.get_section_by_name(".intList")
        assert metadata["sh_size"] == 8192
        assert metadata["sh_flags"] & 2 == 0
        retained = [
            sym
            for sym in elf.get_section_by_name(".symtab").iter_symbols()
            if sym.name == "retained"
        ]
        assert len(retained) == 1
        assert retained[0]["st_shndx"] != "SHN_UNDEF"
        section = elf.get_section(retained[0]["st_shndx"])
        assert section["sh_flags"] & 6 == 6
