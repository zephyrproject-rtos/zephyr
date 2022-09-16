/* Copyright(c) 2022 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/xtensa/cache.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#include <adsp_memory.h>
#include <adsp_shim.h>
#include <mem_window.h>
#include <soc.h>

/* host windows */
#define DMWBA(win_base) (win_base + 0x0)
#define DMWLO(win_base) (win_base + 0x4)

#define DT_DRV_COMPAT intel_adsp_mem_window

#define WIN_SIZE(N)   (CONFIG_MEMORY_WIN_##N##_SIZE)
#define WIN0_OFFSET DT_PROP(DT_NODELABEL(mem_window0), offset)
#define WIN1_OFFSET WIN0_OFFSET + WIN_SIZE(0)
#define WIN2_OFFSET WIN1_OFFSET + WIN_SIZE(1)
#define WIN3_OFFSET WIN2_OFFSET + WIN_SIZE(2)

#define WIN_OFFSET(N) (WIN##N##_OFFSET)

__imr int mem_win_init(const struct device *dev)
{
	const struct mem_win_config *config = dev->config;

	if (config->initialize) {
		uint32_t *win = z_soc_uncached_ptr((void *)config->mem_base);
		/* Software protocol: "firmware entered" has the value 5 */
		win[0] = 5;
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

#define MEM_WIN_INIT(inst)                                                                         \
	static const struct mem_win_config mem_win_config_##inst = {                               \
		.base_addr = DT_INST_REG_ADDR(inst),                                               \
		.size = WIN_SIZE(inst),                                                            \
		.offset = WIN_OFFSET(inst),                                                        \
		.read_only = DT_INST_PROP(inst, read_only),					   \
		.mem_base = DT_REG_ADDR(DT_INST_PHANDLE(inst, memory)) + WIN_OFFSET(inst),         \
		.initialize = DT_INST_PROP(inst, initialize),                                      \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, mem_win_init, NULL, NULL, &mem_win_config_##inst, ARCH,        \
			      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, NULL);

DT_INST_FOREACH_STATUS_OKAY(MEM_WIN_INIT)
