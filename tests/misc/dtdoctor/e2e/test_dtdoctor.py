# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""
End-to-end DT Doctor checks, run by ctest against a real application build.

The analyzer and the SCA wrapper are exercised as real subprocesses against the
build's edt.pickle. The deliberately-failing translation units use the real
devicetree macros and are compiled with the application's own compile commands
(replayed from compile_commands.json), so the whole chain is real:
gen_defines.py output, <devicetree.h> expansion, toolchain message, wrapper,
analyzer.
"""

import subprocess
import sys
from pathlib import Path

import pytest
from conftest import retarget

ZEPHYR_BASE = Path(__file__).parents[4]
ANALYZER = ZEPHYR_BASE / "scripts" / "dts" / "dtdoctor_analyzer.py"
WRAPPER = ZEPHYR_BASE / "scripts" / "dts" / "dtdoctor_sca_wrapper.py"

HEADER = "#include <zephyr/device.h>\n#include <zephyr/devicetree.h>\n\n"

# DEVICE_DT_GET() on the disabled node fails at compile time: <zephyr/device.h>
# only declares device symbols for status "okay" nodes
BAD_COMPILE_SNIPPETS = [
    "const struct device *bad = DEVICE_DT_GET(DT_NODELABEL(dtdoctor_disabled));\n",
    "const struct device *get_bad(void)\n"
    "{\n"
    "\treturn DEVICE_DT_GET(DT_NODELABEL(dtdoctor_disabled));\n"
    "}\n",
]

# DEVICE_DT_GET() on the enabled, driver-less node compiles (the symbol is
# declared) and fails at link time instead
BAD_LINK_SNIPPET = (
    "const struct device *okdev = DEVICE_DT_GET(DT_NODELABEL(dtdoctor_enabled));\n"
    "int main(void)\n"
    "{\n"
    "\treturn okdev != (const struct device *)0;\n"
    "}\n"
)


def ord_symbol(edt, label):
    return f"__device_dts_ord_{edt.label2node[label].dep_ordinal}"


def run_analyzer(edt_pickle, symbol):
    return subprocess.run(
        [sys.executable, str(ANALYZER), "--edt-pickle", str(edt_pickle), "--symbol", symbol],
        capture_output=True,
        text=True,
    )


def run_wrapper_around(cmd, edt_pickle, cwd=None):
    return subprocess.run(
        [sys.executable, str(WRAPPER), "--edt-pickle", str(edt_pickle), "--", *cmd],
        capture_output=True,
        text=True,
        cwd=cwd,
    )


def test_analyzer_reports_disabled_node(edt, edt_pickle):
    proc = run_analyzer(edt_pickle, ord_symbol(edt, "dtdoctor_disabled"))
    assert proc.returncode == 0
    assert "DT Doctor" in proc.stdout
    assert "is disabled in" in proc.stdout
    assert "dtdoctor-disabled-device" in proc.stdout
    assert "'dtdoctor,dev'" in proc.stdout
    assert "'dtdoctor-dev'" in proc.stdout
    assert "'status' property to 'okay'" in proc.stdout


def test_analyzer_reports_enabled_node_without_driver(edt, edt_pickle):
    proc = run_analyzer(edt_pickle, ord_symbol(edt, "dtdoctor_enabled"))
    assert proc.returncode == 0
    assert "is enabled but no driver" in proc.stdout


@pytest.mark.parametrize('suffix', ['src/main.c', 'src/main.cpp'], ids=['c', 'cpp'])
@pytest.mark.parametrize('snippet', BAD_COMPILE_SNIPPETS, ids=['file-scope', 'function-scope'])
def test_wrapper_diagnoses_compile_error(compile_cmd, edt_pickle, tmp_path, suffix, snippet):
    argv, cwd, source = compile_cmd(suffix)
    bad_src = tmp_path / f"bad{Path(suffix).suffix}"
    bad_src.write_text(HEADER + snippet, encoding="utf-8")

    cmd = retarget(argv, source, bad_src, tmp_path / "bad.obj")
    proc = run_wrapper_around(cmd, edt_pickle, cwd=cwd)
    assert proc.returncode != 0

    # Pin the real toolchain spelling the wrapper regexes exist for: g++ has a
    # C++-only one, while clang++ shares the C message
    if suffix.endswith(".cpp") and "g++" in Path(argv[0]).name:
        assert "was not declared" in proc.stderr
    else:
        assert "undeclared" in proc.stderr

    assert "DT Doctor" in proc.stdout
    assert "is disabled in" in proc.stdout


def test_wrapper_diagnoses_link_error(compile_cmd, edt_pickle, cc, tmp_path):
    argv, cwd, source = compile_cmd('src/main.c')
    bad_src = tmp_path / "bad.c"
    bad_src.write_text(HEADER + BAD_LINK_SNIPPET, encoding="utf-8")

    obj = tmp_path / "bad.obj"
    subprocess.run(retarget(argv, source, bad_src, obj), cwd=cwd, check=True)

    link_cmd = [cc, str(obj), "-nostdlib", "-o", str(tmp_path / "bad.elf")]
    proc = run_wrapper_around(link_cmd, edt_pickle)
    assert proc.returncode != 0
    if "undefined reference" not in proc.stderr and "undefined symbol" not in proc.stderr:
        pytest.skip(f"unsupported linker error format: {proc.stderr[:200]}")
    assert "DT Doctor" in proc.stdout
    assert "is enabled but no driver" in proc.stdout
