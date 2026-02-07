/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT onnn_ap1302

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/drivers/video.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(ap1302, CONFIG_VIDEO_LOG_LEVEL);

/* AP1302 Register Definitions */
#define AP1302_CHIP_VERSION			0x0000
#define AP1302_CHIP_ID				0x0265
#define AP1302_CHIP_REV				0x0050

/* Preview Context Registers */
#define AP1302_PREVIEW_WIDTH			0x2000
#define AP1302_PREVIEW_HEIGHT			0x2002
#define AP1302_PREVIEW_OUT_FMT			0x2012
#define AP1302_PREVIEW_HINF_CTRL		0x2030

/* PREVIEW_OUT_FMT bits */
#define AP1302_PREVIEW_OUT_FMT_FT_YUV		(3U << 4)
#define AP1302_PREVIEW_OUT_FMT_FST_YUV_422	(0U << 0)

/* PREVIEW_HINF_CTRL bits */
#define AP1302_PREVIEW_HINF_CTRL_SPOOF		BIT(4)
#define AP1302_PREVIEW_HINF_CTRL_MIPI_LANES(n)	((n) << 0)

/* System Registers */
#define AP1302_BOOTDATA_STAGE			0x6002
#define AP1302_SYS_START			0x601A
#define AP1302_BOOTDATA_CHECKSUM		0x6134

/* SYS_START bits */
#define AP1302_SYS_START_PLL_LOCK		BIT(15)
#define AP1302_SYS_START_STALL_STATUS		BIT(9)
#define AP1302_SYS_START_STALL_EN		BIT(8)
#define AP1302_SYS_START_STALL_MODE_DISABLED	(1U << 6)

/* Misc Registers */
#define AP1302_SIP_CRC				0xF052

/* Firmware load window */
#define AP1302_FW_WINDOW_OFFSET			0x8000
#define AP1302_FW_WINDOW_SIZE			0x2000

/* Output size constraints */
#define AP1302_MIN_WIDTH			24U
#define AP1302_MIN_HEIGHT			16U
#define AP1302_MAX_WIDTH			4224U
#define AP1302_MAX_HEIGHT			4092U

struct ap1302_firmware_header {
	uint32_t crc;
	uint32_t checksum;
	uint32_t pll_init_size;
	uint32_t total_size;
} __packed;

struct ap1302_format_info {
	uint32_t code;
	uint16_t out_fmt;
};

struct ap1302_config {
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec reset_gpio;
	struct gpio_dt_spec standby_gpio;
	struct gpio_dt_spec isp_en_gpio;

	uint32_t firmware_address;

	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;

	const struct pinctrl_dev_config *pincfg;

	const struct device *const *power_supplies;
	size_t num_power_supplies;

	uint8_t mipi_data_lanes;
};

struct ap1302_data {
	struct video_format fmt;
	const struct ap1302_format_info *info;
	bool streaming;
};

/* Supported video formats */
static const struct ap1302_format_info supported_formats[] = {
	{
		.code = VIDEO_PIX_FMT_YUYV,
		.out_fmt = AP1302_PREVIEW_OUT_FMT_FT_YUV | AP1302_PREVIEW_OUT_FMT_FST_YUV_422,
	},
};

static const struct ap1302_format_info *ap1302_find_format(uint32_t pixelformat)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(supported_formats); i++) {
		if (supported_formats[i].code == pixelformat) {
			return &supported_formats[i];
		}
	}

	/* Return default format if not found */
	return &supported_formats[0];
}

/* Register access functions */
static int ap1302_write_reg16(const struct device *dev, uint16_t reg, uint16_t val)
{
	const struct ap1302_config *cfg = dev->config;
	uint8_t buf[4];
	int ret;

	sys_put_be16(reg, &buf[0]);
	sys_put_be16(val, &buf[2]);

	ret = i2c_write_dt(&cfg->i2c, buf, sizeof(buf));
	if (ret < 0) {
		LOG_ERR("Failed to write reg 0x%04x: %d", reg, ret);
	}

	return ret;
}

static int ap1302_read_reg16(const struct device *dev, uint16_t reg, uint16_t *val)
{
	const struct ap1302_config *cfg = dev->config;
	uint8_t reg_buf[2];
	uint8_t val_buf[2];
	int ret;

	sys_put_be16(reg, reg_buf);

	ret = i2c_write_read_dt(&cfg->i2c, reg_buf, sizeof(reg_buf),
				val_buf, sizeof(val_buf));
	if (ret < 0) {
		LOG_ERR("Failed to read reg 0x%04x: %d", reg, ret);
		return ret;
	}

	*val = sys_get_be16(val_buf);

	return 0;
}

static int ap1302_power_on(const struct device *dev)
{
	const struct ap1302_config *cfg = dev->config;
	int ret;

	for (size_t i = 0; i < cfg->num_power_supplies; i++) {
		if (cfg->power_supplies[i] == NULL) {
			continue;
		}

		ret = regulator_enable(cfg->power_supplies[i]);
		if (ret < 0) {
			LOG_ERR("Failed to enable supply %zu: %d", i, ret);
			/* Disable already enabled supplies */
			for (int j = (int)i - 1; j >= 0; j--) {
				if (cfg->power_supplies[j] != NULL) {
					(void)regulator_disable(cfg->power_supplies[j]);
				}
			}
			return ret;
		}

		/* Small delay after each supply */
		k_usleep(2000);
	}

	return 0;
}

static void ap1302_power_off(const struct device *dev)
{
	const struct ap1302_config *cfg = dev->config;

	/* Disable supplies in reverse order */
	for (int i = (int)cfg->num_power_supplies - 1; i >= 0; i--) {
		if (cfg->power_supplies[i] != NULL) {
			(void)regulator_disable(cfg->power_supplies[i]);
		}
	}
}

static int ap1302_reset_sequence(const struct device *dev)
{
	const struct ap1302_config *cfg = dev->config;
	int ret;

	if (cfg->standby_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->standby_gpio)) {
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&cfg->standby_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			return ret;
		}

		gpio_pin_set_dt(&cfg->standby_gpio, 1);
		k_usleep(1000);
	}

	ret = ap1302_power_on(dev);
	if (ret < 0) {
		return ret;
	}

	if (cfg->standby_gpio.port != NULL) {
		gpio_pin_set_dt(&cfg->standby_gpio, 0);
		k_usleep(1000);
	}

	if (cfg->reset_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->reset_gpio)) {
			ret = -ENODEV;
			goto err_power;
		}

		ret = gpio_pin_configure_dt(&cfg->reset_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			goto err_power;
		}

		gpio_pin_set_dt(&cfg->reset_gpio, 0);
		k_msleep(5);
		gpio_pin_set_dt(&cfg->reset_gpio, 1);
		k_msleep(20);
	}

	return 0;

err_power:
	ap1302_power_off(dev);
	return ret;
}

static int ap1302_write_fw_window(const struct device *dev, const uint8_t *buf,
				  uint32_t len, unsigned int *win_pos)
{
	const struct ap1302_config *cfg = dev->config;
	uint32_t pos = 0;
	int ret;

	while (pos < len) {
		uint32_t write_size;
		uint16_t write_addr;
		uint8_t i2c_buf[258]; /* 2 bytes addr + 256 bytes data */

		write_size = MIN(len - pos, AP1302_FW_WINDOW_SIZE - *win_pos);
		write_size = MIN(write_size, 256U);
		write_addr = (uint16_t)(AP1302_FW_WINDOW_OFFSET + *win_pos);

		sys_put_be16(write_addr, &i2c_buf[0]);
		memcpy(&i2c_buf[2], &buf[pos], write_size);

		/* Write to device */
		ret = i2c_write_dt(&cfg->i2c, i2c_buf, write_size + 2);
		if (ret < 0) {
			LOG_ERR("Firmware write failed at pos %u: %d", pos, ret);
			return ret;
		}

		pos += write_size;
		*win_pos += write_size;

		/* Wrap around window */
		if (*win_pos >= AP1302_FW_WINDOW_SIZE) {
			*win_pos = 0;
		}
	}

	return 0;
}

static int ap1302_load_firmware(const struct device *dev)
{
	const struct ap1302_config *cfg = dev->config;
	const struct ap1302_firmware_header *fw_hdr;
	const uint8_t *fw_data;
	uint32_t fw_size;
	unsigned int win_pos = 0;
	uint16_t checksum;
	int ret;

	fw_hdr = (const struct ap1302_firmware_header *)(uintptr_t)cfg->firmware_address;
	fw_data = (const uint8_t *)&fw_hdr[1];
	fw_size = fw_hdr->total_size;

	LOG_INF("Loading firmware: PLL init size %u, total size %u",
		fw_hdr->pll_init_size, fw_size);

	ret = ap1302_write_reg16(dev, AP1302_SIP_CRC, 0xFFFF);
	if (ret < 0) {
		return ret;
	}

	/* Load PLL initialization settings */
	ret = ap1302_write_fw_window(dev, fw_data, fw_hdr->pll_init_size, &win_pos);
	if (ret < 0) {
		return ret;
	}

	/* Set bootdata stage to apply PLL settings */
	ret = ap1302_write_reg16(dev, AP1302_BOOTDATA_STAGE, 0x0002);
	if (ret < 0) {
		return ret;
	}

	k_msleep(1);

	/* Load rest of firmware */
	ret = ap1302_write_fw_window(dev, fw_data + fw_hdr->pll_init_size,
				     fw_size - fw_hdr->pll_init_size, &win_pos);
	if (ret < 0) {
		return ret;
	}

	k_msleep(40);

	/* Finalize firmware load */
	ret = ap1302_write_reg16(dev, AP1302_BOOTDATA_STAGE, 0xFFFF);
	if (ret < 0) {
		return ret;
	}

	k_msleep(10);

	ret = ap1302_read_reg16(dev, AP1302_BOOTDATA_CHECKSUM, &checksum);
	if (ret < 0) {
		return ret;
	}

	if (checksum != (uint16_t)fw_hdr->checksum) {
		LOG_ERR("Firmware checksum mismatch: expected 0x%04x, got 0x%04x",
			(uint16_t)fw_hdr->checksum, checksum);
		return -EIO;
	}

	LOG_INF("Firmware loaded successfully, checksum verified");

	return 0;
}

static int ap1302_detect_chip(const struct device *dev)
{
	uint16_t chip_version;
	uint16_t chip_rev;
	int ret;

	ret = ap1302_read_reg16(dev, AP1302_CHIP_VERSION, &chip_version);
	if (ret < 0) {
		LOG_ERR("Failed to read chip version: %d", ret);
		return ret;
	}

	if (chip_version != AP1302_CHIP_ID) {
		LOG_ERR("Invalid chip version: expected 0x%04x, got 0x%04x",
			AP1302_CHIP_ID, chip_version);
		return -ENODEV;
	}

	ret = ap1302_read_reg16(dev, AP1302_CHIP_REV, &chip_rev);
	if (ret < 0) {
		LOG_ERR("Failed to read chip revision: %d", ret);
		return ret;
	}

	LOG_INF("AP1302 detected: version 0x%04x, revision %u.%u.%u",
		chip_version,
		(chip_rev & 0xF000) >> 12,
		(chip_rev & 0x0F00) >> 8,
		chip_rev & 0x00FF);

	return 0;
}

static void ap1302_sanitize_format(struct video_format *fmt)
{
	fmt->width = CLAMP(fmt->width, AP1302_MIN_WIDTH, AP1302_MAX_WIDTH);
	fmt->height = CLAMP(fmt->height, AP1302_MIN_HEIGHT, AP1302_MAX_HEIGHT);

	fmt->width = ROUND_DOWN(fmt->width, 4U);
	fmt->height = ROUND_DOWN(fmt->height, 2U);

	/* Ensure minimum size after alignment */
	if (fmt->width < AP1302_MIN_WIDTH) {
		fmt->width = AP1302_MIN_WIDTH;
	}
	if (fmt->height < AP1302_MIN_HEIGHT) {
		fmt->height = AP1302_MIN_HEIGHT;
	}
}

static int ap1302_configure(const struct device *dev)
{
	const struct ap1302_config *cfg = dev->config;
	struct ap1302_data *data = dev->data;
	const struct video_format *fmt = &data->fmt;
	const struct ap1302_format_info *info = data->info;
	int ret;

	/* Configure MIPI interface */
	ret = ap1302_write_reg16(dev, AP1302_PREVIEW_HINF_CTRL,
				 AP1302_PREVIEW_HINF_CTRL_SPOOF |
				 AP1302_PREVIEW_HINF_CTRL_MIPI_LANES(cfg->mipi_data_lanes));
	if (ret < 0) {
		return ret;
	}

	ret = ap1302_write_reg16(dev, AP1302_PREVIEW_OUT_FMT, info->out_fmt);
	if (ret < 0) {
		return ret;
	}

	ret = ap1302_write_reg16(dev, AP1302_PREVIEW_WIDTH, (uint16_t)fmt->width);
	if (ret < 0) {
		return ret;
	}

	ret = ap1302_write_reg16(dev, AP1302_PREVIEW_HEIGHT, (uint16_t)fmt->height);
	if (ret < 0) {
		LOG_ERR("Failed to configure format: %d", ret);
		return ret;
	}

	return 0;
}

static int ap1302_stall(const struct device *dev, bool stall)
{
	int ret;

	if (stall) {
		ret = ap1302_write_reg16(dev, AP1302_SYS_START,
					 AP1302_SYS_START_PLL_LOCK |
					 AP1302_SYS_START_STALL_MODE_DISABLED);
		if (ret < 0) {
			return ret;
		}

		ret = ap1302_write_reg16(dev, AP1302_SYS_START,
					 AP1302_SYS_START_PLL_LOCK |
					 AP1302_SYS_START_STALL_EN |
					 AP1302_SYS_START_STALL_MODE_DISABLED);
		if (ret < 0) {
			return ret;
		}

		k_msleep(200);
	} else {
		ret = ap1302_write_reg16(dev, AP1302_SYS_START,
					 AP1302_SYS_START_PLL_LOCK |
					 AP1302_SYS_START_STALL_STATUS |
					 AP1302_SYS_START_STALL_EN |
					 AP1302_SYS_START_STALL_MODE_DISABLED);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int ap1302_set_format(const struct device *dev, struct video_format *fmt)
{
	struct ap1302_data *data = dev->data;
	const struct ap1302_format_info *info;

	if (fmt == NULL) {
		return -EINVAL;
	}

	info = ap1302_find_format(fmt->pixelformat);
	if (info == NULL) {
		LOG_ERR("Unsupported pixel format 0x%08x", fmt->pixelformat);
		return -ENOTSUP;
	}

	ap1302_sanitize_format(fmt);

	if (!memcmp(&data->fmt, fmt, sizeof(*fmt))) {
		return 0;
	}

	/* Calculate buffer parameters for YUYV (2 bytes per pixel) */
	fmt->pitch = fmt->width * 2;
	fmt->size = fmt->pitch * fmt->height;

	/* Update format */
	data->fmt = *fmt;
	data->info = info;

	LOG_DBG("Format set: %ux%u, pixfmt 0x%08x, pitch %u, size %u",
		fmt->width, fmt->height, fmt->pixelformat, fmt->pitch, fmt->size);

	return 0;
}

static int ap1302_get_format(const struct device *dev, struct video_format *fmt)
{
	struct ap1302_data *data = dev->data;

	if (fmt == NULL) {
		return -EINVAL;
	}

	*fmt = data->fmt;

	return 0;
}

static int ap1302_set_stream(const struct device *dev, bool enable, enum video_buf_type type)
{
	struct ap1302_data *data = dev->data;
	int ret;

	ARG_UNUSED(type);

	if (enable == data->streaming) {
		return 0;
	}

	if (enable) {
		/* Configure format before streaming */
		ret = ap1302_configure(dev);
		if (ret < 0) {
			LOG_ERR("Failed to configure device: %d", ret);
			return ret;
		}

		/* Start streaming */
		ret = ap1302_stall(dev, false);
		if (ret < 0) {
			LOG_ERR("Failed to start streaming: %d", ret);
			return ret;
		}

		LOG_DBG("Streaming started");
	} else {
		/* Stop streaming */
		ret = ap1302_stall(dev, true);
		if (ret < 0) {
			LOG_ERR("Failed to stop streaming: %d", ret);
			return ret;
		}

		LOG_DBG("Streaming stopped");
	}

	data->streaming = enable;

	return 0;
}

static int ap1302_get_caps(const struct device *dev, struct video_caps *caps)
{
	static const struct video_format_cap fmts[] = {
		{
			.pixelformat = VIDEO_PIX_FMT_YUYV,
			.width_min = AP1302_MIN_WIDTH,
			.width_max = AP1302_MAX_WIDTH,
			.height_min = AP1302_MIN_HEIGHT,
			.height_max = AP1302_MAX_HEIGHT,
			.width_step = 4,
			.height_step = 2,
		},
		{ 0 }
	};

	ARG_UNUSED(dev);

	if (caps == NULL) {
		return -EINVAL;
	}

	caps->format_caps = fmts;
	caps->min_vbuf_count = 0;

	return 0;
}

static DEVICE_API(video, ap1302_driver_api) = {
	.set_format = ap1302_set_format,
	.get_format = ap1302_get_format,
	.set_stream = ap1302_set_stream,
	.get_caps = ap1302_get_caps,
};

static int ap1302_init(const struct device *dev)
{
	const struct ap1302_config *cfg = dev->config;
	struct ap1302_data *data = dev->data;
	int ret;

	LOG_DBG("Initializing AP1302");

	if (cfg->pincfg != NULL) {
		ret = pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_DEFAULT);
		if (ret < 0) {
			LOG_ERR("Failed to apply pinctrl state: %d", ret);
			return ret;
		}
	}

	if (cfg->clock_dev != NULL) {
		if (!device_is_ready(cfg->clock_dev)) {
			LOG_ERR("Clock device not ready");
			return -ENODEV;
		}

		ret = clock_control_on(cfg->clock_dev, cfg->clock_subsys);
		if (ret < 0) {
			LOG_ERR("Failed to enable clock: %d", ret);
			return ret;
		}
	}

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR("I2C bus not ready");
		ret = -ENODEV;
		goto err_clock;
	}

	if (cfg->isp_en_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->isp_en_gpio)) {
			LOG_ERR("ISP enable GPIO not ready");
			ret = -ENODEV;
			goto err_clock;
		}

		ret = gpio_pin_configure_dt(&cfg->isp_en_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Failed to configure ISP enable GPIO: %d", ret);
			goto err_clock;
		}

		gpio_pin_set_dt(&cfg->isp_en_gpio, 1);
	}

	ret = ap1302_reset_sequence(dev);
	if (ret < 0) {
		LOG_ERR("Failed to reset device: %d", ret);
		goto err_clock;
	}

	ret = ap1302_detect_chip(dev);
	if (ret < 0) {
		LOG_ERR("Failed to detect chip: %d", ret);
		goto err_power;
	}

	ret = ap1302_load_firmware(dev);
	if (ret < 0) {
		LOG_ERR("Failed to load firmware: %d", ret);
		goto err_power;
	}

	ret = ap1302_stall(dev, true);
	if (ret < 0) {
		LOG_ERR("Failed to stall device: %d", ret);
		goto err_power;
	}

	data->fmt.pixelformat = VIDEO_PIX_FMT_YUYV;
	data->info = ap1302_find_format(VIDEO_PIX_FMT_YUYV);
	data->fmt.width = 1920;
	data->fmt.height = 1080;
	data->fmt.pitch = data->fmt.width * 2;
	data->fmt.size = data->fmt.pitch * data->fmt.height;
	data->streaming = false;

	LOG_INF("AP1302 initialized: %ux%u YUYV", data->fmt.width, data->fmt.height);

	return 0;

err_power:
	ap1302_power_off(dev);
err_clock:
	if (cfg->clock_dev != NULL) {
		(void)clock_control_off(cfg->clock_dev, cfg->clock_subsys);
	}
	return ret;
}

/* Device instantiation macros */
#define AP1302_POWER_SUPPLY(node_id, prop, idx)							\
	DEVICE_DT_GET(DT_PHANDLE_BY_IDX(node_id, prop, idx)),

#define AP1302_INIT_POWER_SUPPLIES(n)								\
	static const struct device *const ap1302_power_supplies_##n[] = {			\
		COND_CODE_1(DT_INST_NODE_HAS_PROP(n, power_supplies),				\
			(DT_INST_FOREACH_PROP_ELEM(n, power_supplies, AP1302_POWER_SUPPLY)),	\
			())									\
	};											\

#define AP1302_NUM_POWER_SUPPLIES(n)								\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, power_supplies),					\
		(DT_INST_PROP_LEN(n, power_supplies)), (0))					\

PINCTRL_DT_INST_DEFINE(0);

#define AP1302_INIT(n)										\
	AP1302_INIT_POWER_SUPPLIES(n)								\
	static const struct ap1302_config ap1302_config_##n = {					\
		.i2c = I2C_DT_SPEC_INST_GET(n),							\
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(n, reset_gpios, {0}),			\
		.standby_gpio = GPIO_DT_SPEC_INST_GET_OR(n, standby_gpios, {0}),		\
		.isp_en_gpio = GPIO_DT_SPEC_INST_GET_OR(n, isp_en_gpios, {0}),			\
		.firmware_address = DT_INST_PROP(n, firmware_address),				\
		.clock_dev = DEVICE_DT_GET_OR_NULL(DT_INST_CLOCKS_CTLR(n)),			\
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_IDX(n, 0, name),	\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),					\
		.power_supplies = ap1302_power_supplies_##n,					\
		.num_power_supplies = AP1302_NUM_POWER_SUPPLIES(n),				\
		.mipi_data_lanes = DT_INST_PROP(n, mipi_data_lanes),				\
	};											\
	static struct ap1302_data ap1302_data_##n;						\
	DEVICE_DT_INST_DEFINE(n,								\
			      ap1302_init,							\
			      NULL,								\
			      &ap1302_data_##n,							\
			      &ap1302_config_##n,						\
			      POST_KERNEL,							\
			      CONFIG_VIDEO_AP1302_INIT_PRIORITY,				\
			      &ap1302_driver_api);

DT_INST_FOREACH_STATUS_OKAY(AP1302_INIT)
