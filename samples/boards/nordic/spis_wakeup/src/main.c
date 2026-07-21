/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>

#define BUF_SIZE 32
static const struct device *spis_dev = DEVICE_DT_GET(DT_ALIAS(spis));
static const struct spi_config spis_config = {.operation = SPI_OP_MODE_SLAVE | SPI_WORD_SET(8)};

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led), gpios);

LOG_MODULE_REGISTER(spi_wakeup);

int main(void)
{
	bool status;
	static char rx_buffer[BUF_SIZE];
	struct spi_buf rx_spi_bufs = {.buf = rx_buffer, .len = sizeof(rx_buffer)};
	struct spi_buf_set rx_spi_buf_set = {.buffers = &rx_spi_bufs, .count = 1};

	LOG_INF("Hello world from %s", CONFIG_BOARD_TARGET);

	status = gpio_is_ready_dt(&led);
	__ASSERT(status, "Error: GPIO Device not ready");

	status = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	__ASSERT(status == 0, "Could not configure led GPIO");

	status = device_is_ready(spis_dev);
	__ASSERT(status, "Error: SPI device is not ready");

	while (1) {
		memset(rx_buffer, 0x00, sizeof(rx_buffer));
		LOG_INF("SPIS: waiting for SPI transfer; going to sleep...");
		gpio_pin_set_dt(&led, 0);
		spi_read(spis_dev, &spis_config, &rx_spi_buf_set);
		gpio_pin_set_dt(&led, 1);
		LOG_INF("SPIS: woken up by %s", rx_buffer);
		LOG_INF("SPIS: will be active for 500ms after transfer");
		k_busy_wait(500000);
	}

	return 0;
}
