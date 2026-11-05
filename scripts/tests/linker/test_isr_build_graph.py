# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Exercise production ISR CMake dependencies with host links and command spies.

The spies record generator/checker invocations; ELF contents are covered by the
generator tests and kernel tests, rather than emulated here.
"""

import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[3]


@pytest.mark.parametrize(
    "enabled,local,extra_pass,software,vectors",
    [
        (False, False, False, False, False),
        (True, False, False, True, True),
        (True, False, True, True, False),
        (True, True, False, True, True),
        (True, True, False, False, True),
        (True, True, True, True, False),
    ],
    ids=["disabled", "legacy-pre0", "legacy-pre1", "local-array", "local-vector", "local-pre1"],
)
def test_generation_stages_and_incremental_dependencies(
    tmp_path, enabled, local, extra_pass, software, vectors
):
    cmake = os.environ.get("CMAKE", shutil.which("cmake"))
    if cmake is None or shutil.which("ninja") is None or shutil.which("cc") is None:
        pytest.skip("CMake, Ninja and a host C compiler are required")
    source = tmp_path / "source"
    scripts = source / "scripts/build"
    scripts.mkdir(parents=True)
    events = tmp_path / "events.jsonl"
    generator = scripts / "gen_isr_tables.py"
    generator.write_text(
        "import argparse, json\nfrom pathlib import Path\n"
        "p = argparse.ArgumentParser()\n"
        "p.add_argument('--kernel', required=True)\n"
        "p.add_argument('--output-source', required=True)\n"
        "p.add_argument('--linker-output-files', nargs=2, required=True)\n"
        "p.add_argument('--intlist-section', action='append')\n"
        "for name in ('shared-linker-output', 'entries-linker-output', 'cmake-output', "
        "'layout-output'):\n p.add_argument('--' + name)\n"
        "for name in ('sw-isr-table', 'vector-table', 'big-endian', 'debug'):\n"
        " p.add_argument('--' + name, action='store_true')\n"
        "args = vars(p.parse_args())\n"
        "assert Path(args['kernel']).is_file()\n"
        "Path(args['output_source']).write_text('int generated_isr;\\n')\n"
        "for path in args['linker_output_files'] + "
        "[args[k] for k in ('shared_linker_output', 'entries_linker_output', "
        "'cmake_output', 'layout_output') if args[k]]:\n"
        " Path(path).write_text('generated\\n')\n"
        f"with Path({str(events)!r}).open('a') as f:\n"
        " f.write(json.dumps({'generator': args}) + '\\n')\n"
    )
    checker = scripts / "check_isr_tables.py"
    checker.write_text(
        "import argparse, json\nfrom pathlib import Path\n"
        "p = argparse.ArgumentParser()\n"
        "for name in ('elf', 'manifest', 'prelinked', 'report'):\n"
        " p.add_argument('--' + name, required=True)\n"
        "args = vars(p.parse_args())\n"
        "assert all(Path(args[k]).is_file() for k in ('elf', 'manifest', 'prelinked'))\n"
        "Path(args['report']).write_text(json.dumps(args))\n"
        f"with Path({str(events)!r}).open('a') as f:\n"
        " f.write(json.dumps({'checker': args}) + '\\n')\n"
    )
    local_parser = scripts / "gen_isr_tables_parser_local.py"
    legacy_parser = scripts / "gen_isr_tables_parser_carrays.py"
    layout = scripts / "isr_table_layout.py"
    for path in (local_parser, legacy_parser, layout):
        path.write_text("# Dependency input\n")
    main = source / "main.c"
    main.write_text("int main(void) { return 0; }\n")
    (source / "discovery.c").write_text("int discovery_isr;\n")
    prebuilt = "zephyr_pre1" if local or extra_pass else "zephyr_pre0"
    (source / "CMakeLists.txt").write_text(f'''
cmake_minimum_required(VERSION 3.20)
project(isr_build_graph C)
set(ZEPHYR_BASE "{source}")
set(PYTHON_EXECUTABLE "{sys.executable}")
set(CONFIG_GEN_ISR_TABLES {int(enabled)})
set(CONFIG_ISR_TABLES_LOCAL_DECLARATION {int(local)})
set(CONFIG_GEN_SW_ISR_TABLE {int(software)})
set(CONFIG_GEN_IRQ_VECTOR_TABLE {int(vectors)})
set(CONFIG_CMAKE_LINKER_GENERATOR TRUE)
set(CONFIG_BIG_ENDIAN TRUE)
add_library(zephyr_interface INTERFACE)
add_custom_target(zephyr_generated_headers)
function(zephyr_linker_include_generated)
  file(WRITE "${{PROJECT_BINARY_DIR}}/layout-includes" "${{ARGV}}")
endfunction()
if(CONFIG_ISR_TABLES_LOCAL_DECLARATION)
  add_library(isr_tables_discovery OBJECT discovery.c)
endif()
include("{ROOT}/cmake/modules/isr_tables.cmake")
# Register before the input executable exists, as in the kernel build.
zephyr_isr_tables_generate({prebuilt})
add_executable(zephyr_pre0 main.c ${{ISR_TABLES_DISCOVERY_SOURCES}})
if("{prebuilt}" STREQUAL "zephyr_pre1")
  add_executable(zephyr_pre1 main.c ${{ISR_TABLES_GENERATED_SOURCES}})
  add_dependencies(zephyr_pre1 zephyr_pre0)
endif()
get_property(legacy_sources GLOBAL PROPERTY GENERATED_KERNEL_SOURCE_FILES)
add_executable(zephyr_final main.c ${{ISR_TABLES_GENERATED_SOURCES}} ${{legacy_sources}})
add_dependencies(zephyr_final {prebuilt})
zephyr_isr_tables_check(zephyr_final)
''')
    build = tmp_path / "build"

    def run(*args):
        result = subprocess.run([cmake, *args], capture_output=True, text=True)
        assert result.returncode == 0, result.stdout + result.stderr

    def records(kind):
        if not events.exists():
            return []
        rows = [json.loads(line) for line in events.read_text().splitlines()]
        return [row[kind] for row in rows if kind in row]

    def rebuild_after_change(path):
        path.write_text(path.read_text() + "\n")
        run("--build", str(build), "--target", "zephyr_final")

    run("-S", str(source), "-B", str(build), "-G", "Ninja", "-Dlayout=unrelated-cache-value")
    run("--build", str(build), "--target", "zephyr_final")
    generated = records("generator")
    assert len(generated) == int(enabled)
    assert len(records("checker")) == int(local)
    if not enabled:
        assert not (build / "isr_tables.c").exists()
        return

    invocation = generated[0]
    assert Path(invocation["kernel"]).name == ("zephyr_pre0" if local else prebuilt)
    assert invocation["sw_isr_table"] == software
    assert invocation["vector_table"] == vectors
    assert invocation["big_endian"] == (not local)
    assert invocation["intlist_section"] == [".intList", "intList"]
    if local:
        assert Path(records("checker")[0]["prelinked"]).name == "zephyr_pre1"
        assert (build / "layout-includes").read_text() == (
            f"CMAKE;{build}/isr_tables_layout.cmake;PASS;LINKER_ZEPHYR_PREBUILT;LINKER_ZEPHYR_FINAL"
        )
        ninja = (build / "build.ninja").read_text()
        for stage, obj in (
            ("zephyr_pre0", "discovery"),
            ("zephyr_pre1", "generated"),
            ("zephyr_final", "generated"),
        ):
            link = next(line for line in ninja.splitlines() if line.startswith(f"build {stage}:"))
            assert f"CMakeFiles/isr_tables_{obj}.dir/" in link
            other = "generated" if obj == "discovery" else "discovery"
            assert f"CMakeFiles/isr_tables_{other}.dir/" not in link

    run("--build", str(build), "--target", "zephyr_final")
    assert len(records("generator")) == 1
    assert len(records("checker")) == int(local)
    rebuild_after_change(legacy_parser if local else local_parser)
    assert len(records("generator")) == 1
    expected = 1
    for dependency in (local_parser if local else legacy_parser, generator, main):
        rebuild_after_change(dependency)
        expected += 1
        assert len(records("generator")) == expected
    rebuild_after_change(layout)
    expected += int(local)
    assert len(records("generator")) == expected
    checks = len(records("checker"))
    rebuild_after_change(checker)
    assert len(records("generator")) == expected
    assert len(records("checker")) == checks + int(local)
