/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx95_dphy_rx

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/sys/util.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/video/mipi_dphy.h>

LOG_MODULE_REGISTER(imx_dphy_rx, CONFIG_VIDEO_LOG_LEVEL);

/* PHY data rate limits in Mbps */
#define MHZ_TO_HZ			1000000
#define DPHY_MIN_DATA_RATE_MBPS		80
#define DPHY_MAX_DATA_RATE_MBPS		2500

#define CSIS_N_LANES			0x04
#define N_LANES(x)			FIELD_PREP(GENMASK(2, 0), ((x) - 1))
#define CSIS_DPHY_SHUTDOWNZ		0x40
#define PHY_SHUTDOWNZ			BIT(0)
#define CSIS_DPHY_RSTZ			0x44
#define PHY_RSTZ			BIT(0)
#define CSIS_DPHY_STOPSTATE		0x4C
#define CSIS_DPHY_TEST_CTRL0		0x50
#define PHY_TESTCLR			BIT(0)
#define PHY_TESTCLK			BIT(1)
#define CSIS_DPHY_TEST_CTRL1		0x54

/* DPHY CSR registers */
#define CSR_PHY_MODE_CTRL		0x00
#define CSR_PHY_FREQ_CTRL		0x04
#define CSR_PHY_TEST_MODE_CTRL		0x08

/* PHY register definitions */
enum dphy_reg_id {
	DPHY_RX_CFGCLKFREQRANGE = 0,
	DPHY_RX_HSFREQRANGE,
	DPHY_RX_DATA_LANE_EN,
	DPHY_RX_DATA_LANE_BASEDIR,
	DPHY_RX_DATA_LANE_FORCERXMODE,
	DPHY_RX_ENABLE_CLK_EXT,
	DPHY_RX_PHY_ENABLE_BYP,
};

struct dw_dphy_reg {
	uint16_t offset;
	uint8_t mask;
	uint8_t shift;
};

#define PHY_REG(_offset, _width, _shift) \
	{ .offset = _offset, .mask = BIT(_width) - 1, .shift = _shift, }

struct dphy_mbps_hsfreqrange_map {
	uint16_t mbps;
	uint16_t hsfreqrange;
};

/* iMX95 DPHY register mapping */
static const struct dw_dphy_reg imx95_dphy_regs[] = {
	[DPHY_RX_CFGCLKFREQRANGE] = PHY_REG(CSR_PHY_FREQ_CTRL, 6, 0),
	[DPHY_RX_HSFREQRANGE] = PHY_REG(CSR_PHY_FREQ_CTRL, 7, 16),
	[DPHY_RX_DATA_LANE_EN] = PHY_REG(CSR_PHY_MODE_CTRL, 4, 4),
	[DPHY_RX_DATA_LANE_BASEDIR] = PHY_REG(CSR_PHY_TEST_MODE_CTRL, 1, 0),
	[DPHY_RX_DATA_LANE_FORCERXMODE] = PHY_REG(CSR_PHY_TEST_MODE_CTRL, 4, 8),
	[DPHY_RX_ENABLE_CLK_EXT] = PHY_REG(CSR_PHY_TEST_MODE_CTRL, 1, 12),
	[DPHY_RX_PHY_ENABLE_BYP] = PHY_REG(CSR_PHY_TEST_MODE_CTRL, 1, 14),
};

/* Frequency range mapping table */
static const struct dphy_mbps_hsfreqrange_map hsfreqrange_table[] = {
	{ .mbps = 80,   .hsfreqrange = 0x00 },
	{ .mbps = 90,   .hsfreqrange = 0x10 },
	{ .mbps = 100,  .hsfreqrange = 0x20 },
	{ .mbps = 110,  .hsfreqrange = 0x30 },
	{ .mbps = 120,  .hsfreqrange = 0x01 },
	{ .mbps = 130,  .hsfreqrange = 0x11 },
	{ .mbps = 140,  .hsfreqrange = 0x21 },
	{ .mbps = 150,  .hsfreqrange = 0x31 },
	{ .mbps = 160,  .hsfreqrange = 0x02 },
	{ .mbps = 170,  .hsfreqrange = 0x12 },
	{ .mbps = 180,  .hsfreqrange = 0x22 },
	{ .mbps = 190,  .hsfreqrange = 0x32 },
	{ .mbps = 205,  .hsfreqrange = 0x03 },
	{ .mbps = 220,  .hsfreqrange = 0x13 },
	{ .mbps = 235,  .hsfreqrange = 0x23 },
	{ .mbps = 250,  .hsfreqrange = 0x33 },
	{ .mbps = 275,  .hsfreqrange = 0x04 },
	{ .mbps = 300,  .hsfreqrange = 0x14 },
	{ .mbps = 325,  .hsfreqrange = 0x25 },
	{ .mbps = 350,  .hsfreqrange = 0x35 },
	{ .mbps = 400,  .hsfreqrange = 0x05 },
	{ .mbps = 450,  .hsfreqrange = 0x16 },
	{ .mbps = 500,  .hsfreqrange = 0x26 },
	{ .mbps = 550,  .hsfreqrange = 0x37 },
	{ .mbps = 600,  .hsfreqrange = 0x07 },
	{ .mbps = 650,  .hsfreqrange = 0x18 },
	{ .mbps = 700,  .hsfreqrange = 0x28 },
	{ .mbps = 750,  .hsfreqrange = 0x39 },
	{ .mbps = 800,  .hsfreqrange = 0x09 },
	{ .mbps = 850,  .hsfreqrange = 0x19 },
	{ .mbps = 900,  .hsfreqrange = 0x29 },
	{ .mbps = 950,  .hsfreqrange = 0x3a },
	{ .mbps = 1000, .hsfreqrange = 0x0a },
	{ .mbps = 1050, .hsfreqrange = 0x1a },
	{ .mbps = 1100, .hsfreqrange = 0x2a },
	{ .mbps = 1150, .hsfreqrange = 0x3b },
	{ .mbps = 1200, .hsfreqrange = 0x0b },
	{ .mbps = 1250, .hsfreqrange = 0x1b },
	{ .mbps = 1300, .hsfreqrange = 0x2b },
	{ .mbps = 1350, .hsfreqrange = 0x3c },
	{ .mbps = 1400, .hsfreqrange = 0x0c },
	{ .mbps = 1450, .hsfreqrange = 0x1c },
	{ .mbps = 1500, .hsfreqrange = 0x2c },
	{ .mbps = 1550, .hsfreqrange = 0x3d },
	{ .mbps = 1600, .hsfreqrange = 0x0d },
	{ .mbps = 1650, .hsfreqrange = 0x1d },
	{ .mbps = 1700, .hsfreqrange = 0x2e },
	{ .mbps = 1750, .hsfreqrange = 0x3e },
	{ .mbps = 1800, .hsfreqrange = 0x0e },
	{ .mbps = 1850, .hsfreqrange = 0x1e },
	{ .mbps = 1900, .hsfreqrange = 0x1f },
	{ .mbps = 1950, .hsfreqrange = 0x3f },
	{ .mbps = 2000, .hsfreqrange = 0x0f },
	{ .mbps = 2050, .hsfreqrange = 0x40 },
	{ .mbps = 2100, .hsfreqrange = 0x41 },
	{ .mbps = 2150, .hsfreqrange = 0x42 },
	{ .mbps = 2200, .hsfreqrange = 0x43 },
	{ .mbps = 2250, .hsfreqrange = 0x44 },
	{ .mbps = 2300, .hsfreqrange = 0x45 },
	{ .mbps = 2350, .hsfreqrange = 0x46 },
	{ .mbps = 2400, .hsfreqrange = 0x47 },
	{ .mbps = 2450, .hsfreqrange = 0x48 },
	{ .mbps = 2500, .hsfreqrange = 0x49 },
	{ .mbps = 0,    .hsfreqrange = 0x00 }, /* End marker */
};

struct imx_dphy_config {
	mem_addr_t dphy_base;
	mem_addr_t csis_base;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	uint32_t max_lanes;
	uint32_t max_data_rate;
};

struct imx_dphy_data {
	mem_addr_t dphy_regs;
	mem_addr_t csis_regs;

	struct k_mutex lock;

	struct phy_configure_opts_mipi_dphy config;
	uint16_t hsfreqrange;
	uint16_t cfgclkfreqrange;

	bool configured;
	bool powered_on;
};

/* Register access helper functions */
static inline void csis_write(struct imx_dphy_data *data,
			      unsigned int offset, uint32_t val)
{
	sys_write32(val, data->csis_regs + offset);
}

static inline uint32_t csis_read(struct imx_dphy_data *data,
				 unsigned int offset)
{
	return sys_read32(data->csis_regs + offset);
}

static int dphy_write(struct imx_dphy_data *data, unsigned int index,
		      uint32_t val)
{
	const struct dw_dphy_reg *reg;
	uint32_t mask;
	uint32_t tmp;

	if (index >= ARRAY_SIZE(imx95_dphy_regs)) {
		LOG_ERR("Index out of range");
		return -EINVAL;
	}

	reg = &imx95_dphy_regs[index];
	mask = reg->mask << reg->shift;
	val <<= reg->shift;

	tmp = sys_read32(data->dphy_regs + reg->offset);
	tmp = (tmp & ~mask) | val;
	sys_write32(tmp, data->dphy_regs + reg->offset);

	return 0;
}

static int set_freqrange_by_mbps(struct imx_dphy_data *data, uint64_t mbps)
{
	const struct dphy_mbps_hsfreqrange_map *value;
	const struct dphy_mbps_hsfreqrange_map *prev_value = NULL;

	for (value = hsfreqrange_table; value->mbps; value++) {
		if (value->mbps >= mbps) {
			break;
		}
		prev_value = value;
	}

	if (prev_value && ((mbps - prev_value->mbps) <= (value->mbps - mbps))) {
		value = prev_value;
	}

	if (!value->mbps) {
		LOG_ERR("Unsupported PHY speed (%llu Mbps)", mbps);
		return -ERANGE;
	}

	data->hsfreqrange = value->hsfreqrange;

	LOG_DBG("Set hsfreqrange=0x%02x for %llu Mbps", data->hsfreqrange, mbps);

	return 0;
}

static void imx_dphy_hw_config(struct imx_dphy_data *data)
{
	struct phy_configure_opts_mipi_dphy *config = &data->config;
	uint32_t active_lanes = GENMASK(config->lanes - 1, 0);

	/* Configure the PHY frequency range */
	dphy_write(data, DPHY_RX_CFGCLKFREQRANGE, data->cfgclkfreqrange);
	dphy_write(data, DPHY_RX_HSFREQRANGE, data->hsfreqrange);

	/* Configure data lanes */
	dphy_write(data, DPHY_RX_DATA_LANE_BASEDIR, 1);
	k_busy_wait(1);

	dphy_write(data, DPHY_RX_DATA_LANE_FORCERXMODE, active_lanes);
	k_busy_wait(1);

	dphy_write(data, DPHY_RX_DATA_LANE_EN, active_lanes);
	dphy_write(data, DPHY_RX_DATA_LANE_FORCERXMODE, 0);
	dphy_write(data, DPHY_RX_ENABLE_CLK_EXT, 1);
	dphy_write(data, DPHY_RX_PHY_ENABLE_BYP, 1);

	LOG_DBG("PHY configured: lanes=%d, hsfreq=0x%02x", config->lanes, data->hsfreqrange);
}

static int imx_dphy_configure(const struct device *dev, void *opts)
{
	struct imx_dphy_data *data = dev->data;
	const struct imx_dphy_config *config = dev->config;
	struct phy_configure_opts_mipi_dphy *dphy_opts = opts;
	uint64_t data_rate_mbps;
	int ret;

	if (!dphy_opts) {
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	/* Validate lane count */
	if (dphy_opts->lanes == 0 || dphy_opts->lanes > config->max_lanes) {
		LOG_ERR("Invalid lane count %d (max %d)",
			dphy_opts->lanes, config->max_lanes);
		k_mutex_unlock(&data->lock);
		return -EINVAL;
	}

	/* Validate data rate */
	data_rate_mbps = dphy_opts->hs_clk_rate / MHZ_TO_HZ;
	if (data_rate_mbps < DPHY_MIN_DATA_RATE_MBPS ||
	    data_rate_mbps > config->max_data_rate) {
		LOG_ERR("Data rate %llu Mbps out of range [%d, %d]",
			data_rate_mbps, DPHY_MIN_DATA_RATE_MBPS,
			config->max_data_rate);
		k_mutex_unlock(&data->lock);
		return -EINVAL;
	}

	LOG_DBG("Configuring PHY: lanes=%d, data_rate=%llu Mbps",
		dphy_opts->lanes, data_rate_mbps);

	/* Set frequency range */
	ret = set_freqrange_by_mbps(data, data_rate_mbps);
	if (ret < 0) {
		k_mutex_unlock(&data->lock);
		return ret;
	}

	/* Save configuration */
	data->config = *dphy_opts;
	data->configured = true;

	k_mutex_unlock(&data->lock);
	return 0;
}

static int imx_dphy_init(const struct device *dev)
{
	struct imx_dphy_data *data = dev->data;
	const struct imx_dphy_config *config = dev->config;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	/* Enable clock */
	if (config->clock_dev) {
		ret = clock_control_on(config->clock_dev, config->clock_subsys);
		if (ret) {
			LOG_ERR("Failed to enable clock: %d", ret);
			k_mutex_unlock(&data->lock);
			return ret;
		}
	}

	k_mutex_unlock(&data->lock);

	LOG_DBG("PHY initialized");
	return 0;
}

static int imx_dphy_power_on(const struct device *dev)
{
	struct imx_dphy_data *data = dev->data;
	struct phy_configure_opts_mipi_dphy *config = &data->config;
	uint32_t val;

	if (!data->configured) {
		LOG_ERR("PHY not configured");
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	/* Release Synopsys DPHY test codes from reset */
	csis_write(data, CSIS_DPHY_RSTZ, 0x0);
	csis_write(data, CSIS_DPHY_SHUTDOWNZ, 0x0);

	val = csis_read(data, CSIS_DPHY_TEST_CTRL0);
	val &= ~PHY_TESTCLR;
	csis_write(data, CSIS_DPHY_TEST_CTRL0, val);

	/* Wait for at least 15ns */
	k_busy_wait(1);

	/* Set testclr=1'b1 */
	val = csis_read(data, CSIS_DPHY_TEST_CTRL0);
	val |= PHY_TESTCLR;
	csis_write(data, CSIS_DPHY_TEST_CTRL0, val);

	/* Configure lane count */
	csis_write(data, CSIS_N_LANES, N_LANES(config->lanes));

	/* Apply PHY configuration */
	imx_dphy_hw_config(data);

	/* Release PHY from reset */
	csis_write(data, CSIS_DPHY_SHUTDOWNZ, 0x1);
	k_busy_wait(5);
	csis_write(data, CSIS_DPHY_RSTZ, 0x1);
	k_busy_wait(5);

	data->powered_on = true;

	k_mutex_unlock(&data->lock);

	LOG_DBG("PHY powered on (lanes=%d)", config->lanes);
	return 0;
}

static int imx_dphy_power_off(const struct device *dev)
{
	struct imx_dphy_data *data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);

	/* Put PHY in reset state */
	csis_write(data, CSIS_N_LANES, 0);
	csis_write(data, CSIS_DPHY_RSTZ, 0x0);
	csis_write(data, CSIS_DPHY_SHUTDOWNZ, 0x0);

	data->powered_on = false;

	k_mutex_unlock(&data->lock);

	LOG_DBG("PHY powered off");
	return 0;
}

static int imx_dphy_exit(const struct device *dev)
{
	struct imx_dphy_data *data = dev->data;
	const struct imx_dphy_config *config = dev->config;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	/* Ensure PHY is powered off */
	if (data->powered_on) {
		csis_write(data, CSIS_DPHY_RSTZ, 0);
		csis_write(data, CSIS_DPHY_SHUTDOWNZ, 0);
		data->powered_on = false;
	}

	/* Disable clock */
	if (config->clock_dev) {
		ret = clock_control_off(config->clock_dev, config->clock_subsys);
		if (ret) {
			LOG_WRN("Failed to disable clock: %d", ret);
		}
	}

	k_mutex_unlock(&data->lock);

	LOG_DBG("PHY exited");
	return 0;
}

static const struct phy_driver_api imx_dphy_driver_api = {
	.configure = imx_dphy_configure,
	.init = imx_dphy_init,
	.power_on = imx_dphy_power_on,
	.power_off = imx_dphy_power_off,
	.exit = imx_dphy_exit,
};

static int imx_dphy_device_init(const struct device *dev)
{
	struct imx_dphy_data *data = dev->data;
	const struct imx_dphy_config *config = dev->config;

	data->dphy_regs = config->dphy_base;
	data->csis_regs = config->csis_base;

	/* cfgclkfreqrange[5:0] = round[(cfg_clk(MHz) - 17) * 4]
	 * Assuming typical 24MHz config clock.
	 */
	data->cfgclkfreqrange = 0x1C;

	data->configured = false;
	data->powered_on = false;

	k_mutex_init(&data->lock);

	return 0;
}

#define IMX_DPHY_DEVICE_INIT(inst)						\
	static struct imx_dphy_data imx_dphy_data_##inst;			\
										\
	static const struct imx_dphy_config imx_dphy_config_##inst = {		\
		.dphy_base = DT_INST_REG_ADDR_BY_NAME(inst, dphy),		\
		.csis_base = DT_INST_REG_ADDR_BY_NAME(inst, csis),		\
		.clock_dev = DEVICE_DT_GET_OR_NULL(DT_INST_CLOCKS_CTLR(inst)),	\
		.clock_subsys = (clock_control_subsys_t)			\
				DT_INST_CLOCKS_CELL_BY_IDX(inst, 0, name),	\
		.max_lanes = 4,							\
		.max_data_rate = DPHY_MAX_DATA_RATE_MBPS,			\
	};									\
										\
	DEVICE_DT_INST_DEFINE(inst,						\
			      imx_dphy_device_init,				\
			      NULL,						\
			      &imx_dphy_data_##inst,				\
			      &imx_dphy_config_##inst,				\
			      POST_KERNEL,					\
			      CONFIG_IMX_DPHY_INIT_PRIORITY,			\
			      &imx_dphy_driver_api);

DT_INST_FOREACH_STATUS_OKAY(IMX_DPHY_DEVICE_INIT)
