/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/iso.h>

#include "iso.h"

DEFINE_FAKE_VALUE_FUNC(int, bt_iso_chan_send, struct bt_iso_chan *, struct net_buf *, uint16_t,
		       uint32_t);
DEFINE_FAKE_VALUE_FUNC(int, bt_iso_server_register, struct bt_iso_server *);
DEFINE_FAKE_VALUE_FUNC(int, bt_iso_server_unregister, struct bt_iso_server *);
DEFINE_FAKE_VALUE_FUNC(int, bt_iso_chan_disconnect, struct bt_iso_chan *);
