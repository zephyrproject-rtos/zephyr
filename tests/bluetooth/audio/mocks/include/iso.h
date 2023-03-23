/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MOCKS_ISO_H_
#define MOCKS_ISO_H_

#include <zephyr/fff.h>
#include <zephyr/bluetooth/iso.h>

/* List of fakes used by this unit tester */
#define ISO_FFF_FAKES_LIST(FAKE)                                                                   \
	FAKE(bt_iso_chan_send)                                                                     \
	FAKE(bt_iso_server_register)                                                               \
	FAKE(bt_iso_server_unregister)                                                             \
	FAKE(bt_iso_chan_disconnect)                                                               \

DECLARE_FAKE_VALUE_FUNC(int, bt_iso_chan_send, struct bt_iso_chan *, struct net_buf *, uint16_t,
			uint32_t);
DECLARE_FAKE_VALUE_FUNC(int, bt_iso_server_register, struct bt_iso_server *);
DECLARE_FAKE_VALUE_FUNC(int, bt_iso_server_unregister, struct bt_iso_server *);
DECLARE_FAKE_VALUE_FUNC(int, bt_iso_chan_disconnect, struct bt_iso_chan *);

#endif /* MOCKS_ISO_H_ */
