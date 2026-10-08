#!/usr/bin/env python3
# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Writes the input of the BabbleSim playback device of the anonymous advertising tests.

The tests need anonymous advertising with AD data, in which neither the ADV_EXT_IND nor the
AUX_ADV_IND has an AdvA. The Controller can not send that: it puts the AdvA in every
AUX_ADV_IND, so its anonymous advertising has no AD data. The playback device
(bs_device_2G4_playback) sends the PDUs written here instead, from a Tx dump of the 2G4 Phy.

The tests use anonymous.Tx.csv and anonymous_other.Tx.csv as they are in the tree. Run this
script to write them again, into the folder it is in.
"""

import struct
from pathlib import Path

# The TargetA of the directed advertising, both static random addresses: the scanner of the
# tests, C0:00:00:00:00:02 (scanner_addr in src/anonymous.c), and a device that is not in the
# simulation, C0:00:00:00:00:03 (other_addr)
SCANNER_ADDR = bytes.fromhex("02 00 00 00 00 C0")
OTHER_ADDR = bytes.fromhex("03 00 00 00 00 C0")

# The AD data of each kind of advertising, as in src/anonymous.c
AD_UNDIRECTED = 0x01
AD_DIRECTED = 0x02
AD_DIRECTED_PRI = 0x03

DURATION_US = 6_000_000
PERIOD_US = 500_000

ADV_ACCESS_ADDRESS = 0x8E89BED6
PDU_TYPE_ADV_EXT_IND = 0x07  # AUX_ADV_IND has the same type
PDU_HDR_RX_ADD = 0x80
EXT_HDR_TARGET_A = 0x02
EXT_HDR_ADI = 0x08
EXT_HDR_AUX_PTR = 0x10
AUX_PTR_CA = 0x40  # advertiser clock accuracy of 0 to 50 ppm

# Center frequencies in MHz above 2400 MHz: advertising channels 37, 38 and 39, and the
# channel index and frequency of the AUX_ADV_INDs
PRIMARY_FREQS = (2, 26, 80)
AUX_CHAN_IDX = 5
AUX_FREQ = 14

# Start of each ADV_EXT_IND after the one before it, and of the AUX_ADV_IND after the first
# ADV_EXT_IND
PRIMARY_SPACING_US = 300
AUX_OFFSET_US = 1500
AUX_OFFSET_UNIT_US = 30

MODULATION_BLE_1M = 16
NEVER = 2**64 - 1


def adv_crc(pdu):
    """The CRC of an advertising channel PDU, in the order its octets are sent."""
    # The CRC init of advertising channel PDUs, 0x555555, with its bits reversed for this
    # shift register, which takes the LSB of each octet first
    state = 0xAAAAAA

    for octet in pdu:
        for _ in range(8):
            feedback = (state ^ octet) & 1
            octet >>= 1
            state >>= 1
            if feedback:
                state = (state | 0x800000) ^ 0x5A6000

    return state.to_bytes(3, "little")


def ext_adv_pdu(sid, target_a=None, aux_offset_us=None, ad_value=None):
    """A non-connectable and non-scannable extended advertising PDU without AdvA."""
    flags = EXT_HDR_ADI
    fields = b""

    if target_a is not None:
        flags |= EXT_HDR_TARGET_A
        fields += target_a

    # Advertising Data ID 1 of the advertising set
    fields += struct.pack("<H", 1 | (sid << 12))

    if aux_offset_us is not None:
        flags |= EXT_HDR_AUX_PTR
        # The channel, the offset in units of 30 us and the LE 1M PHY
        fields += bytes([AUX_CHAN_IDX | AUX_PTR_CA])
        fields += struct.pack("<H", aux_offset_us // AUX_OFFSET_UNIT_US)

    # The extended header length shares its octet with the AdvMode, 0b00
    ext_hdr = bytes([flags]) + fields
    payload = bytes([len(ext_hdr)]) + ext_hdr

    if ad_value is not None:
        # Manufacturer Specific Data of one octet
        payload += bytes([2, 0xFF, ad_value])

    # The TargetA is a random address
    hdr = PDU_TYPE_ADV_EXT_IND | (PDU_HDR_RX_ADD if target_a is not None else 0)

    return bytes([hdr, len(payload)]) + payload


def tx(start_us, freq, pdu):
    """A line of the Tx dump: the PDU sent on the LE 1M PHY at the given time."""
    packet = pdu + adv_crc(pdu)
    # The preamble and the access address take 5 octets before the PDU
    end_us = start_us + (5 + len(packet)) * 8 - 1

    return (
        f"{start_us},{end_us},{freq}.000000,0x{ADV_ACCESS_ADDRESS:08X},{MODULATION_BLE_1M},"
        f"0.000000,{NEVER},{NEVER},{len(packet)},{packet.hex(' ').upper()}"
    )


def event(start_us, sid, ad_value, pri_target_a=None, aux_target_a=None):
    """An extended advertising event: three ADV_EXT_INDs that point to one AUX_ADV_IND."""
    lines = []

    for i, freq in enumerate(PRIMARY_FREQS):
        offset_us = i * PRIMARY_SPACING_US
        pdu = ext_adv_pdu(sid, pri_target_a, aux_offset_us=AUX_OFFSET_US - offset_us)
        lines.append(tx(start_us + offset_us, freq, pdu))

    pdu = ext_adv_pdu(sid, aux_target_a, ad_value=ad_value)
    lines.append(tx(start_us + AUX_OFFSET_US, AUX_FREQ, pdu))

    return lines


def tx_dump(target_a, period_us=PERIOD_US):
    """The Tx dump: undirected advertising, and directed to the given address with the
    TargetA in the ADV_EXT_IND and with the TargetA in the AUX_ADV_IND.
    """
    lines = [
        "start_time,end_time,center_freq,phy_address,modulation,power_level,"
        "abort_time,recheck_time,packet_size,packet"
    ]

    for start_us in range(50_000, DURATION_US, period_us):
        lines += event(start_us, 1, AD_UNDIRECTED)
        lines += event(start_us + 25_000, 3, AD_DIRECTED_PRI, pri_target_a=target_a)
        lines += event(start_us + 50_000, 2, AD_DIRECTED, aux_target_a=target_a)

    return "\n".join(lines) + "\n"


def main():
    folder = Path(__file__).parent

    (folder / "anonymous.Tx.csv").write_text(tx_dump(SCANNER_ADDR), encoding="ascii")
    (folder / "anonymous_other.Tx.csv").write_text(tx_dump(OTHER_ADDR), encoding="ascii")


if __name__ == "__main__":
    main()
