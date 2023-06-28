/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_control/clock_control_mcux_ccm_rev3.h>
#include <zephyr/drivers/firmware/imx_scu.h>
#include <fsl_clock.h>
#include <errno.h>

/* taken from imx8qm_ccm_clock_tree.c */
extern struct imx_ccm_clock_config clock_config;

struct imx_ccm_data mcux_ccm_data;

struct imx_ccm_config mcux_ccm_config = {
	.clock_config = &clock_config,
};

int imx_ccm_init(const struct device *dev)
{
	const struct device *scu_dev;
	struct imx_ccm_data *data;
	sc_ipc_t ipc_handle;

	data = dev->data;

	scu_dev = DEVICE_DT_GET(DT_NODELABEL(scu));

	/* this is somewhat pointless as the init priority
	 * for the SCU firmware driver is set to a lower value
	 * than the clock control subsystem's.
	 *
	 * Despite this, let's just be cautious.
	 */
	if (!device_is_ready(scu_dev))
		return -ENODEV;

	ipc_handle = imx_scu_get_ipc_handle(scu_dev);

	CLOCK_Init(ipc_handle);

	return 0;
}

int imx_ccm_clock_on_off(const struct device *dev, struct imx_ccm_clock *clk, bool on)
{
	struct imx_ccm_data *data;
	bool ret;

	data = dev->data;

	/* dynamically map LPCG regmap */
	if (!clk->lpcg_regmap)
		device_map(&clk->lpcg_regmap, clk->lpcg_regmap_phys,
				clk->lpcg_regmap_size, K_MEM_CACHE_NONE);

	switch (clk->state) {
		case IMX_CCM_CLOCK_STATE_INIT:
			if (on) {
				ret = CLOCK_EnableClockMapped((uint32_t *)clk->lpcg_regmap, clk->id);
				if (!ret)
					return -EINVAL;
				clk->state = IMX_CCM_CLOCK_STATE_UNGATED;
			} else {
				ret = CLOCK_DisableClockMapped((uint32_t *)clk->lpcg_regmap, clk->id);
				if (!ret)
					return -EINVAL;
				clk->state = IMX_CCM_CLOCK_STATE_GATED;
			}
			break;
		case IMX_CCM_CLOCK_STATE_GATED:
			if (on) {
				ret = CLOCK_EnableClockMapped((uint32_t *)clk->lpcg_regmap, clk->id);
				if (!ret)
					return -EINVAL;
				clk->state = IMX_CCM_CLOCK_STATE_UNGATED;
			}
			break;
		case IMX_CCM_CLOCK_STATE_UNGATED:
			if (!on) {
				ret = CLOCK_DisableClockMapped((uint32_t *)clk->lpcg_regmap, clk->id);
				if (!ret)
					return -EINVAL;
				clk->state = IMX_CCM_CLOCK_STATE_GATED;
			}
			break;
	}
	return 0;
}


int imx_ccm_clock_get_rate(const struct device *dev, struct imx_ccm_clock *clk)
{
	uint32_t rate = CLOCK_GetIpFreq(clk->id);

	if (!rate)
		return -EINVAL;

	return rate;
}

int imx_ccm_clock_set_rate(const struct device *dev, struct imx_ccm_clock *clk, uint32_t rate)
{
	int returned_rate;

	if (!rate)
		return -EINVAL;

	returned_rate = CLOCK_SetIpFreq(clk->id, rate);

	if (rate == returned_rate)
		return -EALREADY;

	if (!returned_rate)
		return -EINVAL;

	return returned_rate;
}
