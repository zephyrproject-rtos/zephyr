/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/hci_core.h"

#include <zephyr/bluetooth/hci.h>
#include <zephyr/kernel.h>

DEFINE_FAKE_VALUE_FUNC(struct net_buf *, bt_hci_cmd_create, uint16_t, uint8_t);
DEFINE_FAKE_VALUE_FUNC(int, bt_hci_cmd_send_sync, uint16_t, struct net_buf *, struct net_buf **);
