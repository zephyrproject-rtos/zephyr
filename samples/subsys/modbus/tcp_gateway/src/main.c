/*
 * Copyright (c) 2020 PHYTEC Messtechnik GmbH
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/modbus/modbus.h>
#include <zephyr/net/socket.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tcp_gateway, LOG_LEVEL_INF);

#define MODBUS_TCP_PORT 502

static struct modbus_adu tmp_adu;
static int backend;

const static struct modbus_iface_param backend_param = {
	.mode = MODBUS_MODE_RTU,
	.rx_timeout = 50000,
	.serial = {
		.baud = 19200,
		.parity = UART_CFG_PARITY_NONE,
	},
};

#define MODBUS_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(zephyr_modbus_serial)

static int init_backend_iface(void)
{
	const char bend_name[] = {DEVICE_DT_NAME(MODBUS_NODE)};

	backend = modbus_iface_get_by_name(bend_name);
	if (backend < 0) {
		LOG_ERR("Failed to get iface index for %s",
			bend_name);
		return -ENODEV;
	}

	return modbus_init_client(backend, backend_param);
}

static int modbus_tcp_reply(int client, struct modbus_adu *adu)
{
	uint8_t header[MODBUS_MBAP_AND_FC_LENGTH];

	modbus_raw_put_header(adu, header);
	if (send(client, header, sizeof(header), 0) < 0) {
		return -errno;
	}

	if (send(client, adu->data, adu->length, 0) < 0) {
		return -errno;
	}

	return 0;
}

static int modbus_tcp_connection(int client)
{
	uint8_t header[MODBUS_MBAP_AND_FC_LENGTH];
	int rc;
	int data_len;

	rc = recv(client, header, sizeof(header), MSG_WAITALL);
	if (rc <= 0) {
		return rc == 0 ? -ENOTCONN : -errno;
	}

	LOG_HEXDUMP_DBG(header, sizeof(header), "h:>");
	modbus_raw_get_header(&tmp_adu, header);
	data_len = tmp_adu.length;

	rc = recv(client, tmp_adu.data, data_len, MSG_WAITALL);
	if (rc <= 0) {
		return rc == 0 ? -ENOTCONN : -errno;
	}

	LOG_HEXDUMP_DBG(tmp_adu.data, tmp_adu.length, "d:>");
	rc = modbus_raw_backend_txn(backend, &tmp_adu);
	if (rc == -ENOTSUP || rc == -ENODEV) {
		LOG_WRN("Backend interface error: %d", rc);
	}

	return modbus_tcp_reply(client, &tmp_adu);
}

void main(void)
{
	int serv;
	struct sockaddr_in bind_addr;
	static int counter;

	if (init_backend_iface()) {
		LOG_ERR("Modbus initialization failed");
		return;
	}

	serv = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (serv < 0) {
		LOG_ERR("error: socket: %d", errno);
		return;
	}

	bind_addr.sin_family = AF_INET;
	bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	bind_addr.sin_port = htons(MODBUS_TCP_PORT);

	if (bind(serv, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) < 0) {
		LOG_ERR("error: bind: %d", errno);
		return;
	}

	if (listen(serv, 5) < 0) {
		LOG_ERR("error: listen: %d", errno);
		return;
	}

	LOG_INF("Started MODBUS TCP gateway example on port %d", MODBUS_TCP_PORT);

	while (1) {
		struct sockaddr_in client_addr;
		socklen_t client_addr_len = sizeof(client_addr);
		char addr_str[INET_ADDRSTRLEN];
		int client;
		int rc;

		client = accept(serv, (struct sockaddr *)&client_addr,
				&client_addr_len);

		if (client < 0) {
			LOG_ERR("error: accept: %d", errno);
			continue;
		}

		inet_ntop(client_addr.sin_family, &client_addr.sin_addr,
			  addr_str, sizeof(addr_str));
		LOG_INF("Connection #%d from %s",
			counter++, addr_str);

		do {
			rc = modbus_tcp_connection(client);
		} while (!rc);

		close(client);
		LOG_INF("Connection from %s closed, errno %d",
			addr_str, rc);
	}
}
