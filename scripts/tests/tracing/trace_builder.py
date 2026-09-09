#!/usr/bin/env python3
#
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0
"""Build a CTF stream the way the Zephyr CTF backend writes one.

Traces are encoded from the tracing subsystem's own TSDL metadata, so the event
ids and field layouts under test are the ones a Zephyr build emits. The tracing
scripts are standalone files rather than an installable package, so the
directory holding them is put on ``sys.path`` before they are imported.
"""

import os
import struct
import sys

ZEPHYR_BASE = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts", "tracing"))

import trace_viewer  # noqa: E402

TSDL_METADATA = os.path.join(ZEPHYR_BASE, "subsys", "tracing", "ctf", "tsdl", "metadata")


def parse_metadata(path):
    """Parse the TSDL metadata file into the {event id: EventDef} table."""
    return trace_viewer.parse_metadata(path)


class TraceBuilder:
    """Build a CTF stream the way the CTF backend writes one.

    ``has_ts`` mirrors ``CONFIG_TRACING_CTF_TIMESTAMP``: when it is off the
    backend writes no timestamp, so the record header is the event id alone.
    """

    def __init__(self, defs, has_ts=True):
        self._defs = {edef.name: edef for edef in defs.values()}
        self._hdr = trace_viewer.HDR if has_ts else trace_viewer.HDR_NO_TS
        self._has_ts = has_ts
        self.data = b""

    def event(self, ts, name, /, **fields):
        """Append the ``name`` event, timestamped ``ts`` ns, carrying ``fields``.

        ``ts`` and ``name`` are positional-only so that a CTF field of the same
        name, such as the thread name, can be passed as a keyword.
        """
        edef = self._defs[name]
        payload = b""
        for fname, kind in edef.fields:
            value = fields.pop(fname)
            if isinstance(kind, tuple):
                encoded = value.encode()
                assert len(encoded) < kind[1], f"{value!r} does not fit in {name}.{fname}"
                payload += encoded.ljust(kind[1], b"\x00")
            else:
                payload += struct.pack("<" + kind, value)
        assert not fields, f"{name} has no field {sorted(fields)}"
        hdr = self._hdr.pack(ts, edef.eid) if self._has_ts else self._hdr.pack(edef.eid)
        self.data += hdr + payload
        return self
