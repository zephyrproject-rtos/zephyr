#!/usr/bin/env python3
# Copyright (c) 2026 Aditya Grewal
# SPDX-License-Identifier: Apache-2.0

import json
import sys
from pathlib import Path

ZEPHYR_BASE = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ZEPHYR_BASE / "scripts" / "footprint"))

import syscall_stack_report as ssr  # noqa: E402


def test_parse_su_files(tmp_path: Path):
    su_file = tmp_path / "sample.c.su"
    su_file.write_text(
        "foo.c:10:6:z_mrsh_k_uptime_get\t32\tstatic\n"
        "foo.c:20:6:z_vrfy_k_uptime_get\t48\tstatic\n"
        "foo.c:30:6:helper_dyn\t96\tdynamic\n",
        encoding="utf-8",
    )

    frames = ssr.parse_su_files(tmp_path)
    assert frames["z_mrsh_k_uptime_get"] == (32, False)
    assert frames["z_vrfy_k_uptime_get"] == (48, False)
    assert frames["helper_dyn"] == (96, True)


def test_compute_worst_case_stack_and_outlier():
    frames = {
        "z_mrsh_big_call": (64, False),
        "z_vrfy_big_call": (128, False),
        "z_impl_big_call": (900, False),
        "z_impl_small_call": (16, False),
    }
    call_graph = {
        "z_mrsh_big_call": {"z_vrfy_big_call"},
        "z_vrfy_big_call": {"z_impl_big_call", "z_impl_small_call"},
    }

    res = ssr.compute_worst_case_stack(
        entry_func="z_mrsh_big_call",
        stack_frames=frames,
        call_graph=call_graph,
        indirect_callers=set(),
        trap_overhead=64,
    )

    # 64 (trap) + 64 (mrsh) + 128 (vrfy) + 900 (max branch) = 1156
    assert res["stack_bytes"] == 1156
    assert res["deepest_path"] == [
        "z_mrsh_big_call",
        "z_vrfy_big_call",
        "z_impl_big_call",
    ]
    assert res["notes"] == []


def test_cycle_and_indirect_call_detection():
    frames = {
        "z_mrsh_rec": (32, False),
        "func_a": (40, False),
        "func_b": (24, True),
    }
    call_graph = {
        "z_mrsh_rec": {"func_a"},
        "func_a": {"func_b"},
        "func_b": {"func_a"},
    }

    res = ssr.compute_worst_case_stack(
        entry_func="z_mrsh_rec",
        stack_frames=frames,
        call_graph=call_graph,
        indirect_callers={"func_b"},
        trap_overhead=64,
    )

    assert res["stack_bytes"] == 64 + 32 + 40 + 24
    assert "recursive" in res["notes"]
    assert "indirect-call" in res["notes"]
    assert "dynamic-frame" in res["notes"]


def test_analyze_syscalls_end_to_end(tmp_path: Path, monkeypatch):
    (tmp_path / "test.c.su").write_text(
        "a.c:1:1:z_mrsh_ok_call\t32\tstatic\n"
        "a.c:2:1:z_impl_ok_call\t64\tstatic\n"
        "a.c:3:1:z_mrsh_bad_call\t64\tstatic\n"
        "a.c:4:1:z_impl_bad_call\t1000\tstatic\n",
        encoding="utf-8",
    )

    gen_dir = tmp_path / "zephyr" / "include" / "generated" / "zephyr"
    gen_dir.mkdir(parents=True)
    (gen_dir / "syscall_list.h").write_text(
        "#define K_SYSCALL_OK_CALL 0\n"
        "#define K_SYSCALL_BAD_CALL 1\n"
        "#define K_SYSCALL_UNUSED_CALL 2\n"
        "#define K_SYSCALL_BAD 3\n",
        encoding="utf-8",
    )

    fake_graph = {
        "z_mrsh_ok_call": {"z_impl_ok_call"},
        "z_mrsh_bad_call": {"z_impl_bad_call"},
    }
    monkeypatch.setattr(ssr, "parse_disassembly", lambda elf, obj: (fake_graph, set()))

    results = ssr.analyze_syscalls(
        build_dir=tmp_path,
        elf_path=tmp_path / "zephyr.elf",
        syscalls_json=None,
        objdump_bin="objdump",
        priv_stack_size=1024,
        trap_overhead=64,
    )

    assert [r["syscall"] for r in results] == ["bad_call", "ok_call"]
    assert results[0]["exceeds_limit"] is True
    assert results[1]["exceeds_limit"] is False

    # Also verify explicit syscalls.json override
    syscalls_json = tmp_path / "syscalls.json"
    syscalls_json.write_text(json.dumps({"ok_call": {}}), encoding="utf-8")

    json_results = ssr.analyze_syscalls(
        build_dir=tmp_path,
        elf_path=tmp_path / "zephyr.elf",
        syscalls_json=syscalls_json,
        objdump_bin="objdump",
        priv_stack_size=1024,
        trap_overhead=64,
    )
    assert [r["syscall"] for r in json_results] == ["ok_call"]
