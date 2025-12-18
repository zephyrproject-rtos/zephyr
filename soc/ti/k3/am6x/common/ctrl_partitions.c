/*
 * Copyright (c) 2025 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
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

#if defined CONFIG_SOC_AM6442_M4 || defined CONFIG_SOC_AM2434_M4
#define MCU_PADCFG_BASE (0x4080000)
#elif defined CONFIG_SOC_AM6234_M4 | defined CONFIG_SOC_AM6232_M4
#define WKUP_PADCFG_BASE (0x4080000)
#elif defined CONFIG_SOC_AM2434_R5F0_0
#define DT_TIMER_INST                                                                              \
	((DT_REG_ADDR(DT_ALIAS(system_timer)) - DT_REG_ADDR(DT_NODELABEL(main_timer0))) / 0x10000)
#define MCU_PADCFG_BASE            (0x4080000)
#define MAIN_PADCFG_BASE           (0xf0000)
#define MAIN_MMR_BASE              (0x43000000)
#define MAIN_MMR_TIMER_CLKSEL_PART CTRL_PARTITION(MAIN_MMR_BASE, 2)
#define MAIN_MMR_TIMER_CLKSEL_VAL  (0x0) /* HFOSC0_CLKOUT */
#define MAIN_MMR_TIMER_CLKSEL      (MAIN_MMR_BASE + 0x81B0 + (DT_TIMER_INST * 4))
#endif

static const uintptr_t ctrl_partitions[] = {
#if defined CONFIG_SOC_AM6442_M4 || defined CONFIG_SOC_AM2434_M4
	CTRL_PARTITION(MCU_PADCFG_BASE, 0),
	CTRL_PARTITION(MCU_PADCFG_BASE, 1),
#elif defined CONFIG_SOC_AM6234_M4 | defined CONFIG_SOC_AM6232_M4
	CTRL_PARTITION(WKUP_PADCFG_BASE, 0),
	CTRL_PARTITION(WKUP_PADCFG_BASE, 1),
#elif defined CONFIG_SOC_AM2434_R5F0_0
	CTRL_PARTITION(MAIN_PADCFG_BASE, 0),
	CTRL_PARTITION(MAIN_PADCFG_BASE, 1),
	CTRL_PARTITION(MCU_PADCFG_BASE, 0),
	CTRL_PARTITION(MCU_PADCFG_BASE, 1),
#endif
};

void k3_write_mmr(uint32_t value, mm_reg_t base_addr)
{
#ifdef DEVICE_MMIO_IS_IN_RAM
	device_map(&base_addr, base_addr, sizeof(base_addr), K_MEM_CACHE_NONE);
#endif

	sys_write32(value, base_addr);
}

void k3_unlock_partition(mm_reg_t base_addr)
{
	k3_write_mmr(KICK0_UNLOCK_VAL, base_addr + KICK0_OFFSET);
	k3_write_mmr(KICK1_UNLOCK_VAL, base_addr + KICK1_OFFSET);
}

void k3_lock_partition(mm_reg_t base_addr)
{
	k3_write_mmr(KICK_LOCK_VAL, base_addr + KICK0_OFFSET);
	k3_write_mmr(KICK_LOCK_VAL, base_addr + KICK1_OFFSET);
}

void k3_unlock_all_ctrl_partitions(void)
{
	ARRAY_FOR_EACH(ctrl_partitions, i) {
		k3_unlock_partition(ctrl_partitions[i]);
	}

/* Currently setting clock parent is not possible via standard clock control API,
 * hence do this for now
 */
#ifdef MAIN_MMR_TIMER_CLKSEL
	k3_unlock_partition(MAIN_MMR_TIMER_CLKSEL_PART);
	k3_write_mmr(MAIN_MMR_TIMER_CLKSEL_VAL, MAIN_MMR_TIMER_CLKSEL);
	k3_lock_partition(MAIN_MMR_TIMER_CLKSEL_PART);
#endif
}
