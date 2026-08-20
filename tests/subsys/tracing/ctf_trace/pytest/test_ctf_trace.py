# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""
Decode the CTF stream emitted by the ctf_trace application and assert that the
expected events are present with sane fields.

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

# One representative event per object type the app exercises.
EXPECTED_EVENTS = [
    "thread_create",
    "semaphore_give_enter",
    "semaphore_take_exit",
    "mutex_lock_enter",
    "queue_init",
    "queue_append_enter",
    "queue_get_exit",
    "fifo_init_enter",
    "fifo_put_enter",
    "lifo_put_enter",
    "stack_push_enter",
    "heap_alloc_enter",
    "heap_alloc_exit",
]


def _find_trace(build_dir):
    candidates = [os.path.join(str(build_dir), "channel0_0")]
    candidates += glob.glob(os.path.join(str(build_dir), "**", "channel0_0"), recursive=True)
    for c in candidates:
        if os.path.isfile(c) and os.path.getsize(c) > 0:
            return c
    return None


def test_ctf_trace(dut):
    # Wait for the app to finish emitting its tracepoints.
    dut.readlines_until(regex=".*CTF TRACE DONE", timeout=30)

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
    tr = tv.parse_trace(trace, defs, has_ts=True)

    names = [e.name for e in tr.events]
    seen = set(names)
    logger.info("decoded %d CTF events, %d distinct types", len(tr.events), len(seen))
    assert len(tr.events) > 0, "no CTF events decoded"

    missing = [e for e in EXPECTED_EVENTS if e not in seen]
    assert not missing, f"missing expected CTF events {missing}; decoded types: {sorted(seen)}"

    assert "queue_get_blocking" not in names, "K_NO_WAIT queue get emitted a blocking event"

    # Field sanity: queue_get_exit must carry the object id, timeout and return value.
    get_exit = next(e for e in tr.events if e.name == "queue_get_exit")
    for field in ("id", "timeout", "ret"):
        assert field in get_exit.fields, f"queue_get_exit missing field {field}: {get_exit.fields}"

    # Ordering sanity: the queue is initialised before it is read from.
    idx = {n: names.index(n) for n in ("queue_init", "queue_get_exit")}
    assert idx["queue_init"] < idx["queue_get_exit"], "queue_init must precede queue_get_exit"

    logger.info("CTF trace validated: all %d expected event types present", len(EXPECTED_EVENTS))
