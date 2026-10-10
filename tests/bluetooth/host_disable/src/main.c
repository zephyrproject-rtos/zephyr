/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/buf.h>
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
#include <zephyr/ztest_error_hook.h>

#define DT_DRV_COMPAT zephyr_bt_hci_test

/* Large enough for the return parameters of initialization commands that are
 * irrelevant to these tests.
 */
#define GENERIC_RP_SIZE 64U

struct cmd_handler {
	uint16_t opcode;
	uint8_t len;
	void (*handler)(struct net_buf *buf, struct net_buf **evt, uint8_t len, uint16_t opcode);
};

static void evt_create(struct net_buf *buf, uint8_t evt, uint8_t len)
{
	struct bt_hci_evt_hdr *hdr;

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->evt = evt;
	hdr->len = len;
}

static void *cmd_complete(struct net_buf **buf, uint8_t plen, uint16_t opcode)
{
	struct bt_hci_evt_cmd_complete *cc;

	*buf = bt_buf_get_evt(BT_HCI_EVT_CMD_COMPLETE, false, K_FOREVER);
	evt_create(*buf, BT_HCI_EVT_CMD_COMPLETE, sizeof(*cc) + plen);
	cc = net_buf_add(*buf, sizeof(*cc));
	cc->ncmd = 1U;
	cc->opcode = sys_cpu_to_le16(opcode);

	return net_buf_add(*buf, plen);
}

static void generic_success(struct net_buf *buf, struct net_buf **evt, uint8_t len, uint16_t opcode)
{
	struct bt_hci_evt_cc_status *rp;

	ARG_UNUSED(buf);

	rp = cmd_complete(evt, len, opcode);
	(void)memset(rp, 0, len);
	rp->status = BT_HCI_ERR_SUCCESS;
}

static void read_local_features(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				uint16_t opcode)
{
	struct bt_hci_rp_read_local_features *rp;

	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(rp->features, 0, sizeof(rp->features));
	/* LE supported and BR/EDR not supported. */
	rp->features[4] = BIT(5) | BIT(6);
}

static void read_supported_commands(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				    uint16_t opcode)
{
	struct bt_hci_rp_read_supported_commands *rp;

	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(rp->commands, 0xFF, sizeof(rp->commands));
}

static void read_bd_addr(struct net_buf *buf, struct net_buf **evt, uint8_t len, uint16_t opcode)
{
	struct bt_hci_rp_read_bd_addr *rp;

	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(&rp->bdaddr, 0x11, sizeof(rp->bdaddr));
}

static void le_read_local_features(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				   uint16_t opcode)
{
	struct bt_hci_rp_le_read_local_features *rp;

	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(rp->features, 0, sizeof(rp->features));
}

static void le_read_supp_states(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				uint16_t opcode)
{
	struct bt_hci_rp_le_read_supp_states *rp;

	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(rp->le_states, 0xFF, sizeof(rp->le_states));
}

/* A vendor command whose response is longer than the Host is configured to
 * receive, although it fits an HCI event.
 */
#define TEST_OP_LONG_RSP BT_OP(BT_OGF_VS, 0x03ff)
#define LONG_RSP_SIZE    200U

BUILD_ASSERT(LONG_RSP_SIZE > CONFIG_BT_BUF_EVT_RX_SIZE);

static void long_rsp(struct net_buf *buf, struct net_buf **evt, uint8_t len, uint16_t opcode)
{
	struct bt_hci_evt_cc_status *rp;

	ARG_UNUSED(buf);

	rp = cmd_complete(evt, len, opcode);
	(void)memset(rp, 0xAA, len);
	rp->status = BT_HCI_ERR_SUCCESS;
}

static const struct cmd_handler cmds[] = {
	{
		TEST_OP_LONG_RSP,
		LONG_RSP_SIZE,
		long_rsp,
	},
	{
		BT_HCI_OP_READ_LOCAL_FEATURES,
		sizeof(struct bt_hci_rp_read_local_features),
		read_local_features,
	},
	{
		BT_HCI_OP_READ_SUPPORTED_COMMANDS,
		sizeof(struct bt_hci_rp_read_supported_commands),
		read_supported_commands,
	},
	{
		BT_HCI_OP_READ_BD_ADDR,
		sizeof(struct bt_hci_rp_read_bd_addr),
		read_bd_addr,
	},
	{
		BT_HCI_OP_LE_READ_LOCAL_FEATURES,
		sizeof(struct bt_hci_rp_le_read_local_features),
		le_read_local_features,
	},
	{
		BT_HCI_OP_LE_READ_SUPP_STATES,
		sizeof(struct bt_hci_rp_le_read_supp_states),
		le_read_supp_states,
	},
};

/* What the test looks at: how often the controller has been reset, and what
 * the driver's close() is to return. A silent controller drops every command,
 * and a controller that stops goes silent once it has answered a command. A
 * driver that blocks stays in send() for longer than the command timeout.
 */
static unsigned int reset_count;
static unsigned int close_count;
static int close_err;
static bool silent;
static bool stops;
static bool blocks;

static void cmd_handle(const struct device *dev, struct net_buf *cmd)
{
	struct net_buf *evt = NULL;
	struct bt_hci_cmd_hdr *chdr;
	uint16_t opcode;

	chdr = net_buf_pull_mem(cmd, sizeof(*chdr));
	opcode = sys_le16_to_cpu(chdr->opcode);

	if (opcode == BT_HCI_OP_RESET) {
		reset_count++;
	}

	for (size_t i = 0U; i < ARRAY_SIZE(cmds); i++) {
		if (cmds[i].opcode == opcode) {
			cmds[i].handler(cmd, &evt, cmds[i].len, opcode);
			bt_hci_recv(dev, evt);
			return;
		}
	}

	generic_success(cmd, &evt, GENERIC_RP_SIZE, opcode);
	bt_hci_recv(dev, evt);
}

static int driver_open(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static __maybe_unused int driver_close(const struct device *dev)
{
	ARG_UNUSED(dev);

	close_count++;

	return close_err;
}

static int driver_send(const struct device *dev, struct net_buf *buf)
{
	uint8_t type = net_buf_pull_u8(buf);

	zassert_equal(type, BT_HCI_H4_CMD, "Unexpected buffer type %u", type);
	if (blocks) {
		/* The command timeout of the Host is 10 s. */
		k_sleep(K_SECONDS(15));
	} else if (!silent) {
		cmd_handle(dev, buf);
		silent = stops;
	}
	net_buf_unref(buf);

	return 0;
}

static DEVICE_API(bt_hci, driver_api) = {
	.open = driver_open,
#if !defined(CONFIG_TEST_DRIVER_NO_CLOSE)
	.close = driver_close,
#endif
	.send = driver_send,
};

#define TEST_DEVICE_INIT(inst)                                                                     \
	static struct bt_hci_driver_data driver_data_##inst = {0};                                 \
	static const struct bt_hci_driver_config driver_config_##inst =                            \
		BT_DT_HCI_DRIVER_CONFIG_INST_GET(inst);                                            \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, &driver_data_##inst, &driver_config_##inst,        \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &driver_api)

DT_INST_FOREACH_STATUS_OKAY(TEST_DEVICE_INIT)

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	/* The Host asserts in the tests where the controller stops or the
	 * driver blocks, which leaves it in the middle of bt_enable(), so each
	 * of those tests runs alone.
	 */
	if (IS_ENABLED(CONFIG_TEST_CTLR_STOPS) || IS_ENABLED(CONFIG_TEST_SEND_BLOCKS)) {
		ztest_test_skip();
	}

	close_err = 0;
	silent = false;
	if (!bt_is_ready()) {
		zassert_ok(bt_enable(NULL), "Bluetooth init failed");
	}

	reset_count = 0U;
	close_count = 0U;
}

ZTEST_SUITE(bt_disable, NULL, NULL, before, NULL, NULL);

/* The response to a command is handed to the sender in the command's own
 * buffer, which is as large as the events that the Host is configured to
 * receive. A longer response must fail the command instead of being written
 * past that buffer, and the commands that follow must work.
 */
static ZTEST(bt_disable, test_command_response_too_long)
{
	struct bt_hci_rp_read_bd_addr *rp;
	struct net_buf *rsp = NULL;
	int err;

	err = bt_hci_cmd_send_sync(TEST_OP_LONG_RSP, NULL, &rsp);
	zassert_equal(err, -EIO, "Unexpected result %d for a response that is too long", err);
	zassert_is_null(rsp, "A response was handed over");

	err = bt_hci_cmd_send_sync(BT_HCI_OP_READ_BD_ADDR, NULL, &rsp);
	zassert_ok(err, "The next command failed (err %d)", err);
	zassert_equal(rsp->len, sizeof(*rp), "Unexpected response length %u", rsp->len);
	rp = (void *)rsp->data;
	zassert_equal(rp->bdaddr.val[0], 0x11, "Unexpected response contents");
	net_buf_unref(rsp);
}

/* A driver without a close() op cannot be disabled, and bt_disable() has to
 * find that out before it tears anything down: the controller must not have
 * been reset under a Host that goes on saying it is ready.
 */
static ZTEST(bt_disable, test_driver_without_close_op)
{
	int err;

	if (!IS_ENABLED(CONFIG_TEST_DRIVER_NO_CLOSE)) {
		ztest_test_skip();
	}

	err = bt_disable();
	zassert_equal(err, -ENOSYS, "bt_disable() gave %d (!= %d)", err, -ENOSYS);
	zassert_equal(reset_count, 0U, "The controller was reset %u times", reset_count);
	zassert_true(bt_is_ready(), "Bluetooth is no longer ready");
}

/* When the driver fails to close its transport the controller has already
 * been reset and the connections are gone, so the Host must not claim to be
 * ready again. The disable can be retried, after which Bluetooth can be
 * enabled again.
 */
static ZTEST(bt_disable, test_close_failure)
{
	int err;

	if (IS_ENABLED(CONFIG_TEST_DRIVER_NO_CLOSE)) {
		ztest_test_skip();
	}

	close_err = -EIO;
	err = bt_disable();
	zassert_equal(err, -EIO, "bt_disable() gave %d (!= %d)", err, -EIO);
	zassert_equal(close_count, 1U, "close() was called %u times", close_count);
	zassert_equal(reset_count, 1U, "The controller was reset %u times", reset_count);
	zassert_false(bt_is_ready(), "Bluetooth claims to be ready after a failed disable");

	close_err = 0;
	zassert_ok(bt_disable(), "Retrying the disable failed");
	zassert_false(bt_is_ready(), "Bluetooth is ready after a disable");

	zassert_ok(bt_enable(NULL), "Bluetooth init after a disable failed");
	zassert_true(bt_is_ready(), "Bluetooth is not ready after init");
}

static ZTEST(bt_disable, test_enable_disable_cycle)
{
	if (IS_ENABLED(CONFIG_TEST_DRIVER_NO_CLOSE)) {
		ztest_test_skip();
	}

	zassert_ok(bt_disable(), "Bluetooth disable failed");
	zassert_equal(close_count, 1U, "close() was called %u times", close_count);
	zassert_false(bt_is_ready(), "Bluetooth is ready after a disable");

	zassert_ok(bt_enable(NULL), "Bluetooth init after a disable failed");
	zassert_true(bt_is_ready(), "Bluetooth is not ready after init");
}

/* A controller that never responds makes bt_enable() fail, instead of the
 * Host asserting on the timeout of its first command. The transport is closed
 * again, so Bluetooth can be enabled once the controller responds.
 */
static ZTEST(bt_disable, test_enable_silent_controller)
{
	int err;

	if (IS_ENABLED(CONFIG_TEST_DRIVER_NO_CLOSE)) {
		ztest_test_skip();
	}

	zassert_ok(bt_disable(), "Bluetooth disable failed");

	silent = true;
	err = bt_enable(NULL);
	zassert_equal(err, -ETIMEDOUT, "bt_enable() gave %d (!= %d)", err, -ETIMEDOUT);
	zassert_equal(close_count, 2U, "close() was called %u times", close_count);
	zassert_false(bt_is_ready(), "Bluetooth is ready without a controller");

	silent = false;
	zassert_ok(bt_enable(NULL), "Bluetooth init after a silent controller failed");
	zassert_true(bt_is_ready(), "Bluetooth is not ready after init");
}

static K_SEM_DEFINE(ready_sem, 0, 1);
static int ready_err;

static void ready_cb(int err)
{
	ready_err = err;
	k_sem_give(&ready_sem);
}

/* As above, with the initialization and the close of the transport done on
 * the system work queue.
 */
static ZTEST(bt_disable, test_enable_silent_controller_cb)
{
	if (IS_ENABLED(CONFIG_TEST_DRIVER_NO_CLOSE)) {
		ztest_test_skip();
	}

	zassert_ok(bt_disable(), "Bluetooth disable failed");

	silent = true;
	zassert_ok(bt_enable(ready_cb), "bt_enable() failed");
	zassert_ok(k_sem_take(&ready_sem, K_SECONDS(30)), "The ready callback was not called");
	zassert_equal(ready_err, -ETIMEDOUT, "The ready callback gave %d (!= %d)", ready_err,
		      -ETIMEDOUT);
	zassert_equal(close_count, 2U, "close() was called %u times", close_count);
	zassert_false(bt_is_ready(), "Bluetooth is ready without a controller");

	silent = false;
	zassert_ok(bt_enable(NULL), "Bluetooth init after a silent controller failed");
	zassert_true(bt_is_ready(), "Bluetooth is not ready after init");
}

static K_THREAD_STACK_DEFINE(enable_stack, CONFIG_ZTEST_STACK_SIZE);
static struct k_thread enable_thread;
static bool enable_returned;

static void enable_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	ztest_set_fault_valid(true);
	(void)bt_enable(NULL);
	enable_returned = true;
}

/* Call bt_enable() in a thread of its own, which the assert aborts, and check
 * that the Host asserted without closing the transport.
 */
static void enable_until_assert(void)
{
	k_thread_create(&enable_thread, enable_stack, K_THREAD_STACK_SIZEOF(enable_stack),
			enable_entry, NULL, NULL, NULL, CONFIG_ZTEST_THREAD_PRIORITY, 0, K_NO_WAIT);
	zassert_ok(k_thread_join(&enable_thread, K_FOREVER), "Joining the enable thread failed");

	zassert_false(enable_returned, "bt_enable() returned instead of asserting");
	zassert_equal(close_count, 0U, "close() was called %u times", close_count);
}

/* A controller that answers HCI Reset and then stops responding is there but
 * broken, so the Host asserts on the timeout instead of failing bt_enable().
 */
static ZTEST(bt_ctlr_stops, test_enable_ctlr_stops)
{
	if (!IS_ENABLED(CONFIG_TEST_CTLR_STOPS)) {
		ztest_test_skip();
	}

	stops = true;
	enable_until_assert();
	zassert_equal(reset_count, 1U, "The controller was reset %u times", reset_count);
}

ZTEST_SUITE(bt_ctlr_stops, NULL, NULL, NULL, NULL, NULL);

/* The transport must not be closed while the driver is in send(), so when the
 * command times out during a send, the Host asserts instead of failing
 * bt_enable().
 */
static ZTEST(bt_send_blocks, test_enable_send_blocks)
{
	if (!IS_ENABLED(CONFIG_TEST_SEND_BLOCKS)) {
		ztest_test_skip();
	}

	blocks = true;
	enable_until_assert();
}

ZTEST_SUITE(bt_send_blocks, NULL, NULL, NULL, NULL, NULL);
