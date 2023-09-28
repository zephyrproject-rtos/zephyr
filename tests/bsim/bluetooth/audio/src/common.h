/**
 * Common functions and helpers for BSIM audio tests
 *
 * Copyright (c) 2019 Bose Corporation
 * Copyright (c) 2020-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TEST_BSIM_BT_AUDIO_TEST_
#define ZEPHYR_TEST_BSIM_BT_AUDIO_TEST_

#include <zephyr/kernel.h>

#include "bs_types.h"
#include "bs_tracing.h"
#include "time_machine.h"
#include "bstests.h"

#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>
#include <zephyr/sys_clock.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#define WAIT_SECONDS 60 /* seconds */
#define WAIT_TIME (WAIT_SECONDS * USEC_PER_SEC) /* microseconds*/

#define WAIT_FOR_COND(cond) while (!(cond)) { k_sleep(K_MSEC(1)); }

#define CREATE_FLAG(flag) static atomic_t flag = (atomic_t)false
#define SET_FLAG(flag) (void)atomic_set(&flag, (atomic_t)true)
#define UNSET_FLAG(flag) (void)atomic_clear(&flag)
#define TEST_FLAG(flag) (atomic_get(&flag) == (atomic_t)true)
#define WAIT_FOR_FLAG(flag) \
	while (!(bool)atomic_get(&flag)) { \
		(void)k_sleep(K_MSEC(1)); \
	}
#define WAIT_FOR_UNSET_FLAG(flag) \
	while (atomic_get(&flag) != (atomic_t)false) { \
		(void)k_sleep(K_MSEC(1)); \
	}


#define FAIL(...) \
	do { \
		bst_result = Failed; \
		bs_trace_error_time_line(__VA_ARGS__); \
	} while (0)

#define PASS(...) \
	do { \
		bst_result = Passed; \
		bs_trace_info_time(1, "PASSED: " __VA_ARGS__); \
	} while (0)

#define AD_SIZE 1

#define INVALID_BROADCAST_ID (BT_AUDIO_BROADCAST_ID_MAX + 1)
#define SYNC_RETRY_COUNT     6 /* similar to retries for connections */
#define PA_SYNC_SKIP         5

extern struct bt_le_scan_cb common_scan_cb;
extern const struct bt_data ad[AD_SIZE];
extern struct bt_conn *default_conn;
extern atomic_t flag_connected;
extern atomic_t flag_conn_updated;

void disconnected(struct bt_conn *conn, uint8_t reason);
void test_tick(bs_time_t HW_device_time);
void test_init(void);

#endif /* ZEPHYR_TEST_BSIM_BT_AUDIO_TEST_ */
