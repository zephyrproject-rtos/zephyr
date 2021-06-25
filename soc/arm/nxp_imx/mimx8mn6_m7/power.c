/*
 * Copyright (c) 2021, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <power/power.h>
#include <fsl_gpc.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);



/* Invoke Low Power/System Off specific Tasks */
void pm_power_state_set(struct pm_state_info info)
{
	/* FIXME: When this function is entered the Kernel has disabled
	 * interrupts using BASEPRI register. This is incorrect as it prevents
	 * waking up from any interrupt which priority is not 0. Work around the
	 * issue and disable interrupts using PRIMASK register as recommended
	 * by ARM.
	 */

	 
    gpc_lpm_config_t config;
    config.enCpuClk              = false;
    config.enFastWakeUp          = false;
    config.enDsmMask             = false;
    config.enWfiMask             = false;
    config.enVirtualPGCPowerdown = true;
    config.enVirtualPGCPowerup   = true;
    
    
	switch (info.state) {
	case PM_STATE_SUSPEND_TO_IDLE:
	case PM_STATE_RUNTIME_IDLE:
		//printk("\tPM: Entered WAIT mode iA\n");
		__disable_irq();
		irq_unlock(0);
		GPC_EnterWaitMode(GPC, &config);
		break;
	case PM_STATE_SUSPEND_TO_RAM:
		//printk("\tPM: Entered SUSPEND mode iA\n");
		__disable_irq();
		irq_unlock(0);
		GPC_EnterWaitMode(GPC, &config);
		break;
	case PM_STATE_ACTIVE:
		//printk("\tPM: Waking up\n");
		GPC->LPCR_M7 = GPC->LPCR_M7 & (~GPC_LPCR_M7_LPM0_MASK);
		__enable_irq();
	default:
		//printk("\tPM: Unsupported power state %u", info.state);
		break;
	}

}

/* Handle SOC specific activity after Low Power Mode Exit */
void pm_power_state_exit_post_ops(struct pm_state_info info)
{

	//printk("\tPM: Exit ops called iA\n");
	ARG_UNUSED(info);

	/* Clear PRIMASK */
	GPC->LPCR_M7 = GPC->LPCR_M7 & (~GPC_LPCR_M7_LPM0_MASK);
	__enable_irq();
}
