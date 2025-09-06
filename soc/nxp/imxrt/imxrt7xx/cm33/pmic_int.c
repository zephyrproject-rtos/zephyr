/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fsl_power.h"

/* Weak so board can override this function */
void __weak imxrt_disable_pmic_interrupt(void)
{
	/* Disable PMIC interrupt */
	POWER_DisableInterrupts(PMC_INTRCTRL_INTNIE_MASK);
}

void __weak imxrt_enable_pmic_interrupt(void)
{
	/* Enable PMIC interrupt */
	POWER_EnableInterrupts(PMC_INTRCTRL_INTNIE_MASK);
}

void __weak imxrt_clear_pmic_interrupt(void)
{
	/* Clear PMIC interrupt flag */
	POWER_ClearEventFlags(PMC_FLAGS_INTNF_MASK);
}
