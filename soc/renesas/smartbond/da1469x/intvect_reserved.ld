/*
 * Copyright (c) 2022 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

SECTION_PROLOGUE(.da1469x_intvect_reserved,(NOLOAD),)
{
    /* Access to addresses in range 0x0-0x1ff is remapped to sram to allow rw
     * access to interrupt vector which is located at 0x0 where flash is remapped.
     * This means first 512 bytes of sram has to be reserved so remapped access does
     * not corrupt data in sram.
     */
    . += 0x200;
} GROUP_DATA_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)
