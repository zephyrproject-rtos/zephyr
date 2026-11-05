# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Check finalization ordering; the licensed linker invocation is a test double."""

import os
import shutil
import subprocess
import sys
from pathlib import Path

import pytest
import test_isr_metadata_finalize as finalizer_tests

tools = finalizer_tests.tools
ROOT = Path(__file__).resolve().parents[3]


def test_finalizer_precedes_generation_and_final_checks(tmp_path, tools):
    cmake = os.environ.get("CMAKE", shutil.which("cmake"))
    if cmake is None:
        pytest.skip("CMake is required")
    version = subprocess.check_output([cmake, "--version"], text=True).split()[2]
    if tuple(map(int, version.split(".")[:2])) < (3, 28):
        pytest.skip("CMake 3.28 or newer is required")
    baseline = tmp_path / "baseline"
    baseline.mkdir()
    original = finalizer_tests.build(baseline, tools)
    source = tmp_path / "source"
    source.mkdir()
    (source / "empty.c").write_text("void empty(void) {}\n")
    (source / "copy_link_result.py").write_text(
        "import shutil, sys\nshutil.copyfile(sys.argv[1], sys.argv[2])\n"
    )
    (source / "consumer.py").write_text(
        "import hashlib, json, pathlib, sys\n"
        "from elftools.elf.elffile import ELFFile\n"
        "image = pathlib.Path(sys.argv[1])\n"
        "report = json.loads(pathlib.Path(str(image) + '.isr-metadata.json').read_text())\n"
        "exports = json.loads(pathlib.Path(str(image) + '.armlink-symbols.json').read_text())\n"
        "assert exports['program_image_unchanged']\n"
        "assert hashlib.sha256(image.read_bytes()).hexdigest() == report['elf_after_sha256']\n"
        "with image.open('rb') as stream:\n"
        " elf = ELFFile(stream)\n"
        " symbols = elf.get_section_by_name('.symtab')\n"
        " target = symbols.get_symbol_by_name('handler')[0]['st_value']\n"
        " assert symbols.get_symbol_by_name('test_handler_export')[0]['st_value'] == target\n"
        " assert exports['exported']['test_handler_export']['value'] == target\n"
        " metadata = elf.get_section_by_name('.intList')\n"
        " assert not metadata['sh_flags'] & 2\n"
        " assert not any(s['p_type'] == 'PT_LOAD' and s.section_in_segment(metadata)\n"
        "                for s in elf.iter_segments())\n"
        "pathlib.Path(sys.argv[2]).write_text('verified finalized input\\n')\n"
    )
    (source / "CMakeLists.txt").write_text(f'''
cmake_minimum_required(VERSION 3.28)
project(finalization_order C)
set(ZEPHYR_BASE "{ROOT}")
set(PYTHON_EXECUTABLE "{sys.executable}")
set(CONFIG_ISR_TABLES_LOCAL_DECLARATION TRUE)
add_library(zephyr_interface INTERFACE)
add_library(arch__arm__core__cortex_m OBJECT empty.c)
add_library(offsets OBJECT empty.c)
add_library(kernel INTERFACE)
add_custom_target(linker)
set_property(TARGET linker PROPERTY ENTRY empty)
set(OFFSETS_LIB offsets)
include("{ROOT}/cmake/linker/armlink/target.cmake")
# Only the unavailable licensed link command is replaced. It copies a real
# compiler/linker-produced executable which still needs production finalization.
set(CMAKE_C_LINK_EXECUTABLE
  "{sys.executable} {source}/copy_link_result.py {original} <TARGET>")
foreach(stage pre0 pre1 final)
  set(script "${{CMAKE_BINARY_DIR}}/${{stage}}.cmd")
  file(WRITE ${{script}} "test-only licensed linker input\\n")
  file(WRITE ${{script}}.steer "RESOLVE test_handler_export AS handler\\n")
  file(WRITE ${{script}}.keep "")
  string(SHA256 script_id "${{script}}")
  set_property(GLOBAL PROPERTY ARMLINK_STEERING_${{script_id}} offsets)
  set_property(GLOBAL PROPERTY ARMLINK_EDIT_${{script_id}} "${{script}}.steer")
  set_property(GLOBAL PROPERTY ARMLINK_KEEP_${{script_id}} "${{script}}.keep")
  add_executable(elf_${{stage}} empty.c)
  toolchain_ld_link_elf(TARGET_ELF elf_${{stage}}
    OUTPUT_MAP "${{CMAKE_BINARY_DIR}}/${{stage}}.map" LINKER_SCRIPT "${{script}}")
  # Match the root build's later append of its linker script dependency.
  set_property(TARGET elf_${{stage}} APPEND PROPERTY LINK_DEPENDS "${{script}}")
endforeach()
add_custom_command(OUTPUT "${{CMAKE_BINARY_DIR}}/generated"
  COMMAND "{sys.executable}" "{source}/consumer.py" $<TARGET_FILE:elf_pre0>
    "${{CMAKE_BINARY_DIR}}/generated"
  DEPENDS elf_pre0 VERBATIM)
add_custom_target(generated_input DEPENDS "${{CMAKE_BINARY_DIR}}/generated")
add_dependencies(elf_pre1 generated_input)
add_dependencies(elf_final elf_pre1)
add_custom_command(TARGET elf_final POST_BUILD
  COMMAND "{sys.executable}" "{source}/consumer.py" $<TARGET_FILE:elf_final>
    "${{CMAKE_BINARY_DIR}}/checked" VERBATIM)
''')
    build = tmp_path / "build"
    configured = subprocess.run(
        [cmake, "-S", str(source), "-B", str(build), "-G", "Ninja"],
        capture_output=True,
        text=True,
    )
    assert configured.returncode == 0, configured.stdout + configured.stderr
    linked = subprocess.run(
        [cmake, "--build", str(build), "--target", "elf_final"],
        capture_output=True,
        text=True,
    )
    assert linked.returncode == 0, linked.stdout + linked.stderr
    assert (build / "generated").read_text() == "verified finalized input\n"
    assert (build / "checked").read_text() == "verified finalized input\n"
    ninja = (build / "build.ninja").read_text()
    finalizer = str(ROOT / "scripts/build/isr_metadata_finalize.py")
    exporter = str(ROOT / "scripts/build/armlink_symbol_export.py")
    for stage in ("pre0", "pre1", "final"):
        rule = next(line for line in ninja.splitlines() if line.startswith(f"build elf_{stage}:"))
        assert finalizer in rule
        assert exporter in rule
        post_build = next(
            line
            for line in ninja.splitlines()
            if line.startswith("  POST_BUILD =") and f"/elf_{stage} " in line
        )
        assert post_build.index(exporter) < post_build.index(finalizer)
    assert ninja.count("--emit_relocs") == 3
