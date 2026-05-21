/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 LANDROVAL
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * STMicroelectronics VD56G3 / VB56G3 1.5 MP monochrome global shutter
 * CMOS image sensor driver (DS12968 datasheet; ST Linux V4L2 reference).
 */

#define DT_DRV_COMPAT st_vd56g3

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video-controls.h>
#include <zephyr/dt-bindings/video/video-interfaces.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "video_common.h"
#include "video_ctrls.h"
#include "video_device.h"

LOG_MODULE_REGISTER(video_vd56g3, CONFIG_VIDEO_LOG_LEVEL);

/* Default polling deadline for sensor command ACKs (ms). */
#define VD56G3_CMD_POLL_TIMEOUT_MS 500U
#define VD56G3_FSM_POLL_TIMEOUT_MS 500U
/* Linux driver waits ~3.5 ms after XSHUTDOWN; allow margin for CLKIN/OTP. */
#define VD56G3_BOOT_POLL_TIMEOUT_MS 3500U

/* Register map (User Interface bank, ST Linux driver). */

#define VD56G3_REG8(addr)  ((uint32_t)(addr) | VIDEO_REG_ADDR16_DATA8)
#define VD56G3_REG16(addr) ((uint32_t)(addr) | VIDEO_REG_ADDR16_DATA16_LE)
#define VD56G3_REG32(addr) ((uint32_t)(addr) | VIDEO_REG_ADDR16_DATA32_LE)

/* Identification */
#define VD56G3_REG_MODEL_ID                 VD56G3_REG16(0x0000)
#define VD56G3_MODEL_ID_VAL                 0x5603

#define VD56G3_REG_REVISION                 VD56G3_REG16(0x0002)
#define VD56G3_REVISION_CUT2                0x20
#define VD56G3_REVISION_CUT3                0x31

#define VD56G3_REG_OPTICAL_REVISION         VD56G3_REG8(0x001a)
#define VD56G3_OPTICAL_REVISION_MONO        0
#define VD56G3_OPTICAL_REVISION_BAYER       1
#define VD56G3_OPTICAL_REVISION_RGBNIR      2

/* System FSM (read) and command registers (write) */
#define VD56G3_REG_SYSTEM_FSM               VD56G3_REG8(0x0028)
#define VD56G3_SYSTEM_FSM_READY_TO_BOOT     0x01
#define VD56G3_SYSTEM_FSM_SW_STBY           0x02
#define VD56G3_SYSTEM_FSM_STREAMING         0x03

#define VD56G3_REG_TEMPERATURE              VD56G3_REG16(0x004c)
#define VD56G3_REG_APPLIED_COARSE_EXPOSURE  VD56G3_REG16(0x0064)
#define VD56G3_REG_APPLIED_ANALOG_GAIN      VD56G3_REG8(0x0068)
#define VD56G3_REG_APPLIED_DIGITAL_GAIN     VD56G3_REG16(0x006a)

/* Boot command. Write CMD_BOOT to leave READY_TO_BOOT. The MCU acks by
 * clearing the register back to CMD_ACK (0).
 */
#define VD56G3_REG_BOOT                     VD56G3_REG8(0x0200)
#define VD56G3_CMD_ACK                      0
#define VD56G3_CMD_BOOT                     1

#define VD56G3_REG_STBY                     VD56G3_REG8(0x0201)
#define VD56G3_CMD_START_STREAM             1
#define VD56G3_CMD_THSENS_READ              4

#define VD56G3_REG_STREAMING                VD56G3_REG8(0x0202)
#define VD56G3_CMD_STOP_STREAM              1

/* Clock / PLL */
#define VD56G3_REG_EXT_CLOCK                VD56G3_REG32(0x0220)
#define VD56G3_REG_CLK_PLL_PREDIV           VD56G3_REG8(0x0224)
#define VD56G3_REG_CLK_SYS_PLL_MULT         VD56G3_REG8(0x0226)

/* Output interface (MIPI CSI-2) */
#define VD56G3_REG_VT_CTRL                  VD56G3_REG8(0x0309)
#define VD56G3_REG_FORMAT_CTRL              VD56G3_REG8(0x030a)
#define VD56G3_FORMAT_RAW8                  8
#define VD56G3_FORMAT_RAW10                 10

#define VD56G3_REG_OIF_CTRL                 VD56G3_REG16(0x030c)
/* OIF_CTRL bit layout (mirrors the Linux driver, vd56g3.c:2171)
 *   [1:0]   nb_of_lanes (1 or 2)
 *   [3]     lane0 polarity inversion
 *   [5:4]   physical lane index for logical lane 0
 *   [6]     lane1 polarity inversion
 *   [8:7]   physical lane index for logical lane 1
 *   [9]     clock lane polarity inversion
 */
#define VD56G3_OIF_CTRL_LANES_MASK          GENMASK(1, 0)

#define VD56G3_REG_OIF_IMG_CTRL             VD56G3_REG8(0x030f)
#define VD56G3_OIF_IMG_CTRL_DT_RAW8         0x2A
#define VD56G3_OIF_IMG_CTRL_DT_RAW10        0x2B

#define VD56G3_REG_OIF_CSI_BITRATE          VD56G3_REG16(0x0312)

/* Image processing modules */
#define VD56G3_REG_DUSTER_CTRL              VD56G3_REG8(0x0318)
#define VD56G3_DUSTER_DISABLE               0
#define VD56G3_DUSTER_ENABLE_DEF_MODULES    0x13

#define VD56G3_REG_ISL_ENABLE               VD56G3_REG8(0x0333)

#define VD56G3_REG_DARKCAL_CTRL             VD56G3_REG8(0x0340)
#define VD56G3_DARKCAL_ENABLE               1
#define VD56G3_DARKCAL_DISABLE_DARKAVG      2

#define VD56G3_REG_DARKCAL_PEDESTAL         VD56G3_REG8(0x0415)

/* Test pattern generator */
#define VD56G3_REG_PATGEN_CTRL              VD56G3_REG16(0x0400)
#define VD56G3_PATGEN_ENABLE                1
#define VD56G3_PATGEN_TYPE_SHIFT            4

enum vd56g3_test_pattern {
	VD56G3_TP_DISABLED = 0,
	VD56G3_TP_SOLID    = 1,
	VD56G3_TP_COLORBAR = 2,
	VD56G3_TP_GRADBAR  = 3,
	VD56G3_TP_HGREY    = 4,
	VD56G3_TP_VGREY    = 5,
	VD56G3_TP_DGREY    = 6,
	VD56G3_TP_PN28     = 7,
};

/* Auto-exposure (AE) and manual exposure / gain */
#define VD56G3_REG_AE_COLDSTART_COARSE_EXPOSURE  VD56G3_REG16(0x042a)
#define VD56G3_REG_AE_COLDSTART_ANALOG_GAIN      VD56G3_REG8(0x042c)
#define VD56G3_REG_AE_COLDSTART_DIGITAL_GAIN     VD56G3_REG16(0x042e)
#define VD56G3_REG_AE_COMPILER_CONTROL           VD56G3_REG8(0x0430)
#define VD56G3_REG_AE_ROI_START_H                VD56G3_REG16(0x0432)
#define VD56G3_REG_AE_ROI_START_V                VD56G3_REG16(0x0434)
#define VD56G3_REG_AE_ROI_END_H                  VD56G3_REG16(0x0436)
#define VD56G3_REG_AE_ROI_END_V                  VD56G3_REG16(0x0438)
#define VD56G3_REG_AE_COMPENSATION               VD56G3_REG16(0x043a)
#define VD56G3_REG_AE_TARGET_PERCENTAGE          VD56G3_REG16(0x043c)
#define VD56G3_REG_AE_STEP_PROPORTION            VD56G3_REG16(0x043e)
#define VD56G3_REG_AE_LEAK_PROPORTION            VD56G3_REG16(0x0440)

#define VD56G3_REG_EXP_MODE                      VD56G3_REG8(0x044c)
#define VD56G3_EXP_MODE_AUTO                     0
#define VD56G3_EXP_MODE_FREEZE                   1
#define VD56G3_EXP_MODE_MANUAL                   2

#define VD56G3_REG_MANUAL_ANALOG_GAIN            VD56G3_REG8(0x044d)
#define VD56G3_REG_MANUAL_COARSE_EXPOSURE        VD56G3_REG16(0x044e)
#define VD56G3_REG_MANUAL_DIGITAL_GAIN_CH0       VD56G3_REG16(0x0450)
#define VD56G3_REG_MANUAL_DIGITAL_GAIN_CH1       VD56G3_REG16(0x0452)
#define VD56G3_REG_MANUAL_DIGITAL_GAIN_CH2       VD56G3_REG16(0x0454)
#define VD56G3_REG_MANUAL_DIGITAL_GAIN_CH3       VD56G3_REG16(0x0456)

/* Frame timing and ROI */
#define VD56G3_REG_FRAME_LENGTH             VD56G3_REG16(0x0458)
#define VD56G3_REG_Y_START                  VD56G3_REG16(0x045a)
#define VD56G3_REG_Y_END                    VD56G3_REG16(0x045c)
#define VD56G3_REG_OUT_ROI_X_START          VD56G3_REG16(0x045e)
#define VD56G3_REG_OUT_ROI_X_END            VD56G3_REG16(0x0460)
#define VD56G3_REG_OUT_ROI_Y_START          VD56G3_REG16(0x0462)
#define VD56G3_REG_OUT_ROI_Y_END            VD56G3_REG16(0x0464)

/* Image orientation / readout */
#define VD56G3_REG_ORIENTATION              VD56G3_REG8(0x0302)
#define VD56G3_ORIENTATION_NORMAL           0x00
#define VD56G3_ORIENTATION_HFLIP            BIT(0)
#define VD56G3_ORIENTATION_VFLIP            BIT(1)

#define VD56G3_REG_GPIO_0_CTRL              VD56G3_REG8(0x0467)
#define VD56G3_GPIOX_FSYNC_OUT              0x00
#define VD56G3_GPIOX_GPIO_IN                0x01
#define VD56G3_GPIOX_STROBE_MODE            0x02
#define VD56G3_GPIOX_VT_SLAVE_MODE          0x0a
#define VD56G3_NB_GPIOS                     8

#define VD56G3_REG_READOUT_CTRL             VD56G3_REG8(0x047e)
#define VD56G3_READOUT_NORMAL               0x00
#define VD56G3_READOUT_DIGITAL_BINNING_X2   0x01

/* Geometry / timing constants (datasheet §2.2 and §3.8, Linux driver) */
#define VD56G3_NATIVE_WIDTH                 1124
#define VD56G3_NATIVE_HEIGHT                1364

#define VD56G3_DEFAULT_WIDTH                1120
#define VD56G3_DEFAULT_HEIGHT               1360

#define VD56G3_BIN_X2_WIDTH                 (VD56G3_NATIVE_WIDTH / 2)
#define VD56G3_BIN_X2_HEIGHT                (VD56G3_NATIVE_HEIGHT / 2)

#define VD56G3_CROP_720x1280_W              720
#define VD56G3_CROP_720x1280_H              1280

#define VD56G3_CROP_VGA_W                   640
#define VD56G3_CROP_VGA_H                   480

#define VD56G3_LINE_LENGTH_MIN              1236
#define VD56G3_VBLANK_MIN                   110
#define VD56G3_FRAME_LENGTH_DEF_60FPS       2168
#define VD56G3_FRAME_LENGTH_MAX             0xffffu

#define VD56G3_EXPOSURE_MARGIN              75
#define VD56G3_EXPOSURE_MIN                 21u
#define VD56G3_EXPOSURE_DEFAULT             1420u

/* PLL settings (Linux driver vd56g3.c:206-) */
#define VD56G3_TARGET_PLL                   804000000UL
#define VD56G3_VT_CLOCK_DIV                 5

/* Output interface - default link rates from the Linux driver */
#define VD56G3_LINK_FREQ_DEF_1LANE_HZ       750000000UL
#define VD56G3_LINK_FREQ_DEF_2LANES_HZ      400000000UL

#define VD56G3_MAX_CSI_DATA_LANES           2

/* CLKIN range supported by the sensor (datasheet §3.3). */
#define VD56G3_CLKIN_MIN_HZ                 (6u * 1000000u)
#define VD56G3_CLKIN_MAX_HZ                 (27u * 1000000u)
#define VD56G3_CLKIN_DEFAULT_HZ             (12u * 1000000u)

/* Analog gain code is encoded as: gain = 32 / (32 - code), code in [0..28]. */
#define VD56G3_AGAIN_CODE_MIN               0
#define VD56G3_AGAIN_CODE_MAX               28

/* Digital gain Q5.8 fixed-point representation [0x100, 0x800] = [1.0, 8.0]. */
#define VD56G3_DGAIN_MIN                    0x100
#define VD56G3_DGAIN_MAX                    0x800
#define VD56G3_DGAIN_DEFAULT                0x100

/* Resolution and framerate tables */

enum vd56g3_res {
	VD56G3_RES_FULL_RAW10,    /* 1124 x 1364 RAW10 packed (native)        */
	VD56G3_RES_FULL_RAW8,     /* 1124 x 1364 RAW8        (native)         */
	VD56G3_RES_DEFAULT_RAW10, /* 1120 x 1360 RAW10 packed (16-aligned)    */
	VD56G3_RES_DEFAULT_RAW8,  /* 1120 x 1360 RAW8        (16-aligned)     */
	VD56G3_RES_BIN2_RAW10,    /*  562 x  682 RAW10 packed (binning x2)    */
	VD56G3_RES_CROP_720x1280, /*  720 x 1280 RAW10 packed (centered crop) */
	VD56G3_RES_CROP_VGA,      /*  640 x  480 RAW10 packed (centered crop) */
	VD56G3_RES_COUNT,
};

enum vd56g3_fps {
	VD56G3_FPS_210,
	VD56G3_FPS_121,
	VD56G3_FPS_88,
	VD56G3_FPS_60,
	VD56G3_FPS_30,
	VD56G3_FPS_20,
	VD56G3_FPS_15,
	VD56G3_FPS_10,
	VD56G3_FPS_COUNT,
};

static const uint32_t vd56g3_framerates[VD56G3_FPS_COUNT] = {
	[VD56G3_FPS_210] = 210,
	[VD56G3_FPS_121] = 121,
	[VD56G3_FPS_88]  = 88,
	[VD56G3_FPS_60]  = 60,
	[VD56G3_FPS_30]  = 30,
	[VD56G3_FPS_20]  = 20,
	[VD56G3_FPS_15]  = 15,
	[VD56G3_FPS_10]  = 10,
};

static const struct video_format_cap vd56g3_fmts[] = {
	[VD56G3_RES_FULL_RAW10] = {
		.pixelformat = VIDEO_PIX_FMT_Y10P,
		.width_min   = VD56G3_NATIVE_WIDTH,
		.width_max   = VD56G3_NATIVE_WIDTH,
		.height_min  = VD56G3_NATIVE_HEIGHT,
		.height_max  = VD56G3_NATIVE_HEIGHT,
	},
	[VD56G3_RES_FULL_RAW8] = {
		.pixelformat = VIDEO_PIX_FMT_GREY,
		.width_min   = VD56G3_NATIVE_WIDTH,
		.width_max   = VD56G3_NATIVE_WIDTH,
		.height_min  = VD56G3_NATIVE_HEIGHT,
		.height_max  = VD56G3_NATIVE_HEIGHT,
	},
	[VD56G3_RES_DEFAULT_RAW10] = {
		.pixelformat = VIDEO_PIX_FMT_Y10P,
		.width_min   = VD56G3_DEFAULT_WIDTH,
		.width_max   = VD56G3_DEFAULT_WIDTH,
		.height_min  = VD56G3_DEFAULT_HEIGHT,
		.height_max  = VD56G3_DEFAULT_HEIGHT,
	},
	[VD56G3_RES_DEFAULT_RAW8] = {
		.pixelformat = VIDEO_PIX_FMT_GREY,
		.width_min   = VD56G3_DEFAULT_WIDTH,
		.width_max   = VD56G3_DEFAULT_WIDTH,
		.height_min  = VD56G3_DEFAULT_HEIGHT,
		.height_max  = VD56G3_DEFAULT_HEIGHT,
	},
	[VD56G3_RES_BIN2_RAW10] = {
		.pixelformat = VIDEO_PIX_FMT_Y10P,
		.width_min   = VD56G3_BIN_X2_WIDTH,
		.width_max   = VD56G3_BIN_X2_WIDTH,
		.height_min  = VD56G3_BIN_X2_HEIGHT,
		.height_max  = VD56G3_BIN_X2_HEIGHT,
	},
	[VD56G3_RES_CROP_720x1280] = {
		.pixelformat = VIDEO_PIX_FMT_Y10P,
		.width_min   = VD56G3_CROP_720x1280_W,
		.width_max   = VD56G3_CROP_720x1280_W,
		.height_min  = VD56G3_CROP_720x1280_H,
		.height_max  = VD56G3_CROP_720x1280_H,
	},
	[VD56G3_RES_CROP_VGA] = {
		.pixelformat = VIDEO_PIX_FMT_Y10P,
		.width_min   = VD56G3_CROP_VGA_W,
		.width_max   = VD56G3_CROP_VGA_W,
		.height_min  = VD56G3_CROP_VGA_H,
		.height_max  = VD56G3_CROP_VGA_H,
	},
	{0},
};

/* Maximum framerate (Hz) per resolution (datasheet Table 3 / Linux driver). */
static const uint32_t vd56g3_max_fps[VD56G3_RES_COUNT] = {
	[VD56G3_RES_FULL_RAW10]    = 88,
	[VD56G3_RES_FULL_RAW8]     = 88,
	[VD56G3_RES_DEFAULT_RAW10] = 88,
	[VD56G3_RES_DEFAULT_RAW8]  = 88,
	[VD56G3_RES_BIN2_RAW10]    = 121,
	[VD56G3_RES_CROP_720x1280] = 88,
	[VD56G3_RES_CROP_VGA]      = 210,
};

/* Driver-private types */

struct vd56g3_ctrls {
	struct video_ctrl exposure_auto;
	struct video_ctrl gain;       /* analog gain code [0..28] */
	struct video_ctrl exposure;   /* coarse exposure in lines */
	struct video_ctrl digital_gain;
	struct video_ctrl ae_bias;
	struct video_ctrl hflip;
	struct video_ctrl vflip;
	struct video_ctrl link_freq;
	struct video_ctrl pixel_rate;
	struct video_ctrl test_pattern;
};

/* EV bias menu (milli-EV), aligned with the Linux driver. */
static const int64_t vd56g3_ev_bias_qmenu[] = {
	-4000, -3500, -3000, -2500, -2000, -1500, -1000, -500, 0,
	500, 1000, 1500, 2000, 2500, 3000, 3500, 4000,
};

struct vd56g3_config {
	struct i2c_dt_spec i2c;
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
	struct gpio_dt_spec reset_gpio;
#endif
	uint32_t input_clk;
	uint8_t  num_lanes;
	/* CSI-2 D-PHY polarity inversion (P/N swap) */
	bool     csi_lane0_polarity_invert;
	bool     csi_lane1_polarity_invert;
	bool     csi_clock_polarity_invert;
	/* External Vertical Timing sync (sensor pin indices, not Zephyr gpios) */
	uint8_t  out_sync_mask;	/* bitmask of sensor GPIOs as FSYNC_OUT (leader) */
	bool     in_sync;	/* sensor GPIO0 in VT_SLAVE_MODE (follower)      */
};

struct vd56g3_data {
	struct vd56g3_ctrls ctrls;
	struct video_format fmt;
	uint32_t frame_rate;
	bool enabled;
	bool is_fastboot;     /* cut3 silicon (fastboot boot path) */
	uint8_t pll_prediv;
	uint8_t pll_mult;
	uint64_t pixel_clock_hz;
	int64_t link_freq_menu[1];
	uint32_t link_freq_hz;
};

static bool vd56g3_is_ae_auto(const struct vd56g3_data *drv_data)
{
	/* Boolean auto cluster: 1 = automatic exposure, 0 = manual. */
	return drv_data->ctrls.exposure_auto.val != 0;
}

/* Low-level helpers */

/**
 * Poll a register until it equals @target_val, give up after @timeout_ms.
 * Used both for FSM transitions (read SYSTEM_FSM) and for command ACK
 * polling (write CMD then poll BOOT/STBY/STREAMING for the value 0).
 */
static int vd56g3_poll_reg(const struct i2c_dt_spec *i2c, uint32_t reg,
			   uint32_t target_val, uint32_t timeout_ms)
{
	int64_t deadline = k_uptime_get() + (int64_t)timeout_ms;
	uint32_t val;
	int ret;

	for (;;) {
		ret = video_read_cci_reg(i2c, reg, &val);
		if (ret < 0) {
			return ret;
		}
		if (val == target_val) {
			return 0;
		}
		if (k_uptime_get() >= deadline) {
			LOG_ERR("Timeout polling reg 0x%04x for 0x%02x (got 0x%02x)",
				(unsigned int)(reg & 0xFFFFu),
				target_val, val);
			return -ETIMEDOUT;
		}
		k_sleep(K_MSEC(2));
	}
}

static inline int vd56g3_wait_fsm(const struct i2c_dt_spec *i2c,
				  uint8_t state)
{
	return vd56g3_poll_reg(i2c, VD56G3_REG_SYSTEM_FSM, state,
			       VD56G3_FSM_POLL_TIMEOUT_MS);
}

/* Sensor identification */

static int vd56g3_detect(const struct device *dev)
{
	const struct vd56g3_config *cfg = dev->config;
	struct vd56g3_data *drv_data = dev->data;
	uint32_t model_id = 0;
	uint32_t revision = 0;
	uint32_t optical_rev = 0;
	int ret;

	ret = video_read_cci_reg(&cfg->i2c, VD56G3_REG_MODEL_ID, &model_id);
	if (ret < 0) {
		LOG_ERR("Cannot read MODEL_ID");
		return ret;
	}

	if (model_id != VD56G3_MODEL_ID_VAL) {
		LOG_ERR("Unsupported sensor MODEL_ID 0x%04x (expected 0x%04x)",
			model_id, VD56G3_MODEL_ID_VAL);
		return -ENODEV;
	}

	ret = video_read_cci_reg(&cfg->i2c, VD56G3_REG_REVISION, &revision);
	if (ret < 0) {
		return ret;
	}

	switch ((revision >> 8) & 0xFFu) {
	case VD56G3_REVISION_CUT2:
		LOG_ERR("VD56G3 cut2 (revision 0x%04x) is not supported: "
			"firmware patch upload is not implemented", revision);
		return -ENOTSUP;
	case VD56G3_REVISION_CUT3:
		drv_data->is_fastboot = true;
		break;
	default:
		LOG_ERR("Unsupported VD56G3 cut revision 0x%04x", revision);
		return -ENODEV;
	}

	ret = video_read_cci_reg(&cfg->i2c, VD56G3_REG_OPTICAL_REVISION,
				 &optical_rev);
	if (ret < 0) {
		return ret;
	}

	LOG_INF("VD56G3 detected: model 0x%04x, cut %s, optical %s",
		model_id,
		drv_data->is_fastboot ? "3 (fastboot)" : "2",
		optical_rev == VD56G3_OPTICAL_REVISION_MONO   ? "MONO"   :
		optical_rev == VD56G3_OPTICAL_REVISION_BAYER  ? "BAYER"  :
		optical_rev == VD56G3_OPTICAL_REVISION_RGBNIR ? "RGBNIR" :
		"?");
	return 0;
}

/* Leader / follower: on-sensor GPIO pins (st,out-sync / st,in-sync). */

static int vd56g3_setup_gpios(const struct device *dev)
{
	const struct vd56g3_config *cfg = dev->config;
	int ret;

	for (uint8_t io = 0; io < VD56G3_NB_GPIOS; io++) {
		uint8_t mode;

		if (cfg->out_sync_mask & BIT(io)) {
			mode = VD56G3_GPIOX_FSYNC_OUT;
		} else if (io == 0U && cfg->in_sync) {
			mode = VD56G3_GPIOX_VT_SLAVE_MODE;
		} else {
			mode = VD56G3_GPIOX_GPIO_IN;
		}

		ret = video_write_cci_reg(&cfg->i2c,
					  VD56G3_REG_GPIO_0_CTRL + io, mode);
		if (ret < 0) {
			LOG_ERR("GPIO%u_CTRL write failed: %d", io, ret);
			return ret;
		}
	}

	if (cfg->out_sync_mask) {
		LOG_INF("VD56G3 leader: FSYNC_OUT on GPIO mask 0x%02x",
			cfg->out_sync_mask);
	}
	if (cfg->in_sync) {
		LOG_INF("VD56G3 follower: GPIO0 in VT_SLAVE_MODE");
	}

	return 0;
}

/* Issue the BOOT command and wait for SW_STANDBY. */
static int vd56g3_boot(const struct device *dev)
{
	const struct vd56g3_config *cfg = dev->config;
	int ret;

	ret = video_write_cci_reg(&cfg->i2c, VD56G3_REG_BOOT,
				  VD56G3_CMD_BOOT);
	if (ret < 0) {
		return ret;
	}

	ret = vd56g3_poll_reg(&cfg->i2c, VD56G3_REG_BOOT, VD56G3_CMD_ACK,
			      VD56G3_CMD_POLL_TIMEOUT_MS);
	if (ret < 0) {
		return ret;
	}

	return vd56g3_wait_fsm(&cfg->i2c, VD56G3_SYSTEM_FSM_SW_STBY);
}

/* PLL / clock tree */

static int vd56g3_prepare_clock_tree(const struct device *dev)
{
	const struct vd56g3_config *cfg = dev->config;
	struct vd56g3_data *drv_data = dev->data;
	const uint8_t predivs[] = {1, 2, 4};
	uint32_t pll_out;

	if (cfg->input_clk < VD56G3_CLKIN_MIN_HZ ||
	    cfg->input_clk > VD56G3_CLKIN_MAX_HZ) {
		LOG_ERR("CLKIN %u Hz outside [%u..%u]",
			cfg->input_clk, VD56G3_CLKIN_MIN_HZ,
			VD56G3_CLKIN_MAX_HZ);
		return -EINVAL;
	}

	for (size_t i = 0; i < ARRAY_SIZE(predivs); i++) {
		drv_data->pll_prediv = predivs[i];
		if (cfg->input_clk / drv_data->pll_prediv < 12U * 1000000U) {
			break;
		}
	}

	drv_data->pll_mult = (uint8_t)((VD56G3_TARGET_PLL * drv_data->pll_prediv +
					cfg->input_clk / 2U) /
				       cfg->input_clk);

	pll_out = cfg->input_clk * drv_data->pll_mult / drv_data->pll_prediv;
	drv_data->pixel_clock_hz = (uint64_t)pll_out / VD56G3_VT_CLOCK_DIV;

	return 0;
}

/* Format / capabilities */

static int vd56g3_get_caps(const struct device *dev, struct video_caps *caps)
{
	ARG_UNUSED(dev);
	caps->format_caps = vd56g3_fmts;
	caps->min_vbuf_count = 1;
	return 0;
}

static int vd56g3_get_fmt(const struct device *dev, struct video_format *fmt)
{
	struct vd56g3_data *drv_data = dev->data;

	*fmt = drv_data->fmt;
	return 0;
}

static int vd56g3_apply_geometry(const struct device *dev, size_t fmt_idx)
{
	const struct vd56g3_config *cfg = dev->config;
	uint32_t width, height;
	uint32_t roi_x, roi_y;
	uint8_t  format_ctrl;
	uint8_t  oif_dt;
	uint8_t  readout;
	int ret;

	switch (fmt_idx) {
	case VD56G3_RES_FULL_RAW10:
		width = VD56G3_NATIVE_WIDTH;
		height = VD56G3_NATIVE_HEIGHT;
		format_ctrl = VD56G3_FORMAT_RAW10;
		oif_dt = VD56G3_OIF_IMG_CTRL_DT_RAW10;
		readout = VD56G3_READOUT_NORMAL;
		break;
	case VD56G3_RES_FULL_RAW8:
		width = VD56G3_NATIVE_WIDTH;
		height = VD56G3_NATIVE_HEIGHT;
		format_ctrl = VD56G3_FORMAT_RAW8;
		oif_dt = VD56G3_OIF_IMG_CTRL_DT_RAW8;
		readout = VD56G3_READOUT_NORMAL;
		break;
	case VD56G3_RES_DEFAULT_RAW10:
		width = VD56G3_DEFAULT_WIDTH;
		height = VD56G3_DEFAULT_HEIGHT;
		format_ctrl = VD56G3_FORMAT_RAW10;
		oif_dt = VD56G3_OIF_IMG_CTRL_DT_RAW10;
		readout = VD56G3_READOUT_NORMAL;
		break;
	case VD56G3_RES_DEFAULT_RAW8:
		width = VD56G3_DEFAULT_WIDTH;
		height = VD56G3_DEFAULT_HEIGHT;
		format_ctrl = VD56G3_FORMAT_RAW8;
		oif_dt = VD56G3_OIF_IMG_CTRL_DT_RAW8;
		readout = VD56G3_READOUT_NORMAL;
		break;
	case VD56G3_RES_BIN2_RAW10:
		width = VD56G3_BIN_X2_WIDTH;
		height = VD56G3_BIN_X2_HEIGHT;
		format_ctrl = VD56G3_FORMAT_RAW10;
		oif_dt = VD56G3_OIF_IMG_CTRL_DT_RAW10;
		readout = VD56G3_READOUT_DIGITAL_BINNING_X2;
		break;
	case VD56G3_RES_CROP_720x1280:
		width = VD56G3_CROP_720x1280_W;
		height = VD56G3_CROP_720x1280_H;
		format_ctrl = VD56G3_FORMAT_RAW10;
		oif_dt = VD56G3_OIF_IMG_CTRL_DT_RAW10;
		readout = VD56G3_READOUT_NORMAL;
		break;
	case VD56G3_RES_CROP_VGA:
		width = VD56G3_CROP_VGA_W;
		height = VD56G3_CROP_VGA_H;
		format_ctrl = VD56G3_FORMAT_RAW10;
		oif_dt = VD56G3_OIF_IMG_CTRL_DT_RAW10;
		readout = VD56G3_READOUT_NORMAL;
		break;
	default:
		return -EINVAL;
	}

	roi_x = (VD56G3_NATIVE_WIDTH  - width)  / 2U;
	roi_y = (VD56G3_NATIVE_HEIGHT - height) / 2U;

	ret = video_write_cci_reg(&cfg->i2c, VD56G3_REG_FORMAT_CTRL,
				  format_ctrl);
	if (ret < 0) {
		return ret;
	}

	ret = video_write_cci_reg(&cfg->i2c, VD56G3_REG_OIF_IMG_CTRL, oif_dt);
	if (ret < 0) {
		return ret;
	}

	ret = video_write_cci_reg(&cfg->i2c, VD56G3_REG_READOUT_CTRL, readout);
	if (ret < 0) {
		return ret;
	}

	/* Vertical readout window (Y_START / Y_END are inclusive). */
	ret = video_write_cci_reg(&cfg->i2c, VD56G3_REG_Y_START, roi_y);
	if (ret < 0) {
		return ret;
	}
	ret = video_write_cci_reg(&cfg->i2c, VD56G3_REG_Y_END,
				  roi_y + height - 1U);
	if (ret < 0) {
		return ret;
	}

	/* Output ROI window (relative to readout). */
	ret = video_write_cci_reg(&cfg->i2c, VD56G3_REG_OUT_ROI_X_START, roi_x);
	if (ret < 0) {
		return ret;
	}
	ret = video_write_cci_reg(&cfg->i2c, VD56G3_REG_OUT_ROI_X_END,
				  roi_x + width - 1U);
	if (ret < 0) {
		return ret;
	}
	ret = video_write_cci_reg(&cfg->i2c, VD56G3_REG_OUT_ROI_Y_START, 0U);
	if (ret < 0) {
		return ret;
	}
	ret = video_write_cci_reg(&cfg->i2c, VD56G3_REG_OUT_ROI_Y_END,
				  height - 1U);
	if (ret < 0) {
		return ret;
	}

	/* AE measurement window matches the output crop. */
	ret = video_write_cci_reg(&cfg->i2c, VD56G3_REG_AE_ROI_START_H, roi_x);
	if (ret < 0) {
		return ret;
	}
	ret = video_write_cci_reg(&cfg->i2c, VD56G3_REG_AE_ROI_END_H,
				  roi_x + width - 1U);
	if (ret < 0) {
		return ret;
	}
	ret = video_write_cci_reg(&cfg->i2c, VD56G3_REG_AE_ROI_START_V, 0U);
	if (ret < 0) {
		return ret;
	}
	ret = video_write_cci_reg(&cfg->i2c, VD56G3_REG_AE_ROI_END_V,
				  height - 1U);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int vd56g3_set_frmival(const struct device *dev,
			      struct video_frmival *frmival);

static int vd56g3_set_fmt(const struct device *dev, struct video_format *fmt)
{
	struct vd56g3_data *drv_data = dev->data;
	struct video_format sensor_fmt;
	size_t fmt_idx;
	int ret;

	if (drv_data->enabled) {
		LOG_ERR("Cannot set format while streaming");
		return -EBUSY;
	}

	/*
	 * "Trick" mode for DCMIPP dump-pipe: accept RAW8 Bayer requests (SRGGB8)
	 * as an alias of RAW8 monochrome (GREY) at the sensor level.
	 *
	 * Rationale: VD56G3 is monochrome but its RAW8 payload is byte-compatible
	 * with RAW8 Bayer; downstream blocks (DCMIPP/UVC) may only list SRGGB8.
	 * We keep reporting the originally requested pixelformat to downstream
	 * while programming the sensor geometry as GREY.
	 */
	sensor_fmt = *fmt;
	if (sensor_fmt.pixelformat == VIDEO_PIX_FMT_SRGGB8) {
		sensor_fmt.pixelformat = VIDEO_PIX_FMT_GREY;
	}

	ret = video_format_caps_index(vd56g3_fmts, &sensor_fmt, &fmt_idx);
	if (ret < 0) {
		LOG_ERR("Unsupported pixel format / resolution: %s %ux%u",
			VIDEO_FOURCC_TO_STR(fmt->pixelformat), fmt->width,
			fmt->height);
		return ret;
	}

	ret = vd56g3_apply_geometry(dev, fmt_idx);
	if (ret < 0) {
		LOG_ERR("Failed to apply geometry for format index %zu",
			fmt_idx);
		return ret;
	}

	ret = video_estimate_fmt_size(fmt);
	if (ret < 0) {
		return ret;
	}

	drv_data->fmt = *fmt;

	struct video_frmival frmival = {
		.numerator = 1,
		.denominator = drv_data->frame_rate,
	};
	return vd56g3_set_frmival(dev, &frmival);
}

/* Frame interval */

static int vd56g3_set_frmival(const struct device *dev,
			      struct video_frmival *frmival)
{
	const struct vd56g3_config *cfg = dev->config;
	struct vd56g3_data *drv_data = dev->data;
	struct video_frmival_enum match = {
		.format = &drv_data->fmt,
		.type = VIDEO_FRMIVAL_TYPE_DISCRETE,
		.discrete = *frmival,
	};
	uint32_t frame_length;
	uint32_t fps;
	int ret;

	ret = video_closest_frmival(dev, &match);
	if (ret < 0) {
		return ret;
	}

	fps = vd56g3_framerates[match.index];

	/*
	 * The VD56G3 internally derives the line length from the active
	 * geometry. We compute the frame length from the cached pixel clock
	 * and the line length minimum. This is conservative but matches
	 * what the Linux driver does in vd56g3_update_controls().
	 */
	if (drv_data->pixel_clock_hz != 0U && fps != 0U) {
		uint64_t fl = drv_data->pixel_clock_hz /
			      (VD56G3_LINE_LENGTH_MIN * (uint64_t)fps);

		frame_length = (uint32_t)CLAMP(fl, drv_data->fmt.height +
						   VD56G3_VBLANK_MIN,
					       VD56G3_FRAME_LENGTH_MAX);

		ret = video_write_cci_reg(&cfg->i2c, VD56G3_REG_FRAME_LENGTH,
					  frame_length);
		if (ret < 0) {
			LOG_WRN("FRAME_LENGTH write failed: %d", ret);
		}
	}

	frmival->numerator = 1;
	frmival->denominator = fps;
	drv_data->frame_rate = fps;
	return 0;
}

static int vd56g3_get_frmival(const struct device *dev,
			      struct video_frmival *frmival)
{
	struct vd56g3_data *drv_data = dev->data;

	frmival->numerator = 1;
	frmival->denominator = drv_data->frame_rate;
	return 0;
}

static int vd56g3_enum_frmival(const struct device *dev,
			       struct video_frmival_enum *fie)
{
	size_t fmt_idx;
	int ret;

	ret = video_format_caps_index(vd56g3_fmts, fie->format, &fmt_idx);
	if (ret < 0) {
		return ret;
	}

	if (fie->index >= VD56G3_FPS_COUNT) {
		return -EINVAL;
	}

	if (vd56g3_framerates[fie->index] > vd56g3_max_fps[fmt_idx]) {
		return -EINVAL;
	}

	fie->type = VIDEO_FRMIVAL_TYPE_DISCRETE;
	fie->discrete.numerator = 1;
	fie->discrete.denominator = vd56g3_framerates[fie->index];
	return 0;
}

/* Controls */

static int vd56g3_read_applied_expo(const struct device *dev)
{
	const struct vd56g3_config *cfg = dev->config;
	struct vd56g3_data *drv_data = dev->data;
	uint32_t exposure;
	uint32_t again;
	uint32_t dgain;
	int ret;

	ret = video_read_cci_reg(&cfg->i2c, VD56G3_REG_APPLIED_COARSE_EXPOSURE,
				 &exposure);
	if (ret < 0) {
		return ret;
	}
	ret = video_read_cci_reg(&cfg->i2c, VD56G3_REG_APPLIED_ANALOG_GAIN, &again);
	if (ret < 0) {
		return ret;
	}
	ret = video_read_cci_reg(&cfg->i2c, VD56G3_REG_APPLIED_DIGITAL_GAIN, &dgain);
	if (ret < 0) {
		return ret;
	}

	drv_data->ctrls.exposure.val = (int32_t)exposure;
	drv_data->ctrls.gain.val = (int32_t)again;
	drv_data->ctrls.digital_gain.val = (int32_t)dgain;
	return 0;
}

static int vd56g3_update_expo_cluster(const struct device *dev, bool is_auto)
{
	const struct vd56g3_config *cfg = dev->config;
	struct vd56g3_data *drv_data = dev->data;
	uint8_t expo_state = is_auto ? VD56G3_EXP_MODE_AUTO : VD56G3_EXP_MODE_MANUAL;
	int ret;

	ret = video_write_cci_reg(&cfg->i2c, VD56G3_REG_EXP_MODE, expo_state);
	if (ret < 0) {
		return ret;
	}

	if (is_auto) {
		ret = video_write_cci_reg(&cfg->i2c,
					  VD56G3_REG_AE_COLDSTART_COARSE_EXPOSURE,
					  (uint32_t)drv_data->ctrls.exposure.val);
		if (ret < 0) {
			return ret;
		}
		ret = video_write_cci_reg(&cfg->i2c, VD56G3_REG_AE_COLDSTART_ANALOG_GAIN,
					  (uint32_t)drv_data->ctrls.gain.val);
		if (ret < 0) {
			return ret;
		}
		return video_write_cci_reg(&cfg->i2c, VD56G3_REG_AE_COLDSTART_DIGITAL_GAIN,
					   (uint32_t)drv_data->ctrls.digital_gain.val);
	}

	ret = video_write_cci_reg(&cfg->i2c, VD56G3_REG_MANUAL_COARSE_EXPOSURE,
				  (uint32_t)drv_data->ctrls.exposure.val);
	if (ret < 0) {
		return ret;
	}
	ret = video_write_cci_reg(&cfg->i2c, VD56G3_REG_MANUAL_ANALOG_GAIN,
				  (uint32_t)drv_data->ctrls.gain.val);
	if (ret < 0) {
		return ret;
	}
	ret = video_write_cci_reg(&cfg->i2c, VD56G3_REG_MANUAL_DIGITAL_GAIN_CH0,
				  (uint32_t)drv_data->ctrls.digital_gain.val);
	if (ret < 0) {
		return ret;
	}
	ret = video_write_cci_reg(&cfg->i2c, VD56G3_REG_MANUAL_DIGITAL_GAIN_CH1,
				  (uint32_t)drv_data->ctrls.digital_gain.val);
	if (ret < 0) {
		return ret;
	}
	ret = video_write_cci_reg(&cfg->i2c, VD56G3_REG_MANUAL_DIGITAL_GAIN_CH2,
				  (uint32_t)drv_data->ctrls.digital_gain.val);
	if (ret < 0) {
		return ret;
	}
	return video_write_cci_reg(&cfg->i2c, VD56G3_REG_MANUAL_DIGITAL_GAIN_CH3,
				   (uint32_t)drv_data->ctrls.digital_gain.val);
}

static int vd56g3_set_ctrl_ae_bias(const struct device *dev)
{
	const struct vd56g3_config *cfg = dev->config;
	struct vd56g3_data *drv_data = dev->data;
	int32_t idx = drv_data->ctrls.ae_bias.val;
	int32_t bias_milli;
	uint32_t ae_comp;

	if (idx < 0 || idx >= (int32_t)ARRAY_SIZE(vd56g3_ev_bias_qmenu)) {
		return -EINVAL;
	}

	bias_milli = (int32_t)vd56g3_ev_bias_qmenu[idx];
	if (bias_milli >= 0) {
		ae_comp = (uint32_t)((bias_milli * 256 + 500) / 1000);
	} else {
		ae_comp = (uint32_t)((bias_milli * 256 - 500) / 1000);
	}

	return video_write_cci_reg(&cfg->i2c, VD56G3_REG_AE_COMPENSATION, ae_comp);
}

static int vd56g3_set_ctrl_orientation(const struct device *dev)
{
	const struct vd56g3_config *cfg = dev->config;
	struct vd56g3_data *drv_data = dev->data;
	uint8_t orientation = VD56G3_ORIENTATION_NORMAL;

	if (drv_data->ctrls.hflip.val) {
		orientation |= VD56G3_ORIENTATION_HFLIP;
	}
	if (drv_data->ctrls.vflip.val) {
		orientation |= VD56G3_ORIENTATION_VFLIP;
	}

	return video_write_cci_reg(&cfg->i2c, VD56G3_REG_ORIENTATION,
				   orientation);
}

static int vd56g3_set_ctrl_test_pattern(const struct device *dev)
{
	const struct vd56g3_config *cfg = dev->config;
	struct vd56g3_data *drv_data = dev->data;
	int32_t idx = drv_data->ctrls.test_pattern.val;
	uint16_t patgen;
	uint8_t  duster;
	uint8_t  darkcal;
	int ret;

	if (idx < 0 || idx > VD56G3_TP_PN28) {
		return -EINVAL;
	}

	patgen = (uint16_t)idx << VD56G3_PATGEN_TYPE_SHIFT;

	if (idx == VD56G3_TP_DISABLED) {
		duster = VD56G3_DUSTER_ENABLE_DEF_MODULES;
		darkcal = VD56G3_DARKCAL_ENABLE;
	} else {
		patgen |= VD56G3_PATGEN_ENABLE;
		duster = VD56G3_DUSTER_DISABLE;
		darkcal = VD56G3_DARKCAL_DISABLE_DARKAVG;
	}

	ret = video_write_cci_reg(&cfg->i2c, VD56G3_REG_DUSTER_CTRL, duster);
	if (ret < 0) {
		return ret;
	}
	ret = video_write_cci_reg(&cfg->i2c, VD56G3_REG_DARKCAL_CTRL, darkcal);
	if (ret < 0) {
		return ret;
	}
	return video_write_cci_reg(&cfg->i2c, VD56G3_REG_PATGEN_CTRL, patgen);
}

static int vd56g3_get_volatile_ctrl(const struct device *dev, uint32_t id)
{
	struct vd56g3_data *drv_data = dev->data;

	if (!vd56g3_is_ae_auto(drv_data)) {
		return 0;
	}

	switch (id) {
	case VIDEO_CID_EXPOSURE_AUTO:
	case VIDEO_CID_ANALOGUE_GAIN:
	case VIDEO_CID_EXPOSURE:
	case VIDEO_CID_DIGITAL_GAIN:
		return vd56g3_read_applied_expo(dev);
	default:
		return -ENOTSUP;
	}
}

static int vd56g3_set_ctrl(const struct device *dev, uint32_t cid)
{
	struct vd56g3_data *drv_data = dev->data;

	switch (cid) {
	case VIDEO_CID_EXPOSURE_AUTO:
		return vd56g3_update_expo_cluster(dev, vd56g3_is_ae_auto(drv_data));
	case VIDEO_CID_ANALOGUE_GAIN:
	case VIDEO_CID_EXPOSURE:
	case VIDEO_CID_DIGITAL_GAIN:
		return vd56g3_update_expo_cluster(dev, false);
	case VIDEO_CID_AUTO_EXPOSURE_BIAS:
		return vd56g3_set_ctrl_ae_bias(dev);
	case VIDEO_CID_HFLIP:
	case VIDEO_CID_VFLIP:
		return vd56g3_set_ctrl_orientation(dev);
	case VIDEO_CID_TEST_PATTERN:
		return vd56g3_set_ctrl_test_pattern(dev);
	default:
		return -ENOTSUP;
	}
}

/* Stream control */

static uint16_t vd56g3_compute_oif_ctrl(const struct vd56g3_config *cfg)
{
	/*
	 * Default 1:1 lane mapping (no remap, no polarity inversion). The
	 * Linux driver derives the full bit pattern from the DT endpoint;
	 * for the targets we support (Raspberry Pi CSI/B-CAMS-IMX), the
	 * physical layout matches the logical one so this constant value
	 * is enough.
	 */
	uint16_t lane_mapping = (1U << 7);

	uint16_t oif = (uint16_t)cfg->num_lanes | lane_mapping;

	/* Polarity inversion bits (see VD56G3_REG_OIF_CTRL layout comment). */
	if (cfg->csi_lane0_polarity_invert) {
		oif |= BIT(3);
	}
	if (cfg->csi_lane1_polarity_invert) {
		oif |= BIT(6);
	}
	if (cfg->csi_clock_polarity_invert) {
		oif |= BIT(9);
	}

	return oif;
}

static int vd56g3_stream_enable(const struct device *dev)
{
	const struct vd56g3_config *cfg = dev->config;
	struct vd56g3_data *drv_data = dev->data;
	uint32_t csi_mbps;
	int ret;

	/* Re-program the clock tree (idempotent at the sensor side). */
	ret = video_write_cci_reg(&cfg->i2c, VD56G3_REG_EXT_CLOCK, cfg->input_clk);
	if (ret < 0) {
		return ret;
	}

	ret = video_write_cci_reg(&cfg->i2c, VD56G3_REG_CLK_PLL_PREDIV,
				  drv_data->pll_prediv);
	if (ret < 0) {
		return ret;
	}

	ret = video_write_cci_reg(&cfg->i2c, VD56G3_REG_CLK_SYS_PLL_MULT,
				  drv_data->pll_mult);
	if (ret < 0) {
		return ret;
	}

	ret = video_write_cci_reg(&cfg->i2c, VD56G3_REG_OIF_CTRL,
				  vd56g3_compute_oif_ctrl(cfg));
	if (ret < 0) {
		return ret;
	}

	csi_mbps = (uint32_t)((uint64_t)drv_data->link_freq_hz * 2U / 1000000U);
	ret = video_write_cci_reg(&cfg->i2c, VD56G3_REG_OIF_CSI_BITRATE, csi_mbps);
	if (ret < 0) {
		return ret;
	}

	ret = video_write_cci_reg(&cfg->i2c, VD56G3_REG_ISL_ENABLE, 0);
	if (ret < 0) {
		return ret;
	}

	ret = video_write_cci_reg(&cfg->i2c, VD56G3_REG_STBY, VD56G3_CMD_START_STREAM);
	if (ret < 0) {
		return ret;
	}

	ret = vd56g3_poll_reg(&cfg->i2c, VD56G3_REG_STBY, VD56G3_CMD_ACK,
			      VD56G3_CMD_POLL_TIMEOUT_MS);
	if (ret < 0) {
		return ret;
	}

	ret = vd56g3_wait_fsm(&cfg->i2c, VD56G3_SYSTEM_FSM_STREAMING);
	if (ret < 0) {
		return ret;
	}

	return vd56g3_update_expo_cluster(dev, vd56g3_is_ae_auto(drv_data));
}

static int vd56g3_stream_disable(const struct device *dev)
{
	const struct vd56g3_config *cfg = dev->config;
	int ret;

	ret = video_write_cci_reg(&cfg->i2c, VD56G3_REG_STREAMING,
				  VD56G3_CMD_STOP_STREAM);
	if (ret < 0) {
		return ret;
	}

	ret = vd56g3_poll_reg(&cfg->i2c, VD56G3_REG_STREAMING, VD56G3_CMD_ACK,
			      VD56G3_CMD_POLL_TIMEOUT_MS);
	if (ret < 0) {
		return ret;
	}

	return vd56g3_wait_fsm(&cfg->i2c, VD56G3_SYSTEM_FSM_SW_STBY);
}

static int vd56g3_set_stream(const struct device *dev, bool enable,
			     enum video_buf_type type)
{
	struct vd56g3_data *drv_data = dev->data;
	int ret;

	ARG_UNUSED(type);

	if (enable) {
		ret = vd56g3_stream_enable(dev);
	} else {
		ret = vd56g3_stream_disable(dev);
	}

	if (ret < 0) {
		return ret;
	}

	drv_data->enabled = enable;
	return 0;
}

/* Driver API */

static DEVICE_API(video, vd56g3_driver_api) = {
	.set_format   = vd56g3_set_fmt,
	.get_format   = vd56g3_get_fmt,
	.get_caps     = vd56g3_get_caps,
	.set_stream   = vd56g3_set_stream,
	.set_ctrl     = vd56g3_set_ctrl,
	.get_volatile_ctrl = vd56g3_get_volatile_ctrl,
	.set_frmival  = vd56g3_set_frmival,
	.get_frmival  = vd56g3_get_frmival,
	.enum_frmival = vd56g3_enum_frmival,
};

/* Init */

static const char *const vd56g3_test_pattern_menu[] = {
	"Disabled",
	"Solid",
	"Colorbar",
	"Gradbar",
	"Hgrey",
	"Vgrey",
	"Dgrey",
	"PN28",
	NULL,
};

static int vd56g3_init_controls(const struct device *dev)
{
	const struct vd56g3_config *cfg = dev->config;
	struct vd56g3_data *drv_data = dev->data;
	struct vd56g3_ctrls *ctrls = &drv_data->ctrls;
	int64_t pixel_rate = (int64_t)drv_data->pixel_clock_hz;
	int ret;

	ret = video_init_ctrl(&ctrls->exposure_auto, dev, VIDEO_CID_EXPOSURE_AUTO,
			      (struct video_ctrl_range){
				      .min = 0, .max = 1, .step = 1, .def = 1,
			      });
	if (ret < 0) {
		return ret;
	}

	/*
	 * Analog gain is the raw register code in [0..28]. The linear gain
	 * is 32 / (32 - code), so the V4L2 user-space value is intentionally
	 * the code itself (consistent with the Linux driver). 0 means x1.
	 */
	ret = video_init_ctrl(
		&ctrls->gain, dev, VIDEO_CID_ANALOGUE_GAIN,
		(struct video_ctrl_range){
			.min = VD56G3_AGAIN_CODE_MIN,
			.max = VD56G3_AGAIN_CODE_MAX,
			.step = 1,
			.def = 0,
		});
	if (ret < 0) {
		return ret;
	}

	ret = video_init_ctrl(
		&ctrls->exposure, dev, VIDEO_CID_EXPOSURE,
		(struct video_ctrl_range){
			.min = VD56G3_EXPOSURE_MIN,
			.max = VD56G3_FRAME_LENGTH_MAX -
			       VD56G3_EXPOSURE_MARGIN,
			.step = 1,
			.def = VD56G3_EXPOSURE_DEFAULT,
		});
	if (ret < 0) {
		return ret;
	}

	ret = video_init_ctrl(
		&ctrls->digital_gain, dev, VIDEO_CID_DIGITAL_GAIN,
		(struct video_ctrl_range){
			.min = VD56G3_DGAIN_MIN,
			.max = VD56G3_DGAIN_MAX,
			.step = 1,
			.def = VD56G3_DGAIN_DEFAULT,
		});
	if (ret < 0) {
		return ret;
	}

	ret = video_auto_cluster_ctrl(&ctrls->exposure_auto, 4, true);
	if (ret < 0) {
		return ret;
	}

	ret = video_init_int_menu_ctrl(
		&ctrls->ae_bias, dev, VIDEO_CID_AUTO_EXPOSURE_BIAS,
		ARRAY_SIZE(vd56g3_ev_bias_qmenu) / 2, vd56g3_ev_bias_qmenu,
		ARRAY_SIZE(vd56g3_ev_bias_qmenu));
	if (ret < 0) {
		return ret;
	}

	ret = video_init_ctrl(
		&ctrls->hflip, dev, VIDEO_CID_HFLIP,
		(struct video_ctrl_range){.min = 0, .max = 1, .step = 1, .def = 0});
	if (ret < 0) {
		return ret;
	}

	ret = video_init_ctrl(
		&ctrls->vflip, dev, VIDEO_CID_VFLIP,
		(struct video_ctrl_range){.min = 0, .max = 1, .step = 1, .def = 0});
	if (ret < 0) {
		return ret;
	}

	ARG_UNUSED(cfg);
	/* Expose VIDEO_CID_LINK_FREQ as DDR bitrate in Hz for CSI-2 receivers. */
	drv_data->link_freq_menu[0] = (int64_t)drv_data->link_freq_hz * 2;
	ret = video_init_int_menu_ctrl(&ctrls->link_freq, dev, VIDEO_CID_LINK_FREQ,
				       0, drv_data->link_freq_menu,
				       ARRAY_SIZE(drv_data->link_freq_menu));
	if (ret < 0) {
		return ret;
	}
	ctrls->link_freq.flags |= VIDEO_CTRL_FLAG_READ_ONLY;

	ret = video_init_ctrl(
		&ctrls->pixel_rate, dev, VIDEO_CID_PIXEL_RATE,
		(struct video_ctrl_range){
			.min64 = pixel_rate,
			.max64 = pixel_rate,
			.step64 = 1,
			.def64 = pixel_rate,
		});
	if (ret < 0) {
		return ret;
	}
	ctrls->pixel_rate.flags |= VIDEO_CTRL_FLAG_READ_ONLY;

	ret = video_init_menu_ctrl(&ctrls->test_pattern, dev,
				   VIDEO_CID_TEST_PATTERN, 0,
				   vd56g3_test_pattern_menu);
	if (ret < 0) {
		return ret;
	}

	ret = vd56g3_set_ctrl_ae_bias(dev);
	if (ret < 0) {
		return ret;
	}

	/* Enable auto-exposure by default (coldstart from control defaults). */
	return vd56g3_update_expo_cluster(dev, true);
}

static int vd56g3_init(const struct device *dev)
{
	const struct vd56g3_config *cfg = dev->config;
	struct vd56g3_data *drv_data = dev->data;
	int ret;

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("I2C bus %s is not ready", cfg->i2c.bus->name);
		return -ENODEV;
	}

	ret = vd56g3_prepare_clock_tree(dev);
	if (ret < 0) {
		return ret;
	}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
	if (cfg->reset_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->reset_gpio)) {
			LOG_ERR("reset-gpios controller %s not ready",
				cfg->reset_gpio.port->name);
			return -ENODEV;
		}

		/* Hold XSHUTDOWN asserted while supplies / CLKIN ramp up. */
		ret = gpio_pin_configure_dt(&cfg->reset_gpio,
					    GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			return ret;
		}

		k_sleep(K_MSEC(1));
		gpio_pin_set_dt(&cfg->reset_gpio, 0);

		/* Linux driver waits ~3.5 ms after XSHUTDOWN release; use 10 ms margin. */
		k_sleep(K_MSEC(10));
	}
#endif

	/* Wait for the internal MCU to reach READY_TO_BOOT. */
	ret = vd56g3_poll_reg(&cfg->i2c, VD56G3_REG_SYSTEM_FSM,
			      VD56G3_SYSTEM_FSM_READY_TO_BOOT,
			      VD56G3_BOOT_POLL_TIMEOUT_MS);
	if (ret < 0) {
		LOG_ERR("VD56G3 did not reach READY_TO_BOOT");
		return ret;
	}

	ret = vd56g3_detect(dev);
	if (ret < 0) {
		return ret;
	}

	/* Boot transition READY_TO_BOOT -> SW_STANDBY. */
	ret = vd56g3_boot(dev);
	if (ret < 0) {
		LOG_ERR("VD56G3 boot failed");
		return ret;
	}

	/* Tell the sensor at which frequency CLKIN is running. */
	ret = video_write_cci_reg(&cfg->i2c, VD56G3_REG_EXT_CLOCK,
				  cfg->input_clk);
	if (ret < 0) {
		return ret;
	}

	/* External Vertical Timing sync (leader / follower). */
	ret = vd56g3_setup_gpios(dev);
	if (ret < 0) {
		return ret;
	}

	/*
	 * The VD56G3 has different recommended link-frequency defaults depending on
	 * the number of data lanes (from the Linux driver). Use those runtime
	 * defaults for both the OIF bitrate register and VIDEO_CID_LINK_FREQ.
	 */
	drv_data->link_freq_hz = (cfg->num_lanes == 2U) ? VD56G3_LINK_FREQ_DEF_2LANES_HZ
						       : VD56G3_LINK_FREQ_DEF_1LANE_HZ;

	/* Apply the boot-time format. */
	ret = vd56g3_set_fmt(dev, &drv_data->fmt);
	if (ret < 0) {
		return ret;
	}

	ret = vd56g3_init_controls(dev);
	if (ret < 0) {
		return ret;
	}

	LOG_INF("VD56G3 ready: %u lanes @ %u Hz, CLKIN %u Hz, pixclk %llu Hz",
		cfg->num_lanes, drv_data->link_freq_hz, cfg->input_clk,
		drv_data->pixel_clock_hz);
	return 0;
}

/* Devicetree instantiation */

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
#define VD56G3_RESET_GPIO(n) \
	.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(n, reset_gpios, {0}),
#else
#define VD56G3_RESET_GPIO(n)
#endif

/*
 * The CSI-2 endpoint of the sensor is the standard
 *
 *   sensor@10 {
 *       compatible = "st,vd56g3";
 *       port {
 *           endpoint {
 *               data-lanes = <1 2>;
 *           };
 *       };
 *   };
 *
 * Number of lanes is read from data-lanes (defaults to 2 if absent).
 * Link frequency is derived at runtime from the lane count (see init()).
 */
#define VD56G3_ENDPOINT(n) DT_CHILD(DT_INST_CHILD(n, port), endpoint)

#define VD56G3_NUM_LANES(n) \
	COND_CODE_1(DT_NODE_HAS_PROP(VD56G3_ENDPOINT(n), data_lanes), \
		    (DT_PROP_LEN(VD56G3_ENDPOINT(n), data_lanes)), \
		    (2))

/*
 * Build a bitmask from ``st,out-sync``: each entry is a sensor GPIO index
 * in [0..7]; bit i set means that pin is configured as FSYNC_OUT at init.
 */
#define VD56G3_OUT_SYNC_BIT(node_id, prop, idx) \
	| BIT(DT_PROP_BY_IDX(node_id, prop, idx))

#define VD56G3_OUT_SYNC_MASK(n) \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, st_out_sync),			\
		    ((0 DT_INST_FOREACH_PROP_ELEM(n, st_out_sync,		\
						 VD56G3_OUT_SYNC_BIT))),	\
		    (0))

#define VD56G3_INIT(n)								\
	static struct vd56g3_data vd56g3_data_##n = {				\
		.fmt = {							\
			.pixelformat = VIDEO_PIX_FMT_Y10P,			\
			.width = VD56G3_NATIVE_WIDTH,				\
			.height = VD56G3_NATIVE_HEIGHT,				\
		},								\
		.frame_rate = 30,						\
		.enabled = false,						\
	};									\
										\
	static const struct vd56g3_config vd56g3_cfg_##n = {			\
		.i2c = I2C_DT_SPEC_INST_GET(n),					\
		VD56G3_RESET_GPIO(n)						\
		.input_clk = DT_INST_PROP_BY_PHANDLE(n, clocks,			\
						     clock_frequency),		\
		.num_lanes = VD56G3_NUM_LANES(n),				\
		.csi_lane0_polarity_invert = DT_INST_PROP(n, st_csi_lane0_polarity_invert), \
		.csi_lane1_polarity_invert = DT_INST_PROP(n, st_csi_lane1_polarity_invert), \
		.csi_clock_polarity_invert = DT_INST_PROP(n, st_csi_clock_polarity_invert), \
		.out_sync_mask = VD56G3_OUT_SYNC_MASK(n),			\
		.in_sync = DT_INST_PROP(n, st_in_sync),				\
	};									\
										\
	DEVICE_DT_INST_DEFINE(n, &vd56g3_init, NULL, &vd56g3_data_##n,		\
			      &vd56g3_cfg_##n, POST_KERNEL,			\
			      CONFIG_VIDEO_INIT_PRIORITY,			\
			      &vd56g3_driver_api);				\
										\
	VIDEO_DEVICE_DEFINE(vd56g3_##n, DEVICE_DT_INST_GET(n), NULL);

DT_INST_FOREACH_STATUS_OKAY(VD56G3_INIT)
