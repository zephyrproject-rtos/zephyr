/*
 * Copyright (c) 2026 Realtek Semiconductor, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/arch/arm/mpu/arm_mpu.h>
#include <zephyr/arch/arm/cortex_m/scb.h>
#include <zephyr/arch/arm/cortex_m/fpu.h>
#include <zephyr/arch/common/pm_s2ram.h>
#include <zephyr/cache.h>
#include <zephyr/linker/sections.h>
#include <zephyr/sys/util.h>
#include <zephyr/dt-bindings/wuc/wuc_rts5817.h>
#include <cmsis_core.h>

#include "dlink_sys_reg.h"
#include "dlink_ups_reg.h"
#include "soc_pm_s2ram.h"

#define S2RAM_SUB_ID_OFFSET 3
#define S2RAM_SUB_ID_MASK   GENMASK(4, 3)

#define S2RAM_MARK_OFFSET 16
#define S2RAM_MARK_VALUE  0x5AA5
#define S2RAM_MARK_MASK   GENMASK(31, 16)

#define S2RAM_WAIT_COUNT 1000

#define DLINK_SYS_BASE DT_REG_ADDR(DT_NODELABEL(dlink_sys))
#define DLINK_UPS_BASE DT_REG_ADDR(DT_NODELABEL(dlink_ups))

#define USB_SIE_SYS_CTRL_BASE DT_REG_ADDR_BY_NAME(DT_NODELABEL(usb), u2sie_sys)

#define R_U2SIE_SYS_CTRL 0x0
#define SUSPEND_EN_MASK  BIT(2)

#define NVIC_MEMBER_SIZE(member) ARRAY_SIZE(((NVIC_Type *)0)->member)

typedef struct {
	uint32_t ISER[NVIC_MEMBER_SIZE(ISER)];
	uint32_t ISPR[NVIC_MEMBER_SIZE(ISPR)];
	uint8_t IPR[NVIC_MEMBER_SIZE(IPR)];
} nvic_context_t;

typedef struct {
	uint32_t CTRL;
	uint32_t LOAD;
	uint32_t VAL;
} systick_context_t;

struct backup {
	nvic_context_t nvic_ctx;
	systick_context_t systick_ctx;
#if defined(CONFIG_ARM_MPU)
	struct z_mpu_context_retained mpu_context;
#endif
	struct scb_context scb_ctx;
#if defined(CONFIG_FPU) && !defined(CONFIG_FPU_SHARING)
	struct fpu_ctx_full fpu_ctx;
#endif
};

static __noinit struct backup backup_data;

static inline void sys_write32_mask(uint32_t val, uint32_t mask, mem_addr_t reg)
{
	uint32_t v = sys_read32(reg) & ~(mask);

	sys_write32(v | (val & mask), reg);
}

void pm_s2ram_mark_set(void)
{
	sys_write32_mask(S2RAM_MARK_VALUE << S2RAM_MARK_OFFSET, S2RAM_MARK_MASK,
			 DLINK_SYS_BASE + R_AL_DUMMY1);
}

bool pm_s2ram_mark_check_and_clear(void)
{
	uint16_t marker;

	marker = (sys_read32(DLINK_SYS_BASE + R_AL_DUMMY1) & S2RAM_MARK_MASK) >> S2RAM_MARK_OFFSET;

	if (marker == S2RAM_MARK_VALUE) {
		sys_clear_bits(DLINK_SYS_BASE + R_AL_DUMMY1, S2RAM_MARK_MASK);
		return true;
	}

	return false;
}

static void nvic_save(nvic_context_t *backup)
{
	memcpy(backup->ISER, (uint32_t *)NVIC->ISER, sizeof(NVIC->ISER));
	memcpy(backup->ISPR, (uint32_t *)NVIC->ISPR, sizeof(NVIC->ISPR));
	memcpy(backup->IPR, (uint32_t *)NVIC->IPR, sizeof(NVIC->IPR));
}

static void nvic_restore(nvic_context_t *backup)
{
	memcpy((uint32_t *)NVIC->ISER, backup->ISER, sizeof(NVIC->ISER));
	memcpy((uint32_t *)NVIC->ISPR, backup->ISPR, sizeof(NVIC->ISPR));
	memcpy((uint32_t *)NVIC->IPR, backup->IPR, sizeof(NVIC->IPR));
}

static void systick_save(systick_context_t *backup)
{
	backup->CTRL = SysTick->CTRL;
	backup->LOAD = SysTick->LOAD;
	backup->VAL = SysTick->VAL;
}

static void systick_restore(systick_context_t *backup)
{
	SysTick->VAL = backup->VAL;
	SysTick->LOAD = backup->LOAD;
	SysTick->CTRL = backup->CTRL;
}

static __aligned(CONFIG_ICACHE_LINE_SIZE) __noinline
	int enter_low_power(mem_addr_t addr, uint32_t mask, uint32_t cnt)
{
	sys_set_bits(addr, mask);
	/* Delay to avoid execute subsequent code, because RTS5817 does not
	 * enter low power state immediately after setting the register.
	 */
	while (cnt) {
		cnt--;
		__NOP();
	}

	/* Normally the code wouldn't reach here. If it reaches here, it means that a wake-up
	 * event occurred during the process of entering low power mode. So return EBUSY here to
	 * indicate that failed to entering low power mode.
	 */
	return -EBUSY;
}

static void config_power_gating(void)
{
	sys_write32(ISO_EN_MASK | DV12S_POWEROFF_EN_MASK | RESUME_RST_EN_MASK | PLL_SW_ON_EN_MASK |
			    APHY_HANDLE_EN_MASK,
		    DLINK_SYS_BASE + R_AL_PG_EN);

	sys_set_bits(DLINK_SYS_BASE + R_CHIP_SRAM_PG, FW_CFG_SUS_SRAM_DS_MASK);

	sys_write32_mask(2 << REG_LVD_DEG_OFFSET, REG_LVD_DEG_MASK,
			 DLINK_UPS_BASE + R_UPS_ANA_CFG4);
}

static int soc_system_off(void)
{
	uint8_t substate_id = (sys_read32(DLINK_SYS_BASE + R_AL_DUMMY0) & S2RAM_SUB_ID_MASK) >>
			      S2RAM_SUB_ID_OFFSET;

	/* Flush DCache */
	sys_cache_data_flush_all();

	config_power_gating();

	switch (substate_id) {
	case RTS5817_S2RAM_SUB_ID_SUSPEND:
		sys_set_bits(DLINK_SYS_BASE + R_DLINK_PG_CTRL, CFG_HW_WAKEUP_EN_MASK);
		return enter_low_power(USB_SIE_SYS_CTRL_BASE + R_U2SIE_SYS_CTRL, SUSPEND_EN_MASK,
				       S2RAM_WAIT_COUNT);
	case RTS5817_S2RAM_SUB_ID_SLEEP:
		sys_set_bits(DLINK_SYS_BASE + R_EXIT_FLAG, PAD_EXIT_FLAG_CLR_PRE_MASK);
		sys_set_bits(DLINK_SYS_BASE + R_DLINK_PG_CTRL, CFG_SLEEP_PG_EN_MASK);
		sys_clear_bits(DLINK_SYS_BASE + R_DLINK_PG_CTRL, CFG_HW_WAKEUP_EN_MASK);
		return enter_low_power(DLINK_SYS_BASE + R_SLEEP_IN_OUT_CTRL, SLEEP_FW_ENTER_EN_MASK,
				       S2RAM_WAIT_COUNT);
	default:
		return -EINVAL;
	}
}

int soc_s2ram_suspend(uint8_t substate_id)
{
	int ret;

	z_arm_save_scb_context(&backup_data.scb_ctx);
#if defined(CONFIG_FPU) && !defined(CONFIG_FPU_SHARING)
	z_arm_save_fp_context(&backup_data.fpu_ctx);
#endif
	nvic_save(&backup_data.nvic_ctx);
	systick_save(&backup_data.systick_ctx);
#if defined(CONFIG_ARM_MPU)
	z_arm_save_mpu_context(&backup_data.mpu_context);
#endif
	/* Set substate_id to register */
	sys_write32_mask(substate_id << S2RAM_SUB_ID_OFFSET, S2RAM_SUB_ID_MASK,
			 DLINK_SYS_BASE + R_AL_DUMMY0);

	ret = arch_pm_s2ram_suspend(soc_system_off);

	/* Check if failed to entering low power mode */
	if (ret) {
		return ret;
	}

	/* DCache and ICache are disabled automatically in low power mode, enable them here */
	sys_cache_instr_enable();
	sys_cache_data_enable();

#if defined(CONFIG_ARM_MPU)
	z_arm_restore_mpu_context(&backup_data.mpu_context);
#endif
	systick_restore(&backup_data.systick_ctx);
	nvic_restore(&backup_data.nvic_ctx);
#if defined(CONFIG_FPU) && !defined(CONFIG_FPU_SHARING)
	z_arm_restore_fp_context(&backup_data.fpu_ctx);
#endif
	z_arm_restore_scb_context(&backup_data.scb_ctx);

	return 0;
}
