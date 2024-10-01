/*
 * MIPI DBI Type A and B driver using GPIO
 *
 * Copyright 2024 Stefan Gloor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_mipi_dbi_bitbang

#include <zephyr/drivers/mipi_dbi.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mipi_dbi_bitbang, CONFIG_MIPI_DBI_LOG_LEVEL);

/* The MIPI DBI spec allows 8, 9, and 16 bits */
#define MIPI_DBI_MAX_DATA_BUS_WIDTH 16

/* Compile in a data bus LUT for improved performance if at least one instance uses an 8-bit bus */
#define _8_BIT_MODE_PRESENT(n) (DT_INST_PROP_LEN(n, data_gpios) == 8) |
#define MIPI_DBI_8_BIT_MODE    DT_INST_FOREACH_STATUS_OKAY(_8_BIT_MODE_PRESENT) 0

struct mipi_dbi_bitbang_config {
	/* Parallel 8080/6800 data GPIOs */
	const struct gpio_dt_spec data[MIPI_DBI_MAX_DATA_BUS_WIDTH];
	const uint8_t data_bus_width;

	/* Read (type B) GPIO */
	const struct gpio_dt_spec rd;

	/* Write (type B) or Read/!Write (type A) GPIO */
	const struct gpio_dt_spec wr;

	/* Enable/strobe GPIO (type A) */
	const struct gpio_dt_spec e;

	/* Chip-select GPIO */
	const struct gpio_dt_spec cs;

	/* Command/Data GPIO */
	const struct gpio_dt_spec cmd_data;

	/* Reset GPIO */
	const struct gpio_dt_spec reset;

#if MIPI_DBI_8_BIT_MODE
	/* Data GPIO remap look-up table. Valid if mipi_dbi_bitbang_data.single_port is set */
	const uint32_t data_lut[256];

	/* Mask of all data pins. Valid if mipi_dbi_bitbang_data.single_port is set */
	const uint32_t data_mask;
#endif
};

struct mipi_dbi_bitbang_data {
	struct k_mutex lock;

#if MIPI_DBI_8_BIT_MODE
	/* Indicates whether all data GPIO pins are on the same port and the data LUT is used. */
	bool single_port;

	/* Data GPIO port device. Valid if mipi_dbi_bitbang_data.single_port is set */
	const struct device *data_port;
#endif
};

static inline void mipi_dbi_bitbang_set_data_gpios(const struct mipi_dbi_bitbang_config *config,
						   struct mipi_dbi_bitbang_data *data,
						   uint32_t value)
{
#if MIPI_DBI_8_BIT_MODE
	if (data->single_port) {
		gpio_port_set_masked(data->data_port, config->data_mask, config->data_lut[value]);
	} else {
#endif
		for (int i = 0; i < config->data_bus_width; i++) {
			gpio_pin_set_dt(&config->data[i], (value & (1 << i)) != 0);
		}
#if MIPI_DBI_8_BIT_MODE
	}
#endif
}

static int mipi_dbi_bitbang_write_helper(const struct device *dev,
					 const struct mipi_dbi_config *dbi_config, bool cmd_present,
					 uint8_t cmd, const uint8_t *data_buf, size_t len)
{
	const struct mipi_dbi_bitbang_config *config = dev->config;
	struct mipi_dbi_bitbang_data *data = dev->data;
	int ret = 0;
	uint8_t value;

	ret = k_mutex_lock(&data->lock, K_FOREVER);
	if (ret < 0) {
		return ret;
	}

	switch (dbi_config->mode) {
	case MIPI_DBI_MODE_8080_BUS_8_BIT:
	case MIPI_DBI_MODE_8080_BUS_9_BIT:
	case MIPI_DBI_MODE_8080_BUS_16_BIT:
		gpio_pin_set_dt(&config->cs, 1);
		if (cmd_present) {
			gpio_pin_set_dt(&config->wr, 0);
			gpio_pin_set_dt(&config->cmd_data, 0);
			mipi_dbi_bitbang_set_data_gpios(config, data, cmd);
			gpio_pin_set_dt(&config->wr, 1);
		}
		if (len > 0) {
			gpio_pin_set_dt(&config->cmd_data, 1);
			while (len > 0) {
				value = *(data_buf++);
				gpio_pin_set_dt(&config->wr, 0);
				mipi_dbi_bitbang_set_data_gpios(config, data, value);
				gpio_pin_set_dt(&config->wr, 1);
				len--;
			}
		}
		gpio_pin_set_dt(&config->cs, 0);
		break;

	/* Clocked E mode */
	case MIPI_DBI_MODE_6800_BUS_8_BIT:
	case MIPI_DBI_MODE_6800_BUS_9_BIT:
	case MIPI_DBI_MODE_6800_BUS_16_BIT:
		gpio_pin_set_dt(&config->cs, 1);
		gpio_pin_set_dt(&config->wr, 0);
		if (cmd_present) {
			gpio_pin_set_dt(&config->e, 1);
			gpio_pin_set_dt(&config->cmd_data, 0);
			mipi_dbi_bitbang_set_data_gpios(config, data, cmd);
			gpio_pin_set_dt(&config->e, 0);
		}
		if (len > 0) {
			gpio_pin_set_dt(&config->cmd_data, 1);
			while (len > 0) {
				value = *(data_buf++);
				gpio_pin_set_dt(&config->e, 1);
				mipi_dbi_bitbang_set_data_gpios(config, data, value);
				gpio_pin_set_dt(&config->e, 0);
				len--;
			}
		}
		gpio_pin_set_dt(&config->cs, 0);
		break;

	default:
		LOG_ERR("MIPI DBI mode %u is not supported.", dbi_config->mode);
		ret = -ENOTSUP;
	}

	k_mutex_unlock(&data->lock);
	return ret;
}

static int mipi_dbi_bitbang_command_write(const struct device *dev,
					  const struct mipi_dbi_config *dbi_config, uint8_t cmd,
					  const uint8_t *data_buf, size_t len)
{
	return mipi_dbi_bitbang_write_helper(dev, dbi_config, true, cmd, data_buf, len);
}

static int mipi_dbi_bitbang_write_display(const struct device *dev,
					  const struct mipi_dbi_config *dbi_config,
					  const uint8_t *framebuf,
					  struct display_buffer_descriptor *desc,
					  enum display_pixel_format pixfmt)
{
	ARG_UNUSED(pixfmt);

	return mipi_dbi_bitbang_write_helper(dev, dbi_config, false, 0x0, framebuf, desc->buf_size);
}

static int mipi_dbi_bitbang_reset(const struct device *dev, k_timeout_t delay)
{
	const struct mipi_dbi_bitbang_config *config = dev->config;
	int ret;

	LOG_DBG("Performing hw reset.");

	ret = gpio_pin_set_dt(&config->reset, 1);
	if (ret < 0) {
		return ret;
	}
	k_sleep(delay);
	return gpio_pin_set_dt(&config->reset, 0);
}

static int mipi_dbi_bitbang_init(const struct device *dev)
{
	const struct mipi_dbi_bitbang_config *config = dev->config;
	const char *failed_pin = NULL;
	int ret = 0;
#if MIPI_DBI_8_BIT_MODE
	struct mipi_dbi_bitbang_data *data = dev->data;
#endif

	if (gpio_is_ready_dt(&config->cmd_data)) {
		ret = gpio_pin_configure_dt(&config->cmd_data, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			failed_pin = "cmd_data";
			goto fail;
		}
		gpio_pin_set_dt(&config->cmd_data, 0);
	}
	if (gpio_is_ready_dt(&config->rd)) {
		gpio_pin_configure_dt(&config->rd, GPIO_OUTPUT_ACTIVE);
		/* Don't emit an error because this pin is unused in type A */
		gpio_pin_set_dt(&config->rd, 1);
	}
	if (gpio_is_ready_dt(&config->wr)) {
		ret = gpio_pin_configure_dt(&config->wr, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			failed_pin = "wr";
			goto fail;
		}
		gpio_pin_set_dt(&config->wr, 1);
	}
	if (gpio_is_ready_dt(&config->e)) {
		gpio_pin_configure_dt(&config->e, GPIO_OUTPUT_ACTIVE);
		/* Don't emit an error because this pin is unused in type B */
		gpio_pin_set_dt(&config->e, 0);
	}
	if (gpio_is_ready_dt(&config->cs)) {
		ret = gpio_pin_configure_dt(&config->cs, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			failed_pin = "cs";
			goto fail;
		}
		gpio_pin_set_dt(&config->cs, 0);
	}
	if (gpio_is_ready_dt(&config->reset)) {
		ret = gpio_pin_configure_dt(&config->reset, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			failed_pin = "reset";
			goto fail;
		}
		gpio_pin_set_dt(&config->reset, 0);
	}
	for (int i = 0; i < config->data_bus_width; i++) {
		if (gpio_is_ready_dt(&config->data[i])) {
			ret = gpio_pin_configure_dt(&config->data[i], GPIO_OUTPUT_ACTIVE);
			if (ret < 0) {
				failed_pin = "data";
				goto fail;
			}
			gpio_pin_set_dt(&config->data[i], 0);
		}
	}

#if MIPI_DBI_8_BIT_MODE
	/* To optimize performance, we test whether all the data pins are
	 * on the same port. If they are, we can set the whole port in one go
	 * instead of setting each pin individually.
	 * For 8-bit mode only because LUT size grows exponentially.
	 */
	if (config->data_bus_width == 8) {
		data->single_port = true;
		data->data_port = config->data[0].port;
		for (int i = 1; i < config->data_bus_width; i++) {
			if (data->data_port != config->data[i].port) {
				data->single_port = false;
			}
		}
	}
	if (data->single_port) {
		LOG_DBG("LUT optimization enabled. data_mask=0x%x", config->data_mask);
	}
#endif

	return ret;
fail:
	LOG_ERR("Failed to configure %s GPIO pin.", failed_pin);
	return ret;
}

static const struct mipi_dbi_driver_api mipi_dbi_bitbang_driver_api = {
	.reset = mipi_dbi_bitbang_reset,
	.command_write = mipi_dbi_bitbang_command_write,
	.write_display = mipi_dbi_bitbang_write_display
};

/* This macro is repeatedly called by LISTIFY() at compile-time to generate the data bus LUT */
#define LUT_GEN(i, n) (((i & (1 << 0)) ? (1 << DT_INST_GPIO_PIN_BY_IDX(n, data_gpios, 0)) : 0) |   \
		       ((i & (1 << 1)) ? (1 << DT_INST_GPIO_PIN_BY_IDX(n, data_gpios, 1)) : 0) |   \
		       ((i & (1 << 2)) ? (1 << DT_INST_GPIO_PIN_BY_IDX(n, data_gpios, 2)) : 0) |   \
		       ((i & (1 << 3)) ? (1 << DT_INST_GPIO_PIN_BY_IDX(n, data_gpios, 3)) : 0) |   \
		       ((i & (1 << 4)) ? (1 << DT_INST_GPIO_PIN_BY_IDX(n, data_gpios, 4)) : 0) |   \
		       ((i & (1 << 5)) ? (1 << DT_INST_GPIO_PIN_BY_IDX(n, data_gpios, 5)) : 0) |   \
		       ((i & (1 << 6)) ? (1 << DT_INST_GPIO_PIN_BY_IDX(n, data_gpios, 6)) : 0) |   \
		       ((i & (1 << 7)) ? (1 << DT_INST_GPIO_PIN_BY_IDX(n, data_gpios, 7)) : 0))

/* If at least one instance has an 8-bit bus, add a data look-up table to the read-only config.
 * Whether or not it is valid and actually used for a particular instance is decided at runtime
 * and stored in the instance's mipi_dbi_bitbang_data.single_port.
 */
#if MIPI_DBI_8_BIT_MODE
#define DATA_LUT_OPTIMIZATION(n)                                                                   \
		.data_lut = { LISTIFY(256, LUT_GEN, (,), n) },                                     \
		.data_mask = ((1 << DT_INST_GPIO_PIN_BY_IDX(n, data_gpios, 0)) |                   \
			      (1 << DT_INST_GPIO_PIN_BY_IDX(n, data_gpios, 1)) |                   \
			      (1 << DT_INST_GPIO_PIN_BY_IDX(n, data_gpios, 2)) |                   \
			      (1 << DT_INST_GPIO_PIN_BY_IDX(n, data_gpios, 3)) |                   \
			      (1 << DT_INST_GPIO_PIN_BY_IDX(n, data_gpios, 4)) |                   \
			      (1 << DT_INST_GPIO_PIN_BY_IDX(n, data_gpios, 5)) |                   \
			      (1 << DT_INST_GPIO_PIN_BY_IDX(n, data_gpios, 6)) |                   \
			      (1 << DT_INST_GPIO_PIN_BY_IDX(n, data_gpios, 7)))
#else
#define DATA_LUT_OPTIMIZATION(n)
#endif

#define MIPI_DBI_BITBANG_INIT(n)                                                                   \
	static const struct mipi_dbi_bitbang_config mipi_dbi_bitbang_config_##n = {                \
		.data = {GPIO_DT_SPEC_INST_GET_BY_IDX_OR(n, data_gpios, 0, {0}),                   \
			 GPIO_DT_SPEC_INST_GET_BY_IDX_OR(n, data_gpios, 1, {0}),                   \
			 GPIO_DT_SPEC_INST_GET_BY_IDX_OR(n, data_gpios, 2, {0}),                   \
			 GPIO_DT_SPEC_INST_GET_BY_IDX_OR(n, data_gpios, 3, {0}),                   \
			 GPIO_DT_SPEC_INST_GET_BY_IDX_OR(n, data_gpios, 4, {0}),                   \
			 GPIO_DT_SPEC_INST_GET_BY_IDX_OR(n, data_gpios, 5, {0}),                   \
			 GPIO_DT_SPEC_INST_GET_BY_IDX_OR(n, data_gpios, 6, {0}),                   \
			 GPIO_DT_SPEC_INST_GET_BY_IDX_OR(n, data_gpios, 7, {0}),                   \
			 GPIO_DT_SPEC_INST_GET_BY_IDX_OR(n, data_gpios, 8, {0}),                   \
			 GPIO_DT_SPEC_INST_GET_BY_IDX_OR(n, data_gpios, 9, {0}),                   \
			 GPIO_DT_SPEC_INST_GET_BY_IDX_OR(n, data_gpios, 10, {0}),                  \
			 GPIO_DT_SPEC_INST_GET_BY_IDX_OR(n, data_gpios, 11, {0}),                  \
			 GPIO_DT_SPEC_INST_GET_BY_IDX_OR(n, data_gpios, 12, {0}),                  \
			 GPIO_DT_SPEC_INST_GET_BY_IDX_OR(n, data_gpios, 13, {0}),                  \
			 GPIO_DT_SPEC_INST_GET_BY_IDX_OR(n, data_gpios, 14, {0}),                  \
			 GPIO_DT_SPEC_INST_GET_BY_IDX_OR(n, data_gpios, 15, {0})},                 \
		.data_bus_width = DT_INST_PROP_LEN(n, data_gpios),                                 \
		.rd = GPIO_DT_SPEC_INST_GET_OR(n, rd_gpios, {}),                                   \
		.wr = GPIO_DT_SPEC_INST_GET_OR(n, wr_gpios, {}),                                   \
		.e = GPIO_DT_SPEC_INST_GET_OR(n, e_gpios, {}),                                     \
		.cs = GPIO_DT_SPEC_INST_GET_OR(n, cs_gpios, {}),                                   \
		.cmd_data = GPIO_DT_SPEC_INST_GET_OR(n, dc_gpios, {}),                             \
		.reset = GPIO_DT_SPEC_INST_GET_OR(n, reset_gpios, {}),                             \
		DATA_LUT_OPTIMIZATION(n)                                                           \
	};                                                                                         \
	BUILD_ASSERT(DT_INST_PROP_LEN(n, data_gpios) < MIPI_DBI_MAX_DATA_BUS_WIDTH,                \
		     "Number of data GPIOs in DT exceeds MIPI_DBI_MAX_DATA_BUS_WIDTH");            \
	static struct mipi_dbi_bitbang_data mipi_dbi_bitbang_data_##n;                             \
	DEVICE_DT_INST_DEFINE(n, mipi_dbi_bitbang_init, NULL, &mipi_dbi_bitbang_data_##n,          \
			      &mipi_dbi_bitbang_config_##n, POST_KERNEL,                           \
			      CONFIG_MIPI_DBI_INIT_PRIORITY, &mipi_dbi_bitbang_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MIPI_DBI_BITBANG_INIT)
