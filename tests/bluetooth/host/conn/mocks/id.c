/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include "id.h"

DEFINE_FAKE_VALUE_FUNC(bool, bt_id_scan_random_addr_check);
