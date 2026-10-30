/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_mcg_lite

#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/nxp_mcg_lite.h>
#include <soc.h>
#include <fsl_clock.h>

#define LOG_LEVEL CONFIG_CLOCK_CONTROL_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(clock_control_nxp_mcg_lite);

#define MCG_LITE_LIRC_2M_FREQ 2000000U
#define MCG_LITE_LIRC_8M_FREQ 8000000U

/*
 * fsl_clock keeps its LIRC_CLK getter static, so LIRC_DIV1 is read from the
 * registers here. LIRC runs when IRCLKEN is set or when it drives MCGOUTCLK.
 */
static uint32_t nxp_mcg_lite_lirc_div1_freq(void)
{
	uint32_t clkst = (MCG->S & MCG_S_CLKST_MASK) >> MCG_S_CLKST_SHIFT;
	uint32_t fcrdiv = (MCG->SC & MCG_SC_FCRDIV_MASK) >> MCG_SC_FCRDIV_SHIFT;
	uint32_t freq;

	if (((MCG->C1 & MCG_C1_IRCLKEN_MASK) == 0U) && (clkst != kMCGLITE_ClkSrcLirc)) {
		return 0U;
	}

	freq = (MCG->C2 & MCG_C2_IRCS_MASK) ? MCG_LITE_LIRC_8M_FREQ : MCG_LITE_LIRC_2M_FREQ;

	return freq >> fcrdiv;
}

static int nxp_mcg_lite_on(const struct device *dev, clock_control_subsys_t sub_system)
{
	return 0;
}

static int nxp_mcg_lite_off(const struct device *dev, clock_control_subsys_t sub_system)
{
	return 0;
}

static int nxp_mcg_lite_get_rate(const struct device *dev, clock_control_subsys_t sub_system,
				  uint32_t *rate)
{
	switch ((uint32_t)sub_system) {
	case NXP_MCG_LITE_OUT_CLK:
		*rate = CLOCK_GetOutClkFreq();
		break;
	case NXP_MCG_LITE_IRCLK:
		*rate = CLOCK_GetInternalRefClkFreq();
		break;
	case NXP_MCG_LITE_PCLK:
		*rate = CLOCK_GetPeriphClkFreq();
		break;
	case NXP_MCG_LITE_LIRC_DIV1_CLK:
		*rate = nxp_mcg_lite_lirc_div1_freq();
		break;
	default:
		LOG_ERR("Unsupported clock name");
		return -EINVAL;
	}

	return 0;
}

static DEVICE_API(clock_control, nxp_mcg_lite_driver_api) = {
	.on = nxp_mcg_lite_on,
	.off = nxp_mcg_lite_off,
	.get_rate = nxp_mcg_lite_get_rate,
};

DEVICE_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		      &nxp_mcg_lite_driver_api);
