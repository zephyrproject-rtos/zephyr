/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for Nordic Semiconductor nRF53 family processor
 *
 * This module provides routines to initialize and support board-level hardware
 * for the Nordic Semiconductor nRF53 family processor.
 */

#include <kernel.h>
#include <init.h>
#include <arch/arm/aarch32/cortex_m/cmsis.h>
#include <soc/nrfx_coredep.h>
#include <logging/log.h>

#ifdef CONFIG_RUNTIME_NMI
extern void z_arm_nmi_init(void);
#define NMI_INIT() z_arm_nmi_init()
#else
#define NMI_INIT()
#endif

#if defined(CONFIG_SOC_NRF5340_CPUAPP)
#include <system_nrf5340_application.h>
#elif defined(CONFIG_SOC_NRF5340_CPUNET)
#include <system_nrf5340_network.h>
#else
#error "Unknown nRF53 SoC."
#endif

#define LOG_LEVEL CONFIG_SOC_LOG_LEVEL
LOG_MODULE_REGISTER(soc);

static int nordicsemi_nrf53_init(const struct device *arg)
{
	uint32_t key;

	ARG_UNUSED(arg);

	key = irq_lock();

#ifdef CONFIG_NRF_ENABLE_CACHE
#ifdef CONFIG_SOC_NRF5340_CPUAPP
	/* Enable the instruction & data cache */
	NRF_CACHE->ENABLE = CACHE_ENABLE_ENABLE_Msk;
#endif /* CONFIG_SOC_NRF5340_CPUAPP */
#ifdef CONFIG_SOC_NRF5340_CPUNET
	NRF_NVMC->ICACHECNF |= NVMC_ICACHECNF_CACHEEN_Enabled;
#endif /* CONFIG_SOC_NRF5340_CPUNET */
#endif

#if defined(CONFIG_SOC_NRF5340_CPUAPP) && \
	!defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)
	*((uint32_t *)0x500046D0) = 0x1;
#endif

#if defined(CONFIG_SOC_DCDC_NRF53X_APP)
	NRF_REGULATORS->VREGMAIN.DCDCEN = 1;
#endif
#if defined(CONFIG_SOC_DCDC_NRF53X_NET)
	NRF_REGULATORS->VREGRADIO.DCDCEN = 1;
#endif
#if defined(CONFIG_SOC_DCDC_NRF53X_HV)
	NRF_REGULATORS->VREGH.DCDCEN = 1;
#endif

	/* Install default handler that simply resets the CPU
	 * if configured in the kernel, NOP otherwise
	 */
	NMI_INIT();

	irq_unlock(key);

	return 0;
}

void arch_busy_wait(uint32_t time_us)
{
	nrfx_coredep_delay_us(time_us);
}

void z_platform_init(void)
{
	SystemInit();
}

#if defined(CONFIG_SOC_NRF5340_CPUAPP) && \
	!defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)
bool nrf53_has_erratum19(void)
{
	if (NRF_FICR->INFO.PART == 0x5340) {
		if (NRF_FICR->INFO.VARIANT == 0x41414142) {
			return true;
		}
	}
	return false;
}

#ifndef CONFIG_NRF5340_CPUAPP_ERRATUM19
static int check_erratum19(const struct device *arg)
{
	ARG_UNUSED(arg);
	if (nrf53_has_erratum19()) {
		LOG_ERR("This device is affected by nRF53 Erratum 19,");
		LOG_ERR("but workarounds have not been enabled.");
		LOG_ERR("See CONFIG_NRF5340_CPUAPP_ERRATUM19.");
		k_panic();
	}

	return 0;
}

SYS_INIT(check_erratum19, POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY);
#endif
#endif

SYS_INIT(nordicsemi_nrf53_init, PRE_KERNEL_1, 0);
