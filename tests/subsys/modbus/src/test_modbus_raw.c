/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_modbus.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(raw_test, LOG_LEVEL_INF);

static struct modbus_adu tmp_adu;
K_SEM_DEFINE(received, 0, 1);

/*
 * Server wants to send the data back.
 * We just store them in between and pass them to the client.
 */
int server_raw_cb(const int iface, const struct modbus_adu *adu,
		void *user_data)
{
	LOG_DBG("Server raw callback from interface %d", iface);

	tmp_adu.trans_id = adu->trans_id;
	tmp_adu.proto_id = adu->proto_id;
	tmp_adu.length = adu->length;
	tmp_adu.unit_id = adu->unit_id;
	tmp_adu.fc = adu->fc;
	memcpy(tmp_adu.data, adu->data,
	       MIN(adu->length, sizeof(tmp_adu.data)));

	LOG_HEXDUMP_DBG(tmp_adu.data, tmp_adu.length, "resp");
	k_sem_give(&received);

	return 0;
}

/*
 * Client wants to send the data via whatever.
 * We just store it in between and submit to the server.
 */
int client_raw_cb(const int iface, const struct modbus_adu *adu,
		void *user_data)
{
	uint8_t server_iface = test_get_server_iface();
	uint8_t client_iface = test_get_client_iface();

	LOG_DBG("Client raw callback from interface %d", iface);

	tmp_adu.trans_id = adu->trans_id;
	tmp_adu.proto_id = adu->proto_id;
	tmp_adu.length = adu->length;
	tmp_adu.unit_id = adu->unit_id;
	tmp_adu.fc = adu->fc;
	memcpy(tmp_adu.data, adu->data,
	       MIN(adu->length, sizeof(tmp_adu.data)));

	LOG_HEXDUMP_DBG(tmp_adu.data, tmp_adu.length, "c->s");

	/*
	 * modbus_raw_submit_rx() copies the data to the internal memory
	 * so that tmp_adu can be used immediately afterwards.
	 */
	modbus_raw_submit_rx(server_iface, &tmp_adu);

	if (k_sem_take(&received, K_MSEC(1000)) != 0) {
		zassert_true(0, "MODBUS RAW wait time expired");
	}

	modbus_raw_submit_rx(client_iface, &tmp_adu);

	return 0;
}
