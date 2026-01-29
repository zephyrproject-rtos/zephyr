/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/arch/arm/mpu/arm_mpu.h>
#include <zephyr/arch/arm/cortex_m/scb.h>
#include <zephyr/arch/arm/cortex_m/fpu.h>
#include <zephyr/arch/common/pm_s2ram.h>
#include <zephyr/linker/sections.h>
#include <zephyr/sys/util.h>
#include <hal/nrf_resetinfo.h>
#include "pm_s2ram.h"
#include "power.h"

#include <cmsis_core.h>

#define NVIC_MEMBER_SIZE(member) ARRAY_SIZE(((NVIC_Type *)0)->member)

/* Coprocessor Power Control Register Definitions */
#define SCnSCB_CPPWR_SU11_Pos 22U                            /*!< CPPWR: SU11 Position */
#define SCnSCB_CPPWR_SU11_Msk (1UL << SCnSCB_CPPWR_SU11_Pos) /*!< CPPWR: SU11 Mask */

#define SCnSCB_CPPWR_SU10_Pos 20U                            /*!< CPPWR: SU10 Position */
#define SCnSCB_CPPWR_SU10_Msk (1UL << SCnSCB_CPPWR_SU10_Pos) /*!< CPPWR: SU10 Mask */

typedef struct {
	/* NVIC components stored into RAM. */
	uint32_t ISER[NVIC_MEMBER_SIZE(ISER)];
	uint32_t ISPR[NVIC_MEMBER_SIZE(ISPR)];
	uint8_t IPR[NVIC_MEMBER_SIZE(IPR)];
} _nvic_context_t;

struct backup {
	_nvic_context_t nvic_context;
#if defined(CONFIG_ARM_MPU)
	struct z_mpu_context_retained mpu_context;
#endif
	struct scb_context scb_context;
#if defined(CONFIG_FPU) && !defined(CONFIG_FPU_SHARING)
	struct fpu_ctx_full fpu_context;
#endif
};

static __noinit struct backup backup_data;

static void nvic_save(_nvic_context_t *backup)
{
	memcpy(backup->ISER, (uint32_t *)NVIC->ISER, sizeof(NVIC->ISER));
	memcpy(backup->ISPR, (uint32_t *)NVIC->ISPR, sizeof(NVIC->ISPR));
	memcpy(backup->IPR, (uint32_t *)NVIC->IPR, sizeof(NVIC->IPR));
}

static void nvic_restore(_nvic_context_t *backup)
{
	memcpy((uint32_t *)NVIC->ISER, backup->ISER, sizeof(NVIC->ISER));
	memcpy((uint32_t *)NVIC->ISPR, backup->ISPR, sizeof(NVIC->ISPR));
	memcpy((uint32_t *)NVIC->IPR, backup->IPR, sizeof(NVIC->IPR));
}

#if defined(CONFIG_FPU)
static void fpu_power_down(void)
{
	SCB->CPACR &= (~(CPACR_CP10_Msk | CPACR_CP11_Msk));
	SCnSCB->CPPWR |= (SCnSCB_CPPWR_SU11_Msk | SCnSCB_CPPWR_SU10_Msk);
	__DSB();
	__ISB();
}

static void fpu_power_up(void)
{
	SCnSCB->CPPWR &= (~(SCnSCB_CPPWR_SU11_Msk | SCnSCB_CPPWR_SU10_Msk));
	SCB->CPACR |= (CPACR_CP10_Msk | CPACR_CP11_Msk);
	__DSB();
	__ISB();
}
#endif /* defined(CONFIG_FPU) */

int soc_s2ram_suspend(pm_s2ram_system_off_fn_t system_off)
{
	int ret;

	z_arm_save_scb_context(&backup_data.scb_context);
#if defined(CONFIG_FPU)
#if !defined(CONFIG_FPU_SHARING)
	z_arm_save_fp_context(&backup_data.fpu_context);
#endif
	fpu_power_down();
#endif
	nvic_save(&backup_data.nvic_context);
#if defined(CONFIG_ARM_MPU)
	z_arm_save_mpu_context(&backup_data.mpu_context);
#endif
	ret = arch_pm_s2ram_suspend(system_off);
	/* Cache and FPU are powered down so power up is needed even if s2ram failed. */
	nrf_power_up_cache();
#if defined(CONFIG_FPU)
	fpu_power_up();
#if !defined(CONFIG_FPU_SHARING)
	/* Also the FPU content might be lost. */
	z_arm_restore_fp_context(&backup_data.fpu_context);
#endif
#endif
	if (ret < 0) {
		return ret;
	}

#if defined(CONFIG_ARM_MPU)
	z_arm_restore_mpu_context(&backup_data.mpu_context);
#endif
	nvic_restore(&backup_data.nvic_context);
	z_arm_restore_scb_context(&backup_data.scb_context);

	return ret;
}

void pm_s2ram_mark_set(void)
{
	/* empty */
}

bool pm_s2ram_mark_check_and_clear(void)
{
	bool restore_valid;
	uint32_t reset_reason = nrf_resetinfo_resetreas_local_get(NRF_RESETINFO);

	if (reset_reason != NRF_RESETINFO_RESETREAS_LOCAL_UNRETAINED_MASK) {
		return false;
	}
	nrf_resetinfo_resetreas_local_set(NRF_RESETINFO, 0);

	restore_valid = nrf_resetinfo_restore_valid_check(NRF_RESETINFO);
	nrf_resetinfo_restore_valid_set(NRF_RESETINFO, false);

	return restore_valid;
}
