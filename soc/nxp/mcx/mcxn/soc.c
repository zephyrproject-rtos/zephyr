/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for nxp_mcxn94x platform
 *
 * This module provides routines to initialize and support board-level
 * hardware for the nxp_mcxn94x platform.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>

#ifdef CONFIG_SOC_RESET_HOOK

void soc_reset_hook(void)
{
	SystemInit();
}

#endif

#define FLEXCOMM_CHECK_2(n)	\
	BUILD_ASSERT((DT_NODE_HAS_COMPAT(n, nxp_lpuart) == 0) &&		\
		     (DT_NODE_HAS_COMPAT(n, nxp_lpi2c) == 0),			\
		     "Do not enable SPI and UART/I2C on the same Flexcomm node");

/* For SPI node enabled, check if UART or I2C is also enabled on the same parent Flexcomm node */
#define FLEXCOMM_CHECK(n) DT_FOREACH_CHILD_STATUS_OKAY(DT_PARENT(n), FLEXCOMM_CHECK_2)

/* SPI cannot be exist with UART or I2C on the same FlexComm Interface
 * Throw a build error if user is enabling SPI and UART/I2C on a Flexcomm node.
 */
DT_FOREACH_STATUS_OKAY(nxp_lpspi, FLEXCOMM_CHECK)

#if defined(CONFIG_SECOND_CORE_MCUX) && defined(CONFIG_SOC_MCXN947_CPU0)

/* This function is also called at deep sleep resume. */
static int second_core_boot(void)
{
	/* Boot source for Core 1 from flash */
	SYSCON->CPBOOT = ((uint32_t)(char *)DT_REG_ADDR(DT_CHOSEN(zephyr_code_cpu1_partition)) &
			  SYSCON_CPBOOT_CPBOOT_MASK);

	uint32_t temp = SYSCON->CPUCTRL;

	temp |= 0xc0c40000U;
	SYSCON->CPUCTRL = temp | SYSCON_CPUCTRL_CPU1RSTEN_MASK | SYSCON_CPUCTRL_CPU1CLKEN_MASK;
	SYSCON->CPUCTRL = (temp | SYSCON_CPUCTRL_CPU1CLKEN_MASK) & (~SYSCON_CPUCTRL_CPU1RSTEN_MASK);

	return 0;
}

SYS_INIT(second_core_boot, PRE_KERNEL_2, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#endif
