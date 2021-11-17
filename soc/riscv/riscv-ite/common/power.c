/*
 * Copyright (c) 2021 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <pm/pm.h>
#include <soc.h>
#include <zephyr.h>

/* Handle when enter deep doze mode. */
static void ite_power_soc_deep_doze(void)
{
	/* Enter deep doze mode */
	riscv_idle(CHIP_PLL_DEEP_DOZE, MSTATUS_IEN);
}

/* Invoke Low Power/System Off specific Tasks */
__weak void pm_power_state_set(struct pm_state_info info)
{
	switch (info.state) {
	/* Deep doze mode */
	case PM_STATE_STANDBY:
		ite_power_soc_deep_doze();
		break;
	default:
		break;
	}
}

/* Handle SOC specific activity after Low Power Mode Exit */
__weak void pm_power_state_exit_post_ops(struct pm_state_info info)
{
	ARG_UNUSED(info);
}
