/*
 * Copyright (c) 2020 Mario Jaun
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(sram2), okay)

SECTION_DATA_PROLOGUE(sram2_nocache,(NOLOAD),)
{
    . = ABSOLUTE(DT_REG_ADDR(DT_NODELABEL(sram2)));
    *(.sram2_nocache)
    . = ABSOLUTE(DT_REG_ADDR(DT_NODELABEL(sram2))) + 16K;
} GROUP_DATA_LINK_IN(SRAM2, SRAM2)

#endif