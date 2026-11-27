/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/classic/rfcomm.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/device.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>
#include <zephyr/ztest.h>

#define DT_DRV_COMPAT zephyr_bt_hci_test
#define HCI_DEV DEVICE_DT_GET(DT_INST(0, DT_DRV_COMPAT))
#define HANDLE 1U
#define PEER_CID 0x0040U
#define SIGNAL_CID 0x0001U
#define DLCI 2U
#define TIMEOUT K_SECONDS(5)

K_FIFO_DEFINE(acl_tx);
K_SEM_DEFINE(link_connected, 0, 1);
K_SEM_DEFINE(link_disconnected, 0, 1);
K_SEM_DEFINE(link_recycled, 0, 1);
K_SEM_DEFINE(dlc_connected, 0, 1);
K_SEM_DEFINE(dlc_disconnected, 0, 1);
K_SEM_DEFINE(session_closed, 0, 1);
K_SEM_DEFINE(msc_ready, 0, 1);
K_SEM_DEFINE(received, 0, 1);
K_SEM_DEFINE(peer_credit, 0, 255);
K_SEM_DEFINE(wire_updated, 0, 1);
K_SEM_DEFINE(barrier_received, 0, 1);
K_SEM_DEFINE(complete_requested, 0, 1);
K_SEM_DEFINE(complete_done, 0, 1);

static struct bt_conn *conn;
static struct bt_rfcomm_dlc dlc;
static uint16_t host_cid;
static bool cfc;
static atomic_t monitoring;
static atomic_t credits;
static atomic_t fc_set;
static atomic_t fc_clear;
static struct net_buf *held[256];
static size_t held_count;
static struct net_buf *complete_buf;
static int completion_result;
static int callback_result;
static bool disconnection_observed;

enum receive_mode {
	SYNCHRONOUS,
	DEFERRED,
	EARLY_THREAD,
	EARLY_INLINE,
	COMPLETE_PREVIOUS,
	DUPLICATE,
};

static enum receive_mode mode;

static void wait_for(struct k_sem *sem)
{
	zassert_ok(k_sem_take(sem, TIMEOUT), "Timed out waiting for protocol progress");
}

static struct net_buf *event_create(uint8_t event, uint8_t len)
{
	struct net_buf *buf = bt_buf_get_evt(event, false, TIMEOUT);

	zassert_not_null(buf, "HCI event buffers exhausted");
	net_buf_add_u8(buf, event);
	net_buf_add_u8(buf, len);
	return buf;
}

static void event_send(struct net_buf *buf)
{
	bt_hci_recv(HCI_DEV, buf);
}

static void *command_complete(uint16_t opcode, size_t len, struct net_buf **evt)
{
	*evt = event_create(BT_HCI_EVT_CMD_COMPLETE, 3U + len);
	net_buf_add_u8(*evt, 1U);
	net_buf_add_le16(*evt, opcode);
	return net_buf_add_mem(*evt, (uint8_t[65]){0}, len);
}

static void command_status(uint16_t opcode)
{
	struct net_buf *evt = event_create(BT_HCI_EVT_CMD_STATUS, 4U);

	net_buf_add_u8(evt, BT_HCI_ERR_SUCCESS);
	net_buf_add_u8(evt, 1U);
	net_buf_add_le16(evt, opcode);
	event_send(evt);
}

static void command_handle(struct net_buf *buf)
{
	const struct bt_hci_cmd_hdr *hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	uint16_t opcode = sys_le16_to_cpu(hdr->opcode);
	struct net_buf *evt;

	switch (opcode) {
	case BT_HCI_OP_READ_LOCAL_FEATURES: {
		struct bt_hci_rp_read_local_features *rp =
			command_complete(opcode, sizeof(*rp), &evt);

		rp->features[4] = BIT(6);
		break;
	}
	case BT_HCI_OP_READ_LOCAL_VERSION_INFO: {
		struct bt_hci_rp_read_local_version_info *rp =
			command_complete(opcode, sizeof(*rp), &evt);

		rp->hci_version = BT_HCI_VERSION_4_0;
		rp->lmp_version = BT_HCI_VERSION_4_0;
		break;
	}
	case BT_HCI_OP_READ_SUPPORTED_COMMANDS: {
		struct bt_hci_rp_read_supported_commands *rp =
			command_complete(opcode, sizeof(*rp), &evt);

		rp->commands[10] = BIT(5);
		break;
	}
	case BT_HCI_OP_READ_BD_ADDR: {
		struct bt_hci_rp_read_bd_addr *rp = command_complete(opcode, sizeof(*rp), &evt);

		memset(&rp->bdaddr, 0x11, sizeof(rp->bdaddr));
		break;
	}
	case BT_HCI_OP_READ_BUFFER_SIZE: {
		struct bt_hci_rp_read_buffer_size *rp = command_complete(opcode, sizeof(*rp), &evt);

		rp->acl_max_len = sys_cpu_to_le16(128U);
		rp->acl_max_num = sys_cpu_to_le16(CONFIG_BT_BUF_ACL_TX_COUNT);
		break;
	}
	case BT_HCI_OP_LE_READ_BUFFER_SIZE: {
		struct bt_hci_rp_le_read_buffer_size *rp =
			command_complete(opcode, sizeof(*rp), &evt);

		rp->le_max_len = sys_cpu_to_le16(128U);
		rp->le_max_num = CONFIG_BT_BUF_ACL_TX_COUNT;
		break;
	}
	case BT_HCI_OP_LE_READ_LOCAL_FEATURES:
		command_complete(opcode, sizeof(struct bt_hci_rp_le_read_local_features), &evt);
		break;
	case BT_HCI_OP_LE_READ_SUPP_STATES:
		command_complete(opcode, sizeof(struct bt_hci_rp_le_read_supp_states), &evt);
		break;
	case BT_HCI_OP_READ_DEFAULT_LINK_POLICY_SETTINGS:
		command_complete(opcode, sizeof(struct bt_hci_rp_read_default_link_policy_settings),
				 &evt);
		break;
	case BT_HCI_OP_CONNECT: {
		bt_addr_t addr;

		/* Command Status can synchronously replace the command buffer's contents. */
		memcpy(&addr, buf->data, sizeof(addr));
		command_status(opcode);
		evt = event_create(BT_HCI_EVT_CONN_COMPLETE, 11U);
		net_buf_add_u8(evt, BT_HCI_ERR_SUCCESS);
		net_buf_add_le16(evt, HANDLE);
		net_buf_add_mem(evt, &addr, sizeof(addr));
		net_buf_add_u8(evt, BT_HCI_ACL);
		net_buf_add_u8(evt, 0U);
		break;
	}
	case BT_HCI_OP_READ_REMOTE_FEATURES:
		command_status(opcode);
		evt = event_create(BT_HCI_EVT_REMOTE_FEATURES, 11U);
		net_buf_add_u8(evt, BT_HCI_ERR_SUCCESS);
		net_buf_add_le16(evt, HANDLE);
		net_buf_add_mem(evt, (uint8_t[8]){0}, 8U);
		break;
	case BT_HCI_OP_READ_REMOTE_VERSION_INFO:
		command_status(opcode);
		evt = event_create(BT_HCI_EVT_REMOTE_VERSION_INFO, 8U);
		net_buf_add_u8(evt, BT_HCI_ERR_SUCCESS);
		net_buf_add_le16(evt, HANDLE);
		net_buf_add_u8(evt, BT_HCI_VERSION_4_0);
		net_buf_add_le16(evt, 0U);
		net_buf_add_le16(evt, 0U);
		break;
	case BT_HCI_OP_DISCONNECT:
		command_status(opcode);
		evt = event_create(BT_HCI_EVT_DISCONN_COMPLETE, 4U);
		net_buf_add_u8(evt, BT_HCI_ERR_SUCCESS);
		net_buf_add_le16(evt, HANDLE);
		net_buf_add_u8(evt, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		break;
	case BT_HCI_OP_RESET:
	case BT_HCI_OP_SET_EVENT_MASK:
	case BT_HCI_OP_LE_SET_EVENT_MASK:
	case BT_HCI_OP_LE_WRITE_LE_HOST_SUPP:
	case BT_HCI_OP_WRITE_SSP_MODE:
	case BT_HCI_OP_WRITE_INQUIRY_MODE:
	case BT_HCI_OP_WRITE_LOCAL_NAME:
	case BT_HCI_OP_WRITE_CLASS_OF_DEVICE:
	case BT_HCI_OP_WRITE_PAGE_TIMEOUT:
	case BT_HCI_OP_WRITE_DEFAULT_LINK_POLICY_SETTINGS:
	case BT_HCI_OP_HOST_BUFFER_SIZE:
	case BT_HCI_OP_SET_CTL_TO_HOST_FLOW:
		command_complete(opcode, 1U, &evt);
		break;
	case BT_HCI_OP_HOST_NUM_COMPLETED_PACKETS:
		return;
	default:
		zassert_unreachable("Unsupported simulated HCI command 0x%04x", opcode);
		return;
	}
	event_send(evt);
}

static int driver_open(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

static int driver_send(const struct device *dev, struct net_buf *buf)
{
	uint8_t type = buf->data[0];

	ARG_UNUSED(dev);
	if (type == BT_HCI_H4_CMD) {
		const struct bt_hci_cmd_hdr *hdr = (const void *)&buf->data[1];

		/* Disconnect only after completing all previously transmitted ACL packets. */
		if (sys_le16_to_cpu(hdr->opcode) == BT_HCI_OP_DISCONNECT) {
			k_fifo_put(&acl_tx, buf);
			return 0;
		}
		net_buf_pull_u8(buf);
		command_handle(buf);
		net_buf_unref(buf);
	} else {
		zassert_equal(type, BT_HCI_H4_ACL, "Unexpected HCI packet type %u", type);
		k_fifo_put(&acl_tx, buf);
	}
	return 0;
}

static DEVICE_API(bt_hci, driver_api) = {
	.open = driver_open,
	.send = driver_send,
};

static struct bt_hci_driver_data driver_data;
static const struct bt_hci_driver_config driver_config = BT_DT_HCI_DRIVER_CONFIG_INST_GET(0);

DEVICE_DT_INST_DEFINE(0, NULL, NULL, &driver_data, &driver_config, POST_KERNEL,
		      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &driver_api);

static void acl_send(uint16_t cid, const void *data, size_t len)
{
	struct net_buf *buf = bt_buf_get_rx(BT_BUF_ACL_IN, TIMEOUT);

	zassert_not_null(buf, "ACL receive buffers exhausted");
	net_buf_add_le16(buf, HANDLE | (BT_ACL_START << 12));
	net_buf_add_le16(buf, 4U + len);
	net_buf_add_le16(buf, len);
	net_buf_add_le16(buf, cid);
	net_buf_add_mem(buf, data, len);
	event_send(buf);
}

static void signal_send(uint8_t code, uint8_t ident, const void *data, size_t len)
{
	uint8_t packet[32] = {code, ident};

	zassert_true(len <= sizeof(packet) - 4U);
	sys_put_le16(len, &packet[2]);
	if (len > 0U) {
		memcpy(&packet[4], data, len);
	}
	acl_send(SIGNAL_CID, packet, len + 4U);
}

static void rfcomm_send(uint8_t dlci, uint8_t control, const void *data, size_t len)
{
	uint8_t packet[32] = {(dlci << 2) | 1U, control, (len << 1) | 1U};
	size_t fcs_len = (control & 0xefU) == 0xefU ? 2U : 3U;

	zassert_true(len <= sizeof(packet) - 4U);
	if (fcs_len == 3U) {
		packet[0] |= BIT(1);
	}
	if (len > 0U) {
		memcpy(&packet[3], data, len);
	}
	packet[3U + len] = 0xffU - crc8(packet, fcs_len, CRC8_REFLECT_POLY, 0xffU, true);
	acl_send(host_cid, packet, len + 4U);
}

static void signal_handle(struct net_buf_simple *buf)
{
	uint8_t code = net_buf_simple_pull_u8(buf);
	uint8_t ident = net_buf_simple_pull_u8(buf);
	uint16_t len = net_buf_simple_pull_le16(buf);
	uint8_t reply[12] = {0};

	zassert_equal(len, buf->len);
	switch (code) {
	case 0x02:
		zassert_equal(net_buf_simple_pull_le16(buf), 0x0003U, "Unexpected L2CAP PSM");
		host_cid = net_buf_simple_pull_le16(buf);
		sys_put_le16(PEER_CID, &reply[0]);
		sys_put_le16(host_cid, &reply[2]);
		signal_send(0x03, ident, reply, 8U);
		sys_put_le16(host_cid, &reply[0]);
		sys_put_le16(0U, &reply[2]);
		reply[4] = 1U;
		reply[5] = 2U;
		sys_put_le16(128U, &reply[6]);
		signal_send(0x04, 1U, reply, 8U);
		break;
	case 0x04:
		zassert_equal(net_buf_simple_pull_le16(buf), PEER_CID);
		sys_put_le16(host_cid, &reply[0]);
		signal_send(0x05, ident, reply, 6U);
		break;
	case 0x05:
		zassert_equal(sys_get_le16(&buf->data[4]), 0U, "L2CAP configuration rejected");
		break;
	case 0x06:
		signal_send(0x07, ident, buf->data, buf->len);
		k_sem_give(&session_closed);
		break;
	case 0x0b:
		zassert_equal(ident, 0x7fU);
		zassert_equal(len, 8U);
		zassert_equal(sys_get_le16(buf->data), 0x0002U);
		zassert_equal(sys_get_le16(&buf->data[2]), 0U);
		k_sem_give(&barrier_received);
		break;
	case 0x0a: {
		uint16_t type = sys_get_le16(buf->data);

		memcpy(reply, buf->data, 2U);
		if (type == 0x0002U) {
			signal_send(0x0b, ident, reply, 8U);
		} else {
			zassert_equal(type, 0x0003U, "Unexpected L2CAP information type");
			reply[4] = BIT(1);
			signal_send(0x0b, ident, reply, 12U);
		}
		break;
	}
	default:
		zassert_unreachable("Unexpected L2CAP signal 0x%02x", code);
	}
}

static void rfcomm_handle(struct net_buf_simple *buf)
{
	uint8_t header[3];

	zassert_true(buf->len >= sizeof(header) + 1U);
	memcpy(header, buf->data, sizeof(header));
	uint8_t address = net_buf_simple_pull_u8(buf);
	uint8_t control = net_buf_simple_pull_u8(buf);
	uint8_t len = net_buf_simple_pull_u8(buf) >> 1;
	uint8_t dlci = address >> 2;
	uint8_t reply[16];
	size_t fcs_len = (control & 0xefU) == 0xefU ? 2U : 3U;
	size_t credit_len = fcs_len == 2U && dlci != 0U && (control & BIT(4)) != 0U ? 1U : 0U;

	zassert_equal(buf->len, len + credit_len + 1U);
	zassert_equal(buf->data[buf->len - 1U],
		      0xffU - crc8(header, fcs_len, CRC8_REFLECT_POLY, 0xffU, true));

	if ((control & 0xefU) == 0x2fU || (control & 0xefU) == 0x43U) {
		rfcomm_send(dlci, 0x73U, NULL, 0U);
		return;
	}
	zassert_equal(control & 0xefU, 0xefU, "Unexpected RFCOMM control 0x%02x", control);
	if (dlci != 0U) {
		zassert_equal(dlci, DLCI);
		zassert_true((control & BIT(4)) != 0U, "Expected RFCOMM credits");
		zassert_equal(len, 0U);
		uint8_t granted = net_buf_simple_pull_u8(buf);

		atomic_add(&credits, granted);
		for (uint8_t i = 0U; i < granted; i++) {
			k_sem_give(&peer_credit);
		}
		k_sem_give(&wire_updated);
		return;
	}
	zassert_true(len >= 2U && len <= sizeof(reply));
	memcpy(reply, buf->data, len);
	if ((reply[0] >> 2) == 0x20U) {
		zassert_equal(len, 10U);
		reply[0] &= ~BIT(1);
		reply[3] = cfc ? 0xe0U : 0U;
		if (cfc) {
			for (uint8_t i = 0U; i < reply[9]; i++) {
				k_sem_give(&peer_credit);
			}
		}
		reply[9] = 1U;
		rfcomm_send(0U, 0xefU, reply, len);
	} else if ((reply[0] >> 2) == 0x28U) {
		zassert_equal(len, 2U);
		reply[0] &= ~BIT(1);
		rfcomm_send(0U, 0xefU, reply, len);
	} else {
		zassert_equal(reply[0] >> 2, 0x38U, "Unexpected RFCOMM message");
		zassert_equal(len, 4U);
		if ((reply[0] & BIT(1)) != 0U) {
			if (atomic_get(&monitoring) != 0) {
				atomic_inc((reply[3] & BIT(1)) != 0U ? &fc_set : &fc_clear);
				k_sem_give(&wire_updated);
			}
			reply[0] &= ~BIT(1);
			rfcomm_send(0U, 0xefU, reply, len);
			k_sem_give(&msc_ready);
		}
	}
}

static void peer_thread(void *a, void *b, void *c)
{
	NET_BUF_SIMPLE_DEFINE(packet, BT_HCI_ACL_HDR_SIZE + 128U);

	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);
	while (true) {
		struct net_buf *buf = k_fifo_get(&acl_tx, K_FOREVER);
		struct net_buf *evt;
		uint16_t len;
		uint16_t cid;
		uint8_t type = net_buf_pull_u8(buf);

		if (type == BT_HCI_H4_CMD) {
			command_handle(buf);
			net_buf_unref(buf);
			continue;
		}
		zassert_equal(type, BT_HCI_H4_ACL);
		net_buf_simple_reset(&packet);
		net_buf_simple_add_mem(&packet, buf->data, buf->len);
		net_buf_unref(buf);
		/* Return host TX resources before injecting the peer's response. */
		evt = event_create(BT_HCI_EVT_NUM_COMPLETED_PACKETS, 5U);
		net_buf_add_u8(evt, 1U);
		net_buf_add_le16(evt, HANDLE);
		net_buf_add_le16(evt, 1U);
		event_send(evt);

		zassert_equal(net_buf_simple_pull_le16(&packet) & 0x0fffU, HANDLE);
		len = net_buf_simple_pull_le16(&packet);
		zassert_equal(len, packet.len);
		len = net_buf_simple_pull_le16(&packet);
		zassert_equal(len, packet.len - 2U);
		cid = net_buf_simple_pull_le16(&packet);
		if (cid == SIGNAL_CID) {
			signal_handle(&packet);
		} else {
			zassert_equal(cid, PEER_CID);
			rfcomm_handle(&packet);
		}
	}
}

K_THREAD_DEFINE(peer_tid, 4096, peer_thread, NULL, NULL, NULL, 5, 0, 0);

static void completion_thread(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);
	while (true) {
		k_sem_take(&complete_requested, K_FOREVER);
		completion_result = bt_rfcomm_dlc_recv_complete(&dlc, complete_buf);
		k_sem_give(&complete_done);
	}
}

K_THREAD_DEFINE(completion_tid, 2048, completion_thread, NULL, NULL, NULL, 6, 0, 0);

static int receive_data(struct bt_rfcomm_dlc *channel, struct net_buf *buf)
{
	int err = callback_result;

	zassert_equal(channel, &dlc);
	zassert_equal(buf->len, 1U);
	zassert_equal(buf->data[0], 0x42U);
	switch (mode) {
	case EARLY_THREAD:
		complete_buf = buf;
		k_sem_give(&complete_requested);
		wait_for(&complete_done);
		break;
	case EARLY_INLINE:
	case DUPLICATE:
		zassert_ok(bt_rfcomm_dlc_recv_complete(channel, buf));
		if (mode == DUPLICATE) {
			zassert_equal(bt_rfcomm_dlc_recv_complete(channel, buf), -EALREADY);
		}
		break;
	case COMPLETE_PREVIOUS:
		zassert_equal(held_count, 1U);
		zassert_ok(bt_rfcomm_dlc_recv_complete(channel, held[--held_count]));
		if (err == -EINPROGRESS) {
			held[held_count++] = buf;
		}
		break;
	case DEFERRED:
		zassert_true(held_count < ARRAY_SIZE(held));
		held[held_count++] = buf;
		break;
	case SYNCHRONOUS:
		break;
	}
	k_sem_give(&received);
	return err;
}

static void channel_connected(struct bt_rfcomm_dlc *channel)
{
	zassert_equal(channel, &dlc);
	k_sem_give(&dlc_connected);
}

static void channel_disconnected(struct bt_rfcomm_dlc *channel)
{
	zassert_equal(channel, &dlc);
	k_sem_give(&dlc_disconnected);
}

static struct bt_rfcomm_dlc_ops ops = {
	.connected = channel_connected,
	.disconnected = channel_disconnected,
	.recv = receive_data,
};

static void connected(struct bt_conn *connection, uint8_t err)
{
	ARG_UNUSED(connection);
	zassert_equal(err, 0U);
	k_sem_give(&link_connected);
}

static void disconnected(struct bt_conn *connection, uint8_t reason)
{
	ARG_UNUSED(connection);
	ARG_UNUSED(reason);
	k_sem_give(&link_disconnected);
}

static void recycled(void)
{
	k_sem_give(&link_recycled);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.recycled = recycled,
};

static void *setup(void)
{
	zassert_ok(bt_enable(NULL), "Bluetooth initialization failed");
	dlc.ops = &ops;
	return NULL;
}

static void connect_channel(bool use_cfc)
{
	static const bt_addr_t peer_addr = {{0x22, 0x22, 0x22, 0x22, 0x22, 0x22}};
	struct bt_br_conn_param param = {.allow_role_switch = false};

	cfc = use_cfc;
	atomic_clear(&monitoring);
	host_cid = 0U;
	held_count = 0U;
	mode = SYNCHRONOUS;
	callback_result = 0;
	disconnection_observed = false;
	atomic_clear(&credits);
	atomic_clear(&fc_set);
	atomic_clear(&fc_clear);
	k_sem_reset(&peer_credit);
	k_sem_reset(&msc_ready);
	k_sem_reset(&dlc_disconnected);
	k_sem_reset(&session_closed);
	k_sem_reset(&wire_updated);
	k_sem_reset(&link_recycled);
	dlc.mtu = 128U;
	dlc.rx_credit_limit = 1U;
	conn = bt_conn_create_br(&peer_addr, &param);
	zassert_not_null(conn, "BR/EDR connection allocation failed");
	wait_for(&link_connected);
	zassert_ok(bt_rfcomm_dlc_connect(conn, &dlc, 1U));
	wait_for(&dlc_connected);
	wait_for(&msc_ready);
	atomic_set(&monitoring, 1);
}

static void send_data(void)
{
	static const uint8_t payload = 0x42U;

	if (cfc) {
		wait_for(&peer_credit);
	}
	rfcomm_send(DLCI, 0xefU, &payload, sizeof(payload));
	wait_for(&received);
}

static void barrier(void)
{
	static const uint8_t feature_mask[] = {0x02U, 0x00U};

	signal_send(0x0a, 0x7fU, feature_mask, sizeof(feature_mask));
	wait_for(&barrier_received);
}

static void wait_count(atomic_t *counter, atomic_val_t expected)
{
	while (atomic_get(counter) < expected) {
		wait_for(&wire_updated);
	}
	zassert_equal(atomic_get(counter), expected);
}

static void after(void *fixture)
{
	ARG_UNUSED(fixture);
	while (held_count > 0U) {
		zassert_ok(bt_rfcomm_dlc_recv_complete(&dlc, held[--held_count]));
	}
	if (conn != NULL) {
		if (!disconnection_observed) {
			zassert_ok(bt_rfcomm_dlc_disconnect(&dlc));
			wait_for(&dlc_disconnected);
		}
		wait_for(&session_closed);
		/* Drain the session-close response through RX before link teardown. */
		barrier();
		zassert_ok(bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN));
		wait_for(&link_disconnected);
		bt_conn_unref(conn);
		conn = NULL;
		wait_for(&link_recycled);
	}
}

ZTEST(rfcomm_rx_completion, test_cfc_early_completion_from_thread)
{
	connect_channel(true);
	mode = EARLY_THREAD;
	callback_result = -EINPROGRESS;
	for (unsigned int i = 0U; i < 30U; i++) {
		send_data();
		zassert_ok(completion_result, "Early completion returned %d", completion_result);
		wait_count(&credits, i + 1U);
	}
}

ZTEST(rfcomm_rx_completion, test_non_cfc_early_completion)
{
	connect_channel(false);
	mode = EARLY_INLINE;
	callback_result = -EINPROGRESS;
	for (unsigned int i = 0U; i < 30U; i++) {
		send_data();
	}
	barrier();
	zassert_equal(atomic_get(&fc_set), 0);
	zassert_equal(atomic_get(&fc_clear), 0);
}

ZTEST(rfcomm_rx_completion, test_cfc_deferred_and_synchronous)
{
	connect_channel(true);
	mode = DEFERRED;
	callback_result = -EINPROGRESS;
	send_data();
	barrier();
	zassert_equal(atomic_get(&credits), 0);
	zassert_ok(bt_rfcomm_dlc_recv_complete(&dlc, held[--held_count]));
	wait_count(&credits, 1);
	mode = SYNCHRONOUS;
	callback_result = 0;
	send_data();
	wait_count(&credits, 2);
}

ZTEST(rfcomm_rx_completion, test_non_cfc_more_than_255_held_buffers)
{
	connect_channel(false);
	mode = DEFERRED;
	callback_result = -EINPROGRESS;
	/* Deliver frames already in flight when the peer receives FC=1. */
	for (unsigned int i = 0U; i < ARRAY_SIZE(held); i++) {
		send_data();
	}
	barrier();
	zassert_equal(atomic_get(&fc_set), 1);
	for (unsigned int i = 0U; i < ARRAY_SIZE(held) - 1U; i++) {
		zassert_ok(bt_rfcomm_dlc_recv_complete(&dlc, held[--held_count]));
	}
	barrier();
	zassert_equal(atomic_get(&fc_clear), 0);
	zassert_ok(bt_rfcomm_dlc_recv_complete(&dlc, held[--held_count]));
	wait_count(&fc_clear, 1);
}

static void complete_previous(bool deferred)
{
	connect_channel(false);
	mode = DEFERRED;
	callback_result = -EINPROGRESS;
	send_data();
	wait_count(&fc_set, 1);
	mode = COMPLETE_PREVIOUS;
	callback_result = deferred ? -EINPROGRESS : 0;
	send_data();
	barrier();
	if (deferred) {
		zassert_equal(atomic_get(&fc_clear), 0);
		zassert_ok(bt_rfcomm_dlc_recv_complete(&dlc, held[--held_count]));
	}
	wait_count(&fc_clear, 1);
}

ZTEST(rfcomm_rx_completion, test_complete_previous_during_deferred_recv)
{
	complete_previous(true);
}

ZTEST(rfcomm_rx_completion, test_complete_previous_during_sync_recv)
{
	complete_previous(false);
}

ZTEST(rfcomm_rx_completion, test_duplicate_early_completion)
{
	connect_channel(true);
	mode = DUPLICATE;
	callback_result = -EINPROGRESS;
	send_data();
	wait_count(&credits, 1);
}

static void early_completion_with_error(int err)
{
	connect_channel(true);
	mode = EARLY_INLINE;
	callback_result = err;
	send_data();
	wait_for(&dlc_disconnected);
	disconnection_observed = true;
	zassert_equal(atomic_get(&credits), 0);
}

ZTEST(rfcomm_rx_completion, test_early_completion_without_handoff)
{
	early_completion_with_error(0);
}

ZTEST(rfcomm_rx_completion, test_early_completion_with_callback_error)
{
	early_completion_with_error(-EIO);
}

ZTEST_SUITE(rfcomm_rx_completion, NULL, setup, NULL, after, NULL);
