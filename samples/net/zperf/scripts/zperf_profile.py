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
        --report ../build/zprof/tcp4.report \\
        --boot-report ../build/zprof/tcp4.boot.report \\
        --console ../build/zprof/tcp4.console \\
        --elf ../build/zprof/tcp4/zephyr/zephyr.elf --label tcp4 --top 25

    # What changed between two commits.
    samples/net/zperf/scripts/zperf_profile.py diff --base-dir .. \\
        --baseline ../build/zprof-base/profile.json \\
        --current ../build/zprof/profile.json

    # Check the profile against QEMU's own instruction counter, against
    # binutils, and against a code change whose effect it has to predict.
    samples/net/zperf/scripts/zperf_profile.py verify --base-dir .. \\
        --plugin ../tools/qemu-plugins/lib/libhotblocks.so \\
        --outdir ../build/zprof-verify

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
BOARD_HELP = (
    "qemu_x86 or qemu_x86_64, the boards whose guest halt ends QEMU through "
    "isa-debug-exit (default: %(default)s)"
)
OVERLAYS = "overlay-loopback.conf;overlay-loopback-icount.conf"
# Wall-clock limit for one QEMU run, after which it counts as never halting.
QEMU_TIMEOUT_S = 1800
# QEMU's exit status once the selftest halts the guest: isa-debug-exit turns the
# value written to it into (value << 1) | 1, and the selftest halts with
# K_ERR_KERNEL_PANIC, which is 4.
QEMU_EXIT_HALTED = (4 << 1) | 1
# The exit status the stop plugin is told to use. isa-debug-exit only produces
# odd values, QEMU exits with 1 on its own errors and with 0 on a triple fault.
QEMU_EXIT_STOPPED = 2

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

    def __init__(
        self,
        symbols: list[tuple[str, int, int, str, str]],
        load: list[tuple[int, int]],
        source: str = "<symbols>",
    ):
        """Build the lookup tables from raw symbol tuples.

        *symbols* is (name, address, size, st_info.type, st_info.bind) for
        every symbol defined in an executable section, and *load* is the
        (start, end) of every non-empty PT_LOAD segment. Taking an ELF file
        apart is from_elf()'s job; keeping it out of here lets the interval
        logic be exercised without a binary to hand.
        """
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

        for name, addr, size, kind, binding in symbols:
            if kind == "STT_FUNC" and size:
                rank = 1 if binding == "STB_GLOBAL" else 0
                if addr not in ranges or rank > ranges[addr][0]:
                    ranges[addr] = (rank, size, name)
            elif kind == "STT_FUNC" or (
                kind == "STT_NOTYPE" and binding in ("STB_GLOBAL", "STB_WEAK")
            ):
                rank = 1 if kind == "STT_FUNC" else 0
                if addr not in bounds or rank > bounds[addr][0]:
                    bounds[addr] = (rank, name)

        self._ranges = sorted((a, v[1], v[2]) for a, v in ranges.items())
        self._range_starts = [r[0] for r in self._ranges]

        # Every sized function also acts as a boundary for the unsized ones.
        for addr, (_rank, size, name) in ranges.items():
            bounds.setdefault(addr, (1, name))
            bounds.setdefault(addr + size, (1, UNKNOWN))
        self._bounds = sorted((a, v[1]) for a, v in bounds.items())
        self._bound_starts = [b[0] for b in self._bounds]

        if not self._ranges and not self._bounds:
            sys.exit(f"{source} contains no function symbols.")

        # Names that more than one function carries, with every start
        # address. Two kinds share a name: static functions that happen to be
        # called the same in different files, and the out-of-line copies of
        # one static inline that each calling file gets. key() tells them
        # apart.
        starts: dict[str, set[int]] = defaultdict(set)
        for addr, _size, name in self._ranges:
            starts[name].add(addr)
        for addr, name in self._bounds:
            if name != UNKNOWN:
                starts[name].add(addr)
        self._shared = {name: sorted(addrs) for name, addrs in starts.items() if len(addrs) > 1}
        self._keys: dict[str, dict[int, str] | None] = {}

        # Loadable ranges, used to tell "not a Zephyr function" apart from
        # "not Zephyr at all" (the BIOS the guest boots through).
        self._load = list(load)

        # Filled in by from_elf(); a table built from bare symbols has no
        # DWARF, so source_file() returns None for every address.
        self._fp = None
        self._dwarf = None
        self._aranges = None
        self._file_cache: dict[str, str | None] = {}
        self._sym_cache: dict[int, tuple[str, str, int | None]] = {}
        self._subprogram_cache: dict[int, dict[int, object]] = {}

    @classmethod
    def from_elf(cls, elf_path: str) -> "SymbolTable":
        """Build the table from a build's ELF, DWARF included when present."""
        fp = open(elf_path, "rb")  # noqa: SIM115 - kept open for DWARF
        elf = ELFFile(fp)

        symtab = elf.get_section_by_name(".symtab")
        if symtab is None:
            sys.exit(f"{elf_path} has no .symtab; a stripped build cannot be profiled.")

        exec_sections = {
            idx
            for idx, sec in enumerate(elf.iter_sections())
            if sec["sh_flags"] & SH_FLAGS.SHF_EXECINSTR
        }

        symbols = [
            (
                sym.name,
                sym.entry.st_value,
                sym.entry.st_size,
                sym.entry.st_info.type,
                sym.entry.st_info.bind,
            )
            for sym in symtab.iter_symbols()
            if isinstance(sym.entry.st_shndx, int) and sym.entry.st_shndx in exec_sections
        ]
        load = [
            (seg["p_vaddr"], seg["p_vaddr"] + seg["p_memsz"])
            for seg in elf.iter_segments()
            if seg["p_type"] == "PT_LOAD" and seg["p_memsz"]
        ]

        table = cls(symbols, load, elf_path)
        table._fp = fp
        table._dwarf = elf.get_dwarf_info() if elf.has_dwarf_info() else None
        if table._dwarf is not None:
            try:
                table._aranges = table._dwarf.get_aranges()
            except Exception:  # noqa: BLE001 - DWARF is best effort
                table._aranges = None
        return table

    def close(self) -> None:
        """Close the ELF from_elf() kept open for the DWARF lookups."""
        if self._fp is not None:
            self._fp.close()
            self._fp = None
        self._dwarf = None
        self._aranges = None

    def __enter__(self) -> "SymbolTable":
        return self

    def __exit__(self, *exc) -> None:
        self.close()

    def in_image(self, pc: int) -> bool:
        return any(lo <= pc < hi for lo, hi in self._load)

    def lookup(self, pc: int) -> tuple[str, str]:
        """Return (function name, kind) where kind is exact/inferred/bucket."""
        name, kind, _start = self.lookup_start(pc)
        return name, kind

    def lookup_start(self, pc: int) -> tuple[str, str, int | None]:
        """Like lookup(), plus the function's start address (None for a bucket)."""
        hit = self._sym_cache.get(pc)
        if hit is not None:
            return hit

        result = self._lookup_uncached(pc)
        self._sym_cache[pc] = result
        return result

    def _lookup_uncached(self, pc: int) -> tuple[str, str, int | None]:
        if not self.in_image(pc):
            return FIRMWARE, "bucket", None

        # A sized function wins outright.
        idx = bisect.bisect_right(self._range_starts, pc) - 1
        if idx >= 0:
            addr, size, name = self._ranges[idx]
            if pc < addr + size:
                return name, "exact", addr

        # Otherwise fall back to the nearest preceding entry point.
        idx = bisect.bisect_right(self._bound_starts, pc) - 1
        if idx >= 0:
            addr, name = self._bounds[idx]
            if name != UNKNOWN:
                return name, "inferred", addr

        return UNKNOWN, "bucket", None

    def key(self, name: str, start: int | None) -> tuple[str, bool]:
        """Return (attribution key, merged) for function *name* at *start*.

        A name carried by one function is its own key. Copies of one static
        inline are declared in the same file and also keep the bare name, so
        their costs add up. Functions declared in different files get the
        name followed by the shortest path suffix that tells their files
        apart, such as "out_func (log_output.c)". The keys depend on the
        image and not on which copies ran, so they line up in a diff, until
        a change adds or removes a function of the same name. When the
        declaring file of any copy is unknown, the copies cannot be told
        apart: the bare name is used and *merged* is True.
        """
        addrs = self._shared.get(name)
        if addrs is None or start is None:
            return name, False

        if name not in self._keys:
            self._keys[name] = self._split(name, addrs)
        keys = self._keys[name]
        if keys is None:
            return name, True
        return keys.get(start, name), False

    def _split(self, name: str, addrs: list[int]) -> dict[int, str] | None:
        files = {addr: self.decl_file(addr) for addr in addrs}
        if None in files.values():
            return None

        distinct = set(files.values())
        if len(distinct) == 1:
            return dict.fromkeys(addrs, name)

        suffix = _unique_suffixes(distinct)
        return {addr: f"{name} ({suffix[path]})" for addr, path in files.items()}

    def decl_file(self, addr: int) -> str | None:
        """File declaring the function that starts at *addr*, from DWARF.

        This is the file the function is written in, unlike source_file(),
        which gives the translation unit: for an out-of-line copy of a static
        inline it is the header, the same for every copy. The function's
        own DW_TAG_subprogram is found by its low_pc in the unit covering
        *addr*, and DW_AT_abstract_origin or DW_AT_specification is followed
        to the entry carrying DW_AT_decl_file.
        """
        if self._aranges is None:
            return None

        try:
            cu_offset = self._aranges.cu_offset_at_addr(addr)
            if cu_offset is None:
                return None
            die = self._subprograms(cu_offset).get(addr)
            # Real chains are one or two links long; the bound only stops a
            # loop in broken DWARF.
            for _ in range(8):
                if die is None:
                    return None
                decl = die.attributes.get("DW_AT_decl_file")
                if decl is not None:
                    return _dwarf_file(self._dwarf, die.cu, decl.value)
                origin = next(
                    (
                        attr
                        for attr in ("DW_AT_abstract_origin", "DW_AT_specification")
                        if attr in die.attributes
                    ),
                    None,
                )
                die = die.get_DIE_from_attribute(origin) if origin is not None else None
        except Exception:  # noqa: BLE001 - DWARF is best effort
            return None
        return None

    def _subprograms(self, cu_offset: int) -> dict[int, object]:
        """Every DW_TAG_subprogram with a low_pc in one unit, by address."""
        found = self._subprogram_cache.get(cu_offset)
        if found is None:
            found = {}
            cu = self._dwarf.get_CU_at(cu_offset)
            for die in cu.iter_DIEs():
                # pyelftools already resolves the .debug_addr index of a
                # DW_FORM_addrx low_pc, as clang emits for DWARF 5.
                low = die.attributes.get("DW_AT_low_pc") if die.tag == "DW_TAG_subprogram" else None
                if low is not None:
                    found[low.value] = die
            self._subprogram_cache[cu_offset] = found
        return found

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


def _dwarf_file(dwarf, cu, index: int) -> str | None:
    """Absolute path of file *index* in *cu*'s line program file table.

    DWARF 5 numbers files and directories from 0, with entry 0 the unit
    itself; DWARF 4 and earlier number files from 1 and use directory 0 for
    the compilation directory.
    """
    program = dwarf.line_program_for_CU(cu)
    if program is None:
        return None

    version = program.header["version"]
    files = program.header["file_entry"]
    dirs = program.header["include_directory"]
    idx = index if version >= 5 else index - 1
    if not 0 <= idx < len(files):
        return None

    entry = files[idx]
    path = entry.name.decode()
    dir_idx = entry.dir_index if version >= 5 else entry.dir_index - 1
    if not os.path.isabs(path) and 0 <= dir_idx < len(dirs):
        path = os.path.join(dirs[dir_idx].decode(), path)
    if not os.path.isabs(path):
        comp_dir = cu.get_top_DIE().attributes.get("DW_AT_comp_dir")
        if comp_dir is not None:
            path = os.path.join(comp_dir.value.decode(), path)
    return os.path.normpath(path)


def _fit_name(name: str, width: int) -> str:
    """Shorten a row name to *width*, keeping a "(file)" suffix whole.

    The suffix is what tells same-named functions apart, so cutting it off would
    print two different rows under one name.
    """
    if len(name) <= width:
        return name

    func, sep, suffix = name.rpartition(" (")
    if sep and suffix.endswith(")") and len(sep) + len(suffix) < width:
        return func[: width - len(sep) - len(suffix)] + sep + suffix

    return name[:width]


def _unique_suffixes(paths: set[str]) -> dict[str, str]:
    """Shortest trailing part of each path that no other path ends in."""
    split = {path: path.split("/") for path in paths}
    out = {}
    for path, parts in split.items():
        for count in range(1, len(parts) + 1):
            tail = parts[-count:]
            if not any(other[-count:] == tail for p, other in split.items() if p != path):
                break
        out[path] = "/".join(tail)
    return out


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
    """Fold blocks into per-function instruction totals.

    Functions are keyed by SymbolTable.key(): by name, with the declaring
    file added where functions in different files share a name. An entry
    whose copies could not be told apart is marked "merged". An entry's
    file is the translation unit of the first block met.
    """
    funcs: dict[str, dict] = {}
    total = 0
    kinds: dict[str, int] = defaultdict(int)

    for (pc, insns), ecount in blocks.items():
        cost = insns * ecount
        total += cost
        name, kind, start = table.lookup_start(pc)
        kinds[kind] += cost

        key, merged = table.key(name, start)
        entry = funcs.get(key)
        if entry is None:
            source = table.source_file(pc, zephyr_base, topdir) if kind != "bucket" else None
            entry = {"insns": 0, "file": source, "group": classify(name, source)}
            if merged:
                entry["merged"] = True
            funcs[key] = entry
        entry["insns"] += cost

    return funcs, total, dict(kinds)


def _ipb(insns: int, byts: int) -> float:
    """Instructions per payload byte; 0.0 when nothing was transferred."""
    return (insns / byts) if byts else 0.0


def _mbps(ipb: float, shift: int) -> float:
    """Throughput implied by *ipb* under ``-icount shift=<shift>``.

    8 bits/byte * 1e9 ns/s / (2**shift ns/insn * ipb insn/byte) / 1e6, which
    at the default shift of 5 is 250/ipb.
    """
    return (8000.0 / (2**shift * ipb)) if ipb else 0.0


def _mbps_gain(ipb: float, f_ipb: float, shift: int) -> float:
    """What the throughput would gain if a function costing *f_ipb* were free."""
    if ipb <= f_ipb:
        return 0.0
    return _mbps(ipb - f_ipb, shift) - _mbps(ipb, shift)


def _pct(part: float, whole: float) -> float:
    """*part* as a percentage of *whole*; 0.0 when *whole* is zero."""
    return (100.0 * part / whole) if whole else 0.0


def render(profile: dict, top: int) -> None:
    """Print the header, the per-function table and the group roll-up."""
    label = profile["label"]
    total = profile["total_insns"]
    byts = profile.get("bytes") or 0
    shift = profile.get("icount_shift", 5)
    measured = profile.get("measured_mbps")

    ipb = _ipb(total, byts)
    reconstructed = _mbps(ipb, shift)

    print(f"\n=== {label} ({profile.get('platform', '?')}) ===")
    line = f"instructions {total:>15,}   payload {byts:>12,} B   ipb {ipb:8.3f}"
    if profile.get("boot_insns"):
        line += f"   boot subtracted {profile['boot_insns']:,}"
    print(line)

    if measured:
        # The profile covers everything from the boot boundary to the halt,
        # while the measured throughput covers only the timed transfer, so
        # this ratio is the share of the profiled cost the transfer accounts
        # for. The rest is the network initialisation, the socket setup and
        # the result reporting either side of it.
        #
        # It is not a CPU-bound fraction, and it moves the other way from one.
        # With icount sleep=off, halted time advances virtual time without
        # executing instructions, so time the guest spends idle inside the
        # window makes the window longer without adding instructions, which
        # pushes this figure *above* 100%. On loopback there is next to no
        # such time, and what is seen instead is a steady few percent of work
        # outside the window.
        print(
            f"throughput   measured {measured:8.3f} Mbps   "
            f"from profile {reconstructed:8.3f} Mbps   "
            f"in-window {_pct(reconstructed, measured):5.1f}%"
        )

    window_us = profile.get("window_us")
    if window_us:
        # The accounting behind the line above, in seconds, so the residual is
        # a number rather than an inference.
        cpu_s = total * (2**shift) / 1e9
        window_s = window_us / 1e6
        print(
            f"virtual time profiled {cpu_s:.6f} s of cpu   "
            f"transfer window {window_s:.6f} s   "
            f"outside the window {cpu_s - window_s:+.6f} s"
        )

    kinds = profile.get("kinds") or {}
    if total:
        bucketed = kinds.get("bucket", 0)
        inferred = kinds.get("inferred", 0)
        print(
            f"attribution  exact {_pct(kinds.get('exact', 0), total):5.2f}%   "
            f"inferred {_pct(inferred, total):5.2f}%   "
            f"unattributed {_pct(bucketed, total):5.2f}%"
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
        share = _pct(insns, total)
        f_ipb = _ipb(insns, byts)
        # What the throughput would become if this function cost nothing.
        gain = _mbps_gain(ipb, f_ipb, shift)
        print(
            f"{rank:>3}  {_fit_name(name, 33):<34}{info['group']:<10}{insns:>14,}"
            f"{f_ipb:>8.3f}{share:>6.2f}%{_pct(cum, total):>6.1f}%"
            f"{gain:>+8.2f}  {info['file'] or ''}"
        )

    groups = _group_totals(profile)

    print(f"\n{'group':<12}{'instructions':>14}{'ipb':>9}{'%':>8}")
    ordered = sorted(
        groups.items(), key=lambda kv: (GROUP_ORDER.index(kv[0]) if kv[0] in GROUP_ORDER else 99)
    )
    for group, insns in ordered:
        print(f"{group:<12}{insns:>14,}{_ipb(insns, byts):>9.3f}{_pct(insns, total):>7.2f}%")

    merged = sorted(name for name, info in funcs.items() if info.get("merged"))
    if merged:
        print(
            f"\nmerged: {', '.join(merged)}: each of these rows may add up functions "
            f"in different files that share a name, which the ELF has no debug info "
            f"to tell apart."
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
        if b_bytes == 0 or c_bytes == 0:
            # No payload means no instructions per byte to compare: a 0%
            # change, or -100% against a baseline that did move data, would
            # pass a transfer that failed.
            missing = [side for side, n in (("baseline", b_bytes), ("current", c_bytes)) if n == 0]
            print(f"\n=== {label} ===\n  no payload byte count in {' and '.join(missing)}")
            ok = False
            continue

        b_ipb = _ipb(base["total_insns"], b_bytes)
        c_ipb = _ipb(cur["total_insns"], c_bytes)
        change = _pct(c_ipb - b_ipb, b_ipb)
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
                print(f"  {_fit_name(name, 35):<36}{delta:>+14,}  {note}")

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

    # ninja fails on a build directory that has no such target, which is the
    # case this message is for, so check the exit status here.
    proc = subprocess.run(
        [ninja, "-C", build_dir, "-t", "commands", "zephyr/run_qemu"],
        capture_output=True,
        text=True,
        check=False,
    )
    out = proc.stdout.strip().splitlines()
    if proc.returncode != 0 or not out:
        sys.exit(f"No run_qemu target in {build_dir}; is it a QEMU board build?")

    # The recipe is "cd <dir> && <qemu> <args...>", written as the shell would
    # run it, so a build path containing a space arrives quoted. Split it the
    # same way the shell would: shlex keeps such a path in one token and strips
    # the quotes, which is what execve() needs, and it leaves "&&" as a plain
    # token because it parses words rather than shell grammar. QEMU is the
    # last command: a build that uses a FIFO for the console, as twister's do,
    # prepends "cmake -E touch qemu-fifo &&" to it.
    words = shlex.split(out[-1])
    argv = words[len(words) - words[::-1].index("&&") :] if "&&" in words else words
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


def qemu_exit_problem(status: int, expected: int) -> str | None:
    """Explain an unexpected QEMU exit status, or return None if it is expected."""
    if status == expected:
        return None

    if status < 0:
        cause = f"QEMU was killed by signal {-status}"
    elif status == 1:
        cause = "QEMU itself failed, for example on a plugin that did not load, see its message"
    elif status == 0:
        cause = "the guest triple faulted"
    elif status == QEMU_EXIT_HALTED:
        cause = "the guest halted before reaching the stop address"
    elif status & 1:
        cause = f"the guest halted with reason {status >> 1}, for example on a fatal error"
    else:
        cause = "QEMU was ended by something other than the guest"

    return f"QEMU exited with status {status} instead of {expected}: {cause}."


def run_one(
    build_dir: str,
    plugin: str | None,
    report_path: str,
    console_path: str,
    stop_plugin: str | None = None,
    stop_addr: int | None = None,
    extra_plugins: tuple[str, ...] = (),
) -> None:
    argv = qemu_command(build_dir)
    expected = QEMU_EXIT_HALTED
    argv += [
        "-serial",
        f"file:{console_path}",
        "-display",
        "none",
        "-monitor",
        "none",
    ]
    # No plugin at all is how the run is checked for not being disturbed by
    # being watched: the guest must report the same throughput either way.
    if plugin is not None:
        argv += ["-plugin", f"file={plugin},inline=on"]
    for extra in extra_plugins:
        argv += ["-plugin", extra]
    if stop_plugin is not None and stop_addr is not None:
        # Stop the guest the moment it reaches the address, so this run covers
        # only the prefix shared with the full run.
        argv += ["-plugin", f"file={stop_plugin},addr=0x{stop_addr:x}:{QEMU_EXIT_STOPPED}"]
        expected = QEMU_EXIT_STOPPED
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

    # The guest halts itself through isa-debug-exit and the stop plugin exits
    # with a status of its own, so either run ends with a known exit status.
    # Anything else means the run did not end the way it was meant to, and the
    # report it left behind, if any, cannot be trusted.
    try:
        proc = subprocess.run(argv, cwd=build_dir, check=False, timeout=QEMU_TIMEOUT_S)
    except subprocess.TimeoutExpired:
        sys.exit(
            f"QEMU was still running after {QEMU_TIMEOUT_S} s and was killed. "
            f"The guest never halted: either it was built without "
            f"CONFIG_ZPERF_LOOPBACK_SELFTEST_HALT_ON_DONE=y, or a transfer "
            f"stalled, see {console_path}."
        )

    problem = qemu_exit_problem(proc.returncode, expected)
    if problem is not None:
        sys.exit(f"{problem} Console output is in {console_path}.")

    if plugin is None:
        return

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


def build_one(
    zephyr_base: str, board: str, build_dir: str, only: str, extra: tuple[str, ...] = ()
) -> None:
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
        *extra,
    ]
    subprocess.run(argv, check=True, cwd=zephyr_base)


def parse_console(path: str) -> tuple[dict[str, float], dict[str, int], dict[str, int]]:
    """Extract the ZPERF-RESULT throughputs and the ZPERF-INFO byte counts and
    transfer durations.

    The duration is the guest's own view of how long the timed transfer took,
    in virtual microseconds. It is what makes the profile's virtual time
    accounting checkable rather than merely plausible.
    """
    mbps: dict[str, float] = {}
    byts: dict[str, int] = {}
    window: dict[str, int] = {}
    if not os.path.exists(path):
        return mbps, byts, window

    with open(path, errors="replace") as fp:
        text = fp.read()
    for label, value in re.findall(r"ZPERF-RESULT (\w+)_mbps=([0-9.]+)", text):
        mbps[label] = float(value)
    # One ZPERF-INFO line carries several fields, so scope to the line first
    # rather than anchoring each field on the prefix.
    for info in re.findall(r"ZPERF-INFO ([^\n]*)", text):
        for label, value in re.findall(r"(\w+)_bytes=(\d+)", info):
            byts[label] = int(value)
        for label, value in re.findall(r"(\w+)_us=(\d+)", info):
            window[label] = int(value)
    return mbps, byts, window


def check_transfer(console_path: str, label: str) -> None:
    """Exit unless the guest reported a payload byte count for *label*.

    A transfer that fails in the guest prints "<label>_mbps=error(<errno>)"
    and no byte count, but the guest still halts normally, so the plugin
    report looks complete. Without a byte count there are no instructions per
    byte, and the profile would be saved as a run that moved nothing.
    """
    if parse_console(console_path)[1].get(label, 0) != 0:
        return

    text = ""
    if os.path.exists(console_path):
        with open(console_path, errors="replace") as fp:
            text = fp.read()
    failed = re.search(rf"ZPERF-RESULT {re.escape(label)}_mbps=(error\(-?\d+\))", text)
    if failed is not None:
        sys.exit(
            f"{label}: the transfer failed in the guest with {failed.group(1)}, see {console_path}."
        )
    sys.exit(f"{label}: the guest reported no payload byte count, see {console_path}.")


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

    with SymbolTable.from_elf(elf) as table:
        funcs, total, kinds = attribute(blocks, table, zephyr_base, topdir)

    mbps, byts, window = parse_console(console) if console else ({}, {}, {})

    return {
        "schema": 1,
        "label": label,
        "platform": platform,
        "icount_shift": shift,
        "bytes": byts.get(label, 0),
        "measured_mbps": mbps.get(label),
        "window_us": window.get(label),
        "total_insns": total,
        "boot_insns": boot_insns,
        "kinds": kinds,
        "functions": funcs,
    }


def _zephyr_base() -> str:
    """ZEPHYR_BASE, or the tree this script lives in when it is not set."""
    return os.environ.get("ZEPHYR_BASE") or os.path.realpath(
        os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", "..")
    )


def cmd_run(args: argparse.Namespace) -> int:
    zephyr_base = _zephyr_base()
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
        check_transfer(console, label)

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
    zephyr_base = _zephyr_base()
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


def parse_insn_total(path: str) -> int | None:
    """The instruction count QEMU's own insn plugin reported, if it was loaded.

    That plugin counts once per instruction, where hotblocks counts once per
    translation block and multiplies by the block length. The two numbers come
    from different code in QEMU. The block count is an upper bound, because a
    block that QEMU rewinds at a device access in its middle is still charged in
    full, so it may exceed the instruction count slightly but never fall below
    it.
    """
    with open(path, errors="replace") as fp:
        for line in fp:
            match = re.match(r"total insns:\s*(\d+)", line.strip())
            if match:
                return int(match.group(1))
    return None


def cache_tool(build_dir: str, key: str) -> str | None:
    """A toolchain binary the build recorded, e.g. CMAKE_NM."""
    cache = os.path.join(build_dir, "CMakeCache.txt")
    if not os.path.exists(cache):
        return None

    with open(cache, errors="replace") as fp:
        for line in fp:
            name, _, value = line.partition("=")
            if name.split(":")[0] == key:
                return value.strip() or None
    return None


# GCC splits a function into several symbols: a cold partition, a cloned
# constant-propagated body, an interprocedural-scalar-replacement variant. They
# are the same function under decorated names, and nm's size for the parent can
# run into the child, so compare the undecorated stems.
GCC_PARTITION = re.compile(r"\.(cold|hot|part|isra|constprop|lto_priv)(\.[0-9]+)*$")


def base_symbol(name: str) -> str:
    """A symbol name with GCC's per-partition decoration removed."""
    while True:
        stripped = GCC_PARTITION.sub("", name)
        if stripped == name:
            return name
        name = stripped


def nm_functions(nm: str, elf_path: str) -> dict[tuple[int, int], set[str]]:
    """Sized functions as binutils sees them, keyed by (address, size).

    The sysv format is used rather than the default because it names the
    symbol type outright. The single letter codes do not: a constant placed in
    a text section is a "T" like any function, and comparing against those
    would report a difference wherever the profiler is right to ignore one.

    Aliases share an address, so the value is a set of undecorated names: the
    profiler picks one name per address and any of the aliases is a correct
    answer.
    """
    out = subprocess.run(
        [nm, "--format=sysv", "--defined-only", elf_path],
        capture_output=True,
        text=True,
        check=True,
    ).stdout

    functions: dict[tuple[int, int], set[str]] = defaultdict(set)
    for line in out.splitlines():
        fields = [f.strip() for f in line.split("|")]
        # Name | Value | Class | Type | Size | Line | Section
        if len(fields) != 7 or fields[3] != "FUNC":
            continue
        try:
            addr, size = int(fields[1], 16), int(fields[4], 16)
        except ValueError:
            # An unsized symbol leaves the column blank. Those are exactly the
            # ones the profiler has to infer bounds for, so nm has nothing to
            # check them against.
            continue
        if size:
            functions[(addr, size)].add(base_symbol(fields[0]))
    return dict(functions)


def _report(results: list[tuple[str, str, str]], name: str, verdict: str, detail: str) -> None:
    results.append((name, verdict, detail))
    print(f"{verdict:<5} {name:<24} {detail}", flush=True)


def _profile_one(
    label: str,
    build_dir: str,
    outdir: str,
    prefix: str,
    plugin: str,
    stop_plugin: str,
    board: str,
    shift: int,
    zephyr_base: str,
    topdir: str,
    extra_plugins: tuple[str, ...] = (),
) -> tuple[dict, str]:
    """One full run plus its boot baseline, folded into a profile."""
    elf = os.path.join(build_dir, "zephyr", "zephyr.elf")
    report = os.path.join(outdir, f"{prefix}.report")
    console = os.path.join(outdir, f"{prefix}.console")
    boot_report = os.path.join(outdir, f"{prefix}.boot.report")
    boot_console = os.path.join(outdir, f"{prefix}.boot.console")

    run_one(build_dir, plugin, report, console, extra_plugins=extra_plugins)
    run_one(build_dir, plugin, boot_report, boot_console, stop_plugin, symbol_address(elf, "main"))
    profile = build_profile(
        label, report, elf, boot_report, console, board, shift, zephyr_base, topdir
    )
    return profile, report


def cmd_verify(args: argparse.Namespace) -> int:
    """Check the profile against things that are not this script.

    The unit tests establish that the script does what its author meant. They
    cannot establish that what it means is true of the guest, because they
    never run one. These checks compare its output against QEMU's own
    instruction counter, against binutils' view of the same ELF, against the
    guest running unobserved, and against a code change whose effect the
    profile has to predict before it is measured.
    """
    zephyr_base = _zephyr_base()
    topdir = os.path.realpath(os.path.join(zephyr_base, ".."))
    plugin = validate_qemu_arg_path(args.plugin, args.base_dir, for_write=False)
    plugin_dir = os.path.dirname(plugin)
    stop_plugin = validate_qemu_arg_path(
        args.stop_plugin or os.path.join(plugin_dir, "libstoptrigger.so"),
        args.base_dir,
        for_write=False,
    )
    insn_plugin = validate_qemu_arg_path(
        args.insn_plugin or os.path.join(plugin_dir, "libinsn.so"), args.base_dir, for_write=False
    )
    outdir = validate_path(args.outdir, args.base_dir, for_write=False)
    os.makedirs(outdir, exist_ok=True)

    label = args.transfer
    if label not in TRANSFERS:
        sys.exit(f"Unknown transfer '{label}'. One of: {', '.join(TRANSFERS)}")

    build_dir = os.path.join(outdir, label)
    elf = os.path.join(build_dir, "zephyr", "zephyr.elf")
    results: list[tuple[str, str, str]] = []

    print(f"--- {label}: building", flush=True)
    build_one(zephyr_base, args.board, build_dir, label)

    print(f"\n=== Verifying the profile ({label}, {args.board}) ===\n", flush=True)

    # 1. QEMU's own instruction counter against the block accounting. Both
    # plugins go into one run, so this costs nothing and compares the very
    # same execution rather than two runs that merely ought to match.
    profile, report = _profile_one(
        label,
        build_dir,
        outdir,
        label,
        plugin,
        stop_plugin,
        args.board,
        args.icount_shift,
        zephyr_base,
        topdir,
        extra_plugins=(f"file={insn_plugin}",),
    )
    blocks, _ = parse_report(report)
    block_total = sum(insns * count for (_, insns), count in blocks.items())
    insn_total = parse_insn_total(report)
    if insn_total is None:
        _report(
            results,
            "instruction count",
            "SKIP",
            f"no insn plugin output in {os.path.basename(report)}; build libinsn.so "
            f"with qemu_plugin_setup.sh",
        )
    else:
        # These do not have to be equal, and knowing why is the point of
        # running both. Counting per block charges a whole block whenever
        # execution enters it, while counting per instruction charges only
        # what ran. Under icount a device access in the middle of a block makes
        # QEMU rewind the block and resume after the access in a new one, so
        # the rest of the rewound block is charged twice; on this guest that is
        # almost entirely the HPET counter read in sys_clock_elapsed(). So the
        # block count is an upper bound, and what is checked is that the gap is
        # small and in the direction it has to be. A block count *below* the
        # instruction count would mean blocks are being missed entirely.
        gap = _pct(block_total - insn_total, insn_total)
        _report(
            results,
            "instruction count",
            "PASS" if 0 <= gap <= args.block_bias else "FAIL",
            f"hotblocks {block_total:,}, insn plugin {insn_total:,}, blocks high by {gap:.3f}%",
        )

    # 2. Nothing is lost or invented between the blocks and the table.
    func_total = sum(info["insns"] for info in profile["functions"].values())
    kind_total = sum(profile["kinds"].values())
    _report(
        results,
        "conservation",
        "PASS" if func_total == kind_total == profile["total_insns"] else "FAIL",
        f"functions {func_total:,}, kinds {kind_total:,}, total {profile['total_insns']:,}",
    )

    # 3. After the boot prefix comes off, every instruction left belongs to a
    # function in this image. Anything in the bucket is the BIOS, which means
    # the subtraction did not do its job.
    bucket = profile["kinds"].get("bucket", 0)
    share = _pct(bucket, profile["total_insns"])
    _report(
        results,
        "attribution",
        "PASS" if share <= args.unattributed else "FAIL",
        f"{bucket:,} instructions ({share:.4f}%) unattributed after boot subtraction",
    )

    # 4. binutils reading the same ELF has to agree about which function owns
    # which address. This checks the symbol extraction and the interval
    # construction against an implementation that shares no code with it.
    nm = args.nm or cache_tool(build_dir, "CMAKE_NM")
    if nm is None or shutil.which(nm) is None:
        _report(results, "symbol table", "SKIP", "no nm found; pass --nm")
    else:
        with SymbolTable.from_elf(elf) as table:
            functions = nm_functions(nm, elf)
            bad = [
                (addr, size, names)
                for (addr, size), names in functions.items()
                if base_symbol(table.lookup(addr)[0]) not in names
                or base_symbol(table.lookup(addr + size - 1)[0]) not in names
            ]
            detail = f"{len(functions)} sized functions, {len(bad)} disagreeing with nm"
            if bad:
                addr, size, names = bad[0]
                detail += (
                    f" (first: 0x{addr:x}+0x{size:x} is {sorted(names)}, profiler says "
                    f"{table.lookup(addr)[0]} at the start and "
                    f"{table.lookup(addr + size - 1)[0]} at the end)"
                )
        _report(results, "symbol table", "PASS" if not bad else "FAIL", detail)

    # 5. Watching the guest must not change what it does. The plugin callbacks
    # run on the host and execute no guest instructions, so the throughput the
    # guest reports has to be identical with and without them.
    bare_console = os.path.join(outdir, f"{label}.noplugin.console")
    run_one(build_dir, None, os.path.join(outdir, f"{label}.noplugin.report"), bare_console)
    bare_mbps, _bytes, _window = parse_console(bare_console)
    watched_mbps, _bytes, _window = parse_console(os.path.join(outdir, f"{label}.console"))
    _report(
        results,
        "non-perturbation",
        "PASS" if bare_mbps == watched_mbps and bare_mbps else "FAIL",
        f"unwatched {bare_mbps}, watched {watched_mbps}",
    )

    # 6. Under icount the same binary executes the same instructions, so a
    # repeat run should land on the same total. The logging thread is
    # scheduled independently of the traffic, which is what the tolerance is
    # for; anything larger means the measurement is not repeatable.
    repeat, _ = _profile_one(
        label,
        build_dir,
        outdir,
        f"{label}.repeat",
        plugin,
        stop_plugin,
        args.board,
        args.icount_shift,
        zephyr_base,
        topdir,
    )
    drift = abs(_pct(repeat["total_insns"] - profile["total_insns"], profile["total_insns"]))
    _report(
        results,
        "determinism",
        "PASS" if drift < args.drift else "FAIL",
        f"{profile['total_insns']:,} then {repeat['total_insns']:,}, {drift:.4f}% apart",
    )

    # 7. The one that matters. If instructions per byte really is the
    # throughput in different units, then removing known work from the stack
    # must move the measured throughput by the amount the profile says. Take
    # the receive side UDP checksum out, which is one of the two passes the
    # loopback path makes over every datagram, and compare.
    if not label.startswith("udp"):
        _report(results, "prediction", "SKIP", f"needs a UDP transfer, not {label}")
    else:
        mod_dir = os.path.join(outdir, f"{label}-nocksum")
        print(f"\n--- {label}: building without the UDP checksum", flush=True)
        build_one(zephyr_base, args.board, mod_dir, label, extra=("-DCONFIG_NET_UDP_CHECKSUM=n",))
        modified, _ = _profile_one(
            label,
            mod_dir,
            outdir,
            f"{label}.nocksum",
            plugin,
            stop_plugin,
            args.board,
            args.icount_shift,
            zephyr_base,
            topdir,
        )
        base_mbps = profile["measured_mbps"]
        mod_mbps = modified["measured_mbps"]
        if base_mbps is None or mod_mbps is None:
            _report(
                results,
                "prediction",
                "FAIL",
                f"no measured throughput in the console (base {base_mbps}, modified {mod_mbps})",
            )
        else:
            base_ipb = _ipb(profile["total_insns"], profile["bytes"])
            mod_ipb = _ipb(modified["total_insns"], modified["bytes"])
            predicted = _pct(base_ipb - mod_ipb, mod_ipb)
            measured = _pct(mod_mbps - base_mbps, base_mbps)
            _report(
                results,
                "prediction",
                "PASS" if abs(predicted - measured) <= args.prediction else "FAIL",
                f"profile said {predicted:+.2f}%, guest measured {measured:+.2f}%, "
                f"{abs(predicted - measured):.2f} points apart",
            )

    failed = [name for name, verdict, _ in results if verdict == "FAIL"]
    skipped = [name for name, verdict, _ in results if verdict == "SKIP"]
    print(
        f"\n{len(results) - len(failed) - len(skipped)} passed, "
        f"{len(failed)} failed, {len(skipped)} skipped"
    )
    if failed:
        print(f"Failed: {', '.join(failed)}", file=sys.stderr)
    return 1 if failed else 0


def main() -> int:
    # --base-dir is accepted on either side of the subcommand name. Only the
    # root parser has a default: a subparser writes its defaults over the
    # namespace the root parser filled in, so a default on the subcommand side
    # would discard a --base-dir given before the subcommand name.
    base_dir_help = "every file read or written must resolve under this directory"
    common = argparse.ArgumentParser(add_help=False, allow_abbrev=False)
    common.add_argument("--base-dir", default=argparse.SUPPRESS, help=base_dir_help)

    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False,
    )
    parser.add_argument("--base-dir", default=".", help=base_dir_help)
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
    run.add_argument("--board", default="qemu_x86", help=BOARD_HELP)
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

    ver = sub.add_parser(
        "verify",
        help="check the profile against QEMU, binutils and a measured code change",
        parents=[common],
        allow_abbrev=False,
    )
    ver.add_argument("--plugin", required=True, help="path to the QEMU TCG plugin")
    ver.add_argument("--outdir", required=True, help="directory for builds and reports")
    ver.add_argument("--board", default="qemu_x86", help=BOARD_HELP)
    ver.add_argument("--transfer", default="udp4", help=f"one of: {' '.join(TRANSFERS)}")
    ver.add_argument("--icount-shift", type=int, default=5)
    ver.add_argument("--stop-plugin", help="default: libstoptrigger.so next to --plugin")
    ver.add_argument("--insn-plugin", help="default: libinsn.so next to --plugin")
    ver.add_argument("--nm", help="default: the nm the build used")
    ver.add_argument(
        "--drift",
        type=float,
        default=0.01,
        help="allowed instruction count drift between two runs, in percent",
    )
    ver.add_argument(
        "--block-bias",
        type=float,
        default=1.0,
        help="how far the per-block instruction count may exceed the "
        "per-instruction one, in percent",
    )
    ver.add_argument(
        "--unattributed",
        type=float,
        default=0.01,
        help="allowed share of instructions belonging to no function, in percent",
    )
    ver.add_argument(
        "--prediction",
        type=float,
        default=0.5,
        help="allowed gap between the predicted and the measured throughput "
        "change, in percentage points",
    )
    ver.set_defaults(func=cmd_verify)

    args = parser.parse_args()
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
