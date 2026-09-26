#!/usr/bin/env python3
#
# Copyright (c) 2026 Zephyr Contributors
#
# SPDX-License-Identifier: Apache-2.0

from gdbstubs.arch.arm_aarch32 import GdbStub_ARM_AArch32

# Without a target description, GDB infers the register layout from the
# ELF's build attributes instead of asking this stub. An M-profile ELF
# happens to match our 17-register (r0-r12, sp, lr, pc, cpsr) reply, but
# an R/A-profile, hard-float ELF makes GDB assume the legacy 26-register
# set (16 GPRs + 8 FPA + fps + cpsr) and fail to parse our shorter 'g'
# packet reply. Advertising this minimal description avoids the guess.
_TARGET_XML = (
    b'<?xml version="1.0"?>'
    b'<!DOCTYPE target SYSTEM "gdb-target.dtd">'
    b'<target>'
    b'<architecture>arm</architecture>'
    b'<feature name="org.gnu.gdb.arm.core">'
    b'<reg name="r0" bitsize="32"/>'
    b'<reg name="r1" bitsize="32"/>'
    b'<reg name="r2" bitsize="32"/>'
    b'<reg name="r3" bitsize="32"/>'
    b'<reg name="r4" bitsize="32"/>'
    b'<reg name="r5" bitsize="32"/>'
    b'<reg name="r6" bitsize="32"/>'
    b'<reg name="r7" bitsize="32"/>'
    b'<reg name="r8" bitsize="32"/>'
    b'<reg name="r9" bitsize="32"/>'
    b'<reg name="r10" bitsize="32"/>'
    b'<reg name="r11" bitsize="32"/>'
    b'<reg name="r12" bitsize="32"/>'
    b'<reg name="sp" bitsize="32" type="data_ptr"/>'
    b'<reg name="lr" bitsize="32"/>'
    b'<reg name="pc" bitsize="32" type="code_ptr"/>'
    b'<reg name="cpsr" bitsize="32"/>'
    b'</feature>'
    b'</target>'
)


def _bin_escape(data):
    """Escape '#', '$', '*' and '}' per the GDB remote protocol's binary encoding."""
    out = bytearray()
    for b in data:
        if b in (0x23, 0x24, 0x2A, 0x7D):
            out.append(0x7D)
            out.append(b ^ 0x20)
        else:
            out.append(b)
    return bytes(out)


class GdbStub_ARM_CortexAR(GdbStub_ARM_AArch32):
    """GDB stub for ARM Cortex-A/R (AArch32).

    Register parsing and 'g'/'p'/'P' packet handling come from the
    shared GdbStub_ARM_AArch32 base (see arm_aarch32.py). Advertises a
    target description so GDB uses our register layout -- see
    _TARGET_XML above.

    Thread register operations require threads metadata which is not
    yet enabled for Cortex-A/R, so that path is disabled.
    """

    def arch_supports_thread_operations(self):
        return False

    def handle_general_query_packet(self, pkt):
        if pkt.startswith(b"qSupported"):
            self.put_gdb_packet(b"qXfer:features:read+;PacketSize=1000")
        elif pkt.startswith(b"qXfer:features:read:target.xml:"):
            offset_str, length_str = pkt.rsplit(b':', 1)[1].split(b',')
            offset = int(offset_str, 16)
            length = int(length_str, 16)

            chunk = _TARGET_XML[offset : offset + length]
            more_data = (offset + len(chunk)) < len(_TARGET_XML)
            self.put_gdb_packet((b'm' if more_data else b'l') + _bin_escape(chunk))
        else:
            super().handle_general_query_packet(pkt)
