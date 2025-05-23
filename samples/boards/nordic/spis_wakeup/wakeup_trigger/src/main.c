/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>

#define SPI_MODE (SPI_OP_MODE_MASTER | SPI_WORD_SET(8))
static const struct spi_dt_spec spim = SPI_DT_SPEC_GET(DT_NODELABEL(spim_dt), SPI_MODE, 0);

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led), gpios);

LOG_MODULE_REGISTER(spi_wakeup);

int main(void)
{
	bool status;
	static char tx_buffer[] = CONFIG_BOARD_QUALIFIERS;
	struct spi_buf tx_spi_bufs = {.buf = tx_buffer, .len = sizeof(tx_buffer)};
	struct spi_buf_set tx_spi_buf_set = {.buffers = &tx_spi_bufs, .count = 1};

	LOG_INF("Hello world from %s", CONFIG_BOARD_TARGET);

	status = gpio_is_ready_dt(&led);
	__ASSERT(status, "Error: GPIO Device not ready");

	status = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	__ASSERT(status == 0, "Could not configure led GPIO");

	status = spi_is_ready_dt(&spim);
	__ASSERT(status, "Error: SPI device is not ready");

	while (1) {
		LOG_INF("SPIM: going to sleep for 1.5s...");
		gpio_pin_set_dt(&led, 0);
		k_msleep(1500);
		gpio_pin_set_dt(&led, 1);
		LOG_INF("SPIM: will be active for 500ms before transfer");
		k_busy_wait(500000);
		LOG_INF("SPIM: transferring the CONFIG_BOARD_QUALIFIERS: '%s'", tx_buffer);
		spi_write_dt(&spim, &tx_spi_buf_set);
	}

	return 0;
}
