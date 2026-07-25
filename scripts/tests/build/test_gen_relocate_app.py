# Copyright (c) 2026 The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0

from collections import defaultdict
import importlib.util
from pathlib import Path


SCRIPT = Path(__file__).parents[2] / "build" / "gen_relocate_app.py"
SPEC = importlib.util.spec_from_file_location("gen_relocate_app", SCRIPT)
assert SPEC is not None and SPEC.loader is not None
gen_relocate_app = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(gen_relocate_app)


def test_numeric_alignment_suffix_follows_region_name():
    gen_relocate_app.mpu_align = {}
    sections = defaultdict(list)
    sections[gen_relocate_app.SectionKind.TEXT].append(
        gen_relocate_app.OutputSection("file.c.obj", ".text.function")
    )

    result = gen_relocate_app.assign_to_correct_mem_region(
        "SRAM_FAST_TEXT_32|COPY|NOKEEP", sections
    )

    assert list(result) == ["SRAM_FAST|COPY"]
    assert gen_relocate_app.mpu_align == {"SRAM_FAST": 32}
    assert result["SRAM_FAST|COPY"][gen_relocate_app.SectionKind.TEXT] == [
        gen_relocate_app.OutputSection("file.c.obj", ".text.function", keep=False)
    ]


def test_non_numeric_suffix_remains_part_of_region_name():
    assert gen_relocate_app.split_alignment_suffix("SRAM_FAST") == ("SRAM_FAST", "")


def test_relocation_metadata_is_not_classified_as_code():
    for name in (".rel.text.function", ".rela.text.function"):
        assert gen_relocate_app.SectionKind.for_section_named(name) is None


def test_arm_unwind_metadata_is_not_classified_as_code():
    for name in (".ARM.exidx.text.function", ".ARM.extab.text.function"):
        assert gen_relocate_app.SectionKind.for_section_named(name) is None


def test_regular_code_section_is_still_classified():
    assert (
        gen_relocate_app.SectionKind.for_section_named(".text.function")
        is gen_relocate_app.SectionKind.TEXT
    )
