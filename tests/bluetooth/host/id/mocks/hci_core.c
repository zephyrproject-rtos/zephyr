/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/hci_core.h"

#include <zephyr/bluetooth/hci.h>
#include <zephyr/kernel.h>

#include <host/hci_core.h>

struct bt_dev bt_dev = {
	.manufacturer = 0x1234,
};

DEFINE_FAKE_VALUE_FUNC(int, bt_unpair, uint8_t, const bt_addr_le_t *);
DEFINE_FAKE_VALUE_FUNC(struct net_buf *, bt_hci_cmd_alloc, k_timeout_t);
DEFINE_FAKE_VALUE_FUNC(int, bt_hci_cmd_send_sync, uint16_t, struct net_buf *, struct net_buf **);

/*
 * Production code: bt_hci_cmd_send_sync(op, buf, rsp) calls
 *                  bt_hci_cmd_send_sync_by_dev(&bt_devs[0], op, buf, rsp).
 *
 * In tests, id.c calls _by_dev directly.  We provide a real (non-fake)
 * implementation that delegates to the bt_hci_cmd_send_sync fake so
 * existing test expectations keep working unchanged.
 */
int bt_hci_cmd_send_sync_by_dev(struct bt_dev *hdev, uint16_t opcode, struct net_buf *buf,
				struct net_buf **rsp)
{
	return bt_hci_cmd_send_sync(opcode, buf, rsp);
}
