/*
 * Copyright (c) 2023 Bjarki Arge Andreasen
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/sys/util_macro.h>
#include <soc.h>

#define SOC_SUPC_WAKEUP_SOURCE_IDS (3)

void soc_supc_core_voltage_regulator_off(void)
{
	SUPC->SUPC_CR = SUPC_CR_KEY_PASSWD | SUPC_CR_VROFF_STOP_VREG;
}

void soc_supc_slow_clock_select_crystal_osc(void)
{
	SUPC->SUPC_CR = SUPC_CR_KEY_PASSWD | SUPC_CR_XTALSEL_CRYSTAL_SEL;

	/* Wait for oscillator to be stabilized. */
	while (!(SUPC->SUPC_SR & SUPC_SR_OSCSEL)) {
	}
}

void soc_supc_enable_wakeup_source(uint32_t wakeup_source_id)
{
	__ASSERT(wakeup_source_id <= SOC_SUPC_WAKEUP_SOURCE_IDS,
		 "Wakeup source channel is invalid");

	SUPC->SUPC_WUMR |= 1 << wakeup_source_id;
}
