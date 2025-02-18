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
#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/cache.h>
#include <soc/nrfx_coredep.h>
#include <system_nrf54l.h>
#include <soc.h>
LOG_MODULE_REGISTER(soc, CONFIG_SOC_LOG_LEVEL);

#if (defined(NRF_APPLICATION) && !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)) || \
	!defined(__ZEPHYR__)

#include <nrf_erratas.h>
#include <hal/nrf_glitchdet.h>
#include <hal/nrf_oscillators.h>
#include <hal/nrf_power.h>
#include <hal/nrf_regulators.h>
#include <zephyr/dt-bindings/regulator/nrf5x.h>

#define LFXO_NODE DT_NODELABEL(lfxo)
#define HFXO_NODE DT_NODELABEL(hfxo)

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

	/* Encoding of desired capacitance (single ended) to value required for INTCAP core
	 * calculation: (CAP_VAL - 4 pF)* 0.5
	 * That translate to ((CAP_VAL_FEMTO_F - 4000fF) * 2UL) / 1000UL
	 *
	 * NOTE: The desired capacitance value is used in encoded from in INTCAP calculation formula
	 *       That is different than in case of HFXO.
	 */
	uint32_t cap_val_encoded = (((DT_PROP(LFXO_NODE, load_capacitance_femtofarad) - 4000UL)
				     * 2UL) / 1000UL);

	/* Calculation of INTCAP code before rounding. Min that calculations here are done on
	 * values multiplied by 2^9, e.g. 0.765625 * 2^9 = 392.
	 * offset_k should be divided by 2^6, but to add it to value shifted by 2^9 we have to
	 * multiply it be 2^3.
	 */
	uint32_t mid_val = (cap_val_encoded - 4UL) * (uint32_t)(slope_k + 392UL)
			   + (offset_k << 3UL);

	/* Get integer part of the INTCAP code */
	uint32_t lfxo_intcap = mid_val >> 9UL;

	/* Round based on fractional part */
	if ((mid_val & BIT_MASK(9)) > (BIT_MASK(9) / 2)) {
		lfxo_intcap++;
	}

	nrf_oscillators_lfxo_cap_set(NRF_OSCILLATORS, lfxo_intcap);
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

	/* NOTE 1: Requested HFXO internal capacitance in femto Faradas is used directly in formula
	 *         to calculate INTCAP code. That is different than in case of LFXO.
	 *
	 * NOTE 2: PS formula uses piko Farads, the implementation of the formula uses femto Farads
	 *         to avoid use of floating point data type.
	 */
	uint32_t cap_val_femto_f = DT_PROP(HFXO_NODE, load_capacitance_femtofarad);

	uint32_t mid_val_intcap = (((cap_val_femto_f - 5500UL) * (uint32_t)(slope_m + 791UL))
		 + (offset_m << 2UL) * 1000UL) >> 8UL;

	/* Convert the calculated value to piko Farads */
	uint32_t hfxo_intcap = mid_val_intcap / 1000;

	/* Round based on fractional part */
	if (mid_val_intcap % 1000 >= 500) {
		hfxo_intcap++;
	}

	nrf_oscillators_hfxo_cap_set(NRF_OSCILLATORS, true, hfxo_intcap);

#elif DT_ENUM_HAS_VALUE(HFXO_NODE, load_capacitors, external)
	nrf_oscillators_hfxo_cap_set(NRF_OSCILLATORS, false, 0);
#endif

	if (IS_ENABLED(CONFIG_SOC_NRF_FORCE_CONSTLAT)) {
		nrf_power_task_trigger(NRF_POWER, NRF_POWER_TASK_CONSTLAT);
	}

#if (DT_PROP(DT_NODELABEL(vregmain), regulator_initial_mode) == NRF5X_REG_MODE_DCDC)
#if NRF54L_ERRATA_31_ENABLE_WORKAROUND
	/* Workaround for Errata 31 */
	if (nrf54l_errata_31()) {
		*((volatile uint32_t *)0x50120624ul) = 20 | 1<<5;
		*((volatile uint32_t *)0x5012063Cul) &= ~(1<<19);
	}
#endif
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

	sys_cache_instr_enable();

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

SYS_INIT(nordicsemi_nrf54l_init, PRE_KERNEL_1, 0);
