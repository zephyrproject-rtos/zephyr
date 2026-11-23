/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>

static const struct device *const lp_uart = DEVICE_DT_GET(DT_NODELABEL(lp_uart));

int main(void)
{
	unsigned char c;

	if (!device_is_ready(lp_uart)) {
		return -ENODEV;
	}

	while (true) {
		if (uart_poll_in(lp_uart, &c) == 0) {
			uart_poll_out(lp_uart, c);
		}
	}

	return 0;
}
