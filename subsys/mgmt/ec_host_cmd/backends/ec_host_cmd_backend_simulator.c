/*
 * Copyright (c) 2020 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/init.h>
#include <zephyr/mgmt/ec_host_cmd/backend.h>
#include <zephyr/mgmt/ec_host_cmd/ec_host_cmd.h>
#include <string.h>

#ifndef CONFIG_ARCH_POSIX
#error Simulator only valid on posix
#endif

struct ec_host_cmd_sim_ctx {
	struct ec_host_cmd_rx_ctx *rx_ctx;
	struct ec_host_cmd_tx_buf *tx;
};

#define EC_HOST_CMD_SIM_DEFINE(_name)                                                              \
	static struct ec_host_cmd_sim_ctx _name##_hc_sim;                                          \
	struct ec_host_cmd_backend _name = {                                                       \
		.api = &ec_host_cmd_api,                                                           \
		.ctx = (struct ec_host_cmd_sim_ctx *)&_name##_hc_sim,                              \
	}

static ec_host_cmd_backend_api_send tx;

static int ec_host_cmd_sim_init(const struct ec_host_cmd_backend *backend,
				struct ec_host_cmd_rx_ctx *rx_ctx,
				struct ec_host_cmd_tx_buf *tx_buf)
{
	struct ec_host_cmd_sim_ctx *hc_sim = (struct ec_host_cmd_sim_ctx *)backend->ctx;

	hc_sim->rx_ctx = rx_ctx;
	hc_sim->tx = tx_buf;

	return 0;
}

static int ec_host_cmd_sim_send(const struct ec_host_cmd_backend *backend)
{
	if (tx != NULL) {
		return tx(backend);
	}

	return 0;
}

static const struct ec_host_cmd_backend_api ec_host_cmd_api = {
	.init = &ec_host_cmd_sim_init,
	.send = &ec_host_cmd_sim_send,
};
EC_HOST_CMD_SIM_DEFINE(ec_host_cmd_sim);

void ec_host_cmd_backend_sim_install_send_cb(ec_host_cmd_backend_api_send cb,
					     struct ec_host_cmd_tx_buf **tx_buf)
{
	struct ec_host_cmd_sim_ctx *hc_sim = (struct ec_host_cmd_sim_ctx *)ec_host_cmd_sim.ctx;
	*tx_buf = hc_sim->tx;
	tx = cb;
}

int ec_host_cmd_backend_sim_data_received(const uint8_t *buffer, size_t len)
{
	struct ec_host_cmd_sim_ctx *hc_sim = (struct ec_host_cmd_sim_ctx *)ec_host_cmd_sim.ctx;

	memcpy(hc_sim->rx_ctx->buf, buffer, len);
	hc_sim->rx_ctx->len = len;

	ec_host_cmd_rx_notify();

	return 0;
}

static int host_cmd_init(void)
{

	ec_host_cmd_init(&ec_host_cmd_sim);
	return 0;
}
SYS_INIT(host_cmd_init, POST_KERNEL, CONFIG_EC_HOST_CMD_INIT_PRIORITY);
