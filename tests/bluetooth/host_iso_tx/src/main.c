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
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/iso.h>
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

/* The controller takes two ISO data packets of a few octets, so that an SDU
 * longer than one of them is sent in two fragments, and the second one waits
 * while the controller holds an earlier SDU.
 */
#define ISO_BUF_LEN 40U
#define ISO_BUF_NUM 2U

#define ACL_HANDLE 0x0001U
#define CIS_HANDLE 0x0010U

/* An SDU that fits in one ISO data packet, and one that takes two */
#define SHORT_SDU_LEN 10U
#define LONG_SDU_LEN  50U

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
	sys_put_le64(BIT64(BT_LE_FEAT_BIT_CIS_CENTRAL), rp->features);
}

static void le_read_buffer_size_v2(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				   uint16_t opcode)
{
	struct bt_hci_rp_le_read_buffer_size_v2 *rp;

	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	rp->acl_max_len = sys_cpu_to_le16(27U);
	rp->acl_max_num = 3U;
	rp->iso_max_len = sys_cpu_to_le16(ISO_BUF_LEN);
	rp->iso_max_num = ISO_BUF_NUM;
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

static void le_set_cig_params(struct net_buf *buf, struct net_buf **evt, uint8_t len,
			      uint16_t opcode)
{
	const struct bt_hci_cp_le_set_cig_params *cp = (const void *)buf->data;
	struct bt_hci_rp_le_set_cig_params *rp;

	ARG_UNUSED(len);

	zassert_equal(cp->num_cis, 1U, "CIG with %u CIS", cp->num_cis);

	rp = cmd_complete(evt, sizeof(*rp) + sizeof(rp->handle[0]), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	rp->cig_id = cp->cig_id;
	rp->num_handles = 1U;
	rp->handle[0] = sys_cpu_to_le16(CIS_HANDLE);
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
		BT_HCI_OP_LE_READ_BUFFER_SIZE_V2,
		sizeof(struct bt_hci_rp_le_read_buffer_size_v2),
		le_read_buffer_size_v2,
	},
	{
		BT_HCI_OP_LE_READ_SUPP_STATES,
		sizeof(struct bt_hci_rp_le_read_supp_states),
		le_read_supp_states,
	},
	{
		BT_HCI_OP_LE_SET_CIG_PARAMS,
		sizeof(struct bt_hci_rp_le_set_cig_params),
		le_set_cig_params,
	},
};

/* The packet boundary flags of the ISO data packets the controller has been
 * given, in the order they came.
 */
static uint8_t iso_pb[8];
static size_t iso_count;
static K_SEM_DEFINE(iso_sem, 0, ARRAY_SIZE(iso_pb));

static void cmd_handle(const struct device *dev, struct net_buf *cmd)
{
	struct net_buf *evt = NULL;
	struct bt_hci_cmd_hdr *chdr;
	uint16_t opcode;

	chdr = net_buf_pull_mem(cmd, sizeof(*chdr));
	opcode = sys_le16_to_cpu(chdr->opcode);

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

static void iso_handle(struct net_buf *buf)
{
	const struct bt_hci_iso_hdr *hdr = (const void *)buf->data;
	const uint16_t handle = sys_le16_to_cpu(hdr->handle);

	zassert_equal(bt_iso_handle(handle), CIS_HANDLE, "ISO data for handle %u",
		      bt_iso_handle(handle));
	zassert_true(iso_count < ARRAY_SIZE(iso_pb), "Too many ISO data packets");

	iso_pb[iso_count++] = bt_iso_flags_pb(bt_iso_flags(handle));
	k_sem_give(&iso_sem);
}

static int driver_open(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int driver_close(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int driver_send(const struct device *dev, struct net_buf *buf)
{
	uint8_t type = net_buf_pull_u8(buf);

	if (type == BT_HCI_H4_CMD) {
		cmd_handle(dev, buf);
	} else if (type == BT_HCI_H4_ISO) {
		iso_handle(buf);
	} else {
		zassert_unreachable("Unexpected buffer type %u", type);
	}

	net_buf_unref(buf);

	return 0;
}

static DEVICE_API(bt_hci, driver_api) = {
	.open = driver_open,
	.close = driver_close,
	.send = driver_send,
};

#define TEST_DEVICE_INIT(inst)                                                                     \
	static struct bt_hci_driver_data driver_data_##inst = {0};                                 \
	static const struct bt_hci_driver_config driver_config_##inst =                            \
		BT_DT_HCI_DRIVER_CONFIG_INST_GET(inst);                                            \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, &driver_data_##inst, &driver_config_##inst,        \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &driver_api)

DT_INST_FOREACH_STATUS_OKAY(TEST_DEVICE_INIT)

static void le_meta_event(uint8_t subevent, const void *data, uint8_t len)
{
	const struct device *dev = DEVICE_DT_GET(DT_DRV_INST(0));
	struct bt_hci_evt_le_meta_event *meta;
	struct net_buf *buf;

	buf = bt_buf_get_evt(BT_HCI_EVT_LE_META_EVENT, false, K_FOREVER);
	evt_create(buf, BT_HCI_EVT_LE_META_EVENT, sizeof(*meta) + len);
	meta = net_buf_add(buf, sizeof(*meta));
	meta->subevent = subevent;
	net_buf_add_mem(buf, data, len);
	bt_hci_recv(dev, buf);
}

static const bt_addr_le_t peer = {
	.type = BT_ADDR_LE_RANDOM,
	.a.val = {0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0xc6U},
};

static struct bt_conn *acl;
static K_SEM_DEFINE(acl_connected_sem, 0, 1);

static void acl_connected(struct bt_conn *conn, uint8_t err)
{
	zassert_equal(err, 0U, "Connection failed (0x%02x)", err);

	k_sem_give(&acl_connected_sem);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = acl_connected,
};

static void connect_acl(void)
{
	struct bt_hci_evt_le_conn_complete evt = {
		.status = BT_HCI_ERR_SUCCESS,
		.handle = sys_cpu_to_le16(ACL_HANDLE),
		.role = BT_HCI_ROLE_CENTRAL,
		.interval = sys_cpu_to_le16(BT_GAP_INIT_CONN_INT_MIN),
		.supv_timeout = sys_cpu_to_le16(BT_GAP_MS_TO_CONN_TIMEOUT(4000U)),
	};

	zassert_ok(bt_conn_le_create(&peer, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT, &acl),
		   "Connecting failed");

	bt_addr_le_copy(&evt.peer_addr, &peer);
	le_meta_event(BT_HCI_EVT_LE_CONN_COMPLETE, &evt, sizeof(evt));

	zassert_ok(k_sem_take(&acl_connected_sem, K_SECONDS(1)), "No ACL connection");
}

/* What the CIS has reported about the SDUs given to it */
static unsigned int sent_count;
static unsigned int send_failed_count;

static K_SEM_DEFINE(iso_connected_sem, 0, 1);
static K_SEM_DEFINE(iso_disconnected_sem, 0, 1);

static void iso_connected(struct bt_iso_chan *chan)
{
	ARG_UNUSED(chan);

	k_sem_give(&iso_connected_sem);
}

static void iso_disconnected(struct bt_iso_chan *chan, uint8_t reason)
{
	ARG_UNUSED(chan);
	ARG_UNUSED(reason);

	k_sem_give(&iso_disconnected_sem);
}

static void iso_sent(struct bt_iso_chan *chan)
{
	ARG_UNUSED(chan);

	sent_count++;
}

static void iso_send_failed(struct bt_iso_chan *chan, int err)
{
	ARG_UNUSED(chan);

	zassert_equal(err, -ESHUTDOWN, "Send failed with %d", err);
	send_failed_count++;
}

static struct bt_iso_chan_ops iso_ops = {
	.connected = iso_connected,
	.disconnected = iso_disconnected,
	.sent = iso_sent,
	.send_failed = iso_send_failed,
};

static struct bt_iso_chan_io_qos iso_tx_qos = {
	.sdu = CONFIG_BT_ISO_TX_MTU,
	.phy = BT_GAP_LE_PHY_1M,
	.rtn = 1U,
};

static struct bt_iso_chan_qos iso_qos = {
	.tx = &iso_tx_qos,
};

static struct bt_iso_chan iso_chan = {
	.ops = &iso_ops,
	.qos = &iso_qos,
};

static void connect_cis(void)
{
	const struct bt_iso_connect_param param = {
		.acl = acl,
		.iso_chan = &iso_chan,
	};
	struct bt_hci_evt_le_cis_established evt = {
		.status = BT_HCI_ERR_SUCCESS,
		.conn_handle = sys_cpu_to_le16(CIS_HANDLE),
		.c_phy = BT_HCI_LE_PHY_1M,
		.p_phy = BT_HCI_LE_PHY_1M,
		.nse = 1U,
		.c_bn = 1U,
		.c_ft = 1U,
		.p_ft = 1U,
		.c_max_pdu = sys_cpu_to_le16(CONFIG_BT_ISO_TX_MTU),
		.interval = sys_cpu_to_le16(BT_ISO_ISO_INTERVAL_MIN),
	};

	zassert_ok(bt_iso_chan_connect(&param, 1U), "Connecting the CIS failed");

	le_meta_event(BT_HCI_EVT_LE_CIS_ESTABLISHED, &evt, sizeof(evt));

	zassert_ok(k_sem_take(&iso_connected_sem, K_SECONDS(1)), "No CIS");
}

/* The peer takes the CIS down, the ACL connection stays */
static void disconnect_cis(void)
{
	const struct device *dev = DEVICE_DT_GET(DT_DRV_INST(0));
	struct bt_hci_evt_disconn_complete *evt;
	struct net_buf *buf;

	buf = bt_buf_get_evt(BT_HCI_EVT_DISCONN_COMPLETE, false, K_FOREVER);
	evt_create(buf, BT_HCI_EVT_DISCONN_COMPLETE, sizeof(*evt));
	evt = net_buf_add(buf, sizeof(*evt));
	evt->status = BT_HCI_ERR_SUCCESS;
	evt->handle = sys_cpu_to_le16(CIS_HANDLE);
	evt->reason = BT_HCI_ERR_REMOTE_USER_TERM_CONN;
	bt_hci_recv(dev, buf);

	zassert_ok(k_sem_take(&iso_disconnected_sem, K_SECONDS(1)), "The CIS stayed connected");
}

NET_BUF_POOL_FIXED_DEFINE(sdu_pool, CONFIG_BT_ISO_TX_BUF_COUNT,
			  BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static void send_sdu(size_t len)
{
	static uint16_t seq_num;
	struct net_buf *buf;
	int err;

	buf = net_buf_alloc(&sdu_pool, K_NO_WAIT);
	zassert_not_null(buf, "Out of SDU buffers");

	net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);
	(void)memset(net_buf_add(buf, len), 0xAA, len);

	err = bt_iso_chan_send(&iso_chan, buf, seq_num++);
	zassert_ok(err, "Sending an SDU failed (err %d)", err);
}

/* Wait for the controller to have been given this many ISO data packets in
 * all, and no more.
 */
static void expect_iso_packets(size_t count)
{
	while (iso_count < count) {
		zassert_ok(k_sem_take(&iso_sem, K_SECONDS(1)),
			   "The controller was given %zu ISO data packets (!= %zu)", iso_count,
			   count);
	}

	k_sleep(K_MSEC(10));
	zassert_equal(iso_count, count, "The controller was given %zu ISO data packets (!= %zu)",
		      iso_count, count);
}

static void *setup(void)
{
	struct bt_iso_chan *channels[] = {&iso_chan};
	struct bt_iso_cig_param param = {
		.cis_channels = channels,
		.num_cis = ARRAY_SIZE(channels),
		.sca = BT_GAP_SCA_UNKNOWN,
		.c_to_p_latency = 10U,
		.p_to_c_latency = 10U,
		.c_to_p_interval = 10000U,
		.p_to_c_interval = 10000U,
	};
	struct bt_iso_cig *cig;

	zassert_ok(bt_enable(NULL), "Bluetooth init failed");
	connect_acl();
	zassert_ok(bt_iso_cig_create(&param, &cig), "Creating the CIG failed");

	return NULL;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	k_sem_reset(&iso_sem);
	iso_count = 0U;
	sent_count = 0U;
	send_failed_count = 0U;

	connect_cis();
}

static void after(void *fixture)
{
	ARG_UNUSED(fixture);

	if (iso_chan.state != BT_ISO_STATE_DISCONNECTED) {
		disconnect_cis();
	}
}

ZTEST_SUITE(iso_tx, NULL, setup, before, after, NULL);

/* A central keeps its CIS for the next time it connects it. An SDU that the
 * disconnection cut short must not make the first one after that a
 * continuation of it.
 */
static ZTEST(iso_tx, test_partial_sdu_then_reconnect)
{
	/* The first SDU is held by the controller, so the second one gets only
	 * its first fragment out.
	 */
	send_sdu(SHORT_SDU_LEN);
	send_sdu(LONG_SDU_LEN);
	expect_iso_packets(2U);
	zassert_equal(iso_pb[0], BT_ISO_SINGLE, "First SDU sent with PB 0x%02x", iso_pb[0]);
	zassert_equal(iso_pb[1], BT_ISO_START, "Second SDU sent with PB 0x%02x", iso_pb[1]);

	disconnect_cis();
	connect_cis();

	send_sdu(SHORT_SDU_LEN);
	expect_iso_packets(3U);
	zassert_equal(iso_pb[2], BT_ISO_SINGLE, "SDU after reconnecting sent with PB 0x%02x",
		      iso_pb[2]);
}

/* Every SDU that bt_iso_chan_send() took is reported, also the ones still
 * queued in the Host when the CIS is disconnected.
 */
static ZTEST(iso_tx, test_queued_sdus_on_disconnect)
{
	/* One SDU with the controller, one that got its first fragment out and
	 * one that is still queued as a whole.
	 */
	send_sdu(SHORT_SDU_LEN);
	send_sdu(LONG_SDU_LEN);
	send_sdu(SHORT_SDU_LEN);
	expect_iso_packets(2U);

	disconnect_cis();

	zassert_equal(sent_count, 0U, "%u SDUs reported as sent", sent_count);
	zassert_equal(send_failed_count, 3U, "%u SDUs reported as failed (!= 3)",
		      send_failed_count);
}
