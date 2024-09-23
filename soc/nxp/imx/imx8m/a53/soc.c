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

#if DT_NODE_HAS_STATUS(DT_NODELABEL(rdc), okay)

#define rdc_inst ((RDC_Type *)DT_REG_ADDR(DT_NODELABEL(rdc)))

/* set RDC permission for peripherals */
static void soc_rdc_init(void)
{
	rdc_domain_assignment_t assignment = {0};
	rdc_periph_access_config_t periphConfig;

	RDC_Init(rdc_inst);
	assignment.domainId = A53_DOMAIN_ID;
	RDC_SetMasterDomainAssignment(rdc_inst, kRDC_Master_A53, &assignment);

	RDC_GetDefaultPeriphAccessConfig(&periphConfig);

#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart2), okay) && DT_NODE_HAS_PROP(DT_NODELABEL(uart2), rdc)
	periphConfig.periph = kRDC_Periph_UART2;
	periphConfig.policy = RDC_DT_VAL(uart2);
	RDC_SetPeriphAccessConfig(rdc_inst, &periphConfig);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart4), okay) && DT_NODE_HAS_PROP(DT_NODELABEL(uart4), rdc)
	periphConfig.periph = kRDC_Periph_UART4;
	periphConfig.policy = RDC_DT_VAL(uart4);
	RDC_SetPeriphAccessConfig(rdc_inst, &periphConfig);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(enet), okay) && DT_NODE_HAS_PROP(DT_NODELABEL(enet), rdc)
	periphConfig.periph = kRDC_Periph_ENET1;
	periphConfig.policy = RDC_DT_VAL(enet);
	RDC_SetPeriphAccessConfig(rdc_inst, &periphConfig);
#endif
}
#else

#define soc_rdc_init() do { } while (false)

#endif

void soc_prep_hook(void)
{
	soc_rdc_init();
}
