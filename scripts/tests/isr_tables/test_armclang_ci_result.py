# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Test the CI subprocess protocol, without simulating hardware validation."""

import importlib
import os
import sys
from pathlib import Path

import pytest


@pytest.fixture
def runner(monkeypatch):
    monkeypatch.syspath_prepend(str(Path(__file__).resolve().parents[2] / "ci"))
    return importlib.import_module("run_isr_table_armclang")


@pytest.mark.parametrize(
    "exit_code,failed_marker,skips,hang,accepted",
    [
        (0, False, 0, False, True),
        (1, False, 0, False, False),
        (0, True, 0, False, False),
        (0, False, 1, False, False),
        (0, False, 0, True, True),
    ],
)
def test_subprocess_status_contract(
    runner, tmp_path, exit_code, failed_marker, skips, hang, accepted
):
    output = f"pass = 3, fail = 0, skip = {skips}, total = 3\n"
    output += "PROJECT EXECUTION SUCCESSFUL\n"
    if failed_marker:
        output += "PROJECT EXECUTION FAILED\n"
    program = f"import time,sys; print({output!r},flush=True); "
    program += "time.sleep(60)" if hang else f"sys.exit({exit_code})"
    command = [sys.executable, "-c", program]
    if accepted:
        assert runner.run_qemu(command, tmp_path / "protocol.log", os.environ.copy()) == 3
    else:
        with pytest.raises(RuntimeError, match="failed or skipped"):
            runner.run_qemu(command, tmp_path / "protocol.log", os.environ.copy())


def test_no_success_marker_is_not_a_pass(runner, tmp_path):
    with pytest.raises(RuntimeError, match="failed or skipped"):
        runner.run_qemu(
            [sys.executable, "-c", "print('test started')"],
            tmp_path / "protocol.log",
            os.environ.copy(),
        )
