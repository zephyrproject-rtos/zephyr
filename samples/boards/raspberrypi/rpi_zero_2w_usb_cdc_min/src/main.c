/*
 * Copyright (c) 2026 jetpax
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Minimal CDC ACM demo for rpi_zero_2w.
 *
 * The USB stack initializes itself at SYS_INIT (we set
 * CONFIG_CDC_ACM_SERIAL_INITIALIZE_AT_BOOT=y) and the
 * `zephyr,cdc-acm-uart` DT node is bound by the class driver to look
 * like a regular UART. The app waits for the host to open the port
 * (DTR asserted), then pushes a counted "hello USB" line every 500 ms
 * via the standard UART poll-out API. The CDC class translates that
 * into bulk-IN packets on the chip's endpoint 1 IN; the host PC sees
 * them on its /dev/cu.usbmodem* (macOS) / /dev/ttyACM0 (Linux) /
 * COMn (Windows).
 */

#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(usb_cdc_min, LOG_LEVEL_INF);

static const struct device *const cdc_dev =
	DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart);

int main(void)
{
	uint32_t dtr = 0U;

	if (!device_is_ready(cdc_dev)) {
		LOG_ERR("CDC ACM device not ready -- driver bind failed");
		return -1;
	}

	LOG_INF("CDC ACM device ready: %s -- waiting for host (DTR)",
		cdc_dev->name);

	/* Wait until a host opens the port (asserts DTR) before writing,
	 * and re-check each iteration so the sample stays quiet when the
	 * host closes the port instead of spinning data into a suspended
	 * bus. Mirrors samples/subsys/usb/cdc_acm.
	 */
	for (uint32_t count = 0;; count++) {
		char line[32];
		int len;

		uart_line_ctrl_get(cdc_dev, UART_LINE_CTRL_DTR, &dtr);
		if (!dtr) {
			k_msleep(100);
			continue;
		}

		len = snprintf(line, sizeof(line), "hello USB #%u\r\n", count);
		for (int i = 0; i < len; i++) {
			uart_poll_out(cdc_dev, line[i]);
		}

		k_msleep(500);
	}

	return 0;
}
