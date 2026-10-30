/*
 * SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
 * SPDX-License-Identifier: Apache-2.0
 */

/* Attachment of the console to the emulated UART whose output it shows. */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/display/display_console.h>
#include <zephyr/drivers/serial/uart_emul.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(display_console, CONFIG_DISPLAY_CONSOLE_LOG_LEVEL);

#if !DT_HAS_CHOSEN(zephyr_console)
#error "The display console needs the zephyr,console chosen node"
#endif

BUILD_ASSERT(DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_console), zephyr_uart_emul),
	     "The display console attaches to a zephyr,uart-emul node");

/* Move the data written to the UART onto the console. */
static void display_console_uart_drain(const struct device *uart, size_t size, void *user_data)
{
	uint8_t buf[32];
	uint32_t n;

	ARG_UNUSED(size);
	ARG_UNUSED(user_data);

	while ((n = uart_emul_get_tx_data(uart, buf, sizeof(buf))) > 0U) {
		display_console_write(buf, n);
	}
}

/*
 * Runs after the display driver, long after the UART is in use. The output
 * written until then waits in the TX FIFO of the UART.
 */
static int display_console_uart_attach(void)
{
	const struct device *uart = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
	int err;

	if (!device_is_ready(uart)) {
		LOG_ERR_DEVICE_NOT_READY(uart);
		return -ENODEV;
	}

	err = display_console_init();
	if (err != 0) {
		LOG_ERR("display console unavailable (%d), %s output is lost", err, uart->name);
		return err;
	}

	uart_emul_callback_tx_data_ready_set(uart, display_console_uart_drain, NULL);
	display_console_uart_drain(uart, 0, NULL);

	return 0;
}

SYS_INIT(display_console_uart_attach, POST_KERNEL, CONFIG_DISPLAY_CONSOLE_INIT_PRIORITY);
