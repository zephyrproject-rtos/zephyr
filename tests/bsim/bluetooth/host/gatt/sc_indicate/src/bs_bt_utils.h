/**
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bstests.h"
#include "bs_tracing.h"

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>

extern enum bst_result_t bst_result;

#define DECLARE_FLAG(flag) extern atomic_t flag
#define DEFINE_FLAG(flag)  atomic_t flag = (atomic_t) false
#define SET_FLAG(flag)     (void)atomic_set(&flag, (atomic_t) true)
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
#define GET_FLAG(flag) (bool)atomic_get(&flag)

#define BSIM_ASSERT(expr, ...)                                                                     \
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

DECLARE_FLAG(flag_pairing_complete);
DECLARE_FLAG(flag_bonded);
DECLARE_FLAG(flag_not_bonded);

void scan_connect_to_first_result(void);
struct bt_conn *get_g_conn(void);
void clear_g_conn(void);
void disconnect(void);
void wait_connected(void);
void wait_disconnected(void);
void create_adv(struct bt_le_ext_adv **adv);
void start_adv(struct bt_le_ext_adv *adv);
void stop_adv(struct bt_le_ext_adv *adv);
void set_security(bt_security_t sec);
void pairing_complete(struct bt_conn *conn, bool bonded);
void pairing_failed(struct bt_conn *conn, enum bt_security_err err);
