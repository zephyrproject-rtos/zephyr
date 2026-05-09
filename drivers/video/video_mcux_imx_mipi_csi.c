/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx_mipi_csi

#include <zephyr/device.h>
#include <zephyr/devicetree/port-endpoint.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/video.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#include "video_device.h"

LOG_MODULE_REGISTER(nxp_imx_mipi_csi, CONFIG_VIDEO_LOG_LEVEL);

#define MIPI_CSI2RX_VERSION                 0x00
#define MIPI_CSI2RX_N_LANES                 0x04
#define MIPI_CSI2RX_HOST_RESETN             0x08
#define MIPI_CSI2RX_DPHY_STOPSTATE          0x4C
#define MIPI_CSI2RX_N_LANES_MASK            GENMASK(2, 0)
#define MIPI_CSI2RX_HOST_RESETN_ENABLE      BIT(0)
#define MIPI_CSI2RX_DPHY_STOPSTATE_CLK_LANE BIT(16)

#define CSI_RESET_DELAY_US          10
#define CSI_STOPSTATE_TIMEOUT_US    50000
#define CSI_STOPSTATE_POLL_DELAY_US 50

#define CSIS_N_LANES         0x04
#define CSIS_DPHY_SHUTDOWNZ  0x40
#define CSIS_DPHY_RSTZ       0x44
#define CSIS_DPHY_STOPSTATE  0x4C
#define CSIS_DPHY_TEST_CTRL0 0x50
#define CSIS_DPHY_TEST_CTRL1 0x54

#define PHY_TESTCLR BIT(0)

#define N_LANES(x) FIELD_PREP(GENMASK(2, 0), ((x) - 1))

#define CSR_PHY_MODE_CTRL      0x00
#define CSR_PHY_FREQ_CTRL      0x04
#define CSR_PHY_TEST_MODE_CTRL 0x08

#define MHZ_TO_HZ               1000000ULL
#define DPHY_MIN_DATA_RATE_MBPS 80
#define DPHY_MAX_DATA_RATE_MBPS 2500

struct nxp_imx_dphy_field {
	uint16_t offset;
	uint32_t mask;
};

struct nxp_imx_dphy_fields {
	struct nxp_imx_dphy_field cfgclkfreqrange;
	struct nxp_imx_dphy_field hsfreqrange;
	struct nxp_imx_dphy_field data_lane_en;
	struct nxp_imx_dphy_field data_lane_basedir;
	struct nxp_imx_dphy_field data_lane_forcerxmode;
	struct nxp_imx_dphy_field enable_clk_ext;
	struct nxp_imx_dphy_field phy_enable_byp;
};

struct dphy_mbps_hsfreqrange_map {
	uint16_t mbps;
	uint16_t hsfreqrange;
};

struct nxp_imx_mipi_csi_data {
	mem_addr_t csis_regs;
	mem_addr_t dphy_regs;

	struct k_mutex lock;

	struct video_format fmt;
	struct video_frmival frmival;

	uint16_t hsfreqrange;
	uint16_t cfgclkfreqrange;
	bool frmival_set;

	bool streaming;
};

struct nxp_imx_mipi_csi_config {
	mem_addr_t csis_base;
	mem_addr_t dphy_base;
	const struct device *dphy_clock_dev;
	clock_control_subsys_t dphy_clock_subsys;
	const struct device *source_dev;
	uint8_t num_lanes;

	/* Selected PHY backend (based on phys node compatible) */
	const struct nxp_imx_dphy_drv_data *dphy_drv_data;
};

struct nxp_imx_dphy_config_ops {
	/**
	 * SoC/PHY-specific DPHY CSR configuration after common freqrange setup.
	 */
	int (*config)(const struct device *dev);
};

struct nxp_imx_dphy_drv_data {
	const struct nxp_imx_dphy_fields *fields;
	const struct dphy_mbps_hsfreqrange_map *hsfreq_tbl;
	uint8_t max_lanes;
	uint32_t max_data_rate_mbps;
	const struct nxp_imx_dphy_config_ops *ops;
};


static void nxp_imx_csis_host_reset(const struct device *dev)
{
	struct nxp_imx_mipi_csi_data *data = dev->data;

	sys_write32(0, data->csis_regs + MIPI_CSI2RX_HOST_RESETN);
	k_usleep(CSI_RESET_DELAY_US);
	sys_write32(MIPI_CSI2RX_HOST_RESETN_ENABLE, data->csis_regs + MIPI_CSI2RX_HOST_RESETN);
}

static int nxp_imx_csis_wait_stopstate(const struct device *dev, uint8_t lanes)
{
	struct nxp_imx_mipi_csi_data *data = dev->data;
	uint32_t mask = MIPI_CSI2RX_DPHY_STOPSTATE_CLK_LANE | GENMASK(lanes - 1, 0);
	k_timepoint_t end = sys_timepoint_calc(K_USEC(CSI_STOPSTATE_TIMEOUT_US));

	while (!sys_timepoint_expired(end)) {
		uint32_t val = sys_read32(data->csis_regs + MIPI_CSI2RX_DPHY_STOPSTATE);

		if ((val & mask) == mask) {
			LOG_DBG("DPHY lanes in stop state (0x%08x)", val);
			return 0;
		}

		k_usleep(CSI_STOPSTATE_POLL_DELAY_US);
	}

	return -ETIMEDOUT;
}

static inline void nxp_imx_dphy_update(const struct device *dev,
				       const struct nxp_imx_dphy_field *f,
				       uint32_t val)
{
	struct nxp_imx_mipi_csi_data *data = dev->data;
	uint32_t tmp;

	__ASSERT_NO_MSG(f != NULL);

	tmp = sys_read32(data->dphy_regs + f->offset);
	tmp = (tmp & ~f->mask) | FIELD_PREP(f->mask, val);
	sys_write32(tmp, data->dphy_regs + f->offset);
}

static int nxp_imx_dphy_set_freqrange_by_mbps(const struct device *dev,
				     const struct nxp_imx_dphy_drv_data *drv,
				     uint64_t mbps)
{
	struct nxp_imx_mipi_csi_data *data = dev->data;
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

	if ((prev != NULL) && (value->mbps != 0U) &&
			((mbps - prev->mbps) <= (value->mbps - mbps))) {
		value = prev;
	}

	if (value->mbps == 0U) {
		return -ERANGE;
	}

	data->hsfreqrange = value->hsfreqrange;

	/* cfgclkfreqrange: assume 24MHz reference (typical). */
	data->cfgclkfreqrange = 0x2;

	return 0;
}

static int nxp_imx_dphy_configure(const struct device *dev)
{
	struct nxp_imx_mipi_csi_data *data = dev->data;
	const struct nxp_imx_mipi_csi_config *cfg = dev->config;
	const struct nxp_imx_dphy_fields *field = cfg->dphy_drv_data->fields;
	unsigned int bpp;
	uint64_t link_freq;
	uint64_t hs_clk_rate;
	uint64_t data_rate_mbps;
	int ret;

	bpp = video_bits_per_pixel(data->fmt.pixelformat);
	if (bpp == 0U) {
		return -ENOTSUP;
	}

	link_freq = video_get_csi_link_freq(cfg->source_dev, bpp, cfg->num_lanes);
	if ((int64_t)link_freq < 0) {
		return -EINVAL;
	}

	hs_clk_rate = link_freq * 2U;
	data_rate_mbps = hs_clk_rate / MHZ_TO_HZ;

	if (!IN_RANGE(data_rate_mbps, DPHY_MIN_DATA_RATE_MBPS, DPHY_MAX_DATA_RATE_MBPS)) {
		return -ERANGE;
	}

	ret = nxp_imx_dphy_set_freqrange_by_mbps(dev, cfg->dphy_drv_data, data_rate_mbps);
	if (ret < 0) {
		return ret;
	}

	nxp_imx_dphy_update(dev, &field->cfgclkfreqrange, data->cfgclkfreqrange);
	nxp_imx_dphy_update(dev, &field->hsfreqrange, data->hsfreqrange);

	/* SoC-specific DPHY lane configuration */
	if ((cfg->dphy_drv_data->ops != NULL) && (cfg->dphy_drv_data->ops->config != NULL)) {
		ret = cfg->dphy_drv_data->ops->config(dev);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int nxp_imx_dphy_enable(const struct device *dev)
{
	struct nxp_imx_mipi_csi_data *data = dev->data;
	const struct nxp_imx_mipi_csi_config *cfg = dev->config;
	uint32_t val;
	int ret;

	if (cfg->dphy_clock_dev != NULL) {
		ret = clock_control_on(cfg->dphy_clock_dev, cfg->dphy_clock_subsys);
		if (ret < 0) {
			return ret;
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

	ret = nxp_imx_dphy_configure(dev);
	if (ret < 0) {
		goto err;
	}

	/* Release PHY from reset */
	sys_write32(1, data->csis_regs + CSIS_DPHY_SHUTDOWNZ);
	k_busy_wait(5);
	sys_write32(1, data->csis_regs + CSIS_DPHY_RSTZ);
	k_busy_wait(5);

	return 0;

err:
	if (cfg->dphy_clock_dev != NULL) {
		(void)clock_control_off(cfg->dphy_clock_dev, cfg->dphy_clock_subsys);
	}

	return ret;
}

static int nxp_imx_dphy_disable(const struct device *dev)
{
	struct nxp_imx_mipi_csi_data *data = dev->data;
	const struct nxp_imx_mipi_csi_config *cfg = dev->config;
	int ret = 0;

	sys_write32(0, data->csis_regs + CSIS_N_LANES);
	sys_write32(0, data->csis_regs + CSIS_DPHY_RSTZ);
	sys_write32(0, data->csis_regs + CSIS_DPHY_SHUTDOWNZ);

	if (cfg->dphy_clock_dev != NULL) {
		ret = clock_control_off(cfg->dphy_clock_dev, cfg->dphy_clock_subsys);
	}

	return ret;
}

/* i.MX95 DPHY backend */
static const struct nxp_imx_dphy_fields imx95_dphy_fields = {
	.cfgclkfreqrange = { .offset = CSR_PHY_FREQ_CTRL, .mask = GENMASK(5, 0) },
	.hsfreqrange = { .offset = CSR_PHY_FREQ_CTRL, .mask = GENMASK(22, 16) },
	.data_lane_en = { .offset = CSR_PHY_MODE_CTRL, .mask = GENMASK(7, 4) },
	.data_lane_basedir = { .offset = CSR_PHY_TEST_MODE_CTRL, .mask = BIT(0) },
	.data_lane_forcerxmode = { .offset = CSR_PHY_TEST_MODE_CTRL, .mask = GENMASK(11, 8) },
	.enable_clk_ext = { .offset = CSR_PHY_TEST_MODE_CTRL, .mask = BIT(12) },
	.phy_enable_byp = { .offset = CSR_PHY_TEST_MODE_CTRL, .mask = BIT(14) },
};

static const struct dphy_mbps_hsfreqrange_map imx9_hsfreqrange_table[] = {
	{.mbps = 80, .hsfreqrange = 0x00},   {.mbps = 90, .hsfreqrange = 0x10},
	{.mbps = 100, .hsfreqrange = 0x20},  {.mbps = 110, .hsfreqrange = 0x30},
	{.mbps = 120, .hsfreqrange = 0x01},  {.mbps = 130, .hsfreqrange = 0x11},
	{.mbps = 140, .hsfreqrange = 0x21},  {.mbps = 150, .hsfreqrange = 0x31},
	{.mbps = 160, .hsfreqrange = 0x02},  {.mbps = 170, .hsfreqrange = 0x12},
	{.mbps = 180, .hsfreqrange = 0x22},  {.mbps = 190, .hsfreqrange = 0x32},
	{.mbps = 205, .hsfreqrange = 0x03},  {.mbps = 220, .hsfreqrange = 0x13},
	{.mbps = 235, .hsfreqrange = 0x23},  {.mbps = 250, .hsfreqrange = 0x33},
	{.mbps = 275, .hsfreqrange = 0x04},  {.mbps = 300, .hsfreqrange = 0x14},
	{.mbps = 325, .hsfreqrange = 0x25},  {.mbps = 350, .hsfreqrange = 0x35},
	{.mbps = 400, .hsfreqrange = 0x05},  {.mbps = 450, .hsfreqrange = 0x16},
	{.mbps = 500, .hsfreqrange = 0x26},  {.mbps = 550, .hsfreqrange = 0x37},
	{.mbps = 600, .hsfreqrange = 0x07},  {.mbps = 650, .hsfreqrange = 0x18},
	{.mbps = 700, .hsfreqrange = 0x28},  {.mbps = 750, .hsfreqrange = 0x39},
	{.mbps = 800, .hsfreqrange = 0x09},  {.mbps = 850, .hsfreqrange = 0x19},
	{.mbps = 900, .hsfreqrange = 0x29},  {.mbps = 950, .hsfreqrange = 0x3a},
	{.mbps = 1000, .hsfreqrange = 0x0a}, {.mbps = 1050, .hsfreqrange = 0x1a},
	{.mbps = 1100, .hsfreqrange = 0x2a}, {.mbps = 1150, .hsfreqrange = 0x3b},
	{.mbps = 1200, .hsfreqrange = 0x0b}, {.mbps = 1250, .hsfreqrange = 0x1b},
	{.mbps = 1300, .hsfreqrange = 0x2b}, {.mbps = 1350, .hsfreqrange = 0x3c},
	{.mbps = 1400, .hsfreqrange = 0x0c}, {.mbps = 1450, .hsfreqrange = 0x1c},
	{.mbps = 1500, .hsfreqrange = 0x2c}, {.mbps = 1550, .hsfreqrange = 0x3d},
	{.mbps = 1600, .hsfreqrange = 0x0d}, {.mbps = 1650, .hsfreqrange = 0x1d},
	{.mbps = 1700, .hsfreqrange = 0x2e}, {.mbps = 1750, .hsfreqrange = 0x3e},
	{.mbps = 1800, .hsfreqrange = 0x0e}, {.mbps = 1850, .hsfreqrange = 0x1e},
	{.mbps = 1900, .hsfreqrange = 0x1f}, {.mbps = 1950, .hsfreqrange = 0x3f},
	{.mbps = 2000, .hsfreqrange = 0x0f}, {.mbps = 2050, .hsfreqrange = 0x40},
	{.mbps = 2100, .hsfreqrange = 0x41}, {.mbps = 2150, .hsfreqrange = 0x42},
	{.mbps = 2200, .hsfreqrange = 0x43}, {.mbps = 2250, .hsfreqrange = 0x44},
	{.mbps = 2300, .hsfreqrange = 0x45}, {.mbps = 2350, .hsfreqrange = 0x46},
	{.mbps = 2400, .hsfreqrange = 0x47}, {.mbps = 2450, .hsfreqrange = 0x48},
	{.mbps = 2500, .hsfreqrange = 0x49}, {/* sentinel */},
};

static int imx95_dphy_soc_config(const struct device *dev)
{
	const struct nxp_imx_mipi_csi_config *cfg = dev->config;
	const struct nxp_imx_dphy_fields *field = cfg->dphy_drv_data->fields;
	uint8_t active_lanes = BIT(cfg->num_lanes) - 1;

	nxp_imx_dphy_update(dev, &field->data_lane_basedir, 1);
	k_busy_wait(1);

	nxp_imx_dphy_update(dev, &field->data_lane_forcerxmode, active_lanes);
	k_busy_wait(1);

	nxp_imx_dphy_update(dev, &field->data_lane_en, active_lanes);
	nxp_imx_dphy_update(dev, &field->data_lane_forcerxmode, 0);
	nxp_imx_dphy_update(dev, &field->enable_clk_ext, 1);
	nxp_imx_dphy_update(dev, &field->phy_enable_byp, 1);

	return 0;
}

static const struct nxp_imx_dphy_config_ops imx95_cfg_ops = {
	.config = imx95_dphy_soc_config,
};

static const struct nxp_imx_dphy_drv_data nxp_imx95_dphy_drv_data = {
	.fields = &imx95_dphy_fields,
	.hsfreq_tbl = imx9_hsfreqrange_table,
	.max_lanes = 4,
	.max_data_rate_mbps = DPHY_MAX_DATA_RATE_MBPS,
	.ops = &imx95_cfg_ops,
};

/* Video api */
static int nxp_imx_csi_set_fmt(const struct device *dev, struct video_format *fmt)
{
	struct nxp_imx_mipi_csi_data *data = dev->data;
	const struct nxp_imx_mipi_csi_config *cfg = dev->config;
	int ret = 0;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (data->streaming) {
		ret = -EBUSY;
		goto out;
	}

	if (video_mipi_data_type(fmt->pixelformat) == VIDEO_MIPI_CSI2_DT_NULL) {
		ret = -ENOTSUP;
		goto out;
	}

	ret = video_set_format(cfg->source_dev, fmt);
	if (ret < 0) {
		goto out;
	}

	data->fmt = *fmt;

out:
	k_mutex_unlock(&data->lock);
	return ret;
}

static int nxp_imx_csi_get_fmt(const struct device *dev, struct video_format *fmt)
{
	const struct nxp_imx_mipi_csi_config *cfg = dev->config;

	return video_get_format(cfg->source_dev, fmt);
}

static int nxp_imx_csi_set_stream(const struct device *dev, bool enable, enum video_buf_type type)
{
	struct nxp_imx_mipi_csi_data *data = dev->data;
	const struct nxp_imx_mipi_csi_config *cfg = dev->config;
	int ret = 0;

	ARG_UNUSED(type);

	k_mutex_lock(&data->lock, K_FOREVER);

	if (enable) {
		if (data->streaming) {
			goto out;
		}

		ret = nxp_imx_dphy_enable(dev);
		if (ret < 0) {
			goto out;
		}

		nxp_imx_csis_host_reset(dev);

		ret = nxp_imx_csis_wait_stopstate(dev, cfg->num_lanes);
		if (ret < 0) {
			(void)nxp_imx_dphy_disable(dev);
			goto out;
		}

		ret = video_stream_start(cfg->source_dev, VIDEO_BUF_TYPE_OUTPUT);
		if (ret < 0) {
			(void)nxp_imx_dphy_disable(dev);
			goto out;
		}

		data->streaming = true;
	} else {
		if (!data->streaming) {
			goto out;
		}

		ret = video_stream_stop(cfg->source_dev, VIDEO_BUF_TYPE_OUTPUT);
		if (ret < 0) {
			goto out;
		}

		ret = nxp_imx_dphy_disable(dev);
		if (ret < 0) {
			goto out;
		}

		data->streaming = false;
	}

out:
	k_mutex_unlock(&data->lock);

	return ret;
}

static int nxp_imx_csi_get_caps(const struct device *dev, struct video_caps *caps)
{
	const struct nxp_imx_mipi_csi_config *cfg = dev->config;

	return video_get_caps(cfg->source_dev, caps);
}

static int nxp_imx_csi_set_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct nxp_imx_mipi_csi_config *cfg = dev->config;

	return video_set_frmival(cfg->source_dev, frmival);
}

static int nxp_imx_csi_get_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct nxp_imx_mipi_csi_config *cfg = dev->config;

	return video_get_frmival(cfg->source_dev, frmival);
}

static int nxp_imx_csi_enum_frmival(const struct device *dev, struct video_frmival_enum *fie)
{
	const struct nxp_imx_mipi_csi_config *cfg = dev->config;

	return video_enum_frmival(cfg->source_dev, fie);
}

static DEVICE_API(video, nxp_imx_mipi_csi_api) = {
	.set_format = nxp_imx_csi_set_fmt,
	.get_format = nxp_imx_csi_get_fmt,
	.set_stream = nxp_imx_csi_set_stream,
	.get_caps = nxp_imx_csi_get_caps,
	.set_frmival = nxp_imx_csi_set_frmival,
	.get_frmival = nxp_imx_csi_get_frmival,
	.enum_frmival = nxp_imx_csi_enum_frmival,
};

static int nxp_imx_mipi_csi_init(const struct device *dev)
{
	struct nxp_imx_mipi_csi_data *d = dev->data;
	const struct nxp_imx_mipi_csi_config *cfg = dev->config;

	d->csis_regs = cfg->csis_base;
	d->dphy_regs = cfg->dphy_base;

	k_mutex_init(&d->lock);

	if (cfg->dphy_drv_data == NULL) {
		LOG_ERR("No supported DPHY backend for this SoC");
		return -ENODEV;
	}

	nxp_imx_csis_host_reset(dev);
	LOG_DBG("i.MX CSI-2 version: 0x%08x, lanes=%u",
		sys_read32(d->csis_regs + MIPI_CSI2RX_VERSION), cfg->num_lanes);

	return 0;
}

#define CSI_SOURCE_DEV(inst)                                                                       \
	DEVICE_DT_GET(DT_NODE_REMOTE_DEVICE(DT_INST_ENDPOINT_BY_ID(inst, 0, 0)))

#define DPHY_NODE(inst) DT_INST_PHANDLE(inst, phys)

#define NXP_IMX_DPHY_DRV_DATA(inst, compat, drv_data)                                              \
	COND_CODE_1(DT_NODE_HAS_COMPAT(DPHY_NODE(inst), compat), (.dphy_drv_data = drv_data,), ())

#define NXP_IMX_MIPI_CSI_INIT(inst)								\
	static struct nxp_imx_mipi_csi_data nxp_imx_mipi_csi_data_##inst;			\
	static const struct nxp_imx_mipi_csi_config nxp_imx_mipi_csi_config_##inst = {		\
		.csis_base = DT_INST_REG_ADDR(inst),						\
		.dphy_base = DT_REG_ADDR_BY_IDX(DPHY_NODE(inst), 0),				\
		.dphy_clock_dev = COND_CODE_1(DT_NODE_HAS_PROP(DPHY_NODE(inst), clocks),	\
			(DEVICE_DT_GET_OR_NULL(DT_CLOCKS_CTLR(DPHY_NODE(inst)))), (NULL)),	\
		.dphy_clock_subsys = COND_CODE_1(DT_NODE_HAS_PROP(DPHY_NODE(inst), clocks),	\
			((clock_control_subsys_t)DT_CLOCKS_CELL(DPHY_NODE(inst), name)), (NULL)), \
		.source_dev = CSI_SOURCE_DEV(inst),						\
		.num_lanes = DT_PROP_LEN(DT_INST_ENDPOINT_BY_ID(inst, 0, 0), data_lanes),	\
		NXP_IMX_DPHY_DRV_DATA(inst, nxp_imx95_dphy_rx, &nxp_imx95_dphy_drv_data)	\
	};											\
	DEVICE_DT_INST_DEFINE(inst, nxp_imx_mipi_csi_init, NULL, &nxp_imx_mipi_csi_data_##inst,	\
			      &nxp_imx_mipi_csi_config_##inst, POST_KERNEL,			\
			      CONFIG_VIDEO_IMX_MIPI_CSI_INIT_PRIORITY, &nxp_imx_mipi_csi_api);	\
	VIDEO_DEVICE_DEFINE(nxp_imx_mipi_csi_##inst, DEVICE_DT_INST_GET(inst),			\
			    CSI_SOURCE_DEV(inst));

DT_INST_FOREACH_STATUS_OKAY(NXP_IMX_MIPI_CSI_INIT)
