/**
 * Common functions and helpers for this test
 *
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bs_tracing.h"
#include "bs_types.h"
#include "bstests.h"
#include "time_machine.h"
#include <zephyr/sys/__assert.h>

#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/types.h>

extern enum bst_result_t bst_result;

#define BS_SECONDS(dur_sec)    ((bs_time_t)dur_sec * 1000000)
#define TEST_TIMEOUT_SIMULATED BS_SECONDS(60)

#define DECLARE_FLAG(flag) extern atomic_t flag
#define DEFINE_FLAG(flag)  atomic_t flag = (atomic_t) false
#define SET_FLAG(flag)	   (void)atomic_set(&flag, (atomic_t) true)
#define UNSET_FLAG(flag)   (void)atomic_set(&flag, (atomic_t) false)
#define WAIT_FOR_FLAG(flag)                                                                        \
	while (!(bool)atomic_get(&flag)) {                                                         \
		(void)k_sleep(K_MSEC(1));                                                          \
	}
#define WAIT_FOR_FLAG_UNSET(flag)                                                                  \
	while ((bool)atomic_get(&flag)) {                                                          \
		(void)k_sleep(K_MSEC(1));                                                          \
	}
#define TAKE_FLAG(flag)                                                                            \
	while (!(bool)atomic_cas(&flag, true, false)) {                                            \
		(void)k_sleep(K_MSEC(1));                                                          \
	}
#define GET_FLAG(flag)                                                                             \
	(bool)atomic_get(&flag)

#define ASSERT(expr, ...)                                                                          \
	do {                                                                                       \
		if (!(expr)) {                                                                     \
			FAIL(__VA_ARGS__);                                                         \
		}                                                                                  \
	} while (0)

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

void test_tick(bs_time_t HW_device_time);
void test_init(void);

void bs_bt_utils_setup(void);

void clear_conn(struct bt_conn *conn);
void wait_connected(struct bt_conn **conn);
void wait_disconnected(void);
void disconnect(struct bt_conn *conn);
void scan_connect_to_first_result(void);
void advertise_connectable(int id);

void set_security(struct bt_conn *conn, bt_security_t sec);
void wait_pairing_completed(void);

void bas_subscribe(struct bt_conn *conn);
void wait_bas_ccc_subscription(void);
void bas_notify(struct bt_conn *conn);
void wait_bas_notification(void);
