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

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/cache.h>

#if defined(NRF_APPLICATION)
#include <cmsis_core.h>
#include <hal/nrf_glitchdet.h>
#include <hal/nrf_oscillators.h>
#include <hal/nrf_power.h>
#include <hal/nrf_regulators.h>
#endif
#include <soc/nrfx_coredep.h>

#include <system_nrf54l.h>

LOG_MODULE_REGISTER(soc, CONFIG_SOC_LOG_LEVEL);

#if defined(NRF_APPLICATION)
#define LFXO_NODE DT_NODELABEL(lfxo)
#define HFXO_NODE DT_NODELABEL(hfxo)
#endif

static int nordicsemi_nrf54l_init(void)
{
	/* Update the SystemCoreClock global variable with current core clock
	 * retrieved from hardware state.
	 */
	SystemCoreClockUpdate();

#if defined(NRF_APPLICATION)
	/* Enable ICACHE */
	sys_cache_instr_enable();

#if DT_ENUM_HAS_VALUE(LFXO_NODE, load_capacitors, internal)
	uint32_t xosc32ktrim = NRF_FICR->XOSC32KTRIM;

	uint32_t offset_k =
		(xosc32ktrim & FICR_XOSC32KTRIM_OFFSET_Msk) >> FICR_XOSC32KTRIM_OFFSET_Pos;

	uint32_t slope_field_k =
		(xosc32ktrim & FICR_XOSC32KTRIM_SLOPE_Msk) >> FICR_XOSC32KTRIM_SLOPE_Pos;
	uint32_t slope_mask_k = FICR_XOSC32KTRIM_SLOPE_Msk >> FICR_XOSC32KTRIM_SLOPE_Pos;
	uint32_t slope_sign_k = (slope_mask_k - (slope_mask_k >> 1));
	int32_t slope_k = (int32_t)(slope_field_k ^ slope_sign_k) - (int32_t)slope_sign_k;

	/* As specified in the nRF54L15 PS:
	 * CAPVALUE = round( (CAPACITANCE - 4) * (FICR->XOSC32KTRIM.SLOPE + 0.765625 * 2^9)/(2^9)
	 *            + FICR->XOSC32KTRIM.OFFSET/(2^6) );
	 * where CAPACITANCE is the desired capacitor value in pF, holding any
	 * value between 4 pF and 18 pF in 0.5 pF steps.
	 */
	uint32_t mid_val =
		(((DT_PROP(LFXO_NODE, load_capacitance_femtofarad) * 2UL) / 1000UL - 8UL) *
		 (uint32_t)(slope_k + 392)) + (offset_k << 4UL);
	uint32_t capvalue_k = mid_val >> 10UL;

	/* Round. */
	if ((mid_val % 1024UL) >= 512UL) {
		capvalue_k++;
	}
	nrf_oscillators_lfxo_cap_set(NRF_OSCILLATORS, (nrf_oscillators_lfxo_cap_t)capvalue_k);
#elif DT_ENUM_HAS_VALUE(LFXO_NODE, load_capacitors, external)
	nrf_oscillators_lfxo_cap_set(NRF_OSCILLATORS, (nrf_oscillators_lfxo_cap_t)0);
#endif

#if DT_ENUM_HAS_VALUE(HFXO_NODE, load_capacitors, internal)
	uint32_t xosc32mtrim = NRF_FICR->XOSC32MTRIM;
	/* The SLOPE field is in the two's complement form, hence this special
	 * handling. Ideally, it would result in just one SBFX instruction for
	 * extracting the slope value, at least gcc is capable of producing such
	 * output, but since the compiler apparently tries first to optimize
	 * additions and subtractions, it generates slightly less than optimal
	 * code.
	 */
	uint32_t slope_field =
		(xosc32mtrim & FICR_XOSC32MTRIM_SLOPE_Msk) >> FICR_XOSC32MTRIM_SLOPE_Pos;
	uint32_t slope_mask = FICR_XOSC32MTRIM_SLOPE_Msk >> FICR_XOSC32MTRIM_SLOPE_Pos;
	uint32_t slope_sign = (slope_mask - (slope_mask >> 1));
	int32_t slope_m = (int32_t)(slope_field ^ slope_sign) - (int32_t)slope_sign;
	uint32_t offset_m =
		(xosc32mtrim & FICR_XOSC32MTRIM_OFFSET_Msk) >> FICR_XOSC32MTRIM_OFFSET_Pos;
	/* As specified in the nRF54L15 PS:
	 * CAPVALUE = (((CAPACITANCE-5.5)*(FICR->XOSC32MTRIM.SLOPE+791)) +
	 *              FICR->XOSC32MTRIM.OFFSET<<2)>>8;
	 * where CAPACITANCE is the desired total load capacitance value in pF,
	 * holding any value between 4.0 pF and 17.0 pF in 0.25 pF steps.
	 */
	uint32_t capvalue =
		(((((DT_PROP(HFXO_NODE, load_capacitance_femtofarad) * 4UL) / 1000UL) - 22UL) *
		  (uint32_t)(slope_m + 791) / 4UL) + (offset_m << 2UL)) >> 8UL;

	nrf_oscillators_hfxo_cap_set(NRF_OSCILLATORS, true, capvalue);
#elif DT_ENUM_HAS_VALUE(HFXO_NODE, load_capacitors, external)
	nrf_oscillators_hfxo_cap_set(NRF_OSCILLATORS, false, 0);
#endif

	if (IS_ENABLED(CONFIG_SOC_NRF_FORCE_CONSTLAT)) {
		nrf_power_task_trigger(NRF_POWER, NRF_POWER_TASK_CONSTLAT);
	}

	if (IS_ENABLED(CONFIG_SOC_NRF54L_VREG_MAIN_DCDC)) {
		nrf_regulators_vreg_enable_set(NRF_REGULATORS, NRF_REGULATORS_VREG_MAIN, true);
	}

	if (IS_ENABLED(CONFIG_SOC_NRF54L_NORMAL_VOLTAGE_MODE)) {
		nrf_regulators_vreg_enable_set(NRF_REGULATORS, NRF_REGULATORS_VREG_MEDIUM, false);
	}

#if defined(CONFIG_ELV_GRTC_LFXO_ALLOWED)
	nrf_regulators_elv_mode_allow_set(NRF_REGULATORS, NRF_REGULATORS_ELV_ELVGRTCLFXO_MASK);
#endif /* CONFIG_ELV_GRTC_LFXO_ALLOWED */
#endif /* NRF_APPLICATION */

	return 0;
}

void arch_busy_wait(uint32_t time_us)
{
	nrfx_coredep_delay_us(time_us);
}

SYS_INIT(nordicsemi_nrf54l_init, PRE_KERNEL_1, 0);
