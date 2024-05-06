/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/arch/common/pm_s2ram.h>
#include <zephyr/linker/sections.h>
#include <zephyr/sys/util.h>
#include <hal/nrf_resetinfo.h>
#include "pm_s2ram.h"

#include <cmsis_core.h>

#define NVIC_MEMBER_SIZE(member) ARRAY_SIZE(((NVIC_Type *)0)->member)

/* Currently dynamic regions are only used in case of userspace or stack guard and
 * stack guard is not used by default on Cortex-M33 because there is a dedicated
 * mechanism for stack overflow detection. Unless those condition change we don't
 * need to store MPU content, it can just be reinitialized on resuming.
 */
#define MPU_USE_DYNAMIC_REGIONS IS_ENABLED(CONFIG_USERSPACE) || IS_ENABLED(CONFIG_MPU_STACK_GUARD)

/* TODO: The num-mpu-regions property should be used. Needs to be added to dts bindings. */
#define MPU_MAX_NUM_REGIONS 16

typedef struct {
	/* NVIC components stored into RAM. */
	uint32_t ISER[NVIC_MEMBER_SIZE(ISER)];
	uint32_t ISPR[NVIC_MEMBER_SIZE(ISPR)];
	uint8_t IPR[NVIC_MEMBER_SIZE(IPR)];
} _nvic_context_t;

typedef struct {
	uint32_t RNR;
	uint32_t RBAR[MPU_MAX_NUM_REGIONS];
	uint32_t RLAR[MPU_MAX_NUM_REGIONS];
	uint32_t MAIR0;
	uint32_t MAIR1;
	uint32_t CTRL;
} _mpu_context_t;

struct backup {
	_nvic_context_t nvic_context;
	_mpu_context_t mpu_context;
};

static __noinit struct backup backup_data;

extern void z_arm_configure_static_mpu_regions(void);
extern int z_arm_mpu_init(void);

/* MPU registers cannot be simply copied because content of RBARx RLARx registers
 * depends on region which is selected by RNR register.
 */
static void mpu_suspend(_mpu_context_t *backup)
{
	if (!MPU_USE_DYNAMIC_REGIONS) {
		return;
	}

	backup->RNR = MPU->RNR;

	for (uint8_t i = 0; i < MPU_MAX_NUM_REGIONS; i++) {
		MPU->RNR = i;
		backup->RBAR[i] = MPU->RBAR;
		backup->RLAR[i] = MPU->RLAR;
	}
	backup->MAIR0 = MPU->MAIR0;
	backup->MAIR1 = MPU->MAIR1;
	backup->CTRL = MPU->CTRL;
}

static void mpu_resume(_mpu_context_t *backup)
{
	if (!MPU_USE_DYNAMIC_REGIONS) {
		z_arm_mpu_init();
		z_arm_configure_static_mpu_regions();
		return;
	}

	uint32_t rnr = backup->RNR;

	for (uint8_t i = 0; i < MPU_MAX_NUM_REGIONS; i++) {
		MPU->RNR = i;
		MPU->RBAR = backup->RBAR[i];
		MPU->RLAR = backup->RLAR[i];
	}

	MPU->MAIR0 = backup->MAIR0;
	MPU->MAIR1 = backup->MAIR1;
	MPU->RNR = rnr;
	MPU->CTRL = backup->CTRL;
}

static void nvic_suspend(_nvic_context_t *backup)
{
	memcpy(backup->ISER, (uint32_t *)NVIC->ISER, sizeof(NVIC->ISER));
	memcpy(backup->ISPR, (uint32_t *)NVIC->ISPR, sizeof(NVIC->ISPR));
	memcpy(backup->IPR, (uint32_t *)NVIC->IPR, sizeof(NVIC->IPR));
}

static void nvic_resume(_nvic_context_t *backup)
{
	memcpy((uint32_t *)NVIC->ISER, backup->ISER, sizeof(NVIC->ISER));
	memcpy((uint32_t *)NVIC->ISPR, backup->ISPR, sizeof(NVIC->ISPR));
	memcpy((uint32_t *)NVIC->IPR, backup->IPR, sizeof(NVIC->IPR));
}

int soc_s2ram_suspend(pm_s2ram_system_off_fn_t system_off)
{
	int ret;

	__disable_irq();
	nvic_suspend(&backup_data.nvic_context);
	mpu_suspend(&backup_data.mpu_context);
	ret = arch_pm_s2ram_suspend(system_off);
	if (ret < 0) {
		__enable_irq();
		return ret;
	}

	mpu_resume(&backup_data.mpu_context);
	nvic_resume(&backup_data.nvic_context);
	__enable_irq();

	return ret;
}

void pm_s2ram_mark_set(void)
{
	/* empty */
}

bool pm_s2ram_mark_check_and_clear(void)
{
	bool unretained_wake;
	bool restore_valid;
	uint32_t reset_reason = nrf_resetinfo_resetreas_local_get(NRF_RESETINFO);

	if (reset_reason != NRF_RESETINFO_RESETREAS_LOCAL_UNRETAINED_MASK) {
		return false;
	}
	unretained_wake = reset_reason & NRF_RESETINFO_RESETREAS_LOCAL_UNRETAINED_MASK;
	nrf_resetinfo_resetreas_local_set(NRF_RESETINFO, 0);

	restore_valid = nrf_resetinfo_restore_valid_check(NRF_RESETINFO);
	nrf_resetinfo_restore_valid_set(NRF_RESETINFO, false);

	return (unretained_wake & restore_valid) ? true : false;
}
