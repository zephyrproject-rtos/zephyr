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

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(rdc))

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

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(uart2)) && DT_NODE_HAS_PROP(DT_NODELABEL(uart2), rdc)
	periphConfig.periph = kRDC_Periph_UART2;
	periphConfig.policy = RDC_DT_VAL(uart2);
	RDC_SetPeriphAccessConfig(rdc_inst, &periphConfig);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(uart4)) && DT_NODE_HAS_PROP(DT_NODELABEL(uart4), rdc)
	periphConfig.periph = kRDC_Periph_UART4;
	periphConfig.policy = RDC_DT_VAL(uart4);
	RDC_SetPeriphAccessConfig(rdc_inst, &periphConfig);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(enet)) && DT_NODE_HAS_PROP(DT_NODELABEL(enet), rdc)
	periphConfig.periph = kRDC_Periph_ENET1;
	periphConfig.policy = RDC_DT_VAL(enet);
	RDC_SetPeriphAccessConfig(rdc_inst, &periphConfig);
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(i2c3)) && DT_NODE_HAS_PROP(DT_NODELABEL(i2c3), rdc)
	periphConfig.periph = kRDC_Periph_I2C3;
	periphConfig.policy = RDC_DT_VAL(i2c3);
	RDC_SetPeriphAccessConfig(rdc_inst, &periphConfig);
#endif
}
#else

#define soc_rdc_init() do { } while (false)

#endif

static void soc_clock_init(void)
{
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(i2c3))
	/* Set I2C source to SysPLL1 Div5 160MHZ */
	CLOCK_SetRootMux(kCLOCK_RootI2c3, kCLOCK_I2cRootmuxSysPll1Div5);
	/* Set root clock to 160MHZ / 10 = 16MHZ */
	CLOCK_SetRootDivider(kCLOCK_RootI2c3, 1U, 10U);
	CLOCK_EnableClock(kCLOCK_I2c3);
#endif
}

void soc_prep_hook(void)
{
	soc_rdc_init();
	soc_clock_init();
}
