/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
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
#define PEER_CIS_HANDLE 0x0011U

/* An SDU that fits in one ISO data packet, and one that takes two */
#define SHORT_SDU_LEN 10U
#define LONG_SDU_LEN  50U

/* Upper bound for what the Host does at once with this controller */
#define EVENT_TIMEOUT K_SECONDS(1)

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
	sys_put_le64(BIT64(BT_LE_FEAT_BIT_CIS_CENTRAL) | BIT64(BT_LE_FEAT_BIT_CIS_PERIPHERAL),
		     rp->features);
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

/* BT_HCI_ERR_* status that the controller answers LE Accept CIS Request with */
static uint8_t accept_cis_hci_status;
static K_SEM_DEFINE(accept_cis_sem, 0, 1);

static void le_accept_cis(struct net_buf *buf, struct net_buf **evt, uint8_t len, uint16_t opcode)
{
	struct bt_hci_evt_cmd_status *cs;

	ARGS_UNUSED(buf, len);

	*evt = bt_buf_get_evt(BT_HCI_EVT_CMD_STATUS, false, K_FOREVER);
	evt_create(*evt, BT_HCI_EVT_CMD_STATUS, sizeof(*cs));
	cs = net_buf_add(*evt, sizeof(*cs));
	cs->status = accept_cis_hci_status;
	cs->ncmd = 1U;
	cs->opcode = sys_cpu_to_le16(opcode);

	k_sem_give(&accept_cis_sem);
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
	{
		BT_HCI_OP_LE_ACCEPT_CIS,
		0U,
		le_accept_cis,
	},
};

/* More HCI ISO data packets than any test gives the controller */
#define ISO_PKT_MAX 8U

/* The packet boundary flags of the HCI ISO data packets the controller has
 * been given, in the order they came.
 */
static uint8_t iso_pkt_pb[ISO_PKT_MAX];
static size_t iso_pkt_count;
static K_SEM_DEFINE(iso_pkt_sem, 0, ISO_PKT_MAX);

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

	zassert_true(bt_iso_handle(handle) == CIS_HANDLE ||
			     bt_iso_handle(handle) == PEER_CIS_HANDLE,
		     "ISO data for handle %u", bt_iso_handle(handle));
	zassert_true(iso_pkt_count < ISO_PKT_MAX, "Too many ISO data packets");

	iso_pkt_pb[iso_pkt_count++] = bt_iso_flags_pb(bt_iso_flags(handle));
	k_sem_give(&iso_pkt_sem);
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

static void num_completed_evt(uint16_t handle)
{
	const struct device *dev = DEVICE_DT_GET(DT_DRV_INST(0));
	struct bt_hci_evt_num_completed_packets *evt;
	const size_t evt_len = sizeof(*evt) + sizeof(evt->h[0]);
	struct net_buf *buf;

	buf = bt_buf_get_evt(BT_HCI_EVT_NUM_COMPLETED_PACKETS, false, K_FOREVER);
	evt_create(buf, BT_HCI_EVT_NUM_COMPLETED_PACKETS, evt_len);
	evt = net_buf_add(buf, evt_len);
	evt->num_handles = 1U;
	evt->h[0].handle = sys_cpu_to_le16(handle);
	evt->h[0].count = sys_cpu_to_le16(1U);
	bt_hci_recv(dev, buf);
}

/* What the CIS has reported about the SDUs given to it */
static unsigned int sent_count;
static unsigned int send_failed_count;
static bool should_ack_on_send_failed;
static K_SEM_DEFINE(workq_release_sem, 0, 1);
static bool should_ack_on_sent;
static int disable_err;
static K_SEM_DEFINE(disable_done_sem, 0, 1);
static K_SEM_DEFINE(workq_held_sem, 0, 1);
static int workq_hold_err;
static K_SEM_DEFINE(in_sent_sem, 0, 1);
static K_SEM_DEFINE(next_ack_sem, 0, 1);
static int next_ack_err;

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

	if (should_ack_on_sent) {
		should_ack_on_sent = false;
		num_completed_evt(CIS_HANDLE);
		k_sem_give(&in_sent_sem);
		/* Keeps the TX notification work running until the test thread has
		 * delivered the next acknowledgment
		 */
		next_ack_err = k_sem_take(&next_ack_sem, EVENT_TIMEOUT);
	}
}

static void iso_send_failed(struct bt_iso_chan *chan, int err)
{
	ARG_UNUSED(chan);

	zassert_equal(err, -ESHUTDOWN, "Send failed with %d", err);
	send_failed_count++;
	k_sem_give(&workq_release_sem);

	if (should_ack_on_send_failed) {
		should_ack_on_send_failed = false;
		num_completed_evt(CIS_HANDLE);
	}
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

static void cis_established_evt(uint16_t handle)
{
	const struct bt_hci_evt_le_cis_established evt = {
		.status = BT_HCI_ERR_SUCCESS,
		.conn_handle = sys_cpu_to_le16(handle),
		.c_phy = BT_HCI_LE_PHY_1M,
		.p_phy = BT_HCI_LE_PHY_1M,
		.nse = 1U,
		.c_bn = 1U,
		.p_bn = 1U,
		.c_ft = 1U,
		.p_ft = 1U,
		.c_max_pdu = sys_cpu_to_le16(CONFIG_BT_ISO_TX_MTU),
		.p_max_pdu = sys_cpu_to_le16(CONFIG_BT_ISO_TX_MTU),
		.interval = sys_cpu_to_le16(BT_ISO_ISO_INTERVAL_MIN),
	};

	le_meta_event(BT_HCI_EVT_LE_CIS_ESTABLISHED, &evt, sizeof(evt));
}

static void connect_cis(void)
{
	const struct bt_iso_connect_param param = {
		.acl = acl,
		.iso_chan = &iso_chan,
	};

	zassert_ok(bt_iso_chan_connect(&param, 1U), "Connecting the CIS failed");

	cis_established_evt(CIS_HANDLE);

	zassert_ok(k_sem_take(&iso_connected_sem, K_SECONDS(1)), "No CIS");
}

static void cis_disconn_complete_evt(uint16_t handle)
{
	const struct device *dev = DEVICE_DT_GET(DT_DRV_INST(0));
	struct bt_hci_evt_disconn_complete *evt;
	struct net_buf *buf;

	buf = bt_buf_get_evt(BT_HCI_EVT_DISCONN_COMPLETE, false, K_FOREVER);
	evt_create(buf, BT_HCI_EVT_DISCONN_COMPLETE, sizeof(*evt));
	evt = net_buf_add(buf, sizeof(*evt));
	evt->status = BT_HCI_ERR_SUCCESS;
	evt->handle = sys_cpu_to_le16(handle);
	evt->reason = BT_HCI_ERR_REMOTE_USER_TERM_CONN;
	bt_hci_recv(dev, buf);
}

/* The peer takes the CIS down, the ACL connection stays */
static void disconnect_cis(void)
{
	cis_disconn_complete_evt(CIS_HANDLE);

	zassert_ok(k_sem_take(&iso_disconnected_sem, K_SECONDS(1)), "The CIS stayed connected");
}

NET_BUF_POOL_FIXED_DEFINE(sdu_pool, CONFIG_BT_ISO_TX_BUF_COUNT,
			  BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static void send_sdu(struct bt_iso_chan *chan, size_t len)
{
	static uint16_t seq_num;
	struct net_buf *buf;
	int err;

	buf = net_buf_alloc(&sdu_pool, K_NO_WAIT);
	zassert_not_null(buf, "Out of SDU buffers");

	net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);
	(void)memset(net_buf_add(buf, len), 0xAA, len);

	err = bt_iso_chan_send(chan, buf, seq_num);
	seq_num++;
	zassert_ok(err, "Sending an SDU failed (err %d)", err);
}

/* Wait for the controller to have been given this many ISO data packets in
 * all, and no more.
 */
static void expect_iso_packets(size_t count)
{
	while (iso_pkt_count < count) {
		zassert_ok(k_sem_take(&iso_pkt_sem, K_SECONDS(1)),
			   "The controller was given %zu ISO data packets (!= %zu)", iso_pkt_count,
			   count);
	}

	k_sleep(K_MSEC(10));
	zassert_equal(iso_pkt_count, count,
		      "The controller was given %zu ISO data packets (!= %zu)", iso_pkt_count,
		      count);
}

static void expect_sdu_bufs_free(void)
{
	struct net_buf *bufs[CONFIG_BT_ISO_TX_BUF_COUNT];

	ARRAY_FOR_EACH(bufs, i) {
		bufs[i] = net_buf_alloc(&sdu_pool, K_NO_WAIT);
		zassert_not_null(bufs[i], "%zu SDU buffers are still in use", ARRAY_SIZE(bufs) - i);
	}

	ARRAY_FOR_EACH(bufs, i) {
		net_buf_unref(bufs[i]);
	}
}

/* Bluetooth with an ACL connection and a CIG, until a test stops it */
static void start(void)
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
}

static void stop(void)
{
	zassert_ok(bt_disable(), "Bluetooth disable failed");

	bt_conn_unref(acl);
	acl = NULL;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	if (!bt_is_ready()) {
		start();
	}

	k_sem_reset(&iso_pkt_sem);
	k_sem_reset(&iso_connected_sem);
	k_sem_reset(&iso_disconnected_sem);
	k_sem_reset(&in_sent_sem);
	k_sem_reset(&next_ack_sem);
	k_sem_reset(&disable_done_sem);
	k_sem_reset(&workq_held_sem);
	k_sem_reset(&workq_release_sem);
	iso_pkt_count = 0U;
	sent_count = 0U;
	send_failed_count = 0U;

	connect_cis();
}

static void after(void *fixture)
{
	ARG_UNUSED(fixture);

	should_ack_on_send_failed = false;
	should_ack_on_sent = false;

	if (iso_chan.state != BT_ISO_STATE_DISCONNECTED) {
		disconnect_cis();
	}
}

ZTEST_SUITE(iso_tx, NULL, NULL, before, after, NULL);

/* A central keeps its CIS for the next time it connects it. An SDU that the
 * disconnection cut short must not make the first one after that a
 * continuation of it.
 */
static ZTEST(iso_tx, test_partial_sdu_then_reconnect)
{
	/* The first SDU is held by the controller, so the second one gets only
	 * its first fragment out.
	 */
	send_sdu(&iso_chan, SHORT_SDU_LEN);
	send_sdu(&iso_chan, LONG_SDU_LEN);
	expect_iso_packets(2U);
	zassert_equal(iso_pkt_pb[0], BT_ISO_SINGLE, "First SDU sent with PB 0x%02x", iso_pkt_pb[0]);
	zassert_equal(iso_pkt_pb[1], BT_ISO_START, "Second SDU sent with PB 0x%02x", iso_pkt_pb[1]);

	disconnect_cis();
	connect_cis();

	send_sdu(&iso_chan, SHORT_SDU_LEN);
	expect_iso_packets(3U);
	zassert_equal(iso_pkt_pb[2], BT_ISO_SINGLE, "SDU after reconnecting sent with PB 0x%02x",
		      iso_pkt_pb[2]);
}

/* Every SDU that bt_iso_chan_send() took is reported, also the ones still
 * queued in the Host when the CIS is disconnected.
 */
static ZTEST(iso_tx, test_queued_sdus_on_disconnect)
{
	/* One SDU with the controller, one that got its first fragment out and
	 * one that is still queued as a whole.
	 */
	send_sdu(&iso_chan, SHORT_SDU_LEN);
	send_sdu(&iso_chan, LONG_SDU_LEN);
	send_sdu(&iso_chan, SHORT_SDU_LEN);
	expect_iso_packets(2U);

	disconnect_cis();

	zassert_equal(sent_count, 0U, "%u SDUs reported as sent", sent_count);
	zassert_equal(send_failed_count, 3U, "%u SDUs reported as failed (!= 3)",
		      send_failed_count);
}

/* The same when bt_disable() takes the CIS down, which also leaves no SDU
 * buffer behind.
 */
static ZTEST(iso_tx, test_queued_sdus_on_disable)
{
	send_sdu(&iso_chan, SHORT_SDU_LEN);
	send_sdu(&iso_chan, LONG_SDU_LEN);
	send_sdu(&iso_chan, SHORT_SDU_LEN);
	expect_iso_packets(2U);

	stop();

	zassert_equal(sent_count, 0U, "%u SDUs reported as sent", sent_count);
	zassert_equal(send_failed_count, 3U, "%u SDUs reported as failed (!= 3)",
		      send_failed_count);
	expect_sdu_bufs_free();
}

/* With nothing queued in the Host, bt_disable() still reports the SDU that the
 * controller has not acknowledged.
 */
static ZTEST(iso_tx, test_unacked_sdu_on_disable)
{
	send_sdu(&iso_chan, SHORT_SDU_LEN);
	expect_iso_packets(1U);

	stop();

	zassert_equal(sent_count, 0U, "%u SDUs reported as sent", sent_count);
	zassert_equal(send_failed_count, 1U, "%u SDUs reported as failed (!= 1)",
		      send_failed_count);
}

/* Keeps the system workqueue busy until workq_release_sem is given: by the send_failed
 * callback, or by the test.
 */
static void hold_workq(struct k_work *work)
{
	ARG_UNUSED(work);

	k_sem_give(&workq_held_sem);
	workq_hold_err = k_sem_take(&workq_release_sem, EVENT_TIMEOUT);
}

static K_WORK_DEFINE(hold_workq_work, hold_workq);

/* An SDU the controller has acknowledged is reported as sent, also when bt_disable()
 * gets to the CIS before the TX notification work, which runs on the system workqueue.
 * The workqueue is held until bt_disable() has reported the other SDU as failed.
 */
static ZTEST(iso_tx, test_acked_sdu_on_disable)
{
	send_sdu(&iso_chan, SHORT_SDU_LEN);
	send_sdu(&iso_chan, SHORT_SDU_LEN);
	expect_iso_packets(2U);

	zassert_true(k_work_submit(&hold_workq_work) >= 0, "Holding the workqueue failed");
	zassert_ok(k_sem_take(&workq_held_sem, EVENT_TIMEOUT), "Workqueue not held");
	num_completed_evt(CIS_HANDLE);

	stop();

	zassert_ok(workq_hold_err, "Workqueue not held until an SDU was reported as failed");
	zassert_equal(sent_count, 1U, "%u SDUs reported as sent (!= 1)", sent_count);
	zassert_equal(send_failed_count, 1U, "%u SDUs reported as failed (!= 1)",
		      send_failed_count);
}

/* The controller can still acknowledge an SDU while another one is being reported
 * as failed, and that one has to be reported before the object is released too.
 */
static ZTEST(iso_tx, test_sdu_acked_during_disable)
{
	send_sdu(&iso_chan, SHORT_SDU_LEN);
	send_sdu(&iso_chan, SHORT_SDU_LEN);
	expect_iso_packets(2U);

	should_ack_on_send_failed = true;
	stop();

	zassert_equal(sent_count, 1U, "%u SDUs reported as sent (!= 1)", sent_count);
	zassert_equal(send_failed_count, 1U, "%u SDUs reported as failed (!= 1)",
		      send_failed_count);
}

static void disable_on_workq(struct k_work *work)
{
	ARG_UNUSED(work);

	disable_err = bt_disable();
	k_sem_give(&disable_done_sem);
}

static K_WORK_DEFINE(disable_work, disable_on_workq);

/* The same when bt_disable() runs on the workqueue that the TX notification work is
 * queued on, behind it.
 */
static ZTEST(iso_tx, test_acked_sdu_on_disable_from_workq)
{
	send_sdu(&iso_chan, SHORT_SDU_LEN);
	expect_iso_packets(1U);

	zassert_true(k_work_submit(&disable_work) >= 0, "Submitting bt_disable() failed");
	num_completed_evt(CIS_HANDLE);

	zassert_ok(k_sem_take(&disable_done_sem, EVENT_TIMEOUT), "bt_disable() did not return");
	zassert_ok(disable_err, "Bluetooth disable failed (err %d)", disable_err);
	bt_conn_drop(&acl);

	zassert_equal(sent_count, 1U, "%u SDUs reported as sent (!= 1)", sent_count);
	zassert_equal(send_failed_count, 0U, "%u SDUs reported as failed", send_failed_count);
}

/* An acknowledgment handled from within the TX notification work, here from a sent
 * callback, does not keep the next one from being reported. The callback holds that
 * work until the next acknowledgment has come from the test thread.
 */
static ZTEST(iso_tx, test_ack_from_sent_callback)
{
	/* One more than the controller has buffers for */
	const uint8_t sdu_count = ISO_BUF_NUM + 1U;

	for (uint8_t sdu = 0U; sdu < sdu_count; sdu++) {
		send_sdu(&iso_chan, SHORT_SDU_LEN);
	}
	expect_iso_packets(ISO_BUF_NUM);

	should_ack_on_sent = true;
	num_completed_evt(CIS_HANDLE);
	zassert_ok(k_sem_take(&in_sent_sem, EVENT_TIMEOUT), "First SDU not reported as sent");
	expect_iso_packets(sdu_count);
	num_completed_evt(CIS_HANDLE);
	k_sem_give(&next_ack_sem);

	stop();

	zassert_ok(next_ack_err, "Next acknowledgment not delivered");
	zassert_equal(sent_count, sdu_count, "%u SDUs reported as sent (!= %u)", sent_count,
		      sdu_count);
	zassert_equal(send_failed_count, 0U, "%u SDUs reported as failed", send_failed_count);
}

/* A reference kept to the CIS does not get the disconnected callback called again */
static ZTEST(iso_tx, test_reference_kept_on_disable)
{
	struct bt_conn *kept = bt_conn_ref(iso_chan.iso);

	stop();

	zassert_ok(k_sem_take(&iso_disconnected_sem, EVENT_TIMEOUT),
		   "No disconnected callback for the CIS");
	zassert_is_null(iso_chan.iso, "Channel still attached");

	start();

	zassert_equal(k_sem_count_get(&iso_disconnected_sem), 0U,
		      "Disconnected callback called again");
	bt_conn_unref(kept);
}

static K_SEM_DEFINE(peer_cis_connected_sem, 0, 1);
static K_SEM_DEFINE(peer_cis_disconnected_sem, 0, 1);
static uint8_t peer_cis_reason;

/* How long a sent callback keeps the TX notification work running, which is ample time
 * for bt_disable() to get to the CIS meanwhile
 */
#define SENT_CALLBACK_TIME K_MSEC(100)

static bool should_block_in_peer_sent;
static unsigned int peer_sent_count;
static unsigned int peer_sent_count_at_disconnected;

static void peer_cis_connected(struct bt_iso_chan *chan)
{
	ARG_UNUSED(chan);

	k_sem_give(&peer_cis_connected_sem);
}

static void peer_cis_disconnected(struct bt_iso_chan *chan, uint8_t reason)
{
	ARG_UNUSED(chan);

	peer_cis_reason = reason;
	peer_sent_count_at_disconnected = peer_sent_count;
	k_sem_give(&peer_cis_disconnected_sem);
}

static void peer_cis_sent(struct bt_iso_chan *chan)
{
	ARG_UNUSED(chan);

	if (should_block_in_peer_sent) {
		should_block_in_peer_sent = false;
		k_sem_give(&in_sent_sem);
		(void)k_sleep(SENT_CALLBACK_TIME);
	}

	peer_sent_count++;
}

static struct bt_iso_chan_ops peer_cis_ops = {
	.connected = peer_cis_connected,
	.disconnected = peer_cis_disconnected,
	.sent = peer_cis_sent,
};

static struct bt_iso_chan_io_qos peer_cis_tx_qos;

static struct bt_iso_chan_qos peer_cis_qos = {
	.tx = &peer_cis_tx_qos,
};

static struct bt_iso_chan peer_cis_chan = {
	.ops = &peer_cis_ops,
	.qos = &peer_cis_qos,
};

static int peer_cis_accept(const struct bt_iso_accept_info *info, struct bt_iso_chan **chan)
{
	ARG_UNUSED(info);

	*chan = &peer_cis_chan;

	return 0;
}

static struct bt_iso_server peer_cis_server = {
	.accept = peer_cis_accept,
};

static void cis_accept_before(void *fixture)
{
	ARG_UNUSED(fixture);

	if (!bt_is_ready()) {
		start();
	}

	k_sem_reset(&iso_pkt_sem);
	iso_pkt_count = 0U;
	k_sem_reset(&iso_connected_sem);
	k_sem_reset(&iso_disconnected_sem);
	k_sem_reset(&accept_cis_sem);
	k_sem_reset(&peer_cis_connected_sem);
	k_sem_reset(&peer_cis_disconnected_sem);
	k_sem_reset(&workq_held_sem);
	k_sem_reset(&workq_release_sem);
	k_sem_reset(&in_sent_sem);
	peer_sent_count = 0U;
	zassert_ok(bt_iso_server_register(&peer_cis_server), "Registering the server failed");
}

static void cis_accept_after(void *fixture)
{
	ARG_UNUSED(fixture);

	should_block_in_peer_sent = false;
	zassert_ok(bt_iso_server_unregister(&peer_cis_server), "Unregistering the server failed");
}

ZTEST_SUITE(iso_cis_accept, NULL, NULL, cis_accept_before, cis_accept_after, NULL);

static void request_peer_cis(void)
{
	const struct bt_hci_evt_le_cis_req evt = {
		.acl_handle = sys_cpu_to_le16(ACL_HANDLE),
		.cis_handle = sys_cpu_to_le16(PEER_CIS_HANDLE),
	};

	le_meta_event(BT_HCI_EVT_LE_CIS_REQ, &evt, sizeof(evt));
}

static void establish_peer_cis(void)
{
	accept_cis_hci_status = BT_HCI_ERR_SUCCESS;
	request_peer_cis();
	zassert_ok(k_sem_take(&accept_cis_sem, EVENT_TIMEOUT), "CIS request not accepted");

	cis_established_evt(PEER_CIS_HANDLE);
	zassert_ok(k_sem_take(&peer_cis_connected_sem, EVENT_TIMEOUT), "No CIS from the peer");
}

/* The first fragment of an SDU from the peer */
static void peer_sdu_start_fragment(void)
{
	const struct device *dev = DEVICE_DT_GET(DT_DRV_INST(0));
	struct bt_hci_iso_sdu_hdr *sdu_hdr;
	struct bt_hci_iso_hdr *hdr;
	struct net_buf *buf;

	buf = bt_buf_get_rx(BT_BUF_ISO_IN, K_NO_WAIT);
	zassert_not_null(buf, "No ISO RX buffer");

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->handle = sys_cpu_to_le16(bt_iso_handle_pack(PEER_CIS_HANDLE, BT_ISO_START, 0U));
	hdr->len = sys_cpu_to_le16(sizeof(*sdu_hdr) + SHORT_SDU_LEN);
	sdu_hdr = net_buf_add(buf, sizeof(*sdu_hdr));
	sdu_hdr->sn = 0U;
	sdu_hdr->slen = sys_cpu_to_le16(bt_iso_pkt_len_pack(LONG_SDU_LEN, BT_ISO_DATA_VALID));
	(void)memset(net_buf_add(buf, SHORT_SDU_LEN), 0xAA, SHORT_SDU_LEN);
	bt_hci_recv(dev, buf);
}

/* The application already has the channel when the controller refuses the CIS.
 * It gets the disconnected callback, and the channel is detached from the
 * connection object. Requesting as many times as there are objects shows that
 * each one is free again.
 */
static ZTEST(iso_cis_accept, test_accept_refused)
{
	accept_cis_hci_status = BT_HCI_ERR_CMD_DISALLOWED;

	for (uint8_t round = 0U; round < CONFIG_BT_ISO_MAX_CHAN; round++) {
		request_peer_cis();

		zassert_ok(k_sem_take(&peer_cis_disconnected_sem, EVENT_TIMEOUT),
			   "Round %u: no disconnected callback for the refused CIS", round);
		zassert_equal(peer_cis_reason, BT_HCI_ERR_UNSPECIFIED, "Round %u: reason 0x%02x",
			      round, peer_cis_reason);
		zassert_is_null(peer_cis_chan.iso, "Round %u: channel still attached", round);
		zassert_equal(peer_cis_chan.state, BT_ISO_STATE_DISCONNECTED,
			      "Round %u: channel in state %u", round, peer_cis_chan.state);
	}

	zassert_equal(k_sem_count_get(&peer_cis_connected_sem), 0U,
		      "A refused CIS reported as connected");
}

/* Bringing Bluetooth up again also takes the workqueue past what bt_disable()
 * left for it, which must not call the disconnected callback again.
 */
static ZTEST(iso_cis_accept, test_reference_kept_past_disconnection)
{
	struct bt_conn *kept;

	establish_peer_cis();
	kept = bt_conn_ref(peer_cis_chan.iso);

	cis_disconn_complete_evt(PEER_CIS_HANDLE);
	zassert_ok(k_sem_take(&peer_cis_disconnected_sem, EVENT_TIMEOUT), "CIS stayed connected");
	zassert_is_null(peer_cis_chan.iso, "Channel still attached");

	stop();
	start();

	zassert_equal(k_sem_count_get(&peer_cis_disconnected_sem), 0U,
		      "Disconnected callback called again");
	bt_conn_unref(kept);
}

/* An SDU the controller has no buffer for keeps the connection object on the TX
 * ready list, also after the channel has been detached from it.
 */
static ZTEST(iso_cis_accept, test_sdu_queued_behind_other_cis)
{
	connect_cis();
	establish_peer_cis();

	for (uint8_t sdu = 0U; sdu < ISO_BUF_NUM; sdu++) {
		send_sdu(&iso_chan, SHORT_SDU_LEN);
	}
	expect_iso_packets(ISO_BUF_NUM);
	send_sdu(&peer_cis_chan, SHORT_SDU_LEN);

	cis_disconn_complete_evt(PEER_CIS_HANDLE);
	zassert_ok(k_sem_take(&peer_cis_disconnected_sem, EVENT_TIMEOUT), "CIS stayed connected");
	zassert_is_null(peer_cis_chan.iso, "Channel still attached");

	/* The buffers coming back take the TX processor to that object */
	disconnect_cis();
	expect_iso_packets(ISO_BUF_NUM);
}

/* A refused CIS has nothing to report as sent, so its disconnected callback does not
 * wait for the TX notification work, which runs on the system workqueue held here.
 */
static ZTEST(iso_cis_accept, test_refused_with_workq_held)
{
	struct k_work_sync sync;

	zassert_true(k_work_submit(&hold_workq_work) >= 0, "Holding the workqueue failed");
	zassert_ok(k_sem_take(&workq_held_sem, EVENT_TIMEOUT), "Workqueue not held");

	accept_cis_hci_status = BT_HCI_ERR_CMD_DISALLOWED;
	request_peer_cis();
	zassert_ok(k_sem_take(&peer_cis_disconnected_sem, EVENT_TIMEOUT),
		   "No disconnected callback for the refused CIS");

	k_sem_give(&workq_release_sem);
	(void)k_work_flush(&hold_workq_work, &sync);
	zassert_ok(workq_hold_err, "Workqueue released only by its timeout");
}

/* A refused CIS leaves a released connection object without a channel, and
 * connecting a central CIS goes through every object in the pool.
 */
static ZTEST(iso_cis_accept, test_connect_after_refused)
{
	accept_cis_hci_status = BT_HCI_ERR_CMD_DISALLOWED;
	request_peer_cis();
	zassert_ok(k_sem_take(&peer_cis_disconnected_sem, EVENT_TIMEOUT),
		   "No disconnected callback for the refused CIS");

	connect_cis();
	disconnect_cis();
}

/* Nothing more is reported about an accepted CIS once bt_disable() has reset the
 * controller. The channel gets the disconnected callback, and the connection object
 * releases the ACL connection and is free for the next CIS.
 */
static ZTEST(iso_cis_accept, test_disable_before_established)
{
	accept_cis_hci_status = BT_HCI_ERR_SUCCESS;
	request_peer_cis();
	zassert_ok(k_sem_take(&accept_cis_sem, EVENT_TIMEOUT), "CIS request not accepted");

	stop();

	zassert_ok(k_sem_take(&peer_cis_disconnected_sem, EVENT_TIMEOUT),
		   "No disconnected callback for the CIS");
	zassert_equal(peer_cis_reason, BT_HCI_ERR_UNSPECIFIED, "Reason 0x%02x", peer_cis_reason);
	zassert_is_null(peer_cis_chan.iso, "Channel still attached");
	zassert_equal(peer_cis_chan.state, BT_ISO_STATE_DISCONNECTED, "Channel in state %u",
		      peer_cis_chan.state);

	start();

	accept_cis_hci_status = BT_HCI_ERR_CMD_DISALLOWED;
	request_peer_cis();
	zassert_ok(k_sem_take(&peer_cis_disconnected_sem, EVENT_TIMEOUT),
		   "No connection object for the next CIS");
	zassert_equal(k_sem_count_get(&peer_cis_connected_sem), 0U,
		      "A CIS that was not established reported as connected");
}

/* The same with a reference kept to the object, which the connection cleanup of
 * bt_disable() then finds as well.
 */
static ZTEST(iso_cis_accept, test_disable_with_reference_kept)
{
	struct bt_conn *kept;

	accept_cis_hci_status = BT_HCI_ERR_SUCCESS;
	request_peer_cis();
	zassert_ok(k_sem_take(&accept_cis_sem, EVENT_TIMEOUT), "CIS request not accepted");
	kept = bt_conn_ref(peer_cis_chan.iso);

	stop();

	zassert_ok(k_sem_take(&peer_cis_disconnected_sem, EVENT_TIMEOUT),
		   "No disconnected callback for the CIS");
	zassert_is_null(peer_cis_chan.iso, "Channel still attached");

	start();

	zassert_equal(k_sem_count_get(&peer_cis_disconnected_sem), 0U,
		      "Disconnected callback called again");
	bt_conn_unref(kept);
}

/* A connected CIS from the peer is taken down by bt_disable() as well, with the same
 * reason as a central one.
 */
static ZTEST(iso_cis_accept, test_disable_with_peer_cis_connected)
{
	establish_peer_cis();

	stop();

	zassert_ok(k_sem_take(&peer_cis_disconnected_sem, EVENT_TIMEOUT),
		   "No disconnected callback for the CIS");
	zassert_equal(peer_cis_reason, BT_HCI_ERR_UNSPECIFIED, "Reason 0x%02x", peer_cis_reason);
	zassert_is_null(peer_cis_chan.iso, "Channel still attached");

	start();
}

/* bt_disable() lets a sent callback that is still running finish before the
 * disconnected callback, and before the connection object is released.
 */
static ZTEST(iso_cis_accept, test_disable_with_sent_callback_running)
{
	establish_peer_cis();
	send_sdu(&peer_cis_chan, SHORT_SDU_LEN);
	expect_iso_packets(1U);

	should_block_in_peer_sent = true;
	num_completed_evt(PEER_CIS_HANDLE);
	zassert_ok(k_sem_take(&in_sent_sem, EVENT_TIMEOUT), "SDU not reported as sent");

	stop();

	zassert_ok(k_sem_take(&peer_cis_disconnected_sem, EVENT_TIMEOUT),
		   "No disconnected callback for the CIS");
	zassert_equal(peer_sent_count_at_disconnected, 1U,
		      "Disconnected callback called while the sent callback was running");

	start();
}

/* An SDU from the peer that bt_disable() cuts short leaves no ISO RX buffer behind */
static ZTEST(iso_cis_accept, test_disable_with_partial_sdu_from_peer)
{
	struct net_buf *bufs[CONFIG_BT_ISO_RX_BUF_COUNT];

	establish_peer_cis();
	peer_sdu_start_fragment();
	/* The CIS Established event is handled after the fragment */
	connect_cis();

	stop();

	zassert_ok(k_sem_take(&peer_cis_disconnected_sem, EVENT_TIMEOUT),
		   "No disconnected callback for the CIS");

	ARRAY_FOR_EACH(bufs, i) {
		bufs[i] = bt_buf_get_rx(BT_BUF_ISO_IN, K_NO_WAIT);
		zassert_not_null(bufs[i], "%zu ISO RX buffers are still in use",
				 ARRAY_SIZE(bufs) - i);
	}

	ARRAY_FOR_EACH(bufs, i) {
		net_buf_unref(bufs[i]);
	}

	start();
}
