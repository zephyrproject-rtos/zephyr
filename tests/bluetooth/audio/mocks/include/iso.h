/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MOCKS_ISO_H_
#define MOCKS_ISO_H_

#include <zephyr/fff.h>
#include <zephyr/bluetooth/iso.h>

void mock_bt_iso_init(void);
void mock_bt_iso_cleanup(void);
int mock_bt_iso_accept(struct bt_conn *conn, uint8_t cig_id, uint8_t cis_id,
		       struct bt_iso_chan **chan);
int mock_bt_iso_disconnected(struct bt_iso_chan *chan, uint8_t err);

DECLARE_FAKE_VALUE_FUNC(int, bt_iso_chan_send, struct bt_iso_chan *, struct net_buf *, uint16_t,
			uint32_t);
DECLARE_FAKE_VALUE_FUNC(int, bt_iso_chan_get_tx_sync, const struct bt_iso_chan *,
			struct bt_iso_tx_info *);

#endif /* MOCKS_ISO_H_ */
