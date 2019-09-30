/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for Nordic Semiconductor nRF91 family processor
 *
 * This module provides routines to initialize and support board-level hardware
 * for the Nordic Semiconductor nRF91 family processor.
 */

#include <kernel.h>
#include <init.h>
#include <cortex_m/exc.h>
#include <soc/nrfx_coredep.h>
#include <logging/log.h>

#ifdef CONFIG_RUNTIME_NMI
extern void z_arm_nmi_init(void);
#define NMI_INIT() z_arm_nmi_init()
#else
#define NMI_INIT()
#endif

#if defined(CONFIG_SOC_NRF9160)
#include <system_nrf9160.h>
#else
#error "Unknown SoC."
#endif

#define LOG_LEVEL CONFIG_SOC_LOG_LEVEL
LOG_MODULE_REGISTER(soc);

static int nordicsemi_nrf91_init(struct device *arg)
{
	u32_t key;

	ARG_UNUSED(arg);

	key = irq_lock();

#ifdef CONFIG_NRF_ENABLE_ICACHE
	/* Enable the instruction cache */
	NRF_NVMC->ICACHECNF = NVMC_ICACHECNF_CACHEEN_Msk;
#endif

	/* Install default handler that simply resets the CPU
	* if configured in the kernel, NOP otherwise
	*/
	NMI_INIT();

	irq_unlock(key);

	return 0;
}

void z_arch_busy_wait(u32_t time_us)
{
	nrfx_coredep_delay_us(time_us);
}

void z_platform_init(void)
{
	SystemInit();
}


SYS_INIT(nordicsemi_nrf91_init, PRE_KERNEL_1, 0);
