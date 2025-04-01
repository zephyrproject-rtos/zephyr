/*
 * Copyright (c) 2025 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/arch/common/sys_io.h>

#define KICK0_OFFSET     (0x1008)
#define KICK1_OFFSET     (0x100C)
#define KICK0_UNLOCK_VAL (0x68EF3490U)
#define KICK1_UNLOCK_VAL (0xD172BC5AU)
#define KICK_LOCK_VAL    (0x0U)

#define CTRL_PARTITION_SIZE        (0x4000)
#define CTRL_PARTITION(base, part) ((base) + (part) * CTRL_PARTITION_SIZE)

#if defined CONFIG_SOC_AM6442_M4
#define MCU_PADCFG_BASE (0x4080000)
#elif defined CONFIG_SOC_AM6234_M4 | defined CONFIG_SOC_AM6232_M4
#define WKUP_PADCFG_BASE (0x4080000)
#endif

static const uintptr_t ctrl_partitions[] = {
#if defined CONFIG_SOC_AM6442_M4
	CTRL_PARTITION(MCU_PADCFG_BASE, 0),
	CTRL_PARTITION(MCU_PADCFG_BASE, 1),
#elif defined CONFIG_SOC_AM6234_M4 | defined CONFIG_SOC_AM6232_M4
	CTRL_PARTITION(WKUP_PADCFG_BASE, 0),
	CTRL_PARTITION(WKUP_PADCFG_BASE, 1),
#endif
};

void k3_unlock_all_ctrl_partitions(void)
{
	ARRAY_FOR_EACH(ctrl_partitions, i) {
		mm_reg_t base_addr = ctrl_partitions[i];

#ifdef DEVICE_MMIO_IS_IN_RAM
		device_map(&base_addr, base_addr, sizeof(base_addr), K_MEM_CACHE_NONE);
#endif

		sys_write32(KICK0_UNLOCK_VAL, base_addr + KICK0_OFFSET);
		sys_write32(KICK1_UNLOCK_VAL, base_addr + KICK1_OFFSET);
	}
}
