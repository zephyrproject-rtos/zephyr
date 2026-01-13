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
#define NPU_CACHE_ERR_IRQ_ENABLE  (1 << 2)

static int npu_cache_stm32_enable(const struct device *dev)
{
	/* Disable cache */
	*((volatile uint32_t *)(NPU_CACHE_BASE + NPU_CACHE_CTRL_REG_OFFSET)) = NPU_CACHE_DISABLE;

	k_busy_wait(5 * 1000); /* 5ms delay */

	/* Enable cache */
	*((volatile uint32_t *)(NPU_CACHE_BASE + NPU_CACHE_CTRL_REG_OFFSET)) = NPU_CACHE_ENABLE;

	/* Enable cache counters */
	*((volatile uint32_t *)(NPU_CACHE_BASE + NPU_CACHE_CTRL_REG_OFFSET)) |=
		NPU_CACHE_CNT_ENABLE;

	/* Reset cache counters */
	*((volatile uint32_t *)(NPU_CACHE_BASE + NPU_CACHE_CTRL_REG_OFFSET)) |= NPU_CACHE_CNT_RESET;

	/* Enable cache error interrupt */
	*((volatile uint32_t *)(NPU_CACHE_BASE + NPU_CACHE_ERR_IRQ_OFFSET)) =
		NPU_CACHE_ERR_IRQ_ENABLE;

	return 0;
}

#define NPU_CACHE_DEVICE_ENABLE(inst)                                                              \
	DEVICE_DT_INST_DEFINE(inst, npu_cache_stm32_enable, NULL, NULL, NULL, POST_KERNEL,         \
			      CONFIG_STM32N6_NPU_CACHE_INIT_PRIORITY, NULL);

/* This automatically creates a device for every 'st,stm32-npu-cache' node marked 'okay' */
DT_INST_FOREACH_STATUS_OKAY(NPU_CACHE_DEVICE_ENABLE)
