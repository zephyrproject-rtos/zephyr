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

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/arch/arm/aarch32/cortex_m/cmsis.h>
#include <hal/nrf_power.h>
#include <soc/nrfx_coredep.h>
#include <zephyr/logging/log.h>

#ifdef CONFIG_RUNTIME_NMI
extern void z_arm_nmi_init(void);
#define NMI_INIT() z_arm_nmi_init()
#else
#define NMI_INIT()
#endif

#if defined(CONFIG_SOC_NRF52805)
#include <system_nrf52805.h>
#elif defined(CONFIG_SOC_NRF52810)
#include <system_nrf52810.h>
#elif defined(CONFIG_SOC_NRF52811)
#include <system_nrf52811.h>
#elif defined(CONFIG_SOC_NRF52820)
#include <system_nrf52820.h>
#elif defined(CONFIG_SOC_NRF52832)
#include <system_nrf52.h>
#elif defined(CONFIG_SOC_NRF52833)
#include <system_nrf52833.h>
#elif defined(CONFIG_SOC_NRF52840)
#include <system_nrf52840.h>
#else
#error "Unknown SoC."
#endif

#define LOG_LEVEL CONFIG_SOC_LOG_LEVEL
LOG_MODULE_REGISTER(soc);

/*
 * GPREGRET bits [0:4] are use for retain the reset type.
 */
#define SOC_MCUBOOT_RESET_TYPE_Pos (0UL)
#define SOC_MCUBOOT_RESET_TYPE_Msk (0x1FUL << SOC_MCUBOOT_RESET_TYPE_Pos)

static void soc_retention_reg_mask_set(uint8_t value, uint8_t mask, uint8_t pos)
{
	uint8_t x = nrf_power_gpregret_get(NRF_POWER);

	x &= ~mask;
	x |= (value << pos) & mask;

	nrf_power_gpregret_set(NRF_POWER, x);
}

uint8_t soc_retention_reg_mask_get(uint8_t mask, uint8_t pos)
{
	uint8_t x = nrf_power_gpregret_get(NRF_POWER);

	nrf_power_gpregret_set(NRF_POWER, x & ~mask);

	return (x & mask) >> pos;
}

/* Overrides the weak ARM implementation:
   Set general purpose retention register and reboot */
void sys_arch_reboot(int type)
{
	soc_retention_reg_mask_set(type, SOC_MCUBOOT_RESET_TYPE_Msk,
				   SOC_MCUBOOT_RESET_TYPE_Pos);
	NVIC_SystemReset();
}

static int nordicsemi_nrf52_init(const struct device *arg)
{
	uint32_t key;

	ARG_UNUSED(arg);

	key = irq_lock();

#ifdef CONFIG_NRF_ENABLE_ICACHE
	/* Enable the instruction cache */
	NRF_NVMC->ICACHECNF = NVMC_ICACHECNF_CACHEEN_Msk;
#endif

#if defined(CONFIG_SOC_DCDC_NRF52X)
	nrf_power_dcdcen_set(NRF_POWER, true);
#endif
#if NRF_POWER_HAS_DCDCEN_VDDH && defined(CONFIG_SOC_DCDC_NRF52X_HV)
	nrf_power_dcdcen_vddh_set(NRF_POWER, true);
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

SYS_INIT(nordicsemi_nrf52_init, PRE_KERNEL_1, 0);
