/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(mac))

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(axisram2))
#define sram_eth_node	DT_NODELABEL(axisram2)
#else
#define sram_eth_node	DT_NODELABEL(axisram1)
#endif

#if DT_NODE_HAS_STATUS_OKAY(sram_eth_node)
SECTION_DATA_PROLOGUE(eth_stm32,(NOLOAD),)
{
     . = ABSOLUTE(DT_REG_ADDR(sram_eth_node));
    *(.RxDecripSection)
    . = ABSOLUTE(DT_REG_ADDR(sram_eth_node) + 0xC0);
    *(.TxDecripSection)
    . = ABSOLUTE(DT_REG_ADDR(sram_eth_node) + 0x4000);
    *(.eth_stm32_buf)
} GROUP_DATA_LINK_IN(LINKER_DT_NODE_REGION_NAME(sram_eth_node), LINKER_DT_NODE_REGION_NAME(sram_eth_node))
#endif

#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(mac), okay) */
