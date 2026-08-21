#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors

"""Attribute the zperf loopback run's CPU cost to individual functions.

The ``sample.net.zperf.loopback_icount`` setup runs a deterministic throughput
test under QEMU icount mode and reports one throughput number per transfer.
That says whether the stack got slower, not where the cycles went. This script
answers the second question by running the same guest under a QEMU TCG plugin
that counts executed instructions per translation block, and mapping those
blocks back onto the functions in the build's ELF file.

Under ``-icount shift=N`` one guest instruction is 2^N ns of virtual time, so
throughput is a closed-form function of the instructions spent per payload
byte::

    Mbps = 8000 / (2**shift * ipb)      (at shift=5:  Mbps = 250 / ipb)

A per-function breakdown of ``ipb`` is therefore a decomposition of the
reported throughput rather than a correlated proxy for it, which is what makes
the "instructions per byte" column and the "Mbps if this function were free"
column meaningful.

The plugin is *not* shipped with Zephyr, and cannot be: it is built against a
QEMU header, which makes it a derivative work of QEMU, and QEMU's terms are not
among those this tree carries. Use
:file:`samples/net/zperf/scripts/qemu_plugin_setup.sh` to fetch and build one
outside the repository; :file:`samples/net/zperf/README-loopback-profiling.rst`
sets out the split in full. This script only parses the plugin's textual
report:

    collected <N> entries in the hash table
    pc, tcount, icount, ecount
    0x0000000000100238, 1, 1, 9341782
    ...

where ``icount`` is the number of instructions in the block and ``ecount`` how
often the block ran, so the block contributes ``icount * ecount``
instructions. Any plugin emitting that format works.

Typical usage::

    # One-shot: build, run and profile every transfer, boot cost removed.
    samples/net/zperf/scripts/zperf_profile.py run --base-dir .. \\
        --plugin ../tools/qemu-plugins/lib/libhotblocks.so \\
        --outdir ../build/zprof --save ../build/zprof/profile.json

    # Report again from an already collected run.
    samples/net/zperf/scripts/zperf_profile.py report --base-dir .. \\
        --report ../build/zprof/tcp4.hb.log \\
        --elf ../build/zprof/tcp4/zephyr/zephyr.elf --top 25

    # What changed between two commits.
    samples/net/zperf/scripts/zperf_profile.py diff --base-dir .. \\
        --baseline ../build/zprof-base/profile.json \\
        --current ../build/zprof/profile.json

By default all files read or written must live under the current directory;
pass --base-dir to widen that.
"""

import argparse
import bisect
import json
import os
import re
import shlex
import shutil
import subprocess
import sys
from collections import defaultdict

from elftools.elf.constants import SH_FLAGS
from elftools.elf.elffile import ELFFile

# The labels the selftest reports, i.e. the accepted values of
# CONFIG_ZPERF_LOOPBACK_SELFTEST_ONLY.
TRANSFERS = (
    "udp4",
    "udp4_frag",
    "tcp4",
    "tcp4_nodelay",
    "udp6",
    "udp6_frag",
    "tcp6",
    "tcp6_nodelay",
)

SAMPLE = "samples/net/zperf"
OVERLAYS = "overlay-loopback.conf;overlay-loopback-icount.conf"

# Buckets for instructions that belong to no Zephyr function.
UNKNOWN = "[unknown]"
FIRMWARE = "[firmware]"

# Ordered (regex on the source path, group) rules; first match wins. Functions
# whose source file is unknown fall back to SYMBOL_GROUPS below.
FILE_GROUPS = (
    (r"^drivers/net/loopback\.c$", "driver"),
    (r"^subsys/net/lib/zperf/", "harness"),
    (r"^samples/", "harness"),
    (r"^subsys/net/ip/net_pkt\.c$", "pktbuf"),
    (r"^lib/net_buf/", "pktbuf"),
    (r"^subsys/net/ip/utils\.c$", "checksum"),
    (r"^subsys/net/ip/net_tc\.c$", "tc"),
    (r"^subsys/net/ip/(net_core|net_if)\.c$", "core"),
    (r"^subsys/net/ip/tcp\.c$", "tcp"),
    (r"^subsys/net/ip/udp\.c$", "udp"),
    (r"^subsys/net/ip/(connection|net_context)\.c$", "conn"),
    (r"^subsys/net/ip/(ipv4|ipv6|icmp)", "ip"),
    (r"^subsys/net/lib/sockets/", "sockets"),
    (r"^subsys/net/", "net-other"),
    (r"^(kernel|arch)/", "kernel"),
    # rb.c is the scheduler's and the timeout queue's red-black tree, and the
    # timer driver is charged on every tick, so both are kernel overhead.
    (r"^lib/utils/", "kernel"),
    (r"^drivers/timer/", "kernel"),
    (r"^(lib/libc|lib/os|modules/lib/picolibc|modules/picolibc)", "libc"),
)

# Fallback classification by symbol name, used when DWARF gives no source file
# (assembly, or a stripped build).
SYMBOL_GROUPS = (
    (r"^(calc_chksum|net_calc_chksum)", "checksum"),
    (r"^(net_pkt_|pkt_alloc|net_buf_)", "pktbuf"),
    (r"^tcp_", "tcp"),
    (r"^(zsock_|zvfs_)", "sockets"),
    (r"^net_", "net-other"),
    (r"^(z_|k_|arch_|_interrupt|_exception|sys_clock|rb_|queue_)", "kernel"),
    (r"^(mem|str|__udiv|__umod)", "libc"),
)

GROUP_ORDER = (
    "checksum",
    "pktbuf",
    "tcp",
    "udp",
    "ip",
    "conn",
    "sockets",
    "core",
    "tc",
    "driver",
    "net-other",
    "kernel",
    "libc",
    "harness",
    "other",
)


def validate_path(path: str, base_dir: str, *, for_write: bool) -> str:
    """Resolve *path* and ensure it stays inside *base_dir*.

    All files this script reads or writes are taken from CLI arguments. To
    avoid path-traversal or absolute-path escapes when the script is driven
    with untrusted or faulty arguments, every such path is resolved (following
    symlinks and ``..``) and rejected unless it lives under the resolved base
    directory. The validated absolute path is returned for use with ``open()``.
    """
    if not path or "\x00" in path:
        sys.exit(f"Invalid path: {path!r}")

    base_real = os.path.realpath(base_dir)
    if not os.path.isdir(base_real):
        sys.exit(f"Base directory '{base_dir}' does not exist.")

    # Resolve the path as the user means it (relative paths against the current
    # directory, absolute paths as-is, following symlinks and ".."), then
    # require the result to stay under the resolved base directory.
    target_real = os.path.realpath(path)

    if os.path.commonpath([base_real, target_real]) != base_real:
        sys.exit(
            f"Refusing to access '{path}': resolved path '{target_real}' is "
            f"outside the permitted base directory '{base_real}'. Use "
            f"--base-dir to widen the allowed location."
        )

    if for_write:
        parent = os.path.dirname(target_real)
        if not os.path.isdir(parent):
            sys.exit(f"Cannot write '{path}': '{parent}' is not a directory.")
        if os.path.isdir(target_real):
            sys.exit(f"Cannot write '{path}': it is a directory.")

    return target_real


def validate_qemu_arg_path(path: str, base_dir: str, *, for_write: bool) -> str:
    """Like validate_path(), for a path that ends up on the QEMU command line.

    CMake splits QEMU_EXTRA_FLAGS with separate_arguments(), and the plugin
    argument syntax is itself comma separated, so a path containing whitespace
    or a comma would be silently torn apart into several arguments.
    """
    resolved = validate_path(path, base_dir, for_write=for_write)
    bad = [c for c in (" ", "\t", ",") if c in resolved]
    if bad:
        sys.exit(
            f"Refusing to use '{resolved}' on the QEMU command line: it "
            f"contains {bad!r}, which QEMU argument splitting would break."
        )
    return resolved


class SymbolTable:
    """Interval lookup from a guest PC to the function containing it."""

    def __init__(self, elf_path: str):
        self._fp = open(elf_path, "rb")  # noqa: SIM115 - kept open for DWARF
        elf = ELFFile(self._fp)

        symtab = elf.get_section_by_name(".symtab")
        if symtab is None:
            sys.exit(f"{elf_path} has no .symtab; a stripped build cannot be profiled.")

        exec_sections = {
            idx
            for idx, sec in enumerate(elf.iter_sections())
            if sec["sh_flags"] & SH_FLAGS.SHF_EXECINSTR
        }

        # Two views of the symbol table.
        #
        # "ranges" holds sized STT_FUNC symbols. Those are authoritative: a PC
        # inside one belongs to that function, full stop.
        #
        # "bounds" holds entry points with no usable size, which have to be
        # delimited by whatever symbol comes next. Zero-sized STT_FUNC covers
        # hand written assembly such as _interrupt_enter and arch_swap, which
        # is exactly where a network benchmark spends interrupt and context
        # switch time. Global STT_NOTYPE covers assembly entry points that were
        # never given a .type directive - picolibc's memcpy is one, and without
        # it every byte memcpy moves is misattributed to whichever symbol
        # precedes it. Local NOTYPE symbols are excluded: they are internal
        # labels inside a function, and treating them as boundaries would split
        # that function's cost in two.
        ranges: dict[int, tuple[int, int, str]] = {}
        bounds: dict[int, tuple[int, str]] = {}

        for sym in symtab.iter_symbols():
            entry = sym.entry
            shndx = entry.st_shndx
            if not isinstance(shndx, int) or shndx not in exec_sections:
                continue

            kind = entry.st_info.type
            binding = entry.st_info.bind
            addr = entry.st_value
            size = entry.st_size

            if kind == "STT_FUNC" and size:
                rank = 1 if binding == "STB_GLOBAL" else 0
                if addr not in ranges or rank > ranges[addr][0]:
                    ranges[addr] = (rank, size, sym.name)
            elif kind == "STT_FUNC" or (
                kind == "STT_NOTYPE" and binding in ("STB_GLOBAL", "STB_WEAK")
            ):
                rank = 1 if kind == "STT_FUNC" else 0
                if addr not in bounds or rank > bounds[addr][0]:
                    bounds[addr] = (rank, sym.name)

        self._ranges = sorted((a, v[1], v[2]) for a, v in ranges.items())
        self._range_starts = [r[0] for r in self._ranges]

        # Every sized function also acts as a boundary for the unsized ones.
        for addr, (_rank, size, name) in ranges.items():
            bounds.setdefault(addr, (1, name))
            bounds.setdefault(addr + size, (1, UNKNOWN))
        self._bounds = sorted((a, v[1]) for a, v in bounds.items())
        self._bound_starts = [b[0] for b in self._bounds]

        if not self._ranges and not self._bounds:
            sys.exit(f"{elf_path} contains no function symbols.")

        # Loadable ranges, used to tell "not a Zephyr function" apart from
        # "not Zephyr at all" (the BIOS the guest boots through).
        self._load = [
            (seg["p_vaddr"], seg["p_vaddr"] + seg["p_memsz"])
            for seg in elf.iter_segments()
            if seg["p_type"] == "PT_LOAD" and seg["p_memsz"]
        ]

        self._dwarf = elf.get_dwarf_info() if elf.has_dwarf_info() else None
        self._aranges = None
        if self._dwarf is not None:
            try:
                self._aranges = self._dwarf.get_aranges()
            except Exception:  # noqa: BLE001 - DWARF is best effort
                self._aranges = None
        self._file_cache: dict[str, str | None] = {}
        self._sym_cache: dict[int, tuple[str, str]] = {}

    def in_image(self, pc: int) -> bool:
        return any(lo <= pc < hi for lo, hi in self._load)

    def lookup(self, pc: int) -> tuple[str, str]:
        """Return (function name, kind) where kind is exact/inferred/bucket."""
        hit = self._sym_cache.get(pc)
        if hit is not None:
            return hit

        result = self._lookup_uncached(pc)
        self._sym_cache[pc] = result
        return result

    def _lookup_uncached(self, pc: int) -> tuple[str, str]:
        if not self.in_image(pc):
            return FIRMWARE, "bucket"

        # A sized function wins outright.
        idx = bisect.bisect_right(self._range_starts, pc) - 1
        if idx >= 0:
            addr, size, name = self._ranges[idx]
            if pc < addr + size:
                return name, "exact"

        # Otherwise fall back to the nearest preceding entry point.
        idx = bisect.bisect_right(self._bound_starts, pc) - 1
        if idx >= 0:
            name = self._bounds[idx][1]
            if name != UNKNOWN:
                return name, "inferred"

        return UNKNOWN, "bucket"

    def source_file(self, pc: int, zephyr_base: str, topdir: str) -> str | None:
        """Best-effort source file for *pc*, relative to the west topdir."""
        if self._aranges is None:
            return None

        try:
            cu_offset = self._aranges.cu_offset_at_addr(pc)
            if cu_offset is None:
                return None
            cu = self._dwarf.get_CU_at(cu_offset)
            top = cu.get_top_DIE()
            name = top.attributes["DW_AT_name"].value.decode()
            comp_dir = top.attributes.get("DW_AT_comp_dir")
            if not os.path.isabs(name) and comp_dir is not None:
                name = os.path.join(comp_dir.value.decode(), name)
        except Exception:  # noqa: BLE001 - DWARF is best effort
            return None

        return _relativise(os.path.normpath(name), zephyr_base, topdir)


def _relativise(path: str, zephyr_base: str, topdir: str) -> str:
    """Trim an absolute source path down to a repo relative one."""
    for root in (zephyr_base, topdir):
        if root and path.startswith(root.rstrip("/") + "/"):
            return path[len(root.rstrip("/")) + 1 :]
    return path


def classify(func: str, source: str | None) -> str:
    if source is not None:
        for pattern, group in FILE_GROUPS:
            if re.search(pattern, source):
                return group
    for pattern, group in SYMBOL_GROUPS:
        if re.match(pattern, func):
            return group
    return "other"


def parse_report(path: str) -> tuple[dict[tuple[int, int], int], int]:
    """Parse a plugin report into {(pc, insns_per_block): exec_count}.

    Also returns the block count from the header line, so a truncated report
    (released QEMU versions cap contrib/plugins/hotblocks.c output at the 20
    hottest blocks) can be detected instead of being silently believed.
    """
    blocks: dict[tuple[int, int], int] = {}
    collected = -1
    seen_header = False

    with open(path) as fp:
        for line in fp:
            line = line.strip()
            if not line:
                continue
            match = re.match(r"collected (\d+) entries", line)
            if match:
                collected = int(match.group(1))
                continue
            if line.startswith("pc,"):
                seen_header = True
                continue
            if not line.startswith("0x"):
                continue
            # Plugins share the report stream, so skip anything that is not a
            # 4 column data row (the stop plugin prints "0x... reached, exiting").
            fields = [f.strip() for f in line.split(",")]
            if len(fields) != 4:
                continue
            pc, _tcount, insns, ecount = fields
            blocks[(int(pc, 16), int(insns))] = blocks.get((int(pc, 16), int(insns)), 0) + int(
                ecount
            )

    if not seen_header or not blocks:
        sys.exit(
            f"{path} does not look like a QEMU plugin block report. Was the "
            f"plugin loaded, and was '-d plugin' passed? Without '-d plugin' "
            f"QEMU discards the report."
        )

    if collected >= 0 and len(blocks) < collected - 1:
        print(
            f"warning: {os.path.basename(path)}: the plugin reported "
            f"{collected} blocks but emitted only {len(blocks)} rows. The "
            f"profile is truncated to the hottest blocks and every total "
            f"below understates the real cost. Rebuild the plugin with an "
            f"unlimited output limit (see qemu_plugin_setup.sh).",
            file=sys.stderr,
        )

    return blocks, collected


def subtract(full: dict[tuple[int, int], int], boot: dict[tuple[int, int], int]) -> dict:
    """Remove the boot-only cost from a transfer run, block by block.

    The guest is deterministic under icount and the instruction stream before
    the first transfer is identical in both runs, so this is exact rather than
    approximate. A block that ran *fewer* times in the longer run means the two
    runs diverged and the result must not be trusted.
    """
    out = dict(full)
    bad = []
    for key, count in boot.items():
        have = out.get(key, 0)
        if have < count:
            bad.append((key, have, count))
            continue
        remaining = have - count
        if remaining:
            out[key] = remaining
        else:
            out.pop(key, None)

    if bad:
        detail = ", ".join(f"0x{pc:x}(+{n}): {h} < {c}" for (pc, n), h, c in bad[:5])
        sys.exit(
            f"Boot baseline subtraction failed for {len(bad)} block(s) "
            f"[{detail}]: the boot-only run executed blocks more often than "
            f"the transfer run, so the two runs did not share a boot prefix. "
            f"Rebuild both from the same tree and rerun."
        )

    return out


def attribute(
    blocks: dict[tuple[int, int], int], table: SymbolTable, zephyr_base: str, topdir: str
) -> tuple[dict[str, dict], int, dict[str, int]]:
    """Fold blocks into per-function instruction totals."""
    funcs: dict[str, dict] = {}
    total = 0
    kinds: dict[str, int] = defaultdict(int)

    for (pc, insns), ecount in blocks.items():
        cost = insns * ecount
        total += cost
        name, kind = table.lookup(pc)
        kinds[kind] += cost

        entry = funcs.get(name)
        if entry is None:
            source = table.source_file(pc, zephyr_base, topdir) if kind != "bucket" else None
            entry = {"insns": 0, "file": source, "group": classify(name, source)}
            funcs[name] = entry
        entry["insns"] += cost

    return funcs, total, dict(kinds)


def render(profile: dict, top: int) -> None:
    """Print the header, the per-function table and the group roll-up."""
    label = profile["label"]
    total = profile["total_insns"]
    byts = profile.get("bytes") or 0
    shift = profile.get("icount_shift", 5)
    measured = profile.get("measured_mbps")

    ipb = (total / byts) if byts else 0.0
    # Mbps = 8 bits/byte * 1e9 ns/s / (2**shift ns/insn * ipb insn/byte) / 1e6
    reconstructed = (8000.0 / (2**shift * ipb)) if ipb else 0.0

    print(f"\n=== {label} ({profile.get('platform', '?')}) ===")
    line = f"instructions {total:>15,}   payload {byts:>12,} B   ipb {ipb:8.3f}"
    if profile.get("boot_insns"):
        line += f"   boot subtracted {profile['boot_insns']:,}"
    print(line)

    if measured:
        # With icount sleep=off, halted time advances virtual time without
        # executing instructions, so this ratio is the CPU-bound fraction of
        # the run. Below 1 means the guest waited (TCP on ACKs, for example);
        # above 1 means the accounting is wrong.
        ratio = reconstructed / measured if measured else 0.0
        print(
            f"throughput   measured {measured:8.3f} Mbps   "
            f"from profile {reconstructed:8.3f} Mbps   cpu-bound {ratio * 100:5.1f}%"
        )

    kinds = profile.get("kinds") or {}
    if total:
        bucketed = kinds.get("bucket", 0)
        inferred = kinds.get("inferred", 0)
        print(
            f"attribution  exact {100 * kinds.get('exact', 0) / total:5.2f}%   "
            f"inferred {100 * inferred / total:5.2f}%   "
            f"unattributed {100 * bucketed / total:5.2f}%"
        )

    funcs = profile["functions"]
    ranked = sorted(funcs.items(), key=lambda kv: -kv[1]["insns"])

    print(
        f"\n{'#':>3}  {'function':<34}{'group':<10}{'instructions':>14}"
        f"{'ipb':>8}{'%':>7}{'cum%':>7}{'+Mbps':>8}  file"
    )
    cum = 0
    for rank, (name, info) in enumerate(ranked[:top], start=1):
        insns = info["insns"]
        cum += insns
        share = 100.0 * insns / total if total else 0.0
        f_ipb = insns / byts if byts else 0.0
        # What the throughput would become if this function cost nothing.
        gain = ((8000.0 / (2**shift * (ipb - f_ipb))) - reconstructed) if ipb > f_ipb else 0.0
        print(
            f"{rank:>3}  {name[:33]:<34}{info['group']:<10}{insns:>14,}"
            f"{f_ipb:>8.3f}{share:>6.2f}%{100 * cum / total if total else 0:>6.1f}%"
            f"{gain:>+8.2f}  {info['file'] or ''}"
        )

    groups: dict[str, int] = defaultdict(int)
    for info in funcs.values():
        groups[info["group"]] += info["insns"]

    print(f"\n{'group':<12}{'instructions':>14}{'ipb':>9}{'%':>8}")
    ordered = sorted(
        groups.items(), key=lambda kv: (GROUP_ORDER.index(kv[0]) if kv[0] in GROUP_ORDER else 99)
    )
    for group, insns in ordered:
        print(
            f"{group:<12}{insns:>14,}{insns / byts if byts else 0:>9.3f}"
            f"{100.0 * insns / total if total else 0:>7.2f}%"
        )


def render_diff(baseline: dict, current: dict, tolerance_pct: float, top: int) -> bool:
    """Print per-function and per-group deltas; return True if within tolerance."""
    ok = True
    for label in sorted(set(baseline) | set(current)):
        base = baseline.get(label)
        cur = current.get(label)
        if base is None or cur is None:
            print(
                f"\n=== {label} ===\n  only present in {'current' if base is None else 'baseline'}"
            )
            ok = False
            continue

        b_bytes = base.get("bytes") or 0
        c_bytes = cur.get("bytes") or 0
        b_ipb = base["total_insns"] / b_bytes if b_bytes else 0.0
        c_ipb = cur["total_insns"] / c_bytes if c_bytes else 0.0
        change = ((c_ipb - b_ipb) / b_ipb * 100.0) if b_ipb else 0.0
        # More instructions per byte is slower, so a positive change is bad.
        status = "OK" if change <= tolerance_pct else "REGRESSION"
        if status != "OK":
            ok = False

        print(f"\n=== {label} ===")
        print(f"ipb {b_ipb:.3f} -> {c_ipb:.3f}  ({change:+.2f}%)  {status}")

        deltas: dict[str, int] = defaultdict(int)
        for name, info in base["functions"].items():
            deltas[name] -= info["insns"]
        for name, info in cur["functions"].items():
            deltas[name] += info["insns"]

        moved = sorted(deltas.items(), key=lambda kv: -abs(kv[1]))[:top]
        moved = [(n, d) for n, d in moved if d]
        if moved:
            print(f"  {'function':<36}{'delta insns':>14}  note")
            for name, delta in moved:
                note = ""
                if name not in base["functions"]:
                    note = "NEW"
                elif name not in cur["functions"]:
                    note = "GONE"
                print(f"  {name[:35]:<36}{delta:>+14,}  {note}")

    return ok


def _group_totals(profile: dict) -> dict[str, int]:
    groups: dict[str, int] = defaultdict(int)
    for info in profile["functions"].values():
        groups[info["group"]] += info["insns"]
    return dict(groups)


def qemu_command(build_dir: str) -> list[str]:
    """Recover the QEMU command line the build system would use.

    Taking it from the build keeps the board flags, the icount settings and
    the QEMU binary owned by the build system, and lets one build serve several
    runs with different plugin arguments. QEMU_EXTRA_FLAGS cannot do the
    latter: cmake/emu/qemu.cmake consumes it at configure time, so it is baked
    into the run target.
    """
    ninja = shutil.which("ninja")
    if ninja is None:
        sys.exit("ninja not found in PATH; it is needed to recover the QEMU command line.")

    out = (
        subprocess.run(
            [ninja, "-C", build_dir, "-t", "commands", "zephyr/run_qemu"],
            capture_output=True,
            text=True,
            check=True,
        )
        .stdout.strip()
        .splitlines()
    )
    if not out:
        sys.exit(f"No run_qemu target in {build_dir}; is it a QEMU board build?")

    # The recipe is "cd <dir> && <qemu> <args...>", written as the shell would
    # run it, so a build path containing a space arrives quoted. Split it the
    # same way the shell would: shlex keeps such a path in one token and strips
    # the quotes, which is what execve() needs, and it leaves "&&" as a plain
    # token because it parses words rather than shell grammar.
    words = shlex.split(out[-1])
    argv = words[words.index("&&") + 1 :] if "&&" in words else words
    if not argv or "qemu-system" not in argv[0]:
        sys.exit(f"Unexpected run_qemu recipe in {build_dir}: {out[-1]!r}")

    # Some boards do not boot zephyr.elf directly. qemu_x86_64 splits the image
    # into zephyr-qemu-locore.elf and zephyr-qemu-main.elf, which are produced
    # by a target the run target depends on. Running QEMU ourselves skips that
    # dependency, so build it when a referenced image is missing.
    missing = [path for path in _kernel_images(argv) if not os.path.exists(path)]
    if missing:
        subprocess.run([ninja, "-C", build_dir, "qemu_kernel_target"], check=True)
        still = [path for path in missing if not os.path.exists(path)]
        if still:
            sys.exit(f"Still missing after building qemu_kernel_target: {', '.join(still)}")

    # Replace the interactive console with a log file: the profiling run is
    # unattended and must not inherit a terminal.
    cleaned: list[str] = []
    skip_next = False
    for arg in argv:
        if skip_next:
            skip_next = False
            continue
        if arg in ("-chardev", "-serial", "-mon", "-monitor", "-pidfile"):
            skip_next = True
            continue
        if arg == "-nographic":
            continue
        cleaned.append(arg)

    return cleaned


def _kernel_images(argv: list[str]) -> list[str]:
    """Image files the QEMU command line loads into the guest."""
    images = []
    for i, arg in enumerate(argv):
        if arg == "-kernel" and i + 1 < len(argv):
            images.append(argv[i + 1])
        elif arg.startswith("loader,"):
            for field in arg.split(","):
                if field.startswith("file="):
                    images.append(field[len("file=") :])
    return images


def run_one(
    build_dir: str,
    plugin: str,
    report_path: str,
    console_path: str,
    stop_plugin: str | None = None,
    stop_addr: int | None = None,
) -> None:
    argv = qemu_command(build_dir)
    argv += [
        "-serial",
        f"file:{console_path}",
        "-display",
        "none",
        "-monitor",
        "none",
        "-plugin",
        f"file={plugin},inline=on",
    ]
    if stop_plugin is not None and stop_addr is not None:
        # Stop the guest the moment it reaches the address, so this run covers
        # only the prefix shared with the full run.
        argv += ["-plugin", f"file={stop_plugin},addr=0x{stop_addr:x}"]
    argv += [
        # qemu_plugin_outs() goes through qemu_log_mask(CPU_LOG_PLUGIN, ...),
        # so without -d plugin the report is written nowhere.
        "-d",
        "plugin",
        "-D",
        report_path,
    ]

    for path in (report_path, console_path):
        if os.path.exists(path):
            os.unlink(path)

    # The guest halts itself through isa-debug-exit, so QEMU's exit status is
    # the guest's halt reason, not a failure indication.
    subprocess.run(argv, cwd=build_dir, check=False, timeout=1800)

    if not os.path.exists(report_path) or os.path.getsize(report_path) == 0:
        sys.exit(
            f"No plugin report at {report_path}. The guest most likely never "
            f"exited, so QEMU's atexit handler never ran: build with "
            f"CONFIG_ZPERF_LOOPBACK_SELFTEST_HALT_ON_DONE=y."
        )


def symbol_address(elf_path: str, name: str) -> int:
    """Address of a defined function symbol, used as the boot/run boundary."""
    with open(elf_path, "rb") as fp:
        symtab = ELFFile(fp).get_section_by_name(".symtab")
        if symtab is None:
            sys.exit(f"{elf_path} has no .symtab; a stripped build cannot be profiled.")

        for sym in symtab.iter_symbols():
            if (
                sym.name == name
                and sym.entry.st_info.type == "STT_FUNC"
                and sym.entry.st_shndx not in ("SHN_UNDEF", "SHN_ABS")
            ):
                return sym.entry.st_value
    sys.exit(f"No function symbol '{name}' in {elf_path}.")


def build_one(zephyr_base: str, board: str, build_dir: str, only: str) -> None:
    west = shutil.which("west")
    if west is None:
        sys.exit("west not found in PATH; activate the workspace virtualenv first.")

    argv = [
        west,
        "build",
        "-p",
        "-b",
        board,
        "-d",
        build_dir,
        os.path.join(zephyr_base, SAMPLE),
        "--",
        f"-DEXTRA_CONF_FILE={OVERLAYS}",
        # A Kconfig string fragment carries its own quotes; they belong inside
        # the value, not around the -D argument.
        f'-DCONFIG_ZPERF_LOOPBACK_SELFTEST_ONLY="{only}"',
        "-DCONFIG_ZPERF_LOOPBACK_SELFTEST_HALT_ON_DONE=y",
    ]
    subprocess.run(argv, check=True, cwd=zephyr_base)


def parse_console(path: str) -> tuple[dict[str, float], dict[str, int]]:
    """Extract the ZPERF-RESULT throughputs and ZPERF-INFO byte counts."""
    mbps: dict[str, float] = {}
    byts: dict[str, int] = {}
    if not os.path.exists(path):
        return mbps, byts

    with open(path, errors="replace") as fp:
        text = fp.read()
    for label, value in re.findall(r"ZPERF-RESULT (\w+)_mbps=([0-9.]+)", text):
        mbps[label] = float(value)
    for label, value in re.findall(r"ZPERF-INFO (\w+)_bytes=(\d+)", text):
        byts[label] = int(value)
    return mbps, byts


def build_profile(
    label: str,
    report: str,
    elf: str,
    boot_report: str | None,
    console: str | None,
    platform: str,
    shift: int,
    zephyr_base: str,
    topdir: str,
) -> dict:
    blocks, _ = parse_report(report)
    boot_insns = 0
    if boot_report:
        boot_blocks, _ = parse_report(boot_report)
        boot_insns = sum(n * c for (_, n), c in boot_blocks.items())
        blocks = subtract(blocks, boot_blocks)

    table = SymbolTable(elf)
    funcs, total, kinds = attribute(blocks, table, zephyr_base, topdir)

    mbps, byts = parse_console(console) if console else ({}, {})

    return {
        "schema": 1,
        "label": label,
        "platform": platform,
        "icount_shift": shift,
        "bytes": byts.get(label, 0),
        "measured_mbps": mbps.get(label),
        "total_insns": total,
        "boot_insns": boot_insns,
        "kinds": kinds,
        "functions": funcs,
    }


def cmd_run(args: argparse.Namespace) -> int:
    zephyr_base = os.environ.get("ZEPHYR_BASE") or os.getcwd()
    topdir = os.path.realpath(os.path.join(zephyr_base, ".."))
    plugin = validate_qemu_arg_path(args.plugin, args.base_dir, for_write=False)
    outdir = validate_path(args.outdir, args.base_dir, for_write=False)
    os.makedirs(outdir, exist_ok=True)

    stop_plugin = None
    if not args.no_subtract_boot:
        stop = args.stop_plugin or os.path.join(os.path.dirname(plugin), "libstoptrigger.so")
        stop_plugin = validate_qemu_arg_path(stop, args.base_dir, for_write=False)
        if not os.path.exists(stop_plugin):
            sys.exit(
                f"No stop plugin at {stop_plugin}. It is needed to measure the "
                f"boot cost that is subtracted from every transfer; build it "
                f"with qemu_plugin_setup.sh, point --stop-plugin at it, or "
                f"pass --no-subtract-boot."
            )

    labels = args.transfers or list(TRANSFERS)
    unknown = [label for label in labels if label not in TRANSFERS]
    if unknown:
        sys.exit(f"Unknown transfer label(s): {', '.join(unknown)}")

    profiles: dict[str, dict] = {}
    for label in labels:
        build_dir = os.path.join(outdir, label)
        report = os.path.join(outdir, f"{label}.report")
        console = os.path.join(outdir, f"{label}.console")
        boot_report = os.path.join(outdir, f"{label}.boot.report")
        boot_console = os.path.join(outdir, f"{label}.boot.console")
        elf = os.path.join(build_dir, "zephyr", "zephyr.elf")

        print(f"--- {label}: building", flush=True)
        build_one(zephyr_base, args.board, build_dir, label)

        print(f"--- {label}: running", flush=True)
        run_one(build_dir, plugin, report, console)

        used_boot = None
        if stop_plugin is not None:
            # The boot baseline is the *same binary* stopped as it enters the
            # measurement boundary, so the instruction stream it covers is
            # bit-identical to the full run's prefix and the subtraction is
            # exact. A separate boot-only build would not be: a guest that
            # runs no transfer diverges after boot.
            print(f"--- {label}: boot baseline", flush=True)
            run_one(
                build_dir,
                plugin,
                boot_report,
                boot_console,
                stop_plugin,
                symbol_address(elf, args.boot_boundary),
            )
            used_boot = boot_report

        profiles[label] = build_profile(
            label,
            report,
            elf,
            used_boot,
            console,
            args.board,
            args.icount_shift,
            zephyr_base,
            topdir,
        )
        render(profiles[label], args.top)

    if args.save:
        out = validate_path(args.save, args.base_dir, for_write=True)
        with open(out, "w") as fp:
            json.dump(profiles, fp, indent=1, sort_keys=True)
        print(f"\nSaved {len(profiles)} profile(s) to {out}")

    return 0


def cmd_report(args: argparse.Namespace) -> int:
    zephyr_base = os.environ.get("ZEPHYR_BASE") or os.getcwd()
    topdir = os.path.realpath(os.path.join(zephyr_base, ".."))

    report = validate_path(args.report, args.base_dir, for_write=False)
    elf = validate_path(args.elf, args.base_dir, for_write=False)
    boot = (
        validate_path(args.boot_report, args.base_dir, for_write=False)
        if args.boot_report
        else None
    )
    console = validate_path(args.console, args.base_dir, for_write=False) if args.console else None

    profile = build_profile(
        args.label,
        report,
        elf,
        boot,
        console,
        args.platform,
        args.icount_shift,
        zephyr_base,
        topdir,
    )
    render(profile, args.top)

    if args.save:
        out = validate_path(args.save, args.base_dir, for_write=True)
        with open(out, "w") as fp:
            json.dump({args.label: profile}, fp, indent=1, sort_keys=True)
        print(f"\nSaved profile to {out}")

    return 0


def cmd_diff(args: argparse.Namespace) -> int:
    with open(validate_path(args.baseline, args.base_dir, for_write=False)) as fp:
        baseline = json.load(fp)
    with open(validate_path(args.current, args.base_dir, for_write=False)) as fp:
        current = json.load(fp)

    ok = render_diff(baseline, current, args.tolerance, args.top)
    if not ok:
        print("\nAt least one transfer regressed beyond the tolerance.")
    return 0 if ok else 1


def main() -> int:
    # --base-dir is shared by every subcommand and accepted on either side of
    # the subcommand name, so it goes on a parent parser rather than the root.
    common = argparse.ArgumentParser(add_help=False, allow_abbrev=False)
    common.add_argument(
        "--base-dir",
        default=".",
        help="every file read or written must resolve under this directory",
    )

    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        parents=[common],
        allow_abbrev=False,
    )
    sub = parser.add_subparsers(dest="command", required=True)

    common_top = dict(type=int, default=25, help="how many rows to show")

    run = sub.add_parser(
        "run",
        help="build, run and profile the loopback selftest",
        parents=[common],
        allow_abbrev=False,
    )
    run.add_argument("--plugin", required=True, help="path to the QEMU TCG plugin")
    run.add_argument("--outdir", required=True, help="directory for builds and reports")
    run.add_argument("--board", default="qemu_x86")
    run.add_argument("--transfers", nargs="*", help=f"defaults to all of: {' '.join(TRANSFERS)}")
    run.add_argument("--icount-shift", type=int, default=5)
    run.add_argument(
        "--no-subtract-boot", action="store_true", help="keep the boot cost in each profile"
    )
    run.add_argument(
        "--stop-plugin",
        help="QEMU stop-on-address plugin used for the boot "
        "baseline (default: libstoptrigger.so next to --plugin)",
    )
    run.add_argument(
        "--boot-boundary", default="main", help="symbol separating boot from the measured work"
    )
    run.add_argument("--save", help="write the profiles as JSON")
    run.add_argument("--top", **common_top)
    run.set_defaults(func=cmd_run)

    rep = sub.add_parser(
        "report", help="report on an already collected run", parents=[common], allow_abbrev=False
    )
    rep.add_argument("--report", required=True, help="plugin block report")
    rep.add_argument("--elf", required=True, help="zephyr.elf of the same build")
    rep.add_argument("--boot-report", help="boot baseline report to subtract")
    rep.add_argument("--console", help="console log, for the throughput and byte counts")
    rep.add_argument("--label", default="run")
    rep.add_argument("--platform", default="?")
    rep.add_argument("--icount-shift", type=int, default=5)
    rep.add_argument("--save", help="write the profile as JSON")
    rep.add_argument("--top", **common_top)
    rep.set_defaults(func=cmd_report)

    dif = sub.add_parser(
        "diff", help="compare two saved profile sets", parents=[common], allow_abbrev=False
    )
    dif.add_argument("--baseline", required=True)
    dif.add_argument("--current", required=True)
    dif.add_argument(
        "--tolerance",
        type=float,
        default=1.0,
        help="allowed increase in instructions per byte, in percent",
    )
    dif.add_argument("--top", **common_top)
    dif.set_defaults(func=cmd_diff)

    args = parser.parse_args()
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
