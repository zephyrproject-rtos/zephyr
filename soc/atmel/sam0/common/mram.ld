/* SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors */
/* SPDX-License-Identifier: Apache-2.0 */

SECTION_PROLOGUE(.mram_noinit,(NOLOAD),)
{
    . = ALIGN(4);
    *(.mram_noinit)
    . = ALIGN(4);
} > RAM

ASSERT((. - 0x20000000) <= 0x10000, "Error: SAM0 MCAN MRAM buffers exceed 64 KB SRAM boundary!")
