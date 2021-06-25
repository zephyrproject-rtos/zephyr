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


	/*From NXP SDK lpm.h*/
#define APP_PowerUpSlot (5U)
#define APP_PowerDnSlot (6U)
	
GPC_Init(GPC, APP_PowerUpSlot, APP_PowerDnSlot);
void lpm_init();
void lpm_int_config();


#endif /* _SOC_POWER__H_ */
