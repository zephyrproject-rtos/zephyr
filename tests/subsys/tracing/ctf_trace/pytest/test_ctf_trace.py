# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""
Decode the CTF stream emitted by the ctf_trace application and check it against
the expectations the application prints for every traced call.

    EXPECT <event> <field>=<value>...   the next such event must exist, in order
    COUNT <n> <event> <field>=<value>... exactly n such events exist in the stream
"""

import glob
import logging
import os
import sys
import time

logger = logging.getLogger(__name__)

ZEPHYR_BASE = os.getenv("ZEPHYR_BASE")
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts", "tracing"))
import trace_viewer as tv  # noqa: E402

METADATA = os.path.join(ZEPHYR_BASE, "subsys", "tracing", "ctf", "tsdl", "metadata")


def _find_trace(build_dir):
    candidates = [os.path.join(str(build_dir), "channel0_0")]
    candidates += glob.glob(os.path.join(str(build_dir), "**", "channel0_0"), recursive=True)
    for c in candidates:
        if os.path.isfile(c) and os.path.getsize(c) > 0:
            return c
    return None


def _parse_fields(tokens):
    fields = {}
    for tok in tokens:
        key, value = tok.split("=", 1)
        try:
            fields[key] = int(value, 0)
        except ValueError:
            fields[key] = value
    return fields


def _parse_expectations(lines):
    expects, counts = [], []
    for line in lines:
        tokens = line.split()
        if tokens[:1] == ["EXPECT"]:
            expects.append((tokens[1], _parse_fields(tokens[2:])))
        elif tokens[:1] == ["COUNT"]:
            counts.append((int(tokens[1]), tokens[2], _parse_fields(tokens[3:])))
    return expects, counts


def _matches(event, name, fields):
    if event.name != name:
        return False
    for key, want in fields.items():
        got = event.fields.get(key)
        if isinstance(want, int):
            # Ids and return values are emitted as 32-bit fields; negative
            # errno values and 64-bit pointers compare through the same mask.
            if got is None or (got & 0xFFFFFFFF) != (want & 0xFFFFFFFF):
                return False
        elif got != want:
            return False
    return True


def _fmt(event):
    return f"{event.name} {event.fields}"


def test_ctf_trace(dut):
    lines = dut.readlines_until(regex=".*CTF TRACE DONE", timeout=30)
    expects, counts = _parse_expectations(lines)
    assert expects, "the application printed no expectations"

    build_dir = dut.device_config.app_build_dir or dut.device_config.build_dir

    trace = None
    for _ in range(25):
        trace = _find_trace(build_dir)
        if trace:
            break
        time.sleep(0.2)
    assert trace, f"CTF trace file (channel0_0) not found under {build_dir}"
    logger.info("decoding CTF trace %s (%d bytes)", trace, os.path.getsize(trace))

    assert os.path.isfile(METADATA), f"CTF metadata not found at {METADATA}"
    defs = tv.parse_metadata(METADATA)
    events = tv.parse_trace(trace, defs, has_ts=True).events
    logger.info("decoded %d CTF events, checking %d expectations", len(events), len(expects))
    assert events, "no CTF events decoded"

    # Every expected event must appear, in the order the calls were made.
    pos = 0
    for name, fields in expects:
        idx = next((i for i in range(pos, len(events)) if _matches(events[i], name, fields)), None)
        if idx is None:
            same_name = [_fmt(e) for e in events[pos:] if e.name == name]
            raise AssertionError(
                f"no {name} {fields} after event #{pos}; "
                f"later {name} events: {same_name[:5] or 'none'}"
            )
        pos = idx + 1

    # Blocking hooks fire only when the call actually pended.
    for n, name, fields in counts:
        got = sum(1 for e in events if _matches(e, name, fields))
        assert got == n, f"expected {n} {name} {fields} events, decoded {got}"

    logger.info("CTF trace validated: %d events in order, %d counts", len(expects), len(counts))
