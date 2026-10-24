/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/toolchain.h>

#define SRAM_APP DT_NODELABEL(cpuapp_sram_app)

#if DT_NODE_EXISTS(SRAM_APP)

#define SRAM_APP_PARENT DT_PARENT(SRAM_APP)

#define REG_END(node_id) (DT_REG_ADDR(node_id) + DT_REG_SIZE(node_id))

BUILD_ASSERT(DT_REG_ADDR(SRAM_APP) >= DT_REG_ADDR(SRAM_APP_PARENT) &&
		     REG_END(SRAM_APP) <= REG_END(SRAM_APP_PARENT),
	     "cpuapp_sram_app is not fully contained within cpuapp_sram. Resize cpuapp_sram_app "
	     "and not cpuapp_sram when you shrink the RAM of the application.");

#endif
