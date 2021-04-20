/*
 * Copyright (c) 2021 Andrés Manelli <am@toroid.io>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief System module to support early STM32 MCU configuration
 */

#include <device.h>
#include <init.h>
#include <soc.h>
#include <arch/cpu.h>

/**
 * @brief Perform SoC configuration at boot.
 *
 * This should be run early during the boot process but after basic hardware
 * initialization is done.
 *
 * @return 0
 */
static int st_stm32_common_config(const struct device *dev)
{
#ifdef CONFIG_LOG_BACKEND_SWO
	/* TRACE pin assignment for asynchronous mode */
	DBGMCU->CR &= ~DBGMCU_CR_TRACE_MODE_Msk;
	/* Enable the SWO pin */
	DBGMCU->CR |= DBGMCU_CR_TRACE_IOEN;
#endif

	return 0;
}

SYS_INIT(st_stm32_common_config, PRE_KERNEL_1, 1);
