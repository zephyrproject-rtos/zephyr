/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for Nordic Semiconductor nRF54L family processor
 *
 * This module provides routines to initialize and support board-level hardware
 * for the Nordic Semiconductor nRF54L family processor.
 */

/* Include autoconf for cases when this file is used in special build (e.g. TFM) */
#include <zephyr/autoconf.h>

#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/cache.h>
#ifndef __ZEPHYR__
#include <hal/nrf_cache.h>
#endif
#include <lib/nrfx_coredep.h>
#include <soc.h>
LOG_MODULE_REGISTER(soc, CONFIG_SOC_LOG_LEVEL);

#if (defined(NRF_APPLICATION) && !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)) || \
	!defined(__ZEPHYR__)

#include <hal/nrf_oscillators.h>
#include <hal/nrf_power.h>
#include <hal/nrf_regulators.h>
#include <hal/nrf_ficr.h>
#include <zephyr/dt-bindings/regulator/nrf5x.h>

#define LFXO_NODE DT_NODELABEL(lfxo)
#define HFXO_NODE DT_NODELABEL(hfxo)

#define LFXO_CAP_MIN_FF 3000UL  /* min load capacitance in femtofarads */
#define LFXO_CAP_MAX_FF 18000UL /* max load capacitance in femtofarads */

#if DT_PROP(LFXO_NODE, external_clock_source)
BUILD_ASSERT(DT_ENUM_HAS_VALUE(LFXO_NODE, load_capacitors, internal) == 0,
	     "Internal load capacitors must be disconnected when using "
	     "external clock source");
#endif

static inline void power_and_clock_configuration(void)
{
/* NRF_REGULATORS and NRF_OSCILLATORS are configured to be secure
 * as NRF_REGULATORS.POFCON is needed by the secure image to
 * prevent glitches when the power supply is attacked.
 *
 * NRF_OSCILLATORS is also configured as secure because of a HW limitation
 * that requires them to be configured with the same security property.
 */
#if DT_ENUM_HAS_VALUE(LFXO_NODE, load_capacitors, internal)
	BUILD_ASSERT(DT_PROP(LFXO_NODE, load_capacitance_femtofarad) >= LFXO_CAP_MIN_FF &&
		     DT_PROP(LFXO_NODE, load_capacitance_femtofarad) <= LFXO_CAP_MAX_FF,
		     "LFXO load capacitance must be between "
		     STRINGIFY(LFXO_CAP_MIN_FF) " and " STRINGIFY(LFXO_CAP_MAX_FF)
		     " femtofarads");

	nrf_oscillators_lfxo_cap_set(NRF_OSCILLATORS,
				nrf_ficr_lfxo_cap_calculate(NRF_FICR,
					DT_PROP(LFXO_NODE, load_capacitance_femtofarad)));
#else
	nrf_oscillators_lfxo_cap_set(NRF_OSCILLATORS, (nrf_oscillators_lfxo_cap_t)0);
#endif

#if DT_ENUM_HAS_VALUE(HFXO_NODE, load_capacitors, internal)
	nrf_oscillators_hfxo_cap_set(NRF_OSCILLATORS, true,
				nrf_ficr_hfxo_cap_calculate(NRF_FICR,
					DT_PROP(HFXO_NODE, load_capacitance_femtofarad)));
#else
	nrf_oscillators_hfxo_cap_set(NRF_OSCILLATORS, false, 0);
#endif

#if DT_PROP(LFXO_NODE, external_clock_source)
	nrf_oscillators_lfxo_bypass_set(NRF_OSCILLATORS, true);
#else
	nrf_oscillators_lfxo_bypass_set(NRF_OSCILLATORS, false);
#endif

#if (DT_PROP(DT_NODELABEL(vregmain), regulator_initial_mode) == NRF5X_REG_MODE_DCDC)
	nrf_regulators_vreg_enable_set(NRF_REGULATORS, NRF_REGULATORS_VREG_MAIN, true);
#endif

}
#endif /* NRF_APPLICATION && !CONFIG_TRUSTED_EXECUTION_NONSECURE */

int nordicsemi_nrf54l_init(void)
{
	/* Update the SystemCoreClock global variable with current core clock
	 * retrieved from the DT.
	 */
	SystemCoreClock = NRF_PERIPH_GET_FREQUENCY(DT_NODELABEL(cpu));

#ifdef __ZEPHYR__
	sys_cache_instr_enable();
#elif defined(NRF_ICACHE)
	nrf_cache_enable(NRF_ICACHE);
#endif

#if (defined(NRF_APPLICATION) && !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)) || \
	!defined(__ZEPHYR__)
	power_and_clock_configuration();
#endif

	return 0;
}

void arch_busy_wait(uint32_t time_us)
{
	nrfx_coredep_delay_us(time_us);
}

/* Init must precede sys_clock_driver_init */
SYS_INIT(nordicsemi_nrf54l_init, EARLY, 0);
