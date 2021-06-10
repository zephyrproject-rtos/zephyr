/*
 * Copyright (c) 2020 Mario Jaun
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if DT_NODE_HAS_STATUS(DT_ALIAS(sramnocache), okay) && DT_NODE_HAS_STATUS(DT_NODELABEL(mac), okay)

SECTION_DATA_PROLOGUE(eth_stm32,(NOLOAD),)
{
    . = ABSOLUTE(DT_REG_ADDR(DT_ALIAS(sramnocache)));
    *(.eth_stm32_desc)
    . = ABSOLUTE(DT_REG_ADDR(DT_ALIAS(sramnocache))) + 256;
    *(.eth_stm32_buf)
    . = ABSOLUTE(DT_REG_ADDR(DT_ALIAS(sramnocache))) + 16K;
} GROUP_DATA_LINK_IN(sramnocache, sramnocache)

#endif
