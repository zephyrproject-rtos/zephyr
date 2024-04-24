/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/dt-bindings/rdc/imx_rdc.h>
#include <fsl_common.h>
#include <fsl_rdc.h>

/* set RDC permission for peripherals */
static void soc_rdc_init(void)
{
	rdc_domain_assignment_t assignment = {0};
	rdc_periph_access_config_t periphConfig;

	RDC_Init(RDC);
	assignment.domainId = A53_DOMAIN_ID;
	RDC_SetMasterDomainAssignment(RDC, kRDC_Master_A53, &assignment);

	RDC_GetDefaultPeriphAccessConfig(&periphConfig);

#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart2), okay) && DT_NODE_HAS_PROP(DT_NODELABEL(uart2), rdc)
	periphConfig.periph = kRDC_Periph_UART2;
	periphConfig.policy = RDC_DT_VAL(uart2);
	RDC_SetPeriphAccessConfig(RDC, &periphConfig);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart4), okay) && DT_NODE_HAS_PROP(DT_NODELABEL(uart4), rdc)
	periphConfig.periph = kRDC_Periph_UART4;
	periphConfig.policy = RDC_DT_VAL(uart4);
	RDC_SetPeriphAccessConfig(RDC, &periphConfig);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(enet), okay) && DT_NODE_HAS_PROP(DT_NODELABEL(enet), rdc)
	periphConfig.periph = kRDC_Periph_ENET1;
	periphConfig.policy = RDC_DT_VAL(enet);
	RDC_SetPeriphAccessConfig(RDC, &periphConfig);
#endif
}

static int soc_init(void)
{
	soc_rdc_init();
	return 0;
}

SYS_INIT(soc_init, EARLY, 1);
