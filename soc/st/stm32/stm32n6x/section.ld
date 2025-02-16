/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(mac), okay)
SECTION_DATA_PROLOGUE(eth_stm32, (NOLOAD),)
{
#if DT_NODE_HAS_STATUS(DT_NODELABEL(axisram2), okay)
    . = ABSOLUTE(DT_REG_ADDR(DT_NODELABEL(axisram2)));
    *(.RxDecripSection)
    . = ABSOLUTE(DT_REG_ADDR(DT_NODELABEL(axisram2)) + 0xC0);
    *(.TxDecripSection)
    . = ABSOLUTE(DT_REG_ADDR(DT_NODELABEL(axisram2)) + 0x4000);
    *(.eth_stm32_buf)
} GROUP_DATA_LINK_IN(LINKER_DT_NODE_REGION_NAME(DT_NODELABEL(axisram2)), LINKER_DT_NODE_REGION_NAME(DT_NODELABEL(axisram2)))

#else
    . = ABSOLUTE(DT_REG_ADDR(DT_NODELABEL(axisram1)));
    *(.RxDecripSection)
    . = ABSOLUTE(DT_REG_ADDR(DT_NODELABEL(axisram1)) + 0xC0);
    *(.TxDecripSection)
    . = ABSOLUTE(DT_REG_ADDR(DT_NODELABEL(axisram1)) + 0x4000);
    *(.eth_stm32_buf)
} GROUP_DATA_LINK_IN(LINKER_DT_NODE_REGION_NAME(DT_NODELABEL(axisram1)), LINKER_DT_NODE_REGION_NAME(DT_NODELABEL(axisram1)))
#endif
#endif
