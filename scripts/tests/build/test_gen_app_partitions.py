# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: Apache-2.0

"""Check generated application partitions using actual archive inputs."""

import importlib.util
import shutil
import subprocess
from pathlib import Path

import pytest
from elftools.elf.elffile import ELFFile

SCRIPT = Path(__file__).resolve().parents[3] / "scripts/build/gen_app_partitions.py"


@pytest.fixture
def generator():
    spec = importlib.util.spec_from_file_location("gen_app_partitions", SCRIPT)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


@pytest.mark.parametrize(
    "linker,archive_name",
    [
        ("ld", "libdomain.a"),
        ("ld.lld", "libdomain.a"),
        ("ld", "libdomain-extra.a"),
        ("ld.lld", "libdomain-extra.a"),
        ("ld", "libdomain space.a"),
        ("ld", "libdomain++.a"),
        ("ld.lld", "libdomain++.a"),
    ],
)
def test_archive_partition(generator, tmp_path, linker, archive_name):
    """Move only the selected archive's initialized and zero-initialized data."""
    tools = {name: shutil.which(name) for name in ("cc", "ar", linker)}
    if not all(tools.values()):
        pytest.skip("requires a C compiler, ar, and " + linker)

    source = tmp_path / "globals.c"
    source.write_text(
        "int mapped_data = 42;\n"
        "int mapped_bss;\n"
        'int mapped_sdata __attribute__((section(".sdata.custom"))) = 7;\n'
        'int mapped_sbss __attribute__((section(".sbss.custom")));\n'
    )
    obj = tmp_path / "globals.o"
    subprocess.run(
        [tools["cc"], "-fno-common", "-fdata-sections", "-c", str(source), "-o", str(obj)],
        check=True,
    )
    archive = tmp_path / "nested" / archive_name
    archive.parent.mkdir()
    subprocess.run([tools["ar"], "rcs", str(archive), str(obj)], check=True)

    source.write_text("int other_data = 3;\nint other_bss;\n")
    other = tmp_path / "other.o"
    subprocess.run([tools["cc"], "-fno-common", "-c", str(source), "-o", str(other)], check=True)

    fragment = tmp_path / "app_smem.ld"
    generator.generate_final_linker(
        generator.LDScriptTemlate, fragment, {"domain": {"libraries": [archive.name]}}, ""
    )
    script = tmp_path / "linker.ld.in"
    script.write_text(
        "#define _APP_SMEM_SECTION_NAME .app_smem\n"
        "#define SECTION_PROLOGUE(name, options, alignment) name :\n"
        "#define SMEM_PARTITION_ALIGN(size) . = ALIGN(16)\n"
        "#define APP_SHARED_ALIGN . = ALIGN(16)\n"
        "#define GROUP_DATA_LINK_IN(ram, rom)\n"
        "SECTIONS {\n"
        + fragment.read_text()
        + "\n.data : { *(.data .data.* .sdata .sdata.*) }\n"
        + ".bss : { *(.bss .bss.* .sbss .sbss.*) }\n}\n"
    )
    processed = tmp_path / "linker.ld"
    subprocess.run(
        [tools["cc"], "-E", "-P", "-x", "c", str(script), "-o", str(processed)],
        check=True,
    )
    output = tmp_path / "result.o"
    subprocess.run(
        [
            tools[linker],
            "-r",
            "-T",
            str(processed),
            "--whole-archive",
            str(archive),
            "--no-whole-archive",
            str(other),
            "-o",
            str(output),
        ],
        check=True,
    )

    with output.open("rb") as stream:
        elf = ELFFile(stream)
        symbols = {
            symbol.name: symbol for symbol in elf.get_section_by_name(".symtab").iter_symbols()
        }
        section_index = elf.get_section_index(".app_smem")
        start = symbols["z_data_smem_domain_part_start"]["st_value"]
        end = symbols["z_data_smem_domain_part_end"]["st_value"]
        for name in ("mapped_data", "mapped_bss", "mapped_sdata", "mapped_sbss"):
            symbol = symbols[name]
            assert symbol["st_shndx"] == section_index, name
            assert start <= symbol["st_value"] < end, name
        for name in ("other_data", "other_bss"):
            assert symbols[name]["st_shndx"] != section_index, name
