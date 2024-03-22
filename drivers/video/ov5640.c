/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ovti_ov5640

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/video.h>
#include <zephyr/kernel.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ov5640);

#include <zephyr/sys/byteorder.h>

#define CHIP_ID_REG 0x300a
#define CHIP_ID_VAL 0x5640

#define SYS_CTRL0_REG     0x3008
#define SYS_CTRL0_SW_PWDN 0x42
#define SYS_CTRL0_SW_PWUP 0x02
#define SYS_CTRL0_SW_RST  0x82

#define IO_MIPI_CTRL00_REG 0x300e

#define SC_PLL_CTRL0_REG     0x3034
#define SC_PLL_CTRL1_REG     0x3035
#define SC_PLL_CTRL2_REG     0x3036
#define SC_PLL_CTRL3_REG     0x3037
#define SYS_ROOT_DIVIDER_REG 0x3108
#define PCLK_PERIOD_REG      0x4837

#define DEFAULT_MIPI_CHANNEL 0

#define OV5640_RESOLUTION_PARAM_NUM 24

struct ov5640_config {
	struct i2c_dt_spec i2c;
#if DT_INST_NODE_HAS_PROP(0, reset_gpios)
	struct gpio_dt_spec reset_gpio;
#endif
#if DT_INST_NODE_HAS_PROP(0, powerdown_gpios)
	struct gpio_dt_spec powerdown_gpio;
#endif
};

struct ov5640_data {
	struct video_format fmt;
};

struct ov5640_reg {
	uint16_t addr;
	uint8_t val;
};

struct ov5640_mipi_clock_config {
	uint8_t pllCtrl1;
	uint8_t pllCtrl2;
};

struct ov5640_resolution_config {
	uint16_t width;
	uint16_t height;
	const struct ov5640_reg *res_params;
	const struct ov5640_mipi_clock_config mipi_pclk;
};

static const struct ov5640_reg ov5640InitParams[] = {
	/* Power down */
	{0x3008, 0x42},

	/* System setting. */
	{0x3103, 0x13},
	{0x3103, 0x03},
	{0x3000, 0x00},
	{0x3004, 0xff},
	{0x3002, 0x1c},
	{0x3006, 0xc3},
	{0x302e, 0x08},
	{0x3618, 0x00},
	{0x3612, 0x29},
	{0x3708, 0x64},
	{0x3709, 0x52},
	{0x370c, 0x03},
	{0x3820, 0x41},
	{0x3821, 0x07},
	{0x3630, 0x36},
	{0x3631, 0x0e},
	{0x3632, 0xe2},
	{0x3633, 0x12},
	{0x3621, 0xe0},
	{0x3704, 0xa0},
	{0x3703, 0x5a},
	{0x3715, 0x78},
	{0x3717, 0x01},
	{0x370b, 0x60},
	{0x3705, 0x1a},
	{0x3905, 0x02},
	{0x3906, 0x10},
	{0x3901, 0x0a},
	{0x3731, 0x12},
	{0x3600, 0x08},
	{0x3601, 0x33},
	{0x302d, 0x60},
	{0x3620, 0x52},
	{0x371b, 0x20},
	{0x471c, 0x50},
	{0x3a13, 0x43},
	{0x3a18, 0x00},
	{0x3a19, 0x7c},
	{0x3635, 0x13},
	{0x3636, 0x03},
	{0x3634, 0x40},
	{0x3622, 0x01},
	{0x3c01, 0x00},
	{0x3a00, 0x58},
	{0x4001, 0x02},
	{0x4004, 0x02},
	{0x4005, 0x1a},
	{0x5001, 0xa3},

	/* AEC */
	{0x3a0f, 0x30},
	{0x3a10, 0x28},
	{0x3a1b, 0x30},
	{0x3a1e, 0x26},
	{0x3a11, 0x60},
	{0x3a1f, 0x14},

	/* AWB */
	{0x5180, 0xff},
	{0x5181, 0xf2},
	{0x5182, 0x00},
	{0x5183, 0x14},
	{0x5184, 0x25},
	{0x5185, 0x24},
	{0x5186, 0x09},
	{0x5187, 0x09},
	{0x5188, 0x09},
	{0x5189, 0x88},
	{0x518a, 0x54},
	{0x518b, 0xee},
	{0x518c, 0xb2},
	{0x518d, 0x50},
	{0x518e, 0x34},
	{0x518f, 0x6b},
	{0x5190, 0x46},
	{0x5191, 0xf8},
	{0x5192, 0x04},
	{0x5193, 0x70},
	{0x5194, 0xf0},
	{0x5195, 0xf0},
	{0x5196, 0x03},
	{0x5197, 0x01},
	{0x5198, 0x04},
	{0x5199, 0x6c},
	{0x519a, 0x04},
	{0x519b, 0x00},
	{0x519c, 0x09},
	{0x519d, 0x2b},
	{0x519e, 0x38},

	/* Color Matrix */
	{0x5381, 0x1e},
	{0x5382, 0x5b},
	{0x5383, 0x08},
	{0x5384, 0x0a},
	{0x5385, 0x7e},
	{0x5386, 0x88},
	{0x5387, 0x7c},
	{0x5388, 0x6c},
	{0x5389, 0x10},
	{0x538a, 0x01},
	{0x538b, 0x98},

	/* Sharp */
	{0x5300, 0x08},
	{0x5301, 0x30},
	{0x5302, 0x10},
	{0x5303, 0x00},
	{0x5304, 0x08},
	{0x5305, 0x30},
	{0x5306, 0x08},
	{0x5307, 0x16},
	{0x5309, 0x08},
	{0x530a, 0x30},
	{0x530b, 0x04},
	{0x530c, 0x06},

	/* Gamma */
	{0x5480, 0x01},
	{0x5481, 0x08},
	{0x5482, 0x14},
	{0x5483, 0x28},
	{0x5484, 0x51},
	{0x5485, 0x65},
	{0x5486, 0x71},
	{0x5487, 0x7d},
	{0x5488, 0x87},
	{0x5489, 0x91},
	{0x548a, 0x9a},
	{0x548b, 0xaa},
	{0x548c, 0xb8},
	{0x548d, 0xcd},
	{0x548e, 0xdd},
	{0x548f, 0xea},
	{0x5490, 0x1d},

	/* UV adjust. */
	{0x5580, 0x02},
	{0x5583, 0x40},
	{0x5584, 0x10},
	{0x5589, 0x10},
	{0x558a, 0x00},
	{0x558b, 0xf8},

	/* Lens correction. */
	{0x5800, 0x23},
	{0x5801, 0x14},
	{0x5802, 0x0f},
	{0x5803, 0x0f},
	{0x5804, 0x12},
	{0x5805, 0x26},
	{0x5806, 0x0c},
	{0x5807, 0x08},
	{0x5808, 0x05},
	{0x5809, 0x05},
	{0x580a, 0x08},
	{0x580b, 0x0d},
	{0x580c, 0x08},
	{0x580d, 0x03},
	{0x580e, 0x00},
	{0x580f, 0x00},
	{0x5810, 0x03},
	{0x5811, 0x09},
	{0x5812, 0x07},
	{0x5813, 0x03},
	{0x5814, 0x00},
	{0x5815, 0x01},
	{0x5816, 0x03},
	{0x5817, 0x08},
	{0x5818, 0x0d},
	{0x5819, 0x08},
	{0x581a, 0x05},
	{0x581b, 0x06},
	{0x581c, 0x08},
	{0x581d, 0x0e},
	{0x581e, 0x29},
	{0x581f, 0x17},
	{0x5820, 0x11},
	{0x5821, 0x11},
	{0x5822, 0x15},
	{0x5823, 0x28},
	{0x5824, 0x46},
	{0x5825, 0x26},
	{0x5826, 0x08},
	{0x5827, 0x26},
	{0x5828, 0x64},
	{0x5829, 0x26},
	{0x582a, 0x24},
	{0x582b, 0x22},
	{0x582c, 0x24},
	{0x582d, 0x24},
	{0x582e, 0x06},
	{0x582f, 0x22},
	{0x5830, 0x40},
	{0x5831, 0x42},
	{0x5832, 0x24},
	{0x5833, 0x26},
	{0x5834, 0x24},
	{0x5835, 0x22},
	{0x5836, 0x22},
	{0x5837, 0x26},
	{0x5838, 0x44},
	{0x5839, 0x24},
	{0x583a, 0x26},
	{0x583b, 0x28},
	{0x583c, 0x42},
	{0x583d, 0xce},
	{0x5000, 0xa7},
};

static const struct ov5640_reg ov5640_low_res_params[] = {
	{0x3800, 0x00}, {0x3801, 0x00}, {0x3802, 0x00}, {0x3803, 0x04}, {0x3804, 0x0a},
	{0x3805, 0x3f}, {0x3806, 0x07}, {0x3807, 0x9b}, {0x3808, 0x02}, {0x3809, 0x80},
	{0x380a, 0x01}, {0x380b, 0xe0}, {0x380c, 0x07}, {0x380d, 0x68}, {0x380e, 0x03},
	{0x380f, 0xd8}, {0x3810, 0x00}, {0x3811, 0x10}, {0x3812, 0x00}, {0x3813, 0x06},
	{0x3814, 0x31}, {0x3815, 0x31}, {0x3824, 0x02}, {0x460c, 0x22}};

static const struct ov5640_reg ov5640_720p_res_params[] = {
	{0x3800, 0x00}, {0x3801, 0x00}, {0x3802, 0x00}, {0x3803, 0xfa}, {0x3804, 0x0a},
	{0x3805, 0x3f}, {0x3806, 0x06}, {0x3807, 0xa9}, {0x3808, 0x05}, {0x3809, 0x00},
	{0x380a, 0x02}, {0x380b, 0xd0}, {0x380c, 0x07}, {0x380d, 0x64}, {0x380e, 0x02},
	{0x380f, 0xe4}, {0x3810, 0x00}, {0x3811, 0x10}, {0x3812, 0x00}, {0x3813, 0x04},
	{0x3814, 0x31}, {0x3815, 0x31}, {0x3824, 0x04}, {0x460c, 0x20}};

static const struct ov5640_resolution_config resolutionParams[] = {
	{.width = 640,
	 .height = 480,
	 .res_params = ov5640_low_res_params,
	 .mipi_pclk = {
			 .pllCtrl1 = 0x14,
			 .pllCtrl2 = 0x38,
		 }},
	{.width = 1280,
	 .height = 720,
	 .res_params = ov5640_720p_res_params,
	 .mipi_pclk = {
			 .pllCtrl1 = 0x21,
			 .pllCtrl2 = 0x54,
		 }},
};

#define OV5640_VIDEO_FORMAT_CAP(width, height, format)                                             \
	{                                                                                          \
		.pixelformat = (format), .width_min = (width), .width_max = (width),               \
		.height_min = (height), .height_max = (height), .width_step = 0, .height_step = 0  \
	}

static const struct video_format_cap fmts[] = {
	OV5640_VIDEO_FORMAT_CAP(1280, 720, VIDEO_PIX_FMT_RGB565),
	OV5640_VIDEO_FORMAT_CAP(1280, 720, VIDEO_PIX_FMT_YUYV),
	OV5640_VIDEO_FORMAT_CAP(640, 480, VIDEO_PIX_FMT_RGB565),
	OV5640_VIDEO_FORMAT_CAP(640, 480, VIDEO_PIX_FMT_YUYV),
	{0}};

static int ov5640_read_reg(const struct i2c_dt_spec *spec, const uint16_t addr, void *val,
			   const uint8_t val_size)
{
	int ret;
	struct i2c_msg msg[2];
	uint8_t addr_buf[2];

	if (val_size > 4) {
		return -ENOTSUP;
	}

	addr_buf[1] = addr & 0xFF;
	addr_buf[0] = addr >> 8;
	msg[0].buf = addr_buf;
	msg[0].len = 2U;
	msg[0].flags = I2C_MSG_WRITE;

	msg[1].buf = (uint8_t *)val;
	msg[1].len = val_size;
	msg[1].flags = I2C_MSG_READ | I2C_MSG_STOP | I2C_MSG_RESTART;

	ret = i2c_transfer_dt(spec, msg, 2);
	if (ret) {
		return ret;
	}

	switch (val_size) {
	case 4:
		*(uint32_t *)val = sys_be32_to_cpu(*(uint32_t *)val);
		break;
	case 2:
		*(uint16_t *)val = sys_be16_to_cpu(*(uint16_t *)val);
		break;
	case 1:
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int ov5640_write_reg(const struct i2c_dt_spec *spec, const uint16_t addr, const uint8_t val)
{
	uint8_t addr_buf[2];
	struct i2c_msg msg[2];

	addr_buf[1] = addr & 0xFF;
	addr_buf[0] = addr >> 8;
	msg[0].buf = addr_buf;
	msg[0].len = 2U;
	msg[0].flags = I2C_MSG_WRITE;

	msg[1].buf = (uint8_t *)&val;
	msg[1].len = 1;
	msg[1].flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	return i2c_transfer_dt(spec, msg, 2);
}

static int ov5640_modify_reg(const struct i2c_dt_spec *spec, const uint16_t addr,
			     const uint8_t mask, const uint8_t val)
{
	uint8_t regVal = 0;
	int ret = ov5640_read_reg(spec, addr, &regVal, sizeof(regVal));

	if (ret) {
		return ret;
	}

	return ov5640_write_reg(spec, addr, (regVal & ~mask) | (val & mask));
}

static int ov5640_write_multi_regs(const struct i2c_dt_spec *spec, const struct ov5640_reg *regs,
				   const uint32_t num_regs)
{
	int ret;

	for (int i = 0; i < num_regs; i++) {
		ret = ov5640_write_reg(spec, regs[i].addr, regs[i].val);
		if (ret) {
			return ret;
		}
	}

	return 0;
}

static int ov5640_set_fmt(const struct device *dev, enum video_endpoint_id ep,
			  struct video_format *fmt)
{
	struct ov5640_data *drv_data = dev->data;
	const struct ov5640_config *cfg = dev->config;
	int ret;
	int i;

	for (i = 0; i < ARRAY_SIZE(fmts); ++i) {
		if (fmt->pixelformat == fmts[i].pixelformat && fmt->width >= fmts[i].width_min &&
		    fmt->width <= fmts[i].width_max && fmt->height >= fmts[i].height_min &&
		    fmt->height <= fmts[i].height_max) {
			break;
		}
	}

	if (i == ARRAY_SIZE(fmts)) {
		LOG_ERR("Unsupported pixel format or resolution");
		return -ENOTSUP;
	}

	if (!memcmp(&drv_data->fmt, fmt, sizeof(drv_data->fmt))) {
		return 0;
	}

	drv_data->fmt = *fmt;

	/* Set resolution parameters */
	for (i = 0; i < ARRAY_SIZE(resolutionParams); i++) {
		if (fmt->width == resolutionParams[i].width &&
		    fmt->height == resolutionParams[i].height) {
			ret = ov5640_write_multi_regs(&cfg->i2c, resolutionParams[i].res_params,
						      OV5640_RESOLUTION_PARAM_NUM);
			if (ret) {
				LOG_ERR("Unable to set resolution parameters");
				return ret;
			}
			break;
		}
	}

	/* Set pixel format, default to VIDEO_PIX_FMT_RGB565 */
	struct ov5640_reg fmt_params[2] = {
		{0x4300, 0x6f},
		{0x501f, 0x01},
	};

	if (fmt->pixelformat == VIDEO_PIX_FMT_YUYV) {
		fmt_params[0].val = 0x3f;
		fmt_params[1].val = 0x00;
	}

	ret = ov5640_write_multi_regs(&cfg->i2c, fmt_params, ARRAY_SIZE(fmt_params));
	if (ret) {
		LOG_ERR("Unable to set pixel format");
		return ret;
	}

	/* Configure MIPI pixel clock */
	ret |= ov5640_modify_reg(&cfg->i2c, SC_PLL_CTRL0_REG, 0x0f, 0x08);
	ret |= ov5640_modify_reg(&cfg->i2c, SC_PLL_CTRL1_REG, 0xff,
				 resolutionParams[i].mipi_pclk.pllCtrl1);
	ret |= ov5640_modify_reg(&cfg->i2c, SC_PLL_CTRL2_REG, 0xff,
				 resolutionParams[i].mipi_pclk.pllCtrl2);
	ret |= ov5640_modify_reg(&cfg->i2c, SC_PLL_CTRL3_REG, 0x1f, 0x13);
	ret |= ov5640_modify_reg(&cfg->i2c, SYS_ROOT_DIVIDER_REG, 0x3f, 0x01);
	ret |= ov5640_write_reg(&cfg->i2c, PCLK_PERIOD_REG, 0x0a);
	if (ret) {
		LOG_ERR("Unable to configure MIPI pixel clock");
		return ret;
	}

	return 0;
}

static int ov5640_get_fmt(const struct device *dev, enum video_endpoint_id ep,
			  struct video_format *fmt)
{
	struct ov5640_data *drv_data = dev->data;

	*fmt = drv_data->fmt;

	return 0;
}

static int ov5640_get_caps(const struct device *dev, enum video_endpoint_id ep,
			   struct video_caps *caps)
{
	caps->format_caps = fmts;
	return 0;
}

static int ov5640_stream_start(const struct device *dev)
{
	const struct ov5640_config *cfg = dev->config;
	/* Power up MIPI PHY HS Tx & LP Rx in 2 data lanes mode */
	int ret = ov5640_write_reg(&cfg->i2c, IO_MIPI_CTRL00_REG, 0x45);

	if (ret) {
		LOG_ERR("Unable to power up MIPI PHY");
		return ret;
	}
	return ov5640_write_reg(&cfg->i2c, SYS_CTRL0_REG, SYS_CTRL0_SW_PWUP);
}

static int ov5640_stream_stop(const struct device *dev)
{
	const struct ov5640_config *cfg = dev->config;
	/* Power down MIPI PHY HS Tx & LP Rx */
	int ret = ov5640_write_reg(&cfg->i2c, IO_MIPI_CTRL00_REG, 0x40);

	if (ret) {
		LOG_ERR("Unable to power down MIPI PHY");
		return ret;
	}
	return ov5640_write_reg(&cfg->i2c, SYS_CTRL0_REG, SYS_CTRL0_SW_PWDN);
}

static const struct video_driver_api ov5640_driver_api = {
	.set_format = ov5640_set_fmt,
	.get_format = ov5640_get_fmt,
	.get_caps = ov5640_get_caps,
	.stream_start = ov5640_stream_start,
	.stream_stop = ov5640_stream_stop,
};

static int ov5640_init(const struct device *dev)
{
	const struct ov5640_config *cfg = dev->config;
	struct video_format fmt;
	uint16_t chip_id;
	int ret;

	/* Power up sequence */
#if DT_INST_NODE_HAS_PROP(0, powerdown_gpios)
	ret = gpio_pin_configure_dt(&cfg->powerdown_gpio, GPIO_OUTPUT_ACTIVE);
	if (ret) {
		return ret;
	}
#endif

#if DT_INST_NODE_HAS_PROP(0, reset_gpios)
	ret = gpio_pin_configure_dt(&cfg->reset_gpio, GPIO_OUTPUT_ACTIVE);
	if (ret) {
		return ret;
	}
#endif

	k_sleep(K_MSEC(5));

#if DT_INST_NODE_HAS_PROP(0, powerdown_gpios)
	gpio_pin_set_dt(&cfg->powerdown_gpio, 0);
#endif

	k_sleep(K_MSEC(1));

#if DT_INST_NODE_HAS_PROP(0, reset_gpios)
	gpio_pin_set_dt(&cfg->reset_gpio, 0);
#endif

	k_sleep(K_MSEC(20));

	/* Software reset */
	ret = ov5640_write_reg(&cfg->i2c, SYS_CTRL0_REG, SYS_CTRL0_SW_RST);
	if (ret) {
		LOG_ERR("Unable to perform software reset");
		return -EIO;
	}

	k_sleep(K_MSEC(5));

	/* Initialize register values */
	ret = ov5640_write_multi_regs(&cfg->i2c, ov5640InitParams, ARRAY_SIZE(ov5640InitParams));
	if (ret) {
		LOG_ERR("Unable to initialize the sensor");
		return -EIO;
	}

	/* Set virtual channel */
	ret = ov5640_modify_reg(&cfg->i2c, 0x4814, 3U << 6, (uint8_t)(DEFAULT_MIPI_CHANNEL) << 6);
	if (ret) {
		LOG_ERR("Unable to set virtual channel");
		return -EIO;
	}

	/* Check sensor chip id */
	ret = ov5640_read_reg(&cfg->i2c, CHIP_ID_REG, &chip_id, sizeof(chip_id));
	if (ret) {
		LOG_ERR("Unable to read sensor chip ID, ret = %d", ret);
		return -ENODEV;
	}

	if (chip_id != CHIP_ID_VAL) {
		LOG_ERR("Wrong chip ID: %04x (expected %04x)", chip_id, CHIP_ID_VAL);
		return -ENODEV;
	}

	/* Set default format to 720p RGB565 */
	fmt.pixelformat = VIDEO_PIX_FMT_RGB565;
	fmt.width = 1280;
	fmt.height = 720;
	fmt.pitch = fmt.width * 2;
	ret = ov5640_set_fmt(dev, VIDEO_EP_OUT, &fmt);
	if (ret) {
		LOG_ERR("Unable to configure default format");
		return -EIO;
	}

	return 0;
}

static const struct ov5640_config ov5640_cfg_0 = {
	.i2c = I2C_DT_SPEC_INST_GET(0),
#if DT_INST_NODE_HAS_PROP(0, reset_gpios)
	.reset_gpio = GPIO_DT_SPEC_INST_GET(0, reset_gpios),
#endif
#if DT_INST_NODE_HAS_PROP(0, powerdown_gpios)
	.powerdown_gpio = GPIO_DT_SPEC_INST_GET(0, powerdown_gpios),
#endif
};

static struct ov5640_data ov5640_data_0;

static int ov5640_init_0(const struct device *dev)
{
	const struct ov5640_config *cfg = dev->config;

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

#if DT_INST_NODE_HAS_PROP(0, reset_gpios)
	if (!gpio_is_ready_dt(&cfg->reset_gpio)) {
		LOG_ERR("%s: device %s is not ready", dev->name, cfg->reset_gpio.port->name);
		return -ENODEV;
	}
#endif

#if DT_INST_NODE_HAS_PROP(0, powerdown_gpios)
	if (!gpio_is_ready_dt(&cfg->powerdown_gpio)) {
		LOG_ERR("%s: device %s is not ready", dev->name, cfg->powerdown_gpio.port->name);
		return -ENODEV;
	}
#endif

	return ov5640_init(dev);
}

DEVICE_DT_INST_DEFINE(0, &ov5640_init_0, NULL, &ov5640_data_0, &ov5640_cfg_0, POST_KERNEL,
		      CONFIG_VIDEO_INIT_PRIORITY, &ov5640_driver_api);
