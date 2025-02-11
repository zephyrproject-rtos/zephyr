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
#define FFF_FAKES_LIST(FAKE) FAKE(bt_iso_chan_get_tx_sync)

static struct bt_iso_server *iso_server;

DEFINE_FAKE_VALUE_FUNC(int, bt_iso_chan_get_tx_sync, const struct bt_iso_chan *,
		       struct bt_iso_tx_info *);

int bt_iso_chan_send(struct bt_iso_chan *chan, struct net_buf *buf, uint16_t seq_num)
{
	if (chan->ops != NULL && chan->ops->sent != NULL) {
		chan->ops->sent(chan);
	}

	return 0;
}

int bt_iso_chan_send_ts(struct bt_iso_chan *chan, struct net_buf *buf, uint16_t seq_num,
			uint32_t ts)
{
	if (chan->ops != NULL && chan->ops->sent != NULL) {
		chan->ops->sent(chan);
	}

	return 0;
}

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

void mock_bt_iso_connected(struct bt_conn *iso)
{
	struct bt_iso_chan *chan = iso->chan;

	chan->state = BT_ISO_STATE_CONNECTED;
	chan->iso = iso;

	chan->ops->connected(chan);
}

int mock_bt_iso_accept(struct bt_conn *conn, uint8_t cig_id, uint8_t cis_id,
		       struct bt_iso_chan **chan)
{
	struct bt_iso_accept_info info = {
		.acl = conn,
		.cig_id = cig_id,
		.cis_id = cis_id,
	};
	struct bt_conn *iso;
	int err;

	zassert_not_null(iso_server, "iso_server is NULL");

	err = iso_server->accept(&info, chan);
	if (err != 0) {
		return err;
	}

	zassert_not_null(*chan, "chan is NULL");

	iso = malloc(sizeof(struct bt_conn));
	zassert_not_null(iso);

	iso->chan = (*chan);
	mock_bt_iso_connected(iso);

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

#if defined(CONFIG_BT_BAP_BROADCAST_SOURCE)
int bt_iso_big_create(struct bt_le_ext_adv *padv, struct bt_iso_big_create_param *param,
		      struct bt_iso_big **out_big)
{
	struct bt_iso_big *big;

	zassert_not_null(out_big);
	zassert_not_null(param);
	zassert_not_equal(param->num_bis, 0);

	big = malloc(sizeof(struct bt_iso_big));
	zassert_not_null(big);
	big->num_bis = 0U;

	for (uint8_t i = 0U; i < param->num_bis; i++) {
		struct bt_iso_chan *bis = param->bis_channels[i];
		struct bt_conn *iso;

		zassert_not_null(bis);

		iso = malloc(sizeof(struct bt_conn));
		zassert_not_null(iso);
		big->bis[i] = bis;
		big->num_bis++;

		iso->chan = bis;
		mock_bt_iso_connected(iso);
	}

	*out_big = big;

	return 0;
}

int bt_iso_big_terminate(struct bt_iso_big *big)
{
	/* TODO: Call chan->ops->disconnected(*chan); for each BIS */
	zassert_not_equal(big->num_bis, 0);

	for (uint8_t i = 0U; i < big->num_bis; i++) {
		struct bt_iso_chan *bis = big->bis[i];

		zassert_not_null(bis, "big %p", big);

		mock_bt_iso_disconnected(bis, BT_HCI_ERR_LOCALHOST_TERM_CONN);
	}

	free(big);

	return 0;
}
#endif /* CONFIG_BT_BAP_BROADCAST_SOURCE */
