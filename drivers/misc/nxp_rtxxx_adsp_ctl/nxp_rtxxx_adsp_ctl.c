/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_rtxxx_adsp_ctl

#include <zephyr/drivers/misc/nxp_rtxxx_adsp_ctl/nxp_rtxxx_adsp_ctl.h>
#include <zephyr/dt-bindings/misc/nxp_rtxxx_adsp_ctl.h>

#include <fsl_device_registers.h>
#include <fsl_dsp.h>
#include <fsl_clock.h>

#include <errno.h>
#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>

struct nxp_rtxxx_adsp_ctl_region {
	void *base;
	int32_t length;
};

struct nxp_rtxxx_adsp_ctl_config {
	SYSCTL0_Type *sysctl;
	struct nxp_rtxxx_adsp_ctl_region regions[NXP_RTXXX_ADSP_REGION_MAX];
};

static void adsp_ctl_enable(const struct device *dev)
{
	SYSCTL0_Type *sysctl = ((struct nxp_rtxxx_adsp_ctl_config *)dev->config)->sysctl;

	sysctl->DSPSTALL = 0;
}

static void adsp_ctl_disable(const struct device *dev)
{
	SYSCTL0_Type *sysctl = ((struct nxp_rtxxx_adsp_ctl_config *)dev->config)->sysctl;

	sysctl->DSPSTALL = 1;
}

static int adsp_ctl_load_section(const struct device *dev, const void *base, size_t length,
				 enum nxp_rtxxx_adsp_ctl_section_type section)
{
	if (section >= NXP_RTXXX_ADSP_REGION_MAX) {
		return -EINVAL;
	}

	struct nxp_rtxxx_adsp_ctl_config *cfg = (struct nxp_rtxxx_adsp_ctl_config *)dev->config;

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
	uint32_t *src = (uint32_t *)base;
	uint32_t *dst = cfg->regions[section].base;

	for (size_t remaining = length; remaining > 0; remaining -= sizeof(uint32_t)) {
		*dst++ = *src++;

		if (remaining < sizeof(uint32_t)) {
			break;
		}
	}

	return 0;
}

static int nxp_rtxxx_adsp_ctl_init(const struct device *dev)
{
	/*
	 * Initialize clocks associated with the DSP.
	 * Taken from DSP examples for the MIMXRT685-EVK in the MCUXpresso SDK.
	 */
	CLOCK_InitSysPfd(kCLOCK_Pfd1, 16);
	CLOCK_AttachClk(kDSP_PLL_to_DSP_MAIN_CLK);
	CLOCK_SetClkDiv(kCLOCK_DivDspCpuClk, 1);
	CLOCK_SetClkDiv(kCLOCK_DivDspRamClk, 2);

	DSP_Init();

	return 0;
}

static struct nxp_rtxxx_adsp_ctl_api nxp_rtxxx_adsp_ctl_api = {.load_section =
								       adsp_ctl_load_section,
							       .enable = adsp_ctl_enable,
							       .disable = adsp_ctl_disable};

#define NXP_RTXXX_ADSP_SECTION(child_node_id, n)                                                   \
	[DT_PROP(child_node_id, type)] = {.base = (void *)DT_REG_ADDR(child_node_id),              \
					  .length = DT_REG_SIZE(child_node_id)},

#define NXP_RTXXX_ADSP_CTL(n)                                                                      \
	static const struct nxp_rtxxx_adsp_ctl_config nxp_rtxxx_adsp_ctl_##n##_config = {          \
		.sysctl = (SYSCTL0_Type *)DT_REG_ADDR(DT_INST_PHANDLE(n, sysctl)),                 \
		.regions = {DT_INST_FOREACH_CHILD_VARGS(n, NXP_RTXXX_ADSP_SECTION, n)}};           \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, nxp_rtxxx_adsp_ctl_init, NULL, NULL,                              \
			      &nxp_rtxxx_adsp_ctl_##n##_config, PRE_KERNEL_1,                      \
			      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &nxp_rtxxx_adsp_ctl_api);

DT_INST_FOREACH_STATUS_OKAY(NXP_RTXXX_ADSP_CTL);
