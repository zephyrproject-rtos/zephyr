/*
 * Copyright (c) 2026 Charles Dias
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT adi_max96724

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/video-controls.h>
#include <zephyr/drivers/video.h>
#include <zephyr/logging/log.h>

#include "video_common.h"
#include "video_device.h"
#include "video_ctrls.h"

LOG_MODULE_REGISTER(video_max96724, CONFIG_VIDEO_LOG_LEVEL);

#define MAX96724_POLL_RETRIES  10
#define MAX96724_POLL_DELAY_MS 100

#define MAX96724_REG8(addr) ((addr) | VIDEO_REG_ADDR16_DATA8)

/* */
#define MAX96724_REG_REG0 MAX96724_REG8(0x0000)

#define MAX96724_REG_REG6          MAX96724_REG8(0x0006)
#define MAX96724_REG6_LINK_EN_MASK GENMASK(3, 0)

#define MAX96724_REG_REG13 MAX96724_REG8(0x000D)

#define MAX96724_REG_PWR1       MAX96724_REG8(0x0013)
#define MAX96724_PWR1_RESET_ALL BIT(6)

#define MAX96724_REG_DEVID        MAX96724_REG8(0x000D)
#define MAX96724_REG_CTRL12       MAX96724_REG8(0x000A)
#define MAX96724_REG_CTRL13       MAX96724_REG8(0x000B)
#define MAX96724_REG_CTRL14       MAX96724_REG8(0x000C)
#define MAX96724_REG_CTRL3        MAX96724_REG8(0x001A)
#define MAX96724_CTRL_LOCKED      BIT(3)
#define MAX96724_CTRL3_ERROR      BIT(2)
#define MAX96724_CTRL3_CMU_LOCKED BIT(1)
#define MAX96724_CTRL3_LOCK_PIN   BIT(0)

/*
 * Per-pipe tunnel mode registers. MIPI_TX54/57 addresses use the pipe index (which equals the
 * default controller index) with stride 0x40.
 */
#define MAX96724_REG_MIPI_TX54(p) MAX96724_REG8(0x0936 + (p) * 0x40)
#define MAX96724_MIPI_TX54_TUN_EN BIT(0)

#define MAX96724_REG_MIPI_TX57(p)           MAX96724_REG8(0x0939 + (p) * 0x40)
#define MAX96724_MIPI_TX57_DIS_AUTO_TUN_DET BIT(6)

/* Pattern generator timing/colour registers */
#define MAX96724_REG_PATGEN_0                   MAX96724_REG8(0x1050)
#define MAX96724_PATGEN_0_VTG_MODE_MASK         GENMASK(1, 0)
#define MAX96724_PATGEN_0_VTG_MODE_FREE_RUNNING 0x3
#define MAX96724_PATGEN_0_GEN_VS                BIT(7)
#define MAX96724_PATGEN_0_GEN_HS                BIT(6)
#define MAX96724_PATGEN_0_GEN_DE                BIT(5)
#define MAX96724_PATGEN_0_VS_INV                BIT(4)
#define MAX96724_PATGEN_0_HS_INV                BIT(3)
#define MAX96724_PATGEN_0_DE_INV                BIT(2)

/* Pattern generator mode registers.*/
#define MAX96724_REG_PATGEN_1           MAX96724_REG8(0x1051)
#define MAX96724_PATGEN_1_MODE_MASK     GENMASK(5, 4)
#define MAX96724_PATGEN_1_MODE_DISABLED 0x0
/* Generate checkerboard pattern */
#define MAX96724_PATGEN_1_MODE_CHECKER  0x1
/* Generate gradient pattern */
#define MAX96724_PATGEN_1_MODE_GRADIENT 0x2
/* Gradient colour-increment per line. */
#define MAX96724_REG_GRAD_INCR          MAX96724_REG8(0x106D)

/* Checkerboard-mode colour/repeat registers. */
#define MAX96724_REG_CHKR_COLOR_A_L MAX96724_REG8(0x106E) /* R */
#define MAX96724_REG_CHKR_COLOR_A_M MAX96724_REG8(0x106F) /* G */
#define MAX96724_REG_CHKR_COLOR_A_H MAX96724_REG8(0x1070) /* B */
#define MAX96724_REG_CHKR_COLOR_B_L MAX96724_REG8(0x1071) /* R */
#define MAX96724_REG_CHKR_COLOR_B_M MAX96724_REG8(0x1072) /* G */
#define MAX96724_REG_CHKR_COLOR_B_H MAX96724_REG8(0x1073) /* B */
#define MAX96724_REG_CHKR_RPT_A     MAX96724_REG8(0x1074)
#define MAX96724_REG_CHKR_RPT_B     MAX96724_REG8(0x1075)
#define MAX96724_REG_CHKR_ALT       MAX96724_REG8(0x1076)

/*
 * Checkerboard tile geometry. With 800x480 active area:
 *  - RPT_A=RPT_B=80  → 10 horizontal tiles per line (800 / 80)
 *  - ALT=48          → 10 vertical tile bands (480 / 48)
 * Colour A is blue, colour B is red — matches Figure 22 in the MAX96724 user guide (p. 57).
 */
#define MAX96724_VPG_CHKR_RPT_A 80U
#define MAX96724_VPG_CHKR_RPT_B 80U
#define MAX96724_VPG_CHKR_ALT   48U
#define MAX96724_VPG_CHKR_A_R   0x00U
#define MAX96724_VPG_CHKR_A_G   0x00U
#define MAX96724_VPG_CHKR_A_B   0xFFU
#define MAX96724_VPG_CHKR_B_R   0xFFU
#define MAX96724_VPG_CHKR_B_G   0x00U
#define MAX96724_VPG_CHKR_B_B   0x00U

/*
 * 800x480 @ 60 Hz, 25 MHz pixel clock. Sized to exactly match the STM32N6570-DK on-board LTDC
 * panel so the VPG fills the screen with no unwritten margin.
 *
 *   hactive=800, hfp=4,  hsync=20, hbp=6   -> htotal=830
 *   vactive=480, vfp=2,  vsync=10, vbp=10  -> vtotal=502
 *   25 MHz / (830 * 502) ≈ 60.001 Hz
 */
#define MAX96724_VPG_HACTIVE 800U
#define MAX96724_VPG_HFP     4U
#define MAX96724_VPG_HSYNC   20U
#define MAX96724_VPG_HBP     6U
#define MAX96724_VPG_HTOTAL                                                                        \
	(MAX96724_VPG_HACTIVE + MAX96724_VPG_HFP + MAX96724_VPG_HSYNC + MAX96724_VPG_HBP)

#define MAX96724_VPG_VACTIVE 480U
#define MAX96724_VPG_VFP     2U
#define MAX96724_VPG_VSYNC   10U
#define MAX96724_VPG_VBP     10U
#define MAX96724_VPG_VTOTAL                                                                        \
	(MAX96724_VPG_VACTIVE + MAX96724_VPG_VFP + MAX96724_VPG_VSYNC + MAX96724_VPG_VBP)

/* Computed VPG timing fields */
#define MAX96724_VPG_VS_DLY  0U
#define MAX96724_VPG_V2H     0U
#define MAX96724_VPG_VS_HIGH (MAX96724_VPG_VSYNC * MAX96724_VPG_HTOTAL)
#define MAX96724_VPG_VS_LOW                                                                        \
	((MAX96724_VPG_VACTIVE + MAX96724_VPG_VFP + MAX96724_VPG_VBP) * MAX96724_VPG_HTOTAL)
#define MAX96724_VPG_HS_HIGH MAX96724_VPG_HSYNC
#define MAX96724_VPG_HS_LOW  (MAX96724_VPG_HACTIVE + MAX96724_VPG_HFP + MAX96724_VPG_HBP)
#define MAX96724_VPG_HS_CNT                                                                        \
	(MAX96724_VPG_VACTIVE + MAX96724_VPG_VFP + MAX96724_VPG_VBP + MAX96724_VPG_VSYNC)
#define MAX96724_VPG_V2D                                                                           \
	(MAX96724_VPG_HTOTAL * (MAX96724_VPG_VSYNC + MAX96724_VPG_VBP) +                           \
	 (MAX96724_VPG_HSYNC + MAX96724_VPG_HBP))
#define MAX96724_VPG_DE_HIGH MAX96724_VPG_HACTIVE
#define MAX96724_VPG_DE_LOW  (MAX96724_VPG_HFP + MAX96724_VPG_HSYNC + MAX96724_VPG_HBP)
#define MAX96724_VPG_DE_CNT  MAX96724_VPG_VACTIVE

static const char *max96724_variant_model(uint8_t id)
{
	if (id == 0xA2) {
		return "MAX96724";
	} else if (id == 0xA3) {
		return "MAX96724F";
	} else if (id == 0xA4) {
		return "MAX96724R";
	}

	return "unknown";
}

struct max96724_config {
	struct i2c_dt_spec i2c;
	const struct device *source_dev;
	uint8_t csi_tx_num_lanes;
	uint8_t csi_tx_phy_index;
	int64_t csi_tx_link_freq;
	uint8_t link_en_mask;
};

struct max96724_data {
	struct video_format fmt;
	struct video_ctrl linkfreq;
	bool streaming;
};

static const struct video_format_cap max96724_fmts[] = {
	{
		.pixelformat = VIDEO_PIX_FMT_BGR24,
		.width_min = MAX96724_VPG_HACTIVE,
		.width_max = MAX96724_VPG_HACTIVE,
		.width_step = 0,
		.height_min = MAX96724_VPG_VACTIVE,
		.height_max = MAX96724_VPG_VACTIVE,
		.height_step = 0,
	},
	{0},
};

static int max96724_poll_device(const struct i2c_dt_spec *i2c)
{
	uint32_t val;
	int ret;

	for (int i = 0; i < MAX96724_POLL_RETRIES; i++) {
		ret = video_read_cci_reg(i2c, MAX96724_REG_REG0, &val);
		if (ret == 0 && val != 0) {
			LOG_DBG("MAX96724 device address: 0x%02x", val);
			return 0;
		}

		LOG_DBG("Waiting for device to become active (%d/%d)", i + 1,
			MAX96724_POLL_RETRIES);
		k_msleep(MAX96724_POLL_DELAY_MS);
	}

	return -ENODEV;
}

static int max96724_device_reset(const struct i2c_dt_spec *i2c)
{
	int ret;

	ret = max96724_poll_device(i2c);
	if (ret < 0) {
		return ret;
	}

	/* This is equivalent to toggling the PWDNB pin. The bit is cleared when written.*/
	ret = video_modify_cci_reg(i2c, MAX96724_REG_PWR1, MAX96724_PWR1_RESET_ALL,
				   MAX96724_PWR1_RESET_ALL);
	if (ret < 0) {
		LOG_ERR("Failed to reset device (err %d)", ret);
		return ret;
	}

	ret = max96724_poll_device(i2c);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static void max96724_log_device_id(const struct i2c_dt_spec *i2c)
{
	uint32_t dev_id;
	int ret;

	ret = video_read_cci_reg(i2c, MAX96724_REG_REG13, &dev_id);
	if (ret < 0) {
		LOG_ERR("Failed to read device ID (err %d)", ret);
		return;
	}

	LOG_DBG("Found %s (ID=0x%02X) on %s@0x%02x", max96724_variant_model(dev_id), dev_id,
		i2c->bus->name, i2c->addr);
}

static void max96724_log_errb_status(const struct i2c_dt_spec *i2c)
{
	uint32_t val;
	int ret;

	ret = video_read_cci_reg(i2c, MAX96724_REG_CTRL3, &val);
	if (ret == 0 && (val & MAX96724_CTRL3_ERROR)) {
		LOG_WRN("ERRB asserted — check GMSL link integrity");
	}
}

static int max96724_check_link_lock(const struct i2c_dt_spec *i2c)
{
	uint32_t val = 0;
	uint8_t locked_mask = 0;
	int ret;

	/* Poll LOCK_PIN — reflects all enabled GMSL links locked */
	for (int i = 0; i < MAX96724_POLL_RETRIES; i++) {
		ret = video_read_cci_reg(i2c, MAX96724_REG_CTRL3, &val);
		if (ret < 0) {
			LOG_ERR("Failed to read CTRL3 (err %d)", ret);
			return ret;
		}

		if (val & MAX96724_CTRL3_LOCK_PIN) {
			LOG_INF("All enabled GMSL2 links locked (CTRL3=0x%02X)", val);
			return 0;
		}

		LOG_DBG("Waiting for GMSL2 link lock (%d/%d, CTRL3=0x%02X)", i + 1,
			MAX96724_POLL_RETRIES, val);
		k_msleep(MAX96724_POLL_DELAY_MS);
	}

	/* LOCK_PIN still 0 — check each link individually */
	static const struct {
		uint32_t reg;
		const char *name;
	} links[] = {
		{MAX96724_REG_CTRL3, "A"},
		{MAX96724_REG_CTRL12, "B"},
		{MAX96724_REG_CTRL13, "C"},
		{MAX96724_REG_CTRL14, "D"},
	};

	LOG_WRN("Not all enabled GMSL2 links locked — checking individually:");

	for (int l = 0; l < ARRAY_SIZE(links); l++) {
		ret = video_read_cci_reg(i2c, links[l].reg, &val);
		if (ret < 0) {
			LOG_ERR("Failed to read link %s status (err %d)", links[l].name, ret);
			return ret;
		}

		if (val & MAX96724_CTRL_LOCKED) {
			LOG_INF("  Link %s: locked", links[l].name);
			locked_mask |= BIT(l);
		} else {
			LOG_WRN("  Link %s: not locked", links[l].name);
		}
	}

	if (locked_mask != 0) {
		return 0;
	}

	return -ENODEV;
}

static int max96724_write_pattern_timings(const struct i2c_dt_spec *i2c)
{
	/*
	 * VTG counters at 0x1052..0x106C. Multi-byte fields are MSB-first across consecutive 8-bit
	 * registers. The helpers below split a host value onto (base, base+1[, base+2]).
	 */
#define VTG_REG_3(base, value)                                                                     \
	{MAX96724_REG8((base) + 0), ((value) >> 16) & 0xFFU},                                      \
		{MAX96724_REG8((base) + 1), ((value) >> 8) & 0xFFU},                               \
		{MAX96724_REG8((base) + 2), ((value) >> 0) & 0xFFU}
#define VTG_REG_2(base, value)                                                                     \
	{MAX96724_REG8((base) + 0), ((value) >> 8) & 0xFFU},                                       \
		{MAX96724_REG8((base) + 1), ((value) >> 0) & 0xFFU}

	const struct video_reg regs[] = {
		VTG_REG_3(0x1052, MAX96724_VPG_VS_DLY),
		VTG_REG_3(0x1055, MAX96724_VPG_VS_HIGH),
		VTG_REG_3(0x1058, MAX96724_VPG_VS_LOW),
		VTG_REG_3(0x105B, MAX96724_VPG_V2H),

		VTG_REG_2(0x105E, MAX96724_VPG_HS_HIGH),
		VTG_REG_2(0x1060, MAX96724_VPG_HS_LOW),
		VTG_REG_2(0x1062, MAX96724_VPG_HS_CNT),

		VTG_REG_3(0x1064, MAX96724_VPG_V2D),

		VTG_REG_2(0x1067, MAX96724_VPG_DE_HIGH),
		VTG_REG_2(0x1069, MAX96724_VPG_DE_LOW),
		VTG_REG_2(0x106B, MAX96724_VPG_DE_CNT),

		{MAX96724_REG_PATGEN_0,
		 FIELD_PREP(MAX96724_PATGEN_0_VTG_MODE_MASK,
			    MAX96724_PATGEN_0_VTG_MODE_FREE_RUNNING) |
			 MAX96724_PATGEN_0_VS_INV | MAX96724_PATGEN_0_GEN_DE |
			 MAX96724_PATGEN_0_GEN_HS | MAX96724_PATGEN_0_GEN_VS},
	};

#undef VTG_REG_3
#undef VTG_REG_2

	return video_write_cci_multiregs(i2c, regs, ARRAY_SIZE(regs));
}

static int max96724_write_gradient_pattern(const struct i2c_dt_spec *i2c)
{
	return video_write_cci_reg(i2c, MAX96724_REG_GRAD_INCR, 8);
}

static int max96724_write_checkerboard_pattern(const struct i2c_dt_spec *i2c)
{
	const struct video_reg regs[] = {
		{MAX96724_REG_CHKR_COLOR_A_L, MAX96724_VPG_CHKR_A_R},
		{MAX96724_REG_CHKR_COLOR_A_M, MAX96724_VPG_CHKR_A_G},
		{MAX96724_REG_CHKR_COLOR_A_H, MAX96724_VPG_CHKR_A_B},
		{MAX96724_REG_CHKR_COLOR_B_L, MAX96724_VPG_CHKR_B_R},
		{MAX96724_REG_CHKR_COLOR_B_M, MAX96724_VPG_CHKR_B_G},
		{MAX96724_REG_CHKR_COLOR_B_H, MAX96724_VPG_CHKR_B_B},
		{MAX96724_REG_CHKR_RPT_A, MAX96724_VPG_CHKR_RPT_A},
		{MAX96724_REG_CHKR_RPT_B, MAX96724_VPG_CHKR_RPT_B},
		{MAX96724_REG_CHKR_ALT, MAX96724_VPG_CHKR_ALT},
	};

	return video_write_cci_multiregs(i2c, regs, ARRAY_SIZE(regs));
}

static int max96724_get_caps(const struct device *dev, struct video_caps *caps)
{
	const struct max96724_config *cfg = dev->config;

	if (caps->type != VIDEO_BUF_TYPE_OUTPUT) {
		return -EINVAL;
	}

	/* Forward to source when in passthrough mode */
	if (cfg->source_dev != NULL) {
		return video_get_caps(cfg->source_dev, caps);
	}

	caps->format_caps = max96724_fmts;
	/* The deserializer doesn't DMA — buffers live downstream in DCMIPP. */
	caps->min_vbuf_count = 1;
	return 0;
}

static int max96724_get_format(const struct device *dev, struct video_format *fmt)
{
	const struct max96724_config *cfg = dev->config;
	struct max96724_data *data = dev->data;

	if (cfg->source_dev != NULL) {
		return video_get_format(cfg->source_dev, fmt);
	}

	*fmt = data->fmt;
	return 0;
}

static int max96724_set_format(const struct device *dev, struct video_format *fmt)
{
	const struct max96724_config *cfg = dev->config;
	struct max96724_data *data = dev->data;
	int ret;

	if (IS_ENABLED(CONFIG_VIDEO_MAX96724_VPG_ENABLE)) {
		/* VPG mode: only the hard-coded 800x480 BGR24 is supported */
		if (fmt->pixelformat != VIDEO_PIX_FMT_BGR24 || fmt->width != MAX96724_VPG_HACTIVE ||
		    fmt->height != MAX96724_VPG_VACTIVE) {
			LOG_ERR("Format %s %ux%u not supported by VPG",
				VIDEO_FOURCC_TO_STR(fmt->pixelformat), fmt->width, fmt->height);
			return -ENOTSUP;
		}
	}

	if (cfg->source_dev != NULL) {
		ret = video_set_format(cfg->source_dev, fmt);
		if (ret < 0) {
			return ret;
		}
	}

	data->fmt = *fmt;
	return 0;
}

static int max96724_set_stream(const struct device *dev, bool enable, enum video_buf_type type)
{

	return -ENOSYS;
}

static int max96724_set_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct max96724_config *cfg = dev->config;

	if (cfg->source_dev == NULL) {
		return -ENOSYS;
	}

	return video_set_frmival(cfg->source_dev, frmival);
}

static int max96724_get_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct max96724_config *cfg = dev->config;

	if (cfg->source_dev == NULL) {
		return -ENOSYS;
	}

	return video_get_frmival(cfg->source_dev, frmival);
}

static int max96724_enum_frmival(const struct device *dev, struct video_frmival_enum *fie)
{
	const struct max96724_config *cfg = dev->config;

	if (cfg->source_dev == NULL) {
		return -ENOSYS;
	}

	return video_enum_frmival(cfg->source_dev, fie);
}

static DEVICE_API(video, max96724_api) = {
	.get_caps = max96724_get_caps,
	.get_format = max96724_get_format,
	.set_format = max96724_set_format,
	.set_stream = max96724_set_stream,
	.set_frmival = max96724_set_frmival,
	.get_frmival = max96724_get_frmival,
	.enum_frmival = max96724_enum_frmival,
};

static int max96724_init(const struct device *dev)
{
	const struct max96724_config *cfg = dev->config;
	struct max96724_data *data = dev->data;
	int ret;

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}

	ret = max96724_device_reset(&cfg->i2c);
	if (ret < 0) {
		return ret;
	}

	max96724_log_device_id(&cfg->i2c);

	/* Enabled only those links specified in the device tree */
	ret = video_modify_cci_reg(&cfg->i2c, MAX96724_REG_REG6, MAX96724_REG6_LINK_EN_MASK,
				   cfg->link_en_mask);
	if (ret < 0) {
		LOG_ERR("Failed to set LINK_EN (err %d)", ret);
		return ret;
	}
	LOG_DBG("LINK_EN=0x%02X (from DTS)", cfg->link_en_mask);

	/* TODO: Reanalyze this verification at this point */
	ret = max96724_check_link_lock(&cfg->i2c);
	if (ret < 0) {
		if (IS_ENABLED(CONFIG_VIDEO_MAX96724_VPG_ENABLE)) {
			LOG_WRN("No GMSL link locked — continuing (VPG mode)");
		} else {
			return ret;
		}
	}

	max96724_log_errb_status(&cfg->i2c);

	/* Disable automatic tunnel mode detection on all pipes and explicitly clear tunnel enable.
	 *
	 * TODO: Consider making this configurable via DTS if use cases arise that require
	 * automatic tunnel detection or per-pipe tunnel mode control.
	 */
	for (int i = 0; i < 4; i++) {
		ret = video_modify_cci_reg(&cfg->i2c, MAX96724_REG_MIPI_TX57(i),
					   MAX96724_MIPI_TX57_DIS_AUTO_TUN_DET,
					   MAX96724_MIPI_TX57_DIS_AUTO_TUN_DET);
		if (ret < 0) {
			return ret;
		}

		ret = video_modify_cci_reg(&cfg->i2c, MAX96724_REG_MIPI_TX54(i),
					   MAX96724_MIPI_TX54_TUN_EN, 0);
		if (ret < 0) {
			return ret;
		}
	}

	ret = video_init_int_menu_ctrl(&data->linkfreq, dev, VIDEO_CID_LINK_FREQ, 0,
				       &cfg->csi_tx_link_freq, 1);
	if (ret < 0) {
		LOG_ERR("Failed to init link-frequency control (err %d)", ret);
		return ret;
	}
	data->linkfreq.flags |= VIDEO_CTRL_FLAG_READ_ONLY;

	return 0;
}

#define SOURCE_DEV(inst)                                                                           \
	COND_CODE_1(                                                                               \
		DT_NODE_HAS_STATUS_OKAY(                                                           \
			DT_NODE_REMOTE_DEVICE(DT_INST_ENDPOINT_BY_ID(inst, 0, 0))),                \
		(DEVICE_DT_GET(DT_NODE_REMOTE_DEVICE(DT_INST_ENDPOINT_BY_ID(inst, 0, 0)))),        \
		(NULL))

#define MAX96724_CSI_OUT_PORT DT_NODELABEL(des_csi_port)
#define MAX96724_CSI_OUT_EP   DT_NODELABEL(des_csi_out)

/* CSI-2 TX PHY index from the port's phy-id property. */
#define CSI_TX_PHY_INDEX(inst) DT_PROP(MAX96724_CSI_OUT_PORT, phy_id)

/* Number of CSI-2 TX data lanes from the CSI output endpoint's data-lanes property. */
#define CSI_TX_NUM_LANES(inst) DT_PROP_LEN(MAX96724_CSI_OUT_EP, data_lanes)

/* CSI-2 TX link frequency (Hz) from the CSI output endpoint's link-frequencies property. */
#define CSI_TX_LINK_FREQ(inst) DT_PROP_BY_IDX(MAX96724_CSI_OUT_EP, link_frequencies, 0)

/*
 * Build the GMSL LINK_EN bitmap from DTS: each port child that declares
 * link-id = N contributes BIT(N). Ports without link-id (e.g. the CSI
 * output port) contribute 0 because BIT(4) is masked off.
 */
#define MAX96724_PORT_LINK_BIT(port_node)                                                          \
	(BIT(DT_PROP_OR(port_node, link_id, 4)) & MAX96724_REG6_LINK_EN_MASK)

#define MAX96724_LINK_EN_FROM_DT(inst)                                                             \
	(DT_FOREACH_CHILD_STATUS_OKAY_SEP(DT_INST_CHILD(inst, ports), MAX96724_PORT_LINK_BIT, (|)))

#define MAX96724_INIT(inst)                                                                        \
	static const struct max96724_config max96724_config_##inst = {                             \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.source_dev = SOURCE_DEV(inst),                                                    \
		.csi_tx_num_lanes = CSI_TX_NUM_LANES(inst),                                        \
		.csi_tx_phy_index = CSI_TX_PHY_INDEX(inst),                                        \
		.csi_tx_link_freq = CSI_TX_LINK_FREQ(inst),                                        \
		.link_en_mask = MAX96724_LINK_EN_FROM_DT(inst),                                    \
	};                                                                                         \
	static struct max96724_data max96724_data_##inst;                                          \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, max96724_init, NULL, &max96724_data_##inst,                    \
			      &max96724_config_##inst, POST_KERNEL,                                \
			      CONFIG_VIDEO_MAX96724_INIT_PRIORITY, &max96724_api);                 \
                                                                                                   \
	VIDEO_DEVICE_DEFINE(max96724_##inst, DEVICE_DT_INST_GET(inst), SOURCE_DEV(inst));

DT_INST_FOREACH_STATUS_OKAY(MAX96724_INIT)
