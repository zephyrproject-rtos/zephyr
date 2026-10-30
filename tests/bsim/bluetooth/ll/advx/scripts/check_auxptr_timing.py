#!/usr/bin/env python3
# Copyright (c) 2026 Kent Hauser
# SPDX-License-Identifier: Apache-2.0

"""Check auxiliary packet timing against the AuxPtr that announced it.

Reads an advertiser's 2G4 phy Tx dump (d_2G4_<n>.Tx.csv). For every
ADV_EXT_IND carrying an AuxPtr, finds the auxiliary packet on the announced
channel and checks its start time, as required by Core Vol 6, Part B,
Section 2.3.4.5: the auxiliary packet shall not start any earlier than the
Aux Offset and shall start no later than the Aux Offset plus one Offset Unit.
The offset is measured from the start of the packet holding the AuxPtr.

The auxiliary packet is matched far outside the window under test: it is the
packet of the same advertising set (ADI SID) on the announced channel nearest
the Aux Offset, within 10 ms of it. Two advertising events may announce the
same auxiliary packet. An event with no such packet had its auxiliary packet
cancelled; that is counted, not checked.

Exits non-zero on any violation, or if no auxiliary packet was checked.
Run with --self-test to check the checker.
"""

import csv
import sys

ADV_ACCESS_ADDRESS = 0x8E89BED6
PRIMARY_FREQS = (2.0, 26.0, 80.0)  # MHz above 2400: channels 37, 38, 39
PDU_TYPE_ADV_EXT_IND = 0x07
TOLERANCE_US = 1  # bsim time resolution
# Primary channel packets of one event are at most 10 ms apart; events are at
# least 20 ms apart (advertising interval minimum).
EVENT_GAP_US = 10000
# How far from its Aux Offset an auxiliary packet is looked for: far wider than
# the window under test (one Offset Unit, 30 or 300 us), so early and late
# packets are found and reported, not taken for cancelled ones.
MATCH_RADIUS_US = 10000

# Extended header fields in order: (flag bit, size in octets, name)
EXT_HDR_FIELDS = (
    (0, 6, 'adva'),
    (1, 6, 'targeta'),
    (2, 1, 'cteinfo'),
    (3, 2, 'adi'),
    (4, 3, 'auxptr'),
    (5, 18, 'syncinfo'),
    (6, 1, 'txpower'),
)


def channel_freq(chan_idx):
    """Data channel index to MHz above 2400."""
    return 4 + 2 * chan_idx if chan_idx <= 10 else 28 + 2 * (chan_idx - 11)


def ext_hdr_fields(pdu):
    """Return the extended header fields of a common extended advertising PDU."""
    if len(pdu) < 3 or (pdu[0] & 0x0F) != PDU_TYPE_ADV_EXT_IND:
        return None
    if (pdu[2] & 0x3F) == 0:
        return {}
    flags = pdu[3]
    pos = 4
    fields = {}
    for bit, size, name in EXT_HDR_FIELDS:
        if flags & (1 << bit):
            fields[name] = pdu[pos : pos + size]
            pos += size
    return fields


def load_tx(path):
    packets = []
    with open(path, newline='') as fh:
        for row in csv.DictReader(fh):
            if int(row['phy_address'], 16) != ADV_ACCESS_ADDRESS:
                continue
            pdu = bytes(int(octet, 16) for octet in row['packet'].split())
            packets.append((int(row['start_time']), float(row['center_freq']), pdu))
    return packets


def sid(fields):
    adi = fields.get('adi')
    return None if adi is None else adi[1] >> 4


def primary_events(packets):
    """Group the primary channel ADV_EXT_INDs carrying an AuxPtr into events.

    Returns a list of events, each a list of (start, fields) in time order.
    """
    events = []
    last = None
    for start, freq, pdu in packets:
        if freq not in PRIMARY_FREQS:
            continue
        fields = ext_hdr_fields(pdu)
        if not fields or 'auxptr' not in fields:
            continue
        if last is None or start - last > EVENT_GAP_US:
            events.append([])
        events[-1].append((start, fields))
        last = start
    return events


def aux_ptr_target(start, aux_ptr):
    """Return (aux channel MHz, Aux Offset time in us, Offset Unit in us)."""
    unit_us = 300 if aux_ptr[0] & 0x80 else 30
    offset = (aux_ptr[1] | (aux_ptr[2] << 8)) & 0x1FFF
    return channel_freq(aux_ptr[0] & 0x3F), start + offset * unit_us, unit_us


def find_aux(packets, freq, set_id, expected):
    """Start of the packet of this advertising set nearest the Aux Offset on its
    announced channel, if within MATCH_RADIUS_US of it; None if cancelled.
    """
    starts = (
        t
        for t, f, pdu in packets
        if f == freq
        and abs(t - expected) <= MATCH_RADIUS_US
        and (fields := ext_hdr_fields(pdu)) is not None
        and sid(fields) == set_id
    )
    return min(starts, key=lambda t: abs(t - expected), default=None)


def check(packets, out=print):
    """Return (checked, violations, cancelled)."""
    events = primary_events(packets)
    checked = violations = cancelled = 0

    for event in events:
        first_start, first_fields = event[0]
        freq, first_expected, _ = aux_ptr_target(first_start, first_fields['auxptr'])
        aux_start = find_aux(packets, freq, sid(first_fields), first_expected)

        for start, fields in event:
            if aux_start is None:
                cancelled += 1
                continue

            _, expected, unit_us = aux_ptr_target(start, fields['auxptr'])
            checked += 1
            late = aux_start - expected
            if late < -TOLERANCE_US or late > unit_us + TOLERANCE_US:
                violations += 1
                out(
                    f"ADV_EXT_IND at {start} us: aux at {aux_start} us, "
                    f"{late} us from the Aux Offset (allowed 0..{unit_us} us)"
                )

    return checked, violations, cancelled


def main(path):
    checked, violations, cancelled = check(load_tx(path))
    print(
        f"Checked {checked} auxiliary packets against their AuxPtr, "
        f"{violations} outside the allowed window, {cancelled} cancelled"
    )
    return 1 if (violations or not checked) else 0


def self_test():
    """Synthetic dumps: a valid event next to early, late and cancelled ones."""

    def pdu(hdr):
        return bytes([PDU_TYPE_ADV_EXT_IND, len(hdr) + 1, len(hdr)]) + bytes(hdr)

    def event(t0, aux_late_us, chan_idx=5, offset=100, aux_did=1):
        # primary packets 13 Offset Units apart, each AuxPtr to the same aux;
        # ADI: DID 1, SID 0xA
        pkts = []
        for i, freq in enumerate(PRIMARY_FREQS):
            off = offset - i * 13
            pkts.append((t0 + i * 390, freq, pdu([0x18, 1, 0xA0, chan_idx, off & 0xFF, off >> 8])))
        if aux_late_us is not None:
            aux_start = t0 + offset * 30 + aux_late_us
            aux_pdu = pdu([0x08, aux_did, 0xA0])
            pkts.append((aux_start, float(channel_freq(chan_idx)), aux_pdu))
        return pkts

    def run(*events):
        return check(sorted(p for e in events for p in e), out=lambda _: None)

    cases = (
        ('valid', run(event(0, 0)), (3, 0, 0)),
        ('late beyond the old search band', run(event(0, 0), event(100000, 61)), (6, 3, 0)),
        ('early beyond the old search band', run(event(0, 0), event(100000, -31)), (6, 3, 0)),
        ('cancelled', run(event(0, 0), event(100000, None), event(200000, 0)), (6, 0, 3)),
        # Aux Offset beyond the next event's primary packets (offset 1000 = 30 ms)
        ('long offset', run(event(0, 0, offset=1000), event(20000, 0, chan_idx=7)), (6, 0, 0)),
        ('DID changed in the aux', run(event(0, 0, aux_did=2)), (3, 0, 0)),
        # the next event announces the same auxiliary packet, 10 us late for it
        (
            'announced twice',
            run(event(0, 0, offset=1000), event(20000, None, offset=333)),
            (6, 0, 0),
        ),
    )
    failed = [(name, got, want) for name, got, want in cases if got != want]
    for name, got, want in failed:
        print(f"self-test {name}: got {got}, want {want}")
    print('self-test', 'FAILED' if failed else 'passed')
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(self_test() if sys.argv[1] == '--self-test' else main(sys.argv[1]))
