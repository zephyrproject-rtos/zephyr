/*
 * Copyright (c) 2018 Synopsys, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Linker script for the Synopsys ARC IoT Development Kit
 */

#include <devicetree.h>
#include <autoconf.h>

/*
 * SRAM base address and size
 *
 */
#if DT_NODE_HAS_PROP(DT_CHOSEN(zephyr_sram), reg) && \
    (DT_REG_SIZE(DT_CHOSEN(zephyr_sram)) > 0)
#define SRAM_START		DT_REG_ADDR(DT_CHOSEN(zephyr_sram))
#define SRAM_SIZE		DT_REG_SIZE(DT_CHOSEN(zephyr_sram))
#endif

#if DT_NODE_HAS_PROP(DT_CHOSEN(zephyr_flash), reg) && \
    (DT_REG_SIZE(DT_CHOSEN(zephyr_flash)) > 0)
#define FLASH_START		DT_REG_ADDR(DT_CHOSEN(zephyr_flash))
#define FLASH_SIZE		DT_REG_SIZE(DT_CHOSEN(zephyr_flash))
#endif

/* Instruction Closely Coupled Memory (ICCM) base address and size */
#if DT_NODE_HAS_PROP(DT_INST(0, arc_iccm), reg) && \
    (DT_REG_SIZE(DT_INST(0, arc_iccm)) > 0)
#define ICCM_START		DT_REG_ADDR(DT_INST(0, arc_iccm))
#define ICCM_SIZE		DT_REG_SIZE(DT_INST(0, arc_iccm))
#endif

/*
 * DCCM base address and size. DCCM is the data memory.
 */
#if DT_NODE_HAS_PROP(DT_INST(0, arc_dccm), reg) && \
    (DT_REG_SIZE(DT_INST(0, arc_dccm)) > 0)
#define DCCM_START		DT_REG_ADDR(DT_INST(0, arc_dccm))
#define DCCM_SIZE		DT_REG_SIZE(DT_INST(0, arc_dccm))
#endif

#include <arch/arc/v2/linker.ld>
