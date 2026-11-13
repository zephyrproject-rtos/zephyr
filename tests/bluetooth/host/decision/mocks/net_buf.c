/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/net_buf.h"

DEFINE_FAKE_VOID_FUNC(net_buf_unref, struct net_buf *);
