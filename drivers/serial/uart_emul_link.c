/*
 * SPDX-FileCopyrightText: 2026 Basalte bv
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_uart_emul_link

#include <zephyr/drivers/serial/uart_emul.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(uart_emul_link, CONFIG_UART_LOG_LEVEL);

struct uart_emul_link_config {
	const struct device *uart_devs[2];
};

static void uart_emul_link_forward(const struct device *sender, size_t size, void *user_data)
{
	const struct device *receiver = user_data;
	uint8_t buf[32];
	size_t recv, sent;

	ARG_UNUSED(size);

	while (true) {
		recv = uart_emul_get_tx_data(sender, buf, sizeof(buf));
		if (recv == 0) {
			break;
		}

		sent = uart_emul_put_rx_data(receiver, buf, recv);
		if (sent < recv) {
			LOG_ERR("Dropped %zu bytes", recv - sent);
		}
	}
}

static int uart_emul_link_init(const struct device *dev)
{
	const struct uart_emul_link_config *cfg = dev->config;

	uart_emul_callback_tx_data_ready_set(cfg->uart_devs[0], uart_emul_link_forward,
					     (void *)cfg->uart_devs[1]);
	uart_emul_callback_tx_data_ready_set(cfg->uart_devs[1], uart_emul_link_forward,
					     (void *)cfg->uart_devs[0]);

	return 0;
}

#define UART_EMUL_LINK_DEFINE(inst)                                                                \
	BUILD_ASSERT(DT_INST_PROP_LEN(inst, uarts) == 2U,                                          \
		     "zephyr,uart-emul-link requires 2 UART devices");                             \
	static const struct uart_emul_link_config uart_emul_link_##inst##_cfg = {                  \
		.uart_devs =                                                                       \
			{                                                                          \
				DEVICE_DT_GET(DT_INST_PHANDLE_BY_IDX(inst, uarts, 0)),             \
				DEVICE_DT_GET(DT_INST_PHANDLE_BY_IDX(inst, uarts, 1)),             \
			},                                                                         \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, uart_emul_link_init, NULL, NULL, &uart_emul_link_##inst##_cfg, \
			      POST_KERNEL, CONFIG_SERIAL_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(UART_EMUL_LINK_DEFINE)
