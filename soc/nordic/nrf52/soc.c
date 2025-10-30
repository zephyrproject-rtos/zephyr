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

#include <zephyr/devicetree.h>
#include <zephyr/dt-bindings/regulator/nrf5x.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <hal/nrf_power.h>
#include <lib/nrfx_coredep.h>
#include <zephyr/logging/log.h>
#include <helpers/nrfx_gppi.h>

#include <cmsis_core.h>

#define LOG_LEVEL CONFIG_SOC_LOG_LEVEL
LOG_MODULE_REGISTER(soc);

void gppi_init(void)
{
	static nrfx_gppi_t gppi_instance;

	gppi_instance.ch_mask = BIT_MASK(PPI_CH_NUM) & ~NRFX_PPI_CHANNELS_USED;
	gppi_instance.group_mask = BIT_MASK(PPI_GROUP_NUM) & ~NRFX_PPI_GROUPS_USED;

	nrfx_gppi_init(&gppi_instance);
}

static int nordicsemi_nrf52_init(void)
{
#ifdef CONFIG_NRF_ENABLE_ICACHE
	/* Enable the instruction cache */
	NRF_NVMC->ICACHECNF = NVMC_ICACHECNF_CACHEEN_Msk;
#endif

#if defined(CONFIG_SOC_DCDC_NRF52X) || (DT_PROP(DT_INST(0, nordic_nrf5x_regulator),                \
						regulator_initial_mode) == NRF5X_REG_MODE_DCDC)
	nrf_power_dcdcen_set(NRF_POWER, true);
#endif
#if NRF_POWER_HAS_DCDCEN_VDDH && (defined(CONFIG_SOC_DCDC_NRF52X_HV) || \
	DT_NODE_HAS_STATUS_OKAY(DT_INST(0, nordic_nrf52x_regulator_hv)))
	nrf_power_dcdcen_vddh_set(NRF_POWER, true);
#endif

	if (IS_ENABLED(CONFIG_NRFX_GPPI) && !IS_ENABLED(CONFIG_NRFX_GPPI_V1)) {
		gppi_init();
	}

	return 0;
}

void arch_busy_wait(uint32_t time_us)
{
	nrfx_coredep_delay_us(time_us);
}

SYS_INIT(nordicsemi_nrf52_init, PRE_KERNEL_1, 0);
