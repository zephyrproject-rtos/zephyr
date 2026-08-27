/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(bt_bsim_ccc_store, LOG_LEVEL_DBG);

#define DUMMY_SERVICE_TYPE    BT_UUID_128_ENCODE(0x2e2b8dc3, 0x06e0, 0x4f93, 0x9bb2, 0x734091c356f0)
#define BT_UUID_DUMMY_SERVICE BT_UUID_DECLARE_128(DUMMY_SERVICE_TYPE)

#define DUMMY_SERVICE_NOTIFY_TYPE                                                                  \
	BT_UUID_128_ENCODE(0x2e2b8dc3, 0x06e0, 0x4f93, 0x9bb2, 0x734091c356f3)
#define BT_UUID_DUMMY_SERVICE_NOTIFY BT_UUID_DECLARE_128(DUMMY_SERVICE_NOTIFY_TYPE)

#define VAL_HANDLE 18
#define CCC_HANDLE 19
