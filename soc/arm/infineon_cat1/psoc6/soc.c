/* Copyright (c) 2021 Cypress Semiconductor Corporation.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <init.h>
#include <kernel.h>
#include "cy_sysint.h"

cy_en_sysint_status_t Cy_SysInt_Init(const cy_stc_sysint_t *config, cy_israddress userIsr)
{
	cy_en_sysint_status_t status = CY_SYSINT_SUCCESS;

#if (CY_CPU_CORTEX_M0P)
	#error Cy_SysInt_Init does not support CM0p core.
#endif /* (CY_CPU_CORTEX_M0P) */

	if (NULL != config) {
		CY_ASSERT_L3(CY_SYSINT_IS_PRIORITY_VALID(config->intrPriority));

		/*  Configure a dynamic interrupt */
		(void) irq_connect_dynamic(config->intrSrc, config->intrPriority, (void *) userIsr, NULL, 0);
	} else {
		status = CY_SYSINT_BAD_PARAM;
	}

	return(status);
}

static int init_cycfg_platform_wraper(const struct device *arg)
{
	ARG_UNUSED(arg);

	/* Initializes the system */
	SystemInit();
	return 0;
}

SYS_INIT(init_cycfg_platform_wraper, PRE_KERNEL_1, 0);
