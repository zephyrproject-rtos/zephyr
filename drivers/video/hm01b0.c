/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT himax_hm01b0

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/video-controls.h>
#include <zephyr/drivers/video.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "video_device.h"
#include "video_ctrls.h"
#include "video_common.h"

LOG_MODULE_REGISTER(hm01b0, CONFIG_VIDEO_LOG_LEVEL);
#define MAX_FRAME_RATE 10
#define MIN_FRAME_RATE 1
#define HM01B0_ID      0x01B0
#define HM01B0_LINE_LEN_PCLK_IDX 5

#define HM01B0_REG8(addr)             ((addr) | VIDEO_REG_ADDR16_DATA8)
#define HM01B0_REG16(addr)            ((addr) | VIDEO_REG_ADDR16_DATA16_BE)
#define HM01B0_CCI_ID                 HM01B0_REG16(0x0000)
#define HM01B0_CCI_IMG_ORIENTATION    HM01B0_REG8(0x0101)
#define HM01B0_CCI_STS                HM01B0_REG8(0x0100)
#define HM01B0_CCI_RESET              HM01B0_REG8(0x0103)
#define HM01B0_CCI_GRP_PARAM_HOLD     HM01B0_REG8(0x0104)
#define HM01B0_CCI_INTEGRATION_H      HM01B0_REG16(0x0202)
#define HM01B0_CCI_FRAME_LENGTH_LINES HM01B0_REG16(0x0340)
#define HM01B0_CCI_LINE_LENGTH_PCLK   HM01B0_REG16(0x0342)
#define HM01B0_CCI_WIDTH              HM01B0_REG8(0x0383)
#define HM01B0_CCI_HEIGHT             HM01B0_REG8(0x0387)
#define HM01B0_CCI_BINNING_MODE       HM01B0_REG8(0x0390)
#define HM01B0_CCI_QVGA_WIN_EN        HM01B0_REG8(0x3010)
#define HM01B0_CCI_BIT_CONTROL        HM01B0_REG8(0x3059)
#define HM01B0_CCI_OSC_CLOCK_DIV      HM01B0_REG8(0x3060)

#define HM01B0_SET_HMIRROR(r, x)      ((r & 0xFE) | ((x & 1) << 0))
#define HM01B0_SET_VMIRROR(r, x)      ((r & 0xFD) | ((x & 1) << 1))

#define HM01B0_CTRL_VAL(data_bits) \
	((data_bits) == 8 ? 0x02 : \
	(data_bits) == 4 ? 0x42 : \
	(data_bits) == 1 ? 0x22 : 0x00)

enum hm01b0_resolution {
	RESOLUTION_160x120,
	RESOLUTION_320x240,
	RESOLUTION_320x320,
	RESOLUTION_164x122,
	RESOLUTION_326x244,
	RESOLUTION_326x324,
	RESOLUTION_164x122_Y4,
	RESOLUTION_326x244_Y4,
	RESOLUTION_326x324_Y4,
	RESOLUTION_164x122_BAYER,
	RESOLUTION_326x244_BAYER,
	RESOLUTION_326x324_BAYER,
};

struct video_reg hm01b0_160x120_regs[] = {
	{HM01B0_CCI_WIDTH, 0x3},
	{HM01B0_CCI_HEIGHT, 0x3},
	{HM01B0_CCI_BINNING_MODE, 0x3},
	{HM01B0_CCI_QVGA_WIN_EN, 0x1},
	{HM01B0_CCI_FRAME_LENGTH_LINES, 0x80},
	{HM01B0_CCI_LINE_LENGTH_PCLK, 0xD7},
};

struct video_reg hm01b0_320x240_regs[] = {
	{HM01B0_CCI_WIDTH, 0x1},
	{HM01B0_CCI_HEIGHT, 0x1},
	{HM01B0_CCI_BINNING_MODE, 0x0},
	{HM01B0_CCI_QVGA_WIN_EN, 0x1},
	{HM01B0_CCI_FRAME_LENGTH_LINES, 0x104},
	{HM01B0_CCI_LINE_LENGTH_PCLK, 0x178},
};

struct video_reg hm01b0_320x320_regs[] = {
	{HM01B0_CCI_WIDTH, 0x1},
	{HM01B0_CCI_HEIGHT, 0x1},
	{HM01B0_CCI_BINNING_MODE, 0x0},
	{HM01B0_CCI_QVGA_WIN_EN, 0x0},
	{HM01B0_CCI_FRAME_LENGTH_LINES, 0x158},
	{HM01B0_CCI_LINE_LENGTH_PCLK, 0x178},
};

struct video_reg *hm01b0_init_regs[] = {
	[RESOLUTION_160x120] = hm01b0_160x120_regs,
	[RESOLUTION_320x240] = hm01b0_320x240_regs,
	[RESOLUTION_320x320] = hm01b0_320x320_regs,
	[RESOLUTION_164x122] = hm01b0_160x120_regs,
	[RESOLUTION_326x244] = hm01b0_320x240_regs,
	[RESOLUTION_326x324] = hm01b0_320x320_regs,
	[RESOLUTION_164x122_Y4] = hm01b0_160x120_regs,
	[RESOLUTION_326x244_Y4] = hm01b0_320x240_regs,
	[RESOLUTION_326x324_Y4] = hm01b0_320x320_regs,
	[RESOLUTION_164x122_BAYER] = hm01b0_160x120_regs,
	[RESOLUTION_326x244_BAYER] = hm01b0_320x240_regs,
	[RESOLUTION_326x324_BAYER] = hm01b0_320x320_regs,
};

struct hm01b0_ctrls {
	struct video_ctrl hflip;
	struct video_ctrl vflip;
};

struct hm01b0_data {
	struct hm01b0_ctrls ctrls;
	struct video_format fmt;
	uint16_t cur_frmrate;
};

struct hm01b0_config {
	const struct i2c_dt_spec i2c;
	const uint8_t data_bits;
	const uint8_t ctrl_val;
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
	struct gpio_dt_spec reset;
#endif
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(pwdn_gpios)
	struct gpio_dt_spec pwdn;
#endif
};

#define HM01B0_VIDEO_FORMAT_CAP(width, height, format)                                             \
	{                                                                                          \
		.pixelformat = (format),                                                           \
		.width_min = (width),                                                              \
		.width_max = (width),                                                              \
		.height_min = (height),                                                            \
		.height_max = (height),                                                            \
		.width_step = 0,                                                                   \
		.height_step = 0,                                                                  \
	}

static const struct video_format_cap hm01b0_fmts[] = {
	HM01B0_VIDEO_FORMAT_CAP(160, 120, VIDEO_PIX_FMT_GREY),
	HM01B0_VIDEO_FORMAT_CAP(320, 240, VIDEO_PIX_FMT_GREY),
	HM01B0_VIDEO_FORMAT_CAP(320, 320, VIDEO_PIX_FMT_GREY),
	HM01B0_VIDEO_FORMAT_CAP(164, 122, VIDEO_PIX_FMT_GREY),
	HM01B0_VIDEO_FORMAT_CAP(326, 244, VIDEO_PIX_FMT_GREY),
	HM01B0_VIDEO_FORMAT_CAP(326, 324, VIDEO_PIX_FMT_GREY),
	HM01B0_VIDEO_FORMAT_CAP(164, 122, VIDEO_PIX_FMT_Y4),
	HM01B0_VIDEO_FORMAT_CAP(326, 324, VIDEO_PIX_FMT_Y4),
	HM01B0_VIDEO_FORMAT_CAP(326, 244, VIDEO_PIX_FMT_Y4),
	HM01B0_VIDEO_FORMAT_CAP(164, 122, VIDEO_PIX_FMT_SBGGR8),
	HM01B0_VIDEO_FORMAT_CAP(326, 324, VIDEO_PIX_FMT_SBGGR8),
	HM01B0_VIDEO_FORMAT_CAP(326, 244, VIDEO_PIX_FMT_SBGGR8),
	{0},
};

static int hm01b0_apply_configuration(const struct device *dev, enum hm01b0_resolution resolution)
{
	const struct hm01b0_config *config = dev->config;
	int ret;

	/* Number of registers is the same for all configuration */
	ret = video_write_cci_multiregs(&config->i2c, hm01b0_init_regs[resolution],
					ARRAY_SIZE(hm01b0_160x120_regs));
	if (ret < 0) {
		LOG_ERR("Failed to write config list registers (%d)", ret);
		return ret;
	}

	/* REG_BIT_CONTROL */
	ret = video_write_cci_reg(&config->i2c, HM01B0_CCI_BIT_CONTROL, config->ctrl_val);
	if (ret < 0) {
		LOG_ERR("Failed to write BIT_CONTROL reg (%d)", ret);
		return ret;
	}

	/* OSC_CLK_DIV */
	ret = video_write_cci_reg(&config->i2c, HM01B0_CCI_OSC_CLOCK_DIV, 0x08);
	if (ret < 0) {
		LOG_ERR("Failed to write OSC_CLK_DIV reg (%d)", ret);
		return ret;
	}

	/* INTEGRATION_H */
	ret = video_write_cci_reg(&config->i2c, HM01B0_CCI_INTEGRATION_H,
				  hm01b0_init_regs[resolution][HM01B0_LINE_LEN_PCLK_IDX].data / 2);
	if (ret < 0) {
		LOG_ERR("Failed to write INTEGRATION_H reg (%d)", ret);
		return ret;
	}

	/* GRP_PARAM_HOLD */
	ret = video_write_cci_reg(&config->i2c, HM01B0_CCI_GRP_PARAM_HOLD, 0x01);
	if (ret < 0) {
		LOG_ERR("Failed to write GRP_PARAM_HOLD reg (%d)", ret);
		return ret;
	}

	return ret;
}

static int hm01b0_get_caps(const struct device *dev, struct video_caps *caps)
{
	caps->format_caps = hm01b0_fmts;

	return 0;
}

static int hm01b0_set_fmt(const struct device *dev, struct video_format *fmt)
{
	struct hm01b0_data *data = dev->data;
	size_t idx;
	int ret;

	LOG_DBG("HM01B0 set_fmt: %d x %d, fmt: %s", fmt->width, fmt->height,
		VIDEO_FOURCC_TO_STR(fmt->pixelformat));

	ret = video_format_caps_index(hm01b0_fmts, fmt, &idx);
	if (ret != 0) {
		LOG_ERR("Image resolution not supported\n");
		return ret;
	}

	if (!memcmp(&data->fmt, fmt, sizeof(data->fmt))) {
		return 0;
	}

	/* Check if camera is capable of handling given format */
	ret = hm01b0_apply_configuration(dev, (enum hm01b0_resolution)idx);
	if (ret != 0) {
		/* Camera is not capable of handling given format */
		LOG_ERR("Image resolution not supported");
		return ret;
	}
	data->fmt = *fmt;

	return ret;
}

static int hm01b0_get_fmt(const struct device *dev, struct video_format *fmt)
{
	struct hm01b0_data *data = dev->data;

	*fmt = data->fmt;
	LOG_DBG("HM01B0 get_fmt: %d x %d, fmt: %s", fmt->width, fmt->height,
		VIDEO_FOURCC_TO_STR(fmt->pixelformat));

	return 0;
}

static int hm01b0_set_stream(const struct device *dev, bool enable, enum video_buf_type type)
{
	const struct hm01b0_config *config = dev->config;

	/* SET MODE_SELECT */
	return video_write_cci_reg(&config->i2c, HM01B0_CCI_STS, enable ? 1 : 0);
}

static int hm01b0_soft_reset(const struct device *dev)
{
	const struct hm01b0_config *config = dev->config;
	uint32_t val = 0xff;
	int ret;

	ret = video_write_cci_reg(&config->i2c, HM01B0_CCI_RESET, 0x01);
	if (ret < 0) {
		LOG_ERR("Error writing HM01B0_CCI_RESET (%d)", ret);
		return ret;
	}

	for (int retries = 0; retries < 10; retries++) {
		ret = video_read_cci_reg(&config->i2c, HM01B0_CCI_STS, &val);
		if (ret != 0 || val == 0x0) {
			break;
		}
		k_msleep(100);
	}
	if (ret != 0) {
		LOG_ERR("Soft reset error (%d)", ret);
	}

	return ret;
}

static int hm01b0_set_frmival(const struct device *dev, struct video_frmival *frmival)
{
	const struct hm01b0_config *config = dev->config;
	struct video_format fmt;
	struct hm01b0_data *drv_data = dev->data;
	int ret;

	uint32_t osc_div = 0;
	bool highres = false;

	ret = hm01b0_get_fmt(dev, &fmt);
	if (ret < 0) {
		LOG_ERR("Can not get format!");
		return ret;
	}

	if (fmt.width == 320) {
		highres = true;
	}


	if (frmival->numerator <= 15) {
		osc_div = highres ? 0x01 : 0x00;
	} else if (frmival->numerator <= 30) {
		osc_div = highres ? 0x02 : 0x01;
	} else if (frmival->numerator <= 60) {
		osc_div = highres ? 0x03 : 0x02;
	} else {
		/* Set to the max possible FPS at this resolution. */
		osc_div = 3;
	}

	osc_div |= 0x8;

	ret = video_write_cci_reg(&config->i2c, HM01B0_CCI_OSC_CLOCK_DIV, osc_div);
	if (ret < 0) {
		LOG_ERR("Failed to write OSC_CLK_DIV = %x reg (%d)", osc_div, ret);
		return ret;
	}

	/* GRP_PARAM_HOLD */
	ret = video_write_cci_reg(&config->i2c, HM01B0_CCI_GRP_PARAM_HOLD, 0x01);
	if (ret < 0) {
		LOG_ERR("Failed to write GRP_PARAM_HOLD reg (%d)", ret);
		return ret;
	}


	drv_data->cur_frmrate = frmival->numerator;

	LOG_DBG("FrameRate selected: %d", frmival->numerator);
	LOG_DBG("HIRES Selected: %d", highres);
	LOG_DBG("OSC DIV: %d", osc_div);

	return 0;
}

static int hm01b0_get_frmival(const struct device *dev, struct video_frmival *frmival)
{
	struct hm01b0_data *drv_data = dev->data;

	frmival->numerator = drv_data->cur_frmrate;
	frmival->denominator = 1;

	return 0;
}

static int hm01b0_init_controls(const struct device *dev)
{
	int ret;
	struct hm01b0_data *drv_data = dev->data;
	struct hm01b0_ctrls *ctrls = &drv_data->ctrls;

	ret = video_init_ctrl(&ctrls->hflip, dev, VIDEO_CID_HFLIP,
			      (struct video_ctrl_range){.min = 0, .max = 1, .step = 1, .def = 0});
	if (ret) {
		return ret;
	}

	return video_init_ctrl(&ctrls->vflip, dev, VIDEO_CID_VFLIP,
			       (struct video_ctrl_range){.min = 0, .max = 1, .step = 1, .def = 0});
}

static int hm01b0_set_ctrl(const struct device *dev, uint32_t id)
{
	const struct hm01b0_config *config = dev->config;
	struct hm01b0_data *drv_data = dev->data;
	struct hm01b0_ctrls *ctrls = &drv_data->ctrls;

	int ret;
	uint32_t data;

	switch (id) {
	case VIDEO_CID_HFLIP:
		ret = video_read_cci_reg(&config->i2c, HM01B0_CCI_IMG_ORIENTATION, &data);
		if (ret < 0) {
			return ret;
		}
		ret = video_write_cci_reg(&config->i2c, HM01B0_CCI_IMG_ORIENTATION,
					  HM01B0_SET_HMIRROR(data, ctrls->hflip.val));
		if (ret < 0) {
			return ret;
		}
		ret = video_write_cci_reg(&config->i2c, HM01B0_CCI_GRP_PARAM_HOLD, 0x01);
		if (ret < 0) {
			return ret;
		}
		break;
	case VIDEO_CID_VFLIP:
		ret = video_read_cci_reg(&config->i2c, HM01B0_CCI_IMG_ORIENTATION, &data);
		if (ret < 0) {
			return ret;
		}
		ret = video_write_cci_reg(&config->i2c, HM01B0_CCI_IMG_ORIENTATION,
					  HM01B0_SET_VMIRROR(data, ctrls->vflip.val));
		if (ret < 0) {
			return ret;
		}
		ret = video_write_cci_reg(&config->i2c, HM01B0_CCI_GRP_PARAM_HOLD, 0x01);
		if (ret < 0) {
			return ret;
		}
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}


static DEVICE_API(video, hm01b0_driver_api) = {
	.set_format = hm01b0_set_fmt,
	.get_format = hm01b0_get_fmt,
	.set_ctrl = hm01b0_set_ctrl,
	.set_stream = hm01b0_set_stream,
	.get_caps = hm01b0_get_caps,
	.set_frmival = hm01b0_set_frmival,
	.get_frmival = hm01b0_get_frmival,
};

static bool hm01b0_check_connection(const struct device *dev)
{
	const struct hm01b0_config *config = dev->config;
	uint32_t model_id;
	int ret;

	ret = video_read_cci_reg(&config->i2c, HM01B0_CCI_ID, &model_id);
	if (ret < 0) {
		LOG_ERR("Error reading id reg (%d)", ret);
		return false;
	}

	return (model_id == HM01B0_ID);
}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(pwdn_gpios) || DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
static bool hm01b0_try_reset_pwdn_pins(const struct device *dev, uint8_t iter)
{
	const struct hm01b0_config *config = dev->config;

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(pwdn_gpios)
	gpio_pin_set_dt(&config->pwdn, iter >> 1);
#else
	if (iter >> 1) {
		return false; /* cut iterations in half if not defined */
	}
#endif
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
	gpio_pin_set_dt(&config->reset, iter & 1);
#else
	if (iter & 1) {
		return false; /* cut iterations in half if not defined */
	}
#endif
	/* lets try a couple of iterations before we punt */
	uint8_t retry_count = 3;

	while (retry_count) {
		k_sleep(K_MSEC(10));
		if (hm01b0_check_connection(dev)) {
			LOG_INF("Reset/pwdn pins valid:%u", retry_count);
			return true;
		}
		retry_count--;
	}
	return false;
}
#endif

static int hm01b0_init(const struct device *dev)
{
	const struct hm01b0_config __maybe_unused *config = dev->config;
	int ret;

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(pwdn_gpios) || DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(pwdn_gpios)
	/* Power up camera module */
	if (config->pwdn.port != NULL) {
		if (!gpio_is_ready_dt(&config->pwdn)) {
			return -ENODEV;
		}
		ret = gpio_pin_configure_dt(&config->pwdn, GPIO_OUTPUT_ACTIVE);
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
		ret = gpio_pin_configure_dt(&config->reset, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Could not set reset pin: %d", ret);
			return ret;
		}
	}
#endif
	bool found_id = false;

	for (uint8_t iter = 0; iter < 4; iter++) {
		found_id = hm01b0_try_reset_pwdn_pins(dev, iter);
		if (found_id) {
			break;
		}
	}
#endif


	LOG_INF("hm01b0_init check connection");
	if (!hm01b0_check_connection(dev)) {
		LOG_ERR("%s is not ready", dev->name);
		return -ENODEV;
	}

	ret = hm01b0_soft_reset(dev);
	if (ret != 0) {
		LOG_ERR("error soft reset (%d)", ret);
		return ret;
	}

	struct video_format fmt = {
		.pixelformat = VIDEO_PIX_FMT_GREY,
		.width = 160,
		.height = 120,
		.type = VIDEO_BUF_TYPE_OUTPUT,
	};

	ret = hm01b0_set_fmt(dev, &fmt);
	if (ret != 0) {
		LOG_ERR("Error setting video format (%d)", ret);
		return ret;
	}


	/* Initialize controls */
	return hm01b0_init_controls(dev);
}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(reset_gpios)
#define HM01B0_RESET_GPIO(inst) .reset = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {}),
#else
#define HM01B0_RESET_GPIO(inst)
#endif

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(pwdn_gpios)
#define HM01B0_PWDN_GPIO(inst) .pwdn = GPIO_DT_SPEC_INST_GET_OR(inst, pwdn_gpios, {}),
#else
#define HM01B0_PWDN_GPIO(inst)
#endif

#define HM01B0_INIT(inst)                                                                       \
	const struct hm01b0_config hm01b0_config_##inst = {                                     \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                              \
		.data_bits = DT_INST_PROP(inst, data_bits),                                     \
		.ctrl_val = HM01B0_CTRL_VAL(DT_INST_PROP(inst, data_bits)),                     \
		HM01B0_RESET_GPIO(inst)								\
		HM01B0_PWDN_GPIO(inst)								\
	};                                                                                      \
	struct hm01b0_data hm01b0_data_##inst;                                                  \
	DEVICE_DT_INST_DEFINE(inst, &hm01b0_init, NULL, &hm01b0_data_##inst,                    \
			      &hm01b0_config_##inst, POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY,   \
			      &hm01b0_driver_api);                                              \
	VIDEO_DEVICE_DEFINE(hm01b0_##inst, DEVICE_DT_INST_GET(inst), NULL);

DT_INST_FOREACH_STATUS_OKAY(HM01B0_INIT)
