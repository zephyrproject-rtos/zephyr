/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdint.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>

#include <fsl_clock.h>
#include <fsl_iomuxc.h>
#include <fsl_qtmr.h>

#include "trigger_src.h"

#define CAPTURE_COUNT DT_PROP_LEN(DT_NODELABEL(counter_loopback_0), test_counter_captures)

BUILD_ASSERT(CAPTURE_COUNT == 1, "RT1064 QTMR XBAR loopback supports one capture channel");

static int driven_level[CAPTURE_COUNT];

static void force_qtmr3_timer0_output(int level)
{
	uint16_t sctrl = TMR3->CHANNEL[kQTMR_Channel_0].SCTRL;

	sctrl &= ~(uint16_t)(TMR_SCTRL_VAL_MASK | TMR_SCTRL_FORCE_MASK);
	sctrl |= TMR_SCTRL_OEN_MASK | TMR_SCTRL_VAL(level ? 1U : 0U) | TMR_SCTRL_FORCE_MASK;
	TMR3->CHANNEL[kQTMR_Channel_0].SCTRL = sctrl;
}

static void setup_internal_loopback(void)
{
	static bool configured;
	qtmr_config_t config;
	uint32_t gpr6;

	if (configured) {
		return;
	}

	QTMR_GetDefaultConfig(&config);
	config.primarySource = kQTMR_ClockDivide_128;
	QTMR_Init(TMR3, kQTMR_Channel_0, &config);
	QTMR_StopTimer(TMR3, kQTMR_Channel_0);

	CLOCK_EnableClock(kCLOCK_Xbar1);
	XBARA1->SEL43 = (XBARA1->SEL43 & ~XBARA_SEL43_SEL86_MASK) |
			XBARA_SEL43_SEL86(kXBARA1_InputQtimer3Tmr0Output);

	gpr6 = IOMUXC_GPR->GPR6 & ~IOMUXC_GPR_GPR6_QTIMER1_TRM0_INPUT_SEL_MASK;
	IOMUXC_GPR->GPR6 = gpr6 | IOMUXC_GPR_GPR6_QTIMER1_TRM0_INPUT_SEL(1U);

	force_qtmr3_timer0_output(0);
	driven_level[0] = 0;
	configured = true;
}

int trigger_src_setup(size_t idx)
{
	if (idx >= CAPTURE_COUNT) {
		return -EINVAL;
	}

	setup_internal_loopback();
	force_qtmr3_timer0_output(0);
	driven_level[idx] = 0;

	return 0;
}

int trigger_src_get(size_t idx)
{
	if (idx >= CAPTURE_COUNT) {
		return -EINVAL;
	}

	return driven_level[idx];
}

int trigger_src_set(size_t idx, int level)
{
	if (idx >= CAPTURE_COUNT) {
		return -EINVAL;
	}

	force_qtmr3_timer0_output(level);
	driven_level[idx] = level ? 1 : 0;

	return 0;
}
