/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/settings/settings.h>

#include <zephyr/logging/log.h>

#include "bs_tracing.h"

LOG_MODULE_DECLARE(bt_bsim_ccc_update, LOG_LEVEL_DBG);

#define DUMMY_SERVICE_TYPE    BT_UUID_128_ENCODE(0x2e2b8dc3, 0x06e0, 0x4f93, 0x9bb2, 0x734091c356f0)
#define BT_UUID_DUMMY_SERVICE BT_UUID_DECLARE_128(DUMMY_SERVICE_TYPE)

#define DUMMY_SERVICE_NOTIFY_TYPE                                                                  \
	BT_UUID_128_ENCODE(0x2e2b8dc3, 0x06e0, 0x4f93, 0x9bb2, 0x734091c356f3)
#define BT_UUID_DUMMY_SERVICE_NOTIFY BT_UUID_DECLARE_128(DUMMY_SERVICE_NOTIFY_TYPE)

#define CCC_HANDLE 19

#define BC_MSG_SIZE 2

#define GOOD_CLIENT_ID 0
#define BAD_CLIENT_ID 1
#define SERVER_ID 2

void backchannel_sync_send(uint channel, uint device_nbr);
void backchannel_sync_wait(uint channel, uint device_nbr);
