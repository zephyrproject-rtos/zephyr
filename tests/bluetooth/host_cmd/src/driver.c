/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/* A scriptable controller behind an HCI driver, for tests of the Host's HCI
 * command handling. It responds to the initialization commands as far as the
 * Host needs, and lets a test decide when and how it responds to two commands.
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "hci_core.h"

#include "driver.h"

#define DT_DRV_COMPAT zephyr_bt_hci_test

/* Large enough for the return parameters of initialization commands that are
 * irrelevant to these tests, of which Read Local Supported Commands is the
 * largest.
 */
#define GENERIC_RP_SIZE sizeof(struct bt_hci_rp_read_supported_commands)

/* The LE ACL buffers of the controller: the smallest size there is, and a
 * few of them
 */
#define LE_ACL_MAX_LEN 27U
#define LE_ACL_MAX_NUM 3U

struct test_cmd_script test_cmd;
struct test_cmd_script test_disconnect;
uint8_t test_cmd_plen[TEST_CMD_PLEN_MAX];
unsigned int test_reset_count;
unsigned int test_close_count;
struct net_buf *test_cmd_bufs[TEST_CMD_BUF_MAX];

/* Given whenever a scripted command arrives or is responded to */
static K_SEM_DEFINE(script_sem, 0, K_SEM_MAX_LIMIT);

static const struct device *const hci_dev = DEVICE_DT_GET(DT_DRV_INST(0));

/* First parameter byte of the TEST_OPCODE command that the controller got last */
static uint8_t test_cmd_param;

static void *evt_create(struct net_buf **buf, uint8_t evt, uint8_t len)
{
	struct bt_hci_evt_hdr *hdr;

	*buf = bt_buf_get_evt(evt, false, K_FOREVER);
	hdr = net_buf_add(*buf, sizeof(*hdr));
	hdr->evt = evt;
	hdr->len = len;

	return net_buf_add(*buf, len);
}

static void *cmd_complete(struct net_buf **buf, uint16_t opcode, uint8_t ncmd, uint8_t plen)
{
	struct bt_hci_evt_cmd_complete *cc;

	cc = evt_create(buf, BT_HCI_EVT_CMD_COMPLETE, sizeof(*cc) + plen);
	cc->ncmd = ncmd;
	cc->opcode = sys_cpu_to_le16(opcode);
	(void)memset(&cc[1], 0, plen);

	return &cc[1];
}

void test_cmd_respond(struct test_cmd_script *script)
{
	struct net_buf *evt;

	if (script == &test_disconnect) {
		struct bt_hci_evt_cmd_status *status;

		status = evt_create(&evt, BT_HCI_EVT_CMD_STATUS, sizeof(*status));
		status->status = script->status;
		status->ncmd = script->ncmd;
		status->opcode = sys_cpu_to_le16(BT_HCI_OP_DISCONNECT);
		bt_hci_recv(hci_dev, evt);

		if (script->status == BT_HCI_ERR_SUCCESS) {
			test_disconnect_complete(BT_HCI_ERR_LOCALHOST_TERM_CONN);
		}
	} else {
		uint8_t *rp;

		rp = cmd_complete(&evt, TEST_OPCODE, script->ncmd, 1U + script->rp_len);
		rp[0] = script->status;
		for (uint8_t i = 0U; i < script->rp_len; i++) {
			rp[1U + i] = script->echo ? test_cmd_param : i;
		}

		bt_hci_recv(hci_dev, evt);
	}

	script->rsp_count++;
	k_sem_give(&script_sem);
}

static int script_wait(const unsigned int *counter, unsigned int count)
{
	while (*counter < count) {
		if (k_sem_take(&script_sem, TEST_TIMEOUT) != 0) {
			return -EAGAIN;
		}
	}

	return 0;
}

int test_cmd_wait(const struct test_cmd_script *script, unsigned int count)
{
	return script_wait(&script->count, count);
}

int test_rsp_wait(const struct test_cmd_script *script, unsigned int count)
{
	return script_wait(&script->rsp_count, count);
}

static K_SEM_DEFINE(flush_sem, 0, 1);

static void flush_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	k_sem_give(&flush_sem);
}

static K_WORK_DEFINE(flush_work, flush_handler);

/* A queue runs its work items in order: the flush work item runs once what
 * was submitted before it has.
 */
static void flush_wait(int submit_err)
{
	struct k_work_sync sync;

	zassert_true(submit_err >= 0, "Submitting the flush failed (err %d)", submit_err);
	zassert_ok(k_sem_take(&flush_sem, TEST_TIMEOUT), "The queue is stuck");

	/* The queue is done with the work item too, for the next flush */
	(void)k_work_flush(&flush_work, &sync);
}

void test_workq_flush(struct k_work_q *queue)
{
	flush_wait(k_work_submit_to_queue(queue, &flush_work));
}

void test_bt_workq_flush(void)
{
	flush_wait(bt_work_submit(&flush_work));
}

void test_settle(void)
{
	/* No time passes on native_sim while a thread runs, so a sleep of one
	 * tick returns only once every thread that was ready has run until it
	 * waits.
	 */
	(void)k_sleep(K_TICKS(1));
}

unsigned int test_cmd_bufs_hold(void)
{
	unsigned int count = 0U;

	ARRAY_FOR_EACH(test_cmd_bufs, i) {
		test_cmd_bufs[i] = bt_hci_cmd_alloc(K_NO_WAIT);
		if (test_cmd_bufs[i] == NULL) {
			break;
		}

		count++;
	}

	zassert_true(count < ARRAY_SIZE(test_cmd_bufs),
		     "The Host has more command buffers than expected");

	return count;
}

void test_cmd_bufs_release(void)
{
	ARRAY_FOR_EACH(test_cmd_bufs, i) {
		net_buf_drop(&test_cmd_bufs[i]);
	}
}

void test_cmd_pool_check(void)
{
	struct net_buf_pool *pool;
	unsigned int count;

	count = test_cmd_bufs_hold();
	zassert_true(count > 0U, "No command buffer is free");
	pool = net_buf_pool_get(test_cmd_bufs[0]->pool_id);
	zassert_equal(count, pool->buf_count, "%u of %u command buffers are free", count,
		      pool->buf_count);
	zassert_equal(atomic_get(&pool->avail_count), 0, "The pool counts %ld free buffers",
		      atomic_get(&pool->avail_count));

	test_cmd_bufs_release();
	zassert_equal(atomic_get(&pool->avail_count), pool->buf_count,
		      "The pool counts %ld free buffers", atomic_get(&pool->avail_count));
}

void test_credit_grant(void)
{
	struct net_buf *evt;

	(void)cmd_complete(&evt, 0U, 1U, 0U);
	bt_hci_recv(hci_dev, evt);
}

void test_credit_revoke(void)
{
	struct net_buf *evt;

	(void)cmd_complete(&evt, 0U, 0U, 0U);
	bt_hci_recv(hci_dev, evt);
}

bool test_with_reset(const void *state)
{
	ARG_UNUSED(state);

	return TEST_DRIVER_RESETS;
}

bool test_without_reset(const void *state)
{
	ARG_UNUSED(state);

	return !TEST_DRIVER_RESETS;
}

static void cmd_rsp_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	test_cmd_respond(&test_cmd);
}

static void disconnect_rsp_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	test_cmd_respond(&test_disconnect);
}

static K_WORK_DELAYABLE_DEFINE(cmd_rsp_work, cmd_rsp_handler);
static K_WORK_DELAYABLE_DEFINE(disconnect_rsp_work, disconnect_rsp_handler);

static void scripted(struct test_cmd_script *script, struct k_work_delayable *work)
{
	script->count++;
	k_sem_give(&script_sem);

	if (K_TIMEOUT_EQ(script->delay, K_NO_WAIT)) {
		test_cmd_respond(script);
	} else if (!K_TIMEOUT_EQ(script->delay, K_FOREVER)) {
		(void)k_work_schedule(work, script->delay);
	} else {
		/* The test responds with test_cmd_respond() */
	}
}

void test_connect(void)
{
	/* A static random address, as its two most significant bits say */
	const bt_addr_le_t peer = {
		.type = BT_ADDR_LE_RANDOM,
		.a.val = { 0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0xc6U },
	};
	struct bt_hci_evt_le_conn_complete *cc;
	struct bt_hci_evt_le_meta_event *meta;
	struct net_buf *evt;

	meta = evt_create(&evt, BT_HCI_EVT_LE_META_EVENT, sizeof(*meta) + sizeof(*cc));
	meta->subevent = BT_HCI_EVT_LE_CONN_COMPLETE;
	cc = (void *)&meta[1];
	(void)memset(cc, 0, sizeof(*cc));
	cc->status = BT_HCI_ERR_SUCCESS;
	cc->handle = sys_cpu_to_le16(TEST_CONN_HANDLE);
	cc->role = BT_HCI_ROLE_PERIPHERAL;
	bt_addr_le_copy(&cc->peer_addr, &peer);
	cc->interval = sys_cpu_to_le16(BT_GAP_INIT_CONN_INT_MIN);
	/* Any valid value: the test controller does not supervise the connection */
	cc->supv_timeout = sys_cpu_to_le16(BT_GAP_MS_TO_CONN_TIMEOUT(4000U));

	bt_hci_recv(hci_dev, evt);
}

void test_disconnect_complete(uint8_t reason)
{
	struct bt_hci_evt_disconn_complete *complete;
	struct net_buf *evt;

	complete = evt_create(&evt, BT_HCI_EVT_DISCONN_COMPLETE, sizeof(*complete));
	complete->status = BT_HCI_ERR_SUCCESS;
	complete->handle = sys_cpu_to_le16(TEST_CONN_HANDLE);
	complete->reason = reason;

	bt_hci_recv(hci_dev, evt);
}

static void cmd_handle(struct net_buf *cmd)
{
	struct bt_hci_cmd_hdr *chdr;
	struct net_buf *evt;
	uint16_t opcode;
	uint8_t *rp;

	chdr = net_buf_pull_mem(cmd, sizeof(*chdr));
	opcode = sys_le16_to_cpu(chdr->opcode);

	switch (opcode) {
	case TEST_OPCODE:
		if (test_cmd.count < ARRAY_SIZE(test_cmd_plen)) {
			test_cmd_plen[test_cmd.count] = (uint8_t)cmd->len;
		}

		test_cmd_param = (cmd->len > 0U) ? cmd->data[0] : 0U;
		scripted(&test_cmd, &cmd_rsp_work);
		return;
	case BT_HCI_OP_DISCONNECT:
		scripted(&test_disconnect, &disconnect_rsp_work);
		return;
	case BT_HCI_OP_RESET:
		test_reset_count++;
		break;
	default:
		break;
	}

	rp = cmd_complete(&evt, opcode, 1U, GENERIC_RP_SIZE);

	switch (opcode) {
	case BT_HCI_OP_READ_LOCAL_FEATURES: {
		struct bt_hci_rp_read_local_features *features = (void *)rp;

		/* LE supported and BR/EDR not supported */
		features->features[4] = BIT(5) | BIT(6);
		break;
	}
	case BT_HCI_OP_READ_SUPPORTED_COMMANDS: {
		struct bt_hci_rp_read_supported_commands *commands = (void *)rp;

		(void)memset(commands->commands, 0xFF, sizeof(commands->commands));
		break;
	}
	case BT_HCI_OP_READ_BD_ADDR: {
		struct bt_hci_rp_read_bd_addr *addr = (void *)rp;

		(void)memset(&addr->bdaddr, 0x11, sizeof(addr->bdaddr));
		break;
	}
	case BT_HCI_OP_LE_READ_BUFFER_SIZE: {
		struct bt_hci_rp_le_read_buffer_size *size = (void *)rp;

		size->le_max_len = sys_cpu_to_le16(LE_ACL_MAX_LEN);
		size->le_max_num = LE_ACL_MAX_NUM;
		break;
	}
	case BT_HCI_OP_LE_READ_SUPP_STATES: {
		struct bt_hci_rp_le_read_supp_states *states = (void *)rp;

		(void)memset(states->le_states, 0xFF, sizeof(states->le_states));
		break;
	}
	default:
		break;
	}

	bt_hci_recv(hci_dev, evt);
}

static int send_err(const struct net_buf *buf)
{
	uint16_t opcode = sys_get_le16(buf->data);

	if (opcode == TEST_OPCODE) {
		return test_cmd.send_err;
	}

	if (opcode == BT_HCI_OP_DISCONNECT) {
		return test_disconnect.send_err;
	}

	return 0;
}

static int driver_open(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int driver_close(const struct device *dev)
{
	ARG_UNUSED(dev);

	test_close_count++;

	return 0;
}

static int driver_send(const struct device *dev, struct net_buf *buf)
{
	ARG_UNUSED(dev);

	if (buf->data[0] == BT_HCI_H4_CMD) {
		int err;

		(void)net_buf_pull_u8(buf);

		err = send_err(buf);
		if (err != 0) {
			/* The buffer stays with the caller */
			return err;
		}

		cmd_handle(buf);
	}

	net_buf_unref(buf);

	return 0;
}

static DEVICE_API(bt_hci, driver_api) = {
	.open = driver_open,
	.close = driver_close,
	.send = driver_send,
};

void test_driver_reset(void)
{
	static const struct test_cmd_script respond_at_once = {
		.delay = K_NO_WAIT,
		.status = BT_HCI_ERR_SUCCESS,
		.ncmd = 1U,
	};

	(void)k_work_cancel_delayable(&cmd_rsp_work);
	(void)k_work_cancel_delayable(&disconnect_rsp_work);

	test_cmd = respond_at_once;
	test_disconnect = respond_at_once;
	test_reset_count = 0U;
	test_close_count = 0U;
	k_sem_reset(&script_sem);
}

void test_driver_release(void)
{
	bool is_cmd_held = (test_cmd.rsp_count < test_cmd.count);
	bool is_disconnect_held = (test_disconnect.rsp_count < test_disconnect.count);

	test_driver_reset();

	if (is_cmd_held) {
		test_cmd_respond(&test_cmd);
	}

	if (is_disconnect_held) {
		test_cmd_respond(&test_disconnect);
	}

	test_credit_grant();
}

#define TEST_DEVICE_INIT(inst)                                                                     \
	static struct bt_hci_driver_data driver_data_##inst = { 0 };                               \
	static const struct bt_hci_driver_config driver_config_##inst =                            \
		BT_DT_HCI_DRIVER_CONFIG_INST_GET(inst);                                            \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, &driver_data_##inst, &driver_config_##inst,        \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &driver_api)

DT_INST_FOREACH_STATUS_OKAY(TEST_DEVICE_INIT)
