/**
 * Common functions and helpers for BSIM ADV tests
 *
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bs_tracing.h"
#include "bs_types.h"
#include "bstests.h"
#include "time_machine.h"
#include <zephyr/sys/__assert.h>

#include <errno.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>

extern enum bst_result_t bst_result;

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

#define ASSERT(expr, ...)                                                                          \
	do {                                                                                       \
		if (!(expr)) {                                                                     \
			FAIL(__VA_ARGS__);                                                         \
		}                                                                                  \
	} while (0)

DECLARE_FLAG(flag_test_end);

#define FAIL(...)					\
	SET_FLAG(flag_test_end);			\
	do {						\
		bst_result = Failed;			\
		bs_trace_error_time_line(__VA_ARGS__);	\
	} while (0)

#define PASS(...)					\
	SET_FLAG(flag_test_end);			\
	do {						\
		bst_result = Passed;			\
		bs_trace_info_time(1, __VA_ARGS__);	\
	} while (0)

void test_tick(bs_time_t HW_device_time);
void test_init(void);
void backchannel_init(uint peer);
void backchannel_sync_send(void);
void backchannel_sync_wait(void);

void bs_bt_utils_setup(void);
void wait_connected(void);
void wait_disconnected(void);
void scan_connect_to_first_result(void);
void disconnect(void);
void advertise_connectable(int id, bool persist);
void scan_expect_same_address(void);
