/*
 * Copyright (c) 2020, FrankLi Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ovti_ov7725
#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include <zephyr/sys/byteorder.h>

#include <zephyr/drivers/video.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ov7725);

#define OV7725_REVISION  0x7721U

#define OV7725_GAIN       0x00U
#define OV7725_BLUE       0x01U
#define OV7725_RED        0x02U
#define OV7725_GREEN      0x03U
#define OV7725_BAVG       0x05U
#define OV7725_GAVG       0x06U
#define OV7725_RAVG       0x07U
#define OV7725_AECH       0x08U
#define OV7725_COM2       0x09U
#define OV7725_PID        0x0AU
#define OV7725_VER        0x0BU
#define OV7725_COM3       0x0CU
#define OV7725_COM4       0x0DU
#define OV7725_COM5       0x0EU
#define OV7725_COM6       0x0FU
#define OV7725_AEC        0x10U
#define OV7725_CLKRC      0x11U
#define OV7725_COM7       0x12U
#define OV7725_COM8       0x13U
#define OV7725_COM9       0x14U
#define OV7725_COM10      0x15U
#define OV7725_REG16      0x16U
#define OV7725_HSTART     0x17U
#define OV7725_HSIZE      0x18U
#define OV7725_VSTART     0x19U
#define OV7725_VSIZE      0x1AU
#define OV7725_PSHFT      0x1BU
#define OV7725_MIDH       0x1CU
#define OV7725_MIDL       0x1DU
#define OV7725_LAEC       0x1FU
#define OV7725_COM11      0x20U
#define OV7725_BDBASE     0x22U
#define OV7725_BDMSTEP    0x23U
#define OV7725_AEW        0x24U
#define OV7725_AEB        0x25U
#define OV7725_VPT        0x26U
#define OV7725_REG28      0x28U
#define OV7725_HOUTSIZE   0x29U
#define OV7725_EXHCH      0x2AU
#define OV7725_EXHCL      0x2BU
#define OV7725_VOUTSIZE   0x2CU
#define OV7725_ADVFL      0x2DU
#define OV7725_ADVFH      0x2EU
#define OV7725_YAVE       0x2FU
#define OV7725_LUMHTH     0x30U
#define OV7725_LUMLTH     0x31U
#define OV7725_HREF       0x32U
#define OV7725_DM_LNL     0x33U
#define OV7725_DM_LNH     0x34U
#define OV7725_ADOFF_B    0x35U
#define OV7725_ADOFF_R    0x36U
#define OV7725_ADOFF_GB   0x37U
#define OV7725_ADOFF_GR   0x38U
#define OV7725_OFF_B      0x39U
#define OV7725_OFF_R      0x3AU
#define OV7725_OFF_GB     0x3BU
#define OV7725_OFF_GR     0x3CU
#define OV7725_COM12      0x3DU
#define OV7725_COM13      0x3EU
#define OV7725_COM14      0x3FU
#define OV7725_COM16      0x41U
#define OV7725_TGT_B      0x42U
#define OV7725_TGT_R      0x43U
#define OV7725_TGT_GB     0x44U
#define OV7725_TGT_GR     0x45U
#define OV7725_LC_CTR     0x46U
#define OV7725_LC_XC      0x47U
#define OV7725_LC_YC      0x48U
#define OV7725_LC_COEF    0x49U
#define OV7725_LC_RADI    0x4AU
#define OV7725_LC_COEFB   0x4BU
#define OV7725_LC_COEFR   0x4CU
#define OV7725_FIXGAIN    0x4DU
#define OV7725_AREF1      0x4FU
#define OV7725_AREF6      0x54U
#define OV7725_UFIX       0x60U
#define OV7725_VFIX       0x61U
#define OV7725_AWBB_BLK   0x62U
#define OV7725_AWB_CTRL0  0x63U
#define OV7725_DSP_CTRL1  0x64U
#define OV7725_DSP_CTRL2  0x65U
#define OV7725_DSP_CTRL3  0x66U
#define OV7725_DSP_CTRL4  0x67U
#define OV7725_AWB_BIAS   0x68U
#define OV7725_AWB_CTRL1  0x69U
#define OV7725_AWB_CTRL2  0x6AU
#define OV7725_AWB_CTRL3  0x6BU
#define OV7725_AWB_CTRL4  0x6CU
#define OV7725_AWB_CTRL5  0x6DU
#define OV7725_AWB_CTRL6  0x6EU
#define OV7725_AWB_CTRL7  0x6FU
#define OV7725_AWB_CTRL8  0x70U
#define OV7725_AWB_CTRL9  0x71U
#define OV7725_AWB_CTRL10 0x72U
#define OV7725_AWB_CTRL11 0x73U
#define OV7725_AWB_CTRL12 0x74U
#define OV7725_AWB_CTRL13 0x75U
#define OV7725_AWB_CTRL14 0x76U
#define OV7725_AWB_CTRL15 0x77U
#define OV7725_AWB_CTRL16 0x78U
#define OV7725_AWB_CTRL17 0x79U
#define OV7725_AWB_CTRL18 0x7AU
#define OV7725_AWB_CTRL19 0x7BU
#define OV7725_AWB_CTRL20 0x7CU
#define OV7725_AWB_CTRL21 0x7DU
#define OV7725_GAM1       0x7EU
#define OV7725_GAM2       0x7FU
#define OV7725_GAM3       0x80U
#define OV7725_GAM4       0x81U
#define OV7725_GAM5       0x82U
#define OV7725_GAM6       0x83U
#define OV7725_GAM7       0x84U
#define OV7725_GAM8       0x85U
#define OV7725_GAM9       0x86U
#define OV7725_GAM10      0x87U
#define OV7725_GAM11      0x88U
#define OV7725_GAM12      0x89U
#define OV7725_GAM13      0x8AU
#define OV7725_GAM14      0x8BU
#define OV7725_GAM15      0x8CU
#define OV7725_SLOP       0x8DU
#define OV7725_DNSTH      0x8EU
#define OV7725_EDGE0      0x8FU
#define OV7725_EDGE1      0x90U
#define OV7725_DNSOFF     0x91U
#define OV7725_EDGE2      0x92U
#define OV7725_EDGE3      0x93U
#define OV7725_MTX1       0x94U
#define OV7725_MTX2       0x95U
#define OV7725_MTX3       0x96U
#define OV7725_MTX4       0x97U
#define OV7725_MTX5       0x98U
#define OV7725_MTX6       0x99U
#define OV7725_MTX_CTRL   0x9AU
#define OV7725_BRIGHT     0x9BU
#define OV7725_CNST       0x9CU
#define OV7725_UVADJ0     0x9EU
#define OV7725_UVADJ1     0x9FU
#define OV7725_SCAL0      0xA0U
#define OV7725_SCAL1      0xA1U
#define OV7725_SCAL2      0xA2U
#define OV7725_SDE        0xA6U
#define OV7725_USAT       0xA7U
#define OV7725_VSAT       0xA8U
#define OV7725_HUECOS     0xA9U
#define OV7725_HUESIN     0xAAU
#define OV7725_SIGN       0xABU
#define OV7725_DSPAUTO    0xACU

#define OV7725_COM10_VSYNC_NEG_MASK    BIT(1)
#define OV7725_COM10_HREF_REVERSE_MASK BIT(3)
#define OV7725_COM10_PCLK_REVERSE_MASK BIT(4)
#define OV7725_COM10_PCLK_OUT_MASK     BIT(5)
#define OV7725_COM10_DATA_NEG_MASK     BIT(7)

struct ov7725_config {
	struct i2c_dt_spec i2c;
#if DT_INST_NODE_HAS_PROP(0, reset_gpios)
	struct gpio_dt_spec reset_gpio;
#endif
};

struct ov7725_data {
	struct video_format fmt;
};

struct ov7725_clock {
	uint32_t input_clk;
	uint32_t framerate;
	uint8_t clkrc;  /*!< Register CLKRC. */
	uint8_t com4;   /*!< Register COM4. */
	uint8_t dm_lnl; /*!< Register DM_LNL. */
};

struct ov7725_pixel_format {
	uint32_t pixel_format;
	uint8_t com7;
};

struct ov7725_reg {
	uint8_t addr;
	uint8_t value;
};

static const struct ov7725_clock ov7725_clock_configs[] = {
	{ .input_clk = 24000000, .framerate = 30,
		.clkrc = 0x01, .com4 = 0x41, .dm_lnl = 0x00 },
	{ .input_clk = 24000000, .framerate = 15,
		.clkrc = 0x03, .com4 = 0x41, .dm_lnl = 0x00 },
	{ .input_clk = 24000000, .framerate = 25,
		.clkrc = 0x01, .com4 = 0x41, .dm_lnl = 0x66 },
	{ .input_clk = 24000000, .framerate = 14,
		.clkrc = 0x03, .com4 = 0x41, .dm_lnl = 0x1a },
	{ .input_clk = 26000000, .framerate = 30,
		.clkrc = 0x01, .com4 = 0x41, .dm_lnl = 0x2b },
	{ .input_clk = 26000000, .framerate = 15,
		.clkrc = 0x03, .com4 = 0x41, .dm_lnl = 0x2b },
	{ .input_clk = 26000000, .framerate = 25,
		.clkrc = 0x01, .com4 = 0x41, .dm_lnl = 0x99 },
	{ .input_clk = 26000000, .framerate = 14,
		.clkrc = 0x03, .com4 = 0x41, .dm_lnl = 0x46 },
	{ .input_clk = 13000000, .framerate = 30,
		.clkrc = 0x00, .com4 = 0x41, .dm_lnl = 0x2b },
	{ .input_clk = 13000000, .framerate = 15,
		.clkrc = 0x01, .com4 = 0x41, .dm_lnl = 0x2b },
	{ .input_clk = 13000000, .framerate = 25,
		.clkrc = 0x00, .com4 = 0x41, .dm_lnl = 0x99 },
	{ .input_clk = 13000000, .framerate = 14,
		.clkrc = 0x01, .com4 = 0x41, .dm_lnl = 0x46 },
};


static const struct ov7725_pixel_format ov7725_pf_configs[] = {
	{ .pixel_format = VIDEO_PIX_FMT_RGB565, .com7 = (1 << 2) | (2) }
};

static const struct ov7725_reg ov7725_init_reg_tb[] = {
	/*Output config*/
	{ OV7725_CLKRC,          0x00 },
	{ OV7725_COM7,           0x06 },
	{ OV7725_HSTART,         0x3f },
	{ OV7725_HSIZE,          0x50 },
	{ OV7725_VSTART,         0x03 },
	{ OV7725_VSIZE,          0x78 },
	{ OV7725_HREF,           0x00 },
	{ OV7725_HOUTSIZE,       0x50 },
	{ OV7725_VOUTSIZE,       0x78 },

	/*DSP control*/
	{ OV7725_TGT_B,          0x7f },
	{ OV7725_FIXGAIN,        0x09 },
	{ OV7725_AWB_CTRL0,      0xe0 },
	{ OV7725_DSP_CTRL1,      0xff },
	{ OV7725_DSP_CTRL2,      0x00 },
	{ OV7725_DSP_CTRL3,      0x00 },
	{ OV7725_DSP_CTRL4,      0x00 },

	/*AGC AEC AWB*/
	{ OV7725_COM8,           0xf0 },
	{ OV7725_COM4,           0x81 },
	{ OV7725_COM6,           0xc5 },
	{ OV7725_COM9,           0x11 },
	{ OV7725_BDBASE,         0x7F },
	{ OV7725_BDMSTEP,        0x03 },
	{ OV7725_AEW,            0x40 },
	{ OV7725_AEB,            0x30 },
	{ OV7725_VPT,            0xa1 },
	{ OV7725_EXHCL,          0x9e },
	{ OV7725_AWB_CTRL3,      0xaa },
	{ OV7725_COM8,           0xff },

	/*matrix sharpness brightness contrast*/
	{ OV7725_EDGE1,          0x08 },
	{ OV7725_DNSOFF,         0x01 },
	{ OV7725_EDGE2,          0x03 },
	{ OV7725_EDGE3,          0x00 },
	{ OV7725_MTX1,           0xb0 },
	{ OV7725_MTX2,           0x9d },
	{ OV7725_MTX3,           0x13 },
	{ OV7725_MTX4,           0x16 },
	{ OV7725_MTX5,           0x7b },
	{ OV7725_MTX6,           0x91 },
	{ OV7725_MTX_CTRL,       0x1e },
	{ OV7725_BRIGHT,         0x08 },
	{ OV7725_CNST,           0x20 },
	{ OV7725_UVADJ0,         0x81 },
	{ OV7725_SDE,            0X06 },
	{ OV7725_USAT,           0x65 },
	{ OV7725_VSAT,           0x65 },
	{ OV7725_HUECOS,         0X80 },
	{ OV7725_HUESIN,         0X80 },

	/*GAMMA config*/
	{ OV7725_GAM1,           0x0c },
	{ OV7725_GAM2,           0x16 },
	{ OV7725_GAM3,           0x2a },
	{ OV7725_GAM4,           0x4e },
	{ OV7725_GAM5,           0x61 },
	{ OV7725_GAM6,           0x6f },
	{ OV7725_GAM7,           0x7b },
	{ OV7725_GAM8,           0x86 },
	{ OV7725_GAM9,           0x8e },
	{ OV7725_GAM10,          0x97 },
	{ OV7725_GAM11,          0xa4 },
	{ OV7725_GAM12,          0xaf },
	{ OV7725_GAM13,          0xc5 },
	{ OV7725_GAM14,          0xd7 },
	{ OV7725_GAM15,          0xe8 },
	{ OV7725_SLOP,           0x20 },

	{ OV7725_COM3,           0x40 },
	{ OV7725_COM5,           0xf5 },
	{ OV7725_COM10,          0x02 },
	{ OV7725_COM2,           0x01 }
};

static int ov7725_write_reg(const struct i2c_dt_spec *spec, uint8_t reg_addr,
			    uint8_t value)
{
	struct i2c_msg msgs[2];

	msgs[0].buf = (uint8_t *)&reg_addr;
	msgs[0].len = 1;
	msgs[0].flags = I2C_MSG_WRITE;

	msgs[1].buf = (uint8_t *)&value;
	msgs[1].len = 1;
	msgs[1].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	return i2c_transfer_dt(spec, msgs, 2);
}

static int ov7725_read_reg(const struct i2c_dt_spec *spec, uint8_t reg_addr,
			   uint8_t *value)
{
	struct i2c_msg msgs[2];

	msgs[0].buf = (uint8_t *)&reg_addr;
	msgs[0].len = 1;
	/*
	 * When using I2C to read the registers of the SCCB device,
	 * a stop bit is required after writing the register address
	 */
	msgs[0].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	msgs[1].buf = (uint8_t *)value;
	msgs[1].len = 1;
	msgs[1].flags = I2C_MSG_READ | I2C_MSG_STOP | I2C_MSG_RESTART;

	return i2c_transfer_dt(spec, msgs, 2);
}

int ov7725_modify_reg(const struct i2c_dt_spec *spec,
		      uint8_t reg_addr,
		      uint8_t clear_mask,
		      uint8_t value)
{
	int ret;
	uint8_t set_value;

	ret = ov7725_read_reg(spec, reg_addr, &set_value);

	if (ret == 0) {
		set_value = (set_value & (~clear_mask)) |
				(set_value & clear_mask);
		ret = ov7725_write_reg(spec, reg_addr, set_value);
	}


	return ret;
}

static int ov7725_write_all(const struct device *dev,
			    const struct ov7725_reg *regs,
			    uint16_t reg_num)
{
	uint16_t i = 0;
	const struct ov7725_config *cfg = dev->config;

	for (i = 0; i < reg_num; i++) {
		int err;

		err = ov7725_write_reg(&cfg->i2c, regs[i].addr, regs[i].value);
		if (err) {
			return err;
		}
	}

	return 0;
}

static int ov7725_set_clock(const struct device *dev,
				unsigned int framerate,
				unsigned int input_clk)
{
	const struct ov7725_config *cfg = dev->config;

	for (unsigned int i = 0; i < ARRAY_SIZE(ov7725_clock_configs); i++) {
		if ((ov7725_clock_configs[i].framerate == framerate) &&
			(ov7725_clock_configs[i].input_clk == input_clk)) {
			ov7725_write_reg(&cfg->i2c, OV7725_CLKRC,
						ov7725_clock_configs[i].clkrc);
			ov7725_modify_reg(&cfg->i2c, OV7725_COM4, 0xc0,
						ov7725_clock_configs[i].com4);
			ov7725_write_reg(&cfg->i2c, OV7725_EXHCL, 0x00);
			ov7725_write_reg(&cfg->i2c, OV7725_DM_LNL,
						ov7725_clock_configs[i].dm_lnl);
			ov7725_write_reg(&cfg->i2c, OV7725_DM_LNH, 0x00);
			ov7725_write_reg(&cfg->i2c, OV7725_ADVFL, 0x00);
			ov7725_write_reg(&cfg->i2c, OV7725_ADVFH, 0x00);
			return ov7725_write_reg(&cfg->i2c, OV7725_COM5, 0x65);
		}
	}

	return -1;
}

static int ov7725_set_fmt(const struct device *dev,
			  enum video_endpoint_id ep,
			  struct video_format *fmt)
{
	struct ov7725_data *drv_data = dev->data;
	const struct ov7725_config *cfg = dev->config;
	uint8_t com10 = 0;
	uint16_t width, height;
	uint16_t hstart, vstart, hsize;
	int ret;

	/* we only support one format for now (VGA RGB565) */
	if (fmt->pixelformat != VIDEO_PIX_FMT_RGB565 || fmt->height != 480 ||
	    fmt->width != 640) {
		return -ENOTSUP;
	}

	width = fmt->width;
	height = fmt->height;

	if (!memcmp(&drv_data->fmt, fmt, sizeof(drv_data->fmt))) {
		/* nothing to do */
		return 0;
	}

	drv_data->fmt = *fmt;

	/* Configure Sensor */
	ret = ov7725_write_all(dev, ov7725_init_reg_tb,
				ARRAY_SIZE(ov7725_init_reg_tb));
	if (ret) {
		LOG_ERR("Unable to write ov7725 config");
		return ret;
	}

	/* Set clock : framerate 30fps, input clock 24M*/
	ov7725_set_clock(dev, 30, 24000000);

	/* Set output format */
	for (uint8_t i = 0; i < ARRAY_SIZE(ov7725_pf_configs); i++) {
		if (ov7725_pf_configs[i].pixel_format == fmt->pixelformat) {
			ret =  ov7725_modify_reg(&cfg->i2c,
						OV7725_COM7,
						0x1FU,
						ov7725_pf_configs[i].com7);
			if (ret) {
				LOG_ERR("Unable to write ov7725 pixel format");
				return ret;
			}
		}
	}

	ov7725_modify_reg(&cfg->i2c, OV7725_COM7, (1 << 5), (0 << 5));

	com10 |= OV7725_COM10_VSYNC_NEG_MASK;
	ov7725_write_reg(&cfg->i2c, OV7725_COM10, com10);

	/* Don't swap output MSB/LSB. */
	ov7725_write_reg(&cfg->i2c, OV7725_COM3, 0x00);

	/*
	 * Output drive capability
	 * 0: 1X
	 * 1: 2X
	 * 2: 3X
	 * 3: 4X
	 */
	ov7725_modify_reg(&cfg->i2c, OV7725_COM2, 0x03, 0x03);

	/* Resolution and timing. */
	hstart = 0x22U << 2U;
	vstart = 0x07U << 1U;
	hsize = width + 16U;

	/* Set the window size. */
	ov7725_write_reg(&cfg->i2c, OV7725_HSTART, hstart >> 2U);
	ov7725_write_reg(&cfg->i2c, OV7725_HSIZE, hsize >> 2U);
	ov7725_write_reg(&cfg->i2c, OV7725_VSTART, vstart >> 1U);
	ov7725_write_reg(&cfg->i2c, OV7725_VSIZE, height >> 1U);
	ov7725_write_reg(&cfg->i2c, OV7725_HOUTSIZE, width >> 2U);
	ov7725_write_reg(&cfg->i2c, OV7725_VOUTSIZE, height >> 1U);
	ov7725_write_reg(&cfg->i2c, OV7725_HREF,
			 ((vstart & 1U) << 6U) |
			 ((hstart & 3U) << 4U) |
			 ((height & 1U) << 2U) |
			 ((hsize & 3U) << 0U));
	return ov7725_write_reg(&cfg->i2c, OV7725_EXHCH,
					((height & 1U) << 2U) |
					((width & 3U) << 0U));
}

static int ov7725_get_fmt(const struct device *dev,
			  enum video_endpoint_id ep,
			  struct video_format *fmt)
{
	struct ov7725_data *drv_data = dev->data;

	*fmt = drv_data->fmt;

	return 0;
}

static int ov7725_stream_start(const struct device *dev)
{
	return 0;
}

static int ov7725_stream_stop(const struct device *dev)
{
	return 0;
}

static const struct video_format_cap fmts[] = {
	{
		.pixelformat = VIDEO_PIX_FMT_RGB565,
		.width_min = 640,
		.width_max = 640,
		.height_min = 480,
		.height_max = 480,
		.width_step = 0,
		.height_step = 0,
	},
	{ 0 }
};

static int ov7725_get_caps(const struct device *dev,
			   enum video_endpoint_id ep,
			   struct video_caps *caps)
{
	caps->format_caps = fmts;
	return 0;
}

static const struct video_driver_api ov7725_driver_api = {
	.set_format = ov7725_set_fmt,
	.get_format = ov7725_get_fmt,
	.get_caps = ov7725_get_caps,
	.stream_start = ov7725_stream_start,
	.stream_stop = ov7725_stream_stop,
};

static int ov7725_init(const struct device *dev)
{
	const struct ov7725_config *cfg = dev->config;
	struct video_format fmt;
	uint8_t pid, ver;
	int ret;

#if DT_INST_NODE_HAS_PROP(0, reset_gpios)
	ret = gpio_pin_configure_dt(&cfg->reset_gpio, GPIO_OUTPUT_ACTIVE);
	if (ret) {
		return ret;
	}

	gpio_pin_set_dt(&cfg->reset_gpio, 0);
	k_sleep(K_MSEC(1));
	gpio_pin_set_dt(&cfg->reset_gpio, 1);
	k_sleep(K_MSEC(1));
#endif

	/* Identify the device. */
	ret = ov7725_read_reg(&cfg->i2c, OV7725_PID, &pid);
	if (ret) {
		LOG_ERR("Unable to read PID");
		return -ENODEV;
	}

	ret = ov7725_read_reg(&cfg->i2c, OV7725_VER, &ver);
	if (ret) {
		LOG_ERR("Unable to read VER");
		return -ENODEV;
	}

	if (OV7725_REVISION != (((uint32_t)pid << 8U) | (uint32_t)ver)) {
		LOG_ERR("OV7725 Get Vision fail\n");
		return -ENODEV;
	}

	/* Device identify OK, perform software reset. */
	ov7725_write_reg(&cfg->i2c, OV7725_COM7, 0x80);

	k_sleep(K_MSEC(2));

	/* set default/init format VGA RGB565 */
	fmt.pixelformat = VIDEO_PIX_FMT_RGB565;
	fmt.width = 640;
	fmt.height = 480;
	fmt.pitch = 640 * 2;
	ret = ov7725_set_fmt(dev, VIDEO_EP_OUT, &fmt);
	if (ret) {
		LOG_ERR("Unable to configure default format");
		return -EIO;
	}

	return 0;
}

/* Unique Instance */
static const struct ov7725_config ov7725_cfg_0 = {
	.i2c = I2C_DT_SPEC_INST_GET(0),
#if DT_INST_NODE_HAS_PROP(0, reset_gpios)
	.reset_gpio = GPIO_DT_SPEC_INST_GET(0, reset_gpios),
#endif
};
static struct ov7725_data ov7725_data_0;

static int ov7725_init_0(const struct device *dev)
{
	const struct ov7725_config *cfg = dev->config;

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

#if DT_INST_NODE_HAS_PROP(0, reset_gpios)
	if (!gpio_is_ready_dt(&cfg->reset_gpio)) {
		LOG_ERR("%s: device %s is not ready", dev->name,
				cfg->reset_gpio.port->name);
		return -ENODEV;
	}
#endif

	return ov7725_init(dev);
}

DEVICE_DT_INST_DEFINE(0, &ov7725_init_0, NULL,
		    &ov7725_data_0, &ov7725_cfg_0,
		    POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY,
		    &ov7725_driver_api);
