#!/usr/bin/env python3
# Copyright (c) 2026 Aditya Grewal
# SPDX-License-Identifier: Apache-2.0

"""
Static syscall privilege-elevation stack usage analyzer for Zephyr.

Combines GCC/Clang -fstack-usage (.su) files, generated syscalls.json,
Kconfig settings, and objdump disassembly call graphs to compute the
worst-case stack depth from each z_mrsh_<syscall> entry point.
"""

import argparse
import json
import re
import subprocess
import sys
from collections import defaultdict
from pathlib import Path

# Matches GCC/Clang .su lines:
# path/file.c:42:6:z_mrsh_k_uptime_get\t32\tstatic
SU_LINE_RE = re.compile(r"^(?:.+?:\d+:\d+:)?([^\t]+)\t(\d+)\t(static|dynamic|bounded)$")

# Matches generated syscall_list.h entries:
# #define K_SYSCALL_K_UPTIME_GET 0
SYSCALL_LIST_RE = re.compile(r"^\s*#define\s+K_SYSCALL_([A-Z0-9_]+)\s+\d+")

# Matches disassembly function header:
# 00001234 <z_mrsh_k_uptime_get>:
OBJDUMP_FUNC_RE = re.compile(r"^[0-9a-fA-F]+\s+<([^>]+)>:")

# Matches direct branch/call instructions with target symbol across ARM, x86, RISC-V:
#   1238: f000 f810   bl  125c <z_vrfy_k_uptime_get>
#   1238: e8 1f 00 00 00 call 125c <z_vrfy_k_uptime_get>
#   1238: 024000ef    jal ra,125c <z_vrfy_k_uptime_get>
OBJDUMP_CALL_RE = re.compile(
    r"^\s*[0-9a-fA-F]+:\s+.*?\b(?:bl|blx|call|callq|jal|jalr)\b.*?<([^+>]+)>"
)

# Matches indirect register branch/call instructions (function pointers / vtables):
#   1240: 4798        blx r3
#   1240: ff d0       call *%rax
#   1240: 000780e7    jalr a5
OBJDUMP_INDIRECT_RE = re.compile(
    r"^\s*[0-9a-fA-F]+:\s+.*?\b(?:blx\s+r\d+|callq?\s+\*|jalr\s+(?!ra\b|zero\b)[a-z0-9]+)\b"
)


def parse_dot_config(config_path: Path) -> dict[str, int]:
    """Extract integer Kconfig values from .config."""
    values: dict[str, int] = {}
    if not config_path.is_file():
        return values

    for line in config_path.read_text(encoding="utf-8", errors="replace").splitlines():
        line = line.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, val = line.split("=", 1)
        if val.isdigit():
            values[key] = int(val)
    return values


def load_syscall_names(
    build_dir: Path,
    syscalls_json_path: Path | None,
    stack_frames: dict[str, tuple[int, bool]],
    call_graph: dict[str, set[str]],
) -> list[str]:
    """
    Discover exposed syscall names in priority order:
      1. Explicit --syscalls-json file if provided and present
      2. Generated include/generated/zephyr/syscall_list.h (K_SYSCALL_<NAME>)
      3. z_mrsh_<name> symbols present in .su files or ELF disassembly
    """
    if syscalls_json_path and syscalls_json_path.is_file():
        data = json.loads(syscalls_json_path.read_text(encoding="utf-8"))
        if isinstance(data, dict):
            return sorted(data.keys())

    syscall_list_h = build_dir / "zephyr" / "include" / "generated" / "zephyr" / "syscall_list.h"
    if syscall_list_h.is_file():
        names: set[str] = set()
        for line in syscall_list_h.read_text(encoding="utf-8", errors="replace").splitlines():
            match = SYSCALL_LIST_RE.match(line)
            if match:
                raw = match.group(1)
                if raw != "BAD":
                    names.add(raw.lower())
        if names:
            return sorted(names)

    mrsh_names = {
        sym[len("z_mrsh_") :]
        for sym in (*stack_frames.keys(), *call_graph.keys())
        if sym.startswith("z_mrsh_")
    }
    return sorted(mrsh_names)


def parse_su_files(build_dir: Path) -> dict[str, tuple[int, bool]]:
    """
    Scan build_dir recursively for *.su files.

    Returns mapping: func_name -> (max_frame_bytes, has_dynamic_frame).
    Takes the maximum frame size if multiple static functions share a name.
    """
    stack_frames: dict[str, tuple[int, bool]] = {}

    for su_path in build_dir.rglob("*.su"):
        for raw_line in su_path.read_text(encoding="utf-8", errors="replace").splitlines():
            match = SU_LINE_RE.match(raw_line.strip())
            if not match:
                continue
            func_name, size_str, qualifier = match.groups()
            frame_size = int(size_str)
            is_dynamic = qualifier == "dynamic"

            prev_size, prev_dyn = stack_frames.get(func_name, (0, False))
            stack_frames[func_name] = (
                max(prev_size, frame_size),
                prev_dyn or is_dynamic,
            )

    return stack_frames


def parse_disassembly(elf_path: Path, objdump_bin: str) -> tuple[dict[str, set[str]], set[str]]:
    """
    Disassemble elf_path using objdump and extract:
      - call_graph: caller -> set of direct callee symbol names
      - indirect_callers: set of functions containing indirect calls
    """
    if not elf_path.is_file():
        raise FileNotFoundError(
            f"ELF binary not found at {elf_path}. "
            "Build the image before running syscall_stack_report."
        )

    proc = subprocess.run(
        [objdump_bin, "-d", "--no-show-raw-insn", str(elf_path)],
        capture_output=True,
        text=True,
        check=True,
    )

    call_graph: dict[str, set[str]] = defaultdict(set)
    indirect_callers: set[str] = set()
    current_func: str | None = None

    for line in proc.stdout.splitlines():
        func_match = OBJDUMP_FUNC_RE.match(line)
        if func_match:
            current_func = func_match.group(1)
            continue

        if current_func is None:
            continue

        call_match = OBJDUMP_CALL_RE.match(line)
        if call_match:
            callee = call_match.group(1)
            call_graph[current_func].add(callee)
            continue

        if OBJDUMP_INDIRECT_RE.match(line):
            indirect_callers.add(current_func)

    return call_graph, indirect_callers


def compute_worst_case_stack(
    entry_func: str,
    stack_frames: dict[str, tuple[int, bool]],
    call_graph: dict[str, set[str]],
    indirect_callers: set[str],
    trap_overhead: int,
) -> dict:
    """
    Compute worst-case cumulative stack depth starting at entry_func.
    Detects cycles (recursion), indirect calls, and dynamic stack frames.
    """
    min_limit = max(4096, len(call_graph) + 256)
    if sys.getrecursionlimit() < min_limit:
        sys.setrecursionlimit(min_limit)

    has_recursion = False
    has_indirect = False
    has_dynamic = False
    missing_su: set[str] = set()
    memo: dict[str, tuple[int, list[str]]] = {}

    def dfs(func: str, active_stack: set[str]) -> tuple[int, list[str], bool]:
        nonlocal has_recursion, has_indirect, has_dynamic

        if func in active_stack:
            has_recursion = True
            return 0, [f"{func} [recursive]"], True

        if func in memo:
            cached_total, cached_path = memo[func]
            return cached_total, cached_path, False

        if func in indirect_callers:
            has_indirect = True

        if func in stack_frames:
            own_size, is_dyn = stack_frames[func]
            if is_dyn:
                has_dynamic = True
        else:
            own_size = 0
            missing_su.add(func)

        active_stack.add(func)
        max_child_size = 0
        max_child_path: list[str] = []
        hit_cycle = False

        for callee in sorted(call_graph.get(func, ())):
            child_size, child_path, child_cycle = dfs(callee, active_stack)
            if child_cycle:
                hit_cycle = True
            if child_size > max_child_size or not max_child_path:
                max_child_size = child_size
                max_child_path = child_path

        active_stack.remove(func)

        total = own_size + max_child_size
        path = [func] + max_child_path
        if not hit_cycle:
            memo[func] = (total, path)
        return total, path, hit_cycle

    call_depth_bytes, deepest_path, _ = dfs(entry_func, set())
    total_bytes = trap_overhead + call_depth_bytes

    notes: list[str] = []
    if has_recursion:
        notes.append("recursive")
    if has_indirect:
        notes.append("indirect-call")
    if has_dynamic:
        notes.append("dynamic-frame")
    if missing_su:
        notes.append(f"no-su({len(missing_su)})")

    return {
        "entry": entry_func,
        "stack_bytes": total_bytes,
        "trap_overhead": trap_overhead,
        "call_chain_bytes": call_depth_bytes,
        "notes": notes,
        "deepest_path": deepest_path,
    }


def analyze_syscalls(
    build_dir: Path,
    elf_path: Path,
    syscalls_json: Path | None,
    objdump_bin: str,
    priv_stack_size: int,
    trap_overhead: int,
) -> list[dict]:
    """Run full syscall stack analysis and return sorted results."""
    stack_frames = parse_su_files(build_dir)
    if not stack_frames:
        raise RuntimeError(
            f"No .su files found under {build_dir}. Ensure CONFIG_STACK_USAGE=y is enabled."
        )

    call_graph, indirect_callers = parse_disassembly(elf_path, objdump_bin)
    syscall_names = load_syscall_names(
        build_dir=build_dir,
        syscalls_json_path=syscalls_json,
        stack_frames=stack_frames,
        call_graph=call_graph,
    )

    results = []
    for name in syscall_names:
        mrsh_sym = f"z_mrsh_{name}"
        if mrsh_sym not in stack_frames and mrsh_sym not in call_graph:
            continue

        entry = compute_worst_case_stack(
            entry_func=mrsh_sym,
            stack_frames=stack_frames,
            call_graph=call_graph,
            indirect_callers=indirect_callers,
            trap_overhead=trap_overhead,
        )
        entry["syscall"] = name
        entry["limit_bytes"] = priv_stack_size
        entry["exceeds_limit"] = entry["stack_bytes"] > priv_stack_size
        results.append(entry)

    results.sort(key=lambda r: r["stack_bytes"], reverse=True)
    return results


def print_report(results: list[dict], priv_stack_size: int, trap_overhead: int) -> int:
    """Print formatted report table and return count of outlier syscalls."""
    outliers = [r for r in results if r["exceeds_limit"]]

    print(
        f"\nSyscall Privilege Stack Usage Report "
        f"(limit={priv_stack_size} B, trap_overhead={trap_overhead} B)"
    )
    print("=" * 96)
    print(f"{'Syscall':<34} {'Stack (B)':>10} {'Limit (B)':>10} {'Status':<10} {'Notes'}")
    print("-" * 96)

    for r in results:
        status = "EXCEEDS!" if r["exceeds_limit"] else "OK"
        notes_str = ", ".join(r["notes"]) if r["notes"] else "-"
        print(
            f"{r['syscall']:<34} {r['stack_bytes']:>10} {r['limit_bytes']:>10} "
            f"{status:<10} {notes_str}"
        )

    print("-" * 96)
    print(
        f"Analyzed {len(results)} active syscalls | "
        f"Outliers exceeding {priv_stack_size} B: {len(outliers)}"
    )

    if outliers:
        print("\nOutlier Call Chains:")
        for r in outliers:
            print(f"  * {r['syscall']} ({r['stack_bytes']} B): {' -> '.join(r['deepest_path'])}")

    return len(outliers)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Generate worst-case privilege stack usage report for Zephyr syscalls."
    )
    parser.add_argument(
        "--build-dir",
        type=Path,
        required=True,
        help="Path to Zephyr build directory (containing .su files and zephyr/)",
    )
    parser.add_argument(
        "--elf",
        type=Path,
        default=None,
        help="Path to zephyr.elf (defaults to <build-dir>/zephyr/zephyr.elf)",
    )
    parser.add_argument(
        "--syscalls-json",
        type=Path,
        default=None,
        help="Optional path to syscalls.json (falls back to syscall_list.h and ELF symbols)",
    )
    parser.add_argument(
        "--config",
        type=Path,
        default=None,
        help="Path to .config (defaults to <build-dir>/zephyr/.config)",
    )
    parser.add_argument(
        "--objdump",
        default="objdump",
        help="Target objdump executable (e.g. arm-zephyr-eabi-objdump)",
    )
    parser.add_argument(
        "--priv-stack-size",
        type=int,
        default=None,
        help="Override CONFIG_PRIVILEGED_STACK_SIZE in bytes",
    )
    parser.add_argument(
        "--trap-overhead",
        type=int,
        default=None,
        help="Override CONFIG_SYSCALL_TRAP_STACK_OVERHEAD in bytes",
    )
    parser.add_argument(
        "--json-out",
        type=Path,
        default=None,
        help="Optional output path to write JSON report",
    )
    parser.add_argument(
        "--fail-on-exceed",
        action="store_true",
        help="Return non-zero exit code if any syscall exceeds CONFIG_PRIVILEGED_STACK_SIZE",
    )
    args = parser.parse_args()

    elf_path = args.elf or (args.build_dir / "zephyr" / "zephyr.elf")
    config_path = args.config or (args.build_dir / "zephyr" / ".config")

    kconfig = parse_dot_config(config_path)
    priv_stack_size = (
        args.priv_stack_size
        if args.priv_stack_size is not None
        else kconfig.get("CONFIG_PRIVILEGED_STACK_SIZE", 1024)
    )
    trap_overhead = (
        args.trap_overhead
        if args.trap_overhead is not None
        else kconfig.get("CONFIG_SYSCALL_TRAP_STACK_OVERHEAD", 64)
    )

    results = analyze_syscalls(
        build_dir=args.build_dir,
        elf_path=elf_path,
        syscalls_json=args.syscalls_json,
        objdump_bin=args.objdump,
        priv_stack_size=priv_stack_size,
        trap_overhead=trap_overhead,
    )

    if args.json_out:
        args.json_out.write_text(json.dumps(results, indent=2), encoding="utf-8")

    outlier_count = print_report(results, priv_stack_size, trap_overhead)
    if args.fail_on_exceed and outlier_count > 0:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
