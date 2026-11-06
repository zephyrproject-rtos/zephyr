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
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/irq_offload.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_error_hook.h>

#include "host/hci_core.h"
#include "host/settings.h"

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

/* A vendor command whose response is longer than the Host is configured to
 * receive, although it fits an HCI event.
 */
#define TEST_OP_LONG_RSP BT_OP(BT_OGF_VS, 0x03ff)
#define LONG_RSP_SIZE    200U

BUILD_ASSERT(IS_ENABLED(CONFIG_BT_EXT_ADV) || LONG_RSP_SIZE > CONFIG_BT_BUF_EVT_RX_SIZE);

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
 * what the driver's close() is to return. A silent controller drops every
 * command, and a controller that stops goes silent once it has answered a
 * command. A driver that blocks stays in send() for longer than the command
 * timeout.
 */
static unsigned int reset_count;
static unsigned int disconnect_count;
static unsigned int data_count;
static unsigned int cmd_count;
static unsigned int close_count;
static int close_err;
static bool silent;
static bool stops;
static bool blocks;

/* The driver reports its controller gone in the middle of sending a command */
static bool lost_in_send;

/* What send() returns for that command */
static int lost_in_send_err;

/* The driver reports its controller gone in the middle of opening the transport */
static bool lost_in_open;

/* What the driver's open() is to return */
static int open_err;

/* What close_count was when open() returned */
static unsigned int open_close_count;

static K_SEM_DEFINE(available_sem, 0, 1);

/* The driver reports its controller available in the middle of a close */
static bool available_in_close;

/* Whether the application was told while the close went on */
static bool available_told_in_close;

/* What the driver's send() is to return for a data packet */
static int data_err;
static int cmd_send_err;

/* How long the controller takes to respond to a reset, as one at the other end
 * of a UART or USB link does.
 */
static k_timeout_t reset_rsp_delay;

/* The status that the controller responds to a reset with */
static uint8_t reset_status;

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
 * responds with, and the connection that the command was for. The controller
 * can also report that connection as disconnected when the transport is opened,
 * as one that open() does not reset does for a connection of before, or while
 * the transport is being closed, as one that has not been reset does.
 */
static k_timeout_t disconnect_rsp_delay;
static uint8_t disconnect_status;
static uint16_t disconnect_handle;
static bool disconnect_at_open;
static bool disconnect_at_close;

/* The controller delivers an advertising report when the transport is opened,
 * as one that open() does not reset does while it goes on scanning.
 */
static bool adv_report_at_open;

/* Whether the transport was still open when the disconnect command arrived */
static bool disconnect_before_close;

static void disconn_complete_recv(const struct device *dev)
{
	struct bt_hci_evt_disconn_complete *complete;
	struct net_buf *evt;

	evt = bt_buf_get_evt(BT_HCI_EVT_DISCONN_COMPLETE, false, K_FOREVER);
	evt_create(evt, BT_HCI_EVT_DISCONN_COMPLETE, sizeof(*complete));
	complete = net_buf_add(evt, sizeof(*complete));
	complete->status = BT_HCI_ERR_SUCCESS;
	complete->handle = disconnect_handle;
	complete->reason = BT_HCI_ERR_LOCALHOST_TERM_CONN;
	bt_hci_recv(dev, evt);
}

/* The controller accepts the command and the connection is gone, unless it is
 * to reject the command.
 */
static void disconnect_rsp_handler(struct k_work *work)
{
	const struct device *dev = DEVICE_DT_GET(DT_DRV_INST(0));
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

	disconn_complete_recv(dev);
}

static K_WORK_DELAYABLE_DEFINE(disconnect_rsp_work, disconnect_rsp_handler);

/* The Host Number Of Completed Packets commands that the controller has been
 * given: how many, and the first entry of each of the first few of them.
 */
#define HOST_NCP_LOG_SIZE 4U

struct host_ncp {
	uint8_t num_handles;
	uint16_t handle;
	uint16_t count;
};

static struct host_ncp host_ncp_log[HOST_NCP_LOG_SIZE];
static atomic_t host_ncp_num;
static K_SEM_DEFINE(host_ncp_sem, 0, K_SEM_MAX_LIMIT);

static void host_ncp_handle(const struct net_buf *cmd)
{
	const struct bt_hci_cp_host_num_completed_packets *cp = (const void *)cmd->data;
	atomic_val_t num = atomic_inc(&host_ncp_num);

	if (num < HOST_NCP_LOG_SIZE) {
		host_ncp_log[num].num_handles = cp->num_handles;
		host_ncp_log[num].handle = sys_le16_to_cpu(cp->h[0].handle);
		host_ncp_log[num].count = sys_le16_to_cpu(cp->h[0].count);
	}

	k_sem_give(&host_ncp_sem);
}

static void cmd_handle(const struct device *dev, struct net_buf *cmd)
{
	struct net_buf *evt = NULL;
	struct bt_hci_cmd_hdr *chdr;
	uint16_t opcode;

	chdr = net_buf_pull_mem(cmd, sizeof(*chdr));
	opcode = sys_le16_to_cpu(chdr->opcode);

	if (opcode == BT_HCI_OP_HOST_NUM_COMPLETED_PACKETS) {
		/* The controller does not respond to this command */
		host_ncp_handle(cmd);
		return;
	}

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

		if (reset_status != BT_HCI_ERR_SUCCESS) {
			struct bt_hci_evt_cc_status *rp;

			rp = cmd_complete(&evt, sizeof(*rp), opcode);
			rp->status = reset_status;
			bt_hci_recv(dev, evt);
			return;
		}

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

static void adv_report_recv(const struct device *dev)
{
	struct bt_hci_evt_le_advertising_info *info;
	struct bt_hci_evt_le_meta_event *meta;
	struct net_buf *buf;

	buf = bt_buf_get_evt(BT_HCI_EVT_LE_META_EVENT, false, K_FOREVER);
	evt_create(buf, BT_HCI_EVT_LE_META_EVENT, sizeof(*meta) + 2U + sizeof(*info));
	meta = net_buf_add(buf, sizeof(*meta));
	meta->subevent = BT_HCI_EVT_LE_ADVERTISING_REPORT;
	net_buf_add_u8(buf, 1U);
	info = net_buf_add(buf, sizeof(*info));
	(void)memset(info, 0, sizeof(*info));
	info->evt_type = BT_GAP_ADV_TYPE_ADV_NONCONN_IND;
	info->addr.type = BT_ADDR_LE_RANDOM;
	net_buf_add_u8(buf, 0U);
	bt_hci_recv(dev, buf);
}

static int driver_open(const struct device *dev)
{
	if (disconnect_at_open) {
		disconn_complete_recv(dev);
	}

	if (adv_report_at_open) {
		adv_report_recv(dev);
	}

	if (IS_ENABLED(CONFIG_TEST_DRIVER_STATE_REPORT) && lost_in_open) {
		lost_in_open = false;
		cmd_send_err = -EIO;
		bt_hci_report_unavailable(dev);

		/* As an open that waits for the controller gets to do. The
		 * Host must not close the transport in the meantime.
		 */
		k_sleep(K_MSEC(10));
		open_close_count = close_count;
	}

	return open_err;
}

static __maybe_unused int driver_close(const struct device *dev)
{
	close_count++;

	if (disconnect_at_close) {
		disconn_complete_recv(dev);
	}

	if (IS_ENABLED(CONFIG_TEST_DRIVER_STATE_REPORT) && available_in_close) {
		available_in_close = false;
		bt_hci_report_available(dev);

		k_sleep(K_MSEC(10));
		available_told_in_close = k_sem_count_get(&available_sem) != 0U;
	}

	return close_err;
}

static int driver_send(const struct device *dev, struct net_buf *buf)
{
	uint8_t type = net_buf_pull_u8(buf);

	if (type == BT_HCI_H4_CMD) {
		cmd_count++;
	}

	if (type != BT_HCI_H4_CMD) {
		data_count++;

		if (data_err != 0) {
			/* The buffer stays with the caller */
			return data_err;
		}
	} else if (cmd_send_err != 0) {
		/* The buffer stays with the caller */
		return cmd_send_err;
	} else if (IS_ENABLED(CONFIG_TEST_DRIVER_STATE_REPORT) && lost_in_send) {
		lost_in_send = false;
		cmd_send_err = -EIO;
		bt_hci_report_unavailable(dev);

		if (lost_in_send_err != 0) {
			return lost_in_send_err;
		}
	} else if (blocks) {
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

	/* Not ready before the settings are loaded, which has its own suite */
	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		ztest_test_skip();
	}

	close_err = 0;
	silent = false;
	data_err = 0;
	cmd_send_err = 0;
	lost_in_send = false;
	lost_in_send_err = 0;
	lost_in_open = false;
	open_err = 0;
	available_in_close = false;
	reset_rsp_delay = K_NO_WAIT;
	reset_status = BT_HCI_ERR_SUCCESS;
	disconnect_rsp_delay = K_NO_WAIT;
	disconnect_status = BT_HCI_ERR_SUCCESS;
	disconnect_at_open = false;
	disconnect_at_close = false;
	adv_report_at_open = false;
	if (!bt_is_ready()) {
		zassert_ok(bt_enable(NULL), "Bluetooth init failed");
	}

	reset_count = 0U;
	disconnect_count = 0U;
	data_count = 0U;
	cmd_count = 0U;
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

	/* The commands of extended advertising make the buffers long enough */
	if (IS_ENABLED(CONFIG_BT_EXT_ADV)) {
		ztest_test_skip();
	}

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

/* bt_disable() with a controller that cannot be reset closes the transport and
 * says so.
 */
static void disable_reset_fails(void)
{
	int err = bt_disable();

	zassert_equal(err, -EHOSTDOWN, "bt_disable() gave %d (!= %d)", err, -EHOSTDOWN);
}

/* A controller that fails the reset does not keep Bluetooth from being
 * disabled: the Host has torn down its advertising sets by then, so it goes on
 * and closes the transport instead of claiming to be ready again. The caller is
 * told, as the controller may go on advertising, and bt_enable() resets the
 * controller once more.
 */
static ZTEST(bt_disable, test_reset_failure)
{
	int err;

	if (IS_ENABLED(CONFIG_TEST_DRIVER_NO_CLOSE)) {
		ztest_test_skip();
	}

	zassert_ok(bt_le_adv_start(BT_LE_ADV_NCONN, NULL, 0U, NULL, 0U),
		   "Advertising failed to start");

	reset_status = BT_HCI_ERR_HW_FAILURE;
	disable_reset_fails();
	zassert_equal(reset_count, 1U, "The controller was reset %u times", reset_count);
	zassert_equal(close_count, 1U, "close() was called %u times", close_count);
	zassert_false(bt_is_ready(), "Bluetooth is ready after a disable");

	err = bt_disable();
	zassert_equal(err, -EALREADY, "bt_disable() gave %d (!= %d)", err, -EALREADY);

	reset_status = BT_HCI_ERR_SUCCESS;
	zassert_ok(bt_enable(NULL), "Bluetooth init after a disable failed");
	zassert_equal(reset_count, 2U, "The controller was reset %u times", reset_count);
	zassert_true(bt_is_ready(), "Bluetooth is not ready after init");
}

/* A driver that has lost its controller can fail every command. The reset then
 * fails without reaching the controller, and that ends as when the controller
 * fails it.
 */
static ZTEST(bt_disable, test_reset_send_failure)
{
	int err;

	if (IS_ENABLED(CONFIG_TEST_DRIVER_NO_CLOSE)) {
		ztest_test_skip();
	}

	cmd_send_err = -EIO;

	err = bt_le_adv_start(BT_LE_ADV_NCONN, NULL, 0U, NULL, 0U);
	zassert_equal(err, -EIO, "A command that cannot be sent gave %d (!= %d)", err, -EIO);

	disable_reset_fails();
	zassert_equal(reset_count, 0U, "The controller was reset %u times", reset_count);
	zassert_equal(close_count, 1U, "close() was called %u times", close_count);
	zassert_false(bt_is_ready(), "Bluetooth is ready after a disable");

	cmd_send_err = 0;
	zassert_ok(bt_enable(NULL), "Bluetooth init after a disable failed");
	zassert_equal(reset_count, 1U, "The controller was reset %u times", reset_count);
	zassert_true(bt_is_ready(), "Bluetooth is not ready after init");
}

/* A failed close goes before a failed reset: Bluetooth is not disabled yet, and
 * the disable can be tried again.
 */
static ZTEST(bt_disable, test_reset_failure_close_failure)
{
	int err;

	if (IS_ENABLED(CONFIG_TEST_DRIVER_NO_CLOSE)) {
		ztest_test_skip();
	}

	reset_status = BT_HCI_ERR_HW_FAILURE;
	close_err = -EIO;
	err = bt_disable();
	zassert_equal(err, -EIO, "bt_disable() gave %d (!= %d)", err, -EIO);
	zassert_false(bt_is_ready(), "Bluetooth is ready after a failed disable");

	close_err = 0;
	disable_reset_fails();
	zassert_equal(reset_count, 2U, "The controller was reset %u times", reset_count);
	zassert_equal(close_count, 2U, "close() was called %u times", close_count);

	reset_status = BT_HCI_ERR_SUCCESS;
	zassert_ok(bt_enable(NULL), "Bluetooth init after a disable failed");
	zassert_true(bt_is_ready(), "Bluetooth is not ready after init");
}

/* A controller that fails the reset when Bluetooth is enabled leaves the
 * transport open. bt_disable() closes it although the controller fails the
 * reset again, so that the enable can be tried once more.
 */
static ZTEST(bt_disable, test_enable_reset_failure)
{
	int err;

	if (IS_ENABLED(CONFIG_TEST_DRIVER_NO_CLOSE)) {
		ztest_test_skip();
	}

	zassert_ok(bt_disable(), "Bluetooth disable failed");

	reset_status = BT_HCI_ERR_HW_FAILURE;
	err = bt_enable(NULL);
	zassert_equal(err, -EIO, "bt_enable() gave %d (!= %d)", err, -EIO);
	zassert_false(bt_is_ready(), "Bluetooth is ready after a failed init");

	disable_reset_fails();
	zassert_equal(close_count, 2U, "close() was called %u times", close_count);

	reset_status = BT_HCI_ERR_SUCCESS;
	zassert_ok(bt_enable(NULL), "Bluetooth init after a disable failed");
	zassert_true(bt_is_ready(), "Bluetooth is not ready after init");
}

#if defined(CONFIG_BT_OBSERVER)
static atomic_t scan_report_count;

static void scan_report(const bt_addr_le_t *addr, int8_t rssi, uint8_t adv_type,
			struct net_buf_simple *buf)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(rssi);
	ARG_UNUSED(adv_type);
	ARG_UNUSED(buf);

	atomic_inc(&scan_report_count);
}

/* A controller that could not be reset goes on scanning, and if open() does not
 * reset it either it delivers a report before bt_enable() does. The scan that
 * the report belongs to ended with the disable, and its callback must not run.
 */
static ZTEST(bt_disable, test_scan_report_after_reset_failure)
{
	if (IS_ENABLED(CONFIG_TEST_DRIVER_NO_CLOSE)) {
		ztest_test_skip();
	}

	zassert_ok(bt_le_scan_start(BT_LE_SCAN_PASSIVE, scan_report), "Scanning failed to start");
	atomic_clear(&scan_report_count);

	reset_status = BT_HCI_ERR_HW_FAILURE;
	disable_reset_fails();

	/* The Host goes on to process the report during the reset */
	reset_status = BT_HCI_ERR_SUCCESS;
	reset_rsp_delay = K_MSEC(20);
	adv_report_at_open = true;
	zassert_ok(bt_enable(NULL), "Bluetooth init after a disable failed");

	zassert_equal(atomic_get(&scan_report_count), 0, "The scan callback ran %ld times",
		      (long)atomic_get(&scan_report_count));
}
#endif /* CONFIG_BT_OBSERVER */

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

static K_THREAD_STACK_DEFINE(enable_stack, CONFIG_ZTEST_STACK_SIZE + CONFIG_TEST_EXTRA_STACK_SIZE);
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

/* The controller reports a connection to its advertiser or, with an error
 * status, the end of its directed advertising.
 */
static void conn_complete_recv(uint8_t status)
{
	const struct device *dev = DEVICE_DT_GET(DT_DRV_INST(0));
	const bt_addr_le_t peer = {
		.type = BT_ADDR_LE_RANDOM,
		.a.val = {0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0xc6U},
	};
	struct bt_hci_evt_le_conn_complete *evt;
	struct bt_hci_evt_le_meta_event *meta;
	struct net_buf *buf;

	buf = bt_buf_get_evt(BT_HCI_EVT_LE_META_EVENT, false, K_FOREVER);
	evt_create(buf, BT_HCI_EVT_LE_META_EVENT, sizeof(*meta) + sizeof(*evt));
	meta = net_buf_add(buf, sizeof(*meta));
	meta->subevent = BT_HCI_EVT_LE_CONN_COMPLETE;
	evt = net_buf_add(buf, sizeof(*evt));
	(void)memset(evt, 0, sizeof(*evt));
	evt->status = status;
	evt->handle = sys_cpu_to_le16(TEST_CONN_HANDLE);
	evt->role = BT_HCI_ROLE_PERIPHERAL;
	bt_addr_le_copy(&evt->peer_addr, &peer);
	evt->interval = sys_cpu_to_le16(BT_GAP_INIT_CONN_INT_MIN);
	evt->supv_timeout = sys_cpu_to_le16(BT_GAP_MS_TO_CONN_TIMEOUT(4000U));
	bt_hci_recv(dev, buf);
}

static void connect_as_peripheral(void)
{
	zassert_ok(bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, NULL, 0U, NULL, 0U),
		   "Advertising failed to start");

	conn_complete_recv(BT_HCI_ERR_SUCCESS);

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
static K_THREAD_STACK_DEFINE(app_stack, 1024 + CONFIG_TEST_EXTRA_STACK_SIZE);
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

/* A connection is taken down when the controller fails the reset, as when it
 * does not, and one can be had again once Bluetooth is enabled.
 */
static ZTEST(bt_disable, test_reset_failure_with_conn)
{
	if (IS_ENABLED(CONFIG_TEST_DRIVER_NO_CLOSE)) {
		ztest_test_skip();
	}

	connect_as_peripheral();
	k_sem_reset(&disconnected_sem);

	reset_status = BT_HCI_ERR_HW_FAILURE;
	disable_reset_fails();
	zassert_ok(k_sem_take(&disconnected_sem, K_SECONDS(1)), "No disconnection");

	bt_conn_drop(&test_conn);

	reset_status = BT_HCI_ERR_SUCCESS;
	zassert_ok(bt_enable(NULL), "Bluetooth init after a disable failed");
	connect_as_peripheral();

	zassert_ok(bt_disable(), "Bluetooth disable failed");
	bt_conn_drop(&test_conn);
}

/* A connection is taken down too when the reset cannot be sent. */
static ZTEST(bt_disable, test_reset_send_failure_with_conn)
{
	if (IS_ENABLED(CONFIG_TEST_DRIVER_NO_CLOSE)) {
		ztest_test_skip();
	}

	connect_as_peripheral();
	k_sem_reset(&disconnected_sem);

	cmd_send_err = -EIO;
	data_err = -EIO;
	disable_reset_fails();
	zassert_ok(k_sem_take(&disconnected_sem, K_SECONDS(1)), "No disconnection");

	bt_conn_drop(&test_conn);

	cmd_send_err = 0;
	data_err = 0;
	zassert_ok(bt_enable(NULL), "Bluetooth init after a disable failed");
	connect_as_peripheral();

	zassert_ok(bt_disable(), "Bluetooth disable failed");
	bt_conn_drop(&test_conn);
}

/* A controller that has not been reset can report a disconnection until the
 * transport is closed, after the connection has been torn down. The report must
 * not be taken for the next connection that gets the same handle.
 */
static ZTEST(bt_disable, test_disconnection_at_close)
{
	struct bt_conn_info info;

	if (IS_ENABLED(CONFIG_TEST_DRIVER_NO_CLOSE)) {
		ztest_test_skip();
	}

	connect_as_peripheral();

	reset_status = BT_HCI_ERR_HW_FAILURE;
	disconnect_handle = sys_cpu_to_le16(TEST_CONN_HANDLE);
	disconnect_at_close = true;
	disable_reset_fails();

	bt_conn_drop(&test_conn);

	reset_status = BT_HCI_ERR_SUCCESS;
	disconnect_at_close = false;
	zassert_ok(bt_enable(NULL), "Bluetooth init after a disable failed");
	connect_as_peripheral();

	zassert_ok(bt_conn_get_info(test_conn, &info), "No connection info");
	zassert_equal(info.state, BT_CONN_STATE_CONNECTED, "The connection is in state %u",
		      info.state);

	zassert_ok(bt_disable(), "Bluetooth disable failed");
	bt_conn_drop(&test_conn);
}

/* A controller that open() does not reset can report the disconnection of a
 * connection of before until it is reset. That must not be taken for the next
 * connection that gets the same handle.
 */
static ZTEST(bt_disable, test_disconnection_at_open)
{
	struct bt_conn_info info;

	if (IS_ENABLED(CONFIG_TEST_DRIVER_NO_CLOSE)) {
		ztest_test_skip();
	}

	zassert_ok(bt_disable(), "Bluetooth disable failed");

	disconnect_handle = sys_cpu_to_le16(TEST_CONN_HANDLE);
	disconnect_at_open = true;
	zassert_ok(bt_enable(NULL), "Bluetooth init after a disable failed");
	disconnect_at_open = false;

	connect_as_peripheral();

	zassert_ok(bt_conn_get_info(test_conn, &info), "No connection info");
	zassert_equal(info.state, BT_CONN_STATE_CONNECTED, "The connection is in state %u",
		      info.state);

	zassert_ok(bt_disable(), "Bluetooth disable failed");
	bt_conn_drop(&test_conn);
}

static atomic_t conn_complete_sent;

static void conn_complete_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	conn_complete_recv(BT_HCI_ERR_SUCCESS);
	atomic_set(&conn_complete_sent, 1);
}

static K_WORK_DELAYABLE_DEFINE(conn_complete_work, conn_complete_handler);

/* bt_disable() lets go of the advertiser, while the controller goes on
 * advertising until it is reset and can report a connection until then. With
 * extended advertising built in and a controller without it, the Host has no
 * advertiser left to look at.
 */
static ZTEST(bt_disable, test_connection_during_disable)
{
	if (IS_ENABLED(CONFIG_TEST_DRIVER_NO_CLOSE)) {
		ztest_test_skip();
	}

	zassert_ok(bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, NULL, 0U, NULL, 0U),
		   "Advertising failed to start");

	atomic_clear(&conn_complete_sent);
	reset_rsp_delay = K_MSEC(20);
	(void)k_work_schedule(&conn_complete_work, K_MSEC(10));

	zassert_ok(bt_disable(), "Bluetooth disable failed");
	zassert_true(atomic_get(&conn_complete_sent) != 0, "The connection was not reported");
}

/* The same for the end of directed advertising, which arrives once the driver
 * has failed to close the transport and the Host processes what it receives
 * again.
 */
static ZTEST(bt_disable, test_adv_timeout_after_close_failure)
{
	int err;

	if (IS_ENABLED(CONFIG_TEST_DRIVER_NO_CLOSE)) {
		ztest_test_skip();
	}

	zassert_ok(bt_le_adv_start(BT_LE_ADV_NCONN, NULL, 0U, NULL, 0U),
		   "Advertising failed to start");

	close_err = -EIO;
	err = bt_disable();
	zassert_equal(err, -EIO, "bt_disable() gave %d (!= %d)", err, -EIO);

	conn_complete_recv(BT_HCI_ERR_ADV_TIMEOUT);
	k_sleep(K_MSEC(10));

	close_err = 0;
	zassert_ok(bt_disable(), "Retrying the disable failed");
}

/* The controller reports completed packets for the connection */
static void num_completed_recv(const struct device *dev, uint16_t count)
{
	struct bt_hci_evt_num_completed_packets *evt;
	struct net_buf *buf;

	buf = bt_buf_get_evt(BT_HCI_EVT_NUM_COMPLETED_PACKETS, false, K_FOREVER);
	evt_create(buf, BT_HCI_EVT_NUM_COMPLETED_PACKETS, sizeof(*evt) + sizeof(evt->h[0]));
	evt = net_buf_add(buf, sizeof(*evt) + sizeof(evt->h[0]));
	evt->num_handles = 1U;
	evt->h[0].handle = sys_cpu_to_le16(TEST_CONN_HANDLE);
	evt->h[0].count = sys_cpu_to_le16(count);
	bt_hci_recv(dev, buf);
}

/* Once a connection is on its way down its unacknowledged packets have been
 * returned, so there is nothing left for a completion to find. It comes to this
 * when the thread that handles the event preempts the teardown of a connection
 * in bt_disable() with a controller that has not been reset. Here the
 * disconnection is reported first, which takes the connection to the same state.
 */
static ZTEST(bt_disable, test_completed_packets_after_disconnection)
{
	const struct device *dev = DEVICE_DT_GET(DT_DRV_INST(0));

	if (IS_ENABLED(CONFIG_TEST_DRIVER_NO_CLOSE)) {
		ztest_test_skip();
	}

	connect_as_peripheral();
	k_sem_reset(&disconnected_sem);

	disconnect_handle = sys_cpu_to_le16(TEST_CONN_HANDLE);
	disconn_complete_recv(dev);
	num_completed_recv(dev, 1U);

	zassert_ok(k_sem_take(&disconnected_sem, K_SECONDS(1)), "No disconnection");
	bt_conn_drop(&test_conn);
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

/* A dynamic LE L2CAP channel ID that no channel has: the Host drops what the
 * peer sends on it, once it has received it.
 */
#define TEST_UNKNOWN_CID 0x007FU

/* The peer sends an L2CAP PDU on the connection */
static void l2cap_pdu_recv(uint16_t cid, const uint8_t *data, uint16_t len)
{
	const struct device *dev = DEVICE_DT_GET(DT_DRV_INST(0));
	struct bt_hci_acl_hdr *hdr;
	struct net_buf *buf;

	buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_FOREVER);
	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->handle = sys_cpu_to_le16(bt_acl_handle_pack(TEST_CONN_HANDLE, BT_ACL_START));
	hdr->len = sys_cpu_to_le16(BT_L2CAP_HDR_SIZE + len);
	net_buf_add_le16(buf, len);
	net_buf_add_le16(buf, cid);
	net_buf_add_mem(buf, data, len);
	bt_hci_recv(dev, buf);
}

static void host_ncp_reset(void)
{
	atomic_clear(&host_ncp_num);
	k_sem_reset(&host_ncp_sem);
}

/* The Host reports nothing more */
static void expect_no_more_host_ncp(void)
{
	/* Not in the assertion, whose message would count before the wait */
	int err = k_sem_take(&host_ncp_sem, K_MSEC(100));

	zassert_equal(err, -EAGAIN, "%ld Host Number Of Completed Packets commands",
		      (long)atomic_get(&host_ncp_num));
}

/* The Host reports count completed packets for handle, in a command for that
 * connection alone, and reports nothing else.
 */
static void expect_host_ncp(uint16_t handle, uint16_t count)
{
	zassert_ok(k_sem_take(&host_ncp_sem, K_SECONDS(1)),
		   "No Host Number Of Completed Packets command");
	zassert_equal(host_ncp_log[0].num_handles, 1U, "Reported for %u connections",
		      host_ncp_log[0].num_handles);
	zassert_equal(host_ncp_log[0].handle, handle, "Reported for handle 0x%04x",
		      host_ncp_log[0].handle);
	zassert_equal(host_ncp_log[0].count, count, "Reported %u packets", host_ncp_log[0].count);
	expect_no_more_host_ncp();
}

ZTEST_SUITE(bt_host_ncp, NULL, NULL, before, NULL, NULL);

/* A received packet that is dropped before the Host has looked up its
 * connection is reported for none, and in particular not for the first
 * connection, which the zeroed metadata of the buffer would name. The packet
 * that the Host does receive is reported for its connection.
 */
static ZTEST(bt_host_ncp, test_dropped_packets_not_reported)
{
	const struct device *dev = DEVICE_DT_GET(DT_DRV_INST(0));
	const uint8_t data = 0x00U;
	struct net_buf *buf;

	if (IS_ENABLED(CONFIG_TEST_DRIVER_NO_CLOSE) ||
	    !IS_ENABLED(CONFIG_BT_HCI_ACL_FLOW_CONTROL)) {
		ztest_test_skip();
	}

	connect_as_peripheral();
	host_ncp_reset();

	/* The driver gives up on a packet that it had a buffer for */
	net_buf_unref(bt_buf_get_rx(BT_BUF_ACL_IN, K_FOREVER));

	/* A packet too short for its header */
	buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_FOREVER);
	net_buf_add_le16(buf, bt_acl_handle_pack(TEST_CONN_HANDLE, BT_ACL_START));
	bt_hci_recv(dev, buf);

	l2cap_pdu_recv(TEST_UNKNOWN_CID, &data, sizeof(data));
	expect_host_ncp(TEST_CONN_HANDLE, 1U);

	zassert_ok(bt_disable(), "Bluetooth disable failed");
	bt_conn_unref(test_conn);
	test_conn = NULL;
}
#endif /* CONFIG_BT_PERIPHERAL && CONFIG_BT_GATT_CLIENT */

#if defined(CONFIG_TEST_DRIVER_STATE_REPORT)
#define TEST_SENDERS 2

static K_SEM_DEFINE(unavailable_sem, 0, 1);

/* What the unavailable callback found */
static bool ready_at_unavailable;
#if defined(CONFIG_BT_PERIPHERAL) && defined(CONFIG_BT_GATT_CLIENT)
static bool disconnected_at_unavailable;
#endif
static bool available_at_unavailable;

static struct bt_dev *reported_dev;

static void ctlr_unavailable(struct bt_dev *dev)
{
	reported_dev = dev;
	ready_at_unavailable = bt_is_ready();
#if defined(CONFIG_BT_PERIPHERAL) && defined(CONFIG_BT_GATT_CLIENT)
	disconnected_at_unavailable = k_sem_count_get(&disconnected_sem) != 0U;
#endif
	available_at_unavailable = k_sem_count_get(&available_sem) != 0U;
	k_sem_give(&unavailable_sem);
}

static void ctlr_available(struct bt_dev *dev)
{
	reported_dev = dev;
	k_sem_give(&available_sem);
}

BT_CONTROLLER_CB_DEFINE(ctlr_callbacks) = {
	.unavailable = ctlr_unavailable,
	.available = ctlr_available,
};

/* send() fails from before the report, as the driver API documents */
static void ctlr_lost(void)
{
	cmd_send_err = -EIO;
	data_err = -EIO;
	bt_hci_report_unavailable(DEVICE_DT_GET(DT_DRV_INST(0)));
}

static void ctlr_back(void)
{
	cmd_send_err = 0;
	data_err = 0;
	silent = false;
	bt_hci_report_available(DEVICE_DT_GET(DT_DRV_INST(0)));
}

static void expect_unavailable(void)
{
	zassert_ok(k_sem_take(&unavailable_sem, K_SECONDS(1)), "No unavailable callback");
	zassert_not_null(reported_dev, "No Host instance in the callback");
	zassert_false(ready_at_unavailable, "Bluetooth was ready in the callback");
	zassert_false(bt_is_ready(), "Bluetooth is ready after the callback");
}

static void expect_enable_after_back(void)
{
	ctlr_back();
	zassert_ok(k_sem_take(&available_sem, K_SECONDS(1)), "No available callback");
	zassert_ok(bt_enable(NULL), "Bluetooth init after the controller is back failed");
	zassert_true(bt_is_ready(), "Bluetooth is not ready after init");
}

static void lost_isr(const void *arg)
{
	ARG_UNUSED(arg);

	ctlr_lost();
}

static void lost_before(void *fixture)
{
	before(fixture);

	k_sem_reset(&unavailable_sem);
	k_sem_reset(&available_sem);
#if defined(CONFIG_BT_PERIPHERAL) && defined(CONFIG_BT_GATT_CLIENT)
	k_sem_reset(&disconnected_sem);
#endif
}

ZTEST_SUITE(bt_hci_lost, NULL, NULL, lost_before, NULL, NULL);

/* Bluetooth is disabled without a reset, which nothing would answer, and can be
 * enabled again.
 */
static ZTEST(bt_hci_lost, test_lost)
{
	zassert_ok(bt_le_adv_start(BT_LE_ADV_NCONN, NULL, 0U, NULL, 0U),
		   "Advertising failed to start");

	ctlr_lost();
	expect_unavailable();

	zassert_equal(reset_count, 0U, "The controller was reset %u times", reset_count);
	zassert_equal(close_count, 1U, "close() was called %u times", close_count);
	zassert_equal(bt_disable(), -EALREADY, "Bluetooth was not disabled already");

	expect_enable_after_back();
	zassert_equal(reset_count, 1U, "The controller was reset %u times", reset_count);

	/* The advertising set of the previous run is gone */
	zassert_ok(bt_le_adv_start(BT_LE_ADV_NCONN, NULL, 0U, NULL, 0U),
		   "Advertising failed to start after the controller is back");
	zassert_ok(bt_le_adv_stop(), "Advertising failed to stop");
}

/* A driver can report from an interrupt, such as that of a reset line */
static ZTEST(bt_hci_lost, test_lost_from_isr)
{
	irq_offload(lost_isr, NULL);
	expect_unavailable();

	zassert_equal(close_count, 1U, "close() was called %u times", close_count);

	expect_enable_after_back();
}

#if defined(CONFIG_BT_PERIPHERAL) && defined(CONFIG_BT_GATT_CLIENT)
/* A connection has gone by the time that the application is told. */
static ZTEST(bt_hci_lost, test_lost_with_conn)
{
	connect_as_peripheral();
	k_sem_reset(&disconnected_sem);

	ctlr_lost();
	expect_unavailable();

	zassert_true(disconnected_at_unavailable, "No disconnection before the callback");

	bt_conn_unref(test_conn);
	test_conn = NULL;

	expect_enable_after_back();
	connect_as_peripheral();

	zassert_ok(bt_disable(), "Bluetooth disable failed");
	bt_conn_unref(test_conn);
	test_conn = NULL;
}
#endif

static K_THREAD_STACK_ARRAY_DEFINE(sender_stacks, TEST_SENDERS,
				   CONFIG_ZTEST_STACK_SIZE + CONFIG_TEST_EXTRA_STACK_SIZE);
static struct k_thread sender_threads[TEST_SENDERS];

static void sender_entry(void *p1, void *p2, void *p3)
{
	int *err = p1;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	*err = bt_hci_cmd_send_sync(BT_HCI_OP_READ_LOCAL_VERSION_INFO, NULL, NULL);
}

/* The commands that are waiting, one for its response and one for its turn,
 * fail at once instead of running into the assert of the command timeout.
 */
static ZTEST(bt_hci_lost, test_lost_with_commands_waiting)
{
	int errs[TEST_SENDERS];

	silent = true;

	ARRAY_FOR_EACH(sender_threads, i) {
		k_thread_create(&sender_threads[i], sender_stacks[i],
				K_THREAD_STACK_SIZEOF(sender_stacks[i]), sender_entry, &errs[i],
				NULL, NULL, CONFIG_ZTEST_THREAD_PRIORITY, 0, K_NO_WAIT);
	}
	k_sleep(K_MSEC(100));
	zassert_equal(cmd_count, 1U, "%u commands were sent, not one", cmd_count);

	ctlr_lost();

	ARRAY_FOR_EACH(sender_threads, i) {
		zassert_ok(k_thread_join(&sender_threads[i], K_SECONDS(1)),
			   "Sender %zu is still waiting", i);
		zassert_true(errs[i] < 0, "Sender %zu got %d", i, errs[i]);
	}

	expect_unavailable();
	expect_enable_after_back();
}

static K_SEM_DEFINE(workq_sender_sem, 0, 1);
static int workq_sender_err;

static void workq_sender_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	workq_sender_err = bt_hci_cmd_send_sync(BT_HCI_OP_READ_LOCAL_VERSION_INFO, NULL, NULL);
	k_sem_give(&workq_sender_sem);
}

static K_WORK_DEFINE(workq_sender_work, workq_sender_handler);

/* An event handler can send a command on the Bluetooth workqueue, and wait for it
 * there. The disable runs on that workqueue, so it can only come after that.
 */
static ZTEST(bt_hci_lost, test_lost_with_sender_on_workq)
{
	int err;

	silent = true;

	k_thread_create(&sender_threads[0], sender_stacks[0],
			K_THREAD_STACK_SIZEOF(sender_stacks[0]), sender_entry, &err, NULL, NULL,
			CONFIG_ZTEST_THREAD_PRIORITY, 0, K_NO_WAIT);
	k_sleep(K_MSEC(100));

	(void)bt_work_submit(&workq_sender_work);
	k_sleep(K_MSEC(100));
	zassert_equal(cmd_count, 1U, "%u commands were sent, not one", cmd_count);

	ctlr_lost();

	zassert_ok(k_sem_take(&workq_sender_sem, K_SECONDS(1)), "The sender is still waiting");
	zassert_true(workq_sender_err < 0, "The sender got %d", workq_sender_err);
	zassert_ok(k_thread_join(&sender_threads[0], K_SECONDS(1)), "The first sender is waiting");
	zassert_true(err < 0, "The first sender got %d", err);

	expect_unavailable();
	expect_enable_after_back();
}

static K_SEM_DEFINE(syswq_sender_sem, 0, 1);
static int syswq_sender_err;

static void syswq_sender_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	syswq_sender_err = bt_hci_cmd_send_sync(BT_HCI_OP_READ_LOCAL_VERSION_INFO, NULL, NULL);
	k_sem_give(&syswq_sender_sem);
}

static K_WORK_DEFINE(syswq_sender_work, syswq_sender_handler);

/* Without a TX processor thread, a sender on the system workqueue sends its
 * command itself and waits for the credit of the command before it. Nothing else
 * that runs on that workqueue can release it, the disable included.
 */
static ZTEST(bt_hci_lost, test_lost_with_sender_on_syswq)
{
	int err;

	if (IS_ENABLED(CONFIG_BT_TX_PROCESSOR_THREAD)) {
		ztest_test_skip();
	}

	silent = true;

	k_thread_create(&sender_threads[0], sender_stacks[0],
			K_THREAD_STACK_SIZEOF(sender_stacks[0]), sender_entry, &err, NULL, NULL,
			CONFIG_ZTEST_THREAD_PRIORITY, 0, K_NO_WAIT);
	k_sleep(K_MSEC(100));

	k_work_submit(&syswq_sender_work);
	k_sleep(K_MSEC(100));
	zassert_equal(cmd_count, 1U, "%u commands were sent, not one", cmd_count);

	ctlr_lost();

	zassert_ok(k_sem_take(&syswq_sender_sem, K_SECONDS(1)), "The sender is still waiting");
	zassert_true(syswq_sender_err < 0, "The sender got %d", syswq_sender_err);
	zassert_ok(k_thread_join(&sender_threads[0], K_SECONDS(1)), "The first sender is waiting");
	zassert_true(err < 0, "The first sender got %d", err);

	expect_unavailable();
	expect_enable_after_back();
}

#if defined(CONFIG_BT_PERIPHERAL) && defined(CONFIG_BT_GATT_CLIENT)
/* The sender on the system workqueue drains the queue up to its own command,
 * which stops at the loss, with the second sender's command ahead of its own.
 * The teardown of a connection waits for that workqueue, so the command has to
 * be failed ahead of the teardown.
 */
static ZTEST(bt_hci_lost, test_lost_with_conn_and_sender_on_syswq)
{
	int errs[TEST_SENDERS];

	if (IS_ENABLED(CONFIG_BT_TX_PROCESSOR_THREAD)) {
		ztest_test_skip();
	}

	connect_as_peripheral();
	cmd_count = 0U;
	silent = true;

	ARRAY_FOR_EACH(sender_threads, i) {
		k_thread_create(&sender_threads[i], sender_stacks[i],
				K_THREAD_STACK_SIZEOF(sender_stacks[i]), sender_entry, &errs[i],
				NULL, NULL, CONFIG_ZTEST_THREAD_PRIORITY, 0, K_NO_WAIT);
		k_sleep(K_MSEC(100));
	}

	k_work_submit(&syswq_sender_work);
	k_sleep(K_MSEC(100));
	zassert_equal(cmd_count, 1U, "%u commands were sent, not one", cmd_count);

	ctlr_lost();

	zassert_ok(k_sem_take(&syswq_sender_sem, K_SECONDS(1)), "The sender is still waiting");
	zassert_true(syswq_sender_err < 0, "The sender got %d", syswq_sender_err);

	ARRAY_FOR_EACH(sender_threads, i) {
		zassert_ok(k_thread_join(&sender_threads[i], K_SECONDS(1)), "Sender %zu is waiting",
			   i);
		zassert_true(errs[i] < 0, "Sender %zu got %d", i, errs[i]);
	}

	expect_unavailable();
	bt_conn_drop(&test_conn);
	expect_enable_after_back();
}
#endif /* CONFIG_BT_PERIPHERAL && CONFIG_BT_GATT_CLIENT */

#if defined(CONFIG_BT_OBSERVER)
/* A controller that comes back can deliver a report before bt_enable() resets
 * it, as one that could not be reset can. The scan that it belongs to ended with
 * the loss, and its callback must not run.
 */
static ZTEST(bt_hci_lost, test_scan_report_after_lost)
{
	zassert_ok(bt_le_scan_start(BT_LE_SCAN_PASSIVE, scan_report), "Scanning failed to start");
	atomic_clear(&scan_report_count);

	ctlr_lost();
	expect_unavailable();

	/* The Host goes on to process the report during the reset */
	reset_rsp_delay = K_MSEC(20);
	adv_report_at_open = true;
	expect_enable_after_back();

	zassert_equal(atomic_get(&scan_report_count), 0, "The scan callback ran %ld times",
		      (long)atomic_get(&scan_report_count));
}
#endif /* CONFIG_BT_OBSERVER */

/* As above, for a driver that reports while it has a command in its hands */
static ZTEST(bt_hci_lost, test_lost_in_send)
{
	int err;

	lost_in_send = true;
	err = bt_hci_cmd_send_sync(BT_HCI_OP_READ_LOCAL_VERSION_INFO, NULL, NULL);
	zassert_true(err < 0, "The command got %d", err);

	expect_unavailable();
	expect_enable_after_back();
}

/* As above, with the send failing too, so the command has two ways to complete */
static ZTEST(bt_hci_lost, test_lost_in_failing_send)
{
	int err;

	lost_in_send = true;
	lost_in_send_err = -EIO;
	err = bt_hci_cmd_send_sync(BT_HCI_OP_READ_LOCAL_VERSION_INFO, NULL, NULL);
	zassert_true(err < 0, "The command got %d", err);

	expect_unavailable();
	expect_enable_after_back();
}

static K_THREAD_STACK_DEFINE(lost_enable_stack,
			     CONFIG_ZTEST_STACK_SIZE + CONFIG_TEST_EXTRA_STACK_SIZE);
static struct k_thread lost_enable_thread;

static void lost_enable_entry(void *p1, void *p2, void *p3)
{
	int *err = p1;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	*err = bt_enable(NULL);
}

/* bt_enable() fails, and the disable waits for it to do so */
static ZTEST(bt_hci_lost, test_lost_during_enable)
{
	int err;

	zassert_ok(bt_disable(), "Bluetooth disable failed");
	close_count = 0U;

	silent = true;
	k_thread_create(&lost_enable_thread, lost_enable_stack,
			K_THREAD_STACK_SIZEOF(lost_enable_stack), lost_enable_entry, &err, NULL,
			NULL, CONFIG_ZTEST_THREAD_PRIORITY, 0, K_NO_WAIT);
	k_sleep(K_MSEC(100));

	ctlr_lost();

	zassert_ok(k_thread_join(&lost_enable_thread, K_SECONDS(1)),
		   "bt_enable() is still waiting");
	zassert_true(err < 0, "bt_enable() gave %d", err);

	expect_unavailable();
	zassert_equal(close_count, 1U, "close() was called %u times", close_count);

	expect_enable_after_back();
}

/* The transport that the driver has opened is closed again */
static ZTEST(bt_hci_lost, test_lost_in_open)
{
	zassert_ok(bt_disable(), "Bluetooth disable failed");
	close_count = 0U;

	lost_in_open = true;
	zassert_true(bt_enable(NULL) < 0, "bt_enable() succeeded without a controller");
	zassert_equal(open_close_count, 0U, "close() was called during open()");

	expect_unavailable();
	zassert_equal(close_count, 1U, "close() was called %u times", close_count);

	expect_enable_after_back();
}

/* A driver that fails the open has no transport left to close */
static ZTEST(bt_hci_lost, test_lost_in_failed_open)
{
	zassert_ok(bt_disable(), "Bluetooth disable failed");
	close_count = 0U;

	lost_in_open = true;
	open_err = -EIO;
	zassert_equal(bt_enable(NULL), -EIO, "bt_enable() did not fail with the open");
	open_err = 0;

	expect_unavailable();
	zassert_equal(close_count, 0U, "close() was called %u times", close_count);

	expect_enable_after_back();
}

/* With a driver that cannot tell that the controller is back, the application
 * tries. A failed try leaves the transport open, as any failed enable does, and
 * the disable that closes it cannot reset the controller.
 */
static ZTEST(bt_hci_lost, test_enable_before_back)
{
	ctlr_lost();
	expect_unavailable();

	zassert_true(bt_enable(NULL) < 0, "bt_enable() succeeded without a controller");
	disable_reset_fails();

	expect_enable_after_back();
}

/* The application is told in the order that the driver reported, and only once
 * Bluetooth has been disabled.
 */
static ZTEST(bt_hci_lost, test_lost_and_back_at_once)
{
	ctlr_lost();
	ctlr_back();

	expect_unavailable();
	zassert_false(available_at_unavailable, "Told of the return before the loss");
	zassert_equal(close_count, 1U, "close() was called %u times", close_count);
	zassert_ok(k_sem_take(&available_sem, K_SECONDS(1)), "No available callback");

	zassert_ok(bt_enable(NULL), "Bluetooth init after the controller is back failed");
}

/* A return that the controller did not stay back for is not reported */
static ZTEST(bt_hci_lost, test_lost_again_before_told)
{
	ctlr_lost();
	ctlr_back();
	ctlr_lost();

	expect_unavailable();
	zassert_equal(k_sem_take(&available_sem, K_MSEC(100)), -EAGAIN, "Told of an old return");

	expect_enable_after_back();
}

/* With the transport closed there is nothing to tear down, only the news */
static ZTEST(bt_hci_lost, test_reports_while_closed)
{
	zassert_ok(bt_disable(), "Bluetooth disable failed");
	close_count = 0U;

	ctlr_lost();
	expect_unavailable();
	zassert_equal(close_count, 0U, "close() was called %u times", close_count);

	expect_enable_after_back();
}

/* The application is not told in the middle of a disable */
static ZTEST(bt_hci_lost, test_available_during_disable)
{
	available_in_close = true;
	zassert_ok(bt_disable(), "Bluetooth disable failed");
	zassert_false(available_told_in_close, "Told of the return during the disable");

	zassert_ok(k_sem_take(&available_sem, K_SECONDS(1)), "No available callback");
}

#if defined(CONFIG_BT_SETTINGS)
static void settings_suite_before(void *fixture)
{
	ARG_UNUSED(fixture);

	k_sem_reset(&unavailable_sem);
	k_sem_reset(&available_sem);
	close_count = 0U;
}

ZTEST_SUITE(bt_hci_lost_settings, NULL, NULL, settings_suite_before, NULL, NULL);

static void enable_with_settings(void)
{
	zassert_ok(bt_enable(NULL), "Bluetooth init failed");
	zassert_false(bt_is_ready(), "Bluetooth is ready before the settings are loaded");
	zassert_ok(settings_load_subtree("bt"), "Loading the settings failed");
	zassert_true(bt_is_ready(), "Bluetooth is not ready with the settings loaded");
}

/* An application that does not get to load the settings does not keep the
 * initialization from ending, as nothing commits them with the transport lost.
 */
static ZTEST(bt_hci_lost_settings, test_lost_before_settings)
{
	zassert_ok(bt_enable(NULL), "Bluetooth init failed");
	zassert_false(bt_is_ready(), "Bluetooth is ready before the settings are loaded");

	ctlr_lost();
	expect_unavailable();
	zassert_equal(close_count, 1U, "close() was called %u times", close_count);

	ctlr_back();
	zassert_ok(k_sem_take(&available_sem, K_SECONDS(1)), "No available callback");
	enable_with_settings();

	zassert_ok(bt_disable(), "Bluetooth disable failed");
}

static void lost_and_store_isr(const void *arg)
{
	ARG_UNUSED(arg);

	ctlr_lost();
	(void)bt_settings_store_id();
}

/* A store behind the disable on the Bluetooth workqueue cannot be waited for there */
static ZTEST(bt_hci_lost_settings, test_lost_with_store_pending)
{
	enable_with_settings();

	irq_offload(lost_and_store_isr, NULL);
	expect_unavailable();

	ctlr_back();
	zassert_ok(k_sem_take(&available_sem, K_SECONDS(1)), "No available callback");
}
#endif /* CONFIG_BT_SETTINGS */

BUILD_ASSERT(DT_SAME_NODE(DT_DRV_INST(0), DT_CHOSEN(zephyr_bt_hci)),
	     "The tests take instance 0 for the one that the Host uses");

static K_SEM_DEFINE(other_state_sem, 0, 1);
static const struct device *other_state_dev;
static bool other_state_available;

static void other_state_cb(const struct device *dev, bool available)
{
	other_state_dev = dev;
	other_state_available = available;
	k_sem_give(&other_state_sem);
}

/* Each instance has a user of its own, here the Host and this test, and neither
 * hears of what happens to the controller of the other.
 */
static ZTEST(bt_hci_lost, test_other_instance)
{
	const struct device *other = DEVICE_DT_GET(DT_DRV_INST(1));

	k_sem_reset(&other_state_sem);
	zassert_ok(bt_hci_state_cb_set(other, other_state_cb), "Setting the callback failed");

	bt_hci_report_unavailable(other);
	zassert_ok(k_sem_take(&other_state_sem, K_SECONDS(1)), "No report for the other instance");
	zassert_equal(other_state_dev, other, "The report is for %p", other_state_dev);
	zassert_false(other_state_available, "The report is not the one made");

	bt_hci_report_available(other);
	zassert_ok(k_sem_take(&other_state_sem, K_SECONDS(1)), "No report for the other instance");
	zassert_true(other_state_available, "The report is not the one made");

	zassert_equal(k_sem_take(&unavailable_sem, K_MSEC(50)), -EAGAIN, "The Host took a report");
	zassert_equal(k_sem_take(&available_sem, K_NO_WAIT), -EBUSY, "The Host took a report");
	zassert_true(bt_is_ready(), "Bluetooth was disabled for another instance");
	zassert_equal(close_count, 0U, "close() was called %u times", close_count);

	ctlr_lost();
	expect_unavailable();
	zassert_equal(k_sem_take(&other_state_sem, K_MSEC(50)), -EAGAIN, "A report of the Host's");

	zassert_ok(bt_hci_state_cb_set(other, NULL), "Removing the callback failed");

	expect_enable_after_back();
}

/* The reports of a device go to one user, as its transport does */
static ZTEST(bt_hci_lost, test_state_cb_one_user)
{
	const struct device *dev = DEVICE_DT_GET(DT_DRV_INST(0));

	zassert_equal(bt_hci_state_cb_set(dev, other_state_cb), -EALREADY,
		      "The callback of the Host was replaced");

	ctlr_lost();
	expect_unavailable();

	/* With another user on the device, the Host does not start */
	zassert_ok(bt_hci_state_cb_set(dev, NULL), "Removing the callback failed");
	zassert_ok(bt_hci_state_cb_set(dev, other_state_cb), "Setting the callback failed");
	cmd_send_err = 0;
	data_err = 0;
	zassert_equal(bt_enable(NULL), -EALREADY, "bt_enable() started on another user's device");

	zassert_ok(bt_hci_state_cb_set(dev, NULL), "Removing the callback failed");
	zassert_ok(bt_enable(NULL), "Bluetooth init without the other user failed");
}
#endif /* CONFIG_TEST_DRIVER_STATE_REPORT */
