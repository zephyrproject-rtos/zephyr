/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_io.h>

#include "nxp_imx_mipi_csi_priv.h"

LOG_MODULE_DECLARE(nxp_imx_mipi_csi, CONFIG_VIDEO_LOG_LEVEL);

int nxp_imx_dphy_write(struct nxp_imx_mipi_csi_data *data, const struct nxp_imx_dphy_drv_data *drv,
		       unsigned int index, uint32_t val)
{
	const struct dw_dphy_reg *reg;
	uint32_t mask;
	uint32_t tmp;

	__ASSERT_NO_MSG(drv != NULL);
	__ASSERT_NO_MSG(drv->regs != NULL);
	__ASSERT_NO_MSG(index < drv->regs_size);

	reg = &drv->regs[index];
	mask = reg->mask << reg->shift;
	val <<= reg->shift;

	tmp = sys_read32(data->dphy_regs + reg->offset);
	tmp = (tmp & ~mask) | val;
	sys_write32(tmp, data->dphy_regs + reg->offset);

	return 0;
}

static int nxp_imx_dphy_set_freqrange_by_mbps(struct nxp_imx_mipi_csi_data *data,
					      const struct nxp_imx_dphy_drv_data *drv,
					      uint64_t mbps)
{
	const struct dphy_mbps_hsfreqrange_map *value;
	const struct dphy_mbps_hsfreqrange_map *prev = NULL;

	__ASSERT_NO_MSG(drv != NULL);
	__ASSERT_NO_MSG(drv->hsfreq_tbl != NULL);

	for (value = drv->hsfreq_tbl; value->mbps; value++) {
		if (value->mbps >= mbps) {
			break;
		}
		prev = value;
	}

	if (prev && value->mbps && ((mbps - prev->mbps) <= (value->mbps - mbps))) {
		value = prev;
	}

	if (!value->mbps) {
		return -ERANGE;
	}

	data->hsfreqrange = value->hsfreqrange;

	/* cfgclkfreqrange: assume 24MHz reference (typical). */
	data->cfgclkfreqrange = 0x2;

	return 0;
}

static int nxp_imx_dphy_configure(struct nxp_imx_mipi_csi_data *data,
				  const struct nxp_imx_mipi_csi_config *cfg)
{
	uint64_t link_freq;
	uint64_t hs_clk_rate;
	uint64_t data_rate_mbps;
	int ret;

	link_freq = video_get_csi_link_freq(cfg->sensor_dev, data->csi_fmt.bpp, cfg->num_lanes);
	if ((int64_t)link_freq < 0) {
		return -EINVAL;
	}

	hs_clk_rate = link_freq * 2U;
	data_rate_mbps = hs_clk_rate / MHZ_TO_HZ;

	if (data_rate_mbps < DPHY_MIN_DATA_RATE_MBPS || data_rate_mbps > DPHY_MAX_DATA_RATE_MBPS) {
		return -ERANGE;
	}

	ret = nxp_imx_dphy_set_freqrange_by_mbps(data, cfg->dphy_drv_data, data_rate_mbps);
	if (ret) {
		return ret;
	}

	nxp_imx_dphy_write(data, cfg->dphy_drv_data, DPHY_RX_CFGCLKFREQRANGE,
			   data->cfgclkfreqrange);
	nxp_imx_dphy_write(data, cfg->dphy_drv_data, DPHY_RX_HSFREQRANGE, data->hsfreqrange);

	/* SoC-specific DPHY lane configuration */
	if (cfg->dphy_drv_data->ops && cfg->dphy_drv_data->ops->config) {
		ret = cfg->dphy_drv_data->ops->config(data, cfg);
		if (ret) {
			return ret;
		}
	}

	return 0;
}

int nxp_imx_dphy_enable(struct nxp_imx_mipi_csi_data *data,
			const struct nxp_imx_mipi_csi_config *cfg)
{
	uint32_t val;
	int ret = 0;

	if (cfg->dphy_clock_dev) {
		ret = clock_control_on(cfg->dphy_clock_dev, cfg->dphy_clock_subsys);
		if (ret) {
			goto out;
		}
	}

	/* Put PHY in reset */
	sys_write32(0, data->csis_regs + CSIS_DPHY_RSTZ);
	sys_write32(0, data->csis_regs + CSIS_DPHY_SHUTDOWNZ);

	val = sys_read32(data->csis_regs + CSIS_DPHY_TEST_CTRL0);
	val &= ~PHY_TESTCLR;
	sys_write32(val, data->csis_regs + CSIS_DPHY_TEST_CTRL0);
	k_busy_wait(1);

	val = sys_read32(data->csis_regs + CSIS_DPHY_TEST_CTRL0);
	val |= PHY_TESTCLR;
	sys_write32(val, data->csis_regs + CSIS_DPHY_TEST_CTRL0);

	sys_write32(N_LANES(cfg->num_lanes), data->csis_regs + CSIS_N_LANES);

	ret = nxp_imx_dphy_configure(data, cfg);
	if (ret) {
		goto out;
	}

	/* Release PHY from reset */
	sys_write32(1, data->csis_regs + CSIS_DPHY_SHUTDOWNZ);
	k_busy_wait(5);
	sys_write32(1, data->csis_regs + CSIS_DPHY_RSTZ);
	k_busy_wait(5);

out:
	if (ret && cfg->dphy_clock_dev) {
		(void)clock_control_off(cfg->dphy_clock_dev, cfg->dphy_clock_subsys);
	}

	return ret;
}

int nxp_imx_dphy_disable(struct nxp_imx_mipi_csi_data *data,
			 const struct nxp_imx_mipi_csi_config *cfg)
{
	int ret = 0;

	sys_write32(0, data->csis_regs + CSIS_N_LANES);
	sys_write32(0, data->csis_regs + CSIS_DPHY_RSTZ);
	sys_write32(0, data->csis_regs + CSIS_DPHY_SHUTDOWNZ);

	if (cfg->dphy_clock_dev) {
		ret = clock_control_off(cfg->dphy_clock_dev, cfg->dphy_clock_subsys);
	}

	return ret;
}
