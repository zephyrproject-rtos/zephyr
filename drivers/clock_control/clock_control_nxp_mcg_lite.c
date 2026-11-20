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

#define MCG_NODE DT_INST(0, nxp_mcg_lite)

/* MCGOUTCLK source as described by the MCG-Lite node */
#if DT_ENUM_HAS_VALUE(MCG_NODE, clock_source, hirc)
#define MCG_LITE_OUT_SRC kMCGLITE_ClkSrcHirc
#elif DT_ENUM_HAS_VALUE(MCG_NODE, clock_source, lirc)
#define MCG_LITE_OUT_SRC kMCGLITE_ClkSrcLirc
#else
#define MCG_LITE_OUT_SRC kMCGLITE_ClkSrcExt
#endif

/* LIRC mode selection as described by the MCG-Lite node */
#if DT_PROP(MCG_NODE, lirc_frequency) == MCG_LITE_LIRC_8M_FREQ
#define MCG_LITE_IRCS kMCGLITE_Lirc8M
#else
#define MCG_LITE_IRCS kMCGLITE_Lirc2M
#endif

/* MCG-Lite has no FLL/PLL, so the configuration is static and applied at init. */
static const mcglite_config_t nxp_mcg_lite_config = {
	.outSrc = MCG_LITE_OUT_SRC,
	.irclkEnableMode = (DT_PROP(MCG_NODE, lirc_enable) ? kMCGLITE_IrclkEnable : 0U) |
			   (DT_PROP(MCG_NODE, lirc_stop_enable) ? kMCGLITE_IrclkEnableInStop : 0U),
	.ircs = MCG_LITE_IRCS,
	.fcrdiv = DT_PROP(MCG_NODE, fcrdiv),
	.lircDiv2 = DT_PROP(MCG_NODE, lircdiv2),
	.hircEnableInNotHircMode = DT_PROP(MCG_NODE, hirc_enable),
};

/* LIRC_DIV1 rate from MCG_SC[FCRDIV] and MCG_C2[IRCS]; 0 while LIRC is idle. */
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

static int nxp_mcg_lite_init(const struct device *dev)
{
	uint32_t clkdiv1;

	ARG_UNUSED(dev);

	/*
	 * CLOCK_SetMcgliteConfig() goes through HIRC when it has to change IRCS
	 * while LIRC drives MCGOUTCLK, because the LIRC frequency cannot be
	 * updated on the fly. Keep the SIM safe dividers active for that
	 * transition so the flash clock stays within its maximum frequency, then
	 * restore the configured dividers.
	 */
	clkdiv1 = SIM->CLKDIV1;
	CLOCK_SetSimSafeDivs();

	if (CLOCK_SetMcgliteConfig(&nxp_mcg_lite_config) != kStatus_Success) {
		SIM->CLKDIV1 = clkdiv1;
		LOG_ERR("Failed to configure MCG-Lite");
		return -EIO;
	}

	SIM->CLKDIV1 = clkdiv1;

	/* Refresh SystemCoreClock for the new MCGOUTCLK. */
	SystemCoreClock = CLOCK_GetCoreSysClkFreq();

	return 0;
}

static int nxp_mcg_lite_on(const struct device *dev, clock_control_subsys_t sub_system)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(sub_system);

	return 0;
}

static int nxp_mcg_lite_off(const struct device *dev, clock_control_subsys_t sub_system)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(sub_system);

	return 0;
}

static int nxp_mcg_lite_get_rate(const struct device *dev, clock_control_subsys_t sub_system,
				  uint32_t *rate)
{
	ARG_UNUSED(dev);

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

DEVICE_DT_INST_DEFINE(0, nxp_mcg_lite_init, NULL, NULL, NULL, PRE_KERNEL_1,
		      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &nxp_mcg_lite_driver_api);
