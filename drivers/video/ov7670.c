/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ovti_ov7670

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/video.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(video_ov7670, CONFIG_VIDEO_LOG_LEVEL);

/* Initialization register structure */
struct ov7670_reg {
	uint8_t reg;
	uint8_t cmd;
};

struct ov7670_config {
	struct i2c_dt_spec bus;
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
	struct gpio_dt_spec reset;
#endif
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(pwdn_gpios)
	struct gpio_dt_spec pwdn;
#endif
};

struct ov7670_data {
	struct video_format fmt;
};

struct ov7670_resolution_cfg {
	uint8_t com7;
	uint8_t com3;
	uint8_t com14;
	uint8_t scaling_xsc;
	uint8_t scaling_ysc;
	uint8_t dcwctr;
	uint8_t pclk_div;
	uint8_t pclk_delay;
};

/* Resolution settings for camera, based on those present in MCUX SDK */
const struct ov7670_resolution_cfg OV7670_RESOLUTION_QCIF = {
	.com7 = 0x2c,
	.com3 = 0x00,
	.com14 = 0x11,
	.scaling_xsc = 0x3a,
	.scaling_ysc = 0x35,
	.dcwctr = 0x11,
	.pclk_div = 0xf1,
	.pclk_delay = 0x52
};

const struct ov7670_resolution_cfg OV7670_RESOLUTION_QVGA = {
	.com7 = 0x14,
	.com3 = 0x04,
	.com14 = 0x19,
	.scaling_xsc = 0x3a,
	.scaling_ysc = 0x35,
	.dcwctr = 0x11,
	.pclk_div = 0xf1,
	.pclk_delay = 0x02
};

const struct ov7670_resolution_cfg OV7670_RESOLUTION_CIF = {
	.com7 = 0x24,
	.com3 = 0x08,
	.com14 = 0x11,
	.scaling_xsc = 0x3a,
	.scaling_ysc = 0x35,
	.dcwctr = 0x11,
	.pclk_div = 0xf1,
	.pclk_delay = 0x02
};

const struct ov7670_resolution_cfg OV7670_RESOLUTION_VGA = {
	.com7 = 0x04,
	.com3 = 0x00,
	.com14 = 0x00,
	.scaling_xsc = 0x3a,
	.scaling_ysc = 0x35,
	.dcwctr = 0x11,
	.pclk_div = 0xf0,
	.pclk_delay = 0x02
};


/* OV7670 registers */
#define OV7670_PID                0x0A
#define OV7670_COM7               0x12
#define OV7670_MVFP               0x1E
#define OV7670_COM10              0x15
#define OV7670_COM12              0x3C
#define OV7670_BRIGHT             0x55
#define OV7670_CLKRC              0x11
#define OV7670_SCALING_PCLK_DIV   0x73
#define OV7670_COM14              0x3E
#define OV7670_DBLV               0x6B
#define OV7670_SCALING_XSC        0x70
#define OV7670_SCALING_YSC        0x71
#define OV7670_COM2               0x09
#define OV7670_SCALING_PCLK_DELAY 0xA2
#define OV7670_BD50MAX            0xA5
#define OV7670_BD60MAX            0xAB
#define OV7670_HAECC7             0xAA
#define OV7670_COM3               0x0C
#define OV7670_COM4               0x0D
#define OV7670_COM6               0x0F
#define OV7670_COM11              0x3B
#define OV7670_EDGE               0x3F
#define OV7670_DNSTH              0x4C
#define OV7670_DM_LNL             0x92
#define OV7670_DM_LNH             0x93
#define OV7670_COM15              0x40
#define OV7670_TSLB               0x3A
#define OV7670_COM13              0x3D
#define OV7670_MANU               0x67
#define OV7670_MANV               0x68
#define OV7670_HSTART             0x17
#define OV7670_HSTOP              0x18
#define OV7670_VSTRT              0x19
#define OV7670_VSTOP              0x1A
#define OV7670_HREF               0x32
#define OV7670_VREF               0x03
#define OV7670_SCALING_DCWCTR     0x72
#define OV7670_GAIN               0x00
#define OV7670_AECHH              0x07
#define OV7670_AECH               0x10
#define OV7670_COM8               0x13
#define OV7670_COM9               0x14
#define OV7670_AEW                0x24
#define OV7670_AEB                0x25
#define OV7670_VPT                0x26
#define OV7670_AWBC1              0x43
#define OV7670_AWBC2              0x44
#define OV7670_AWBC3              0x45
#define OV7670_AWBC4              0x46
#define OV7670_AWBC5              0x47
#define OV7670_AWBC6              0x48
#define OV7670_MTX1               0x4F
#define OV7670_MTX2               0x50
#define OV7670_MTX3               0x51
#define OV7670_MTX4               0x52
#define OV7670_MTX5               0x53
#define OV7670_MTX6               0x54
#define OV7670_LCC1               0x62
#define OV7670_LCC2               0x63
#define OV7670_LCC3               0x64
#define OV7670_LCC4               0x65
#define OV7670_LCC5               0x66
#define OV7670_LCC6               0x94
#define OV7670_LCC7               0x95
#define OV7670_SLOP               0x7A
#define OV7670_GAM1               0x7B
#define OV7670_GAM2               0x7C
#define OV7670_GAM3               0x7D
#define OV7670_GAM4               0x7E
#define OV7670_GAM5               0x7F
#define OV7670_GAM6               0x80
#define OV7670_GAM7               0x81
#define OV7670_GAM8               0x82
#define OV7670_GAM9               0x83
#define OV7670_GAM10              0x84
#define OV7670_GAM11              0x85
#define OV7670_GAM12              0x86
#define OV7670_GAM13              0x87
#define OV7670_GAM14              0x88
#define OV7670_GAM15              0x89
#define OV7670_HAECC1             0x9F
#define OV7670_HAECC2             0xA0
#define OV7670_HSYEN              0x31
#define OV7670_HAECC3             0xA6
#define OV7670_HAECC4             0xA7
#define OV7670_HAECC5             0xA8
#define OV7670_HAECC6             0xA9

/* OV7670 definitions */
#define OV7670_PROD_ID 0x76

#define OV7670_VIDEO_FORMAT_CAP(width, height, format)                                             \
	{                                                                                          \
		.pixelformat = (format), .width_min = (width), .width_max = (width),               \
		.height_min = (height), .height_max = (height), .width_step = 0, .height_step = 0  \
	}

static const struct video_format_cap fmts[] = {
	OV7670_VIDEO_FORMAT_CAP(176, 144, VIDEO_PIX_FMT_RGB565), /* QCIF  */
	OV7670_VIDEO_FORMAT_CAP(320, 240, VIDEO_PIX_FMT_RGB565), /* QVGA  */
	OV7670_VIDEO_FORMAT_CAP(352, 288, VIDEO_PIX_FMT_RGB565), /* CIF   */
	OV7670_VIDEO_FORMAT_CAP(640, 480, VIDEO_PIX_FMT_RGB565), /* VGA   */
	OV7670_VIDEO_FORMAT_CAP(176, 144, VIDEO_PIX_FMT_YUYV),   /* QCIF  */
	OV7670_VIDEO_FORMAT_CAP(320, 240, VIDEO_PIX_FMT_YUYV),   /* QVGA  */
	OV7670_VIDEO_FORMAT_CAP(352, 288, VIDEO_PIX_FMT_YUYV),   /* CIF   */
	OV7670_VIDEO_FORMAT_CAP(640, 480, VIDEO_PIX_FMT_YUYV),   /* VGA   */
	{0}};

/*
 * This initialization table is based on the MCUX SDK driver for the OV7670.
 * Note that this table assumes the camera is fed a 6MHz XCLK signal
 */
static const struct ov7670_reg ov7670_init_regtbl[] = {
	{OV7670_MVFP, 0x20}, /* MVFP: Mirror/VFlip,Normal image */

	/* configure the output timing */
	/* PCLK does not toggle during horizontal blank, one PCLK, one pixel */
	{OV7670_COM10, 0x20}, /* COM10 */
	{OV7670_COM12, 0x00}, /* COM12,No HREF when VSYNC is low */
	/* Brightness Control, with signal -128 to +128, 0x00 is middle value */
	{OV7670_BRIGHT, 0x2f},

	/* Internal clock pre-scalar,F(internal clock) = F(input clock)/(Bit[5:0]+1) */
	{OV7670_CLKRC, 0x80}, /* Clock Div, Input/(n+1), bit6 set to 1 to disable divider */

	/* DBLV,Bit[7:6]: PLL control */
	/* 0:Bypass PLL, 40: Input clock x4  , 80: Input clock x6  ,C0: Input clock x8 */
	{OV7670_DBLV, 0x00},

	/* Output Drive Capability */
	{OV7670_COM2, 0x00}, /* Common Control 2, Output Drive Capability: 1x */
	{OV7670_BD50MAX, 0x05},
	{OV7670_BD60MAX, 0x07},
	{OV7670_HAECC7, 0x94},

	{OV7670_COM4, 0x00},
	{OV7670_COM6, 0x4b},
	{OV7670_COM11, 0x9F}, /* Night mode */
	{OV7670_EDGE, 0x04},  /* Edge Enhancement Adjustment */
	{OV7670_DNSTH, 0x00}, /* De-noise Strength */

	{OV7670_DM_LNL, 0x00},
	{OV7670_DM_LNH, 0x00},

	/* reserved */
	{0x16, 0x02},
	{0x21, 0x02},
	{0x22, 0x91},
	{0x29, 0x07},
	{0x35, 0x0b},
	{0x33, 0x0b},
	{0x37, 0x1d},
	{0x38, 0x71},
	{0x39, 0x2a},
	{0x0e, 0x61},
	{0x56, 0x40},
	{0x57, 0x80},
	{0x69, 0x00},
	{0x74, 0x19},

	/* display , need retain */
	{OV7670_COM15, 0xD0}, /* Common Control 15 */
	{OV7670_TSLB, 0x0C},  /* Line Buffer Test Option */
	{OV7670_COM13, 0x80}, /* Common Control 13 */
	{OV7670_MANU, 0x11},  /* Manual U Value */
	{OV7670_MANV, 0xFF},  /* Manual V Value */
	/* config the output window data, this can be configed later */
	{OV7670_HSTART, 0x16}, /*  HSTART */
	{OV7670_HSTOP, 0x04},  /*  HSTOP */
	{OV7670_VSTRT, 0x02},  /*  VSTRT */
	{OV7670_VSTOP, 0x7a},  /*  VSTOP */
	{OV7670_HREF, 0x80},   /*  HREF */
	{OV7670_VREF, 0x0a},   /*  VREF */

	/* AGC/AEC - Automatic Gain Control/Automatic exposure Control */
	{OV7670_GAIN, 0x00},  /*  AGC */
	{OV7670_AECHH, 0x3F}, /* Exposure Value */
	{OV7670_AECH, 0xFF},
	{OV7670_COM8, 0x66},
	{OV7670_COM9, 0x21}, /*  limit the max gain */
	{OV7670_AEW, 0x75},
	{OV7670_AEB, 0x63},
	{OV7670_VPT, 0xA5},
	/* Automatic white balance control */
	{OV7670_AWBC1, 0x14},
	{OV7670_AWBC2, 0xf0},
	{OV7670_AWBC3, 0x34},
	{OV7670_AWBC4, 0x58},
	{OV7670_AWBC5, 0x28},
	{OV7670_AWBC6, 0x3a},
	/* Matrix Coefficient */
	{OV7670_MTX1, 0x80},
	{OV7670_MTX2, 0x80},
	{OV7670_MTX3, 0x00},
	{OV7670_MTX4, 0x22},
	{OV7670_MTX5, 0x5e},
	{OV7670_MTX6, 0x80},
	/* AWB Control */
	{0x59, 0x88},
	{0x5a, 0x88},
	{0x5b, 0x44},
	{0x5c, 0x67},
	{0x5d, 0x49},
	{0x5e, 0x0e},
	{0x6c, 0x0a},
	{0x6d, 0x55},
	{0x6e, 0x11},
	{0x6f, 0x9f},
	/* Lens Correction Option */
	{OV7670_LCC1, 0x00},
	{OV7670_LCC2, 0x00},
	{OV7670_LCC3, 0x04},
	{OV7670_LCC4, 0x20},
	{OV7670_LCC5, 0x05},
	{OV7670_LCC6, 0x04}, /* effective only when LCC5[2] is high */
	{OV7670_LCC7, 0x08}, /* effective only when LCC5[2] is high */
	/* Gamma Curve, needn't config */
	{OV7670_SLOP, 0x20},
	{OV7670_GAM1, 0x1c},
	{OV7670_GAM2, 0x28},
	{OV7670_GAM3, 0x3c},
	{OV7670_GAM4, 0x55},
	{OV7670_GAM5, 0x68},
	{OV7670_GAM6, 0x76},
	{OV7670_GAM7, 0x80},
	{OV7670_GAM8, 0x88},
	{OV7670_GAM9, 0x8f},
	{OV7670_GAM10, 0x96},
	{OV7670_GAM11, 0xa3},
	{OV7670_GAM12, 0xaf},
	{OV7670_GAM13, 0xc4},
	{OV7670_GAM14, 0xd7},
	{OV7670_GAM15, 0xe8},
	/* Histogram-based AEC/AGC Control */
	{OV7670_HAECC1, 0x78},
	{OV7670_HAECC2, 0x68},
	{OV7670_HSYEN, 0xff},
	{0xa1, 0x03},
	{OV7670_HAECC3, 0xdf},
	{OV7670_HAECC4, 0xdf},
	{OV7670_HAECC5, 0xf0},
	{OV7670_HAECC6, 0x90},
	/* Automatic black Level Compensation */
	{0xb0, 0x84},
	{0xb1, 0x0c},
	{0xb2, 0x0e},
	{0xb3, 0x82},
	{0xb8, 0x0a},
};

static int ov7670_get_caps(const struct device *dev, enum video_endpoint_id ep,
			   struct video_caps *caps)
{
	caps->format_caps = fmts;
	return 0;
}

static int ov7670_set_fmt(const struct device *dev, enum video_endpoint_id ep,
			  struct video_format *fmt)
{
	const struct ov7670_config *config = dev->config;
	struct ov7670_data *data = dev->data;
	const struct ov7670_resolution_cfg *resolution;
	int ret;
	uint8_t i = 0U;

	if (fmt->pixelformat != VIDEO_PIX_FMT_RGB565 && fmt->pixelformat != VIDEO_PIX_FMT_YUYV) {
		LOG_ERR("Only RGB565 and YUYV supported!");
		return -ENOTSUP;
	}

	if (!memcmp(&data->fmt, fmt, sizeof(data->fmt))) {
		/* nothing to do */
		return 0;
	}

	memcpy(&data->fmt, fmt, sizeof(data->fmt));

	/* Set output resolution */
	while (fmts[i].pixelformat) {
		if (fmts[i].width_min == fmt->width && fmts[i].height_min == fmt->height &&
		    fmts[i].pixelformat == fmt->pixelformat) {
			/* Set output format */
			switch (fmts[i].width_min) {
			case 176: /* QCIF */
				resolution = &OV7670_RESOLUTION_QCIF;
				break;
			case 320: /* QVGA */
				resolution = &OV7670_RESOLUTION_QVGA;
				break;
			case 352: /* CIF */
				resolution = &OV7670_RESOLUTION_CIF;
				break;
			default: /* VGA */
				resolution = &OV7670_RESOLUTION_VGA;
				break;
			}
			/* Program resolution bytes settings */
			ret = i2c_reg_write_byte_dt(&config->bus, OV7670_COM7,
						    resolution->com7);
			if (ret < 0) {
				return ret;
			}
			ret = i2c_reg_write_byte_dt(&config->bus, OV7670_COM3,
						    resolution->com3);
			if (ret < 0) {
				return ret;
			}
			ret = i2c_reg_write_byte_dt(&config->bus, OV7670_COM14,
						    resolution->com14);
			if (ret < 0) {
				return ret;
			}
			ret = i2c_reg_write_byte_dt(&config->bus, OV7670_SCALING_XSC,
						    resolution->scaling_xsc);
			if (ret < 0) {
				return ret;
			}
			ret = i2c_reg_write_byte_dt(&config->bus, OV7670_SCALING_YSC,
						    resolution->scaling_ysc);
			if (ret < 0) {
				return ret;
			}
			ret = i2c_reg_write_byte_dt(&config->bus, OV7670_SCALING_DCWCTR,
						    resolution->dcwctr);
			if (ret < 0) {
				return ret;
			}
			ret = i2c_reg_write_byte_dt(&config->bus, OV7670_SCALING_PCLK_DIV,
						    resolution->pclk_div);
			if (ret < 0) {
				return ret;
			}
			return i2c_reg_write_byte_dt(&config->bus, OV7670_SCALING_PCLK_DELAY,
						     resolution->pclk_delay);
		}
		i++;
	}

	LOG_ERR("Unsupported format");
	return -ENOTSUP;
}

static int ov7670_get_fmt(const struct device *dev, enum video_endpoint_id ep,
			  struct video_format *fmt)
{
	struct ov7670_data *data = dev->data;

	if (fmt == NULL) {
		return -EINVAL;
	}
	memcpy(fmt, &data->fmt, sizeof(data->fmt));
	return 0;
}

static int ov7670_init(const struct device *dev)
{
	const struct ov7670_config *config = dev->config;
	int ret, i;
	uint8_t pid;
	struct video_format fmt;
	const struct ov7670_reg *reg;

	if (!i2c_is_ready_dt(&config->bus)) {
		/* I2C device is not ready, return */
		return -ENODEV;
	}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(pwdn_gpios)
	/* Power up camera module */
	if (config->pwdn.port != NULL) {
		if (!gpio_is_ready_dt(&config->pwdn)) {
			return -ENODEV;
		}
		ret = gpio_pin_configure_dt(&config->pwdn, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_ERR("Could not clear power down pin: %d", ret);
			return ret;
		}
	}
#endif
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
	/* Reset camera module */
	if (config->reset.port != NULL) {
		if (!gpio_is_ready_dt(&config->reset)) {
			return -ENODEV;
		}
		ret = gpio_pin_configure_dt(&config->reset, GPIO_OUTPUT);
		if (ret < 0) {
			LOG_ERR("Could not set reset pin: %d", ret);
			return ret;
		}
		/* Reset is active low, has 1ms settling time*/
		gpio_pin_set_dt(&config->reset, 0);
		k_msleep(1);
		gpio_pin_set_dt(&config->reset, 1);
		k_msleep(1);
	}
#endif

	/*
	 * Read product ID from camera. This camera implements the SCCB,
	 * spec- which *should* be I2C compatible, but in practice does
	 * not seem to respond when I2C repeated start commands are used.
	 * To work around this, use a write then a read to interface with
	 * registers.
	 */
	uint8_t cmd = OV7670_PID;

	ret = i2c_write_dt(&config->bus, &cmd, sizeof(cmd));
	if (ret < 0) {
		LOG_ERR("Could not request product ID: %d", ret);
		return ret;
	}
	ret = i2c_read_dt(&config->bus, &pid, sizeof(pid));
	if (ret < 0) {
		LOG_ERR("Could not read product ID: %d", ret);
		return ret;
	}

	if (pid != OV7670_PROD_ID) {
		LOG_ERR("Incorrect product ID: 0x%02X", pid);
		return -ENODEV;
	}

	/* Reset camera registers */
	ret = i2c_reg_write_byte_dt(&config->bus, OV7670_COM7, 0x80);
	if (ret < 0) {
		LOG_ERR("Could not reset camera: %d", ret);
		return ret;
	}
	/* Delay after reset */
	k_msleep(5);

	/* Set default camera format (QVGA, YUYV) */
	fmt.pixelformat = VIDEO_PIX_FMT_YUYV;
	fmt.width = 640;
	fmt.height = 480;
	fmt.pitch = fmt.width * 2;
	ret = ov7670_set_fmt(dev, VIDEO_EP_OUT, &fmt);
	if (ret < 0) {
		return ret;
	}

	/* Write initialization values to OV7670 */
	for (i = 0; i < ARRAY_SIZE(ov7670_init_regtbl); i++) {
		reg = &ov7670_init_regtbl[i];
		ret = i2c_reg_write_byte_dt(&config->bus, reg->reg, reg->cmd);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static DEVICE_API(video, ov7670_api) = {
	.set_format = ov7670_set_fmt,
	.get_format = ov7670_get_fmt,
	.get_caps = ov7670_get_caps,
};

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
#define OV7670_RESET_GPIO(inst) .reset = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {}),
#else
#define OV7670_RESET_GPIO(inst)
#endif

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(pwdn_gpios)
#define OV7670_PWDN_GPIO(inst) .pwdn = GPIO_DT_SPEC_INST_GET_OR(inst, pwdn_gpios, {}),
#else
#define OV7670_PWDN_GPIO(inst)
#endif

#define OV7670_INIT(inst)                                                                          \
	const struct ov7670_config ov7670_config_##inst = {.bus = I2C_DT_SPEC_INST_GET(inst),      \
							   OV7670_RESET_GPIO(inst)                 \
								   OV7670_PWDN_GPIO(inst)};        \
	struct ov7670_data ov7670_data_##inst;                                                     \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, ov7670_init, NULL, &ov7670_data_##inst, &ov7670_config_##inst, \
			      POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY, &ov7670_api);

DT_INST_FOREACH_STATUS_OKAY(OV7670_INIT)
