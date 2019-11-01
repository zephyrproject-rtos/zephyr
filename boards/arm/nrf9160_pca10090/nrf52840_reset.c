/*
 * Copyright (c) 2019 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/gpio.h>
#include <drivers/uart.h>
#include <device.h>

#define RESET_PIN CONFIG_BOARD_NRF52840_GPIO_RESET_PIN

/* Must be a pin from 17 to 23.
 * Only those can be connected to the nRF52840.
 */
BUILD_ASSERT_MSG(RESET_PIN > 16 && RESET_PIN < 24,
		 "Selected pin is not connected to nRF52840");

int bt_hci_transport_setup(struct device *h4)
{
	int err;
	char c;
	struct device *port;

	port = device_get_binding(DT_GPIO_P0_DEV_NAME);
	if (!port) {
		return -EIO;
	}

	/* Pull the pin low before configuring it as output, to ensure that
	 * it is driven to the correct level as soon as it is configured.
	 */
	err = gpio_pin_write(port, RESET_PIN, 0);
	if (err) {
		return err;
	}

	err = gpio_pin_configure(port, RESET_PIN, GPIO_DIR_OUT);
	if (err) {
		return err;
	}

	/* Reset the nRF52840 and let it wait until the pin is
	 * pulled low again before running to main to ensure
	 * that it won't send any data until the H4 device
	 * is setup and ready to receive.
	 */
	err = gpio_pin_write(port, RESET_PIN, 1);
	if (err) {
		return err;
	}

	/* Wait for the nRF52840 peripheral to stop sending data.
	 *
	 * It is critical (!) to wait here, so that all bytes
	 * on the lines are received and drained correctly.
	 */
	k_sleep(K_MSEC(1));

	/* Drain bytes */
	while (uart_fifo_read(h4, &c, 1)) {
		continue;
	}

	/* We are ready, let the nRF52840 run to main */
	err = gpio_pin_write(port, RESET_PIN, 0);
	if (err) {
		return err;
	}

	return 0;
}
