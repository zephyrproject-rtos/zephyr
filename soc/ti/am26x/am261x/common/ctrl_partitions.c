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

#define KICK_LOCK_VAL (0x0U)
#endif

#define CTRL_PARTITION_SIZE        (0x4000)
#define CTRL_PARTITION(base, part) ((base) + (part) * CTRL_PARTITION_SIZE)

#if defined CONFIG_SOC_AM2612_R5F
#define MSS_RCM_BASE (0x53208000)
#define IOMUX_BASE   (0x53100000ul)
#endif /* CONFIG_SOC_AM2612_R5F */

static const uintptr_t ctrl_partitions[] = {
#if defined CONFIG_SOC_AM2612_R5F
	CTRL_PARTITION(MSS_RCM_BASE, 0)
#endif
};

void am26x_unlock_all_ctrl_partitions(void)
{
	ARRAY_FOR_EACH(ctrl_partitions, i) {
		mm_reg_t base_addr = ctrl_partitions[i];

#ifdef DEVICE_MMIO_IS_IN_RAM
		device_map(&base_addr, base_addr, sizeof(base_addr), K_MEM_CACHE_NONE);
#endif

		sys_write32(MSS_RCM_KICK0_UNLOCK_VAL, base_addr + MSS_RCM_KICK0_OFFSET);
		sys_write32(MSS_RCM_KICK1_UNLOCK_VAL, base_addr + MSS_RCM_KICK1_OFFSET);
	}
}
