/*
 * Copyright (c) 2026 Charles Dias
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT adi_max96717

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/video.h>
#include <zephyr/logging/log.h>
#include <zephyr/video/controls.h>

#include "video_common.h"

LOG_MODULE_REGISTER(video_max96717, CONFIG_VIDEO_LOG_LEVEL);

#define MAX96717_POLL_RETRIES      10
#define MAX96717_POLL_DELAY_MS     100
#define MAX96717_PROBE_TIMEOUT_MS  1000
#define MAX96717_PROBE_INTERVAL_MS 5

/*
 * The MAX96717 has a single video pipe at hardware index 2. All per-pipe registers must be
 * addressed with this index.
 */
#define MAX96717_PIPE_HW_ID 2

#define MAX96717_REG8(addr) ((addr) | VIDEO_REG_ADDR16_DATA8)

/* Per-pipe video TX enable (REG2) */
#define MAX96717_REG_REG2         MAX96717_REG8(0x0002)
#define MAX96717_REG2_VID_TX_EN_Z BIT(6)

#define MAX96717_REG_REG13 MAX96717_REG8(0x000D)

#define MAX96717_REG_CTRL0       MAX96717_REG8(0x0010)
#define MAX96717_CTRL0_RESET_ALL BIT(7)

#define MAX96717_REG_CTRL3        MAX96717_REG8(0x0013)
#define MAX96717_CTRL3_LOCKED     BIT(3)
#define MAX96717_CTRL3_ERROR      BIT(2)
#define MAX96717_CTRL3_CMU_LOCKED BIT(1)

/* CFGV VIDEO_Z.TX3 (0x005B) video-pipe-Z GMSL stream-ID selector. */
#define MAX96717_REG_VIDEO_Z_TX3        MAX96717_REG8(0x005B)
#define MAX96717_VIDEO_Z_TX3_TX_STR_SEL GENMASK(1, 0)

/* VID_TX Z block: VIDEO_TX0/TX1/TX2, the pipe Z video TX configuration registers. */
#define MAX96717_REG_VIDEO_Z_TX0      MAX96717_REG8(0x0110)
#define MAX96717_VIDEO_Z_TX0_AUTO_BPP BIT(3)

#define MAX96717_REG_VIDEO_Z_TX1      MAX96717_REG8(0x0111)
#define MAX96717_VIDEO_Z_TX1_BPP_MASK GENMASK(5, 0)

/*
 * Bit 1 is reserved in the datasheet, with reset value 1. The datasheet documents the drift
 * status only (DRIFT_ERR, bit 6): the pixel clock drift detection of the GMSL2 General User
 * Guide (section 6.2.1), which restarts the video subsystem when the clock drifts by more than
 * 1.25%. The Linux maxim-serdes driver names this bit its enable, DRIFT_DET_EN, and clears it
 * when it programs a fixed bpp. Same here: the bit keeps its reset value with AUTO_BPP.
 */
#define MAX96717_REG_VIDEO_Z_TX2          MAX96717_REG8(0x0112)
#define MAX96717_VIDEO_Z_TX2_LIM_HEART    BIT(2)
#define MAX96717_VIDEO_Z_TX2_DRIFT_DET_EN BIT(1)

/*
 * Bit 6 enables the line start info frames and bit 5 the CSI-2 port. Bit 2 (CLK_SELZ, the clock
 * source of pipe Z, hidden in the datasheet) keeps its reset value, port B.
 */
#define MAX96717_REG_FRONTTOP_0              MAX96717_REG8(0x0308)
#define MAX96717_FRONTTOP_0_ENABLE_LINE_INFO BIT(6)
#define MAX96717_FRONTTOP_0_START_PORTB      BIT(5)

#define MAX96717_REG_FRONTTOP_9          MAX96717_REG8(0x0311)
#define MAX96717_FRONTTOP_9_START_PORTBZ BIT(6)

#define MAX96717_REG_FRONTTOP_11       MAX96717_REG8(0x0313)
#define MAX96717_FRONTTOP_11_BPP10DBLZ BIT(2)
#define MAX96717_REG_FRONTTOP_16             MAX96717_REG8(0x0318)
#define MAX96717_FRONTTOP_16_MEM_DT1_SELZ_DT GENMASK(5, 0)
#define MAX96717_FRONTTOP_16_MEM_DT1_SELZ_EN BIT(6)

#define MAX96717_REG_FRONTTOP_22          MAX96717_REG8(0x031E)
#define MAX96717_FRONTTOP_22_SOFT_BPPZ_EN BIT(5)
#define MAX96717_FRONTTOP_22_SOFT_BPPZ    GENMASK(4, 0)

/* MIPI CSI-2 RX PHY configuration */
#define MAX96717_REG_MIPI_RX0           MAX96717_REG8(0x0330)
#define MAX96717_MIPI_RX0_NONCONTCLK_EN BIT(6)

#define MAX96717_REG_MIPI_RX1            MAX96717_REG8(0x0331)
#define MAX96717_MIPI_RX1_CTRL_NUM_LANES GENMASK(5, 4)

/* EXT11 tunnel mode control */
#define MAX96717_REG_EXT11      MAX96717_REG8(0x0383)
#define MAX96717_EXT11_TUN_MODE BIT(7)

/* PFDDIV regulator voltage control. */
#define MAX96717_REG_CMU2                MAX96717_REG8(0x0302)
#define MAX96717_CMU2_PFDDIV_RSHORT_MASK GENMASK(6, 4)
#define MAX96717_CMU2_PFDDIV_RSHORT_1_1V 0x1 /* 0b001 = 1.1V */

/* Per-pipe PATGEN block (base 0x1C8, stride 0x43 per pipe). */
#define MAX96717_REG_VTX(p, off) MAX96717_REG8(0x01C8 + (p) * 0x43 + (off))

#define MAX96717_REG_VTX0              MAX96717_REG8(0x024E)
#define MAX96717_VTX0_GEN_VS           BIT(7)
#define MAX96717_VTX0_GEN_HS           BIT(6)
#define MAX96717_VTX0_GEN_DE           BIT(5)
#define MAX96717_VTX0_VS_INV           BIT(4)
#define MAX96717_VTX0_HS_INV           BIT(3)
#define MAX96717_VTX0_DE_INV           BIT(2)
#define MAX96717_VTX0_VTG_MODE_MASK    GENMASK(1, 0)
#define MAX96717_VTX0_VTG_FREE_RUNNING 0x3

#define MAX96717_REG_VTX1                 MAX96717_REG8(0x024F)
#define MAX96717_VTX1_PATGEN_CLK_SRC_MASK GENMASK(3, 1)
#define MAX96717_VTX1_PATGEN_CLK_25MHZ    0x4 /* 0b100 */

/* VTX29 PATGEN mode select */
#define MAX96717_REG_VTX29              MAX96717_REG8(0x026B)
#define MAX96717_VTX29_PATGEN_MODE_MASK GENMASK(1, 0)
#define MAX96717_VTX29_PATGEN_DISABLED  0x0
#define MAX96717_VTX29_PATGEN_CHECKER   0x1
#define MAX96717_VTX29_PATGEN_GRADIENT  0x2

/* VTX30..VTX39 gradient increment and checkerboard colour/repeat */
#define MAX96717_REG_VTX30_GRAD_INCR  MAX96717_REG8(0x026C)
#define MAX96717_REG_VTX31_CHKR_A_L   MAX96717_REG8(0x026D)
#define MAX96717_REG_VTX32_CHKR_A_M   MAX96717_REG8(0x026E)
#define MAX96717_REG_VTX33_CHKR_A_H   MAX96717_REG8(0x026F)
#define MAX96717_REG_VTX34_CHKR_B_L   MAX96717_REG8(0x0270)
#define MAX96717_REG_VTX35_CHKR_B_M   MAX96717_REG8(0x0271)
#define MAX96717_REG_VTX36_CHKR_B_H   MAX96717_REG8(0x0272)
#define MAX96717_REG_VTX37_CHKR_RPT_A MAX96717_REG8(0x0273)
#define MAX96717_REG_VTX38_CHKR_RPT_B MAX96717_REG8(0x0274)
#define MAX96717_REG_VTX39_CHKR_ALT   MAX96717_REG8(0x0275)

/*
 * Checkerboard tile geometry. With 800x480 active area:
 *  - RPT_A=RPT_B=80  -> 10 horizontal tiles per line (800 / 80)
 *  - ALT=48          -> 10 vertical tile bands (480 / 48)
 * Colour A is blue, colour B is red, matches Figure 22 in the MAX96724 user guide (p. 57).
 */
#define MAX96717_VPG_CHKR_RPT_A 80U
#define MAX96717_VPG_CHKR_RPT_B 80U
#define MAX96717_VPG_CHKR_ALT   48U
#define MAX96717_VPG_CHKR_A_R   0x00U
#define MAX96717_VPG_CHKR_A_G   0x00U
#define MAX96717_VPG_CHKR_A_B   0xFFU
#define MAX96717_VPG_CHKR_B_R   0xFFU
#define MAX96717_VPG_CHKR_B_G   0x00U
#define MAX96717_VPG_CHKR_B_B   0x00U

/*
 * 800x480 @ 60 Hz, 25 MHz pixel clock.
 *
 *   hactive=800, hfp=4,  hsync=20, hbp=6   -> htotal=830
 *   vactive=480, vfp=2,  vsync=10, vbp=10  -> vtotal=502
 *   25 MHz / (830 * 502) = 60.001 Hz
 */
#define MAX96717_VPG_HACTIVE 800U
#define MAX96717_VPG_HFP     4U
#define MAX96717_VPG_HSYNC   20U
#define MAX96717_VPG_HBP     6U
#define MAX96717_VPG_HTOTAL                                                                        \
	(MAX96717_VPG_HACTIVE + MAX96717_VPG_HFP + MAX96717_VPG_HSYNC + MAX96717_VPG_HBP)

#define MAX96717_VPG_VACTIVE 480U
#define MAX96717_VPG_VFP     2U
#define MAX96717_VPG_VSYNC   10U
#define MAX96717_VPG_VBP     10U

#define MAX96717_VPG_VS_DLY  0U
#define MAX96717_VPG_V2H     0U
#define MAX96717_VPG_VS_HIGH (MAX96717_VPG_VSYNC * MAX96717_VPG_HTOTAL)
#define MAX96717_VPG_VS_LOW                                                                        \
	((MAX96717_VPG_VACTIVE + MAX96717_VPG_VFP + MAX96717_VPG_VBP) * MAX96717_VPG_HTOTAL)
#define MAX96717_VPG_HS_HIGH MAX96717_VPG_HSYNC
#define MAX96717_VPG_HS_LOW  (MAX96717_VPG_HACTIVE + MAX96717_VPG_HFP + MAX96717_VPG_HBP)
#define MAX96717_VPG_HS_CNT                                                                        \
	(MAX96717_VPG_VACTIVE + MAX96717_VPG_VFP + MAX96717_VPG_VBP + MAX96717_VPG_VSYNC)
#define MAX96717_VPG_V2D                                                                           \
	(MAX96717_VPG_HTOTAL * (MAX96717_VPG_VSYNC + MAX96717_VPG_VBP) +                           \
	 (MAX96717_VPG_HSYNC + MAX96717_VPG_HBP))
#define MAX96717_VPG_DE_HIGH MAX96717_VPG_HACTIVE
#define MAX96717_VPG_DE_LOW  (MAX96717_VPG_HFP + MAX96717_VPG_HSYNC + MAX96717_VPG_HBP)
#define MAX96717_VPG_DE_CNT  MAX96717_VPG_VACTIVE

static const char *max96717_variant_name(uint8_t id)
{
	if (id == 0xBF) {
		return "MAX96717";
	}
	if (id == 0xC8) {
		return "MAX96717F";
	}

	return "unknown";
}

struct max96717_config {
	struct i2c_dt_spec i2c;
	const struct device *source_dev;
	uint8_t csi_rx_num_lanes;
};

/* VIDEO_CID_TEST_PATTERN: the pattern generator replaces the sensor video on pipe Z. */
enum max96717_test_pattern {
	MAX96717_TEST_PATTERN_OFF,
	MAX96717_TEST_PATTERN_GRADIENT,
	MAX96717_TEST_PATTERN_CHECKERBOARD,
};

static const char *const max96717_test_pattern_menu[] = {
	"Off",
	"Gradient",
	"Checkerboard",
	NULL,
};

struct max96717_data {
	struct video_format fmt;
	struct video_ctrl test_pattern;
	bool streaming;
};

/* The pattern generator's only format. */
static const struct video_format_cap max96717_fmts[] = {
	{
		.pixelformat = VIDEO_PIX_FMT_RGB24,
		.width_min = MAX96717_VPG_HACTIVE,
		.width_max = MAX96717_VPG_HACTIVE,
		.width_step = 0,
		.height_min = MAX96717_VPG_VACTIVE,
		.height_max = MAX96717_VPG_VACTIVE,
		.height_step = 0,
	},
	{0},
};

static const struct video_format max96717_vpg_fmt = {
	.type = VIDEO_BUF_TYPE_OUTPUT,
	.pixelformat = VIDEO_PIX_FMT_RGB24,
	.width = MAX96717_VPG_HACTIVE,
	.height = MAX96717_VPG_VACTIVE,
	.pitch = MAX96717_VPG_HACTIVE * 3,
	.size = MAX96717_VPG_HACTIVE * 3 * MAX96717_VPG_VACTIVE,
};

/*
 * Wait for the device to answer, with raw reads that neither retry nor log: after power-up and
 * after a reset the device stays silent for a while, which is expected.
 */
static int max96717_poll_device(const struct i2c_dt_spec *i2c)
{
	const uint16_t addr = MAX96717_REG_REG13 & VIDEO_REG_ADDR_MASK;
	const uint8_t reg[] = {addr >> 8, addr & 0xFF};
	uint8_t val;

	for (int ms = 0; ms < MAX96717_PROBE_TIMEOUT_MS; ms += MAX96717_PROBE_INTERVAL_MS) {
		if (i2c_write_read_dt(i2c, reg, sizeof(reg), &val, sizeof(val)) == 0 && val != 0) {
			LOG_DBG("Device answered after %d ms", ms);
			return 0;
		}

		k_msleep(MAX96717_PROBE_INTERVAL_MS);
	}

	LOG_ERR("No answer from %s@0x%02x within %d ms", i2c->bus->name, i2c->addr,
		MAX96717_PROBE_TIMEOUT_MS);

	return -ENODEV;
}

/*
 * This is equivalent to toggling the PWDNB pin. The bit is cleared when written.
 *
 * The reset drops the GMSL reverse channel momentarily, so we poll the device-ID register until
 * the MAX96717 responds again before returning.
 */
static int max96717_reset_device(const struct i2c_dt_spec *i2c)
{
	int ret;

	ret = max96717_poll_device(i2c);
	if (ret < 0) {
		return ret;
	}

	ret = video_write_cci_reg(i2c, MAX96717_REG_CTRL0, MAX96717_CTRL0_RESET_ALL);
	if (ret < 0) {
		LOG_ERR("Failed to reset device (err %d)", ret);
		return ret;
	}

	ret = max96717_poll_device(i2c);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static void max96717_log_device_id(const struct i2c_dt_spec *i2c)
{
	uint32_t dev_id;
	int ret;

	ret = video_read_cci_reg(i2c, MAX96717_REG_REG13, &dev_id);
	if (ret < 0) {
		LOG_ERR("Failed to read device ID (err %d)", ret);
		return;
	}

	LOG_DBG("Found %s (ID=0x%02X) on %s@0x%02x", max96717_variant_name(dev_id), dev_id,
		i2c->bus->name, i2c->addr);
}

static int max96717_check_link_lock(const struct i2c_dt_spec *i2c)
{
	uint32_t val;
	int ret;

	for (int i = 0; i < MAX96717_POLL_RETRIES; i++) {
		ret = video_read_cci_reg(i2c, MAX96717_REG_CTRL3, &val);
		if (ret < 0) {
			LOG_ERR("Failed to read CTRL3 (err %d)", ret);
			return ret;
		}

		if (val & MAX96717_CTRL3_LOCKED) {
			LOG_DBG("GMSL2 link locked (CTRL3=0x%02X)", val);
			break;
		}

		LOG_DBG("Waiting for GMSL2 link lock (%d/%d, CTRL3=0x%02X)", i + 1,
			MAX96717_POLL_RETRIES, val);
		k_msleep(MAX96717_POLL_DELAY_MS);
	}

	if (!(val & MAX96717_CTRL3_LOCKED)) {
		LOG_WRN("GMSL2 link not locked (CTRL3=0x%02X)", val);
	}

	if (!(val & MAX96717_CTRL3_CMU_LOCKED)) {
		LOG_WRN("CMU not locked");
	}

	if (val & MAX96717_CTRL3_ERROR) {
		LOG_WRN("ERRB asserted. Check GMSL link integrity");
	}

	return 0;
}

static int max96717_write_pattern_timings(const struct i2c_dt_spec *i2c)
{
	const int p = MAX96717_PIPE_HW_ID;
#define VTG_REG_3(base, value)                                                                     \
	{MAX96717_REG_VTX(p, (base) + 0), ((value) >> 16) & 0xFFU},                                \
		{MAX96717_REG_VTX(p, (base) + 1), ((value) >> 8) & 0xFFU},                         \
		{MAX96717_REG_VTX(p, (base) + 2), ((value) >> 0) & 0xFFU}
#define VTG_REG_2(base, value)                                                                     \
	{MAX96717_REG_VTX(p, (base) + 0), ((value) >> 8) & 0xFFU},                                 \
		{MAX96717_REG_VTX(p, (base) + 1), ((value) >> 0) & 0xFFU}

	const struct video_reg regs[] = {
		VTG_REG_3(2, MAX96717_VPG_VS_DLY),
		VTG_REG_3(5, MAX96717_VPG_VS_HIGH),
		VTG_REG_3(8, MAX96717_VPG_VS_LOW),
		VTG_REG_3(11, MAX96717_VPG_V2H),

		VTG_REG_2(14, MAX96717_VPG_HS_HIGH),
		VTG_REG_2(16, MAX96717_VPG_HS_LOW),
		VTG_REG_2(18, MAX96717_VPG_HS_CNT),

		VTG_REG_3(20, MAX96717_VPG_V2D),

		VTG_REG_2(23, MAX96717_VPG_DE_HIGH),
		VTG_REG_2(25, MAX96717_VPG_DE_LOW),
		VTG_REG_2(27, MAX96717_VPG_DE_CNT),

		{MAX96717_REG_VTX0,
		 FIELD_PREP(MAX96717_VTX0_VTG_MODE_MASK, MAX96717_VTX0_VTG_FREE_RUNNING) |
			 MAX96717_VTX0_VS_INV | MAX96717_VTX0_GEN_DE | MAX96717_VTX0_GEN_HS |
			 MAX96717_VTX0_GEN_VS},
	};

#undef VTG_REG_3
#undef VTG_REG_2

	return video_write_cci_multiregs(i2c, regs, ARRAY_SIZE(regs));
}

static int max96717_write_gradient_pattern(const struct i2c_dt_spec *i2c)
{
	return video_write_cci_reg(i2c, MAX96717_REG_VTX30_GRAD_INCR, 16);
}

static int max96717_write_checkerboard_pattern(const struct i2c_dt_spec *i2c)
{
	const struct video_reg regs[] = {
		{MAX96717_REG_VTX31_CHKR_A_L, MAX96717_VPG_CHKR_A_R},
		{MAX96717_REG_VTX32_CHKR_A_M, MAX96717_VPG_CHKR_A_G},
		{MAX96717_REG_VTX33_CHKR_A_H, MAX96717_VPG_CHKR_A_B},
		{MAX96717_REG_VTX34_CHKR_B_L, MAX96717_VPG_CHKR_B_R},
		{MAX96717_REG_VTX35_CHKR_B_M, MAX96717_VPG_CHKR_B_G},
		{MAX96717_REG_VTX36_CHKR_B_H, MAX96717_VPG_CHKR_B_B},
		{MAX96717_REG_VTX37_CHKR_RPT_A, MAX96717_VPG_CHKR_RPT_A},
		{MAX96717_REG_VTX38_CHKR_RPT_B, MAX96717_VPG_CHKR_RPT_B},
		{MAX96717_REG_VTX39_CHKR_ALT, MAX96717_VPG_CHKR_ALT},
	};

	return video_write_cci_multiregs(i2c, regs, ARRAY_SIZE(regs));
}

static int max96717_configure_patgen(const struct i2c_dt_spec *i2c,
				     enum max96717_test_pattern pattern)
{
	int ret;

	/* PCLK = 25 MHz */
	ret = video_modify_cci_reg(
		i2c, MAX96717_REG_VTX1, MAX96717_VTX1_PATGEN_CLK_SRC_MASK,
		FIELD_PREP(MAX96717_VTX1_PATGEN_CLK_SRC_MASK, MAX96717_VTX1_PATGEN_CLK_25MHZ));
	if (ret < 0) {
		return ret;
	}

	ret = max96717_write_pattern_timings(i2c);
	if (ret < 0) {
		return ret;
	}

	if (pattern == MAX96717_TEST_PATTERN_CHECKERBOARD) {
		ret = max96717_write_checkerboard_pattern(i2c);
		if (ret < 0) {
			return ret;
		}

		return video_modify_cci_reg(
			i2c, MAX96717_REG_VTX29, MAX96717_VTX29_PATGEN_MODE_MASK,
			FIELD_PREP(MAX96717_VTX29_PATGEN_MODE_MASK, MAX96717_VTX29_PATGEN_CHECKER));
	}

	ret = max96717_write_gradient_pattern(i2c);
	if (ret < 0) {
		return ret;
	}

	return video_modify_cci_reg(
		i2c, MAX96717_REG_VTX29, MAX96717_VTX29_PATGEN_MODE_MASK,
		FIELD_PREP(MAX96717_VTX29_PATGEN_MODE_MASK, MAX96717_VTX29_PATGEN_GRADIENT));
}

static int max96717_set_vpg_mode(const struct i2c_dt_spec *i2c, enum max96717_test_pattern pattern)
{
	int ret;

	/* Disable tunnel mode. PATGEN only works in pixel mode. */
	ret = video_modify_cci_reg(i2c, MAX96717_REG_EXT11, MAX96717_EXT11_TUN_MODE, 0);
	if (ret < 0) {
		return ret;
	}

	ret = max96717_configure_patgen(i2c, pattern);
	if (ret < 0) {
		LOG_ERR("PATGEN configure failed (err %d)", ret);
		return ret;
	}

	/* Configure HW pipe 2 for RGB888 (24 BPP). This is the only supported format for VPG. */
	ret = video_modify_cci_reg(i2c, MAX96717_REG_VIDEO_Z_TX0, MAX96717_VIDEO_Z_TX0_AUTO_BPP, 0);
	if (ret < 0) {
		return ret;
	}

	ret = video_modify_cci_reg(i2c, MAX96717_REG_VIDEO_Z_TX1, MAX96717_VIDEO_Z_TX1_BPP_MASK,
				   FIELD_PREP(MAX96717_VIDEO_Z_TX1_BPP_MASK, 24));
	if (ret < 0) {
		return ret;
	}

	/* Drift detection off with the fixed bpp, see MAX96717_VIDEO_Z_TX2_DRIFT_DET_EN. */
	ret = video_modify_cci_reg(i2c, MAX96717_REG_VIDEO_Z_TX2, MAX96717_VIDEO_Z_TX2_DRIFT_DET_EN,
				   0);
	if (ret < 0) {
		return ret;
	}

	/* Set SOFT_BPP = 24 for RGB888. */
	ret = video_modify_cci_reg(
		i2c, MAX96717_REG_FRONTTOP_22,
		MAX96717_FRONTTOP_22_SOFT_BPPZ | MAX96717_FRONTTOP_22_SOFT_BPPZ_EN,
		FIELD_PREP(MAX96717_FRONTTOP_22_SOFT_BPPZ, 24) | MAX96717_FRONTTOP_22_SOFT_BPPZ_EN);
	if (ret < 0) {
		return ret;
	}

	/* Send the line start info frames and enable the CSI-2 port. */
	ret = video_modify_cci_reg(
		i2c, MAX96717_REG_FRONTTOP_0,
		MAX96717_FRONTTOP_0_ENABLE_LINE_INFO | MAX96717_FRONTTOP_0_START_PORTB,
		MAX96717_FRONTTOP_0_ENABLE_LINE_INFO | MAX96717_FRONTTOP_0_START_PORTB);
	if (ret < 0) {
		return ret;
	}

	/* Enable pipe Z from CSI port B (towards GMSL link) */
	ret = video_modify_cci_reg(i2c, MAX96717_REG_FRONTTOP_9, MAX96717_FRONTTOP_9_START_PORTBZ,
				   MAX96717_FRONTTOP_9_START_PORTBZ);
	if (ret < 0) {
		return ret;
	}

	/* Select RGB888 (0x24) in the first data-type filter slot of pipe Z. */
	ret = video_write_cci_reg(
		i2c, MAX96717_REG_FRONTTOP_16,
		FIELD_PREP(MAX96717_FRONTTOP_16_MEM_DT1_SELZ_DT, VIDEO_MIPI_CSI2_DT_RGB888) |
			MAX96717_FRONTTOP_16_MEM_DT1_SELZ_EN);
	if (ret < 0) {
		return ret;
	}

	/* Stream ID 0, matching the stream select of the deserializer's pipe 0. */
	ret = video_modify_cci_reg(i2c, MAX96717_REG_VIDEO_Z_TX3, MAX96717_VIDEO_Z_TX3_TX_STR_SEL,
				   FIELD_PREP(MAX96717_VIDEO_Z_TX3_TX_STR_SEL, 0));
	if (ret < 0) {
		return ret;
	}

	/* Enable video TX on HW pipe 2 so PATGEN data leaves over GMSL */
	ret = video_modify_cci_reg(i2c, MAX96717_REG_REG2, MAX96717_REG2_VID_TX_EN_Z,
				   MAX96717_REG2_VID_TX_EN_Z);
	if (ret < 0) {
		return ret;
	}

	LOG_DBG("MAX96717 PATGEN streaming on pipe %d (800x480 RGB888)", MAX96717_PIPE_HW_ID);

	return 0;
}

/* fourcc -> CSI-2 DT byte for the MAX96717 pixel-mode packet mapper. */
struct max96717_csi_fmt {
	uint32_t pixelformat;
	uint8_t csi_dt;
};

#define INPUT_FMT(fmt, dt) {.pixelformat = VIDEO_PIX_FMT_##fmt, .csi_dt = VIDEO_MIPI_CSI2_DT_##dt}

static const struct max96717_csi_fmt max96717_csi_fmts[] = {
	/* RGB */
	INPUT_FMT(RGB565, RGB565),
	INPUT_FMT(RGB24, RGB888),
	/* RAW8 Bayer */
	INPUT_FMT(SBGGR8, RAW8),
	INPUT_FMT(SGBRG8, RAW8),
	INPUT_FMT(SGRBG8, RAW8),
	INPUT_FMT(SRGGB8, RAW8),
	/* RAW10 Bayer (packed + unpacked) */
	INPUT_FMT(SBGGR10P, RAW10),
	INPUT_FMT(SGBRG10P, RAW10),
	INPUT_FMT(SGRBG10P, RAW10),
	INPUT_FMT(SRGGB10P, RAW10),
	INPUT_FMT(SBGGR10, RAW10),
	INPUT_FMT(SGBRG10, RAW10),
	INPUT_FMT(SGRBG10, RAW10),
	INPUT_FMT(SRGGB10, RAW10),
};

static int max96717_pixfmt_to_csi2_dt(uint32_t pixfmt, uint8_t *dt)
{
	for (size_t i = 0; i < ARRAY_SIZE(max96717_csi_fmts); i++) {
		if (max96717_csi_fmts[i].pixelformat == pixfmt) {
			*dt = max96717_csi_fmts[i].csi_dt;
			return 0;
		}
	}
	return -ENOTSUP;
}

/*
 * Sets the lane count from DT and enables non-continuous clock mode (required by sensors like
 * the IMX219).
 */
static int max96717_configure_csi_rx_phy(const struct i2c_dt_spec *i2c, uint8_t num_lanes)
{
	int ret;

	/* Set CSI-2 data lane count (register encodes num_lanes - 1) */
	ret = video_modify_cci_reg(i2c, MAX96717_REG_MIPI_RX1, MAX96717_MIPI_RX1_CTRL_NUM_LANES,
				   FIELD_PREP(MAX96717_MIPI_RX1_CTRL_NUM_LANES, num_lanes - 1));
	if (ret < 0) {
		return ret;
	}

	/*
	 * TODO: Read clock-noncontinuous from the CSI-2 input endpoint DT property instead of
	 * unconditionally enabling it. Requires adding clock-noncontinuous to Zephyr's
	 * video-interfaces.yaml first.
	 */
	ret = video_modify_cci_reg(i2c, MAX96717_REG_MIPI_RX0, MAX96717_MIPI_RX0_NONCONTCLK_EN,
				   MAX96717_MIPI_RX0_NONCONTCLK_EN);
	if (ret < 0) {
		return ret;
	}

	LOG_DBG("MIPI RX PHY: %u data lanes, non-continuous clock", num_lanes);

	return 0;
}

/* Configure the serializer to pixel mode. */
static int max96717_configure_csi_passthrough(const struct i2c_dt_spec *i2c, uint8_t csi2_dt,
					      uint8_t num_lanes)
{
	int ret;
	const bool double_raw10 = IS_ENABLED(CONFIG_VIDEO_GMSL_RAW10_DOUBLE) &&
				  csi2_dt == VIDEO_MIPI_CSI2_DT_RAW10;

	/* Configure CSI-2 RX PHY for external sensor input */
	ret = max96717_configure_csi_rx_phy(i2c, num_lanes);
	if (ret < 0) {
		return ret;
	}

	/* Disable tunnel mode (enable pixel mode) */
	ret = video_modify_cci_reg(i2c, MAX96717_REG_EXT11, MAX96717_EXT11_TUN_MODE, 0);
	if (ret < 0) {
		return ret;
	}

	/* Enable AUTO_BPP; detect from CSI-2 input */
	ret = video_modify_cci_reg(i2c, MAX96717_REG_VIDEO_Z_TX0, MAX96717_VIDEO_Z_TX0_AUTO_BPP,
				   MAX96717_VIDEO_Z_TX0_AUTO_BPP);
	if (ret < 0) {
		return ret;
	}

	/*
	 * Heartbeat off during blanking (LIM_HEART). Drift detection stays on with AUTO_BPP and
	 * goes off with the fixed bpp of doubled RAW10, see MAX96717_VIDEO_Z_TX2_DRIFT_DET_EN.
	 */
	ret = video_modify_cci_reg(i2c, MAX96717_REG_VIDEO_Z_TX2,
				   MAX96717_VIDEO_Z_TX2_LIM_HEART |
					   MAX96717_VIDEO_Z_TX2_DRIFT_DET_EN,
				   MAX96717_VIDEO_Z_TX2_LIM_HEART |
					   (double_raw10 ? 0 : MAX96717_VIDEO_Z_TX2_DRIFT_DET_EN));
	if (ret < 0) {
		return ret;
	}

	/*
	 * Bit-depth doubling for RAW10: two 10-bit pixels per 20-bit GMSL word
	 * (bpp10dblz plus a soft bpp of 20), unpacked by the deserializer's
	 * alternate memory map. It halves the pixel clock the deserializer has
	 * to process. Other formats keep AUTO_BPP.
	 */
	ret = video_modify_cci_reg(i2c, MAX96717_REG_FRONTTOP_11, MAX96717_FRONTTOP_11_BPP10DBLZ,
				   double_raw10 ? MAX96717_FRONTTOP_11_BPP10DBLZ : 0);
	if (ret < 0) {
		return ret;
	}

	ret = video_modify_cci_reg(i2c, MAX96717_REG_FRONTTOP_22,
				   MAX96717_FRONTTOP_22_SOFT_BPPZ_EN |
					   MAX96717_FRONTTOP_22_SOFT_BPPZ,
				   double_raw10 ? (MAX96717_FRONTTOP_22_SOFT_BPPZ_EN |
						   FIELD_PREP(MAX96717_FRONTTOP_22_SOFT_BPPZ, 20))
						: 0);
	if (ret < 0) {
		return ret;
	}

	/*
	 * Route only the video data type to pipe Z: FRONTTOP_16.mem_dt1_selz is a data-type
	 * filter, and with no filter enabled every data type reaches the pipe.
	 */
	ret = video_write_cci_reg(i2c, MAX96717_REG_FRONTTOP_16,
				  FIELD_PREP(MAX96717_FRONTTOP_16_MEM_DT1_SELZ_DT, csi2_dt) |
					  MAX96717_FRONTTOP_16_MEM_DT1_SELZ_EN);
	if (ret < 0) {
		return ret;
	}

	/* Ensure CSI port is enabled and line-start info-frames are sent. */
	ret = video_modify_cci_reg(
		i2c, MAX96717_REG_FRONTTOP_0,
		MAX96717_FRONTTOP_0_ENABLE_LINE_INFO | MAX96717_FRONTTOP_0_START_PORTB,
		MAX96717_FRONTTOP_0_ENABLE_LINE_INFO | MAX96717_FRONTTOP_0_START_PORTB);
	if (ret < 0) {
		return ret;
	}

	/* Start video stream. */
	ret = video_modify_cci_reg(i2c, MAX96717_REG_FRONTTOP_9, MAX96717_FRONTTOP_9_START_PORTBZ,
				   MAX96717_FRONTTOP_9_START_PORTBZ);
	if (ret < 0) {
		return ret;
	}

	/* Stream ID 0, matching the stream select of the deserializer's pipe 0. */
	ret = video_modify_cci_reg(i2c, MAX96717_REG_VIDEO_Z_TX3, MAX96717_VIDEO_Z_TX3_TX_STR_SEL,
				   FIELD_PREP(MAX96717_VIDEO_Z_TX3_TX_STR_SEL, 0));
	if (ret < 0) {
		return ret;
	}

	/* Enable video TX on the pipe */
	ret = video_modify_cci_reg(i2c, MAX96717_REG_REG2, MAX96717_REG2_VID_TX_EN_Z,
				   MAX96717_REG2_VID_TX_EN_Z);
	if (ret < 0) {
		return ret;
	}

	LOG_DBG("MAX96717 CSI-2 passthrough on pipe %d (DT=0x%02X)", MAX96717_PIPE_HW_ID, csi2_dt);

	return 0;
}

static bool max96717_test_pattern_on(const struct max96717_data *data)
{
	return data->test_pattern.val != MAX96717_TEST_PATTERN_OFF;
}

static bool max96717_is_vpg_format(const struct video_format *fmt)
{
	return fmt->pixelformat == VIDEO_PIX_FMT_RGB24 && fmt->width == MAX96717_VPG_HACTIVE &&
	       fmt->height == MAX96717_VPG_VACTIVE;
}

static int max96717_get_caps(const struct device *dev, struct video_caps *caps)
{
	const struct max96717_config *cfg = dev->config;
	const struct max96717_data *data = dev->data;

	/* The video is the sensor's; the pattern generator has its own format. */
	if (cfg->source_dev != NULL && !max96717_test_pattern_on(data)) {
		return video_get_caps(cfg->source_dev, caps);
	}

	caps->format_caps = max96717_fmts;
	caps->min_vbuf_count = 1;
	return 0;
}

static int max96717_get_format(const struct device *dev, struct video_format *fmt)
{
	const struct max96717_config *cfg = dev->config;
	struct max96717_data *data = dev->data;

	/* The pattern generator's format, whatever was set before the pattern was selected. */
	if (max96717_test_pattern_on(data)) {
		*fmt = max96717_vpg_fmt;
		return 0;
	}

	if (cfg->source_dev != NULL) {
		return video_get_format(cfg->source_dev, fmt);
	}

	*fmt = data->fmt;
	return 0;
}

static int max96717_set_format(const struct device *dev, struct video_format *fmt)
{
	const struct max96717_config *cfg = dev->config;
	struct max96717_data *data = dev->data;
	int ret;

	if (max96717_test_pattern_on(data)) {
		/* The pattern generator emits its fixed format; the sensor is not involved. */
		if (!max96717_is_vpg_format(fmt)) {
			LOG_ERR("Format %s %ux%u not supported by the pattern generator",
				VIDEO_FOURCC_TO_STR(fmt->pixelformat), fmt->width, fmt->height);
			return -ENOTSUP;
		}
	} else if (cfg->source_dev != NULL) {
		ret = video_set_format(cfg->source_dev, fmt);
		if (ret < 0) {
			return ret;
		}
	}

	data->fmt = *fmt;
	return 0;
}

static int max96717_set_stream(const struct device *dev, bool enable, enum video_buf_type type)
{
	const struct max96717_config *cfg = dev->config;
	struct max96717_data *data = dev->data;
	const bool patgen = max96717_test_pattern_on(data);
	uint8_t data_dt;
	int ret;

	if (enable == data->streaming) {
		return 0;
	}

	if (!enable) {
		/* Stop the sensor first, then the video TX that carried it. */
		ret = 0;
		if (cfg->source_dev != NULL && !patgen) {
			ret = video_stream_stop(cfg->source_dev, type);
		}

		video_modify_cci_reg(&cfg->i2c, MAX96717_REG_REG2, MAX96717_REG2_VID_TX_EN_Z, 0);

		if (patgen) {
			video_modify_cci_reg(&cfg->i2c, MAX96717_REG_VTX29,
					     MAX96717_VTX29_PATGEN_MODE_MASK,
					     FIELD_PREP(MAX96717_VTX29_PATGEN_MODE_MASK,
							MAX96717_VTX29_PATGEN_DISABLED));
		}

		data->streaming = false;
		return ret;
	}

	if (patgen) {
		ret = max96717_set_vpg_mode(&cfg->i2c, data->test_pattern.val);
		if (ret < 0) {
			LOG_ERR("PATGEN stream configure failed (err %d)", ret);
			return ret;
		}

		data->streaming = true;
		return 0;
	}

	if (cfg->source_dev == NULL) {
		LOG_ERR("No sensor and no test pattern selected");
		return -EINVAL;
	}

	/* Map the sensor's format, whatever was set while the pattern generator was on. */
	ret = video_get_format(cfg->source_dev, &data->fmt);
	if (ret < 0) {
		LOG_ERR("Failed to get the sensor format (err %d)", ret);
		return ret;
	}

	ret = max96717_pixfmt_to_csi2_dt(data->fmt.pixelformat, &data_dt);
	if (ret < 0) {
		LOG_ERR("Unsupported pixel format 0x%08x for MIPI map", data->fmt.pixelformat);
		return ret;
	}

	if (data_dt == 0) {
		LOG_ERR("Unsupported format %s for CSI-2 passthrough",
			VIDEO_FOURCC_TO_STR(data->fmt.pixelformat));
		return -ENOTSUP;
	}

	ret = max96717_configure_csi_passthrough(&cfg->i2c, data_dt, cfg->csi_rx_num_lanes);
	if (ret < 0) {
		LOG_ERR("CSI passthrough configure failed (err %d)", ret);
		return ret;
	}

	ret = video_stream_start(cfg->source_dev, type);
	if (ret < 0) {
		LOG_ERR("Failed to start source (err %d)", ret);

		/* The sensor did not start: take the video TX down again. */
		video_modify_cci_reg(&cfg->i2c, MAX96717_REG_REG2, MAX96717_REG2_VID_TX_EN_Z, 0);
		return ret;
	}

	data->streaming = true;
	return 0;
}

static int max96717_set_ctrl(const struct device *dev, uint32_t id)
{
	const struct max96717_data *data = dev->data;

	switch (id) {
	case VIDEO_CID_TEST_PATTERN:
		/* Applied at the next stream start, which sets the whole pipe up around it. */
		return data->streaming ? -EBUSY : 0;
	default:
		return -ENOTSUP;
	}
}

static int max96717_set_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct max96717_config *cfg = dev->config;

	if (cfg->source_dev == NULL) {
		return -ENOSYS;
	}

	return video_set_frmival(cfg->source_dev, frmival);
}

static int max96717_get_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct max96717_config *cfg = dev->config;

	if (cfg->source_dev == NULL) {
		return -ENOSYS;
	}

	return video_get_frmival(cfg->source_dev, frmival);
}

static int max96717_enum_frmival(const struct device *dev, struct video_frmival_enum *fie)
{
	const struct max96717_config *cfg = dev->config;

	if (cfg->source_dev == NULL) {
		return -ENOSYS;
	}

	return video_enum_frmival(cfg->source_dev, fie);
}

static DEVICE_API(video, max96717_api) = {
	.set_ctrl = max96717_set_ctrl,
	.get_caps = max96717_get_caps,
	.get_format = max96717_get_format,
	.set_format = max96717_set_format,
	.set_stream = max96717_set_stream,
	.set_frmival = max96717_set_frmival,
	.get_frmival = max96717_get_frmival,
	.enum_frmival = max96717_enum_frmival,
};

static int max96717_init(const struct device *dev)
{
	const struct max96717_config *cfg = dev->config;
	struct max96717_data *data = dev->data;
	int ret;

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}

	ret = max96717_reset_device(&cfg->i2c);
	if (ret < 0) {
		return ret;
	}

	max96717_log_device_id(&cfg->i2c);

	ret = max96717_check_link_lock(&cfg->i2c);
	if (ret < 0) {
		return ret;
	}

	/* PFDDIV regulator voltage control. */
	ret = video_modify_cci_reg(
		&cfg->i2c, MAX96717_REG_CMU2, MAX96717_CMU2_PFDDIV_RSHORT_MASK,
		FIELD_PREP(MAX96717_CMU2_PFDDIV_RSHORT_MASK, MAX96717_CMU2_PFDDIV_RSHORT_1_1V));
	if (ret < 0) {
		LOG_ERR("Failed to set the PFDDIV regulator voltage (err %d)", ret);
		return ret;
	}

	if (cfg->source_dev == NULL) {
		/* No sensor: the pattern generator's format until set_format() is called. */
		data->fmt = max96717_vpg_fmt;
	}

	return video_init_menu_ctrl(&data->test_pattern, dev, VIDEO_CID_TEST_PATTERN,
				    MAX96717_TEST_PATTERN_OFF, max96717_test_pattern_menu);
}

/*
 * With no sensor on port 0, or a disabled one, the serializer is the source: its pattern
 * generator.
 */
#define SOURCE_DEV(inst)                                                                           \
	COND_CODE_1(                                                                               \
		DT_NODE_EXISTS(DT_INST_ENDPOINT_BY_ID(inst, 0, 0)),                                \
		(DEVICE_DT_GET_OR_NULL(DT_NODE_REMOTE_DEVICE(DT_INST_ENDPOINT_BY_ID(inst, 0, 0)))),\
		(NULL))

/* CSI-2 data lanes of the sensor on port 0, 0 without a sensor. */
#define CSI_RX_NUM_LANES(inst)                                                                     \
	COND_CODE_1(                                                                               \
		DT_NODE_EXISTS(DT_INST_ENDPOINT_BY_ID(inst, 0, 0)),                                \
		(DT_PROP_LEN_OR(DT_INST_ENDPOINT_BY_ID(inst, 0, 0), data_lanes, 0)),              \
		(0))

#define MAX96717_INIT(inst)                                                                        \
	BUILD_ASSERT(!DT_NODE_EXISTS(DT_INST_ENDPOINT_BY_ID(inst, 0, 0)) ||                        \
			     IN_RANGE(CSI_RX_NUM_LANES(inst), 1, 4),                               \
		     "MAX96717 CSI-2 input data-lanes must list 1 to 4 lanes");                    \
                                                                                                   \
	static const struct max96717_config max96717_config_##inst = {                             \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.source_dev = SOURCE_DEV(inst),                                                    \
		.csi_rx_num_lanes = CSI_RX_NUM_LANES(inst),                                        \
	};                                                                                         \
	static struct max96717_data max96717_data_##inst;                                          \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, max96717_init, NULL, &max96717_data_##inst,                    \
			      &max96717_config_##inst, POST_KERNEL,                                \
			      CONFIG_VIDEO_MAX96717_INIT_PRIORITY, &max96717_api);                 \
                                                                                                   \
	VIDEO_DEVICE_DEFINE(max96717_##inst, DEVICE_DT_INST_GET(inst), SOURCE_DEV(inst));

DT_INST_FOREACH_STATUS_OKAY(MAX96717_INIT)
