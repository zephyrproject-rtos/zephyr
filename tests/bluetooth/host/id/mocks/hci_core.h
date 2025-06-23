/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/addr.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>

/* List of fakes used by this unit tester */
#define HCI_CORE_FFF_FAKES_LIST(FAKE)                                                              \
	FAKE(bt_unpair)                                                                            \
	FAKE(bt_hci_cmd_alloc)                                                                    \
	FAKE(bt_hci_cmd_send_sync)

DECLARE_FAKE_VALUE_FUNC(int, bt_unpair, uint8_t, const bt_addr_le_t *);
DECLARE_FAKE_VALUE_FUNC(struct net_buf *, bt_hci_cmd_alloc, k_timeout_t);
DECLARE_FAKE_VALUE_FUNC(int, bt_hci_cmd_send_sync, uint16_t, struct net_buf *, struct net_buf **);
