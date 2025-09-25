/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>

#include <zephyr/bluetooth/audio/bap.h>

#include "bap_broadcast_source.h"
#include "zephyr/fff.h"

/* List of fakes used by this unit tester */
#define FFF_FAKES_LIST(FAKE)                                                                       \
	FAKE(mock_bap_broadcast_source_started_cb)                                                 \
	FAKE(mock_bap_broadcast_source_stopped_cb)

DEFINE_FAKE_VOID_FUNC(mock_bap_broadcast_source_started_cb, struct bt_bap_broadcast_source *);

DEFINE_FAKE_VOID_FUNC(mock_bap_broadcast_source_stopped_cb, struct bt_bap_broadcast_source *,
		      uint8_t);

struct bt_bap_broadcast_source_cb mock_bap_broadcast_source_cb = {
	.started = mock_bap_broadcast_source_started_cb,
	.stopped = mock_bap_broadcast_source_stopped_cb,
};

void mock_bap_broadcast_source_init(void)
{
	FFF_FAKES_LIST(RESET_FAKE);
}
