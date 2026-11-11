/*
 * Copyright (c) 2026 Conclusive Engineering Sp. z o.o.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/multi_heap/shared_multi_heap.h>

#if DT_NODE_HAS_STATUS(DT_CHOSEN(zephyr_dtcm), okay)

static struct shared_multi_heap_region dtcm_region = {
	.addr = (uintptr_t)DT_REG_ADDR(DT_CHOSEN(zephyr_dtcm)),
	.size = DT_REG_SIZE(DT_CHOSEN(zephyr_dtcm)),
	.attr = SMH_REG_ATTR_NON_CACHEABLE,
};

static int cc35xx_multi_heap_init(void)
{
	int ret = shared_multi_heap_pool_init();

	return ret ? ret : shared_multi_heap_add(&dtcm_region, NULL);
}

SYS_INIT(cc35xx_multi_heap_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#endif /* DT_NODE_HAS_STATUS(DT_CHOSEN(zephyr_dtcm), okay) */
