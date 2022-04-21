/* common.h - Common test code */

/*
 * Copyright (c) 2022 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>

#include <zephyr/types.h>
#include <sys/printk.h>
#include <sys/util.h>
#include <sys/byteorder.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/att.h>

#include "bs_types.h"
#include "bs_tracing.h"
#include "bstests.h"

extern enum bst_result_t bst_result;

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

extern volatile int num_eatt_channels;
extern struct bt_conn *default_conn;

void central_setup_and_connect(void);
void peripheral_setup_and_connect(void);
void wait_for_disconnect(void);
void disconnect(void);
void test_init(void);
void test_tick(bs_time_t HW_device_time);
