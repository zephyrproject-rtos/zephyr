#!/usr/bin/env python3

# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Tests for dtdoctor_sca_wrapper.py error detection and analyzer dispatch."""

import subprocess
import sys

import dtdoctor_sca_wrapper
import pytest
from conftest import DTS_DISABLED_FULL, ZEPHYR_BASE, ord_symbol

WRAPPER = ZEPHYR_BASE / "scripts" / "dts" / "dtdoctor_sca_wrapper.py"


def make_fake_run(rc, stdout="", stderr=""):
    """Fake subprocess.run: the compiler call uses capture_output, analyzer calls don't."""
    calls = {"compiler": None, "analyzer": []}

    def fake_run(cmd, **kwargs):
        if kwargs.get("capture_output"):
            calls["compiler"] = cmd
            return subprocess.CompletedProcess(cmd, rc, stdout, stderr)
        calls["analyzer"].append(cmd)
        return subprocess.CompletedProcess(cmd, 0, "", "")

    return fake_run, calls


def run_wrapper(monkeypatch, argv, fake_run):
    monkeypatch.setattr(sys, "argv", ["dtdoctor_sca_wrapper.py", *argv])
    monkeypatch.setattr(subprocess, "run", fake_run)
    return dtdoctor_sca_wrapper.main()


TESTDATA_TOOLCHAIN_ERRORS = [
    (
        "main.c:10:23: error: '__device_dts_ord_7' undeclared here (not in a function); "
        "did you mean 'device_get_binding'?",
        "__device_dts_ord_7",
    ),
    (
        "main.c:12:9: error: '__device_dts_ord_7' undeclared (first use in this function)",
        "__device_dts_ord_7",
    ),
    (
        # gcc built with NLS uses Unicode quotes in UTF-8 locales
        "main.c:10:23: error: ‘__device_dts_ord_7’ undeclared here (not in a function)",
        "__device_dts_ord_7",
    ),
    (
        "main.cpp:10:11: error: '__device_dts_ord_7' was not declared in this scope",
        "__device_dts_ord_7",
    ),
    (
        "main.c:(.text+0x12): undefined reference to `__device_dts_ord_7'",
        "__device_dts_ord_7",
    ),
    (
        "main.c:10:23: error: use of undeclared identifier '__device_dts_ord_7'",
        "__device_dts_ord_7",
    ),
    (
        "ld.lld: error: undefined symbol: __device_dts_ord_7",
        "__device_dts_ord_7",
    ),
]


@pytest.mark.parametrize(
    'stderr_line, expected_symbol',
    TESTDATA_TOOLCHAIN_ERRORS,
    ids=[
        'gcc-file-scope',
        'gcc-function-scope',
        'gcc-utf8-quotes',
        'g++',
        'gnu-ld',
        'clang',
        'lld',
    ],
)
def test_toolchain_regex_detection(monkeypatch, stderr_line, expected_symbol):
    fake_run, calls = make_fake_run(rc=1, stderr=stderr_line + "\n")
    rc = run_wrapper(
        monkeypatch, ["--edt-pickle", "edt.pickle", "--", "cc", "-c", "main.c"], fake_run
    )
    assert rc == 1
    assert calls["compiler"] == ["cc", "-c", "main.c"]
    assert len(calls["analyzer"]) == 1
    cmd = calls["analyzer"][0]
    assert cmd[0] == sys.executable
    assert cmd[1].endswith("dtdoctor_analyzer.py")
    assert cmd[2:] == ["--edt-pickle", "edt.pickle", "--symbol", expected_symbol]


def test_deduplication(monkeypatch):
    stderr = "\n".join(
        [
            "main.c:10:23: error: '__device_dts_ord_7' undeclared here (not in a function)",
            "main.c:(.text+0x12): undefined reference to `__device_dts_ord_7'",
            "main.c:11:5: error: use of undeclared identifier '__device_dts_ord_9'",
        ]
    )
    fake_run, calls = make_fake_run(rc=1, stderr=stderr)
    run_wrapper(monkeypatch, ["--edt-pickle", "edt.pickle", "--", "cc"], fake_run)
    symbols = [cmd[-1] for cmd in calls["analyzer"]]
    assert symbols == ["__device_dts_ord_7", "__device_dts_ord_9"]


def test_success_runs_no_analysis(monkeypatch):
    stderr = "main.c:10:23: error: '__device_dts_ord_7' undeclared here (not in a function)"
    fake_run, calls = make_fake_run(rc=0, stderr=stderr)
    rc = run_wrapper(monkeypatch, ["--edt-pickle", "edt.pickle", "--", "cc"], fake_run)
    assert rc == 0
    assert calls["analyzer"] == []


def test_no_edt_pickle_no_analysis(monkeypatch):
    stderr = "main.c:10:23: error: '__device_dts_ord_7' undeclared here (not in a function)"
    fake_run, calls = make_fake_run(rc=1, stderr=stderr)
    rc = run_wrapper(monkeypatch, ["--", "cc"], fake_run)
    assert rc == 1
    assert calls["analyzer"] == []


def test_compiler_output_replayed(monkeypatch, capsys):
    fake_run, _ = make_fake_run(rc=1, stdout="compiler stdout\n", stderr="compiler stderr\n")
    rc = run_wrapper(monkeypatch, ["--", "cc"], fake_run)
    out, err = capsys.readouterr()
    assert rc == 1
    assert out == "compiler stdout\n"
    assert err == "compiler stderr\n"


def test_rc_passthrough(monkeypatch):
    fake_run, _ = make_fake_run(rc=3)
    assert run_wrapper(monkeypatch, ["--edt-pickle", "edt.pickle", "--", "cc"], fake_run) == 3


def test_rc_passthrough_with_analysis(monkeypatch):
    stderr = "main.c:(.text+0x12): undefined reference to `__device_dts_ord_7'"
    fake_run, calls = make_fake_run(rc=1, stderr=stderr)
    rc = run_wrapper(monkeypatch, ["--edt-pickle", "edt.pickle", "--", "cc"], fake_run)
    assert rc == 1
    assert len(calls["analyzer"]) == 1


def test_no_double_dash_fallback(monkeypatch):
    fake_run, calls = make_fake_run(rc=0)
    run_wrapper(monkeypatch, ["cc", "-c", "main.c"], fake_run)
    assert calls["compiler"] == ["cc", "-c", "main.c"]


def test_no_double_dash_keeps_flag_in_cmd(monkeypatch):
    # Without a '--' separator the whole argv is used as the command, --edt-pickle included
    fake_run, calls = make_fake_run(rc=0)
    run_wrapper(monkeypatch, ["--edt-pickle", "edt.pickle", "cc"], fake_run)
    assert calls["compiler"] == ["--edt-pickle", "edt.pickle", "cc"]


def test_end_to_end_real_processes(make_edt, make_pickle, tmp_path):
    edt, _ = make_edt(DTS_DISABLED_FULL)
    symbol = ord_symbol(edt, "foo_dev")
    fake_cc = tmp_path / "fake_cc.py"
    fake_cc.write_text(
        "import sys\n"
        f"sys.stderr.write(\"main.c: undefined reference to `{symbol}'\\n\")\n"
        "sys.exit(1)\n",
        encoding="utf-8",
    )
    proc = subprocess.run(
        [
            sys.executable,
            str(WRAPPER),
            "--edt-pickle",
            str(make_pickle(edt)),
            "--",
            sys.executable,
            str(fake_cc),
        ],
        capture_output=True,
        text=True,
    )
    assert proc.returncode == 1
    assert "undefined reference" in proc.stderr
    assert "DT Doctor" in proc.stdout
    assert "is disabled in" in proc.stdout
