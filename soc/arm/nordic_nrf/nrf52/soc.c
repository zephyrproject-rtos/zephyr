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
#include <nrfx.h>
#include <soc/nrfx_coredep.h>

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

/**
 * @brief REG0 voltage definitions for nRF52840
 */
#if defined(CONFIG_NRF52840_REG0_1V8)
#define NRF52840_REG0_VOUT	UICR_REGOUT0_VOUT_1V8
#elif defined(CONFIG_NRF52840_REG0_2V1)
#define NRF52840_REG0_VOUT	UICR_REGOUT0_VOUT_2V1
#elif defined(CONFIG_NRF52840_REG0_2V4)
#define NRF52840_REG0_VOUT	UICR_REGOUT0_VOUT_2V4
#elif defined(CONFIG_NRF52840_REG0_2V7)
#define NRF52840_REG0_VOUT	UICR_REGOUT0_VOUT_2V7
#elif defined(CONFIG_NRF52840_REG0_3V0)
#define NRF52840_REG0_VOUT	UICR_REGOUT0_VOUT_3V0
#elif defined(CONFIG_NRF52840_REG0_3V3)
#define NRF52840_REG0_VOUT	UICR_REGOUT0_VOUT_3V3
#endif

#define LOG_LEVEL CONFIG_SOC_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(soc);

static void nordicsemi_nrf52840_reg0_voltage_set(void);

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

#if defined(NRF_POWER_REG0_VOLT)
	nordicsemi_nrf52840_reg0_voltage_set();
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

void z_arch_busy_wait(u32_t time_us)
{
	nrfx_coredep_delay_us(time_us);
}

/**
 * @brief Sets REG0 output voltage for nRF52840
 *
 * @note  REG0 output voltage will only be set for reseted boards
 * 		  as function checks if current voltage is DEFAULT
 * 		  (factory default)
 *
 */
static void nordicsemi_nrf52840_reg0_voltage_set(void)
{
	if ((nrf_power_mainregstatus_get() == NRF_POWER_MAINREGSTATUS_HIGH) &&
		((NRF_UICR->REGOUT0 & UICR_REGOUT0_VOUT_Msk) ==
		 (UICR_REGOUT0_VOUT_DEFAULT << UICR_REGOUT0_VOUT_Pos))) {

		NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos;
		while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {
			;
		}

		NRF_UICR->REGOUT0 =
				(NRF_UICR->REGOUT0 & ~((uint32_t)UICR_REGOUT0_VOUT_Msk)) |
				(NRF52840_REG0_VOUT << UICR_REGOUT0_VOUT_Pos);

		NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;
		while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {
			;
		}

		/* a reset is required for changes to take effect */
		NVIC_SystemReset();
	}
}

SYS_INIT(nordicsemi_nrf52_init, PRE_KERNEL_1, 0);
