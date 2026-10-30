#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0

"""Writes the input of the BabbleSim playback device of the anonymous advertising tests.

The tests need anonymous advertising with AD data, in which neither the ADV_EXT_IND nor the
AUX_ADV_IND has an AdvA. The Controller can not send that: it puts the AdvA in every
AUX_ADV_IND, so its anonymous advertising has no AD data. The playback device
(bs_device_2G4_playback) sends the PDUs written here instead, from a Tx dump of the 2G4 Phy.

The input for the scanner of anonymous.sh also has scannable advertising, with the AdvA in its
AUX_ADV_IND and without, which is not valid: the scanner may only send AUX_SCAN_REQs to the
first. The input for both tests has an AUX_ADV_IND whose flags announce a TxPower that its
extended header has no room for, which the scanner may not report.

The input for the scanner of anonymous.sh also has directed advertising whose TargetA is a
resolvable private address in an ADV_EXT_IND without AdvA, with the AdvA only in the
AUX_ADV_IND: the scanner has to report the advertising whose TargetA resolves with its local
IRK, and not the advertising whose TargetA does not.

The test scripts run this script with the folder to write anonymous.Tx.csv and
anonymous_other.Tx.csv into.
"""

import struct
import sys
from pathlib import Path

# The TargetA of the directed advertising, both static random addresses: the scanner of the
# tests, C0:00:00:00:00:02 (scanner_addr in src/anonymous.c), and a device that is not in the
# simulation, C0:00:00:00:00:03 (other_addr)
SCANNER_ADDR = bytes.fromhex("02 00 00 00 00 C0")
OTHER_ADDR = bytes.fromhex("03 00 00 00 00 C0")

# The AdvA of the scannable advertising, the static random address C0:00:00:00:00:04, which
# tests_scripts/anonymous.sh looks for in the AUX_SCAN_REQs of the scanner
SCANNABLE_ADDR = bytes.fromhex("04 00 00 00 00 C0")

# The TargetA of the directed advertising to a resolvable private address: the address from
# the prand 0x708194 and the IRK of the sample data of the random address hash function ah
# (Core Spec v6.3, Vol 3, Part H, Appendix D.7), which is the local IRK of the resolving list
# entry of the scanner (local_irk in src/anonymous.c), and an address with another hash
RESOLVABLE_RPA = bytes.fromhex("AA FB 0D 94 81 70")
UNRESOLVABLE_RPA = bytes.fromhex("AB FB 0D 94 81 70")

# The AdvA of the directed advertising to a resolvable private address, the static random
# address C0:00:00:00:00:05 (rpa_directed_addr in src/anonymous.c)
RPA_DIRECTED_ADDR = bytes.fromhex("05 00 00 00 00 C0")

# The AD data of each kind of advertising, as in src/anonymous.c
AD_UNDIRECTED = 0x01
AD_DIRECTED = 0x02
AD_DIRECTED_PRI = 0x03
AD_DIRECTED_RPA = 0x04
AD_DIRECTED_RPA_OTHER = 0x05

# The advertising set of the AUX_ADV_IND with the truncated extended header, as in
# src/anonymous.c
SID_TRUNCATED = 6

DURATION_US = 6_000_000
PERIOD_US = 500_000

ADV_ACCESS_ADDRESS = 0x8E89BED6
PDU_TYPE_ADV_EXT_IND = 0x07  # AUX_ADV_IND has the same type
PDU_HDR_TX_ADD = 0x40
PDU_HDR_RX_ADD = 0x80
EXT_HDR_ADV_A = 0x01
EXT_HDR_TARGET_A = 0x02
EXT_HDR_ADI = 0x08
EXT_HDR_AUX_PTR = 0x10
EXT_HDR_TX_PWR = 0x40
AUX_PTR_CA = 0x40  # advertiser clock accuracy of 0 to 50 ppm
ADV_MODE_SCANNABLE = 0b10

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


def ext_adv_pdu(sid=None, adv_mode=0, adv_a=None, target_a=None, aux_offset_us=None, ad_value=None):
    """An extended advertising PDU with the given fields, and without an extended header when
    it has none of them.
    """
    flags = 0
    fields = b""

    if adv_a is not None:
        flags |= EXT_HDR_ADV_A
        fields += adv_a

    if target_a is not None:
        flags |= EXT_HDR_TARGET_A
        fields += target_a

    if sid is not None:
        flags |= EXT_HDR_ADI
        # Advertising Data ID 1 of the advertising set
        fields += struct.pack("<H", 1 | (sid << 12))

    if aux_offset_us is not None:
        flags |= EXT_HDR_AUX_PTR
        # The channel, the offset in units of 30 us and the LE 1M PHY
        fields += bytes([AUX_CHAN_IDX | AUX_PTR_CA])
        fields += struct.pack("<H", aux_offset_us // AUX_OFFSET_UNIT_US)

    # The extended header length shares its octet with the AdvMode
    ext_hdr = bytes([flags]) + fields if flags != 0 else b""
    payload = bytes([len(ext_hdr) | (adv_mode << 6)]) + ext_hdr

    if ad_value is not None:
        # Manufacturer Specific Data of one octet
        payload += bytes([2, 0xFF, ad_value])

    # The AdvA and the TargetA are random addresses
    hdr = PDU_TYPE_ADV_EXT_IND
    if adv_a is not None:
        hdr |= PDU_HDR_TX_ADD
    if target_a is not None:
        hdr |= PDU_HDR_RX_ADD

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


def event(start_us, sid, aux_pdu, adv_mode=0, pri_target_a=None):
    """An extended advertising event: three ADV_EXT_INDs that point to the AUX_ADV_IND."""
    lines = []

    for i, freq in enumerate(PRIMARY_FREQS):
        offset_us = i * PRIMARY_SPACING_US
        pdu = ext_adv_pdu(
            sid, adv_mode, target_a=pri_target_a, aux_offset_us=AUX_OFFSET_US - offset_us
        )
        lines.append(tx(start_us + offset_us, freq, pdu))

    lines.append(tx(start_us + AUX_OFFSET_US, AUX_FREQ, aux_pdu))

    return lines


def tx_dump(target_a, scannable=False, rpa=False):
    """The Tx dump: undirected advertising, and directed to the given address with the
    TargetA in the ADV_EXT_IND and with the TargetA in the AUX_ADV_IND, all of it anonymous,
    the one with the truncated extended header, and the scannable advertising and the
    directed advertising to resolvable private addresses if asked for.
    """
    lines = [
        "start_time,end_time,center_freq,phy_address,modulation,power_level,"
        "abort_time,recheck_time,packet_size,packet"
    ]

    for start_us in range(50_000, DURATION_US, PERIOD_US):
        lines += event(start_us, 1, ext_adv_pdu(1, ad_value=AD_UNDIRECTED))
        lines += event(
            start_us + 25_000, 3, ext_adv_pdu(3, ad_value=AD_DIRECTED_PRI), pri_target_a=target_a
        )
        lines += event(
            start_us + 50_000, 2, ext_adv_pdu(2, target_a=target_a, ad_value=AD_DIRECTED)
        )

        if scannable:
            aux_pdu = ext_adv_pdu(4, ADV_MODE_SCANNABLE, adv_a=SCANNABLE_ADDR)
            lines += event(start_us + 75_000, 4, aux_pdu, adv_mode=ADV_MODE_SCANNABLE)
            # The AUX_ADV_IND of scannable advertising has to have the AdvA
            aux_pdu = ext_adv_pdu(adv_mode=ADV_MODE_SCANNABLE)
            lines += event(start_us + 100_000, 5, aux_pdu, adv_mode=ADV_MODE_SCANNABLE)

        # An extended header of one octet, the flags, which announce a TxPower
        aux_pdu = bytes([PDU_TYPE_ADV_EXT_IND, 2, 1, EXT_HDR_TX_PWR])
        lines += event(start_us + 125_000, SID_TRUNCATED, aux_pdu)

        if rpa:
            # Non-connectable and non-scannable directed advertising, whose AUX_ADV_IND has
            # the AdvA and no TargetA (Core Spec v6.3, Vol 6, Part B, Tables 2.4 and 2.5)
            aux_pdu = ext_adv_pdu(7, adv_a=RPA_DIRECTED_ADDR, ad_value=AD_DIRECTED_RPA)
            lines += event(start_us + 150_000, 7, aux_pdu, pri_target_a=RESOLVABLE_RPA)
            aux_pdu = ext_adv_pdu(8, adv_a=RPA_DIRECTED_ADDR, ad_value=AD_DIRECTED_RPA_OTHER)
            lines += event(start_us + 175_000, 8, aux_pdu, pri_target_a=UNRESOLVABLE_RPA)

    return "\n".join(lines) + "\n"


def main():
    if len(sys.argv) != 2:
        sys.exit(f"usage: {sys.argv[0]} <output folder>")

    folder = Path(sys.argv[1])

    (folder / "anonymous.Tx.csv").write_text(
        tx_dump(SCANNER_ADDR, scannable=True, rpa=True), encoding="ascii"
    )
    (folder / "anonymous_other.Tx.csv").write_text(tx_dump(OTHER_ADDR), encoding="ascii")


if __name__ == "__main__":
    main()
