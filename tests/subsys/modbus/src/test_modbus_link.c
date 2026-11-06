/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Connect the emulated UARTs crosswise: what one transmits is received by
 * the other.
 */

#include <zephyr/device.h>
#include <zephyr/drivers/serial/uart_emul.h>
#include <zephyr/init.h>

static const struct device *const uart0 = DEVICE_DT_GET(DT_NODELABEL(euart0));
static const struct device *const uart1 = DEVICE_DT_GET(DT_NODELABEL(euart1));

/*
 * The emulated UART links every child node to a device, but the Modbus
 * serial nodes are only configuration for the subsystem.
 */
DEVICE_DT_DEFINE(DT_NODELABEL(modbus0), NULL, NULL, NULL, NULL, POST_KERNEL, 99, NULL);
DEVICE_DT_DEFINE(DT_NODELABEL(modbus1), NULL, NULL, NULL, NULL, POST_KERNEL, 99, NULL);

static void tx_data_ready(const struct device *dev, size_t size, void *user_data)
{
	const struct device *peer = user_data;
	uint8_t buf[64];
	uint32_t len;

	while ((len = uart_emul_get_tx_data(dev, buf, sizeof(buf))) > 0) {
		uart_emul_put_rx_data(peer, buf, len);
	}
}

static int link_init(void)
{
	uart_emul_callback_tx_data_ready_set(uart0, tx_data_ready, (void *)uart1);
	uart_emul_callback_tx_data_ready_set(uart1, tx_data_ready, (void *)uart0);

	return 0;
}

SYS_INIT(link_init, POST_KERNEL, 99);
