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
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video-controls.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#if CONFIG_VIDEO_AP1302_BUILTIN_FIRMWARE
extern const uint8_t ap1302_fw_blob[];
extern const size_t ap1302_fw_blob_len;
#endif

#include <zephyr/drivers/regulator.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "../video_common.h"
#include "../video_ctrls.h"
#include "../video_device.h"

LOG_MODULE_REGISTER(ap1302, CONFIG_VIDEO_LOG_LEVEL);

#define AP1302_REG16(addr) ((addr) | VIDEO_REG_ADDR16_DATA16_BE)

/* AP1302 Register Definitions */

#define AP1302_CHIP_ID				0x0265
#define AP1302_CHIP_VERSION			AP1302_REG16(0x0000)
#define AP1302_CHIP_REV				AP1302_REG16(0x0050)

/* Preview Context Registers */
#define AP1302_PREVIEW_WIDTH			AP1302_REG16(0x2000)
#define AP1302_PREVIEW_HEIGHT			AP1302_REG16(0x2002)
#define AP1302_PREVIEW_MAX_FPS			AP1302_REG16(0x2020)
#define AP1302_PREVIEW_OUT_FMT			AP1302_REG16(0x2012)
#define AP1302_PREVIEW_HINF_CTRL		AP1302_REG16(0x2030)

#define AP1302_FPS_Q8_8(fps) ((uint16_t)((fps) << 8))

#define AP1302_PREVIEW_OUT_FMT_FT_YUV		(3U << 4)
#define AP1302_PREVIEW_OUT_FMT_FST_YUV_422	(0U << 0)
#define AP1302_PREVIEW_HINF_CTRL_SPOOF		BIT(4)
#define AP1302_PREVIEW_HINF_CTRL_MIPI_LANES(n)	((n) << 0)

#define AP1302_BOOTDATA_STAGE			AP1302_REG16(0x6002)
#define AP1302_SYS_START			AP1302_REG16(0x601A)
#define AP1302_BOOTDATA_CHECKSUM		AP1302_REG16(0x6134)

#define AP1302_SYS_START_PLL_LOCK		BIT(15)
#define AP1302_SYS_START_STALL_STATUS		BIT(9)
#define AP1302_SYS_START_STALL_EN		BIT(8)
#define AP1302_SYS_START_STALL_MODE_DISABLED	(1U << 6)

#define AP1302_SIP_CRC				AP1302_REG16(0xF052)
#define AP1302_HINF_MIPI_FREQ			AP1302_REG16(0x0068)

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

	uintptr_t firmware_partition_addr;
	size_t firmware_partition_size;

	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;

	const struct pinctrl_dev_config *pincfg;

	const struct device *const *power_supplies;
	size_t num_power_supplies;

	uint8_t mipi_data_lanes;
};

struct ap1302_ctrls {
	struct video_ctrl linkfreq;
};

struct ap1302_data {
	struct ap1302_ctrls ctrls;
	struct video_format fmt;
	struct video_frmival frmival;
	const struct ap1302_format_info *info;
	bool streaming;
};

static const int64_t ap1302_link_frequencies[] = {
	445000000,
};

static const struct ap1302_format_info supported_formats[] = {
	{
		.code = VIDEO_PIX_FMT_YUYV,
		.out_fmt = AP1302_PREVIEW_OUT_FMT_FT_YUV | AP1302_PREVIEW_OUT_FMT_FST_YUV_422,
	},
};

static const struct video_format_cap ap1302_fmts[] = {
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

static const struct ap1302_format_info *ap1302_find_format(uint32_t pixelformat)
{
	for (size_t i = 0; i < ARRAY_SIZE(supported_formats); i++) {
		if (supported_formats[i].code == pixelformat) {
			return &supported_formats[i];
		}
	}

	return NULL;
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
	uint32_t checksum;
	int ret;

#if defined(CONFIG_VIDEO_AP1302_BUILTIN_FIRMWARE)
	size_t fw_blob_size = ap1302_fw_blob_len;

	if (fw_blob_size < sizeof(*fw_hdr)) {
		LOG_ERR("Built-in firmware blob too small: %zu", fw_blob_size);
		return -EINVAL;
	}

	fw_hdr = (const struct ap1302_firmware_header *)ap1302_fw_blob;
	fw_data = ap1302_fw_blob + sizeof(*fw_hdr);
	fw_size = fw_hdr->total_size;

	if ((sizeof(*fw_hdr) + (size_t)fw_size) > fw_blob_size) {
		LOG_ERR("Built-in firmware size %zu exceeds blob size %zu",
				sizeof(*fw_hdr) + (size_t)fw_size, fw_blob_size);
		return -EINVAL;
	}
#else
	if (cfg->firmware_partition_addr == 0U) {
		LOG_ERR("No firmware-partition provided");
		return -EINVAL;
	}

	fw_hdr = (const struct ap1302_firmware_header *)cfg->firmware_partition_addr;

	fw_data = (const uint8_t *)&fw_hdr[1];
	fw_size = fw_hdr->total_size;
#endif

	if ((fw_size == 0U) || (fw_hdr->pll_init_size > fw_size)) {
		LOG_ERR("Invalid firmware header: pll_init_size %u total_size %u",
			fw_hdr->pll_init_size, fw_size);
		return -EINVAL;
	}

	LOG_INF("Loading firmware: PLL init size %u, total size %u",
		fw_hdr->pll_init_size, fw_size);

	ret = video_write_cci_reg(&cfg->i2c, AP1302_SIP_CRC, 0xFFFF);
	if (ret < 0) {
		return ret;
	}

	/* Load PLL initialization settings */
	ret = ap1302_write_fw_window(dev, fw_data, fw_hdr->pll_init_size, &win_pos);
	if (ret < 0) {
		return ret;
	}

	/* Set bootdata stage to apply PLL settings */
	ret = video_write_cci_reg(&cfg->i2c, AP1302_BOOTDATA_STAGE, 0x0002);

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
	ret = video_write_cci_reg(&cfg->i2c, AP1302_BOOTDATA_STAGE, 0xFFFF);
	if (ret < 0) {
		return ret;
	}

	k_msleep(10);

	ret = video_read_cci_reg(&cfg->i2c, AP1302_BOOTDATA_CHECKSUM, &checksum);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int ap1302_detect_chip(const struct device *dev)
{
	const struct ap1302_config *cfg = dev->config;
	uint32_t chip_version;
	uint32_t chip_rev;
	int ret;

	ret = video_read_cci_reg(&cfg->i2c, AP1302_CHIP_VERSION, &chip_version);
	if (ret < 0) {
		LOG_ERR("Failed to read chip version: %d", ret);
		return ret;
	}

	if (chip_version != AP1302_CHIP_ID) {
		LOG_ERR("Invalid chip version: expected 0x%04x, got 0x%04x",
			AP1302_CHIP_ID, chip_version);
		return -ENODEV;
	}

	ret = video_read_cci_reg(&cfg->i2c, AP1302_CHIP_REV, &chip_rev);
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

static int ap1302_program_max_fps(const struct device *dev, uint32_t fps)
{
	const struct ap1302_config *cfg = dev->config;
	uint16_t v = AP1302_FPS_Q8_8(fps);
	int ret;

	ret = video_write_cci_reg(&cfg->i2c, AP1302_PREVIEW_MAX_FPS, v);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int ap1302_configure(const struct device *dev)
{
	const struct ap1302_config *cfg = dev->config;
	struct ap1302_data *data = dev->data;
	const struct video_format *fmt = &data->fmt;
	const struct ap1302_format_info *info = data->info;
	uint16_t hinf_ctrl;
	int ret;

	hinf_ctrl = AP1302_PREVIEW_HINF_CTRL_SPOOF |
			AP1302_PREVIEW_HINF_CTRL_MIPI_LANES(cfg->mipi_data_lanes);

	ret = video_write_cci_reg(&cfg->i2c, AP1302_PREVIEW_HINF_CTRL, hinf_ctrl);
	if (ret < 0) {
		return ret;
	}

	ret = video_write_cci_reg(&cfg->i2c, AP1302_PREVIEW_OUT_FMT, info->out_fmt);
	if (ret < 0) {
		return ret;
	}

	ret = video_write_cci_reg(&cfg->i2c, AP1302_PREVIEW_WIDTH, (uint16_t)fmt->width);
	if (ret < 0) {
		return ret;
	}

	ret = video_write_cci_reg(&cfg->i2c, AP1302_PREVIEW_HEIGHT, (uint16_t)fmt->height);
	if (ret < 0) {
		LOG_ERR("Failed to configure preview format: %d", ret);
		return ret;
	}

	return 0;
}

static int ap1302_stall(const struct device *dev, bool stall)
{
	const struct ap1302_config *cfg = dev->config;
	int ret;

	if (stall) {
		ret = video_write_cci_reg(&cfg->i2c, AP1302_SYS_START,
					 AP1302_SYS_START_PLL_LOCK |
					 AP1302_SYS_START_STALL_MODE_DISABLED);
		if (ret < 0) {
			return ret;
		}

		ret = video_write_cci_reg(&cfg->i2c, AP1302_SYS_START,
					 AP1302_SYS_START_PLL_LOCK |
					 AP1302_SYS_START_STALL_EN |
					 AP1302_SYS_START_STALL_MODE_DISABLED);
		if (ret < 0) {
			return ret;
		}

		k_msleep(200);
	} else {
		ret = video_write_cci_reg(&cfg->i2c, AP1302_SYS_START,
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
	size_t idx;
	int ret;

	ap1302_sanitize_format(fmt);

	ret = video_format_caps_index(ap1302_fmts, fmt, &idx);
	if (ret < 0) {
		LOG_ERR("Unsupported pixel format or resolution");
		return ret;
	}

	info = ap1302_find_format(fmt->pixelformat);
	if (info == NULL) {
		return -ENOTSUP;
	}

	if (!memcmp(&data->fmt, fmt, sizeof(*fmt))) {
		return 0;
	}

	ret = video_estimate_fmt_size(fmt);
	if (ret < 0) {
		return ret;
	}

	data->fmt = *fmt;
	data->info = info;

	return 0;
}

static int ap1302_get_format(const struct device *dev, struct video_format *fmt)
{
	struct ap1302_data *data = dev->data;

	*fmt = data->fmt;

	return 0;
}

static int ap1302_set_frmival(const struct device *dev, struct video_frmival *frmival)
{
	struct ap1302_data *data = dev->data;
	uint32_t fps;
	int ret;

	if ((frmival->numerator == 0U) || (frmival->denominator == 0U)) {
		return -EINVAL;
	}

	fps = DIV_ROUND_CLOSEST(frmival->denominator, frmival->numerator);
	fps = CLAMP(fps, 1U, 240U);

	data->frmival.numerator = 1U;
	data->frmival.denominator = fps;

	if (data->streaming) {
		ret = ap1302_stall(dev, true);
		if (ret < 0) {
			return ret;
		}
	}

	ret = ap1302_program_max_fps(dev, fps);
	if (ret < 0) {
		LOG_WRN("Failed to program max_fps=%u: %d", fps, ret);
	}

	if (data->streaming) {
		(void)ap1302_stall(dev, false);
	}

	LOG_INF("frmival applied: 1/%u s (%u fps)", fps, fps);

	return 0;
}

static int ap1302_get_frmival(const struct device *dev, struct video_frmival *frmival)
{
	struct ap1302_data *data = dev->data;

	*frmival = data->frmival;
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
	ARG_UNUSED(dev);

	caps->format_caps = ap1302_fmts;
	caps->min_vbuf_count = 0;

	return 0;
}

static int ap1302_init_ctrls(const struct device *dev)
{
	struct ap1302_data *data = dev->data;
	int ret;

	ret = video_init_int_menu_ctrl(&data->ctrls.linkfreq, dev, VIDEO_CID_LINK_FREQ,
				       0, ap1302_link_frequencies,
				       ARRAY_SIZE(ap1302_link_frequencies));
	if (ret < 0) {
		return ret;
	}

	data->ctrls.linkfreq.flags |= VIDEO_CTRL_FLAG_READ_ONLY;

	return 0;
}

static int ap1302_set_ctrl(const struct device *dev, unsigned int cid)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cid);

	return -ENOTSUP;
}

static DEVICE_API(video, ap1302_driver_api) = {
	.set_format = ap1302_set_format,
	.get_format = ap1302_get_format,
	.set_stream = ap1302_set_stream,
	.get_caps = ap1302_get_caps,
	.set_ctrl = ap1302_set_ctrl,
	.set_frmival = ap1302_set_frmival,
	.get_frmival = ap1302_get_frmival,
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

	ret = ap1302_init_ctrls(dev);
	if (ret < 0) {
		LOG_ERR("Failed to init ctrls: %d", ret);
		goto err_power;
	}

	data->fmt.type = VIDEO_BUF_TYPE_OUTPUT;
	data->fmt.pixelformat = VIDEO_PIX_FMT_YUYV;
	data->fmt.width = 1920;
	data->fmt.height = 1080;

	ret = video_estimate_fmt_size(&data->fmt);
	if (ret < 0) {
		LOG_ERR("Failed to estimate format size: %d", ret);
		goto err_power;
	}

	data->info = ap1302_find_format(data->fmt.pixelformat);
	data->streaming = false;
	data->frmival.numerator = 1U;
	data->frmival.denominator = 60U;

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
			(NULL))									\
	};

#define AP1302_NUM_POWER_SUPPLIES(n)								\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, power_supplies),					\
		(DT_INST_PROP_LEN(n, power_supplies)), (0))					\

#define AP1302_CONFIG_POWER_SUPPLIES(n)								\
	.power_supplies = ap1302_power_supplies_##n,						\
	.num_power_supplies = AP1302_NUM_POWER_SUPPLIES(n),

PINCTRL_DT_INST_DEFINE(0);

/* Output endpoint (AP1302 -> CSI receiver) */
#define AP1302_DT_ENDPOINT(inst) DT_INST_ENDPOINT_BY_ID(inst, 0, 0)				\

/* Number of CSI-2 data lanes derived from the endpoint's data-lanes property */
#define AP1302_DT_NUM_LANES(inst) DT_PROP_LEN(AP1302_DT_ENDPOINT(inst), data_lanes)		\


#define AP1302_INIT(n)										\
	AP1302_INIT_POWER_SUPPLIES(n)								\
	static const struct ap1302_config ap1302_config_##n = {					\
		.i2c = I2C_DT_SPEC_INST_GET(n),							\
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(n, reset_gpios, {0}),			\
		.standby_gpio = GPIO_DT_SPEC_INST_GET_OR(n, standby_gpios, {0}),		\
		.isp_en_gpio = GPIO_DT_SPEC_INST_GET_OR(n, isp_en_gpios, {0}),			\
		.firmware_partition_addr = DT_REG_ADDR(DT_INST_PHANDLE(n, firmware_partition)), \
		.firmware_partition_size = DT_REG_SIZE(DT_INST_PHANDLE(n, firmware_partition)), \
		.clock_dev = DEVICE_DT_GET_OR_NULL(DT_INST_CLOCKS_CTLR(n)),			\
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL_BY_IDX(n, 0, name), \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),					\
		AP1302_CONFIG_POWER_SUPPLIES(n)							\
		.mipi_data_lanes = AP1302_DT_NUM_LANES(n),					\
	};											\
	static struct ap1302_data ap1302_data_##n;						\
	DEVICE_DT_INST_DEFINE(n,								\
			      ap1302_init,							\
			      NULL,								\
			      &ap1302_data_##n,							\
			      &ap1302_config_##n,						\
			      POST_KERNEL,							\
			      CONFIG_VIDEO_AP1302_INIT_PRIORITY,				\
			      &ap1302_driver_api);\
	VIDEO_DEVICE_DEFINE(ap1302_##n, DEVICE_DT_INST_GET(n), NULL);

DT_INST_FOREACH_STATUS_OKAY(AP1302_INIT)

