/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#define DT_DRV_COMPAT zephyr_bt_hci_test

/* Large enough for the return parameters of initialization commands that are
 * irrelevant to these tests.
 */
#define GENERIC_RP_SIZE 64U

/* The ISO data packets the controller takes, which the Host reads at init
 * although nothing is sent: as many as the Host has TX contexts for.
 */
#define ISO_BUF_LEN 40U
#define ISO_BUF_NUM CONFIG_BT_ISO_TX_BUF_COUNT

/* More than the HCI command pool holds, which has buffers of its own on top of
 * CONFIG_BT_BUF_CMD_TX_COUNT in some configurations.
 */
#define HELD_CMDS_MAX 32U

/* How long to wait for a callback or a thread. Nothing here waits for
 * simulated time to pass, so this only bounds a failed wait.
 */
#define EVENT_TIMEOUT K_SECONDS(1)

/* The handles the controller gives: one PA sync handle per SID and one BIS
 * handle per BIG.
 */
#define PA_SYNC_HANDLE(sid)    (0x0001U + (sid))
#define BIS_HANDLE(big_handle) (0x0100U + (big_handle))

struct cmd_handler {
	uint16_t opcode;
	uint8_t len;
	void (*handler)(struct net_buf *buf, struct net_buf **evt, uint8_t len, uint16_t opcode);
};

/* The handle in the last LE BIG Create Sync, and the BIGs the controller is
 * synchronized to.
 */
static uint8_t created_big_handle;
static bool is_big_synced[CONFIG_BT_ISO_MAX_BIG];

/* The LE BIG Terminate Sync commands the controller has been given, and the
 * handle in the last one.
 */
static size_t terminate_sync_count;
static uint8_t terminated_big_handle;

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

static void cmd_status(struct net_buf **buf, uint8_t status, uint16_t opcode)
{
	struct bt_hci_evt_cmd_status *cs;

	*buf = bt_buf_get_evt(BT_HCI_EVT_CMD_STATUS, false, K_FOREVER);
	evt_create(*buf, BT_HCI_EVT_CMD_STATUS, sizeof(*cs));
	cs = net_buf_add(*buf, sizeof(*cs));
	cs->status = status;
	cs->ncmd = 1U;
	cs->opcode = sys_cpu_to_le16(opcode);
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

	ARGS_UNUSED(buf, len);

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

	ARGS_UNUSED(buf, len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(rp->commands, 0xFF, sizeof(rp->commands));
}

static void read_bd_addr(struct net_buf *buf, struct net_buf **evt, uint8_t len, uint16_t opcode)
{
	struct bt_hci_rp_read_bd_addr *rp;

	ARGS_UNUSED(buf, len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(&rp->bdaddr, 0x11, sizeof(rp->bdaddr));
}

static void le_read_local_features(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				   uint16_t opcode)
{
	struct bt_hci_rp_le_read_local_features *rp;

	ARGS_UNUSED(buf, len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	sys_put_le64(BIT64(BT_LE_FEAT_BIT_EXT_ADV) | BIT64(BT_LE_FEAT_BIT_PER_ADV) |
			     BIT64(BT_LE_FEAT_BIT_SYNC_RECEIVER),
		     rp->features);
}

static void le_read_buffer_size_v2(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				   uint16_t opcode)
{
	struct bt_hci_rp_le_read_buffer_size_v2 *rp;

	ARGS_UNUSED(buf, len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	(void)memset(rp, 0, sizeof(*rp));
	rp->status = BT_HCI_ERR_SUCCESS;
	rp->iso_max_len = sys_cpu_to_le16(ISO_BUF_LEN);
	rp->iso_max_num = ISO_BUF_NUM;
}

static void le_big_create_sync(struct net_buf *buf, struct net_buf **evt, uint8_t len,
			       uint16_t opcode)
{
	const struct bt_hci_cp_le_big_create_sync *cp = (const void *)buf->data;

	ARG_UNUSED(len);

	zassert_true(cp->big_handle < ARRAY_SIZE(is_big_synced), "BIG handle %u", cp->big_handle);
	created_big_handle = cp->big_handle;

	cmd_status(evt, BT_HCI_ERR_SUCCESS, opcode);
}

static void le_big_terminate_sync(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				  uint16_t opcode)
{
	const struct bt_hci_cp_le_big_terminate_sync *cp = (const void *)buf->data;
	struct bt_hci_rp_le_big_terminate_sync *rp;

	ARG_UNUSED(len);

	terminate_sync_count++;
	terminated_big_handle = cp->big_handle;

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->big_handle = cp->big_handle;

	if (cp->big_handle < ARRAY_SIZE(is_big_synced) && is_big_synced[cp->big_handle]) {
		is_big_synced[cp->big_handle] = false;
		rp->status = BT_HCI_ERR_SUCCESS;
	} else {
		/* The handle identifies no BIG (Core 6.3 Vol 4 Part E 7.8.107) */
		rp->status = BT_HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
	}
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
		BT_HCI_OP_LE_BIG_CREATE_SYNC,
		0U,
		le_big_create_sync,
	},
	{
		BT_HCI_OP_LE_BIG_TERMINATE_SYNC,
		sizeof(struct bt_hci_rp_le_big_terminate_sync),
		le_big_terminate_sync,
	},
};

static void cmd_handle(const struct device *dev, struct net_buf *cmd)
{
	struct bt_hci_cmd_hdr *chdr;
	struct net_buf *evt = NULL;
	uint16_t opcode;

	chdr = net_buf_pull_mem(cmd, sizeof(*chdr));
	opcode = sys_le16_to_cpu(chdr->opcode);

	ARRAY_FOR_EACH_PTR(cmds, entry) {
		if (entry->opcode == opcode) {
			entry->handler(cmd, &evt, entry->len, opcode);
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

static int driver_close(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int driver_send(const struct device *dev, struct net_buf *buf)
{
	uint8_t type = net_buf_pull_u8(buf);

	zassert_equal(type, BT_HCI_H4_CMD, "Unexpected buffer type %u", type);
	cmd_handle(dev, buf);
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
	(void)net_buf_add_mem(buf, data, len);
	bt_hci_recv(dev, buf);
}

/* A periodic advertising train with a BIG of one BIS, which the Host is
 * synchronized to.
 */
struct broadcast {
	struct bt_le_per_adv_sync *pa_sync;
	struct bt_iso_big *big;
	struct bt_iso_chan bis;
	/* The handle the Host gave the BIG in LE BIG Create Sync */
	uint8_t big_handle;
	atomic_t stopped_cnt;
};

static K_SEM_DEFINE(pa_synced_sem, 0, 1);
static K_SEM_DEFINE(bis_connected_sem, 0, 1);
static K_SEM_DEFINE(big_stopped_sem, 0, CONFIG_BT_ISO_MAX_BIG);

static void bis_connected(struct bt_iso_chan *chan)
{
	ARG_UNUSED(chan);

	k_sem_give(&bis_connected_sem);
}

static struct bt_iso_chan_ops bis_ops = {
	.connected = bis_connected,
};

static struct bt_iso_chan_io_qos bis_rx_qos;

static struct bt_iso_chan_qos bis_qos = {
	.rx = &bis_rx_qos,
};

/* The BIG that the test terminates and that the controller reports lost, and
 * the other one, which is synchronized to first.
 */
static struct broadcast lost = {
	.bis = { .ops = &bis_ops, .qos = &bis_qos },
};
static struct broadcast kept = {
	.bis = { .ops = &bis_ops, .qos = &bis_qos },
};

static void pa_synced(struct bt_le_per_adv_sync *sync, struct bt_le_per_adv_sync_synced_info *info)
{
	ARGS_UNUSED(sync, info);

	k_sem_give(&pa_synced_sem);
}

static void big_stopped(struct bt_iso_big *big, uint8_t reason)
{
	ARG_UNUSED(reason);

	if (big == lost.big) {
		(void)atomic_inc(&lost.stopped_cnt);
	} else if (big == kept.big) {
		(void)atomic_inc(&kept.stopped_cnt);
	} else {
		/* A BIG the test has not synchronized to */
	}

	k_sem_give(&big_stopped_sem);
}

static void sync_pa(struct broadcast *bc, uint8_t sid)
{
	struct bt_hci_evt_le_per_adv_sync_established evt = {
		.status = BT_HCI_ERR_SUCCESS,
		.handle = sys_cpu_to_le16(PA_SYNC_HANDLE(sid)),
		.sid = sid,
		.phy = BT_HCI_LE_PHY_1M,
		.interval = sys_cpu_to_le16(BT_GAP_PER_ADV_MIN_INTERVAL),
	};
	struct bt_le_per_adv_sync_param param = {
		.sid = sid,
		.timeout = BT_GAP_PER_ADV_MAX_TIMEOUT,
	};
	static const bt_addr_le_t broadcaster = {
		.type = BT_ADDR_LE_PUBLIC,
		.a.val = { 0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U },
	};

	bt_addr_le_copy(&param.addr, &broadcaster);
	zassert_ok(bt_le_per_adv_sync_create(&param, &bc->pa_sync), "Creating the PA sync failed");

	bt_addr_le_copy(&evt.adv_addr, &broadcaster);
	le_meta_event(BT_HCI_EVT_LE_PER_ADV_SYNC_ESTABLISHED, &evt, sizeof(evt));

	zassert_ok(k_sem_take(&pa_synced_sem, EVENT_TIMEOUT), "No PA sync");
}

static void sync_big(struct broadcast *bc)
{
	struct bt_iso_chan *channels[] = { &bc->bis };
	struct bt_iso_big_sync_param param = {
		.bis_channels = channels,
		.num_bis = ARRAY_SIZE(channels),
		.bis_bitfield = BT_ISO_BIS_INDEX_BIT(1),
		.mse = BT_ISO_SYNC_MSE_ANY,
		.sync_timeout = BT_ISO_SYNC_TIMEOUT_MIN,
	};
	struct {
		struct bt_hci_evt_le_big_sync_established evt;
		uint16_t bis_handle;
	} __packed evt = {
		.evt = {
			.status = BT_HCI_ERR_SUCCESS,
			.nse = 1U,
			.bn = 1U,
			.irc = 1U,
			.max_pdu = sys_cpu_to_le16(ISO_BUF_LEN),
			.iso_interval = sys_cpu_to_le16(BT_ISO_ISO_INTERVAL_MIN),
			.num_bis = 1U,
		},
	};

	zassert_ok(bt_iso_big_sync(bc->pa_sync, &param, &bc->big), "Syncing to the BIG failed");
	bc->big_handle = created_big_handle;

	is_big_synced[bc->big_handle] = true;
	evt.evt.big_handle = bc->big_handle;
	evt.bis_handle = sys_cpu_to_le16(BIS_HANDLE(bc->big_handle));
	le_meta_event(BT_HCI_EVT_LE_BIG_SYNC_ESTABLISHED, &evt, sizeof(evt));

	zassert_ok(k_sem_take(&bis_connected_sem, EVENT_TIMEOUT), "No BIS");
}

/* The broadcaster ends the BIG, so the controller reports it lost */
static void big_sync_lost(const struct broadcast *bc)
{
	const struct bt_hci_evt_le_big_sync_lost evt = {
		.big_handle = bc->big_handle,
		.reason = BT_HCI_ERR_REMOTE_USER_TERM_CONN,
	};

	is_big_synced[bc->big_handle] = false;
	le_meta_event(BT_HCI_EVT_LE_BIG_SYNC_LOST, &evt, sizeof(evt));
}

/* The buffers held to keep the HCI command pool empty */
static struct net_buf *held_cmds[HELD_CMDS_MAX];
static size_t held_cmd_count;

static void release_cmds(void)
{
	while (held_cmd_count > 0U) {
		held_cmd_count--;
		net_buf_drop(&held_cmds[held_cmd_count]);
	}
}

/* Empty the HCI command pool, so that a command blocks in bt_hci_cmd_alloc()
 * until the buffers are released.
 */
static void hold_cmds(void)
{
	struct net_buf *buf;

	while (held_cmd_count < ARRAY_SIZE(held_cmds)) {
		buf = bt_hci_cmd_alloc(K_NO_WAIT);
		if (buf == NULL) {
			break;
		}

		held_cmds[held_cmd_count] = buf;
		held_cmd_count++;
	}

	zassert_true(held_cmd_count > 0U && held_cmd_count < ARRAY_SIZE(held_cmds),
		     "Could not empty the command pool (%zu held)", held_cmd_count);
}

static struct k_thread terminator_thread;
static struct k_thread peer_thread;
static bool are_threads_created;

/* What bt_iso_big_terminate() returned in the terminator thread */
static int terminate_err;

static void terminator_fn(void *p1, void *p2, void *p3)
{
	ARGS_UNUSED(p1, p2, p3);

	terminate_err = bt_iso_big_terminate(lost.big);
}

/* Runs only once the terminator has blocked, being the lower priority. The
 * BIG is lost before the terminator gets a command buffer.
 */
static void peer_fn(void *p1, void *p2, void *p3)
{
	ARGS_UNUSED(p1, p2, p3);

	big_sync_lost(&lost);
	zassert_ok(k_sem_take(&big_stopped_sem, EVENT_TIMEOUT), "The lost BIG was not stopped");
	release_cmds();
}

static void broadcast_reset(struct broadcast *bc)
{
	bc->pa_sync = NULL;
	bc->big = NULL;
	(void)atomic_clear(&bc->stopped_cnt);
}

static void *setup(void)
{
	static struct bt_le_per_adv_sync_cb pa_sync_cb = {
		.synced = pa_synced,
	};
	static struct bt_iso_big_cb big_cb = {
		.stopped = big_stopped,
	};

	zassert_ok(bt_le_per_adv_sync_cb_register(&pa_sync_cb), "Registering PA callbacks failed");
	zassert_ok(bt_iso_big_register_cb(&big_cb), "Registering BIG callbacks failed");

	return NULL;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	zassert_ok(bt_enable(NULL), "Bluetooth init failed");

	sync_pa(&kept, 0U);
	sync_big(&kept);
	sync_pa(&lost, 1U);
	sync_big(&lost);
}

static void after(void *fixture)
{
	ARG_UNUSED(fixture);

	if (are_threads_created) {
		k_thread_abort(&terminator_thread);
		k_thread_abort(&peer_thread);
		are_threads_created = false;
	}

	release_cmds();

	zassert_ok(bt_disable(), "Bluetooth disable failed");

	broadcast_reset(&kept);
	broadcast_reset(&lost);

	k_sem_reset(&pa_synced_sem);
	k_sem_reset(&bis_connected_sem);
	k_sem_reset(&big_stopped_sem);
	created_big_handle = 0U;
	(void)memset(is_big_synced, 0, sizeof(is_big_synced));
	terminate_sync_count = 0U;
	terminated_big_handle = 0U;
	terminate_err = 0;
}

ZTEST_SUITE(iso_big_sync, NULL, setup, before, after, NULL);

/* bt_iso_big_terminate() waits for a command buffer, and the BIG is lost in
 * the meantime. The Host must name that BIG in LE BIG Terminate Sync, which
 * the controller refuses, rather than terminate the other BIG, and must
 * report the lost BIG stopped only once.
 */
static ZTEST(iso_big_sync, test_terminate_lost_big)
{
	static K_THREAD_STACK_DEFINE(terminator_stack,
				     CONFIG_ZTEST_STACK_SIZE + CONFIG_TEST_EXTRA_STACK_SIZE);
	static K_THREAD_STACK_DEFINE(peer_stack,
				     CONFIG_ZTEST_STACK_SIZE + CONFIG_TEST_EXTRA_STACK_SIZE);

	hold_cmds();

	are_threads_created = true;
	(void)k_thread_create(&terminator_thread, terminator_stack,
			      K_THREAD_STACK_SIZEOF(terminator_stack), terminator_fn, NULL, NULL,
			      NULL, K_PRIO_PREEMPT(5), 0, K_NO_WAIT);
	(void)k_thread_create(&peer_thread, peer_stack, K_THREAD_STACK_SIZEOF(peer_stack), peer_fn,
			      NULL, NULL, NULL, K_PRIO_PREEMPT(6), 0, K_NO_WAIT);

	zassert_ok(k_thread_join(&terminator_thread, EVENT_TIMEOUT),
		   "bt_iso_big_terminate() did not return");
	zassert_ok(k_thread_join(&peer_thread, EVENT_TIMEOUT), "The peer did not finish");

	zassert_equal(terminate_sync_count, 1U, "%zu LE BIG Terminate Sync commands (!= 1)",
		      terminate_sync_count);
	zassert_equal(terminated_big_handle, lost.big_handle,
		      "LE BIG Terminate Sync for BIG %u, not the lost BIG %u",
		      terminated_big_handle, lost.big_handle);
	zassert_true(is_big_synced[kept.big_handle], "The other BIG was terminated");
	zassert_not_equal(terminate_err, 0, "Terminating the lost BIG succeeded");
	zassert_equal(atomic_get(&lost.stopped_cnt), 1, "The lost BIG stopped %ld times (!= 1)",
		      atomic_get(&lost.stopped_cnt));
	zassert_equal(atomic_get(&kept.stopped_cnt), 0, "The other BIG stopped");
}
