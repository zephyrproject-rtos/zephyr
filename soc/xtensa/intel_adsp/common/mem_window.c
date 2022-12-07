/* Copyright(c) 2022 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/xtensa/cache.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/arch/cpu.h>

#include <adsp_memory.h>
#include <adsp_shim.h>
#include <mem_window.h>
#include <soc.h>

/* host windows */
#define DMWBA(win_base) (win_base + 0x0)
#define DMWLO(win_base) (win_base + 0x4)

#define DT_DRV_COMPAT intel_adsp_mem_window

__imr int mem_win_init(const struct device *dev)
{
	const struct mem_win_config *config = dev->config;

	if (config->initialize) {
		bbzero((void *)config->mem_base, config->size);
	}

	sys_write32(config->size | 0x7, DMWLO(config->base_addr));
	if (config->read_only) {
		sys_write32((config->mem_base | ADSP_DMWBA_READONLY | ADSP_DMWBA_ENABLE),
			    DMWBA(config->base_addr));
	} else {
		sys_write32((config->mem_base | ADSP_DMWBA_ENABLE), DMWBA(config->base_addr));
	}

	return 0;
}

#define MEM_WINDOW_DEFINE(n)                                                                       \
	static const struct mem_win_config mem_win_config_##n = {                                  \
		.base_addr = DT_REG_ADDR(MEM_WINDOW_NODE(n)),                                      \
		.size = WIN_SIZE(n),                                                               \
		.offset = WIN_OFFSET(n),                                                           \
		.read_only = DT_PROP(MEM_WINDOW_NODE(n), read_only),                               \
		.mem_base = DT_REG_ADDR(DT_PHANDLE(MEM_WINDOW_NODE(n), memory)) + WIN_OFFSET(n),   \
		.initialize = DT_PROP(MEM_WINDOW_NODE(n), initialize),                             \
	};                                                                                         \
	DEVICE_DT_DEFINE(MEM_WINDOW_NODE(n), mem_win_init, NULL, NULL, &mem_win_config_##n, EARLY, \
			 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, NULL);

#if DT_NODE_HAS_STATUS(MEM_WINDOW_NODE(0), okay)
MEM_WINDOW_DEFINE(0)
#endif
#if DT_NODE_HAS_STATUS(MEM_WINDOW_NODE(1), okay)
MEM_WINDOW_DEFINE(1)
#endif
#if DT_NODE_HAS_STATUS(MEM_WINDOW_NODE(2), okay)
MEM_WINDOW_DEFINE(2)
#endif
#if DT_NODE_HAS_STATUS(MEM_WINDOW_NODE(3), okay)
MEM_WINDOW_DEFINE(3)
#endif
