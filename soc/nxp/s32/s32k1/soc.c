/*
 * Copyright 2023 NXP
 *
 * Based on zephyr/soc/soc_legacy/arm/nxp_kinetis/ke1xf/soc.c, which is:
 * Copyright (c) 2019-2021 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/sys/barrier.h>

#include <cmsis_core.h>
#include <OsIf.h>

#if defined(CONFIG_HAS_MCUX_CACHE)
#include <fsl_cache.h>
#endif

#if defined(CONFIG_WDOG_INIT)
void z_arm_watchdog_init(void)
{
	/*
	 * NOTE: DO NOT SINGLE STEP THROUGH THIS SECTION!!! Watchdog
	 * reconfiguration must take place within 128 bus clocks from
	 * unlocking. Single stepping through the code will cause the
	 * watchdog to close the unlock window again.
	 */
	if ((IP_WDOG->CS & WDOG_CS_CMD32EN_MASK) != 0U) {
		IP_WDOG->CNT = WDOG_UPDATE_KEY;
	} else {
		IP_WDOG->CNT = WDOG_UPDATE_KEY & 0xFFFFU;
		IP_WDOG->CNT = (WDOG_UPDATE_KEY >> 16U) & 0xFFFFU;
	}
	while (!(IP_WDOG->CS & WDOG_CS_ULK_MASK)) {
		;
	}

	IP_WDOG->TOVAL = 0xFFFFU;
	IP_WDOG->CS = (uint32_t) ((IP_WDOG->CS) & ~WDOG_CS_EN_MASK) | WDOG_CS_UPDATE_MASK;

	/* Wait for new configuration to take effect */
	while (!(IP_WDOG->CS & WDOG_CS_RCS_MASK)) {
		;
	}
}
#endif /* CONFIG_WDOG_INIT */

static int soc_init(void)
{
#if !defined(CONFIG_ARM_MPU)
	uint32_t tmp;

	/*
	 * Disable memory protection and clear slave port errors.
	 * Note that the S32K1xx does not implement the optional Arm MPU but
	 * instead the Soc includes its own NXP MPU module.
	 */
	tmp = IP_MPU->CESR;
	tmp &= ~MPU_CESR_VLD_MASK;
	tmp |= MPU_CESR_SPERR0_MASK | MPU_CESR_SPERR1_MASK
		| MPU_CESR_SPERR2_MASK | MPU_CESR_SPERR3_MASK;
	IP_MPU->CESR = tmp;
#endif /* !CONFIG_ARM_MPU */

#if defined(CONFIG_HAS_MCUX_CACHE) && defined(CONFIG_NXP_S32_ENABLE_CODE_CACHE)
	L1CACHE_EnableCodeCache();
	barrier_isync_fence_full();
#endif

	OsIf_Init(NULL);

	return 0;
}

SYS_INIT(soc_init, PRE_KERNEL_1, 0);
