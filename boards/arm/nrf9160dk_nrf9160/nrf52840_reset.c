/*
 * Copyright (c) 2019 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/gpio.h>
#include <drivers/uart.h>
#include <device.h>
#include <devicetree.h>

#define RESET_NODE DT_NODELABEL(nrf52840_reset)

#if DT_NODE_HAS_STATUS(RESET_NODE, okay)

#define RESET_GPIO_CTRL  DT_GPIO_CTLR(RESET_NODE, gpios)
#define RESET_GPIO_PIN   DT_GPIO_PIN(RESET_NODE, gpios)
#define RESET_GPIO_FLAGS DT_GPIO_FLAGS(RESET_NODE, gpios)

int bt_hci_transport_setup(const struct device *h4)
{
	int err;
	char c;
	const struct device *port = DEVICE_DT_GET(RESET_GPIO_CTRL);

	if (!device_is_ready(port)) {
		return -EIO;
	}

	/* Configure pin as output and initialize it to inactive state. */
	err = gpio_pin_configure(port, RESET_GPIO_PIN,
				 RESET_GPIO_FLAGS | GPIO_OUTPUT_INACTIVE);
	if (err) {
		return err;
	}

	/* Reset the nRF52840 and let it wait until the pin is inactive again
	 * before running to main to ensure that it won't send any data until
	 * the H4 device is setup and ready to receive.
	 */
	err = gpio_pin_set(port, RESET_GPIO_PIN, 1);
	if (err) {
		return err;
	}

	/* Wait for the nRF52840 peripheral to stop sending data.
	 *
	 * It is critical (!) to wait here, so that all bytes
	 * on the lines are received and drained correctly.
	 */
	k_sleep(K_MSEC(10));

	/* Drain bytes */
	while (uart_fifo_read(h4, &c, 1)) {
		continue;
	}

	/* We are ready, let the nRF52840 run to main */
	err = gpio_pin_set(port, RESET_GPIO_PIN, 0);
	if (err) {
		return err;
	}

	return 0;
}

#endif /* DT_NODE_HAS_STATUS(RESET_NODE, okay) */
