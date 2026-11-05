# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Check the actual template macros; GNU preprocessing substitutes for vendor CPP.

These tests validate script generation and dependencies, not a MetaWare linker
or compiler. Native ELD links are exercised separately in test_eld_isr.py.
"""

import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[3]


@pytest.mark.parametrize(
    "backend,define", [("arcmwdt", "__MWDT_LINKER_CMD__"), ("eld", "__ELD_LINKER_CMD__")]
)
def test_template_stages_and_generated_dependencies(tmp_path, monkeypatch, backend, define):
    cmake = os.environ.get("CMAKE", shutil.which("cmake"))
    cc = shutil.which("cc")
    if cmake is None or cc is None:
        pytest.skip("CMake and a C preprocessor are required")
    source = tmp_path / "source"
    source.mkdir()
    cpp = source / "preprocess.py"
    cpp.write_text(
        f"#!{sys.executable}\nimport subprocess, sys\n"
        + f"raise SystemExit(subprocess.run([{cc!r}, "
        + "*[arg for arg in sys.argv[1:] if arg != '-Hnocopyr']]).returncode)\n"
    )
    cpp.chmod(0o755)
    config = source / "autoconf.h"
    config.write_text("#define CONFIG_ISR_TABLES_LOCAL_DECLARATION 1\n")
    manifest = source / "isr_tables_swi.ld"
    # Use the production layout emitter to generate an actual linker fragment.
    monkeypatch.syspath_prepend(str(ROOT / "scripts/build"))
    from isr_table_layout import InputSection, Table

    with manifest.open("w") as stream:
        Table(
            "sw_isr_table", "_sw_isr_table", 8, 16, [InputSection(".irq.first", 0, 16, 8, "entry")]
        ).write_ld(stream)
    template = source / "fixture.ld.S"
    template.write_text(
        f"#ifndef {define}\n#error Wrong backend definition\n#endif\n"
        'SECTIONS { sw_isr_table : {\n#include <zephyr/linker/isr-table-swi.ld>\n} }\n'
    )
    original = (ROOT / f"cmake/linker/{backend}/target.cmake").read_text()
    macro = re.search(r"macro\(configure_linker_script .*?endmacro\(\)", original, re.S)
    assert macro is not None
    (source / "CMakeLists.txt").write_text(
        f'''cmake_minimum_required(VERSION 3.28)
project(template_stages NONE)
set(PROJECT_BINARY_DIR "${{CMAKE_CURRENT_BINARY_DIR}}")
set(CMAKE_C_COMPILER "{cpp}")
set(LINKER_SCRIPT "{template}")
set(AUTOCONF_H "{config}")
function(zephyr_get_include_directories_for_lang lang result)
  set(${{result}} "-I{ROOT}/include" PARENT_SCOPE)
endfunction()
'''
        + macro.group(0)
        + f'''
configure_linker_script(pre0.cmd "" "{manifest}")
configure_linker_script(pre1.cmd "CUSTOM;LINKER_ZEPHYR_PREBUILT" "{manifest}")
configure_linker_script(final.cmd "LINKER_ZEPHYR_FINAL" "{manifest}")
add_custom_target(scripts DEPENDS pre0.cmd pre1.cmd final.cmd)
'''
    )
    build = tmp_path / "build"
    subprocess.run(
        [cmake, "-S", str(source), "-B", str(build), "-G", "Ninja"],
        check=True,
        capture_output=True,
        text=True,
    )
    subprocess.run(
        [cmake, "--build", str(build), "--target", "scripts"],
        check=True,
        capture_output=True,
        text=True,
    )
    assert ".isr_pre0.sw" in (build / "pre0.cmd").read_text()
    for stage in ("pre1", "final"):
        result = (build / f"{stage}.cmd").read_text()
        assert "INCLUDE isr_tables_swi.ld" in result
        assert ".isr_pre0.sw" not in result
    ninja = (build / "build.ninja").read_text()
    assert str(manifest) in ninja
    assert str(config) in ninja

    # A generated fragment is a real build dependency, even if an older stage
    # script exists. Dropping ARGN from the backend must fail this check.
    manifest.unlink()
    result = subprocess.run(
        [cmake, "--build", str(build), "--target", "scripts"], capture_output=True, text=True
    )
    assert result.returncode != 0
    assert manifest.name in result.stdout + result.stderr
