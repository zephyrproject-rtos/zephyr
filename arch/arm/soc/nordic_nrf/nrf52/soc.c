/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for Nordic Semiconductor nRF52 family processor
 *
 * This module provides routines to initialize and support board-level hardware
 * for the Nordic Semiconductor nRF52 family processor.
 */

#include <kernel.h>
#include <init.h>
#include <cortex_m/exc.h>

#ifdef CONFIG_RUNTIME_NMI
extern void _NmiInit(void);
#define NMI_INIT() _NmiInit()
#else
#define NMI_INIT()
#endif

#if defined(CONFIG_SOC_NRF52810)
#include <system_nrf52810.h>
#elif defined(CONFIG_SOC_NRF52832)
#include <system_nrf52.h>
#elif defined(CONFIG_SOC_NRF52840)
#include <system_nrf52840.h>
#else
#error "Unknown SoC."
#endif

#include <nrf.h>
#include <hal/nrf_power.h>

static int nordicsemi_nrf52_init(struct device *arg)
{
	u32_t key;

	ARG_UNUSED(arg);

	key = irq_lock();

	SystemInit();

#ifdef CONFIG_NRF_ENABLE_ICACHE
	/* Enable the instruction cache */
	NRF_NVMC->ICACHECNF = NVMC_ICACHECNF_CACHEEN_Msk;
#endif

#if defined(CONFIG_SOC_DCDC_NRF52X)
	nrf_power_dcdcen_set(true);
#endif

	_ClearFaults();

	/* Install default handler that simply resets the CPU
	* if configured in the kernel, NOP otherwise
	*/
	NMI_INIT();

	irq_unlock(key);

	return 0;
}

SYS_INIT(nordicsemi_nrf52_init, PRE_KERNEL_1, 0);
