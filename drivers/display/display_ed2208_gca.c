/*
 * Copyright (c) 2026 The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT eink_ed2208_gca

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/display/ed2208-gca.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mipi_dbi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>

LOG_MODULE_REGISTER(ed2208_gca, CONFIG_DISPLAY_LOG_LEVEL);

#define ED2208_GCA_RESET_DELAY_MS    20U
#define ED2208_GCA_RESET_WAIT_MS     10U
#define ED2208_GCA_BUSY_TIMEOUT_MS   5000U  /* 5 seconds for normal operations */
#define ED2208_GCA_REFRESH_TIMEOUTMS 40000U /* 40 seconds for display refresh */
#define ED2208_GCA_TX_CHUNK_SIZE     256U

#define ED2208_GCA_CMD_PSR          0x00
#define ED2208_GCA_CMD_CMDH         0xAA
#define ED2208_GCA_CMD_PWRR         0x01
#define ED2208_GCA_CMD_POF          0x02
#define ED2208_GCA_CMD_POFS         0x03
#define ED2208_GCA_CMD_PON          0x04
#define ED2208_GCA_CMD_BTST1        0x05
#define ED2208_GCA_CMD_BTST2        0x06
#define ED2208_GCA_CMD_DSLP         0x07
#define ED2208_GCA_CMD_BTST3        0x08
#define ED2208_GCA_CMD_DTM          0x10
#define ED2208_GCA_CMD_IPC          0x13
#define ED2208_GCA_CMD_PLL          0x30
#define ED2208_GCA_CMD_TSE          0x41
#define ED2208_GCA_CMD_CDI          0x50
#define ED2208_GCA_CMD_TCON         0x60
#define ED2208_GCA_CMD_TRES         0x61
#define ED2208_GCA_CMD_VDCS         0x82
#define ED2208_GCA_CMD_T_VDCS       0x84
#define ED2208_GCA_CMD_AGID         0x86
#define ED2208_GCA_CMD_DRF          0x12
#define ED2208_GCA_CMD_CCSET        0xE0
#define ED2208_GCA_CMD_PWS          0xE3
#define ED2208_GCA_CMD_TSSET        0xE6
#define ED2208_GCA_DEEP_SLEEP_CHECK 0xA5

struct ed2208_gca_config {
	const struct device *mipi_dev;
	struct mipi_dbi_config dbi_config;
	struct gpio_dt_spec busy_gpio;
	uint32_t color_palette[ED2208_GCA_COLOR_PALETTE_SIZE];
	uint16_t width;
	uint16_t height;
};

struct ed2208_gca_data {
	bool blanking_on;
	struct k_sem busy_sem;
	struct gpio_callback busy_cb;
};

static inline int ed2208_gca_write_cmd(const struct device *dev, uint8_t cmd, const uint8_t *data,
				       size_t len)
{
	const struct ed2208_gca_config *config = dev->config;

	return mipi_dbi_command_write(config->mipi_dev, &config->dbi_config, cmd, data, len);
}

static inline int ed2208_gca_write_cmd_uint8(const struct device *dev, uint8_t cmd, uint8_t data)
{
	return ed2208_gca_write_cmd(dev, cmd, &data, 1);
}

/* Run a packed init sequence of {cmd, len, bytes...} tuples. */
static int ed2208_gca_run_init_sequence(const struct device *dev, const uint8_t *seq,
					size_t seq_len)
{
	size_t i = 0;

	while (i < seq_len) {
		uint8_t cmd = seq[i++];
		uint8_t len = seq[i++];
		int ret = ed2208_gca_write_cmd(dev, cmd, len ? &seq[i] : NULL, len);

		if (ret < 0) {
			return ret;
		}
		i += len;
	}

	return 0;
}

static void ed2208_gca_busy_cb(const struct device *gpio_dev, struct gpio_callback *cb,
			       uint32_t pins)
{
	struct ed2208_gca_data *data = CONTAINER_OF(cb, struct ed2208_gca_data, busy_cb);

	ARG_UNUSED(gpio_dev);
	ARG_UNUSED(pins);

	k_sem_give(&data->busy_sem);
}

/* Wait until BUSY_N indicates the panel is idle. */
static int ed2208_gca_wait_until_ready(const struct device *dev, uint32_t timeout_ms)
{
	const struct ed2208_gca_config *config = dev->config;
	struct ed2208_gca_data *data = dev->data;
	int pin;
	int ret;

	pin = gpio_pin_get_dt(&config->busy_gpio);
	if (pin < 0) {
		LOG_ERR("Failed to read busy GPIO: %d", pin);
		return pin;
	}

	if (pin != 0) {
		return 0;
	}

	k_sem_reset(&data->busy_sem);

	ret = gpio_pin_interrupt_configure_dt(&config->busy_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure busy GPIO interrupt: %d", ret);
		return ret;
	}

	ret = k_sem_take(&data->busy_sem, K_MSEC(timeout_ms));

	gpio_pin_interrupt_configure_dt(&config->busy_gpio, GPIO_INT_DISABLE);

	if (ret < 0) {
		LOG_ERR("Timeout waiting for BUSY_N");
		return -ETIMEDOUT;
	}

	return 0;
}

static int ed2208_gca_hw_init(const struct device *dev)
{
	const struct ed2208_gca_config *config = dev->config;
	const uint8_t init_seq[] = {
		ED2208_GCA_CMD_CMDH,   6, 0x49, 0x55, 0x20, 0x08, 0x09, 0x18,
		ED2208_GCA_CMD_PWRR,   6, 0x3F, 0x00, 0x32, 0x2A, 0x0E, 0x2A,
		ED2208_GCA_CMD_PSR,    2, 0x5F, 0x69,
		ED2208_GCA_CMD_POFS,   4, 0x00, 0x54, 0x00, 0x44,
		ED2208_GCA_CMD_BTST1,  4, 0x40, 0x1F, 0x1F, 0x2C,
		ED2208_GCA_CMD_BTST2,  4, 0x6F, 0x1F, 0x16, 0x25,
		ED2208_GCA_CMD_BTST3,  4, 0x6F, 0x1F, 0x1F, 0x22,
		ED2208_GCA_CMD_IPC,    2, 0x00, 0x04,
		ED2208_GCA_CMD_PLL,    1, 0x02,
		ED2208_GCA_CMD_TSE,    1, 0x00,
		ED2208_GCA_CMD_CDI,    1, 0x3F,
		ED2208_GCA_CMD_TCON,   2, 0x02, 0x00,
		ED2208_GCA_CMD_TRES,   4, (config->width  >> 8) & 0xFF, config->width  & 0xFF,
					  (config->height >> 8) & 0xFF, config->height & 0xFF,
		ED2208_GCA_CMD_VDCS,   1, 0x1E,
		ED2208_GCA_CMD_T_VDCS, 1, 0x01,
		ED2208_GCA_CMD_AGID,   1, 0x00,
		ED2208_GCA_CMD_PWS,    1, 0x2F,
		ED2208_GCA_CMD_CCSET,  1, 0x00,
		ED2208_GCA_CMD_TSSET,  1, 0x00,
		ED2208_GCA_CMD_PON,    0,
	};
	int ret;

	ret = mipi_dbi_reset(config->mipi_dev, ED2208_GCA_RESET_DELAY_MS);
	if (ret < 0) {
		LOG_ERR("Failed to reset display: %d", ret);
		return ret;
	}

	k_msleep(ED2208_GCA_RESET_WAIT_MS);

	ret = ed2208_gca_run_init_sequence(dev, init_seq, sizeof(init_seq));
	if (ret < 0) {
		goto out;
	}

	ret = ed2208_gca_wait_until_ready(dev, ED2208_GCA_BUSY_TIMEOUT_MS);

out:
	mipi_dbi_release(config->mipi_dev, &config->dbi_config);
	return ret;
}

static int ed2208_gca_deep_sleep(const struct device *dev)
{
	return ed2208_gca_write_cmd_uint8(dev, ED2208_GCA_CMD_DSLP, ED2208_GCA_DEEP_SLEEP_CHECK);
}

static int ed2208_gca_write(const struct device *dev, const uint16_t x, const uint16_t y,
			    const struct display_buffer_descriptor *desc, const void *buf)
{
	const struct ed2208_gca_config *config = dev->config;
	struct ed2208_gca_data *data = dev->data;
	const uint8_t *src = buf;
	struct display_buffer_descriptor mipi_desc = {
		.width = ED2208_GCA_TX_CHUNK_SIZE,
		.height = 1,
		.pitch = ED2208_GCA_TX_CHUNK_SIZE,
	};
	size_t buf_len = config->width * config->height / 2U;
	size_t offset = 0U;
	int ret;

	if (x != 0U || y != 0U || desc->width != config->width || desc->height != config->height) {
		LOG_ERR("Partial updates not supported");
		return -ENOTSUP;
	}

	if (desc->pitch != desc->width) {
		LOG_ERR("Pitch must match width");
		return -EINVAL;
	}

	if (buf == NULL || desc->buf_size < buf_len) {
		LOG_ERR("Invalid buffer: %p (%zu < %zu)", buf, desc->buf_size, buf_len);
		return -EINVAL;
	}

	ret = ed2208_gca_hw_init(dev);
	if (ret < 0) {
		return ret;
	}

	ret = ed2208_gca_write_cmd(dev, ED2208_GCA_CMD_DTM, NULL, 0);
	if (ret < 0) {
		goto out;
	}

	while (offset < buf_len) {
		size_t chunk_len = MIN(ED2208_GCA_TX_CHUNK_SIZE, buf_len - offset);

		mipi_desc.buf_size = chunk_len;
		mipi_desc.width = chunk_len;
		mipi_desc.pitch = chunk_len;
		mipi_desc.frame_incomplete = (offset + chunk_len) != buf_len;

		ret = mipi_dbi_write_display(config->mipi_dev, &config->dbi_config, src + offset,
					     &mipi_desc, PIXEL_FORMAT_I_4);
		if (ret < 0) {
			goto out;
		}

		offset += chunk_len;
	}

	if (!data->blanking_on) {
		ret = ed2208_gca_write_cmd_uint8(dev, ED2208_GCA_CMD_DRF, 0x00);
		if (ret < 0) {
			goto out;
		}

		ret = ed2208_gca_wait_until_ready(dev, ED2208_GCA_REFRESH_TIMEOUTMS);
		if (ret < 0) {
			goto out;
		}

		ret = ed2208_gca_write_cmd_uint8(dev, ED2208_GCA_CMD_POF, 0x00);
		if (ret < 0) {
			goto out;
		}

		ret = ed2208_gca_wait_until_ready(dev, ED2208_GCA_BUSY_TIMEOUT_MS);
		if (ret < 0) {
			goto out;
		}

		ret = ed2208_gca_deep_sleep(dev);
	}

out:
	mipi_dbi_release(config->mipi_dev, &config->dbi_config);
	return ret;
}

static int ed2208_gca_blanking_on(const struct device *dev)
{
	struct ed2208_gca_data *data = dev->data;

	data->blanking_on = true;
	return 0;
}

static int ed2208_gca_blanking_off(const struct device *dev)
{
	struct ed2208_gca_data *data = dev->data;

	data->blanking_on = false;
	return 0;
}

static void ed2208_gca_get_capabilities(const struct device *dev, struct display_capabilities *caps)
{
	const struct ed2208_gca_config *config = dev->config;

	memset(caps, 0, sizeof(*caps));
	memcpy(caps->color_palette, config->color_palette, sizeof(config->color_palette));
	caps->x_resolution = config->width;
	caps->y_resolution = config->height;
	caps->supported_pixel_formats = PIXEL_FORMAT_I_4;
	caps->current_pixel_format = PIXEL_FORMAT_I_4;
	caps->screen_info = SCREEN_INFO_EPD;
}

static int ed2208_gca_set_pixel_format(const struct device *dev, const enum display_pixel_format pf)
{
	ARG_UNUSED(dev);

	if (pf == PIXEL_FORMAT_I_4) {
		return 0;
	}

	return -ENOTSUP;
}

static int ed2208_gca_init(const struct device *dev)
{
	const struct ed2208_gca_config *config = dev->config;
	struct ed2208_gca_data *data = dev->data;
	int ret;

	if (!device_is_ready(config->mipi_dev)) {
		LOG_ERR_DEVICE_NOT_READY(config->mipi_dev);
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&config->busy_gpio)) {
		LOG_ERR("Busy GPIO not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->busy_gpio, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure busy GPIO: %d", ret);
		return ret;
	}

	k_sem_init(&data->busy_sem, 0, 1);

	gpio_init_callback(&data->busy_cb, ed2208_gca_busy_cb, BIT(config->busy_gpio.pin));
	ret = gpio_add_callback(config->busy_gpio.port, &data->busy_cb);
	if (ret < 0) {
		LOG_ERR("Failed to add busy GPIO callback: %d", ret);
		return ret;
	}

	data->blanking_on = true;

	return ed2208_gca_hw_init(dev);
}

#ifdef CONFIG_PM_DEVICE
static int ed2208_gca_pm_action(const struct device *dev, enum pm_device_action action)
{
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = ed2208_gca_hw_init(dev);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		ret = ed2208_gca_deep_sleep(dev);
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}
#endif

static DEVICE_API(display, ed2208_gca_api) = {
	.blanking_on = ed2208_gca_blanking_on,
	.blanking_off = ed2208_gca_blanking_off,
	.write = ed2208_gca_write,
	.get_capabilities = ed2208_gca_get_capabilities,
	.set_pixel_format = ed2208_gca_set_pixel_format,
};

#define ED2208_GCA_DEFINE(inst)                                                                    \
	BUILD_ASSERT(CONFIG_DISPLAY_COLOR_PALETTE_MAX_SIZE >=                                      \
		     DT_PROP_LEN(DT_INST_CHILD(inst, color_palette), colors));                     \
	static const struct ed2208_gca_config ed2208_gca_cfg_##inst = {                            \
		.mipi_dev = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                   \
		.dbi_config =                                                                      \
			{                                                                          \
				.mode = MIPI_DBI_MODE_SPI_4WIRE,                                   \
				.config = MIPI_DBI_SPI_CONFIG_DT_INST(                             \
					inst, SPI_OP_MODE_MASTER | SPI_WORD_SET(8), 0),            \
			},                                                                         \
		.busy_gpio = GPIO_DT_SPEC_INST_GET(inst, busy_gpios),                              \
		.color_palette = DT_PROP(DT_INST_CHILD(inst, color_palette), colors),              \
		.width = DT_INST_PROP(inst, width),                                                \
		.height = DT_INST_PROP(inst, height),                                              \
	};                                                                                         \
	static struct ed2208_gca_data ed2208_gca_data_##inst;                                      \
	PM_DEVICE_DT_INST_DEFINE(inst, ed2208_gca_pm_action);                                      \
	DEVICE_DT_INST_DEFINE(inst, ed2208_gca_init, PM_DEVICE_DT_INST_GET(inst),                  \
			      &ed2208_gca_data_##inst, &ed2208_gca_cfg_##inst, POST_KERNEL,        \
			      CONFIG_DISPLAY_INIT_PRIORITY, &ed2208_gca_api);

DT_INST_FOREACH_STATUS_OKAY(ED2208_GCA_DEFINE)
