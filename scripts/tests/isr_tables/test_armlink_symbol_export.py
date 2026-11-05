# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Export actual same-link ELF values without changing runtime storage."""

import importlib
from io import BytesIO

import pytest
import test_isr_metadata_finalize as metadata_tests
from elftools.elf.elffile import ELFFile

tools = metadata_tests.tools


@pytest.fixture
def exporter(monkeypatch):
    monkeypatch.syspath_prepend(str(metadata_tests.ROOT / "scripts/build"))
    return importlib.import_module("armlink_symbol_export")


@pytest.fixture
def image(tmp_path, tools):
    image = metadata_tests.build(tmp_path, tools, control="flash0")
    data = image.read_bytes()
    if ELFFile(BytesIO(data))["e_machine"] != "EM_ARM":
        pytest.skip("Use ISR_METADATA_CC and ISR_METADATA_LD with ARM ELF tools")
    return data


def test_same_link_aliases_preserve_image_and_symbol_indices(image, exporter):
    steering = "RESOLVE public_a AS vector_alias\nRESOLVE public_b AS vector_alias\n"
    data, report = exporter.transform(image, steering)
    before, after = ELFFile(BytesIO(image)), ELFFile(BytesIO(data))
    old, new = before.get_section_by_name(".symtab"), after.get_section_by_name(".symtab")
    assert exporter.load_image(before) == exporter.load_image(after)
    assert new.num_symbols() == old.num_symbols() + 2
    assert [symbol.entry for symbol in old.iter_symbols()] == [
        new.get_symbol(index).entry for index in range(old.num_symbols())
    ]
    for name in ("public_a", "public_b"):
        symbol = new.get_symbol_by_name(name)[0]
        assert symbol["st_value"] == 0
        assert symbol["st_shndx"] == "SHN_ABS"
        assert symbol["st_info"]["type"] == "STT_NOTYPE"
        assert symbol["st_size"] == 0
    assert report["program_image_unchanged"]
    assert exporter.transform(data, steering)[0] == data


@pytest.mark.parametrize(
    "steering,error",
    [
        ("RESOLVE public AS no_such_symbol\n", "Missing or ambiguous"),
        ("RESOLVE public AS vector_alias\nRESOLVE public AS argument\n", "Conflicting armlink"),
        ("RESOLVE argument AS vector_alias\n", "Conflicting existing"),
        ("EXPORT argument AS argument\n", "No RESOLVE mappings"),
    ],
)
def test_refuse_inconsistent_exports(image, exporter, steering, error):
    with pytest.raises(ValueError, match=error):
        exporter.transform(image, steering)
