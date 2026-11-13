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
#include <zephyr/bluetooth/iso.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_error_hook.h>

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

	/* Not with the tests of periodic advertising sync transfer, so that
	 * their connection is made as in the other tests
	 */
	if (IS_ENABLED(CONFIG_BT_PER_ADV_SYNC) && !IS_ENABLED(CONFIG_TEST_PAST)) {
		rp->features[BT_LE_FEAT_BIT_EXT_ADV / 8U] |= BIT(BT_LE_FEAT_BIT_EXT_ADV % 8U);
	}

	if (IS_ENABLED(CONFIG_BT_PER_ADV_SYNC)) {
		rp->features[BT_LE_FEAT_BIT_PER_ADV / 8U] |= BIT(BT_LE_FEAT_BIT_PER_ADV % 8U);
	}

	if (IS_ENABLED(CONFIG_BT_PER_ADV_SYNC_TRANSFER_RECEIVER)) {
		rp->features[BT_LE_FEAT_BIT_PAST_RECV / 8U] |= BIT(BT_LE_FEAT_BIT_PAST_RECV % 8U);
	}
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

BUILD_ASSERT(LONG_RSP_SIZE > CONFIG_BT_BUF_EVT_RX_SIZE);

static void long_rsp(struct net_buf *buf, struct net_buf **evt, uint8_t len, uint16_t opcode)
{
	struct bt_hci_evt_cc_status *rp;

	ARG_UNUSED(buf);

	rp = cmd_complete(evt, len, opcode);
	(void)memset(rp, 0xAA, len);
	rp->status = BT_HCI_ERR_SUCCESS;
}

/* The controller takes a periodic advertising create sync command and then
 * looks for the advertiser, as it does for the commands that start a
 * procedure.
 */
static void cmd_status_success(struct net_buf *buf, struct net_buf **evt, uint8_t len,
			       uint16_t opcode)
{
	struct bt_hci_evt_cmd_status *status;

	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	*evt = bt_buf_get_evt(BT_HCI_EVT_CMD_STATUS, false, K_FOREVER);
	evt_create(*evt, BT_HCI_EVT_CMD_STATUS, sizeof(*status));
	status = net_buf_add(*evt, sizeof(*status));
	status->status = BT_HCI_ERR_SUCCESS;
	status->ncmd = 1U;
	status->opcode = sys_cpu_to_le16(opcode);
}

/* Whether the controller has been told to scan */
static bool scan_enabled;

static void le_set_ext_scan_enable(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				   uint16_t opcode)
{
	const struct bt_hci_cp_le_set_ext_scan_enable *cp = (const void *)buf->data;

	scan_enabled = (cp->enable == BT_HCI_LE_SCAN_ENABLE);
	generic_success(buf, evt, len, opcode);
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
	{
		BT_HCI_OP_LE_PER_ADV_CREATE_SYNC,
		0U,
		cmd_status_success,
	},
	{
		BT_HCI_OP_LE_SET_EXT_SCAN_ENABLE,
		GENERIC_RP_SIZE,
		le_set_ext_scan_enable,
	},
};

/* The response of the controller to a command */
static void cmd_rsp(struct net_buf *cmd, struct net_buf **evt, uint16_t opcode)
{
	for (size_t i = 0U; i < ARRAY_SIZE(cmds); i++) {
		if (cmds[i].opcode == opcode) {
			cmds[i].handler(cmd, evt, cmds[i].len, opcode);
			return;
		}
	}

	generic_success(cmd, evt, GENERIC_RP_SIZE, opcode);
}

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
static unsigned int close_count;
static int close_err;
static bool silent;
static bool stops;
static bool blocks;

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

/* The opcodes of the commands that the controller has been given, the first
 * ones of them
 */
static uint16_t sent_opcodes[64];
static size_t sent_num;

/* The command that the controller takes long to respond to, if any, how long
 * it takes, whether it rejects the command then, and whether it reports the
 * periodic advertising sync as established right after the response.
 */
static uint16_t slow_opcode;
static k_timeout_t slow_rsp_delay;
static uint8_t slow_status;
static bool slow_established;
static uint8_t slow_established_status;

static void sync_established(uint8_t status);

static void slow_rsp_handler(struct k_work *work)
{
	const struct device *dev = DEVICE_DT_GET(DT_DRV_INST(0));
	struct bt_hci_evt_cmd_status *status;
	struct net_buf *evt = NULL;

	ARG_UNUSED(work);

	if (slow_status == BT_HCI_ERR_SUCCESS) {
		cmd_rsp(NULL, &evt, slow_opcode);
	} else {
		evt = bt_buf_get_evt(BT_HCI_EVT_CMD_STATUS, false, K_FOREVER);
		evt_create(evt, BT_HCI_EVT_CMD_STATUS, sizeof(*status));
		status = net_buf_add(evt, sizeof(*status));
		status->status = slow_status;
		status->ncmd = 1U;
		status->opcode = sys_cpu_to_le16(slow_opcode);
	}

	bt_hci_recv(dev, evt);

	if (slow_established) {
		sync_established(slow_established_status);
	}
}

static K_WORK_DELAYABLE_DEFINE(slow_rsp_work, slow_rsp_handler);

/* The command whose arrival at the controller lets a suspended thread run, with
 * a priority above all the others
 */
static uint16_t cmd_resume_opcode;
static struct k_thread *cmd_resume_thread;

/* The periodic advertising train that the tests sync to */
#define TEST_SYNC_HANDLE 0x0001U

static const struct bt_le_per_adv_sync_param sync_param = {
	.addr.type = BT_ADDR_LE_RANDOM,
	.addr.a.val = {0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0xc6U},
	.sid = 1U,
	.timeout = BT_GAP_PER_ADV_MAX_TIMEOUT,
};

/* The controller reports the outcome of the periodic advertising sync it was
 * creating, a sync to the advertising set sid.
 */
static void sync_established_sid(uint8_t status, uint8_t sid)
{
	const struct device *dev = DEVICE_DT_GET(DT_DRV_INST(0));
#if defined(CONFIG_BT_PER_ADV_SYNC_RSP)
	const uint8_t subevent = BT_HCI_EVT_LE_PER_ADV_SYNC_ESTABLISHED_V2;
	struct bt_hci_evt_le_per_adv_sync_established_v2 *evt;
#else
	const uint8_t subevent = BT_HCI_EVT_LE_PER_ADV_SYNC_ESTABLISHED;
	struct bt_hci_evt_le_per_adv_sync_established *evt;
#endif
	struct bt_hci_evt_le_meta_event *meta;
	struct net_buf *buf;

	buf = bt_buf_get_evt(BT_HCI_EVT_LE_META_EVENT, false, K_FOREVER);
	evt_create(buf, BT_HCI_EVT_LE_META_EVENT, sizeof(*meta) + sizeof(*evt));
	meta = net_buf_add(buf, sizeof(*meta));
	meta->subevent = subevent;
	evt = net_buf_add(buf, sizeof(*evt));
	(void)memset(evt, 0, sizeof(*evt));
	evt->status = status;
	evt->handle = sys_cpu_to_le16(TEST_SYNC_HANDLE);
	evt->sid = sid;
	bt_addr_le_copy(&evt->adv_addr, &sync_param.addr);
	evt->phy = BT_HCI_LE_PHY_1M;
	evt->interval = sys_cpu_to_le16(BT_HCI_LE_PER_ADV_INTERVAL_MIN);
	bt_hci_recv(dev, buf);
}

static void sync_established(uint8_t status)
{
	sync_established_sid(status, sync_param.sid);
}

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

	if (sent_num < ARRAY_SIZE(sent_opcodes)) {
		sent_opcodes[sent_num] = opcode;
	}

	sent_num++;

	if (opcode == cmd_resume_opcode) {
		cmd_resume_opcode = 0U;
		k_thread_priority_set(cmd_resume_thread, K_PRIO_COOP(0));
		k_thread_resume(cmd_resume_thread);
	}

	if (opcode == slow_opcode) {
		(void)k_work_schedule(&slow_rsp_work, slow_rsp_delay);
		return;
	}

	if (opcode == BT_HCI_OP_LE_PER_ADV_CREATE_SYNC_CANCEL) {
		/* The sync that was being created is reported as not
		 * established, after the response to the command.
		 */
		generic_success(cmd, &evt, GENERIC_RP_SIZE, opcode);
		bt_hci_recv(dev, evt);
		sync_established(BT_HCI_ERR_OP_CANCELLED_BY_HOST);
		return;
	}

	if (opcode == BT_HCI_OP_RESET) {
		reset_count++;
		scan_enabled = false;

		if (!K_TIMEOUT_EQ(reset_rsp_delay, K_NO_WAIT)) {
			(void)k_work_schedule(&reset_rsp_work, reset_rsp_delay);
			return;
		}
	}

	cmd_rsp(cmd, &evt, opcode);
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

	if (type != BT_HCI_H4_CMD) {
		data_count++;

		if (data_err != 0) {
			/* The buffer stays with the caller */
			return data_err;
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

/* Reset what the test looks at, and enable Bluetooth */
static void test_reset(void)
{
	close_err = 0;
	silent = false;
	data_err = 0;
	reset_rsp_delay = K_NO_WAIT;
	slow_opcode = 0U;
	slow_status = BT_HCI_ERR_SUCCESS;
	slow_established = false;
	slow_established_status = BT_HCI_ERR_SUCCESS;
	disconnect_rsp_delay = K_NO_WAIT;
	disconnect_status = BT_HCI_ERR_SUCCESS;
	if (!bt_is_ready()) {
		zassert_ok(bt_enable(NULL), "Bluetooth init failed");
	}

	reset_count = 0U;
	disconnect_count = 0U;
	sent_num = 0U;
	data_count = 0U;
	close_count = 0U;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	/* The Host asserts in the tests where the controller stops or the
	 * driver blocks, which leaves it in the middle of bt_enable(), so each
	 * of those tests runs alone. So do the tests of periodic advertising
	 * sync transfer, for which the controller does not support extended
	 * advertising.
	 */
	if (IS_ENABLED(CONFIG_TEST_CTLR_STOPS) || IS_ENABLED(CONFIG_TEST_SEND_BLOCKS) ||
	    IS_ENABLED(CONFIG_TEST_PAST)) {
		ztest_test_skip();
	}

	test_reset();
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

#if defined(CONFIG_BT_PER_ADV_SYNC)
/* How many of the commands that the controller has been given had opcode */
static unsigned int sent_count(uint16_t opcode)
{
	unsigned int count = 0U;

	for (size_t i = 0U; i < MIN(sent_num, ARRAY_SIZE(sent_opcodes)); i++) {
		if (sent_opcodes[i] == opcode) {
			count++;
		}
	}

	return count;
}

static K_SEM_DEFINE(synced_sem, 0, K_SEM_MAX_LIMIT);
static struct bt_le_per_adv_sync *synced_sync;

/* The sync object that the application thread has been given, and what that
 * was by the time of the last synced callback
 */
static struct bt_le_per_adv_sync *sync_app_sync;
static struct bt_le_per_adv_sync *synced_app_sync;

/* Whether the last sync was established with reports enabled */
static bool synced_recv_enabled;
static atomic_t term_count;
static struct bt_le_per_adv_sync *term_sync;
static uint8_t term_reason;

/* The terminate sync commands that the controller had been given by the
 * time of the last term callback
 */
static unsigned int term_terminate_count;
static atomic_t state_changed_count;

/* Whether the term callback is to create a new sync, and how that went */
static bool term_recreates;
static K_SEM_DEFINE(term_recreated_sem, 0, 1);
static int term_recreate_err;

/* Whether the synced callback is to wait for synced_release_sem, as one that
 * issues HCI commands does, and whether it was given
 */
static bool synced_blocks;
static K_SEM_DEFINE(synced_release_sem, 0, 1);
static bool synced_released;

static void synced_cb(struct bt_le_per_adv_sync *sync, struct bt_le_per_adv_sync_synced_info *info)
{
	synced_sync = sync;
	synced_recv_enabled = info->recv_enabled;
	synced_app_sync = sync_app_sync;
	k_sem_give(&synced_sem);

	if (synced_blocks) {
		synced_blocks = false;
		synced_released = (k_sem_take(&synced_release_sem, K_SECONDS(1)) == 0);
	}
}

static void term_cb(struct bt_le_per_adv_sync *sync,
		    const struct bt_le_per_adv_sync_term_info *info)
{
	term_sync = sync;
	term_reason = info->reason;
	term_terminate_count = sent_count(BT_HCI_OP_LE_PER_ADV_TERMINATE_SYNC);
	atomic_inc(&term_count);

	if (term_recreates) {
		term_recreates = false;
		slow_opcode = 0U;
		slow_established = false;
		term_recreate_err = bt_le_per_adv_sync_create(&sync_param, &sync_app_sync);
		k_sem_give(&term_recreated_sem);
	}
}

static void state_changed_cb(struct bt_le_per_adv_sync *sync,
			     const struct bt_le_per_adv_sync_state_info *info)
{
	ARG_UNUSED(sync);
	ARG_UNUSED(info);

	atomic_inc(&state_changed_count);
}

static void *per_adv_sync_setup(void)
{
	static struct bt_le_per_adv_sync_cb cb = {
		.synced = synced_cb,
		.term = term_cb,
		.state_changed = state_changed_cb,
	};

	int err;

	/* The suites share them */
	err = bt_le_per_adv_sync_cb_register(&cb);
	zassert_true(err == 0 || err == -EEXIST, "Failed to register the callbacks (%d)", err);

	return NULL;
}

static void per_adv_sync_reset(void)
{
	k_sem_reset(&synced_sem);
	synced_sync = NULL;
	sync_app_sync = NULL;
	synced_app_sync = NULL;
	synced_blocks = false;
	k_sem_reset(&synced_release_sem);
	synced_released = false;
	atomic_clear(&term_count);
	term_sync = NULL;
	term_reason = BT_HCI_ERR_SUCCESS;
	atomic_clear(&state_changed_count);
	term_recreates = false;
	k_sem_reset(&term_recreated_sem);
}

static void per_adv_sync_before(void *fixture)
{
	before(fixture);
	per_adv_sync_reset();
}

/* The sync is created, and established once the controller has found the
 * advertiser.
 */
static struct bt_le_per_adv_sync *sync_create_established(void)
{
	struct bt_le_per_adv_sync *sync;
	int err;

	err = bt_le_per_adv_sync_create(&sync_param, &sync);
	zassert_ok(err, "Creating the sync failed (%d)", err);
	zassert_true(scan_enabled, "The scanner was not started for the sync");

	sync_established(BT_HCI_ERR_SUCCESS);
	zassert_ok(k_sem_take(&synced_sem, K_SECONDS(1)), "The sync was not established");
	zassert_equal_ptr(synced_sync, sync, "Another sync was established");
	zassert_false(scan_enabled, "The scanner was not stopped once synced");

	return sync;
}

/* An application thread, which makes a call of the test. It is lower in
 * priority than the test thread, which is cooperative, and so runs only while
 * the test thread waits.
 */
static K_THREAD_STACK_DEFINE(sync_app_stack,
			     CONFIG_ZTEST_STACK_SIZE + CONFIG_TEST_EXTRA_STACK_SIZE);
static struct k_thread sync_app_thread;
static int (*sync_app_call)(void);
static bool sync_app_returned;
static int sync_app_err;

static void sync_app_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	sync_app_err = sync_app_call();
	sync_app_returned = true;
}

static void sync_app_start(int (*call)(void))
{
	sync_app_call = call;
	sync_app_returned = false;
	sync_app_err = -EINPROGRESS;
	k_thread_create(&sync_app_thread, sync_app_stack, K_THREAD_STACK_SIZEOF(sync_app_stack),
			sync_app_entry, NULL, NULL, NULL, K_LOWEST_APPLICATION_THREAD_PRIO, 0,
			K_NO_WAIT);
}

static int sync_app_join(void)
{
	zassert_ok(k_thread_join(&sync_app_thread, K_SECONDS(1)), "The call did not return");

	return sync_app_err;
}

/* The application thread of a test that failed is not left running, and
 * bt_disable() takes the syncs down, so that each test starts without any.
 */
static void per_adv_sync_after(void *fixture)
{
	ARG_UNUSED(fixture);

	if (sync_app_call != NULL) {
		if (k_thread_join(&sync_app_thread, K_SECONDS(1)) != 0) {
			k_thread_abort(&sync_app_thread);
		}

		sync_app_call = NULL;
	}

	cmd_resume_opcode = 0U;

	if (bt_is_ready()) {
		zassert_ok(bt_disable(), "Bluetooth disable failed");
	}
}

ZTEST_SUITE(bt_per_adv_sync, NULL, per_adv_sync_setup, per_adv_sync_before, per_adv_sync_after,
	    NULL);

static int sync_app_create(void)
{
	return bt_le_per_adv_sync_create(&sync_param, &sync_app_sync);
}

static int sync_app_create_release(void)
{
	int err = sync_app_create();

	k_sem_give(&synced_release_sem);

	return err;
}

static int sync_app_delete(void)
{
	return bt_le_per_adv_sync_delete(sync_app_sync);
}

static int sync_app_recv_disable(void)
{
	return bt_le_per_adv_sync_recv_disable(sync_app_sync);
}

#if defined(CONFIG_BT_PER_ADV_SYNC_RSP)
static int sync_app_subevent(void)
{
	uint8_t subevent = 0U;
	struct bt_le_per_adv_sync_subevent_params params = {
		.num_subevents = 1U,
		.subevents = &subevent,
	};

	return bt_le_per_adv_sync_subevent(sync_app_sync, &params);
}
#endif /* CONFIG_BT_PER_ADV_SYNC_RSP */

/* Bluetooth is disabled and enabled again while the application waits for the
 * response to a command of the call that it makes, and the call returns only
 * after a new sync has been created, which has the same sync object.
 */
static struct bt_le_per_adv_sync *sync_app_call_across_disable(int (*call)(void), uint16_t opcode)
{
	struct bt_le_per_adv_sync *sync;
	int err;

	slow_opcode = opcode;
	slow_rsp_delay = K_MSEC(20);
	sync_app_start(call);
	k_sleep(K_MSEC(10));
	zassert_true(k_work_delayable_is_pending(&slow_rsp_work), "The command was not sent");

	/* The reset waits for the response to the command */
	zassert_ok(bt_disable(), "Bluetooth disable failed");
	slow_opcode = 0U;
	zassert_ok(bt_enable(NULL), "Bluetooth init after a disable failed");

	err = bt_le_per_adv_sync_create(&sync_param, &sync);
	zassert_ok(err, "Creating a sync after bt_enable() failed (%d)", err);
	zassert_false(sync_app_returned, "The call returned before the sync was created");

	return sync;
}

/* A sync that is created and established, a sync whose creation is cancelled,
 * and one created after that.
 */
static ZTEST(bt_per_adv_sync, test_sync_create_and_delete)
{
	struct bt_le_per_adv_sync *sync;
	int err;

	sync = sync_create_established();

	zassert_ok(bt_le_per_adv_sync_delete(sync), "Deleting the sync failed");
	zassert_equal(atomic_get(&term_count), 1, "%ld term callbacks", atomic_get(&term_count));
	zassert_equal_ptr(term_sync, sync, "Another sync was terminated");
	zassert_equal(term_reason, BT_HCI_ERR_LOCALHOST_TERM_CONN, "Terminated with 0x%02x",
		      term_reason);

	err = bt_le_per_adv_sync_create(&sync_param, &sync);
	zassert_ok(err, "Creating the sync failed (%d)", err);
	zassert_ok(bt_le_per_adv_sync_delete(sync), "Cancelling the sync failed");
	k_sleep(K_MSEC(1));
	zassert_equal(atomic_get(&term_count), 1, "%ld term callbacks", atomic_get(&term_count));
	zassert_false(scan_enabled, "The scanner was not stopped once cancelled");

	(void)sync_create_established();
}

/* An application thread creates a sync while bt_disable() waits for the RX
 * work in flight, a synced callback that issues HCI commands. The sync must not
 * be left behind to keep the next one from being created after bt_enable().
 */
static ZTEST(bt_per_adv_sync, test_sync_create_during_disable)
{
	struct bt_le_per_adv_sync *sync;
	int err;

	err = bt_le_per_adv_sync_create(&sync_param, &sync);
	zassert_ok(err, "Creating the sync failed (%d)", err);

	synced_blocks = true;
	sync_established(BT_HCI_ERR_SUCCESS);
	zassert_ok(k_sem_take(&synced_sem, K_SECONDS(1)), "The sync was not established");

	sync_app_start(sync_app_create_release);
	zassert_ok(bt_disable(), "Bluetooth disable failed");
	zassert_true(synced_released, "The sync was not created during bt_disable()");

	zassert_ok(bt_enable(NULL), "Bluetooth init after a disable failed");
	(void)sync_create_established();

	err = sync_app_join();
	zassert_equal(err, -EAGAIN, "Creating the sync during bt_disable() gave %d", err);
}

/* bt_disable() takes down the sync that an application thread is creating,
 * which still waits for the response to the create sync command once a new sync
 * has been created. It must leave that one alone.
 */
static ZTEST(bt_per_adv_sync, test_sync_create_across_disable)
{
	struct bt_le_per_adv_sync *sync;
	int app_err;
	int err;

	sync = sync_app_call_across_disable(sync_app_create, BT_HCI_OP_LE_PER_ADV_CREATE_SYNC);
	app_err = sync_app_join();

	/* The new sync is still being created */
	zassert_true(scan_enabled, "The scanner was stopped");
	err = bt_le_per_adv_sync_create(&sync_param, &sync_app_sync);
	zassert_equal(err, -EBUSY, "Creating another sync gave %d", err);

	sync_established(BT_HCI_ERR_SUCCESS);
	zassert_ok(k_sem_take(&synced_sem, K_SECONDS(1)), "The sync was not established");
	zassert_equal_ptr(synced_sync, sync, "Another sync was established");
	zassert_equal(atomic_get(&term_count), 0, "%ld term callbacks", atomic_get(&term_count));

	zassert_equal(app_err, -ECANCELED, "Creating the sync across bt_disable() gave %d",
		      app_err);
}

/* An application thread deletes a sync, and bt_disable() comes before the call
 * returns, after a new sync has been established. It must leave that one alone.
 */
static ZTEST(bt_per_adv_sync, test_sync_delete_across_disable)
{
	struct bt_le_per_adv_sync *sync;
	int err;

	sync_app_sync = sync_create_established();
	sync = sync_app_call_across_disable(sync_app_delete, BT_HCI_OP_LE_PER_ADV_TERMINATE_SYNC);
	zassert_equal_ptr(sync, sync_app_sync, "The new sync has another object");

	sync_established(BT_HCI_ERR_SUCCESS);
	zassert_ok(k_sem_take(&synced_sem, K_SECONDS(1)), "The sync was not established");
	zassert_equal_ptr(synced_sync, sync, "Another sync was established");

	err = sync_app_join();
	zassert_ok(err, "Deleting the sync across bt_disable() failed (%d)", err);
	zassert_equal(atomic_get(&term_count), 0, "%ld term callbacks", atomic_get(&term_count));

	zassert_ok(bt_le_per_adv_sync_delete(sync), "Deleting the new sync failed");
	zassert_equal(atomic_get(&term_count), 1, "%ld term callbacks", atomic_get(&term_count));
	zassert_equal_ptr(term_sync, sync, "Another sync was terminated");
}

/* As above, with the application thread disabling the reception of reports */
static ZTEST(bt_per_adv_sync, test_sync_recv_disable_across_disable)
{
	struct bt_le_per_adv_sync *sync;
	int err;

	sync_app_sync = sync_create_established();
	sync = sync_app_call_across_disable(sync_app_recv_disable,
					    BT_HCI_OP_LE_SET_PER_ADV_RECV_ENABLE);
	zassert_equal_ptr(sync, sync_app_sync, "The new sync has another object");

	sync_established(BT_HCI_ERR_SUCCESS);
	zassert_ok(k_sem_take(&synced_sem, K_SECONDS(1)), "The sync was not established");
	zassert_equal_ptr(synced_sync, sync, "Another sync was established");

	err = sync_app_join();
	zassert_ok(err, "Disabling reception across bt_disable() failed (%d)", err);
	zassert_equal(atomic_get(&state_changed_count), 0, "%ld state changed callbacks",
		      atomic_get(&state_changed_count));

	/* The new sync still receives reports */
	zassert_ok(bt_le_per_adv_sync_recv_disable(sync), "Disabling reception failed");
	zassert_equal(atomic_get(&state_changed_count), 1, "%ld state changed callbacks",
		      atomic_get(&state_changed_count));
}

/* As sync_app_call_across_disable(), with the application thread waiting for a
 * command buffer, which it gets only once a new sync has been established.
 */
static struct bt_le_per_adv_sync *sync_app_call_waiting_for_buffer(int (*call)(void))
{
	struct net_buf *bufs[CONFIG_BT_BUF_CMD_TX_COUNT];
	struct bt_le_per_adv_sync *sync;

	for (size_t i = 0U; i < ARRAY_SIZE(bufs); i++) {
		bufs[i] = bt_hci_cmd_alloc(K_NO_WAIT);
		zassert_not_null(bufs[i], "No command buffer");
	}

	sync_app_start(call);
	k_sleep(K_MSEC(1));

	/* The first one goes to the waiting application thread */
	for (size_t i = 0U; i < ARRAY_SIZE(bufs); i++) {
		net_buf_unref(bufs[i]);
	}

	zassert_ok(bt_disable(), "Bluetooth disable failed");
	zassert_ok(bt_enable(NULL), "Bluetooth init after a disable failed");
	sent_num = 0U;

	sync = sync_create_established();
	zassert_false(sync_app_returned, "The call returned before the sync was established");

	return sync;
}

/* As test_sync_delete_across_disable, with the application thread waiting for a
 * command buffer. It must not have the new sync terminated either.
 */
static ZTEST(bt_per_adv_sync, test_sync_delete_waiting_for_buffer)
{
	struct bt_le_per_adv_sync *sync;
	int err;

	sync_app_sync = sync_create_established();
	sync = sync_app_call_waiting_for_buffer(sync_app_delete);
	zassert_equal_ptr(sync, sync_app_sync, "The new sync has another object");

	err = sync_app_join();
	zassert_ok(err, "Deleting the sync across bt_disable() failed (%d)", err);
	zassert_equal(sent_count(BT_HCI_OP_LE_PER_ADV_TERMINATE_SYNC), 0U,
		      "The controller was told to terminate a sync");
	zassert_equal(atomic_get(&term_count), 0, "%ld term callbacks", atomic_get(&term_count));

	zassert_ok(bt_le_per_adv_sync_delete(sync), "Deleting the new sync failed");
	zassert_equal(atomic_get(&term_count), 1, "%ld term callbacks", atomic_get(&term_count));
	zassert_equal_ptr(term_sync, sync, "Another sync was terminated");
}

/* As test_sync_recv_disable_across_disable, with the application thread waiting
 * for a command buffer. It must not have the reports of the new sync disabled.
 */
static ZTEST(bt_per_adv_sync, test_sync_recv_disable_waiting_for_buffer)
{
	struct bt_le_per_adv_sync *sync;
	int err;

	sync_app_sync = sync_create_established();
	sync = sync_app_call_waiting_for_buffer(sync_app_recv_disable);
	zassert_equal_ptr(sync, sync_app_sync, "The new sync has another object");

	err = sync_app_join();
	zassert_equal(err, -EINVAL, "Disabling reception across bt_disable() gave %d", err);
	zassert_equal(sent_count(BT_HCI_OP_LE_SET_PER_ADV_RECV_ENABLE), 0U,
		      "The controller was told to disable reception");
	zassert_equal(atomic_get(&state_changed_count), 0, "%ld state changed callbacks",
		      atomic_get(&state_changed_count));

	zassert_ok(bt_le_per_adv_sync_recv_disable(sync), "Disabling reception failed");
	zassert_equal(atomic_get(&state_changed_count), 1, "%ld state changed callbacks",
		      atomic_get(&state_changed_count));
}

/* The controller rejects the create sync command, but only once bt_disable()
 * has taken down the sync that an application thread is creating, and a new
 * sync is being created. The new one is left alone.
 */
static ZTEST(bt_per_adv_sync, test_sync_create_rejected_across_disable)
{
	struct bt_le_per_adv_sync *sync;
	int err;

	slow_status = BT_HCI_ERR_UNSPECIFIED;
	sync = sync_app_call_across_disable(sync_app_create, BT_HCI_OP_LE_PER_ADV_CREATE_SYNC);

	err = sync_app_join();
	zassert_equal(err, -ECANCELED, "Creating the sync across bt_disable() gave %d", err);
	zassert_true(scan_enabled, "The scanner was stopped");

	sync_established(BT_HCI_ERR_SUCCESS);
	zassert_ok(k_sem_take(&synced_sem, K_SECONDS(1)), "The sync was not established");
	zassert_equal_ptr(synced_sync, sync, "Another sync was established");
}

/* An application thread that creates a sync has a command buffer for starting
 * the scanner, but does not get to run until it is resumed.
 */
static void sync_app_create_held(void)
{
	struct net_buf *bufs[CONFIG_BT_BUF_CMD_TX_COUNT];

	for (size_t i = 0U; i < ARRAY_SIZE(bufs); i++) {
		bufs[i] = bt_hci_cmd_alloc(K_NO_WAIT);
		zassert_not_null(bufs[i], "No command buffer");
	}

	sync_app_start(sync_app_create);
	k_sleep(K_MSEC(1));
	k_thread_suspend(&sync_app_thread);

	/* The first one goes to the waiting application thread */
	for (size_t i = 0U; i < ARRAY_SIZE(bufs); i++) {
		net_buf_unref(bufs[i]);
	}
}

/* The application thread runs again only after Bluetooth has been disabled and
 * enabled again. The scanner it then starts must not be left running.
 */
static ZTEST(bt_per_adv_sync, test_sync_create_scanner_across_disable)
{
	int err;

	sync_app_create_held();

	zassert_ok(bt_disable(), "Bluetooth disable failed");
	zassert_ok(bt_enable(NULL), "Bluetooth init after a disable failed");
	zassert_false(scan_enabled, "The scanner was started too early");

	k_thread_resume(&sync_app_thread);

	err = sync_app_join();
	zassert_equal(err, -ECANCELED, "Creating the sync across bt_disable() gave %d", err);
	zassert_false(scan_enabled, "The scanner was left running");
}

/* As test_sync_create_scanner_across_disable, with the application thread
 * running again while bt_enable() is still initializing, before Bluetooth is
 * ready.
 */
static ZTEST(bt_per_adv_sync, test_sync_create_scanner_during_enable)
{
	int err;

	sync_app_create_held();

	zassert_ok(bt_disable(), "Bluetooth disable failed");

	cmd_resume_thread = &sync_app_thread;
	cmd_resume_opcode = BT_HCI_OP_READ_LOCAL_VERSION_INFO;
	zassert_ok(bt_enable(NULL), "Bluetooth init after a disable failed");
	zassert_equal(cmd_resume_opcode, 0U, "The application thread was not resumed");

	err = sync_app_join();
	zassert_equal(err, -ECANCELED, "Creating the sync across bt_disable() gave %d", err);
	zassert_false(scan_enabled, "The scanner was left running");
}

/* An application thread deletes a sync whose creation is pending, and waits
 * for a command buffer while the sync is established. It must not send a
 * cancel, which would cancel any other sync being created by then, but fail as
 * a cancel that comes too late does.
 */
static ZTEST(bt_per_adv_sync, test_sync_cancel_after_established)
{
	struct net_buf *bufs[CONFIG_BT_BUF_CMD_TX_COUNT];
	int err;

	/* The scanner is then not updated when the sync is deleted */
	zassert_ok(bt_le_scan_start(BT_LE_SCAN_PASSIVE, NULL), "Scanning failed to start");

	err = bt_le_per_adv_sync_create(&sync_param, &sync_app_sync);
	zassert_ok(err, "Creating the sync failed (%d)", err);

	for (size_t i = 0U; i < ARRAY_SIZE(bufs); i++) {
		bufs[i] = bt_hci_cmd_alloc(K_NO_WAIT);
		zassert_not_null(bufs[i], "No command buffer");
	}

	sync_app_start(sync_app_delete);
	k_sleep(K_MSEC(1));

	sync_established(BT_HCI_ERR_SUCCESS);
	zassert_ok(k_sem_take(&synced_sem, K_SECONDS(1)), "The sync was not established");
	zassert_false(sync_app_returned, "The delete returned before the sync was established");

	sent_num = 0U;

	for (size_t i = 0U; i < ARRAY_SIZE(bufs); i++) {
		net_buf_unref(bufs[i]);
	}

	err = sync_app_join();
	zassert_equal(err, -EACCES, "Deleting the established sync gave %d", err);
	zassert_equal(sent_count(BT_HCI_OP_LE_PER_ADV_CREATE_SYNC_CANCEL), 0U,
		      "The controller was told to cancel a sync");
	zassert_equal(atomic_get(&term_count), 0, "%ld term callbacks", atomic_get(&term_count));

	zassert_ok(bt_le_per_adv_sync_delete(sync_app_sync), "Deleting the sync failed");
	zassert_equal(atomic_get(&term_count), 1, "%ld term callbacks", atomic_get(&term_count));
	zassert_ok(bt_le_scan_stop(), "Scanning failed to stop");
}

/* The controller reports a sync to another advertising set than the one asked
 * for. The Host terminates that sync before it ends the pending one with the
 * term callback, which can disable and enable Bluetooth, after which the
 * handle could be another sync's.
 */
static ZTEST(bt_per_adv_sync, test_sync_unexpected_terminated_first)
{
	struct bt_le_per_adv_sync *sync;
	int err;

	err = bt_le_per_adv_sync_create(&sync_param, &sync);
	zassert_ok(err, "Creating the sync failed (%d)", err);

	sent_num = 0U;
	sync_established_sid(BT_HCI_ERR_SUCCESS, sync_param.sid + 1U);
	k_sleep(K_MSEC(1));

	zassert_equal(atomic_get(&term_count), 1, "%ld term callbacks", atomic_get(&term_count));
	zassert_equal_ptr(term_sync, sync, "Another sync was terminated");
	zassert_equal(term_terminate_count, 1U, "%u terminate commands before the term callback",
		      term_terminate_count);
	zassert_false(scan_enabled, "The scanner was not stopped");
}

/* A create that the controller rejects leaves the variable for the sync as it
 * was, here with a sync that is established and must still be found there.
 */
static ZTEST(bt_per_adv_sync, test_sync_create_rejected_keeps_sync)
{
	struct bt_le_per_adv_sync *sync;
	int err;

	sync = sync_create_established();
	sync_app_sync = sync;

	slow_opcode = BT_HCI_OP_LE_PER_ADV_CREATE_SYNC;
	slow_rsp_delay = K_MSEC(1);
	slow_status = BT_HCI_ERR_CMD_DISALLOWED;
	err = bt_le_per_adv_sync_create(&sync_param, &sync_app_sync);
	zassert_equal(err, -EACCES, "Creating another sync gave %d", err);
	zassert_equal_ptr(sync_app_sync, sync, "The variable was changed");

	zassert_ok(bt_le_per_adv_sync_delete(sync_app_sync), "Deleting the sync failed");
	zassert_equal(atomic_get(&term_count), 1, "%ld term callbacks", atomic_get(&term_count));
	zassert_equal_ptr(term_sync, sync, "Another sync was terminated");
}

/* The controller reports the sync as established right after its response to
 * the create sync command, before the application thread that creates the sync
 * has returned. The synced callback must find the sync where that call stores
 * it, as the applications look for it there.
 */
static ZTEST(bt_per_adv_sync, test_sync_established_before_create_returns)
{
	int err;

	slow_opcode = BT_HCI_OP_LE_PER_ADV_CREATE_SYNC;
	slow_rsp_delay = K_MSEC(10);
	slow_established = true;
	sync_app_start(sync_app_create);

	zassert_ok(k_sem_take(&synced_sem, K_SECONDS(1)), "The sync was not established");
	zassert_false(sync_app_returned, "Creating the sync returned before it was established");
	zassert_not_null(synced_sync, "No sync was established");
	zassert_equal_ptr(synced_app_sync, synced_sync, "The sync was not stored yet");

	err = sync_app_join();
	zassert_ok(err, "Creating the sync failed (%d)", err);
	zassert_equal_ptr(sync_app_sync, synced_sync, "Another sync was created");
}

/* The sync fails to be established before the application thread that creates
 * it has returned, and the term callback creates a new one, which gets the same
 * object, in the same variable. The call that failed must not take it away.
 */
static ZTEST(bt_per_adv_sync, test_sync_recreated_in_term_callback)
{
	int err;

	slow_opcode = BT_HCI_OP_LE_PER_ADV_CREATE_SYNC;
	slow_rsp_delay = K_MSEC(10);
	slow_established = true;
	slow_established_status = BT_HCI_ERR_CONN_FAIL_TO_ESTAB;
	term_recreates = true;
	sync_app_start(sync_app_create);

	zassert_ok(k_sem_take(&term_recreated_sem, K_SECONDS(1)), "No sync was created again");
	zassert_ok(term_recreate_err, "Creating the sync again failed (%d)", term_recreate_err);

	err = sync_app_join();
	zassert_equal(err, -ECANCELED, "Creating the sync that failed gave %d", err);
	zassert_equal_ptr(sync_app_sync, term_sync, "The new sync was lost");

	sync_established(BT_HCI_ERR_SUCCESS);
	zassert_ok(k_sem_take(&synced_sem, K_SECONDS(1)), "The sync was not established");
	zassert_equal_ptr(synced_sync, sync_app_sync, "Another sync was established");
}

#if defined(CONFIG_BT_ISO_SYNC_RECEIVER)
/* A BIG sync that the Host rejects only once it has the command buffer, as the
 * BIS bitfield names more BISes than there are channels, must give the buffer
 * back.
 */
static ZTEST(bt_per_adv_sync, test_big_sync_invalid_bis)
{
	static struct bt_iso_chan_io_qos rx_qos;
	static struct bt_iso_chan_qos qos = {
		.rx = &rx_qos,
	};
	static struct bt_iso_chan_ops ops;
	static struct bt_iso_chan chans[2] = {
		{
			.ops = &ops,
			.qos = &qos,
		},
		{
			.ops = &ops,
			.qos = &qos,
		},
	};
	struct bt_iso_chan *bis_channels[] = {&chans[0], &chans[1]};
	struct bt_iso_big_sync_param param = {
		.bis_channels = bis_channels,
		.num_bis = 1U,
		.bis_bitfield = BT_ISO_BIS_INDEX_BIT(1) | BT_ISO_BIS_INDEX_BIT(2),
		.sync_timeout = BT_ISO_SYNC_TIMEOUT_MIN,
	};
	struct bt_le_per_adv_sync *sync;
	struct bt_iso_big *big;
	struct net_buf *buf;
	int err;

	sync = sync_create_established();

	for (size_t i = 0U; i < CONFIG_BT_BUF_CMD_TX_COUNT; i++) {
		err = bt_iso_big_sync(sync, &param, &big);
		zassert_equal(err, -EINVAL, "The BIG sync gave %d", err);
	}

	/* Fewer BISes than channels instead */
	param.num_bis = ARRAY_SIZE(bis_channels);
	param.bis_bitfield = BT_ISO_BIS_INDEX_BIT(1);
	err = bt_iso_big_sync(sync, &param, &big);
	zassert_equal(err, -EINVAL, "The BIG sync gave %d", err);

	buf = bt_hci_cmd_alloc(K_NO_WAIT);
	zassert_not_null(buf, "The command buffers were not given back");
	net_buf_unref(buf);
}
#endif /* CONFIG_BT_ISO_SYNC_RECEIVER */

#if defined(CONFIG_BT_PER_ADV_SYNC_RSP)
/* As test_sync_delete_waiting_for_buffer, with the application thread choosing
 * the subevents to synchronize with. It must not do so for the new sync.
 */
static ZTEST(bt_per_adv_sync, test_sync_subevent_waiting_for_buffer)
{
	struct bt_le_per_adv_sync *sync;
	int err;

	sync_app_sync = sync_create_established();
	sync = sync_app_call_waiting_for_buffer(sync_app_subevent);
	zassert_equal_ptr(sync, sync_app_sync, "The new sync has another object");

	err = sync_app_join();
	zassert_equal(err, -EINVAL, "Choosing subevents across bt_disable() gave %d", err);
	zassert_equal(sent_count(BT_HCI_OP_LE_SET_PER_ADV_SYNC_SUBEVENT), 0U,
		      "The controller was told to choose subevents");

	zassert_ok(sync_app_subevent(), "Choosing subevents of the new sync failed");
}
#endif /* CONFIG_BT_PER_ADV_SYNC_RSP */
#endif /* CONFIG_BT_PER_ADV_SYNC */

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

#if defined(CONFIG_BT_PER_ADV_SYNC_TRANSFER_RECEIVER)
/* The peer transfers the periodic advertising sync over the connection */
static void past_received(void)
{
	const struct device *dev = DEVICE_DT_GET(DT_DRV_INST(0));
	struct bt_hci_evt_le_past_received *evt;
	struct bt_hci_evt_le_meta_event *meta;
	struct net_buf *buf;

	buf = bt_buf_get_evt(BT_HCI_EVT_LE_META_EVENT, false, K_FOREVER);
	evt_create(buf, BT_HCI_EVT_LE_META_EVENT, sizeof(*meta) + sizeof(*evt));
	meta = net_buf_add(buf, sizeof(*meta));
	meta->subevent = BT_HCI_EVT_LE_PAST_RECEIVED;
	evt = net_buf_add(buf, sizeof(*evt));
	(void)memset(evt, 0, sizeof(*evt));
	evt->status = BT_HCI_ERR_SUCCESS;
	evt->conn_handle = sys_cpu_to_le16(TEST_CONN_HANDLE);
	evt->sync_handle = sys_cpu_to_le16(TEST_SYNC_HANDLE);
	evt->adv_sid = sync_param.sid;
	bt_addr_le_copy(&evt->addr, &sync_param.addr);
	evt->phy = BT_HCI_LE_PHY_1M;
	evt->interval = sys_cpu_to_le16(BT_HCI_LE_PER_ADV_INTERVAL_MIN);
	bt_hci_recv(dev, buf);
}

/* The peer disconnects */
static void peer_disconnects(void)
{
	const struct device *dev = DEVICE_DT_GET(DT_DRV_INST(0));
	struct bt_hci_evt_disconn_complete *evt;
	struct net_buf *buf;

	buf = bt_buf_get_evt(BT_HCI_EVT_DISCONN_COMPLETE, false, K_FOREVER);
	evt_create(buf, BT_HCI_EVT_DISCONN_COMPLETE, sizeof(*evt));
	evt = net_buf_add(buf, sizeof(*evt));
	evt->status = BT_HCI_ERR_SUCCESS;
	evt->handle = sys_cpu_to_le16(TEST_CONN_HANDLE);
	evt->reason = BT_HCI_ERR_REMOTE_USER_TERM_CONN;
	bt_hci_recv(dev, buf);
}

static const struct bt_le_per_adv_sync_transfer_param past_reports = {
	.timeout = BT_GAP_PER_ADV_MAX_TIMEOUT,
};

static const struct bt_le_per_adv_sync_transfer_param past_no_reports = {
	.timeout = BT_GAP_PER_ADV_MAX_TIMEOUT,
	.options = BT_LE_PER_ADV_SYNC_TRANSFER_OPT_REPORTING_INITIALLY_DISABLED,
};

static int sync_app_past_no_reports(void)
{
	return bt_le_per_adv_sync_transfer_subscribe(test_conn, &past_no_reports);
}

/* An application thread changes how syncs transferred over a connection are
 * received, and the connection is gone by the time the controller rejects the
 * command. The mode that the connection had must not be restored then: the
 * next connection, which has the same connection object, would have it.
 */
static void past_before(void *fixture)
{
	ARG_UNUSED(fixture);

	test_reset();
	per_adv_sync_reset();
}

ZTEST_SUITE(bt_past, NULL, per_adv_sync_setup, past_before, per_adv_sync_after, NULL);

static ZTEST(bt_past, test_past_mode_after_disconnection)
{
	int err;

	k_sem_reset(&disconnected_sem);
	connect_as_peripheral();
	zassert_ok(bt_le_per_adv_sync_transfer_subscribe(test_conn, &past_reports),
		   "Subscribing the connection failed");
	zassert_ok(bt_le_per_adv_sync_transfer_subscribe(NULL, &past_no_reports),
		   "Subscribing by default failed");

	slow_opcode = BT_HCI_OP_LE_PAST_PARAM;
	slow_rsp_delay = K_MSEC(20);
	slow_status = BT_HCI_ERR_UNKNOWN_CONN_ID;
	sync_app_start(sync_app_past_no_reports);
	k_sleep(K_MSEC(10));

	peer_disconnects();
	zassert_ok(k_sem_take(&disconnected_sem, K_SECONDS(1)), "No disconnection");

	err = sync_app_join();
	zassert_equal(err, -EIO, "Subscribing the connection gave %d", err);

	bt_conn_unref(test_conn);
	test_conn = NULL;

	/* The next connection gets syncs with the default mode */
	slow_opcode = 0U;
	connect_as_peripheral();
	past_received();
	zassert_ok(k_sem_take(&synced_sem, K_SECONDS(1)), "No sync was transferred");
	zassert_false(synced_recv_enabled, "Reports are enabled for the transferred sync");

	bt_conn_unref(test_conn);
	test_conn = NULL;
}
#endif /* CONFIG_BT_PER_ADV_SYNC_TRANSFER_RECEIVER */

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
