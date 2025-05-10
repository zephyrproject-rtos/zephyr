/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This file abstracts operations exposed by fsl_power.h from NXP HAL,
 * for cases when that driver can't be compiled (DSP targets).
 */

#ifndef __ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_NXP_PINT_POWER_H__
#define __ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_NXP_PINT_POWER_H__

#include <stdbool.h>
#include <zephyr/irq.h>
#include <fsl_pint.h>

#if defined(FSL_FEATURE_SOC_PMC_COUNT) && FSL_FEATURE_SOC_PMC_COUNT > 0
#include <fsl_power.h>
#endif

static inline void nxp_pint_pin_deep_sleep_irq(uint8_t irq, bool wake)
{
#if (defined(FSL_FEATURE_SOC_PMC_COUNT) && FSL_FEATURE_SOC_PMC_COUNT > 0) &&                       \
	!(defined(FSL_FEATURE_POWERLIB_EXTEND) && (FSL_FEATURE_POWERLIB_EXTEND != 0))
	if (wake) {
		EnableDeepSleepIRQ(irq);
	} else {
		DisableDeepSleepIRQ(irq);
		irq_enable(irq);
	}
#else
	ARG_UNUSED(irq);
	ARG_UNUSED(wake);
#endif
}

#endif
