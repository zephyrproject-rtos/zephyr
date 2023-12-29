/*
 * Copyright (c) 2024 Kim Bøndergaard <kim@fam-boendergaard.dk>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "display_ili9xxx.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(display_ili9xxx, CONFIG_DISPLAY_LOG_LEVEL);

#define CTRL_PIN_ACTIVE 1
#define CTRL_PIN_INACTIVE 0

static int ili9xxx_chip_select(const struct ili9xxx_config *config, bool select)
{
	const struct ili9xxx_parallel_bus *bus = &config->parallel_bus;

	gpio_pin_set_dt(&bus->cs, select ? CTRL_PIN_ACTIVE : CTRL_PIN_INACTIVE);

	return 0;
}

static int ili9xxx_write(const struct ili9xxx_config *config, uint8_t val)
{
	const struct ili9xxx_parallel_bus *bus = &config->parallel_bus;

	gpio_pin_set_dt(&bus->wr, CTRL_PIN_ACTIVE);

	for (unsigned int i = 0; i < ILI9XXX_DATA_WIDTH; i++) {
		gpio_pin_set_dt(&bus->data[i], val & (1 << i));
	}

	gpio_pin_set_dt(&bus->wr, CTRL_PIN_INACTIVE);

	return 0;
}

static int ili9xxx_write_data(const struct ili9xxx_config *config, const uint8_t *tx_data,
			      size_t tx_len)
{
	int r;

	while (tx_len) {
		r = ili9xxx_write(config, *tx_data);
		if (r < 0) {
			return r;
		}
		--tx_len;
		++tx_data;
	}
	return 0;
}

int ili9xxx_transmit(const struct device *dev, uint8_t cmd, const void *tx_data, size_t tx_len)
{
	const struct ili9xxx_config *config = dev->config;
	int r;

	LOG_DBG("CMD = %02x  %u bytes", cmd, tx_len);
	if (tx_len > 0) {
		LOG_HEXDUMP_DBG(tx_data, MIN(4, tx_len), "Data");
	}

	r = ili9xxx_chip_select(config, true);
	if (r < 0) {
		return r;
	}

	/* send command */
	gpio_pin_set_dt(&config->cmd_data, ILI9XXX_CMD);

	r = ili9xxx_write(config, cmd);
	if (r < 0) {
		return r;
	}

	gpio_pin_set_dt(&config->cmd_data, ILI9XXX_DATA);

	/* send data (if any) */
	r = ili9xxx_write_data(config, (uint8_t *)tx_data, tx_len);
	if (r < 0) {
		return r;
	}

	r = ili9xxx_chip_select(config, false);

	return r;
}

int ili9xxx_transmit_data(const struct device *dev, const void *tx_data, size_t tx_len)
{
	const struct ili9xxx_config *config = dev->config;
	int r;

	r = ili9xxx_chip_select(config, true);
	if (r < 0) {
		return r;
	}

	/* send data */
	r = ili9xxx_write_data(config, (uint8_t *)tx_data, tx_len);
	if (r < 0) {
		return r;
	}

	r = ili9xxx_chip_select(config, false);

	return r;
}

int ili9xxx_bus_init(const struct ili9xxx_config *config)
{
	const struct ili9xxx_parallel_bus *bus = &config->parallel_bus;
	int r;

	if (bus->rd.port != NULL) {
		if (!gpio_is_ready_dt(&bus->rd)) {
			LOG_ERR("rd GPIO not ready");
			return false;
		}

		r = gpio_pin_configure_dt(&bus->rd, GPIO_OUTPUT_INACTIVE);
		if (r < 0) {
			LOG_ERR("Could not configure rd GPIO (%d)", r);
			return false;
		}
	} else {
		LOG_DBG("rd gpio not configured. Please keep high");
	}

	if (!gpio_is_ready_dt(&bus->wr)) {
		LOG_ERR("wr GPIO not ready");
		return false;
	}

	r = gpio_pin_configure_dt(&bus->wr, GPIO_OUTPUT_INACTIVE);
	if (r < 0) {
		LOG_ERR("Could not configure wr GPIO (%d)", r);
		return false;
	}

	if (!gpio_is_ready_dt(&bus->cs)) {
		LOG_ERR("cs GPIO not ready");
		return false;
	}

	r = gpio_pin_configure_dt(&bus->cs, GPIO_OUTPUT_INACTIVE);
	if (r < 0) {
		LOG_ERR("Could not configure cs GPIO (%d)", r);
		return false;
	}

	for (unsigned int i = 0; i < ILI9XXX_DATA_WIDTH; i++) {
		if (!gpio_is_ready_dt(&bus->data[i])) {
			LOG_ERR("data GPIO #%d not ready", i);
			return false;
		}

		r = gpio_pin_configure_dt(&bus->data[i], GPIO_OUTPUT_ACTIVE);
		if (r < 0) {
			LOG_ERR("Could not configure data GPIO #%d (%d)", i, r);
			return false;
		}
	}

	return true;
}
