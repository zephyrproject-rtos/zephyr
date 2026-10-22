/* SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors */
/* SPDX-License-Identifier: Apache-2.0 */

/*
 * Place the MCAN Message RAM buffers in a dedicated section to ensure they are
 * located in the first 64 KB of SRAM, as required by section 39.9.1 of the SAM
 * D5x/E5x Family Data Sheet (DS60001507N).
 */

SECTION_PROLOGUE(.can_message_ram,(NOLOAD),)
{
    . = ALIGN(4);
    *(.can_message_ram)
    . = ALIGN(4);
} GROUP_LINK_IN(RAMABLE_REGION)

ASSERT((. - RAM_ADDR) <= KB(64), "Error: SAM0 MCAN MRAM buffers exceed 64 KB SRAM boundary!")
