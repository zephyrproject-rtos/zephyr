/*
 * Copyright (c) 2026 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_npu_cache

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <soc.h>

#define NPU_CACHE_NODE DT_DRV_INST(0)
#define NPU_CACHE_BASE DT_REG_ADDR(NPU_CACHE_NODE)

#define NPU_CACHE_CTRL_REG_OFFSET 0x0
#define NPU_CACHE_ERR_IRQ_OFFSET  0x8
#define NPU_CACHE_DISABLE         0x0
#define NPU_CACHE_ENABLE          0x1
#define NPU_CACHE_CNT_ENABLE      0x33330000
#define NPU_CACHE_CNT_RESET       0xCCCC0000
#define NPU_CACHE_ERR_IRQ_ENABLE  BIT(2)

/*
 * Define npu_cache_stm32_enable and instantiate the device only if both
 * the NPU and NPU cache nodes are enabled in the device tree.
 * This avoids unused function warnings when the cache is disabled.
 */
#if DT_NODE_HAS_STATUS_OKAY(DT_INST(0, st_stm32_npu)) &&                                           \
	DT_NODE_HAS_STATUS_OKAY(DT_INST(0, st_stm32_npu_cache))
static int npu_cache_stm32_enable(const struct device *dev)
{
	/* Disable cache */
	sys_write32(NPU_CACHE_DISABLE, NPU_CACHE_BASE + NPU_CACHE_CTRL_REG_OFFSET);

	k_busy_wait(5 * USEC_PER_MSEC); /* 5ms delay */

	/* Enable cache */
	sys_write32(NPU_CACHE_ENABLE, NPU_CACHE_BASE + NPU_CACHE_CTRL_REG_OFFSET);

	/* Enable cache counters */
	sys_set_bits(NPU_CACHE_BASE + NPU_CACHE_CTRL_REG_OFFSET, NPU_CACHE_CNT_ENABLE);

	/* Reset cache counters */
	sys_set_bits(NPU_CACHE_BASE + NPU_CACHE_CTRL_REG_OFFSET, NPU_CACHE_CNT_RESET);

	/* Enable cache error interrupt */
	sys_write32(NPU_CACHE_ERR_IRQ_ENABLE, NPU_CACHE_BASE + NPU_CACHE_ERR_IRQ_OFFSET);

	return 0;
}

DEVICE_DT_INST_DEFINE(0, npu_cache_stm32_enable, NULL, NULL, NULL, POST_KERNEL,
		      CONFIG_STM32N6_NPU_CACHE_INIT_PRIORITY, NULL);

/*
 * Guarantee that initialization priority for the NPU is higher than the one of the NPU cache (aka
 * CACHEAXI)
 */
BUILD_ASSERT(CONFIG_STM32N6_NPU_CACHE_INIT_PRIORITY > CONFIG_STM32N6_NPU_INIT_PRIORITY,
	     "NPU cache initialization must run after NPU driver initialization");
#endif /* st_stm32_npu && st_stm32_npu_cache */
