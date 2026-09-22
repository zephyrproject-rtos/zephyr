/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/ead.h>

#ifndef __EAD_SAMPLE_COMMON_H
#define __EAD_SAMPLE_COMMON_H

struct key_material {
	uint8_t session_key[BT_EAD_KEY_SIZE];
	uint8_t iv[BT_EAD_IV_SIZE];
} __packed;

#define CUSTOM_SERVICE_TYPE BT_UUID_128_ENCODE(0x2e2b8dc3, 0x06e0, 0x4f93, 0x9bb2, 0x734091c356f0)
#define BT_UUID_CUSTOM_SERVICE BT_UUID_DECLARE_128(CUSTOM_SERVICE_TYPE)

static inline void await_signal(struct k_poll_signal *sig)
{
	struct k_poll_event events[] = {
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, sig),
	};

	k_poll(events, ARRAY_SIZE(events), K_FOREVER);
}

#endif /* __EAD_SAMPLE_COMMON_H */
