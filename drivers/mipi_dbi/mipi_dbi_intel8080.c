/*
 * Copyright 2024 Kim Bøndergaard <kim@fam-boendergaard.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_mipi_dbi_intel8080

#include <zephyr/drivers/mipi_dbi.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mipi_dbi_intel8080, CONFIG_MIPI_DBI_LOG_LEVEL);

BUILD_ASSERT(CONFIG_MIPI_DBI_INTEL8080_BUS_WIDTH == 8, "Currently only 8-bit bus is supported");

struct mipi_dbi_intel8080_config {
	/* RD gpio */
	const struct gpio_dt_spec rd;
	/* WR gpio */
	const struct gpio_dt_spec wr;
	/* CS gpio */
	const struct gpio_dt_spec cs;
	/* Data gpios */
	const struct gpio_dt_spec data[CONFIG_MIPI_DBI_INTEL8080_BUS_WIDTH];
	/* Command/Data gpio */
	const struct gpio_dt_spec cmd_data;
	/* Reset GPIO */
	const struct gpio_dt_spec reset;
};

struct mipi_dbi_intel8080_data {
	struct k_spinlock lock;
};

/* Expands to 1 if the node does not have the `write-only` property */
#define _WRITE_ONLY_ABSENT(n) (!DT_INST_PROP(n, write_only)) |

/* This macro will evaluate to 1 if any of the nodes with zephyr,mipi-dbi-intel8080
 * lack a `write-only` property. The intention here is to allow the entire
 * command_read function to be optimized out when it is not needed.
 */
#define MIPI_DBI_INTEL8080_READ_REQUIRED DT_INST_FOREACH_STATUS_OKAY(_WRITE_ONLY_ABSENT) 0
uint32_t var = MIPI_DBI_INTEL8080_READ_REQUIRED;

BUILD_ASSERT((MIPI_DBI_INTEL8080_READ_REQUIRED) == 0, "Read currently not supported");

#define CTRL_PIN_ACTIVE   1
#define CTRL_PIN_INACTIVE 0

static int mipi_dbi_intel8080_chip_select(const struct mipi_dbi_intel8080_config *config,
					  bool select)
{
	gpio_pin_set_dt(&config->cs, select ? CTRL_PIN_ACTIVE : CTRL_PIN_INACTIVE);

	return 0;
}

static int mipi_dbi_intel8080_write(const struct mipi_dbi_intel8080_config *config, uint8_t val)
{
	gpio_pin_set_dt(&config->wr, CTRL_PIN_ACTIVE);

	for (unsigned int i = 0; i < CONFIG_MIPI_DBI_INTEL8080_BUS_WIDTH; i++) {
		gpio_pin_set_dt(&config->data[i], val & (1 << i));
	}

	gpio_pin_set_dt(&config->wr, CTRL_PIN_INACTIVE);

	return 0;
}

static int mipi_dbi_intel8080_write_buf(const struct mipi_dbi_intel8080_config *config,
					const uint8_t *data_buf, size_t len)
{
	int r;

	while (len) {
		r = mipi_dbi_intel8080_write(config, *data_buf);
		if (r < 0) {
			return r;
		}
		--len;
		++data_buf;
	}
	return 0;
}

static int mipi_dbi_intel8080_write_helper(const struct device *dev,
					   const struct mipi_dbi_config *dbi_config,
					   bool cmd_present, uint8_t cmd, const uint8_t *data_buf,
					   size_t len)
{
	const struct mipi_dbi_intel8080_config *config = dev->config;
	struct mipi_dbi_intel8080_data *data = dev->data;
	int r;

	LOG_DBG("CMD = %02x  %u bytes", cmd, len);
	if (len > 0) {
		LOG_HEXDUMP_DBG(data_buf, MIN(4, len), "Data");
	}

	k_spinlock_key_t spinlock_key = k_spin_lock(&data->lock);

	r = mipi_dbi_intel8080_chip_select(config, true);
	if (r < 0) {
		goto out;
	}

	if (cmd_present) {
		gpio_pin_set_dt(&config->cmd_data, 0);

		r = mipi_dbi_intel8080_write(config, cmd);
		if (r < 0) {
			goto out;
		}

		gpio_pin_set_dt(&config->cmd_data, 1);
	}

	/* send data (if any) */
	r = mipi_dbi_intel8080_write_buf(config, data_buf, len);
	if (r < 0) {
		goto out;
	}

	r = mipi_dbi_intel8080_chip_select(config, false);

out:
	k_spin_unlock(&data->lock, spinlock_key);
	return r;
}

static int mipi_dbi_intel8080_command_write(const struct device *dev,
					    const struct mipi_dbi_config *dbi_config, uint8_t cmd,
					    const uint8_t *data_buf, size_t len)
{
	return mipi_dbi_intel8080_write_helper(dev, dbi_config, true, cmd, data_buf, len);
}

static int mipi_dbi_intel8080_write_display(const struct device *dev,
					    const struct mipi_dbi_config *dbi_config,
					    const uint8_t *framebuf,
					    struct display_buffer_descriptor *desc,
					    enum display_pixel_format pixfmt)
{
	ARG_UNUSED(pixfmt);

	return mipi_dbi_intel8080_write_helper(dev, dbi_config, false, 0, framebuf, desc->buf_size);
}

#if MIPI_DBI_INTEL8080_READ_REQUIRED
#endif /* MIPI_DBI_INTEL8080_READ_REQUIRED */

static inline bool mipi_dbi_has_pin(const struct gpio_dt_spec *spec)
{
	return spec->port != NULL;
}

static int mipi_dbi_intel8080_reset(const struct device *dev, uint32_t delay)
{
	const struct mipi_dbi_intel8080_config *config = dev->config;
	int ret;

	if (!mipi_dbi_has_pin(&config->reset)) {
		return -ENOTSUP;
	}

	ret = gpio_pin_set_dt(&config->reset, 1);
	if (ret < 0) {
		return ret;
	}
	k_msleep(delay);
	return gpio_pin_set_dt(&config->reset, 0);
}

static int mipi_dbi_intel8080_init(const struct device *dev)
{
	const struct mipi_dbi_intel8080_config *config = dev->config;
	int ret;

	if (mipi_dbi_has_pin(&config->cmd_data)) {
		if (!gpio_is_ready_dt(&config->cmd_data)) {
			return -ENODEV;
		}
		ret = gpio_pin_configure_dt(&config->cmd_data, GPIO_OUTPUT);
		if (ret < 0) {
			LOG_ERR("Could not configure command/data GPIO (%d)", ret);
			return ret;
		}
	}

	if (mipi_dbi_has_pin(&config->reset)) {
		if (!gpio_is_ready_dt(&config->reset)) {
			return -ENODEV;
		}
		ret = gpio_pin_configure_dt(&config->reset, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_ERR("Could not configure reset GPIO (%d)", ret);
			return ret;
		}
	}

	if (mipi_dbi_has_pin(&config->rd)) {
		if (!gpio_is_ready_dt(&config->rd)) {
			return -ENODEV;
		}
		ret = gpio_pin_configure_dt(&config->rd, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_ERR("Could not configure RD GPIO (%d)", ret);
			return ret;
		}
	}

	if (mipi_dbi_has_pin(&config->wr)) {
		if (!gpio_is_ready_dt(&config->wr)) {
			return -ENODEV;
		}
		ret = gpio_pin_configure_dt(&config->wr, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_ERR("Could not configure WR GPIO (%d)", ret);
			return ret;
		}
	}

	if (mipi_dbi_has_pin(&config->cs)) {
		if (!gpio_is_ready_dt(&config->cs)) {
			return -ENODEV;
		}
		ret = gpio_pin_configure_dt(&config->cs, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_ERR("Could not configure CS GPIO (%d)", ret);
			return ret;
		}
	}

	for (unsigned int i = 0; i < CONFIG_MIPI_DBI_INTEL8080_BUS_WIDTH; i++) {
		if (mipi_dbi_has_pin(&config->data[i])) {
			if (!gpio_is_ready_dt(&config->data[i])) {
				return -ENODEV;
			}
			ret = gpio_pin_configure_dt(&config->data[i], GPIO_OUTPUT_ACTIVE);
			if (ret < 0) {
				LOG_ERR("Could not configure data GPIO #%u (%d)", i, ret);
				return ret;
			}
		}
	}
	return 0;
}

static struct mipi_dbi_driver_api mipi_dbi_intel8080_driver_api = {
	.reset = mipi_dbi_intel8080_reset,
	.command_write = mipi_dbi_intel8080_command_write,
	.write_display = mipi_dbi_intel8080_write_display,
#if MIPI_DBI_INTEL8080_READ_REQUIRED
	.command_read = mipi_dbi_intel8080_command_read,
#endif /* MIPI_DBI_INTEL8080_READ_REQUIRED */
};

#define MIPI_DBI_INTEL8080_INIT(n)                                                                 \
	static const struct mipi_dbi_intel8080_config mipi_dbi_intel8080_config_##n = {            \
		.rd = GPIO_DT_SPEC_INST_GET_OR(n, rd_gpios, {}),                                   \
		.wr = GPIO_DT_SPEC_INST_GET_OR(n, wr_gpios, {}),                                   \
		.cs = GPIO_DT_SPEC_INST_GET_OR(n, cs_gpios, {}),                                   \
		.data[0] = GPIO_DT_SPEC_INST_GET_BY_IDX(n, data_gpios, 0),                         \
		.data[1] = GPIO_DT_SPEC_INST_GET_BY_IDX(n, data_gpios, 1),                         \
		.data[2] = GPIO_DT_SPEC_INST_GET_BY_IDX(n, data_gpios, 2),                         \
		.data[3] = GPIO_DT_SPEC_INST_GET_BY_IDX(n, data_gpios, 3),                         \
		.data[4] = GPIO_DT_SPEC_INST_GET_BY_IDX(n, data_gpios, 4),                         \
		.data[5] = GPIO_DT_SPEC_INST_GET_BY_IDX(n, data_gpios, 5),                         \
		.data[6] = GPIO_DT_SPEC_INST_GET_BY_IDX(n, data_gpios, 6),                         \
		.data[7] = GPIO_DT_SPEC_INST_GET_BY_IDX(n, data_gpios, 7),                         \
		.cmd_data = GPIO_DT_SPEC_INST_GET(n, dc_gpios),                                    \
		.reset = GPIO_DT_SPEC_INST_GET_OR(n, reset_gpios, {}),                             \
	};                                                                                         \
	static struct mipi_dbi_intel8080_data mipi_dbi_intel8080_data_##n;                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, mipi_dbi_intel8080_init, NULL, &mipi_dbi_intel8080_data_##n,      \
			      &mipi_dbi_intel8080_config_##n, POST_KERNEL,                         \
			      CONFIG_MIPI_DBI_INIT_PRIORITY, &mipi_dbi_intel8080_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MIPI_DBI_INTEL8080_INIT)
