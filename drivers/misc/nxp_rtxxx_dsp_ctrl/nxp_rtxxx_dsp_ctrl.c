/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/misc/nxp_rtxxx_dsp_ctrl/nxp_rtxxx_dsp_ctrl.h>
#include <zephyr/dt-bindings/misc/nxp_rtxxx_dsp_ctrl.h>

#include <fsl_device_registers.h>
#include <fsl_dsp.h>
#include <fsl_clock.h>
#include <fsl_power.h>

#include <errno.h>
#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>

struct nxp_rtxxx_dsp_ctrl_region {
	void *base;
	int32_t length;
};

struct nxp_rtxxx_dsp_ctrl_config {
	volatile uint32_t *reg_dspstall;
	struct nxp_rtxxx_dsp_ctrl_region regions[NXP_RTXXX_DSP_REGION_MAX];
};

static void dsp_ctrl_enable(const struct device *dev)
{
	const struct nxp_rtxxx_dsp_ctrl_config *cfg =
		(const struct nxp_rtxxx_dsp_ctrl_config *)dev->config;

	*cfg->reg_dspstall = 0;
}

static void dsp_ctrl_disable(const struct device *dev)
{
	const struct nxp_rtxxx_dsp_ctrl_config *cfg =
		(const struct nxp_rtxxx_dsp_ctrl_config *)dev->config;

	*cfg->reg_dspstall = 1;
}

static int dsp_ctrl_load_section(const struct device *dev, const void *base, size_t length,
				 enum nxp_rtxxx_dsp_ctrl_section_type section)
{
	if (section >= NXP_RTXXX_DSP_REGION_MAX) {
		return -EINVAL;
	}

	const struct nxp_rtxxx_dsp_ctrl_config *cfg =
		(const struct nxp_rtxxx_dsp_ctrl_config *)dev->config;

	if (cfg->regions[section].base == NULL) {
		return -EINVAL;
	}

	if (length > cfg->regions[section].length) {
		return -ENOMEM;
	}

	/*
	 * Custom memcpy implementation is needed because the DSP TCMs can be accessed
	 * only by 32 bits.
	 */
	const uint32_t *src = (const uint32_t *)base;
	uint32_t *dst = cfg->regions[section].base;

	for (size_t remaining = length; remaining > 0; remaining -= sizeof(uint32_t)) {
		*dst++ = *src++;

		if (remaining < sizeof(uint32_t)) {
			break;
		}
	}

	return 0;
}

static struct nxp_rtxxx_dsp_ctrl_api nxp_rtxxx_dsp_ctrl_api = {
	.load_section = dsp_ctrl_load_section,
	.enable = dsp_ctrl_enable,
	.disable = dsp_ctrl_disable
};

#define NXP_RTXXX_DSP_SECTION(child_node_id, n)                                                    \
	[DT_PROP(child_node_id, type)] = {.base = (void *)DT_REG_ADDR(child_node_id),              \
					  .length = DT_REG_SIZE(child_node_id)},

#define NXP_RTXXX_DSP_CTRL(n, dspstall)                                                            \
	static const struct nxp_rtxxx_dsp_ctrl_config nxp_rtxxx_dsp_ctrl_##n##_config = {          \
		.reg_dspstall = (dspstall),                                                        \
		.regions = {DT_INST_FOREACH_CHILD_VARGS(n, NXP_RTXXX_DSP_SECTION, n)}};            \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, nxp_rtxxx_dsp_ctrl_##n##_init, NULL, NULL,                        \
			      &nxp_rtxxx_dsp_ctrl_##n##_config, PRE_KERNEL_1,                      \
			      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &nxp_rtxxx_dsp_ctrl_api);

/* VARIANT: nxp,rt600-dsp-ctrl */
#define NXP_RTXXX_DSP_CTRL_RT600_HIFI4(n)                                                          \
	static int nxp_rtxxx_dsp_ctrl_##n##_init(const struct device *dev)                         \
	{                                                                                          \
		CLOCK_InitSysPfd(kCLOCK_Pfd1, 16);                                                 \
		CLOCK_AttachClk(kDSP_PLL_to_DSP_MAIN_CLK);                                         \
		CLOCK_SetClkDiv(kCLOCK_DivDspCpuClk, 1);                                           \
		CLOCK_SetClkDiv(kCLOCK_DivDspRamClk, 2);                                           \
                                                                                                   \
		DSP_Init();                                                                        \
		return 0;                                                                          \
	}                                                                                          \
	NXP_RTXXX_DSP_CTRL(n, &((SYSCTL0_Type *)DT_REG_ADDR(DT_INST_PHANDLE(n, sysctl)))->DSPSTALL)
#define DT_DRV_COMPAT nxp_rt600_dsp_ctrl
DT_INST_FOREACH_STATUS_OKAY(NXP_RTXXX_DSP_CTRL_RT600_HIFI4);

/* VARIANT: nxp,rt700-dsp-ctrl-hifi4 */
#undef DT_DRV_COMPAT
#define NXP_RTXXX_DSP_CTRL_RT700_HIFI4(n)                                                          \
	static int nxp_rtxxx_dsp_ctrl_##n##_init(const struct device *dev)                         \
	{                                                                                          \
		PMC0->PDRUNCFG2 &= ~0x0003C000U; /* power up dsp used SRAM. */                     \
		PMC0->PDRUNCFG3 &= ~0x0003C000U;                                                   \
		POWER_DisablePD(kPDRUNCFG_PD_VDD2_DSP);                                            \
		POWER_ApplyPD();                                                                   \
                                                                                                   \
		CLOCK_SetClkDiv(kCLOCK_DivDspClk, 1U);                                             \
		CLOCK_AttachClk(kFRO0_DIV1_to_DSP);                                                \
                                                                                                   \
		DSP_Init();                                                                        \
		return 0;                                                                          \
	}                                                                                          \
	NXP_RTXXX_DSP_CTRL(n, &((SYSCON0_Type *)DT_REG_ADDR(DT_INST_PHANDLE(n, sysctl)))->DSPSTALL)
#define DT_DRV_COMPAT nxp_rt700_dsp_ctrl_hifi4
DT_INST_FOREACH_STATUS_OKAY(NXP_RTXXX_DSP_CTRL_RT700_HIFI4);
