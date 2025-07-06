/*
 * Copyright (c) 2019 Vestas Wind Systems A/S
 *
 * Based on clock_control_rv32m1_pcc.c, which is:
 * Copyright (c) 2018 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_kinetis_pcc

#include <errno.h>
#include <soc.h>
#include <zephyr/drivers/clock_control.h>
#include <fsl_clock.h>

#define LOG_LEVEL CONFIG_CLOCK_CONTROL_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(clock_control_mcux_pcc);

struct mcux_pcc_config {
	uint32_t base_address;
	uint32_t *clocks;
	uint32_t clock_num;
};

#define DEV_BASE(dev) (((struct mcux_pcc_config *)(dev->config))->base_address)
#ifndef MAKE_PCC_REGADDR
#define MAKE_PCC_REGADDR(base, offset) ((base) + (offset))
#endif

static inline int get_clock_encoding(const struct device *dev,
				     clock_control_subsys_t sub_system,
				     uint32_t *clock_encoding)
{
	const struct mcux_pcc_config *cfg;
	uint32_t clock_name;

	cfg = dev->config;
	clock_name = POINTER_TO_UINT(sub_system);

	if (!cfg->clock_num) {
		*clock_encoding = MAKE_PCC_REGADDR(DEV_BASE(dev), clock_name);
		return 0;
	}

	/* sanity check */
	if (clock_name >= cfg->clock_num) {
		return -EINVAL;
	}

	*clock_encoding = cfg->clocks[clock_name];

	return 0;
}

static int mcux_pcc_on(const struct device *dev,
		       clock_control_subsys_t sub_system)
{
	uint32_t clock_encoding;
	int ret;

	ret = get_clock_encoding(dev, sub_system, &clock_encoding);
	if (ret < 0) {
		return ret;
	}

	CLOCK_EnableClock(clock_encoding);

	return 0;
}

static int mcux_pcc_off(const struct device *dev,
			clock_control_subsys_t sub_system)
{
	uint32_t clock_encoding;
	int ret;

	ret = get_clock_encoding(dev, sub_system, &clock_encoding);
	if (ret < 0) {
		return ret;
	}

	CLOCK_DisableClock(clock_encoding);

	return 0;
}

static int mcux_pcc_get_rate(const struct device *dev,
			       clock_control_subsys_t sub_system,
			       uint32_t *rate)
{
	uint32_t clock_encoding;
	int ret;

	ret = get_clock_encoding(dev, sub_system, &clock_encoding);
	if (ret < 0) {
		return ret;
	}

	*rate = CLOCK_GetIpFreq(clock_encoding);

	return 0;
}

static DEVICE_API(clock_control, mcux_pcc_api) = {
	.on = mcux_pcc_on,
	.off = mcux_pcc_off,
	.get_rate = mcux_pcc_get_rate,
};

static int mcux_pcc_init(const struct device *dev)
{
#ifdef CONFIG_SOC_MIMX8UD7
	/* 8ULP's XTAL is set to 24MHz on EVK9. We keep
	 * this as SOC level because this should also be
	 * the case for the EVK board.
	 */
	CLOCK_SetXtal0Freq(24000000);
#endif /* CONFIG_SOC_MIMX8UD7 */
	return 0;
}

#ifdef CONFIG_SOC_MIMX8UD7
static uint32_t clocks[] = {
	/* clocks managed through PCC4 */
	kCLOCK_Lpuart7,
};
#else
/* this is empty for SOCs which don't need a translation from
 * the clock ID passed through the DTS and the clock ID encoding
 * from the HAL. For these SOCs, the clock ID will be built based
 * on the value passed from the DTS and the PCC base.
 */
static uint32_t clocks[] = {};
#endif /* CONFIG_SOC_MIMX8UD7 */

#define MCUX_PCC_INIT(inst)						\
	static const struct mcux_pcc_config mcux_pcc##inst##_config = {	\
		.base_address = DT_INST_REG_ADDR(inst),			\
		.clocks = clocks,					\
		.clock_num = ARRAY_SIZE(clocks),			\
	};								\
									\
	DEVICE_DT_INST_DEFINE(inst,					\
			    mcux_pcc_init,				\
			    NULL,					\
			    NULL, &mcux_pcc##inst##_config,		\
			    PRE_KERNEL_1,				\
			    CONFIG_CLOCK_CONTROL_INIT_PRIORITY,		\
			    &mcux_pcc_api);

DT_INST_FOREACH_STATUS_OKAY(MCUX_PCC_INIT)
