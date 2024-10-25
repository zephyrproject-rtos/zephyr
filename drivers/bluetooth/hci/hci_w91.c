/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT telink_w91_bt

#include <zephyr/drivers/bluetooth.h>
#include <zephyr/sys/byteorder.h>
#include <ipc/ipc_based_driver.h>
#include <bluetooth/bt_mac.h>

#define LOG_LEVEL CONFIG_BT_HCI_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_hci_driver_w91);

#define HCI_CMD                 0x01
#define HCI_ACL                 0x02
#define HCI_EVT                 0x04

enum {
	IPC_DISPATCHER_BLE_CTRL_OPEN = IPC_DISPATCHER_BLE,
	IPC_DISPATCHER_BLE_CTRL_CLOSE,
	IPC_DISPATCHER_BLE_HCI_HOST_TX,
	IPC_DISPATCHER_BLE_HCI_HOST_RX,
};

enum hci_w91_bt_ctrl_state {
	W91_BT_CTRL_STATE_STOPPED = 0,
	W91_BT_CTRL_STATE_ACTIVATED,
};

struct hci_w91_data {
	uint8_t type;
	uint16_t len;
	uint8_t *buffer;
};

static enum hci_w91_bt_ctrl_state bt_ctrl_state = W91_BT_CTRL_STATE_STOPPED;
static struct ipc_based_driver ipc_data;    /* ipc driver data part */

static const struct device *hci_dev;
static bt_hci_recv_t hci_recv;

/* APIs implementation: open the ble controller */
static size_t pack_hci_w91_open(uint8_t inst, void *unpack_data, uint8_t *pack_data)
{
	uint8_t *p_mac = unpack_data;
	size_t pack_data_len = sizeof(uint32_t) + BT_ADDR_SIZE;

	if (pack_data != NULL) {
		uint32_t id = IPC_DISPATCHER_MK_ID(IPC_DISPATCHER_BLE_CTRL_OPEN, inst);

		IPC_DISPATCHER_PACK_FIELD(pack_data, id);
		IPC_DISPATCHER_PACK_ARRAY(pack_data, p_mac, BT_ADDR_SIZE);
	}

	return pack_data_len;
}

IPC_DISPATCHER_UNPACK_FUNC_ONLY_WITH_ERROR_PARAM(hci_w91_open);

static int hci_w91_open(const struct device *dev, bt_hci_recv_t recv)
{
	LOG_DBG("%s", __func__);

	int err;
	uint8_t mac[BT_ADDR_SIZE];

	if (bt_ctrl_state == W91_BT_CTRL_STATE_ACTIVATED) {
		LOG_ERR("W91 BT has already started");
		return -EPERM;
	}

	telink_bt_blc_mac_init(mac);

	IPC_DISPATCHER_HOST_SEND_DATA(&ipc_data, 0,
			hci_w91_open, mac, &err,
			CONFIG_TELINK_W91_IPC_DISPATCHER_TIMEOUT_MS);

	if (err == 0) {
		bt_ctrl_state = W91_BT_CTRL_STATE_ACTIVATED;
		hci_dev = dev;
		hci_recv = recv;
		LOG_DBG("W91 BT started");
	} else {
		LOG_ERR("W91 BT start failed");
	}

	return err;
}

/* APIs implementation: close the ble controller */
IPC_DISPATCHER_PACK_FUNC_WITHOUT_PARAM(hci_w91_close, IPC_DISPATCHER_BLE_CTRL_CLOSE);

IPC_DISPATCHER_UNPACK_FUNC_ONLY_WITH_ERROR_PARAM(hci_w91_close);

static int hci_w91_close(const struct device *dev)
{
	LOG_DBG("%s", __func__);

	(void) dev;
	int err;

	if (bt_ctrl_state == W91_BT_CTRL_STATE_STOPPED) {
		return -EPERM;
	}

	IPC_DISPATCHER_HOST_SEND_DATA(&ipc_data, 0,
			hci_w91_close, NULL, &err,
			CONFIG_TELINK_W91_IPC_DISPATCHER_TIMEOUT_MS);

	if (err == 0) {
		bt_ctrl_state = W91_BT_CTRL_STATE_STOPPED;
		hci_dev = NULL;
		hci_recv = NULL;
		LOG_DBG("W91 BT stopped");
	} else {
		LOG_ERR("W91 BT stop failed");
	}

	return err;
}

/* APIs implementation: send message to ble controller */
static size_t pack_hci_w91_send(uint8_t inst, void *unpack_data, uint8_t *pack_data)
{
	struct hci_w91_data *p_send_req = unpack_data;
	size_t pack_data_len = sizeof(uint32_t) + sizeof(p_send_req->type) +
			 sizeof(p_send_req->len) + p_send_req->len;

	if (pack_data != NULL) {
		uint32_t id = IPC_DISPATCHER_MK_ID(IPC_DISPATCHER_BLE_HCI_HOST_TX, inst);

		IPC_DISPATCHER_PACK_FIELD(pack_data, id);
		IPC_DISPATCHER_PACK_FIELD(pack_data, p_send_req->type);
		IPC_DISPATCHER_PACK_FIELD(pack_data, p_send_req->len);
		IPC_DISPATCHER_PACK_ARRAY(pack_data, p_send_req->buffer, p_send_req->len);
	}

	return pack_data_len;
}

IPC_DISPATCHER_UNPACK_FUNC_ONLY_WITH_ERROR_PARAM(hci_w91_send);

static int hci_w91_send(const struct device *dev, struct net_buf *buf)
{
	(void) dev;
	int err;
	struct hci_w91_data send_req = {
		.buffer	= (uint8_t *)buf->data,
		.len = buf->len,
	};

	if (bt_ctrl_state == W91_BT_CTRL_STATE_STOPPED) {
		return -EPERM;
	}

	LOG_HEXDUMP_DBG(send_req.buffer, send_req.len, "Sending HCI buffer:");

	switch (bt_buf_get_type(buf)) {
	case BT_BUF_ACL_OUT:
		send_req.type = HCI_ACL;
		break;

	case BT_BUF_CMD:
		send_req.type = HCI_CMD;
		break;

	default:
		LOG_ERR("Unknown type %u", bt_buf_get_type(buf));

		net_buf_unref(buf);
		return -EINVAL;
	}

	IPC_DISPATCHER_HOST_SEND_DATA(&ipc_data, 0,
		hci_w91_send, &send_req, &err,
		CONFIG_TELINK_W91_IPC_DISPATCHER_TIMEOUT_MS);

	net_buf_unref(buf);
	return err;
}

/* APIs implementation: receive message from ble controller */
static bool is_hci_event_discardable(const uint8_t *evt_data)
{
	uint8_t evt_type = evt_data[0];

	switch (evt_type) {
#if defined(CONFIG_BT_CLASSIC)
	case BT_HCI_EVT_INQUIRY_RESULT_WITH_RSSI:
	case BT_HCI_EVT_EXTENDED_INQUIRY_RESULT:
		return true;
#endif
	case BT_HCI_EVT_LE_META_EVENT: {
		uint8_t subevt_type = evt_data[sizeof(struct bt_hci_evt_hdr)];

		switch (subevt_type) {
		case BT_HCI_EVT_LE_ADVERTISING_REPORT:
			return true;
		default:
			return false;
		}
	}
	default:
		return false;
	}
}

static struct net_buf *hci_w91_bt_evt_recv(uint8_t *data, size_t len)
{
	bool discardable;
	struct bt_hci_evt_hdr hdr;
	struct net_buf *buf;
	size_t buf_tailroom;

	if (len < sizeof(hdr)) {
		LOG_ERR("Not enough data for event header");
		return NULL;
	}

	discardable = is_hci_event_discardable(data);

	memcpy((void *)&hdr, data, sizeof(hdr));
	data += sizeof(hdr);
	len -= sizeof(hdr);

	if (len != hdr.len) {
		LOG_ERR("Event payload length is not correct");
		return NULL;
	}
	LOG_DBG("len %u", hdr.len);

	buf = bt_buf_get_evt(hdr.evt, discardable, K_NO_WAIT);
	if (!buf) {
		if (discardable) {
			LOG_DBG("Discardable buffer pool full, ignoring event");
		} else {
			LOG_ERR("No available event buffers!");
		}
		return buf;
	}

	net_buf_add_mem(buf, &hdr, sizeof(hdr));

	buf_tailroom = net_buf_tailroom(buf);
	if (buf_tailroom < len) {
		LOG_ERR("Not enough space in buffer %zu/%zu", len, buf_tailroom);
		net_buf_unref(buf);
		return NULL;
	}

	net_buf_add_mem(buf, data, len);

	return buf;
}

static struct net_buf *hci_w91_bt_acl_recv(uint8_t *data, size_t len)
{
	struct bt_hci_acl_hdr hdr;
	struct net_buf *buf;
	size_t buf_tailroom;

	if (len < sizeof(hdr)) {
		LOG_ERR("Not enough data for ACL header");
		return NULL;
	}

	buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_NO_WAIT);
	if (buf) {
		memcpy((void *)&hdr, data, sizeof(hdr));
		data += sizeof(hdr);
		len -= sizeof(hdr);
	} else {
		LOG_ERR("No available ACL buffers!");
		return NULL;
	}

	if (len != sys_le16_to_cpu(hdr.len)) {
		LOG_ERR("ACL payload length is not correct");
		net_buf_unref(buf);
		return NULL;
	}

	net_buf_add_mem(buf, &hdr, sizeof(hdr));
	buf_tailroom = net_buf_tailroom(buf);
	if (buf_tailroom < len) {
		LOG_ERR("Not enough space in buffer %zu/%zu", len, buf_tailroom);
		net_buf_unref(buf);
		return NULL;
	}

	LOG_DBG("len %u", len);
	net_buf_add_mem(buf, data, len);

	return buf;
}

/* APIs implementation: receive message from ble controller */
static bool unpack_hci_w91_receive(struct hci_w91_data *p_recv_data,
		const uint8_t *pack_data, size_t pack_data_len)
{
	size_t expect_len;

	pack_data += sizeof(uint32_t);
	IPC_DISPATCHER_UNPACK_FIELD(pack_data, p_recv_data->type);
	IPC_DISPATCHER_UNPACK_FIELD(pack_data, p_recv_data->len);

	expect_len = sizeof(uint32_t) + sizeof(p_recv_data->type) +
			sizeof(p_recv_data->len) + p_recv_data->len;
	if (expect_len != pack_data_len) {
		return false;
	}

	p_recv_data->buffer = (uint8_t *)pack_data;

	return true;
}

static void hci_w91_receive(const void *data, size_t len, void *param)
{
	struct hci_w91_data recv_data;
	struct net_buf *buf;

	if (!unpack_hci_w91_receive(&recv_data, data, len)) {
		return;
	}

	switch (recv_data.type) {
	case HCI_EVT:
		LOG_HEXDUMP_DBG(recv_data.buffer, recv_data.len, "host packet event data:");

		buf = hci_w91_bt_evt_recv(recv_data.buffer, recv_data.len);
		break;

	case HCI_ACL:
		LOG_HEXDUMP_DBG(recv_data.buffer, recv_data.len, "host packet acl data:");

		buf = hci_w91_bt_acl_recv(recv_data.buffer, recv_data.len);
		break;

	default:
		LOG_ERR("Unknown HCI type %u", recv_data.type);
		buf = NULL;
	}

	if (buf) {
		LOG_DBG("Calling bt_recv(%p)", buf);
		if (hci_recv && hci_dev) {
			(void) hci_recv(hci_dev, buf);
		} else {
			LOG_ERR("Host reception error, recv: %p dev: %p", hci_recv, hci_dev);
		}
	}
}

static int w91_bt_hci_init(const struct device *dev)
{
	ipc_based_driver_init(&ipc_data);

	ipc_dispatcher_add(IPC_DISPATCHER_MK_ID(IPC_DISPATCHER_BLE_HCI_HOST_RX, 0),
		hci_w91_receive, NULL);

	return 0;
}

static const struct bt_hci_driver_api w91_bt_hci_api = {
	.open  = hci_w91_open,
	.send  = hci_w91_send,
	.close = hci_w91_close,
};

/* BT HCI driver registration */
#define W91_BT_HCI_INIT(n)                                              \
	DEVICE_DT_INST_DEFINE(n, w91_bt_hci_init,                           \
		NULL,                                                           \
		NULL,                                                           \
		NULL,                                                           \
		POST_KERNEL,                                                    \
		CONFIG_TELINK_W91_IPC_DRIVERS_INIT_PRIORITY,                    \
		&w91_bt_hci_api);

DT_INST_FOREACH_STATUS_OKAY(W91_BT_HCI_INIT)

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) > 1
#error only one HCI controller is supported
#endif /* DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) > 1 */
