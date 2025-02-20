/*
 * Copyright (c) 2020 Mario Jaun
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(mac))

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(sram3))
#define sram_eth_node	DT_NODELABEL(sram3)
#else
#define sram_eth_node	DT_NODELABEL(sram2)
#endif

#if DT_NODE_HAS_STATUS_OKAY(sram_eth_node)
SECTION_DATA_PROLOGUE(eth_stm32,(NOLOAD),)
{
    . = ABSOLUTE(DT_REG_ADDR(sram_eth_node));
    *(.eth_stm32_desc)
    . = ABSOLUTE(DT_REG_ADDR(sram_eth_node)) + 256;
    *(.eth_stm32_buf)
    . = ABSOLUTE(DT_REG_ADDR(sram_eth_node)) + 16K;
} GROUP_DATA_LINK_IN(LINKER_DT_NODE_REGION_NAME(sram_eth_node), LINKER_DT_NODE_REGION_NAME(sram_eth_node))
#endif

#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(mac), okay) */
