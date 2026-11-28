/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <host/hci_core.h>

#include "mocks/hci_core.h"

struct bt_dev bt_dev = {0};

DEFINE_FAKE_VALUE_FUNC(struct net_buf *, bt_hci_cmd_alloc, k_timeout_t);
DEFINE_FAKE_VALUE_FUNC(int, bt_hci_cmd_send_sync, uint16_t, struct net_buf *, struct net_buf **);
