/*
 * Copyright (c) 2025 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/arch/common/sys_io.h>

#define KICK0_UNLOCK_VAL (0x68EF3490U)
#define KICK1_UNLOCK_VAL (0xD172BC5AU)

#define K3_UNLOCK_CONTROL_MODULE_(node_id)                                                         \
	const uint32_t conf_##node_id[] = DT_PROP(node_id, ti_unlock_offsets);                     \
	base = DT_REG_ADDR(node_id);                                                               \
	device_map(&base, base, DT_REG_SIZE(node_id), K_MEM_CACHE_NONE);                           \
	ARRAY_FOR_EACH(conf_##node_id, i) {                                                        \
		k3_unlock_partition(conf_##node_id[i] + base);                                     \
	}                                                                                          \
	IF_ENABLED(CONFIG_MMU, (k_mem_unmap_phys_bare((uint8_t *)base, DT_REG_SIZE(node_id));))

#define K3_UNLOCK_CONTROL_MODULE(node_id)                                                          \
	IF_ENABLED(DT_NODE_HAS_PROP(node_id, ti_unlock_offsets),                                   \
		   (K3_UNLOCK_CONTROL_MODULE_(node_id)))

static void k3_unlock_partition(mem_addr_t kick0_address)
{
	sys_write32(KICK0_UNLOCK_VAL, kick0_address);
	sys_write32(KICK1_UNLOCK_VAL, kick0_address + 4);
}

void k3_unlock_all_ctrl_partitions(void)
{
#if DT_HAS_COMPAT_STATUS_OKAY(ti_control_module)
	uintptr_t base;
#endif

	DT_FOREACH_STATUS_OKAY(ti_control_module, K3_UNLOCK_CONTROL_MODULE);
}
