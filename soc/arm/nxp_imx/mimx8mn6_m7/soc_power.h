/*
 * Copyright (c) 2020, Manivannan Sadhasivam <mani@kernel.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC_POWER__H_
#define _SOC_POWER__H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE

#include <fsl_device_registers.h>
#include <fsl_gpc.h>

#endif /* !_ASMLANGUAGE */

#ifdef __cplusplus
}
#endif
	
	/*macros inherited from SDK*/
#define APP_PowerUpSlot (5U)
#define APP_PowerDnSlot (6U)
	
	/*Configure the SoC low power mode module with an IRQID as wakeup source*/
void soc_config_lpm(uint32_t irqId);

	/*Entering a low power mode*/
void soc_enter_lpm(enum pm_state state);

	/*Leaving a low power mode*/
void soc_leave_lpm(void);	

#endif /* _SOC_POWER__H_ */
