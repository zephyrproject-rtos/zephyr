/*
 * Copyright (c) 2025 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/arch/common/sys_io.h>

#if defined(CONFIG_SOC_AM2612_R5F)
#define MSS_RCM_KICK0_OFFSET (0x1008)
#define MSS_RCM_KICK1_OFFSET (0x100C)

#define MSS_RCM_KICK_LOCK_VAL    (0x00000000)
#define MSS_RCM_KICK0_UNLOCK_VAL (0x01234567)
#define MSS_RCM_KICK1_UNLOCK_VAL (0xfedcba8)

#define IOMUX_KICK0_OFFSET (0x000002A4U)
#define IOMUX_KICK1_OFFSET (0x000002A8U)

#define IOMUX_KICK_LOCK_VAL    (0x00000000U)
#define IOMUX_KICK0_UNLOCK_VAL (0x83E70B13U)
#define IOMUX_KICK1_UNLOCK_VAL (0x95A4F1E0U)

#define MSS_RCM_BASE (0x53208000)
#define IOMUX_BASE   (0x53100000ul)
#endif /* CONFIG_SOC_AM2612_R5F */

typedef enum {
	MSS_RCM = 0,
	IOMUX
} ctrl_partition_t;

typedef struct {
	uintptr_t base_addr;
	uint32_t kick_lock_value;
	uint32_t kick0_unlock_value;
	uint32_t kick1_unlock_value;
	uint32_t kick0_offset;
	uint32_t kick1_offset;
} ctrl_partition_lock_info_t;

static const ctrl_partition_lock_info_t ctrl_partitions[] = {
#if defined CONFIG_SOC_AM2612_R5F
	[MSS_RCM] = {.base_addr = MSS_RCM_BASE,
		     .kick_lock_value = MSS_RCM_KICK_LOCK_VAL,
		     .kick0_unlock_value = MSS_RCM_KICK0_UNLOCK_VAL,
		     .kick1_unlock_value = MSS_RCM_KICK1_UNLOCK_VAL,
		     .kick0_offset = MSS_RCM_KICK0_OFFSET,
		     .kick1_offset = MSS_RCM_KICK1_OFFSET},
	[IOMUX] = {.base_addr = IOMUX_BASE,
		   .kick_lock_value = IOMUX_KICK_LOCK_VAL,
		   .kick0_unlock_value = IOMUX_KICK0_UNLOCK_VAL,
		   .kick1_unlock_value = IOMUX_KICK1_UNLOCK_VAL,
		   .kick0_offset = IOMUX_KICK0_OFFSET,
		   .kick1_offset = IOMUX_KICK1_OFFSET}
#endif
};

void am26x_unlock_all_ctrl_partitions(void)
{
	ARRAY_FOR_EACH(ctrl_partitions, i) {
		mm_reg_t base_addr = ctrl_partitions[i].base_addr;
		uint32_t kick0_unlock_value = ctrl_partitions[i].kick0_unlock_value;
		uint32_t kick1_unlock_value = ctrl_partitions[i].kick1_unlock_value;
		uint32_t kick0_offset = ctrl_partitions[i].kick0_offset;
		uint32_t kick1_offset = ctrl_partitions[i].kick1_offset;

#ifdef DEVICE_MMIO_IS_IN_RAM
		device_map(&base_addr, base_addr, sizeof(base_addr), K_MEM_CACHE_NONE);
#endif

		sys_write32(kick0_unlock_value, base_addr + kick0_offset);
		sys_write32(kick1_unlock_value, base_addr + kick1_offset);
	}
}

void am26x_lock_all_ctrl_partitions(void)
{
	ARRAY_FOR_EACH(ctrl_partitions, i) {
		mm_reg_t base_addr = ctrl_partitions[i].base_addr;
		uint32_t kick_lock_value = ctrl_partitions[i].kick_lock_value;
		uint32_t kick0_offset = ctrl_partitions[i].kick0_offset;
		uint32_t kick1_offset = ctrl_partitions[i].kick1_offset;
#ifdef DEVICE_MMIO_IS_IN_RAM
		device_map(&base_addr, base_addr, sizeof(base_addr), K_MEM_CACHE_NONE);
#endif
		sys_write32(kick_lock_value, base_addr + kick1_offset);
		sys_write32(kick_lock_value, base_addr + kick0_offset);
	}
}
