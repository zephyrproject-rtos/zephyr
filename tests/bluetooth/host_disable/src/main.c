/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
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

#define DT_DRV_COMPAT zephyr_bt_hci_test

/* Large enough for the return parameters of initialization commands that are
 * irrelevant to these tests.
 */
#define GENERIC_RP_SIZE 64U

/* The data packets the controller takes: the smallest size an LE controller
 * may have, and a few of them.
 */
#define LE_ACL_MAX_LEN 27U
#define LE_ACL_MAX_NUM 3U

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

static void le_read_buffer_size(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				uint16_t opcode)
{
	struct bt_hci_rp_le_read_buffer_size *rp;

	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	rp->le_max_len = sys_cpu_to_le16(LE_ACL_MAX_LEN);
	rp->le_max_num = LE_ACL_MAX_NUM;
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

static const struct cmd_handler cmds[] = {
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
		BT_HCI_OP_LE_READ_BUFFER_SIZE,
		sizeof(struct bt_hci_rp_le_read_buffer_size),
		le_read_buffer_size,
	},
	{
		BT_HCI_OP_LE_READ_SUPP_STATES,
		sizeof(struct bt_hci_rp_le_read_supp_states),
		le_read_supp_states,
	},
};

/* What the test looks at: how often the controller has been reset, how often
 * it has been told to disconnect, how many data packets it has been given, and
 * what the driver's close() is to return.
 */
static unsigned int reset_count;
static unsigned int disconnect_count;
static unsigned int data_count;
static unsigned int close_count;
static int close_err;

/* What the driver's send() is to return for a data packet */
static int data_err;

/* How long the controller takes to respond to a reset, as one at the other end
 * of a UART or USB link does.
 */
static k_timeout_t reset_rsp_delay;

static void reset_rsp_handler(struct k_work *work)
{
	const struct device *dev = DEVICE_DT_GET(DT_DRV_INST(0));
	struct net_buf *evt = NULL;

	ARG_UNUSED(work);

	generic_success(NULL, &evt, GENERIC_RP_SIZE, BT_HCI_OP_RESET);
	bt_hci_recv(dev, evt);
}

static K_WORK_DELAYABLE_DEFINE(reset_rsp_work, reset_rsp_handler);

/* How long the controller takes to respond to a disconnect command, what it
 * responds with, and the connection that the command was for.
 */
static k_timeout_t disconnect_rsp_delay;
static uint8_t disconnect_status;
static uint16_t disconnect_handle;

/* Whether the transport was still open when the disconnect command arrived */
static bool disconnect_before_close;

/* The controller accepts the command and the connection is gone, unless it is
 * to reject the command.
 */
static void disconnect_rsp_handler(struct k_work *work)
{
	const struct device *dev = DEVICE_DT_GET(DT_DRV_INST(0));
	struct bt_hci_evt_disconn_complete *complete;
	struct bt_hci_evt_cmd_status *status;
	struct net_buf *evt;

	ARG_UNUSED(work);

	evt = bt_buf_get_evt(BT_HCI_EVT_CMD_STATUS, false, K_FOREVER);
	evt_create(evt, BT_HCI_EVT_CMD_STATUS, sizeof(*status));
	status = net_buf_add(evt, sizeof(*status));
	status->status = disconnect_status;
	status->ncmd = 1U;
	status->opcode = sys_cpu_to_le16(BT_HCI_OP_DISCONNECT);
	bt_hci_recv(dev, evt);

	if (disconnect_status != BT_HCI_ERR_SUCCESS) {
		return;
	}

	evt = bt_buf_get_evt(BT_HCI_EVT_DISCONN_COMPLETE, false, K_FOREVER);
	evt_create(evt, BT_HCI_EVT_DISCONN_COMPLETE, sizeof(*complete));
	complete = net_buf_add(evt, sizeof(*complete));
	complete->status = BT_HCI_ERR_SUCCESS;
	complete->handle = disconnect_handle;
	complete->reason = BT_HCI_ERR_LOCALHOST_TERM_CONN;
	bt_hci_recv(dev, evt);
}

static K_WORK_DELAYABLE_DEFINE(disconnect_rsp_work, disconnect_rsp_handler);

static void cmd_handle(const struct device *dev, struct net_buf *cmd)
{
	struct net_buf *evt = NULL;
	struct bt_hci_cmd_hdr *chdr;
	uint16_t opcode;

	chdr = net_buf_pull_mem(cmd, sizeof(*chdr));
	opcode = sys_le16_to_cpu(chdr->opcode);

	if (opcode == BT_HCI_OP_DISCONNECT) {
		const struct bt_hci_cp_disconnect *cp = (const void *)cmd->data;

		disconnect_count++;
		disconnect_handle = cp->handle;
		disconnect_before_close = (close_count == 0U);

		/* K_FOREVER: the response does not make it before the transport
		 * is closed, after which the driver delivers nothing.
		 */
		if (!K_TIMEOUT_EQ(disconnect_rsp_delay, K_FOREVER)) {
			(void)k_work_schedule(&disconnect_rsp_work, disconnect_rsp_delay);
		}

		return;
	}

	if (opcode == BT_HCI_OP_RESET) {
		reset_count++;

		if (!K_TIMEOUT_EQ(reset_rsp_delay, K_NO_WAIT)) {
			(void)k_work_schedule(&reset_rsp_work, reset_rsp_delay);
			return;
		}
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

	if (type == BT_HCI_H4_CMD) {
		cmd_handle(dev, buf);
	} else {
		data_count++;

		if (data_err != 0) {
			/* The buffer stays with the caller */
			return data_err;
		}
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

	close_err = 0;
	data_err = 0;
	reset_rsp_delay = K_NO_WAIT;
	disconnect_rsp_delay = K_NO_WAIT;
	disconnect_status = BT_HCI_ERR_SUCCESS;
	if (!bt_is_ready()) {
		zassert_ok(bt_enable(NULL), "Bluetooth init failed");
	}

	reset_count = 0U;
	disconnect_count = 0U;
	data_count = 0U;
	close_count = 0U;
}

ZTEST_SUITE(bt_disable, NULL, NULL, before, NULL, NULL);

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

#if defined(CONFIG_BT_PERIPHERAL) && defined(CONFIG_BT_GATT_CLIENT)
#define TEST_CONN_HANDLE 0x0001U

static K_SEM_DEFINE(connected_sem, 0, 1);
static K_SEM_DEFINE(disconnected_sem, 0, 1);
static struct bt_conn *test_conn;

static void connected(struct bt_conn *conn, uint8_t err)
{
	zassert_equal(err, 0U, "Connection failed (0x%02x)", err);

	test_conn = bt_conn_ref(conn);
	k_sem_give(&connected_sem);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(reason);

	k_sem_give(&disconnected_sem);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static void connect_as_peripheral(void)
{
	const struct device *dev = DEVICE_DT_GET(DT_DRV_INST(0));
	const bt_addr_le_t peer = {
		.type = BT_ADDR_LE_RANDOM,
		.a.val = {0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0xc6U},
	};
	struct bt_hci_evt_le_conn_complete *evt;
	struct bt_hci_evt_le_meta_event *meta;
	struct net_buf *buf;

	zassert_ok(bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, NULL, 0U, NULL, 0U),
		   "Advertising failed to start");

	buf = bt_buf_get_evt(BT_HCI_EVT_LE_META_EVENT, false, K_FOREVER);
	evt_create(buf, BT_HCI_EVT_LE_META_EVENT, sizeof(*meta) + sizeof(*evt));
	meta = net_buf_add(buf, sizeof(*meta));
	meta->subevent = BT_HCI_EVT_LE_CONN_COMPLETE;
	evt = net_buf_add(buf, sizeof(*evt));
	(void)memset(evt, 0, sizeof(*evt));
	evt->status = BT_HCI_ERR_SUCCESS;
	evt->handle = sys_cpu_to_le16(TEST_CONN_HANDLE);
	evt->role = BT_HCI_ROLE_PERIPHERAL;
	bt_addr_le_copy(&evt->peer_addr, &peer);
	evt->interval = sys_cpu_to_le16(BT_GAP_INIT_CONN_INT_MIN);
	evt->supv_timeout = sys_cpu_to_le16(BT_GAP_MS_TO_CONN_TIMEOUT(4000U));
	bt_hci_recv(dev, buf);

	zassert_ok(k_sem_take(&connected_sem, K_SECONDS(1)), "No connection");
}

static void mtu_exchanged(struct bt_conn *conn, uint8_t err, struct bt_gatt_exchange_params *params)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(err);
	ARG_UNUSED(params);
}

/* Whether the request was sent while bt_disable() was waiting for the reset */
static bool sent_during_reset;

static void send_handler(struct k_work *work)
{
	static struct bt_gatt_exchange_params params = {
		.func = mtu_exchanged,
	};

	bool before;

	ARG_UNUSED(work);

	before = !bt_is_ready() && k_work_delayable_is_pending(&reset_rsp_work);

	zassert_ok(bt_gatt_exchange_mtu(test_conn, &params), "Failed to queue a request");

	sent_during_reset =
		before && !bt_is_ready() && k_work_delayable_is_pending(&reset_rsp_work);
}

static K_WORK_DELAYABLE_DEFINE(send_work, send_handler);

/* An application that keeps sending while bt_disable() waits for the controller
 * to be reset has its data held back, the Host being no longer ready. Nothing
 * of it may reach the controller, and the Host must not disconnect the
 * connection, which bt_disable() is about to take down anyway: the command
 * would be issued by the thread that transmits commands, which cannot wait for
 * one of its own.
 */
static ZTEST(bt_disable, test_data_sent_during_disable)
{
	int64_t start;

	if (IS_ENABLED(CONFIG_TEST_DRIVER_NO_CLOSE)) {
		ztest_test_skip();
	}

	connect_as_peripheral();

	sent_during_reset = false;
	reset_rsp_delay = K_MSEC(20);
	(void)k_work_schedule(&send_work, K_MSEC(10));

	start = k_uptime_get();
	zassert_ok(bt_disable(), "Bluetooth disable failed");
	zassert_true(k_uptime_get() - start < MSEC_PER_SEC, "bt_disable() took %lld ms",
		     k_uptime_get() - start);

	zassert_true(sent_during_reset, "The request was not sent during the reset");
	zassert_equal(disconnect_count, 0U, "The controller was told to disconnect %u times",
		      disconnect_count);
	zassert_equal(data_count, 0U, "The controller was given %u data packets", data_count);

	bt_conn_unref(test_conn);
	test_conn = NULL;

	/* The connection that was taken down with data queued for it has been
	 * let go of, so that a new one can be had.
	 */
	reset_rsp_delay = K_NO_WAIT;
	zassert_ok(bt_enable(NULL), "Bluetooth init after a disable failed");
	connect_as_peripheral();

	zassert_ok(bt_disable(), "Bluetooth disable failed");
	bt_conn_unref(test_conn);
	test_conn = NULL;
}

/* The application disconnects from a thread of its own. It cannot be the
 * system workqueue, which delivers the delayed responses of the controller here
 * and must not be held up by a call that waits for one.
 */
static K_THREAD_STACK_DEFINE(app_stack, 1024);
static struct k_work_q app_workq;
static K_SEM_DEFINE(disconnect_returned, 0, 1);
static int disconnect_err;
static int64_t disconnect_return_time;

static void disconnect_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	disconnect_err = bt_conn_disconnect(test_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	disconnect_return_time = k_uptime_get();
	k_sem_give(&disconnect_returned);
}

static K_WORK_DELAYABLE_DEFINE(disconnect_work, disconnect_handler);

/* A command that another thread sends while bt_disable() waits for the reset is
 * transmitted once the controller has responded to the reset, and is on its way
 * when bt_disable() closes the transport. Its sender must be told that it
 * failed, there and then: the response that it waits for cannot come.
 */
static ZTEST(bt_disable, test_command_in_flight_at_close)
{
	static bool started;
	int64_t start;

	if (IS_ENABLED(CONFIG_TEST_DRIVER_NO_CLOSE)) {
		ztest_test_skip();
	}

	if (!started) {
		k_work_queue_init(&app_workq);
		k_work_queue_start(&app_workq, app_stack, K_THREAD_STACK_SIZEOF(app_stack),
				   K_PRIO_PREEMPT(1), NULL);
		started = true;
	}

	connect_as_peripheral();

	reset_rsp_delay = K_MSEC(20);
	disconnect_rsp_delay = K_FOREVER;
	disconnect_before_close = false;
	k_sem_reset(&disconnect_returned);
	(void)k_work_schedule_for_queue(&app_workq, &disconnect_work, K_MSEC(10));

	start = k_uptime_get();
	zassert_ok(bt_disable(), "Bluetooth disable failed");

	zassert_equal(disconnect_count, 1U, "The controller got %u disconnect commands",
		      disconnect_count);
	zassert_true(disconnect_before_close,
		     "The disconnect command was sent after the transport was closed");

	zassert_ok(k_sem_take(&disconnect_returned, K_SECONDS(1)),
		   "bt_conn_disconnect() did not return");
	zassert_true(disconnect_return_time - start < MSEC_PER_SEC,
		     "bt_conn_disconnect() returned after %lld ms", disconnect_return_time - start);
	zassert_equal(disconnect_err, -EIO, "bt_conn_disconnect() gave %d (!= %d)", disconnect_err,
		      -EIO);

	bt_conn_unref(test_conn);
	test_conn = NULL;

	/* Bluetooth comes up again and connects */
	reset_rsp_delay = K_NO_WAIT;
	disconnect_rsp_delay = K_NO_WAIT;
	zassert_ok(bt_enable(NULL), "Bluetooth init after a disable failed");
	connect_as_peripheral();

	zassert_ok(bt_disable(), "Bluetooth disable failed");
	bt_conn_unref(test_conn);
	test_conn = NULL;
}

ZTEST_SUITE(bt_conn_tx, NULL, NULL, before, NULL, NULL);

/* When the driver fails to send a data packet the Host gives the connection up.
 * It finds that out in the TX processor, which is what transmits the command
 * that disconnects, and so cannot be what waits for it. Nothing more is sent on
 * the connection while the controller takes its time to respond to the command.
 */
static ZTEST(bt_conn_tx, test_data_send_failure)
{
	const uint8_t value = 0U;

	if (IS_ENABLED(CONFIG_TEST_DRIVER_NO_CLOSE)) {
		ztest_test_skip();
	}

	connect_as_peripheral();
	(void)k_sem_take(&disconnected_sem, K_NO_WAIT);

	data_err = -EIO;
	disconnect_rsp_delay = K_MSEC(20);
	zassert_ok(bt_gatt_write_without_response(test_conn, 1U, &value, sizeof(value), false),
		   "Failed to queue the first packet");

	k_sleep(K_MSEC(10));
	zassert_equal(data_count, 1U, "The driver was given %u data packets", data_count);
	zassert_equal(disconnect_count, 1U, "The controller was told to disconnect %u times",
		      disconnect_count);

	zassert_ok(bt_gatt_write_without_response(test_conn, 1U, &value, sizeof(value), false),
		   "Failed to queue the second packet");

	zassert_ok(k_sem_take(&disconnected_sem, K_SECONDS(1)), "No disconnection");
	zassert_equal(data_count, 1U, "The driver was given %u data packets", data_count);
	zassert_equal(disconnect_count, 1U, "The controller was told to disconnect %u times",
		      disconnect_count);

	bt_conn_unref(test_conn);
	test_conn = NULL;
}

/* A connection that the controller refuses to disconnect stays, and goes on
 * sending what was held back while the Host tried.
 */
static ZTEST(bt_conn_tx, test_data_send_failure_disconnect_rejected)
{
	const uint8_t value = 0U;
	struct bt_conn_info info;

	if (IS_ENABLED(CONFIG_TEST_DRIVER_NO_CLOSE)) {
		ztest_test_skip();
	}

	connect_as_peripheral();

	data_err = -EIO;
	disconnect_rsp_delay = K_MSEC(20);
	disconnect_status = BT_HCI_ERR_CMD_DISALLOWED;
	zassert_ok(bt_gatt_write_without_response(test_conn, 1U, &value, sizeof(value), false),
		   "Failed to queue the first packet");

	k_sleep(K_MSEC(10));
	zassert_equal(data_count, 1U, "The driver was given %u data packets", data_count);
	zassert_equal(disconnect_count, 1U, "The controller was told to disconnect %u times",
		      disconnect_count);

	/* The driver works again */
	data_err = 0;
	zassert_ok(bt_gatt_write_without_response(test_conn, 1U, &value, sizeof(value), false),
		   "Failed to queue the second packet");
	zassert_equal(data_count, 1U, "The driver was given %u data packets", data_count);

	k_sleep(K_MSEC(20));
	zassert_equal(data_count, 2U, "The driver was given %u data packets", data_count);
	zassert_equal(disconnect_count, 1U, "The controller was told to disconnect %u times",
		      disconnect_count);
	zassert_ok(bt_conn_get_info(test_conn, &info), "No connection info");
	zassert_equal(info.state, BT_CONN_STATE_CONNECTED, "The connection is in state %u",
		      info.state);

	zassert_ok(bt_disable(), "Bluetooth disable failed");
	bt_conn_unref(test_conn);
	test_conn = NULL;
}
#endif /* CONFIG_BT_PERIPHERAL && CONFIG_BT_GATT_CLIENT */
