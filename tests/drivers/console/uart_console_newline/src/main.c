/*
 * Copyright (c) 2026 Ziyou Wang
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/serial/uart_emul.h>
#include <zephyr/posix/unistd.h>

#define STDOUT_FD 1

static const struct device *const console_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
static const struct device *const report_dev = DEVICE_DT_GET(DT_NODELABEL(uart0));

static const uint8_t expected[] = {'A', '\r', '\n'};

static void report(const char *s)
{
	while (*s != '\0') {
		uart_poll_out(report_dev, (uint8_t)*s++);
	}
}

int main(void)
{
	uint8_t buf[ARRAY_SIZE(expected)];

	if (!device_is_ready(console_dev) || !device_is_ready(report_dev)) {
		report("CONSOLE-NEWLINE-FAIL\n");
		return 0;
	}

	(void)uart_emul_flush_tx_data(console_dev);

	/*
	 * The libc bulk write path must hand the LF to console_out() untouched,
	 * and console_out() adds the CR. The wire therefore carries exactly one
	 * CR LF, not CR CR LF.
	 */
	if (write(STDOUT_FD, "A\n", 2) != 2) {
		report("CONSOLE-NEWLINE-FAIL\n");
		return 0;
	}

	if (uart_emul_get_tx_data(console_dev, buf, sizeof(buf)) != sizeof(buf)) {
		report("CONSOLE-NEWLINE-FAIL\n");
		return 0;
	}

	for (size_t i = 0; i < ARRAY_SIZE(expected); i++) {
		if (buf[i] != expected[i]) {
			report("CONSOLE-NEWLINE-FAIL\n");
			return 0;
		}
	}

	report("CONSOLE-NEWLINE-PASS\n");
	return 0;
}
