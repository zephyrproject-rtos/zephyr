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
#include <zephyr/kernel.h>

#include "bs_types.h"
#include "bs_tracing.h"
#include "time_machine.h"
#include "bstests.h"

#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>
#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/mesh.h>

#define TEST_MOD_ID 0x8888
#define TEST_MSG_OP_1  BT_MESH_MODEL_OP_1(0x0f)
#define TEST_MSG_OP_2  BT_MESH_MODEL_OP_1(0x10)

#define TEST_VND_COMPANY_ID 0x1234
#define TEST_VND_MOD_ID   0x5678

#define MODEL_LIST(...) ((struct bt_mesh_model[]){ __VA_ARGS__ })

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

#define ASSERT_OK(cond)                                                        \
	do {                                                                   \
		int _err = (cond);                                             \
		if (_err) {                                                    \
			bst_result = Failed;                                   \
			bs_trace_error_time_line(                              \
				#cond " failed with error %d\n", _err);        \
		}                                                              \
	} while (0)

#define ASSERT_OK_MSG(cond, fmt, ...)                                          \
	do {                                                                   \
		int _err = (cond);                                             \
		if (_err) {                                                    \
			bst_result = Failed;                                   \
			bs_trace_error_time_line(                              \
				#cond " failed with error %d\n" fmt, _err,     \
				##__VA_ARGS__);                                \
		}                                                              \
	} while (0)

#define ASSERT_TRUE(cond)                                                      \
	do {                                                                   \
		if (!(cond)) {                                                 \
			bst_result = Failed;                                   \
			bs_trace_error_time_line(                              \
				#cond " is false.\n");                         \
		}                                                              \
	} while (0)

#define ASSERT_TRUE_MSG(cond, fmt, ...)                                        \
	do {                                                                   \
		if (!(cond)) {                                                 \
			bst_result = Failed;                                   \
			bs_trace_error_time_line(                              \
				#cond " is false. " fmt, ##__VA_ARGS__);       \
		}                                                              \
	} while (0)


#define ASSERT_FALSE(cond)                                                     \
	do {                                                                   \
		if (cond) {                                                    \
			bst_result = Failed;                                   \
			bs_trace_error_time_line(                              \
				#cond " is true.\n");                          \
		}                                                              \
	} while (0)

#define ASSERT_FALSE_MSG(cond, fmt, ...)                                       \
	do {                                                                   \
		if (cond) {                                                    \
			bst_result = Failed;                                   \
			bs_trace_error_time_line(                              \
				#cond " is true. " fmt, ##__VA_ARGS__);        \
		}                                                              \
	} while (0)

#define ASSERT_EQUAL(expected, got)                                            \
	do {                                                                   \
		if ((expected) != (got)) {                                     \
			bst_result = Failed;                                   \
			bs_trace_error_time_line(                              \
				#expected " not equal to " #got ": %d != %d\n",\
				(expected), (got));                            \
		}                                                              \
	} while (0)

#define ASSERT_IN_RANGE(got, min, max)                                                             \
	do {                                                                                       \
		if (!IN_RANGE(got, min, max)) {                                            \
			bst_result = Failed;                                                       \
			bs_trace_error_time_line(#got " not in range %d <-> %d, " #got " = %d\n",  \
						 (min), (max), (got));                             \
		}                                                                                  \
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

struct bt_mesh_test_sync_ctx {
	uint32_t *dev_nmbr;
	uint32_t *chan_nmbr;
	uint32_t *chan_id;
	uint16_t cnt;
};

extern enum bst_result_t bst_result;
extern const struct bt_mesh_test_cfg *cfg;
extern struct bt_mesh_model *test_model;
extern struct bt_mesh_model *test_vnd_model;
extern const uint8_t test_net_key[16];
extern const uint8_t test_app_key[16];
extern const uint8_t test_va_uuid[16];
extern struct bt_mesh_test_stats test_stats;
extern struct bt_mesh_msg_ctx test_send_ctx;

void bt_mesh_test_cfg_set(const struct bt_mesh_test_cfg *cfg, int wait_time);
void bt_mesh_test_setup(void);
void bt_mesh_test_timeout(bs_time_t HW_device_time);

void bt_mesh_device_setup(const struct bt_mesh_prov *prov, const struct bt_mesh_comp *comp);

int bt_mesh_test_recv(uint16_t len, uint16_t dst, const uint8_t *uuid, k_timeout_t timeout);
int bt_mesh_test_recv_msg(struct bt_mesh_test_msg *msg, k_timeout_t timeout);
int bt_mesh_test_recv_clear(void);

int bt_mesh_test_send(uint16_t addr, const uint8_t *uuid, size_t len,
		      enum bt_mesh_test_send_flags flags, k_timeout_t timeout);
int bt_mesh_test_send_async(uint16_t addr, const uint8_t *uuid, size_t len,
			    enum bt_mesh_test_send_flags flags,
			    const struct bt_mesh_send_cb *send_cb,
			    void *cb_data);
int bt_mesh_test_send_ra(uint16_t addr, uint8_t *data, size_t len,
			 const struct bt_mesh_send_cb *send_cb,
			 void *cb_data);
void bt_mesh_test_ra_cb_setup(void (*cb)(uint8_t *, size_t));

uint16_t bt_mesh_test_own_addr_get(uint16_t start_addr);

#if defined(CONFIG_BT_MESH_SAR_CFG)
void bt_mesh_test_sar_conf_set(struct bt_mesh_sar_tx *tx_set, struct bt_mesh_sar_rx *rx_set);
#endif

#endif /* ZEPHYR_TESTS_BLUETOOTH_BSIM_BT_BSIM_TEST_MESH_MESH_TEST_H_ */
