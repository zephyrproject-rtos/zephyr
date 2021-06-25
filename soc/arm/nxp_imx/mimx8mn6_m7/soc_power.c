/*
 * Copyright (c) 2021, Waseem Hassan
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <power/power_state.h>
#include "soc_power.h"


void soc_config_lpm(uint32_t irqId){

	printk("Configuring low power module\n");

  	GPC_Init(GPC, APP_PowerUpSlot, APP_PowerDnSlot);
	GPC_EnableIRQ(GPC, irqId);

}

void soc_enter_lpm(enum pm_state state){
	
    gpc_lpm_config_t config;
    config.enCpuClk              = false;
    config.enFastWakeUp          = false;
    config.enDsmMask             = false;
    config.enWfiMask             = false;
    config.enVirtualPGCPowerdown = true;
    config.enVirtualPGCPowerup   = true;
    
    
	switch (state) {
	case PM_STATE_SUSPEND_TO_IDLE:
        __DSB();
        __ISB();
        __WFI();
		GPC_EnterWaitMode(GPC, &config);
		break;
	case PM_STATE_SUSPEND_TO_RAM:
        __DSB();
        __ISB();
        __WFI();
		GPC_EnterStopMode(GPC, &config);
		break;
	case PM_STATE_ACTIVE:
		GPC->LPCR_M7 = GPC->LPCR_M7 & (~GPC_LPCR_M7_LPM0_MASK);
		__enable_irq();
		break;
	default:
		printk("Unsupported power state %u", state);
		break;
	}
}

