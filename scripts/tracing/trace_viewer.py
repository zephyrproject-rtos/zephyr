#!/usr/bin/env python3
#
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0

"""
Zephyr CTF trace viewer.

Reads a Common Trace Format (CTF) binary stream produced by Zephyr's tracing
subsystem (for example the ``tracing.bin`` dumped by
``samples/subsys/tracing`` when built with the CTF format and the semihosting
backend) and renders it graphically on the console - a lightweight,
terminal-only take on tools such as SEGGER SystemView or Eclipse Trace Compass.

The viewer focuses on visualization. Most of the screen is a Gantt-style time
graph with one lane per thread (plus lanes for the idle thread and for ISRs);
coloured bars show which thread is running over time and the transitions
between them are the context switches. A movable time cursor (the playhead)
selects a point in the trace, with a small info strip beneath the chart that
can be toggled off to give the graph the whole screen. A metrics panel fills
the area below the lanes with a CPU-busy gauge, context-switch / event rates
and per-thread stacked utilization bars computed over the visible window.

The trace can be replayed: ``space`` starts/stops autoplay, which advances the
playhead in real time and scrolls the chart to follow it, and ``[`` / ``]``
adjust the playback speed. Pressing ``v`` switches to a raw CTF log view: a
scrollable table of every decoded event (timestamp, id, name and fields), with
the event nearest the playhead highlighted.

Unlike ``babeltrace2`` this script needs no external dependencies: it decodes
the packed little-endian CTF records itself. The per-event field layout is read
from the TSDL ``metadata`` file that ships next to the tracing subsystem
(``subsys/tracing/ctf/tsdl/metadata``) so the decoder automatically tracks any
events added there; a built-in fallback table covers the core scheduling events
so the time graph still works even when the metadata file cannot be found.

Usage::

    # interactive viewer (a terminal/TTY is required)
    ./trace_viewer.py build/tracing.bin

    # follow a trace live while the application is still writing it
    ./trace_viewer.py -f build/tracing.bin

    # non-interactive ASCII timeline, e.g. to pipe or redirect
    ./trace_viewer.py build/tracing.bin --text

    # point at a specific metadata file
    ./trace_viewer.py build/tracing.bin --metadata path/to/metadata
"""

import argparse
import os
import re
import struct
import sys


def safe_open(path, mode, **kwargs):
    """Open a command-line file path after canonicalising and validating it.

    The viewer only ever reads pre-existing files named on the command line.
    Resolving symlinks with ``realpath`` and confirming the target is a regular
    file keeps a stray CLI argument (a directory, device node, or dangling
    symlink) from being opened, and yields a clear error instead of a deep
    traceback. Centralising every read here keeps the trusted/untrusted
    boundary in one obvious place.
    """
    real = os.path.realpath(path)
    if not os.path.isfile(real):
        raise SystemExit(f"error: {path!r} is not a readable file")
    return open(real, mode, **kwargs)  # noqa: SIM115


# Record framing, identical for every event:
#   uint64_t timestamp (ns)   <- present when CONFIG_TRACING_CTF_TIMESTAMP=y
#   uint16_t id
#   <packed, byte-aligned event specific fields>
HDR = struct.Struct("<QH")
HDR_NO_TS = struct.Struct("<H")

# Map a TSDL integer typedef to a struct format character and byte size.
TYPES = {
    "int8_t": ("b", 1),
    "uint8_t": ("B", 1),
    "uint16_t": ("H", 2),
    "uint32_t": ("I", 4),
    "int32_t": ("i", 4),
    "uint64_t": ("Q", 8),
}

# Events that drive the running-thread timeline / lane layout.
THREAD_SWITCHED_IN = 0x11
THREAD_SWITCHED_OUT = 0x10
ISR_ENTER = 0x1B
ISR_EXIT = 0x1C
ISR_EXIT_TO_SCHEDULER = 0x1D
IDLE = 0x1E

# Events carrying thread identity we want in the thread table.
THREAD_INFO = 0x19
THREAD_CREATE = 0x13
THREAD_NAME_SET = 0x1A
THREAD_PRIO_SET = 0x12  # k_thread_priority_set bracket (carries prio)
THREAD_SCHED_PRIO_SET = 0xE9  # scheduler priority change, incl. inheritance

# Minimal fallback layout for the scheduling events, used only when the TSDL
# metadata file cannot be located. Each entry is (name, [(field, type), ...]).
FALLBACK_EVENTS = {
    0x10: ("thread_switched_out", [("thread_id", "uint32_t"), ("name", "str20")]),
    0x11: ("thread_switched_in", [("thread_id", "uint32_t"), ("name", "str20")]),
    0x12: (
        "thread_priority_set",
        [("thread_id", "uint32_t"), ("name", "str20"), ("prio", "int8_t")],
    ),
    0x13: ("thread_create", [("thread_id", "uint32_t"), ("name", "str20")]),
    0x14: ("thread_abort", [("thread_id", "uint32_t"), ("name", "str20")]),
    0x19: (
        "thread_info",
        [
            ("thread_id", "uint32_t"),
            ("name", "str20"),
            ("stack_base", "uint32_t"),
            ("stack_size", "uint32_t"),
        ],
    ),
    0x1A: ("thread_name_set", [("thread_id", "uint32_t"), ("name", "str20")]),
    0x1B: ("isr_enter", []),
    0x1C: ("isr_exit", []),
    0x1D: ("isr_exit_to_scheduler", []),
    0x1E: ("idle", []),
    0x7F: ("thread_sleep_enter", [("timeout", "uint32_t")]),
    0x80: ("thread_sleep_exit", [("timeout", "uint32_t"), ("ret", "int32_t")]),
}


class EventDef:
    """Decoded layout for one CTF event id."""

    __slots__ = ("eid", "name", "fields", "size")

    def __init__(self, eid, name, fields):
        self.eid = eid
        self.name = name
        # fields: list of (field_name, kind) where kind is a struct char for
        # scalars or ("str", n) for a fixed length string. Every field is a
        # fixed size, so the whole record body has a known length (self.size),
        # which the streaming reader uses to spot incomplete trailing records.
        self.fields = []
        self.size = 0
        for fname, ftype in fields:
            if ftype == "str20":
                ftype = ("str", 20)
            if isinstance(ftype, tuple) and ftype[0] == "str":
                self.fields.append((fname, ftype))
                self.size += ftype[1]
            else:
                ch, _ = TYPES[ftype]
                self.fields.append((fname, ch))
                self.size += struct.calcsize(ch)

    def decode(self, buf, off):
        """Decode fields from buf starting at off; return (dict, new_off)."""
        out = {}
        for fname, kind in self.fields:
            if isinstance(kind, tuple) and kind[0] == "str":
                n = kind[1]
                raw = buf[off : off + n]
                off += n
                out[fname] = raw.split(b"\x00", 1)[0].decode("ascii", "replace")
            else:
                sz = struct.calcsize(kind)
                (val,) = struct.unpack_from("<" + kind, buf, off)
                off += sz
                out[fname] = val
        return out, off


def parse_metadata(path):
    """Parse the TSDL metadata file into {id: EventDef}."""
    with safe_open(path, "r", errors="replace") as f:
        text = f.read()

    defs = {}
    # Match each "event { ... };" block. The body can contain a nested
    # "struct { ... }", so balance braces manually rather than with a
    # non-greedy regex (which would stop at the inner closing brace).
    for kw in re.finditer(r"\bevent\s*\{", text):
        start = kw.end()
        depth = 1
        i = start
        while i < len(text) and depth:
            if text[i] == "{":
                depth += 1
            elif text[i] == "}":
                depth -= 1
            i += 1
        body = text[start : i - 1]
        name_m = re.search(r"name\s*=\s*([A-Za-z0-9_]+)\s*;", body)
        id_m = re.search(r"id\s*=\s*(0[xX][0-9a-fA-F]+|\d+)\s*;", body)
        if not name_m or not id_m:
            continue
        eid = int(id_m.group(1), 0)
        name = name_m.group(1)

        fields = []
        struct_m = re.search(r"fields\s*:=\s*struct\s*\{(.*?)\}\s*;", body, re.DOTALL)
        if struct_m:
            for fm in re.finditer(
                r"([A-Za-z_][A-Za-z0-9_]*)\s+([A-Za-z_][A-Za-z0-9_]*)\s*(?:\[(\d+)\])?\s*;",
                struct_m.group(1),
            ):
                ftype, fname, arr = fm.group(1), fm.group(2), fm.group(3)
                if ftype == "ctf_bounded_string_t":
                    fields.append((fname, ("str", int(arr) if arr else 1)))
                elif ftype in TYPES:
                    fields.append((fname, ftype))
                # Unknown types are skipped; they would desync decoding, but the
                # Zephyr metadata only uses the integer aliases and strings.
        defs[eid] = EventDef(eid, name, fields)
    return defs


def build_event_defs(metadata_path):
    if metadata_path and os.path.exists(metadata_path):
        try:
            defs = parse_metadata(metadata_path)
            if defs:
                return defs, metadata_path
        except Exception as exc:  # pragma: no cover - defensive
            sys.stderr.write(f"warning: could not parse {metadata_path}: {exc}\n")
    # Fallback: scheduling events only.
    defs = {eid: EventDef(eid, name, flds) for eid, (name, flds) in FALLBACK_EVENTS.items()}
    return defs, None


def find_metadata(binary_path, override):
    if override:
        return override
    rel = os.path.join("subsys", "tracing", "ctf", "tsdl", "metadata")
    candidates = []
    # $ZEPHYR_BASE, if set.
    zbase = os.environ.get("ZEPHYR_BASE")
    if zbase:
        candidates.append(os.path.join(zbase, rel))
    # Climb parents from this script (scripts/tracing/) looking for the source
    # tree metadata, so the viewer works from anywhere in the tree.
    d = os.path.dirname(os.path.abspath(__file__))
    for _ in range(6):
        candidates.append(os.path.join(d, rel))
        d = os.path.dirname(d)
    # Alongside the trace itself (the layout babeltrace2 expects).
    candidates.append(os.path.join(os.path.dirname(os.path.abspath(binary_path)), "metadata"))
    candidates.append("metadata")
    for c in candidates:
        if os.path.exists(c):
            return c
    return None


class Event:
    __slots__ = ("ts", "eid", "name", "fields")

    def __init__(self, ts, eid, name, fields):
        self.ts = ts
        self.eid = eid
        self.name = name
        self.fields = fields


class Trace:
    """Parsed trace: ordered events plus reconstructed running timeline."""

    def __init__(self):
        self.events = []
        self.threads = {}  # tid -> {name, prio, stack_base, stack_size}
        self.segments = []  # (start_ts, end_ts, tid) running thread spans
        self.isr_spans = []  # (start_ts, end_ts) ISR active spans
        self.states = {}  # tid -> [(start, end, state, reason), ...]
        self.state_starts = {}  # tid -> [start, ...] (for bisect)
        self.t0 = 0
        self.t1 = 0

    def thread(self, tid):
        return self.threads.setdefault(
            tid, {"name": "", "prio": None, "stack_base": None, "stack_size": None}
        )


class TraceReader:
    """Incrementally decode a CTF stream into a Trace.

    Because every record has a known fixed length, the reader can be fed bytes
    in arbitrary chunks: complete records are decoded and any partial trailing
    record is kept until the rest arrives. This is what lets the viewer follow
    a tracing.bin file that the application is still writing.

    All derived structures (running segments, ISR spans and the per-thread
    state timeline) are maintained incrementally. The currently-open state of
    each thread is published as a provisional segment extending to the latest
    timestamp so the live edge renders; it is dropped and re-derived on the
    next feed.
    """

    def __init__(self, defs, has_ts=True):
        self.defs = defs
        self.has_ts = has_ts
        self.hdr = HDR if has_ts else HDR_NO_TS
        self.tr = Trace()
        self._buf = b""  # undecoded bytes (possibly a partial record)
        self._fake_ts = 0
        self._desync = False
        # Some platforms back the CTF timestamp with a free-running cycle
        # counter that wraps, giving a sawtooth instead of a monotonic clock.
        # Unwrap it: every time the raw value jumps backwards, add the previous
        # raw value as an offset so the timeline stays monotonic. For a trace
        # whose timestamps are already monotonic this is a no-op.
        self._prev_raw = None
        self._ts_off = 0
        # running-thread / ISR incremental state
        self._cur_tid = None
        self._seg_start = None
        self._isr_depth = 0
        self._isr_start = None
        # per-thread state-machine incremental state
        self._st_cur = {}  # tid -> [state, since, reason]
        self._st_hint = {}  # tid -> (state, reason)
        self._running = None
        self._provisional = []  # tids whose open state is appended for render

    # --- incremental state-machine helpers --------------------------------
    def _st_close(self, tid, ts):
        st = self._st_cur.get(tid)
        if st is not None and ts > st[1]:
            self.tr.states.setdefault(tid, []).append((st[1], ts, st[0], st[2]))
            self.tr.state_starts.setdefault(tid, []).append(st[1])

    def _st_set(self, tid, ts, state, reason=""):
        self._st_close(tid, ts)
        self._st_cur[tid] = [state, ts, reason]

    def _drop_provisional(self):
        for tid in self._provisional:
            segs = self.tr.states.get(tid)
            if segs:
                segs.pop()
                self.tr.state_starts[tid].pop()
        self._provisional = []

    def _add_provisional(self):
        last = self.tr.t1
        for tid, st in self._st_cur.items():
            state, since, reason = st
            if last > since and state != ST_DEAD:
                self.tr.states.setdefault(tid, []).append((since, last, state, reason))
                self.tr.state_starts.setdefault(tid, []).append(since)
                self._provisional.append(tid)

    def _state_machine(self, ts, nm, fields, tid):
        if nm == "thread_switched_in":
            self._running = tid
            if tid is not None:
                self._st_set(tid, ts, ST_RUN)
                self._st_hint.pop(tid, None)
        elif nm == "thread_switched_out":
            t = tid if tid is not None else self._running
            if t is not None and self._st_cur.get(t, [None])[0] == ST_RUN:
                h = self._st_hint.pop(t, None)
                if h:
                    self._st_set(t, ts, h[0], h[1])
                else:
                    self._st_set(t, ts, ST_RDY)  # preempted, still runnable
            self._running = None
        elif nm in _SLEEP_ENTERS:
            if self._running is not None:
                to = fields.get("timeout", fields.get("ms", fields.get("us", "")))
                self._st_hint[self._running] = (ST_SLP, f"sleep {to}")
        elif nm.endswith("_blocking"):
            if self._running is not None:
                self._st_hint[self._running] = (ST_BLK, _block_reason_nm(nm, fields))
        elif nm in _PEND_EVENTS:
            if tid is not None:
                h = self._st_hint.get(tid)
                self._st_set(tid, ts, ST_BLK, h[1] if h else "blocked")
        elif nm in _READY_EVENTS:
            if tid is not None:
                self._st_set(tid, ts, ST_RDY)
                self._st_hint.pop(tid, None)
        elif nm in _SUSPEND_EVENTS:
            if tid is not None:
                self._st_set(tid, ts, ST_SUS)
        elif nm in _ABORT_EVENTS:
            if tid is not None:
                self._st_set(tid, ts, ST_DEAD)
        elif nm == "thread_create":
            if tid is not None and tid not in self._st_cur:
                self._st_set(tid, ts, ST_RDY)

    def _consume(self, ts, eid, name, fields):
        tr = self.tr
        tr.events.append(Event(ts, eid, name, fields))
        if len(tr.events) == 1:
            tr.t0 = ts
        tr.t1 = ts

        tid = fields.get("thread_id")
        if tid is not None:
            t = tr.thread(tid)
            nm = fields.get("name")
            if nm:
                t["name"] = nm
            if eid in (THREAD_PRIO_SET, THREAD_SCHED_PRIO_SET) and "prio" in fields:
                t["prio"] = fields["prio"]
            if eid == THREAD_INFO:
                t["stack_base"] = fields.get("stack_base")
                t["stack_size"] = fields.get("stack_size")

        # Running-thread timeline from context switches.
        if eid == THREAD_SWITCHED_IN:
            if self._cur_tid is not None and self._seg_start is not None:
                tr.segments.append((self._seg_start, ts, self._cur_tid))
            self._cur_tid = tid
            self._seg_start = ts
        elif eid == THREAD_SWITCHED_OUT:
            if self._cur_tid is not None and self._seg_start is not None:
                tr.segments.append((self._seg_start, ts, self._cur_tid))
            self._cur_tid = None
            self._seg_start = None

        # ISR overlay spans (nested enters collapse to one active span).
        if eid == ISR_ENTER:
            if self._isr_depth == 0:
                self._isr_start = ts
            self._isr_depth += 1
        elif eid in (ISR_EXIT, ISR_EXIT_TO_SCHEDULER):
            if self._isr_depth > 0:
                self._isr_depth -= 1
                if self._isr_depth == 0 and self._isr_start is not None:
                    tr.isr_spans.append((self._isr_start, ts))
                    self._isr_start = None

        self._state_machine(ts, name, fields, tid)

    def _decode_buf(self):
        data = self._buf
        off = 0
        n = len(data)
        hsz = self.hdr.size
        new = 0
        while off + hsz <= n:
            if self.has_ts:
                ts, eid = self.hdr.unpack_from(data, off)
            else:
                (eid,) = self.hdr.unpack_from(data, off)
                ts = None
            edef = self.defs.get(eid)
            if edef is None:
                # Unknown id: the record length is unknown, so we cannot safely
                # skip it. Stop and keep the bytes; flag the desync for the UI.
                self._desync = True
                break
            rec = hsz + edef.size
            if off + rec > n:
                break  # incomplete trailing record; wait for more
            if ts is None:
                ts = self._fake_ts
                self._fake_ts += 1
            else:
                if self._prev_raw is not None and ts < self._prev_raw:
                    self._ts_off += self._prev_raw  # counter wrapped
                self._prev_raw = ts
                ts += self._ts_off
            fields, _ = edef.decode(data, off + hsz)
            off += rec
            self._consume(ts, eid, edef.name, fields)
            new += 1
        self._buf = data[off:]
        return new

    def feed(self, data):
        """Decode as many complete records as `data` (appended) allows.

        Returns the number of new events decoded.
        """
        if not data:
            return 0
        self._buf += data
        self._drop_provisional()
        new = self._decode_buf()
        self._add_provisional()
        return new


def parse_trace(path, defs, has_ts=True):
    """Read a complete trace file in one shot (non-live path)."""
    reader = TraceReader(defs, has_ts)
    with safe_open(path, "rb") as fh:
        reader.feed(fh.read())
    return reader.tr


# Thread state codes. The glyph ramp goes from solid (on CPU) to light (idle),
# so darker == closer to running; colour disambiguates further.
ST_RUN = "run"  # executing on a CPU
ST_RDY = "rdy"  # runnable, waiting for a CPU (preempted / just readied)
ST_BLK = "blk"  # pending on a kernel object (see reason)
ST_SLP = "slp"  # sleeping on a timeout (k_sleep and friends)
ST_SUS = "sus"  # explicitly suspended
ST_DEAD = "dead"  # aborted / not yet created

STATE_GLYPH = {ST_RUN: "█", ST_RDY: "▓", ST_BLK: "▒", ST_SLP: "░", ST_SUS: "—", ST_DEAD: " "}
STATE_NAME = {
    ST_RUN: "run",
    ST_RDY: "ready",
    ST_BLK: "blocked",
    ST_SLP: "sleep",
    ST_SUS: "susp",
    ST_DEAD: "dead",
}
# Precedence for choosing one state when a column's time bucket spans several;
# running wins ties so context switches stay visible.
STATE_PREC = {ST_RUN: 5, ST_BLK: 4, ST_RDY: 3, ST_SLP: 2, ST_SUS: 1, ST_DEAD: 0}
# curses colour pair per state (set up in run_curses).
STATE_PAIR = {ST_RUN: 1, ST_RDY: 2, ST_BLK: 3, ST_SLP: 4, ST_SUS: 5}

_SLEEP_ENTERS = {
    "k_sleep_enter",
    "thread_sleep_enter",
    "thread_msleep_enter",
    "thread_usleep_enter",
}
_READY_EVENTS = {
    "thread_sched_ready",
    "thread_ready",
    "thread_wakeup",
    "thread_sched_wakeup",
    "thread_resume",
    "thread_sched_resume",
}
_SUSPEND_EVENTS = {"thread_suspend", "thread_sched_suspend"}
_ABORT_EVENTS = {"thread_abort", "thread_sched_abort"}
_PEND_EVENTS = {"thread_sched_pend", "thread_pending"}


def _block_reason_nm(nm, f):
    """Best-effort 'why is this thread blocked' string for a blocking event."""
    if nm.startswith("semaphore"):
        return f"sem 0x{f.get('id', 0):x}"
    if nm.startswith("mutex"):
        return f"mutex 0x{f.get('id', 0):x}"
    if nm.startswith("msgq"):
        return f"msgq 0x{f.get('id', 0):x}"
    if nm.startswith("condvar"):
        return f"condvar 0x{f.get('id', 0):x}"
    if nm.startswith("event_wait"):
        return f"event 0x{f.get('event_id', 0):x}"
    if nm.startswith("thread_join"):
        return f"join 0x{f.get('thread_id', 0):x}"
    if nm.startswith("mem_slab"):
        return f"memslab 0x{f.get('id', 0):x}"
    if nm.startswith("work"):
        return "work"
    return "blocked"


def state_at(tr, tid, ts):
    """Return (state, reason) of thread tid at time ts, or (None, '')."""
    import bisect

    starts = tr.state_starts.get(tid)
    if not starts:
        return None, ""
    i = bisect.bisect_right(starts, ts) - 1
    if i < 0:
        return None, ""
    s, e, state, reason = tr.states[tid][i]
    if s <= ts < e or (i == len(starts) - 1 and ts >= s):
        return state, reason
    return None, ""


def thread_running_at(tr, ts):
    """Return the thread id running at time ts (single-CPU), or None."""
    for tid in tr.threads:
        st, _ = state_at(tr, tid, ts)
        if st == ST_RUN:
            return tid
    return None


# --------------------------------------------------------------------------
# Rendering helpers shared by the text and curses front-ends.
# --------------------------------------------------------------------------


def lane_order(tr):
    """Return thread ids ordered for display (most-active first)."""
    busy = {}
    for s, e, tid in tr.segments:
        busy[tid] = busy.get(tid, 0) + max(0, e - s)
    # Threads that ran, busiest first; then any known-but-idle threads.
    ran = sorted(busy, key=lambda t: -busy[t])
    others = [t for t in tr.threads if t not in busy]
    return ran + others


def thread_label(tr, tid):
    name = tr.threads.get(tid, {}).get("name") or ""
    if not name:
        # Idle threads are often unnamed; flag the well-known idle id heuristic
        # by name only, otherwise show the handle.
        name = "(unnamed)"
    return name


def fmt_time(ns):
    """Human friendly timestamp."""
    if ns >= 1_000_000:
        return f"{ns / 1_000_000:.3f}ms"
    if ns >= 1_000:
        return f"{ns / 1_000:.3f}us"
    return f"{ns}ns"


def render_rows(tr, order, view0, view1, width):
    """Return {tid: list[str-cells]} bars plus an ISR row, for the time window.

    Each cell is one of ' ' (idle), '#'/full block (running). We use shading to
    indicate partial coverage of a column's time bucket.
    """
    span = max(1, view1 - view0)
    col_ns = span / width
    shades = " ░▒▓█"

    rows = {}
    for tid in order:
        rows[tid] = [0.0] * width
    isr_row = [0.0] * width

    def paint(acc, s, e):
        if e <= view0 or s >= view1:
            return
        s = max(s, view0)
        e = min(e, view1)
        c0 = (s - view0) / col_ns
        c1 = (e - view0) / col_ns
        ic0 = int(c0)
        ic1 = min(width - 1, int(c1))
        for c in range(ic0, ic1 + 1):
            cell0 = c
            cell1 = c + 1
            cov = min(cell1, c1) - max(cell0, c0)
            if cov > 0:
                acc[c] = min(1.0, acc[c] + cov)

    for s, e, tid in tr.segments:
        if tid in rows:
            paint(rows[tid], s, e)
    for s, e in tr.isr_spans:
        paint(isr_row, s, e)

    def to_chars(acc):
        out = []
        for v in acc:
            if v <= 0.0:
                out.append(" ")
            else:
                idx = min(4, 1 + int(v * 3.999))
                out.append(shades[idx])
        return out

    char_rows = {tid: to_chars(acc) for tid, acc in rows.items()}
    return char_rows, to_chars(isr_row)


def render_state_rows(tr, lanes, view0, view1, width):
    """Return {tid: [state-or-None per column]} for the visible window.

    For each column the dominant state (by time covered, ties broken by
    STATE_PREC) is chosen, so a lane reads as run/ready/blocked/sleep bars.
    """
    import bisect

    span = max(1, view1 - view0)
    col_ns = span / width
    out = {}
    for tid in lanes:
        cells = [None] * width
        segs = tr.states.get(tid)
        if segs:
            starts = tr.state_starts[tid]
            acc = {}  # col -> {state: coverage}
            i = max(0, bisect.bisect_right(starts, view0) - 1)
            n = len(segs)
            while i < n:
                s, e, state, _reason = segs[i]
                i += 1
                if s >= view1:
                    break
                if e <= view0 or state == ST_DEAD:
                    continue
                cs = max(s, view0)
                ce = min(e, view1)
                c0 = (cs - view0) / col_ns
                c1 = (ce - view0) / col_ns
                for c in range(int(c0), min(width - 1, int(c1)) + 1):
                    cov = min(c + 1, c1) - max(c, c0)
                    if cov <= 0:
                        continue
                    d = acc.get(c)
                    if d is None:
                        d = {}
                        acc[c] = d
                    d[state] = d.get(state, 0) + cov
            for c, d in acc.items():
                best = None
                bestv = -1.0
                for st, v in d.items():
                    if v > bestv or (v == bestv and STATE_PREC[st] > STATE_PREC.get(best, -1)):
                        best, bestv = st, v
                cells[c] = best
        out[tid] = cells
    return out


def window_stats(tr, view0, view1):
    """Per-thread time in each state over [view0, view1].

    Returns ({tid: {state: ns}}, span_ns). Threads that exist in the window
    have segments covering it, so their per-state times sum to the span.
    """
    import bisect

    span = max(1, view1 - view0)
    per = {}
    for tid, segs in tr.states.items():
        starts = tr.state_starts[tid]
        i = max(0, bisect.bisect_right(starts, view0) - 1)
        n = len(segs)
        acc = {}
        while i < n:
            s, e, st, _r = segs[i]
            i += 1
            if s >= view1:
                break
            if e <= view0:
                continue
            d = min(e, view1) - max(s, view0)
            if d > 0:
                acc[st] = acc.get(st, 0) + d
        if acc:
            per[tid] = acc
    return per, span


# --------------------------------------------------------------------------
# Non-interactive ASCII output.
# --------------------------------------------------------------------------


def run_text(tr, width):
    order = lane_order(tr)
    label_w = max([len("THREAD")] + [len(thread_label(tr, t)) for t in order]) + 1
    label_w = min(label_w, 18)
    tl_w = max(20, width - label_w - 12)

    print(
        f"Zephyr CTF trace  ({len(tr.events)} events, "
        f"{len(tr.threads)} threads, "
        f"{fmt_time(tr.t1 - tr.t0)} span)"
    )
    print(f"t0 = {fmt_time(tr.t0)}   t1 = {fmt_time(tr.t1)}")
    print()

    state_rows = render_state_rows(tr, order, tr.t0, tr.t1, tl_w)
    _, isr_row = render_rows(tr, [], tr.t0, tr.t1, tl_w)

    def lbl(s):
        s = s[: label_w - 1]
        return s.ljust(label_w)

    legend = "  ".join(
        f"{STATE_GLYPH[s]}={STATE_NAME[s]}" for s in (ST_RUN, ST_RDY, ST_BLK, ST_SLP, ST_SUS)
    )
    print("states: " + legend)
    print()
    axis = "".join("|" if i % 10 == 0 else "." for i in range(tl_w))
    print(
        " " * label_w
        + "0"
        + " " * (tl_w - len(fmt_time(tr.t1 - tr.t0)) - 1)
        + fmt_time(tr.t1 - tr.t0)
    )
    print(lbl("THREAD") + axis)
    for tid in order:
        cells = state_rows[tid]
        if all(c is None for c in cells):
            continue
        line = "".join(STATE_GLYPH.get(c, " ") if c else " " for c in cells)
        print(lbl(thread_label(tr, tid)) + line)
    if any(c != " " for c in isr_row):
        print(lbl("[ISR]") + "".join(isr_row))

    print()
    print("Threads")
    print(f"  {'handle':<12}{'name':<18}{'prio':>5}  {'stack_base':<12}{'stack_sz':>9}")
    for tid in order:
        t = tr.threads[tid]
        prio = "" if t["prio"] is None else str(t["prio"])
        sb = "" if t["stack_base"] is None else f"0x{t['stack_base']:08x}"
        ss = "" if t["stack_size"] is None else str(t["stack_size"])
        print(f"  0x{tid:08x}  {thread_label(tr, tid):<18}{prio:>5}  {sb:<12}{ss:>9}")

    # Event histogram by type.
    print()
    print("Event counts")
    counts = {}
    for ev in tr.events:
        counts[ev.name] = counts.get(ev.name, 0) + 1
    for name, c in sorted(counts.items(), key=lambda kv: -kv[1]):
        print(f"  {c:>7}  {name}")


# --------------------------------------------------------------------------
# Interactive curses front-end.
# --------------------------------------------------------------------------


def _fmt_fields(ev):
    """One-line rendering of an event's decoded fields."""
    parts = []
    for k, v in ev.fields.items():
        if isinstance(v, int) and (
            k.endswith("id") or k.endswith("base") or k in ("port", "pin", "iface", "pkt")
        ):
            parts.append(f"{k}=0x{v:x}")
        else:
            parts.append(f"{k}={v}")
    return "  ".join(parts)


def run_curses(stdscr, reader, fh=None):
    import bisect
    import curses
    import time

    tr = reader.tr
    live = fh is not None

    curses.curs_set(0)
    stdscr.keypad(True)

    use_color = curses.has_colors()
    if use_color:
        curses.start_color()
        curses.use_default_colors()
        # Lanes are coloured by thread *state* (not identity) so run / ready /
        # blocked / sleep / suspended read at a glance.
        curses.init_pair(1, curses.COLOR_GREEN, -1)  # running
        curses.init_pair(2, curses.COLOR_YELLOW, -1)  # ready
        curses.init_pair(3, curses.COLOR_RED, -1)  # blocked
        curses.init_pair(4, curses.COLOR_CYAN, -1)  # sleeping
        curses.init_pair(5, curses.COLOR_MAGENTA, -1)  # suspended
        curses.init_pair(20, curses.COLOR_BLACK, curses.COLOR_RED)  # ISR
        curses.init_pair(21, curses.COLOR_BLACK, curses.COLOR_CYAN)  # header/cursor
        curses.init_pair(22, curses.COLOR_WHITE, curses.COLOR_BLUE)  # selected
        curses.init_pair(23, curses.COLOR_BLACK, curses.COLOR_GREEN)  # play

    order = lane_order(tr)
    ts_list = [ev.ts for ev in tr.events]

    full_span = max(1, tr.t1 - tr.t0)
    view0 = tr.t0
    view1 = tr.t1 if tr.t1 > tr.t0 else tr.t0 + 1
    cursor_ns = float(tr.t0)
    sel_row = 0  # selected thread lane
    show_events = True  # event-density row
    show_info = True  # compact info panel under the chart
    show_metrics = True  # metrics panel in the gap below the lanes
    mode = "gantt"  # "gantt" or "log"
    log_scroll = 0  # top event index shown in the log view

    # Autoplay: cursor advances by `speed` trace-ns per real second. Default to
    # replaying the whole trace in ~20 seconds.
    playing = False
    speed = max(1.0, full_span / 20.0)
    last_tick = time.monotonic()

    # Live follow: when reading a file that is still being written, keep the
    # view pinned to the latest events. Panning/zooming detaches; 'f' re-pins.
    live_follow = live
    nthreads = len(tr.threads)

    def clampview():
        nonlocal view0, view1
        span = view1 - view0
        if span < 100:
            span = 100
            view1 = view0 + span
        if view0 < tr.t0:
            view0 = tr.t0
            view1 = view0 + span
        if view1 > tr.t1:
            view1 = tr.t1
            view0 = max(tr.t0, view1 - span)

    def running_at(ts):
        return thread_running_at(tr, ts)

    def nearest_idx(ts):
        if not ts_list:
            return 0
        i = bisect.bisect_left(ts_list, ts)
        if i <= 0:
            return 0
        if i >= len(ts_list):
            return len(ts_list) - 1
        return i if (ts_list[i] - ts) < (ts - ts_list[i - 1]) else i - 1

    def fmt_speed(sp):
        return f"{fmt_time(int(sp))}/s"

    while True:
        # --- Poll the file for newly written events -----------------------
        if live:
            try:
                chunk = fh.read()
            except OSError:
                chunk = b""
            if reader.feed(chunk):
                ts_list.extend(ev.ts for ev in tr.events[len(ts_list) :])
                full_span = max(1, tr.t1 - tr.t0)
                if len(tr.threads) != nthreads:
                    nthreads = len(tr.threads)
                    order = lane_order(tr)
            if live_follow and tr.t1 > tr.t0:
                span = max(100, view1 - view0)
                view1 = tr.t1
                view0 = max(tr.t0, view1 - span)
                cursor_ns = float(tr.t1)
                log_scroll = len(tr.events)  # keep log view at the newest row

        # --- Advance autoplay clock ---------------------------------------
        now = time.monotonic()
        dt = now - last_tick
        last_tick = now
        if playing:
            cursor_ns += speed * dt
            if cursor_ns >= tr.t1:
                cursor_ns = float(tr.t1)
                playing = False
            # Scroll the window so the playhead stays visible (~70% across).
            span = view1 - view0
            if cursor_ns > view1 or cursor_ns < view0:
                view0 = cursor_ns - span * 0.7
                view1 = view0 + span
                clampview()

        stdscr.erase()
        h, w = stdscr.getmaxyx()
        if h < 10 or w < 50:
            stdscr.addstr(0, 0, "Terminal too small (need >=50x10).")
            stdscr.refresh()
            stdscr.timeout(-1)
            if stdscr.getch() in (ord("q"), 27):
                return
            continue

        clampview()
        span = view1 - view0
        cur_i = nearest_idx(int(cursor_ns))

        # --- Header (both modes) ------------------------------------------
        if live:
            play_tag = " LIVE>> " if live_follow else " LIVE   "
        elif playing:
            play_tag = f" PLAY {fmt_speed(speed)} "
        else:
            play_tag = " PAUSED "
        hdr = (
            f" CTF {('LOG' if mode == 'log' else 'GANTT')}  "
            f"ev={len(tr.events)} thr={len(tr.threads)}  "
            f"t={fmt_time(int(cursor_ns) - tr.t0)}"
        )
        hattr = curses.color_pair(21) if use_color else curses.A_REVERSE
        stdscr.addstr(0, 0, hdr[: w - 1].ljust(w - 1 - len(play_tag)), hattr)
        hot = playing or (live and live_follow)
        pattr = (
            curses.color_pair(23)
            if (hot and use_color)
            else (curses.A_REVERSE if not use_color else curses.color_pair(22))
        )
        stdscr.addstr(0, max(0, w - 1 - len(play_tag)), play_tag[: w - 1], pattr)

        if mode == "log":
            _draw_log(stdscr, tr, h, w, cur_i, log_scroll, use_color, curses)
        else:
            ev_lo = bisect.bisect_left(ts_list, view0)
            ev_hi = bisect.bisect_right(ts_list, view1)
            sel_row = _draw_gantt(
                stdscr,
                tr,
                h,
                w,
                view0,
                view1,
                span,
                cursor_ns,
                order,
                sel_row,
                show_events,
                show_info,
                show_metrics,
                use_color,
                curses,
                running_at,
                ev_lo,
                ev_hi,
            )

        # Footer help.
        latest = "[End/f]latest " if live else "[End]latest "
        if mode == "log":
            help_line = f"{latest}[space]play [up/dn]scroll [PgUp/Dn] [g]antt-view [q]uit"
        else:
            help_line = (
                f"{latest}[space]play [<-/->]cursor [+/-]zoom "
                "[up/dn]lane [a]ll [v]log [m]etrics [i]nfo [q]uit"
            )
        stdscr.addstr(h - 1, 0, help_line[: w - 1], hattr)
        stdscr.refresh()

        # --- Input --------------------------------------------------------
        # Block when idle; poll periodically when live or auto-playing.
        stdscr.timeout(33 if playing else (150 if live else -1))
        k = stdscr.getch()
        if k == -1:
            continue

        # End / f: jump to the latest state and re-sync with the newest events
        # (and resume live-follow). Works in both the Gantt and log views.
        if k in (ord("f"), curses.KEY_END):
            jump_span = max(100, view1 - view0)
            view1 = tr.t1
            view0 = max(tr.t0, view1 - jump_span)
            cursor_ns = float(tr.t1)
            log_scroll = len(tr.events)
            if live:
                live_follow = True
            continue
        # Navigating away from the live edge detaches follow.
        if live:
            nav = (
                {
                    curses.KEY_UP,
                    curses.KEY_DOWN,
                    curses.KEY_PPAGE,
                    curses.KEY_NPAGE,
                    curses.KEY_HOME,
                }
                if mode == "log"
                else {
                    curses.KEY_LEFT,
                    curses.KEY_RIGHT,
                    ord(","),
                    ord("."),
                    ord("+"),
                    ord("="),
                    ord("-"),
                    ord("_"),
                    ord("a"),
                    curses.KEY_HOME,
                    curses.KEY_NPAGE,
                    curses.KEY_PPAGE,
                }
            )
            if k in nav:
                live_follow = False

        step = max(1, span // 40)
        pan = max(1, span // 4)

        # Keys common to both views.
        if k in (ord("q"), 27):
            return
        elif k == ord(" "):
            playing = not playing
            last_tick = time.monotonic()
        elif k in (ord("]"), ord(">")):
            speed = min(full_span * 4.0, speed * 1.5)
        elif k in (ord("["), ord("<")):
            speed = max(1.0, speed / 1.5)
        elif k in (ord("v"), ord("g"), ord("\t")):
            mode = "log" if mode == "gantt" else "gantt"
            log_scroll = max(0, cur_i - (h // 2))
            continue

        if mode == "log":
            page = max(1, h - 4)
            if k == curses.KEY_UP:
                log_scroll = max(0, log_scroll - 1)
            elif k == curses.KEY_DOWN:
                log_scroll = min(max(0, len(tr.events) - 1), log_scroll + 1)
            elif k == curses.KEY_PPAGE:
                log_scroll = max(0, log_scroll - page)
            elif k == curses.KEY_NPAGE:
                log_scroll = min(max(0, len(tr.events) - 1), log_scroll + page)
            elif k == curses.KEY_HOME:
                log_scroll = 0
            continue

        # Gantt-only keys.
        if k == curses.KEY_LEFT:
            cursor_ns = max(view0, cursor_ns - step)
        elif k == curses.KEY_RIGHT:
            cursor_ns = min(view1, cursor_ns + step)
        elif k == ord(","):
            view0 -= pan
            view1 -= pan
            clampview()
            cursor_ns = max(view0, min(view1, cursor_ns))
        elif k == ord("."):
            view0 += pan
            view1 += pan
            clampview()
            cursor_ns = max(view0, min(view1, cursor_ns))
        elif k in (ord("+"), ord("=")):
            new_span = max(100, span // 2)
            view0 = cursor_ns - new_span // 2
            view1 = view0 + new_span
            clampview()
        elif k in (ord("-"), ord("_")):
            new_span = min(full_span, span * 2)
            view0 = cursor_ns - new_span // 2
            view1 = view0 + new_span
            clampview()
        elif k == ord("a"):
            view0, view1 = tr.t0, tr.t1
        elif k == curses.KEY_HOME:
            cursor_ns = float(view0)
        elif k == curses.KEY_UP:
            sel_row = max(0, sel_row - 1)
        elif k == curses.KEY_DOWN:
            sel_row += 1
        elif k == ord("e"):
            show_events = not show_events
        elif k == ord("i"):
            show_info = not show_info
        elif k == ord("m"):
            show_metrics = not show_metrics
        elif k == curses.KEY_NPAGE:
            view0 += pan * 2
            view1 += pan * 2
            clampview()
        elif k == curses.KEY_PPAGE:
            view0 -= pan * 2
            view1 -= pan * 2
            clampview()


def _draw_gantt(
    stdscr,
    tr,
    h,
    w,
    view0,
    view1,
    span,
    cursor_ns,
    order,
    sel_row,
    show_events,
    show_info,
    show_metrics,
    use_color,
    curses,
    running_at,
    ev_lo,
    ev_hi,
):
    """Render the Gantt timeline; returns the clamped selected lane index."""
    label_w = 14
    tl_w = w - label_w - 1

    def st_attr(state):
        if use_color and state in STATE_PAIR:
            a = curses.color_pair(STATE_PAIR[state])
            return a | curses.A_BOLD if state == ST_RUN else a
        return curses.A_BOLD if state == ST_RUN else curses.A_NORMAL

    # Time-axis ruler with both end labels.
    stdscr.addstr(1, 0, "t".ljust(label_w))
    ruler = "".join("|" if i % 10 == 0 else "-" for i in range(tl_w))
    stdscr.addstr(1, label_w, ruler[:tl_w])
    left_lbl = fmt_time(view0 - tr.t0)
    right_lbl = fmt_time(view1 - tr.t0)
    stdscr.addstr(2, label_w, left_lbl[:tl_w])
    if len(right_lbl) < tl_w:
        stdscr.addstr(2, label_w + tl_w - len(right_lbl), right_lbl)

    # The info panel at the bottom is intentionally small so the chart gets
    # the bulk of the screen; toggle it off entirely with 'i'.
    panel_h = 5 if show_info else 0
    lanes_top = 3
    overhead = lanes_top + panel_h + 1  # +1 footer
    visible = [
        t for t in order if any(tt == t and s <= view1 and e >= view0 for s, e, tt in tr.segments)
    ] or order
    max_lanes = max(1, h - overhead - 1)  # -1 for the events row
    view_lanes = visible[:max_lanes]
    sel_row = max(0, min(sel_row, len(view_lanes) - 1)) if view_lanes else 0

    state_rows = render_state_rows(tr, view_lanes, view0, view1, tl_w)
    _, isr_row = render_rows(tr, [], view0, view1, tl_w)
    cursor_col = int((cursor_ns - view0) / max(1, span) * tl_w)
    cursor_col = max(0, min(tl_w - 1, cursor_col))

    row_y = lanes_top
    for idx, tid in enumerate(view_lanes):
        sel = idx == sel_row
        lbl = thread_label(tr, tid)[: label_w - 1].ljust(label_w)
        lblattr = (
            curses.color_pair(22)
            if (sel and use_color)
            else (curses.A_BOLD if sel else curses.A_NORMAL)
        )
        stdscr.addstr(row_y, 0, lbl, lblattr)
        # Draw each lane as runs of same-state glyphs, coloured per state.
        cells = state_rows[tid]
        c = 0
        while c < tl_w:
            st = cells[c]
            run = c + 1
            while run < tl_w and cells[run] == st:
                run += 1
            glyph = STATE_GLYPH.get(st, " ") if st else " "
            if glyph != " ":
                stdscr.addstr(row_y, label_w + c, glyph * (run - c), st_attr(st))
            c = run
        # Playhead marker.
        st = cells[cursor_col] if cursor_col < len(cells) else None
        mark = STATE_GLYPH.get(st, "|") if st else "|"
        stdscr.addstr(
            row_y,
            label_w + cursor_col,
            mark,
            curses.color_pair(21) if use_color else curses.A_REVERSE,
        )
        row_y += 1

    if any(c != " " for c in isr_row) and row_y < h - panel_h - 1:
        stdscr.addstr(row_y, 0, "[ISR]".ljust(label_w), curses.A_BOLD)
        iattr = curses.color_pair(20) if use_color else curses.A_REVERSE
        stdscr.addstr(row_y, label_w, "".join(isr_row)[:tl_w], iattr)
        row_y += 1

    if show_events and row_y < h - panel_h - 1:
        marks = [" "] * tl_w
        bucket = {}
        for ev in tr.events[ev_lo:ev_hi]:
            if ev.eid in (
                THREAD_SWITCHED_IN,
                THREAD_SWITCHED_OUT,
                ISR_ENTER,
                ISR_EXIT,
                ISR_EXIT_TO_SCHEDULER,
                IDLE,
            ):
                continue
            c = int((ev.ts - view0) / max(1, span) * tl_w)
            c = max(0, min(tl_w - 1, c))
            bucket[c] = bucket.get(c, 0) + 1
        dens = " .:+*#@"
        for c, n in bucket.items():
            marks[c] = dens[min(len(dens) - 1, n)]
        marks[cursor_col] = "|" if marks[cursor_col] == " " else marks[cursor_col]
        stdscr.addstr(row_y, 0, "events".ljust(label_w), curses.A_DIM)
        stdscr.addstr(row_y, label_w, "".join(marks)[:tl_w])
        row_y += 1

    # --- Metrics panel (fills the gap above the info strip) ---------------
    if show_metrics:
        gap_bot = (h - panel_h) if show_info else (h - 1)
        if gap_bot - row_y >= 3:
            _draw_metrics(
                stdscr,
                tr,
                row_y,
                gap_bot,
                w,
                view0,
                view1,
                view_lanes,
                ev_lo,
                ev_hi,
                curses,
                st_attr,
            )

    # --- Compact info panel -----------------------------------------------
    if show_info:
        py = h - panel_h
        stdscr.hline(py, 0, curses.ACS_HLINE, w)
        py += 1
        # Colour-coded state legend.
        stdscr.addstr(py, 0, " states: ")
        x = 9
        for st in (ST_RUN, ST_RDY, ST_BLK, ST_SLP, ST_SUS):
            seg = f"{STATE_GLYPH[st]} {STATE_NAME[st]}  "
            stdscr.addstr(py, x, seg, st_attr(st))
            x += len(seg)
        py += 1
        # Running thread at the playhead.
        cur_tid = running_at(int(cursor_ns))
        runlbl = thread_label(tr, cur_tid) if cur_tid is not None else "(none)"
        runhandle = f"0x{cur_tid:08x}" if cur_tid is not None else ""
        stdscr.addstr(
            py,
            0,
            (f" running: {runlbl} {runhandle}".ljust(w - 1))[: w - 1],
            curses.color_pair(21) if use_color else curses.A_REVERSE,
        )
        py += 1
        # Selected lane: its state and, when blocked, why.
        sel_tid = view_lanes[sel_row] if view_lanes else None
        if sel_tid is not None:
            t = tr.threads[sel_tid]
            prio = "-" if t["prio"] is None else str(t["prio"])
            ss = "-" if t["stack_size"] is None else f"{t['stack_size']}B"
            st, reason = state_at(tr, sel_tid, int(cursor_ns))
            stext = STATE_NAME.get(st, "-")
            if st == ST_BLK and reason:
                stext = f"blocked on {reason}"
            elif st == ST_SLP and reason:
                stext = reason
            det = (
                f" lane: {thread_label(tr, sel_tid)} 0x{sel_tid:08x}  "
                f"prio={prio} stack={ss}  ->  {stext}"
            )
            stdscr.addstr(py, 0, det[: w - 1].ljust(w - 1), st_attr(st) if st else curses.A_BOLD)

    return sel_row


def _draw_metrics(stdscr, tr, y0, y1, w, view0, view1, lanes, ev_lo, ev_hi, curses, st_attr):
    """Draw key metrics over the visible window in the empty area below the
    lanes: a CPU-busy gauge, context-switch / event rates, and per-thread
    stacked utilization bars (run/ready/blocked/sleep)."""
    per, span = window_stats(tr, view0, view1)
    idle_ids = {t for t in tr.threads if tr.threads[t].get("name") == "idle"}
    run_total = sum(a.get(ST_RUN, 0) for a in per.values())
    idle_run = sum(per.get(t, {}).get(ST_RUN, 0) for t in idle_ids)
    busy = max(0.0, min(1.0, (run_total - idle_run) / span))
    switches = sum(1 for ev in tr.events[ev_lo:ev_hi] if ev.eid == THREAD_SWITCHED_IN)
    nev = ev_hi - ev_lo
    secs = span / 1e9

    def bar(y, x, width, parts, empty="░"):
        cx, used = x, 0
        for st, fr in parts:
            cnt = min(int(round(fr * width)), width - used)
            if cnt <= 0:
                continue
            stdscr.addstr(y, cx, STATE_GLYPH.get(st, "█") * cnt, st_attr(st))
            cx += cnt
            used += cnt
        if used < width:
            stdscr.addstr(y, cx, empty * (width - used), curses.A_DIM)

    y = y0
    stdscr.hline(y, 0, curses.ACS_HLINE, w)
    y += 1
    if y >= y1:
        return
    gw = 14
    head = f" CPU {busy * 100:3.0f}% "
    stdscr.addstr(y, 0, head, curses.A_BOLD)
    bar(y, len(head), gw, [(ST_RUN, busy)])
    sw_rate = f"{switches / secs:.0f}/s" if secs > 0 else "-"
    ev_rate = f"{nev / secs:.0f}/s" if secs > 0 else "-"
    tail = f"  ctxsw {switches} ({sw_rate})  events {nev} ({ev_rate})  window {fmt_time(span)}"
    stdscr.addstr(y, len(head) + gw, tail[: max(0, w - len(head) - gw - 1)])
    y += 1

    # Per-thread utilization: stacked state composition over the window.
    bw = max(8, w - 26)
    for tid in lanes:
        if y >= y1:
            break
        acc = per.get(tid)
        if not acc:
            continue
        util = acc.get(ST_RUN, 0) / span
        name = thread_label(tr, tid)[:11]
        stdscr.addstr(y, 0, f" {name:<11}{util * 100:3.0f}% ", curses.A_NORMAL)
        parts = [(st, acc.get(st, 0) / span) for st in (ST_RUN, ST_RDY, ST_BLK, ST_SLP, ST_SUS)]
        bar(y, 17, bw, parts, empty=" ")
        y += 1


def _draw_log(stdscr, tr, h, w, cur_i, log_scroll, use_color, curses):
    """Render the raw CTF event log as a scrollable table."""
    stdscr.addstr(
        1,
        0,
        (f"{'#':>7}  {'time':>12}  {'id':>5}  {'event':<28} fields")[: w - 1],
        curses.A_UNDERLINE,
    )
    rows = h - 3  # header(0), column titles(1), footer(h-1)
    # Clamp so a large scroll (e.g. "go to latest") lands on the last full page.
    top = max(0, min(log_scroll, max(0, len(tr.events) - rows)))
    for r in range(rows):
        i = top + r
        if i >= len(tr.events):
            break
        ev = tr.events[i]
        line = (
            f"{i:>7}  {fmt_time(ev.ts - tr.t0):>12}  "
            f"0x{ev.eid:03x}  {ev.name:<28} {_fmt_fields(ev)}"
        )
        attr = curses.A_NORMAL
        if i == cur_i:
            attr = curses.color_pair(21) if use_color else curses.A_REVERSE
        stdscr.addstr(2 + r, 0, line[: w - 1], attr)


def main():
    ap = argparse.ArgumentParser(description="Zephyr CTF trace viewer", allow_abbrev=False)
    ap.add_argument("binary", help="CTF trace binary (e.g. build/tracing.bin)")
    ap.add_argument("--metadata", help="path to TSDL metadata file")
    ap.add_argument(
        "--text", action="store_true", help="print a static ASCII timeline instead of the TUI"
    )
    ap.add_argument(
        "--width", type=int, default=0, help="output width for --text (default: terminal width)"
    )
    ap.add_argument(
        "--no-timestamp",
        action="store_true",
        help="trace was built without CONFIG_TRACING_CTF_TIMESTAMP",
    )
    ap.add_argument(
        "-f",
        "--follow",
        action="store_true",
        help="follow the file live as the application writes it "
        "(waits for the file/first events to appear)",
    )
    args = ap.parse_args()

    meta_path = find_metadata(args.binary, args.metadata)
    defs, used = build_event_defs(meta_path)
    if used is None:
        sys.stderr.write(
            "warning: TSDL metadata not found; using built-in scheduling-event "
            "table only (pass --metadata to decode all events)\n"
        )

    has_ts = not args.no_timestamp

    if args.follow:
        if args.text or not sys.stdout.isatty():
            sys.stderr.write("--follow requires an interactive terminal\n")
            return 1
        return _run_follow(args.binary, defs, has_ts)

    tr = parse_trace(args.binary, defs, has_ts=has_ts)
    if not tr.events:
        sys.stderr.write("no events decoded; is this a CTF trace? try --no-timestamp\n")
        return 1

    if args.text or not sys.stdout.isatty():
        width = args.width or (os.get_terminal_size().columns if sys.stdout.isatty() else 100)
        run_text(tr, width)
        return 0

    reader = TraceReader(defs, has_ts)
    with safe_open(args.binary, "rb") as fh:
        reader.feed(fh.read())

    import curses

    curses.wrapper(run_curses, reader)
    return 0


def _run_follow(path, defs, has_ts):
    """Open the trace, catch up on existing bytes, then follow it live."""
    import curses
    import time

    deadline = None
    while not os.path.exists(path):
        if deadline is None:
            sys.stderr.write(f"waiting for {path} to appear (start the application)...\n")
            deadline = True
        time.sleep(0.3)

    reader = TraceReader(defs, has_ts)
    fh = safe_open(path, "rb")
    try:
        reader.feed(fh.read())  # catch up on whatever already exists
        curses.wrapper(run_curses, reader, fh)
    finally:
        fh.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
