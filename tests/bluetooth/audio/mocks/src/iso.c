/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/bluetooth/iso.h>

#include "conn.h"
#include "iso.h"

/* List of fakes used by this unit tester */
#define FFF_FAKES_LIST(FAKE)                                                                       \
	FAKE(bt_iso_chan_send)                                                                     \
	FAKE(bt_iso_chan_get_tx_sync)

static struct bt_iso_server *iso_server;

DEFINE_FAKE_VALUE_FUNC(int, bt_iso_chan_send, struct bt_iso_chan *, struct net_buf *, uint16_t,
		       uint32_t);

DEFINE_FAKE_VALUE_FUNC(int, bt_iso_chan_get_tx_sync, const struct bt_iso_chan *,
		       struct bt_iso_tx_info *);

int bt_iso_server_register(struct bt_iso_server *server)
{
	zassert_not_null(server, "server is NULL");
	zassert_not_null(server->accept, "server->accept is NULL");
	zassert_is_null(iso_server, "iso_server is NULL");

	iso_server = server;

	return 0;
}

int bt_iso_server_unregister(struct bt_iso_server *server)
{
	zassert_not_null(server, "server is NULL");
	zassert_equal_ptr(iso_server, server, "not registered");

	iso_server = NULL;

	return 0;
}

int bt_iso_chan_disconnect(struct bt_iso_chan *chan)
{
	return mock_bt_iso_disconnected(chan, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
}

void mock_bt_iso_init(void)
{
	FFF_FAKES_LIST(RESET_FAKE);
}

void mock_bt_iso_cleanup(void)
{

}

int mock_bt_iso_accept(struct bt_conn *conn, uint8_t cig_id, uint8_t cis_id,
		       struct bt_iso_chan **chan)
{
	struct bt_iso_accept_info info = {
		.acl = conn,
		.cig_id = cig_id,
		.cis_id = cis_id,
	};
	int err;

	zassert_not_null(iso_server, "iso_server is NULL");

	err = iso_server->accept(&info, chan);
	if (err != 0) {
		return err;
	}

	zassert_not_null(*chan, "chan is NULL");

	(*chan)->state = BT_ISO_STATE_CONNECTED;
	(*chan)->iso = malloc(sizeof(struct bt_conn));
	zassert_not_null((*chan)->iso);

	(*chan)->ops->connected(*chan);

	return 0;
}

int mock_bt_iso_disconnected(struct bt_iso_chan *chan, uint8_t err)
{
	chan->state = BT_ISO_STATE_DISCONNECTED;
	chan->ops->disconnected(chan, err);
	free(chan->iso);
	chan->iso = NULL;

	return 0;
}
