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
#include "power.h"

#include <cmsis_core.h>

#define NVIC_MEMBER_SIZE(member) ARRAY_SIZE(((NVIC_Type *)0)->member)

/* Coprocessor Power Control Register Definitions */
#define SCnSCB_CPPWR_SU11_Pos 22U                            /*!< CPPWR: SU11 Position */
#define SCnSCB_CPPWR_SU11_Msk (1UL << SCnSCB_CPPWR_SU11_Pos) /*!< CPPWR: SU11 Mask */

#define SCnSCB_CPPWR_SU10_Pos 20U                            /*!< CPPWR: SU10 Position */
#define SCnSCB_CPPWR_SU10_Msk (1UL << SCnSCB_CPPWR_SU10_Pos) /*!< CPPWR: SU10 Mask */

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

typedef struct {
	uint32_t ICSR;
	uint32_t VTOR;
	uint32_t AIRCR;
	uint32_t SCR;
	uint32_t CCR;
	uint32_t SHPR[12U];
	uint32_t SHCSR;
	uint32_t CFSR;
	uint32_t HFSR;
	uint32_t DFSR;
	uint32_t MMFAR;
	uint32_t BFAR;
	uint32_t AFSR;
	uint32_t CPACR;
} _scb_context_t;

#if defined(CONFIG_FPU) && !defined(CONFIG_FPU_SHARING)
typedef struct {
	uint32_t FPCCR;
	uint32_t FPCAR;
	uint32_t FPDSCR;
	uint32_t S[32];
} _fpu_context_t;
#endif

struct backup {
	_nvic_context_t nvic_context;
	_mpu_context_t mpu_context;
	_scb_context_t scb_context;
#if defined(CONFIG_FPU) && !defined(CONFIG_FPU_SHARING)
	_fpu_context_t fpu_context;
#endif
};

static __noinit struct backup backup_data;

extern void z_arm_configure_static_mpu_regions(void);
extern int z_arm_mpu_init(void);

/* MPU registers cannot be simply copied because content of RBARx RLARx registers
 * depends on region which is selected by RNR register.
 */
static void mpu_save(_mpu_context_t *backup)
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

static void mpu_restore(_mpu_context_t *backup)
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

static void scb_save(_scb_context_t *backup)
{
	backup->ICSR = SCB->ICSR;
	backup->VTOR = SCB->VTOR;
	backup->AIRCR = SCB->AIRCR;
	backup->SCR = SCB->SCR;
	backup->CCR = SCB->CCR;
	memcpy(backup->SHPR, (uint32_t *)SCB->SHPR, sizeof(SCB->SHPR));
	backup->SHCSR = SCB->SHCSR;
	backup->CFSR = SCB->CFSR;
	backup->HFSR = SCB->HFSR;
	backup->DFSR = SCB->DFSR;
	backup->MMFAR = SCB->MMFAR;
	backup->BFAR = SCB->BFAR;
	backup->AFSR = SCB->AFSR;
	backup->CPACR = SCB->CPACR;
}

static void scb_restore(_scb_context_t *backup)
{
	SCB->ICSR = backup->ICSR;
	SCB->VTOR = backup->VTOR;
	SCB->AIRCR = backup->AIRCR;
	SCB->SCR = backup->SCR;
	SCB->CCR = backup->CCR;
	memcpy((uint32_t *)SCB->SHPR, backup->SHPR, sizeof(SCB->SHPR));
	SCB->SHCSR = backup->SHCSR;
	SCB->CFSR = backup->CFSR;
	SCB->HFSR = backup->HFSR;
	SCB->DFSR = backup->DFSR;
	SCB->MMFAR = backup->MMFAR;
	SCB->BFAR = backup->BFAR;
	SCB->AFSR = backup->AFSR;
	SCB->CPACR = backup->CPACR;
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

#if !defined(CONFIG_FPU_SHARING)
static void fpu_save(_fpu_context_t *backup)
{
	backup->FPCCR = FPU->FPCCR;
	backup->FPCAR = FPU->FPCAR;
	backup->FPDSCR = FPU->FPDSCR;

	__asm__ volatile("vstmia %0, {s0-s31}\n" : : "r"(backup->S) : "memory");
}

static void fpu_restore(_fpu_context_t *backup)
{
	FPU->FPCCR = backup->FPCCR;
	FPU->FPCAR = backup->FPCAR;
	FPU->FPDSCR = backup->FPDSCR;

	__asm__ volatile("vldmia %0, {s0-s31}\n" : : "r"(backup->S) : "memory");
}
#endif /* !defined(CONFIG_FPU_SHARING) */
#endif /* defined(CONFIG_FPU) */

int soc_s2ram_suspend(pm_s2ram_system_off_fn_t system_off)
{
	int ret;

	scb_save(&backup_data.scb_context);
#if defined(CONFIG_FPU)
#if !defined(CONFIG_FPU_SHARING)
	fpu_save(&backup_data.fpu_context);
#endif
	fpu_power_down();
#endif
	nvic_save(&backup_data.nvic_context);
	mpu_save(&backup_data.mpu_context);
	ret = arch_pm_s2ram_suspend(system_off);
	/* Cache and FPU are powered down so power up is needed even if s2ram failed. */
	nrf_power_up_cache();
#if defined(CONFIG_FPU)
	fpu_power_up();
#if !defined(CONFIG_FPU_SHARING)
	/* Also the FPU content might be lost. */
	fpu_restore(&backup_data.fpu_context);
#endif
#endif
	if (ret < 0) {
		return ret;
	}

	mpu_restore(&backup_data.mpu_context);
	nvic_restore(&backup_data.nvic_context);
	scb_restore(&backup_data.scb_context);

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
