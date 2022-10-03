/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include "mocks/id.h"

DEFINE_FFF_GLOBALS;

DEFINE_FAKE_VOID_FUNC(bt_id_del, struct bt_keys *);
