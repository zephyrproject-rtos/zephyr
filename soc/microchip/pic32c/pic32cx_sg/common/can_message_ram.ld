/* SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors */
/* SPDX-License-Identifier: Apache-2.0 */

/*
 * Place the M_CAN message RAM buffers at the start of SRAM. The M_CAN start
 * address fields hold 16-bit offsets from 0x20000000, so the buffers must lie
 * in the first 64 KB of SRAM.
 */

SECTION_PROLOGUE(.can_message_ram,(NOLOAD),)
{
    . = ALIGN(4);
    *(.can_message_ram)
    . = ALIGN(4);
} GROUP_LINK_IN(RAMABLE_REGION)

ASSERT((. - RAM_ADDR) <= KB(64), "Error: M_CAN message RAM buffers exceed the first 64 KB of SRAM!")
