/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/settings/settings.h>

#include <zephyr/logging/log.h>

#include "bs_tracing.h"
#include "bstests.h"

#define FAIL(...)                                                                                  \
	do {                                                                                       \
		bst_result = Failed;                                                               \
		bs_trace_error_time_line(__VA_ARGS__);                                             \
	} while (0)

#define PASS(...)                                                                                  \
	do {                                                                                       \
		bst_result = Passed;                                                               \
		bs_trace_info_time(1, __VA_ARGS__);                                                \
	} while (0)

extern enum bst_result_t bst_result;

#define CREATE_FLAG(flag) static atomic_t flag = (atomic_t) false
#define SET_FLAG(flag)	  (void)atomic_set(&flag, (atomic_t) true)
#define GET_FLAG(flag)	  (bool)atomic_get(&flag)
#define UNSET_FLAG(flag)  (void)atomic_set(&flag, (atomic_t) false)
#define WAIT_FOR_FLAG(flag)                                                                        \
	while (!(bool)atomic_get(&flag)) {                                                         \
		(void)k_sleep(K_MSEC(1));                                                          \
	}

LOG_MODULE_DECLARE(bt_bsim_ccc_store, LOG_LEVEL_DBG);

#define DUMMY_SERVICE_TYPE    BT_UUID_128_ENCODE(0x2e2b8dc3, 0x06e0, 0x4f93, 0x9bb2, 0x734091c356f0)
#define BT_UUID_DUMMY_SERVICE BT_UUID_DECLARE_128(DUMMY_SERVICE_TYPE)

#define DUMMY_SERVICE_NOTIFY_TYPE                                                                  \
	BT_UUID_128_ENCODE(0x2e2b8dc3, 0x06e0, 0x4f93, 0x9bb2, 0x734091c356f3)
#define BT_UUID_DUMMY_SERVICE_NOTIFY BT_UUID_DECLARE_128(DUMMY_SERVICE_NOTIFY_TYPE)

#define CCC_HANDLE 19

#define BC_MSG_SIZE 2

#define CLIENT_ID 0
#define SERVER_ID 1

void backchannel_sync_send(uint channel, uint device_nbr);
void backchannel_sync_wait(uint channel, uint device_nbr);
