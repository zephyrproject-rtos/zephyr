/*
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <soc.h>
#include <clock_control.h>
#include <dt-bindings/clock/imx_ccm.h>
#include <fsl_clock.h>

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_CLOCK_CONTROL_LEVEL
#include <logging/sys_log.h>

static int mcux_ccm_on(struct device *dev,
			      clock_control_subsys_t sub_system)
{
	return 0;
}

static int mcux_ccm_off(struct device *dev,
			       clock_control_subsys_t sub_system)
{
	return 0;
}

static int mcux_ccm_get_subsys_rate(struct device *dev,
				    clock_control_subsys_t sub_system,
				    u32_t *rate)
{
	u32_t clock_name = (u32_t) sub_system;

	switch (clock_name) {
	case IMX_CCM_LPUART_CLK:
		if (CLOCK_GetMux(kCLOCK_UartMux) == 0) {
			*rate = CLOCK_GetPllFreq(kCLOCK_PllUsb1) / 6
				/ (CLOCK_GetDiv(kCLOCK_UartDiv) + 1);
		} else {
			*rate = CLOCK_GetOscFreq()
				/ (CLOCK_GetDiv(kCLOCK_UartDiv) + 1);
		}

		break;
	}

	return 0;
}

static int mcux_ccm_init(struct device *dev)
{
	return 0;
}

static const struct clock_control_driver_api mcux_ccm_driver_api = {
	.on = mcux_ccm_on,
	.off = mcux_ccm_off,
	.get_rate = mcux_ccm_get_subsys_rate,
};

DEVICE_AND_API_INIT(mcux_ccm, CONFIG_MCUX_CCM_NAME,
		    &mcux_ccm_init,
		    NULL, NULL,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &mcux_ccm_driver_api);
