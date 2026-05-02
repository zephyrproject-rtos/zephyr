/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include "buf_view.h"

DEFINE_FAKE_VALUE_FUNC(bool, bt_buf_has_view, const struct net_buf *);
DEFINE_FAKE_VALUE_FUNC(struct net_buf *, bt_buf_make_view, struct net_buf *, struct net_buf *,
		       size_t, struct bt_buf_view_meta *);
DEFINE_FAKE_VOID_FUNC(bt_buf_destroy_view, struct net_buf *, struct bt_buf_view_meta *);
