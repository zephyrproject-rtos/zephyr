# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0
"""Reject runtime metadata and missing SoC-owned vector objects in real ELFs."""

import importlib
import struct
from io import BytesIO
from pathlib import Path

import pytest
from elftools.elf.elffile import ELFFile
from test_isr_table_check import CHECKER, check, elf_fixture, manifest_fixture


def metadata_fixture(path, *, flags=0, loaded=False):
    elf_fixture(path)
    data = bytearray(path.read_bytes())
    elf = ELFFile(BytesIO(data))
    section = elf.get_section_by_name(".objects")
    names = elf.get_section(elf.header["e_shstrndx"])
    offset = names["sh_offset"] + names.data().index(b".objects\0")
    data[offset : offset + 8] = b".intList"
    offset = elf.header["e_shoff"] + elf.get_section_index(".objects") * 64 + 8
    struct.pack_into("<Q", data, offset, flags)
    if loaded:
        offset = len(data)
        data.extend(
            struct.pack(
                "<IIQQQQQQ",
                1,
                4,
                section["sh_offset"],
                0,
                0,
                section["sh_size"],
                section["sh_size"],
                1,
            )
        )
        struct.pack_into("<Q", data, 32, offset)
        struct.pack_into("<HH", data, 54, 56, 1)
    path.write_bytes(data)
    return path


@pytest.mark.parametrize(
    "policy,flags,loaded,expected",
    [
        ("absent", 0, False, 1),
        ("nonallocated", 0, False, 0),
        ("nonallocated", CHECKER.SHF_ALLOC, False, 1),
        ("nonallocated", CHECKER.SHF_EXECINSTR, False, 1),
        ("nonallocated", 0, True, 1),
    ],
)
def test_metadata_policy_checks_actual_sections_and_load_bytes(
    tmp_path, policy, flags, loaded, expected
):
    final = metadata_fixture(tmp_path / "image.elf", flags=flags, loaded=loaded)
    manifest = manifest_fixture(tmp_path)
    manifest["metadata_policy"] = policy
    code, report = check(tmp_path, manifest, final)
    assert code == expected, report
    if expected:
        assert any(
            not row["passed"] and row["name"].startswith("metadata.") for row in report["checks"]
        )


def test_nonallocated_metadata_is_checked_in_pre1_too(tmp_path):
    before = metadata_fixture(tmp_path / "pre1.elf", flags=CHECKER.SHF_ALLOC)
    final = metadata_fixture(tmp_path / "final.elf")
    manifest = manifest_fixture(tmp_path)
    manifest["metadata_policy"] = "nonallocated"
    code, report = check(tmp_path, manifest, final, before)
    assert code == 1
    assert any(
        not row["passed"] and row["name"].startswith("prelinked.metadata.")
        for row in report["checks"]
    )


@pytest.mark.parametrize("policy", ["ignored", False, None])
def test_invalid_metadata_policy_is_an_input_error(tmp_path, policy):
    manifest = manifest_fixture(tmp_path)
    manifest["metadata_policy"] = policy
    assert check(tmp_path, manifest)[0] == 2


@pytest.mark.parametrize(
    "symbol,size,writable,expected",
    [
        ("argument", 8, True, 0),
        ("missing_vectors", 8, True, 1),
        ("argument", 16, True, 1),
        ("argument", 8, False, 1),
        ("handler", 4, False, 1),
    ],
)
def test_manual_vector_object_is_required_independently_of_generated_tables(
    tmp_path, symbol, size, writable, expected
):
    manifest = manifest_fixture(tmp_path)
    manifest["manual_vectors"] = [
        {"symbol": symbol, "size": size, "alignment": 4, "writable": writable}
    ]
    code, report = check(tmp_path, manifest)
    assert code == expected, report


def vector_fixture(path, *, bits=64, little=True, kind=0x11, size=16, flags=3):
    elf_fixture(path, bits=bits, little=little)
    data = bytearray(path.read_bytes())
    elf = ELFFile(BytesIO(data))
    symbols = elf.get_section_by_name(".symtab")
    strings = elf.get_section(symbols["sh_link"])
    index, symbol = next(
        (i, s) for i, s in enumerate(symbols.iter_symbols()) if s.name == "_sw_isr_table_end"
    )
    old, new = b"_sw_isr_table_end", b"_irq_vector_table"
    assert len(old) == len(new)
    offset = strings["sh_offset"] + symbol["st_name"]
    data[offset : offset + len(old)] = new
    order = "<" if little else ">"
    fields = (
        (symbol["st_name"], 0x1000, size, kind, 0, 1)
        if bits == 32
        else (symbol["st_name"], kind, 0, 1, 0x1000, size)
    )
    record = struct.pack(order + ("IIIBBH" if bits == 32 else "IBBHQQ"), *fields)
    offset = symbols["sh_offset"] + index * symbols["sh_entsize"]
    data[offset : offset + len(record)] = record
    offset = elf.header["e_shoff"] + elf.header["e_shentsize"] + 8
    struct.pack_into(order + ("I" if bits == 32 else "Q"), data, offset, flags)
    path.write_bytes(data)
    return path


@pytest.fixture
def generator(monkeypatch):
    monkeypatch.syspath_prepend(str(Path(__file__).resolve().parents[2] / "build"))
    return importlib.import_module("gen_isr_tables")


@pytest.mark.parametrize("bits,little", [(32, True), (32, False), (64, True), (64, False)])
def test_generator_records_manual_c_vector_extent(generator, tmp_path, bits, little):
    path = vector_fixture(tmp_path / "image.elf", bits=bits, little=little)
    with path.open("rb") as stream:
        assert generator.get_manual_vector_tables(ELFFile(stream), "") == [
            {
                "symbol": "_irq_vector_table",
                "size": 16,
                "alignment": bits // 8,
                "writable": True,
            }
        ]


@pytest.mark.parametrize("kind,size", [(0x10, 0), (0x12, 16), (0x11, 0)])
def test_generator_does_not_infer_extent_of_assembly_aliases(generator, tmp_path, kind, size):
    path = vector_fixture(tmp_path / "image.elf", kind=kind, size=size)
    with path.open("rb") as stream:
        assert generator.get_manual_vector_tables(ELFFile(stream), "") == []


def test_generator_rejects_nonallocated_manual_vectors(generator, tmp_path):
    path = vector_fixture(tmp_path / "image.elf", flags=0)
    with path.open("rb") as stream, pytest.raises(ValueError, match="not allocated"):
        generator.get_manual_vector_tables(ELFFile(stream), "")
