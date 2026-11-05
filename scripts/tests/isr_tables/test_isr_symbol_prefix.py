# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0
"""Derive C label decoration from the metadata object and preserve C identifiers."""

import importlib
import os
import shlex
import shutil
import struct
import subprocess
from io import BytesIO, StringIO
from pathlib import Path

import pytest
from elftools.elf.elffile import ELFFile
from test_isr_local_layout import Configuration, metadata
from test_isr_table_check import elf_fixture


@pytest.fixture
def modules(monkeypatch):
    monkeypatch.syspath_prepend(str(Path(__file__).resolve().parents[2] / "build"))
    return (
        importlib.import_module("gen_isr_tables"),
        importlib.import_module("gen_isr_tables_parser_local"),
    )


def metadata_elf(path, *, bits=64, little=True, candidates=("",), misplaced=None):
    """Reuse the executable fixture, replacing two aliases with metadata objects."""
    elf_fixture(path, bits=bits, little=little)
    data = bytearray(path.read_bytes())
    image = ELFFile(BytesIO(data))
    names = image.get_section(image.header["e_shstrndx"])
    section_names = names.data()
    start = names["sh_offset"] + section_names.index(b".objects\0")
    data[start : start + len(b".objects")] = b".intList"
    section = image.get_section_by_name(".objects")
    index = image.get_section_index(".objects")
    symbols = image.get_section_by_name(".symtab")
    strings = image.get_section(symbols["sh_link"])
    aliases = ("_sw_isr_table_end", "_sw_isr_table")
    order = "<" if little else ">"
    for alias, prefix in zip(aliases, candidates, strict=False):
        position, symbol = next(
            (i, s) for i, s in enumerate(symbols.iter_symbols()) if s.name == alias
        )
        name = (prefix + "_iheader").encode()
        assert len(name) <= len(alias)
        offset = strings["sh_offset"] + symbol["st_name"]
        data[offset : offset + len(alias)] = name.ljust(len(alias), b"\0")
        symbol_index = image.get_section_index(".text") if misplaced == "section" else index
        value = section["sh_addr"] + (1 if misplaced == "offset" else 0)
        info = 0x12 if misplaced == "type" else 0x11
        fields = (
            (symbol["st_name"], value, 4, info, 0, symbol_index)
            if bits == 32
            else (symbol["st_name"], info, 0, symbol_index, value, 4)
        )
        record = struct.pack(order + ("IIIBBH" if bits == 32 else "IBBHQQ"), *fields)
        offset = symbols["sh_offset"] + position * symbols["sh_entsize"]
        data[offset : offset + len(record)] = record
    path.write_bytes(data)
    return path


@pytest.mark.parametrize("bits,little", [(32, True), (32, False), (64, True), (64, False)])
@pytest.mark.parametrize("prefix", ["", "_"])
def test_prefix_from_real_elf_fields(modules, tmp_path, bits, little, prefix):
    generator, _ = modules
    path = metadata_elf(tmp_path / "metadata.elf", bits=bits, little=little, candidates=(prefix,))
    with path.open("rb") as stream:
        assert generator.get_c_symbol_prefix(ELFFile(stream), [".missing", ".intList"]) == prefix


@pytest.mark.parametrize("misplaced", ["section", "offset", "type"])
def test_lookalike_metadata_object_is_rejected(modules, tmp_path, misplaced):
    generator, _ = modules
    path = metadata_elf(tmp_path / "metadata.elf", candidates=("_",), misplaced=misplaced)
    with path.open("rb") as stream, pytest.raises(ValueError, match="uniquely identify"):
        generator.get_c_symbol_prefix(ELFFile(stream), [".intList"])


@pytest.mark.parametrize("prefixes", [("", "_"), ("_", "_"), ("0",)])
def test_ambiguous_or_invalid_prefix_is_rejected(modules, tmp_path, prefixes):
    generator, _ = modules
    path = metadata_elf(tmp_path / "metadata.elf", candidates=prefixes)
    with path.open("rb") as stream, pytest.raises(ValueError, match="uniquely identify"):
        generator.get_c_symbol_prefix(ELFFile(stream), [".intList"])


def test_missing_metadata_section_is_rejected(modules, tmp_path):
    generator, _ = modules
    path = metadata_elf(tmp_path / "metadata.elf")
    with path.open("rb") as stream, pytest.raises(ValueError, match="Cannot locate ISR metadata"):
        generator.get_c_symbol_prefix(ELFFile(stream), ["missing"])


@pytest.mark.parametrize("leading,prefix", [(False, ""), (True, "_")])
def test_compiler_metadata_label_decoration(modules, tmp_path, leading, prefix):
    generator, _ = modules
    compiler = shlex.split(os.environ.get("CC", ""))
    if not compiler:
        executable = next(
            (
                shutil.which(name)
                for name in ("arm-zephyr-eabi-gcc", "arm-none-eabi-gcc", "aarch64-elf-gcc", "gcc")
                if shutil.which(name)
            ),
            None,
        )
        if executable is None:
            pytest.skip("Set CC to an ELF C compiler to verify compiler label decoration")
        compiler = [executable]
    source = tmp_path / "metadata.c"
    source.write_text(
        'const unsigned _iheader __attribute__((section(".intList"), used)) = 3;\n'
        'const unsigned lookalike_iheader __attribute__((section(".other"))) = 4;\n'
    )
    output = tmp_path / "metadata.o"
    flag = "-fleading-underscore" if leading else "-fno-leading-underscore"
    result = subprocess.run(
        [*compiler, flag, "-c", str(source), "-o", str(output)], text=True, capture_output=True
    )
    if result.returncode and flag in result.stderr:
        pytest.skip(f"Compiler does not support {flag}")
    assert result.returncode == 0, result.stderr
    if output.read_bytes()[:4] != b"\x7fELF":
        pytest.skip("Compiler emits non-ELF objects; synthetic ELF coverage still runs")
    with output.open("rb") as stream:
        assert generator.get_c_symbol_prefix(ELFFile(stream), [".intList"]) == prefix


@pytest.mark.parametrize("pointer_bytes,little", [(4, True), (4, False), (8, True), (8, False)])
@pytest.mark.parametrize("switch", [False, True])
@pytest.mark.parametrize("prefix", ["", "_"])
def test_linker_aliases_are_decorated_but_c_names_are_not(
    modules, pointer_bytes, little, switch, prefix
):
    _, parser_module = modules
    config = Configuration(shared=True, switch=switch, vector=True, little=little)
    config.c_symbol_prefix = prefix
    config.swt_spurious_handler = "z_irq_spurious"
    config.swt_shared_handler = "z_shared_isr"
    config.vt_default_handler = "_isr_wrapper"
    records = [(17, 0, ".irq.first"), (17, 0, ".irq.second"), (18, 1, ".irq.direct")]
    parser = parser_module.gen_isr_parser(
        metadata(parser_module, config, records, pointer_bytes), config, None
    )
    names = {
        "entries" if switch else "swi": "__isr_entries_start" if switch else "_sw_isr_table",
        "shared": "z_shared_sw_isr_table",
        "vt": "_irq_vector_table",
    }
    cmake = StringIO()
    parser.write_cmake(cmake)
    for kind, table in parser.tables.items():
        assert table.symbol == prefix + names[kind]
        script = StringIO()
        table.write_ld(script)
        assert f"{table.symbol} = .;" in script.getvalue()
        assert table.symbol in cmake.getvalue()
        for item in table.inputs:
            assert item.symbol.startswith(prefix + ("z_irq_vector_" if kind == "vt" else "z_isr_"))
            assert item.symbol in script.getvalue() and item.symbol in cmake.getvalue()
    source = StringIO()
    parser.write_source(source)
    text = source.getvalue()
    assert "_Z_ISR_DIRECT_TABLE_DEFAULT(_isr_wrapper," in text
    assert "_Z_ISR_DIRECT_TABLE_DEFAULT(__isr_wrapper," not in text
    assert ".arg = &z_shared_sw_isr_table[1]" in text
    assert ".arg = &_z_shared_sw_isr_table[1]" not in text
    if switch:
        assert "void get_isr_entry(" in text and "void _get_isr_entry(" not in text
        assert "*entry = z_isr_entry_0x1;" in text
        assert "*entry = _z_isr_entry_0x1;" not in text
    else:
        assert "__used __noasan z_isr_default_0 = {" in text
        assert "__used __noasan _z_isr_default_0 = {" not in text
