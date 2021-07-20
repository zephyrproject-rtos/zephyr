/** @file
 *  @brief Common functionality for Bluetooth mesh BabbleSim tests.
 */

/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_TESTS_BLUETOOTH_BSIM_BT_BSIM_TEST_MESH_MESH_TEST_H_
#define ZEPHYR_TESTS_BLUETOOTH_BSIM_BT_BSIM_TEST_MESH_MESH_TEST_H_
#include "kernel.h"

#include "bs_types.h"
#include "bs_tracing.h"
#include "time_machine.h"
#include "bstests.h"

#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>
#include <zephyr.h>
#include <sys/printk.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/mesh.h>

#define TEST_MOD_ID 0x8888
#define TEST_MSG_OP BT_MESH_MODEL_OP_1(0x0f)

#define FAIL(msg, ...)                                                         \
	do {                                                                   \
		bst_result = Failed;                                           \
		bs_trace_error_time_line(msg "\n", ##__VA_ARGS__);             \
	} while (0)

#define PASS()                                                                 \
	do {                                                                   \
		bst_result = Passed;                                           \
		bs_trace_info_time(1, "%s PASSED\n", __func__);                \
	} while (0)

#define ASSERT_OK(cond, ...)                                                   \
	do {                                                                   \
		int _err = (cond);                                             \
		if (_err) {                                                    \
			bst_result = Failed;                                   \
			bs_trace_error_time_line(                              \
				#cond " failed with error %d\n", _err);        \
		}                                                              \
	} while (0)

#define ASSERT_TRUE(cond, ...)                                                 \
	do {                                                                   \
		if (!(cond)) {                                                   \
			bst_result = Failed;                                   \
			bs_trace_error_time_line(                              \
				#cond "is false.", ##__VA_ARGS__);             \
		}                                                              \
	} while (0)

struct bt_mesh_test_cfg {
	uint16_t addr;
	uint8_t dev_key[16];
};

enum bt_mesh_test_send_flags {
	FORCE_SEGMENTATION = BIT(0),
	LONG_MIC = BIT(1),
};

struct bt_mesh_test_stats {
	uint32_t received;
	uint32_t sent;
	uint32_t recv_overflow;
};

struct bt_mesh_test_msg {
	sys_snode_t _node;
	size_t len;
	uint8_t seq;
	struct bt_mesh_msg_ctx ctx;
};

extern enum bst_result_t bst_result;
extern const struct bt_mesh_test_cfg *cfg;
extern struct bt_mesh_model *test_model;
extern const uint8_t test_net_key[16];
extern const uint8_t test_app_key[16];
extern const uint8_t test_va_uuid[16];
extern struct bt_mesh_test_stats test_stats;
extern struct bt_mesh_msg_ctx test_send_ctx;

void bt_mesh_test_cfg_set(const struct bt_mesh_test_cfg *cfg, int wait_time);
void bt_mesh_test_setup(void);
void bt_mesh_test_timeout(bs_time_t HW_device_time);

int bt_mesh_test_recv(uint16_t len, uint16_t dst, k_timeout_t timeout);
int bt_mesh_test_recv_msg(struct bt_mesh_test_msg *msg, k_timeout_t timeout);
int bt_mesh_test_recv_clear(void);

int bt_mesh_test_send(uint16_t addr, size_t len,
		      enum bt_mesh_test_send_flags flags, k_timeout_t timeout);
int bt_mesh_test_send_async(uint16_t addr, size_t len,
			    enum bt_mesh_test_send_flags flags,
			    const struct bt_mesh_send_cb *send_cb,
			    void *cb_data);
#endif /* ZEPHYR_TESTS_BLUETOOTH_BSIM_BT_BSIM_TEST_MESH_MESH_TEST_H_ */
