/*
 * Copyright (c) 2020 Synopsys, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>

/*
 * SRAM base address and size
 */
#if DT_NODE_HAS_PROP(DT_CHOSEN(zephyr_sram), reg) && \
    (DT_REG_SIZE(DT_CHOSEN(zephyr_sram)) > 0)
#define SRAM_START		DT_REG_ADDR(DT_CHOSEN(zephyr_sram))
#define SRAM_SIZE		DT_REG_SIZE(DT_CHOSEN(zephyr_sram))
#endif

/*
 * flash base address and size
 */
#if DT_NODE_HAS_PROP(DT_CHOSEN(zephyr_flash), reg) && \
    (DT_REG_SIZE(DT_CHOSEN(zephyr_flash)) > 0)
#define FLASH_START		DT_REG_ADDR(DT_CHOSEN(zephyr_flash))
#define FLASH_SIZE		DT_REG_SIZE(DT_CHOSEN(zephyr_flash))
#endif

#include <zephyr/arch/arc/v2/linker.ld>
