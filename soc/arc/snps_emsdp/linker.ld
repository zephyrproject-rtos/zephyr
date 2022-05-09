/*
 * Copyright (c) 2019 Synopsys, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Linker script for the Synopsys ARC IoT Development Kit
 */

#include <zephyr/devicetree.h>

/*
 * SRAM base address and size
 *
 */
#if DT_NODE_HAS_PROP(DT_CHOSEN(zephyr_sram), reg) && \
    (DT_REG_SIZE(DT_CHOSEN(zephyr_sram)) > 0)
#define SRAM_START		DT_REG_ADDR(DT_CHOSEN(zephyr_sram))
#define SRAM_SIZE		DT_REG_SIZE(DT_CHOSEN(zephyr_sram))
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
/* Data Closely Coupled Memory (DCCM) base address and size */
#if DT_NODE_HAS_PROP(DT_INST(0, arc_dccm), reg) && \
    (DT_REG_SIZE(DT_INST(0, arc_dccm)) > 0)
#define DCCM_START		DT_REG_ADDR(DT_INST(0, arc_dccm))
#define DCCM_SIZE		DT_REG_SIZE(DT_INST(0, arc_dccm))
#endif

#include <zephyr/arch/arc/v2/linker.ld>
