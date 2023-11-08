/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MOCKS_HAS_CLIENT_H_
#define MOCKS_HAS_CLIENT_H_

#include <zephyr/fff.h>
#include <zephyr/bluetooth/audio/has.h>

/* List of fakes used by this unit tester */
#define HAS_CLIENT_FFF_FAKES_LIST(FAKE)                                                            \
	FAKE(bt_has_client_init)                                                                   \
	FAKE(bt_has_client_bind)                                                                   \
	FAKE(bt_has_client_unbind)                                                                 \
	FAKE(bt_has_client_cmd_presets_read)                                                       \
	FAKE(bt_has_client_cmd_preset_set)                                                         \
	FAKE(bt_has_client_cmd_preset_next)                                                        \
	FAKE(bt_has_client_cmd_preset_prev)                                                        \
	FAKE(bt_has_client_cmd_preset_write)                                                       \
	FAKE(bt_has_client_info_get)                                                               \
	FAKE(bt_has_client_conn_get)                                                               \

struct bt_has_client {

};

DECLARE_FAKE_VALUE_FUNC(int, bt_has_client_init, const struct bt_has_client_cb *);
DECLARE_FAKE_VALUE_FUNC(int, bt_has_client_bind, struct bt_conn *, struct bt_has_client **);
DECLARE_FAKE_VALUE_FUNC(int, bt_has_client_unbind, struct bt_has_client *);
DECLARE_FAKE_VALUE_FUNC(int, bt_has_client_cmd_presets_read, struct bt_has_client *, uint8_t,
			uint8_t);
DECLARE_FAKE_VALUE_FUNC(int, bt_has_client_cmd_preset_set, struct bt_has_client *, uint8_t, bool);
DECLARE_FAKE_VALUE_FUNC(int, bt_has_client_cmd_preset_next, struct bt_has_client *, bool);
DECLARE_FAKE_VALUE_FUNC(int, bt_has_client_cmd_preset_prev, struct bt_has_client *, bool);
DECLARE_FAKE_VALUE_FUNC(int, bt_has_client_cmd_preset_write, struct bt_has_client *, uint8_t,
			const char *);
DECLARE_FAKE_VALUE_FUNC(int, bt_has_client_info_get, const struct bt_has_client *,
			struct bt_has_client_info *);
DECLARE_FAKE_VALUE_FUNC(int, bt_has_client_conn_get, const struct bt_has_client *,
			struct bt_conn **);

#endif /* MOCKS_HAS_CLIENT_H_ */
