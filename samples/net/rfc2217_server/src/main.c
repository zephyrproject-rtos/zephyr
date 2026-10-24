/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/rfc2217.h>

#include "net_sample_common.h"

LOG_MODULE_REGISTER(rfc2217_server_sample, LOG_LEVEL_INF);

#if DT_HAS_COMPAT_STATUS_OKAY(zephyr_rfc2217_server)
/* Defined in the devicetree, started at boot */
#define SERVER_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(zephyr_rfc2217_server)
#define UART_NODE   DT_PHANDLE(SERVER_NODE, uart)
#define PORT        DT_PROP(SERVER_NODE, port)

static struct rfc2217_server *const server = RFC2217_SERVER_DT_GET(SERVER_NODE);
#else
/* Defined and started by the application */
#define UART_NODE DT_ALIAS(rfc2217_uart)
#define PORT      CONFIG_SAMPLE_RFC2217_PORT

RFC2217_SERVER_DEFINE(app_server);
static struct rfc2217_server *const server = &app_server;
#endif

/* Where an application would hold the TX line low, or drive the reset and
 * boot pins of the board on the other side of the UART.
 */
static int set_break(struct rfc2217_server *srv, bool on)
{
	LOG_INF("BREAK %s", on ? "on" : "off");

	return 0;
}

static int set_line(struct rfc2217_server *srv, uint32_t line, bool on)
{
	LOG_INF("%s %s", line == UART_LINE_CTRL_DTR ? "DTR" : "RTS", on ? "on" : "off");

	return 0;
}

static const struct rfc2217_server_callbacks callbacks = {
	.set_break = set_break,
	.set_line = set_line,
};

int main(void)
{
	rfc2217_server_set_callbacks(server, &callbacks, NULL);

#if !DT_HAS_COMPAT_STATUS_OKAY(zephyr_rfc2217_server)
	const struct rfc2217_server_config config = {
		.uart = DEVICE_DT_GET(UART_NODE),
		.port = PORT,
	};
	int ret = rfc2217_server_start(server, &config);

	if (ret < 0) {
		LOG_ERR("Cannot start the server (%d)", ret);
		return 0;
	}
#endif

	wait_for_network();

	LOG_INF("Serving %s on TCP port %d", DEVICE_DT_NAME(UART_NODE), PORT);

	return 0;
}
