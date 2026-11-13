#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright (c) 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

"""Writes the input of the BabbleSim playback device of the periodic sync PDU type test.

Only AUX_SYNC_IND and AUX_CHAIN_IND PDUs, of PDU Type 0b0111, form a periodic advertising
train. The test needs PDUs of other types that a synchronized receiver gets with the Access
Address and the CRCInit of the train, which the Controller does not send. The playback device
(bs_device_2G4_playback) sends the periodic advertising written here instead, from a Tx dump
of the 2G4 Phy.

The periodic advertising events, numbered by their paEventCounter, go through these phases:

- ESTAB: a PDU of another type in place of every AUX_SYNC_IND, while the scanner tries to
  synchronize
- VALID: an AUX_SYNC_IND
- CHAIN: an AUX_SYNC_IND with an AuxPtr to a PDU of another type in place of the AUX_CHAIN_IND
- CHAIN_ALT: as CHAIN in the even events, as VALID in the odd ones
- LOST: as ESTAB, after the scanner has synchronized

From the first event of the CHAIN phase on, each periodic advertising event starts DRIFT_US
later than the interval after the one before it, faster than the SCA in the SyncInfo allows. The
receiver keeps the train in its receive window only with the drift compensation of every event,
based on the AUX_SYNC_IND when the PDU its AuxPtr points to is ignored. The drift stops at the
LOST phase, so that its PDUs stay in the receive window and are received until the sync timeout.

The AD data of each PDU is Manufacturer Specific Data with the kind of the PDU and its
paEventCounter, as in src/sync_pdu_type.c.

Extended advertising events with an AUX_ADV_IND that has the SyncInfo of the train run in
between the periodic advertising events throughout.

The test script runs this script with the folder to write sync_pdu_type.Tx.csv into.
"""

import struct
import sys
from pathlib import Path

# The AdvA of the advertising, the static random address C0:00:00:00:00:05, and its SID, as in
# src/sync_pdu_type.c
ADV_ADDR = bytes.fromhex("05 00 00 00 00 C0")
SID = 1

# The kinds of AD data, as in src/sync_pdu_type.c
AD_KIND_VALID = 0x01
AD_KIND_PARENT = 0x02
AD_KIND_INVALID = 0x03

# The paEventCounter of the first event of each phase, as in src/sync_pdu_type.c
EVT_VALID = 30
EVT_CHAIN = 60
EVT_CHAIN_ALT = 80
EVT_LOST = 100
EVT_END = 130

# The periodic advertising interval of 100 ms, in units of 1.25 ms in the SyncInfo, and the
# start of the periodic advertising event with paEventCounter 0
PER_INTERVAL_US = 100_000
PER_INTERVAL_UNITS = PER_INTERVAL_US * 4 // 5000
PER_START_US = 100_000

# The drift of each periodic advertising event from the CHAIN phase on, 300 ppm of the interval
DRIFT_US = 30

# The SyncInfo offset from the AUX_ADV_IND to the next periodic advertising event, about half
# the interval and a multiple of the 30 us offset unit
SYNC_OFFSET_US = 49_980

ADV_ACCESS_ADDRESS = 0x8E89BED6
ADV_CRC_INIT = 0x555555
SYNC_ACCESS_ADDRESS = 0x71764129
SYNC_CRC_INIT = 0x3B6E0C
# All 37 data channels, and the SCA of 0 to 20 ppm
SYNC_CHM = (1 << 37) - 1
SYNC_SCA = 7

PDU_TYPE_ADV_IND = 0x00
PDU_TYPE_AUX_SCAN_REQ = 0x03
PDU_TYPE_AUX_CONNECT_REQ = 0x05
PDU_TYPE_ADV_EXT_IND = 0x07  # AUX_ADV_IND, AUX_SYNC_IND and AUX_CHAIN_IND have the same type
PDU_TYPE_AUX_CONNECT_RSP = 0x08
PDU_HDR_TX_ADD = 0x40

# The types of the PDUs sent in place of AUX_SYNC_INDs and AUX_CHAIN_INDs, in turn
INVALID_TYPES = (
    PDU_TYPE_AUX_CONNECT_REQ,
    PDU_TYPE_AUX_CONNECT_RSP,
    PDU_TYPE_ADV_IND,
    PDU_TYPE_AUX_SCAN_REQ,
)

EXT_HDR_ADV_A = 0x01
EXT_HDR_ADI = 0x08
EXT_HDR_AUX_PTR = 0x10
EXT_HDR_SYNC_INFO = 0x20
AUX_PTR_CA = 0x40  # advertiser clock accuracy of 0 to 50 ppm

# Center frequencies in MHz above 2400 MHz of advertising channels 37, 38 and 39, the channel
# index of the AUX_ADV_INDs, and the channel index of the PDUs in place of AUX_CHAIN_INDs
PRIMARY_FREQS = (2, 26, 80)
AUX_ADV_CHAN_IDX = 5
AUX_CHAIN_CHAN_IDX = 20

# Start of each ADV_EXT_IND after the one before it, and of the AUX_ADV_IND after the first
# ADV_EXT_IND
PRIMARY_SPACING_US = 300
AUX_OFFSET_US = 1500
OFFSET_UNIT_US = 30

# The Inter Frame Space between the AUX_SYNC_IND and the PDU its AuxPtr points to, at least
# T_MAFS
T_MAFS_US = 300

MODULATION_BLE_1M = 16
NEVER = 2**64 - 1


def chan_freq(chan_idx):
    """The center frequency in MHz above 2400 MHz of a data channel index."""
    if chan_idx <= 10:
        return 4 + 2 * chan_idx

    return 28 + 2 * (chan_idx - 11)


def reverse_bits(value, bits):
    """The value with the order of its given number of low bits reversed."""
    return int(f"{value:0{bits}b}"[::-1], 2)


def csa2_chan_idx(access_address, counter):
    """The channel index of Channel Selection Algorithm #2, with all data channels used."""
    chan_id = ((access_address >> 16) ^ access_address) & 0xFFFF
    prn = counter ^ chan_id

    for _ in range(3):
        # Bit order reversal of each octet, then the multiply, add and modulo operation
        prn = (reverse_bits(prn >> 8, 8) << 8) | reverse_bits(prn & 0xFF, 8)
        prn = (17 * prn + chan_id) & 0xFFFF

    return (prn ^ chan_id) % 37


def crc(pdu, crc_init):
    """The CRC of a PDU, in the order its octets are sent."""
    # The CRC init with its bits reversed for this shift register, which takes the LSB of each
    # octet first
    state = reverse_bits(crc_init, 24)

    for octet in pdu:
        for _ in range(8):
            feedback = (state ^ octet) & 1
            octet >>= 1
            state >>= 1
            if feedback:
                state = (state | 0x800000) ^ 0x5A6000

    return state.to_bytes(3, "little")


def aux_ptr(chan_idx, offset_us):
    """An AuxPtr to a PDU on the LE 1M PHY at the given offset in units of 30 us."""
    return bytes([chan_idx | AUX_PTR_CA]) + struct.pack("<H", offset_us // OFFSET_UNIT_US)


def sync_info(offset_us, counter):
    """The SyncInfo of the train, with the offset in units of 30 us."""
    sca_chm = SYNC_CHM | (SYNC_SCA << 37)

    return (
        struct.pack("<HH", offset_us // OFFSET_UNIT_US, PER_INTERVAL_UNITS)
        + sca_chm.to_bytes(5, "little")
        + struct.pack("<I", SYNC_ACCESS_ADDRESS)
        + SYNC_CRC_INIT.to_bytes(3, "little")
        + struct.pack("<H", counter)
    )


def ext_adv_pdu(pdu_type=PDU_TYPE_ADV_EXT_IND, adv_a=None, adi=False, aux=None, sync=None, ad=None):
    """A PDU of the given type in the extended advertising format with the given fields."""
    flags = 0
    fields = b""

    if adv_a is not None:
        flags |= EXT_HDR_ADV_A
        fields += adv_a

    if adi:
        flags |= EXT_HDR_ADI
        # Advertising Data ID 1 of the advertising set
        fields += struct.pack("<H", 1 | (SID << 12))

    if aux is not None:
        flags |= EXT_HDR_AUX_PTR
        fields += aux

    if sync is not None:
        flags |= EXT_HDR_SYNC_INFO
        fields += sync

    # The extended header length shares its octet with the AdvMode, non-connectable and
    # non-scannable
    ext_hdr = bytes([flags]) + fields if flags != 0 else b""
    payload = bytes([len(ext_hdr)]) + ext_hdr

    if ad is not None:
        payload += ad

    hdr = pdu_type
    if adv_a is not None:
        hdr |= PDU_HDR_TX_ADD

    return bytes([hdr, len(payload)]) + payload


def ad_data(kind, counter):
    """Manufacturer Specific Data of the kind of the PDU and its paEventCounter."""
    return bytes([4, 0xFF, kind]) + struct.pack("<H", counter)


def pdu_us(pdu):
    """The air time of a PDU on the LE 1M PHY with the preamble, access address and CRC."""
    return (1 + 4 + len(pdu) + 3) * 8


def tx(start_us, freq, access_address, crc_init, pdu):
    """A line of the Tx dump: the PDU sent on the LE 1M PHY at the given time."""
    packet = pdu + crc(pdu, crc_init)
    end_us = start_us + pdu_us(pdu) - 1

    return (
        f"{start_us},{end_us},{freq}.000000,0x{access_address:08X},{MODULATION_BLE_1M},"
        f"0.000000,{NEVER},{NEVER},{len(packet)},{packet.hex(' ').upper()}"
    )


def tx_sync(start_us, chan_idx, pdu):
    """A line of the Tx dump: the PDU sent with the Access Address and CRCInit of the train."""
    return tx(start_us, chan_freq(chan_idx), SYNC_ACCESS_ADDRESS, SYNC_CRC_INIT, pdu)


def invalid_pdu(counter):
    """A PDU of a type other than AUX_SYNC_IND and AUX_CHAIN_IND."""
    pdu_type = INVALID_TYPES[counter % len(INVALID_TYPES)]

    return ext_adv_pdu(pdu_type, ad=ad_data(AD_KIND_INVALID, counter))


def per_start_us(counter):
    """The start of the periodic advertising event with the given paEventCounter."""
    drift_us = max(0, min(counter, EVT_LOST - 1) - (EVT_CHAIN - 1)) * DRIFT_US

    return PER_START_US + counter * PER_INTERVAL_US + drift_us


def per_event(counter):
    """The periodic advertising event with the given paEventCounter."""
    start_us = per_start_us(counter)
    chan_idx = csa2_chan_idx(SYNC_ACCESS_ADDRESS, counter)

    if (counter < EVT_VALID) or (counter >= EVT_LOST):
        return [tx_sync(start_us, chan_idx, invalid_pdu(counter))]

    if (counter < EVT_CHAIN) or ((counter >= EVT_CHAIN_ALT) and (counter % 2 != 0)):
        return [tx_sync(start_us, chan_idx, ext_adv_pdu(ad=ad_data(AD_KIND_VALID, counter)))]

    # The AuxPtr has the same size whatever its offset, the offset is the air time of the
    # AUX_SYNC_IND and T_MAFS rounded up to the 30 us offset unit
    ad = ad_data(AD_KIND_PARENT, counter)
    offset_us = pdu_us(ext_adv_pdu(aux=aux_ptr(0, 0), ad=ad)) + T_MAFS_US
    offset_us = -(-offset_us // OFFSET_UNIT_US) * OFFSET_UNIT_US
    pdu = ext_adv_pdu(aux=aux_ptr(AUX_CHAIN_CHAN_IDX, offset_us), ad=ad)

    return [
        tx_sync(start_us, chan_idx, pdu),
        tx_sync(start_us + offset_us, AUX_CHAIN_CHAN_IDX, invalid_pdu(counter)),
    ]


def adv_event(counter):
    """The extended advertising event before the periodic advertising event with the given
    paEventCounter: three ADV_EXT_INDs that point to the AUX_ADV_IND, which has the SyncInfo.
    """
    aux_start_us = per_start_us(counter) - SYNC_OFFSET_US
    start_us = aux_start_us - AUX_OFFSET_US
    lines = []

    for i, freq in enumerate(PRIMARY_FREQS):
        offset_us = i * PRIMARY_SPACING_US
        pdu = ext_adv_pdu(adi=True, aux=aux_ptr(AUX_ADV_CHAN_IDX, AUX_OFFSET_US - offset_us))
        lines.append(tx(start_us + offset_us, freq, ADV_ACCESS_ADDRESS, ADV_CRC_INIT, pdu))

    pdu = ext_adv_pdu(adv_a=ADV_ADDR, adi=True, sync=sync_info(SYNC_OFFSET_US, counter))
    lines.append(
        tx(aux_start_us, chan_freq(AUX_ADV_CHAN_IDX), ADV_ACCESS_ADDRESS, ADV_CRC_INIT, pdu)
    )

    return lines


def tx_dump():
    """The Tx dump of the playback device, in the order of the start of the PDUs."""
    lines = [
        "start_time,end_time,center_freq,phy_address,modulation,power_level,"
        "abort_time,recheck_time,packet_size,packet"
    ]

    for counter in range(EVT_END):
        if counter > 0:
            lines += adv_event(counter)

        lines += per_event(counter)

    return "\n".join(lines) + "\n"


def main():
    if len(sys.argv) != 2:
        sys.exit(f"usage: {sys.argv[0]} <output folder>")

    folder = Path(sys.argv[1])

    (folder / "sync_pdu_type.Tx.csv").write_text(tx_dump(), encoding="ascii")


if __name__ == "__main__":
    main()
