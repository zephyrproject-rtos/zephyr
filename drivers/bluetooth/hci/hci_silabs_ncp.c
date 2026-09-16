/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Bluetooth HCI driver for a controller running on a Silicon Labs network
 * co-processor. HCI packets, in H4 framing, are exchanged on a CPC endpoint
 * of the "silabs,ncp" parent device.
 */

#define DT_DRV_COMPAT silabs_ncp_hci

#include <string.h>

#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/drivers/mfd/silabs_ncp.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>

#include "sl_cpc.h"

LOG_MODULE_REGISTER(hci_silabs_ncp, CONFIG_BT_HCI_DRIVER_LOG_LEVEL);

/* Largest H4 packet in each direction, as a CPC payload */
#define HCI_RX_MTU                                                                                 \
	ROUND_UP(MAX(BT_BUF_RX_SIZE, BT_BUF_EVT_SIZE(CONFIG_BT_BUF_EVT_DISCARDABLE_SIZE)),         \
		 SL_CPC_BUF_MIN_ALIGNMENT)
#if defined(CONFIG_BT_CONN)
#define HCI_TX_MTU                                                                                 \
	ROUND_UP(MAX(BT_BUF_CMD_TX_SIZE, BT_BUF_ACL_SIZE(CONFIG_BT_BUF_ACL_TX_SIZE)),              \
		 SL_CPC_BUF_MIN_ALIGNMENT)
#else
#define HCI_TX_MTU ROUND_UP(BT_BUF_CMD_TX_SIZE, SL_CPC_BUF_MIN_ALIGNMENT)
#endif

#define HCI_CONNECT_TIMEOUT K_SECONDS(5)

/* CPC endpoint carrying the HCI packets: the Bluetooth RCP endpoint of CPC */
#define HCI_NCP_EP_ID 14

/* Receive buffer handed to CPC, queued for delivery to the host once filled */
struct hci_ncp_rx_slot {
	sys_snode_t node;
	sl_cpc_buf_t buf;
	uint8_t data[HCI_RX_MTU] __aligned(SL_CPC_BUF_MIN_ALIGNMENT);
};

/* Transmit context, alive until the CPC send-done event */
struct hci_ncp_tx_slot {
	sl_cpc_buf_t buf;
	sl_cpc_frame_t frame;
	uint8_t data[HCI_TX_MTU] __aligned(SL_CPC_BUF_MIN_ALIGNMENT);
};

struct hci_ncp_config {
	struct bt_hci_driver_config common;
	const struct device *parent;
	struct k_mem_slab *tx_slab;
};

struct hci_ncp_data {
	struct bt_hci_driver_data common;
	const struct device *dev;
	sl_cpc_ep_t ep;
	struct k_sem connected;
	struct k_sem closed;
	/* Received packets waiting for a host buffer, oldest first */
	sys_slist_t rx_pending;
	struct k_spinlock rx_lock;
	struct k_work rx_work;
	bool opened;
	struct hci_ncp_rx_slot rx_slots[CONFIG_BT_HCI_SILABS_NCP_RX_COUNT];
};

static bool hci_ncp_evt_discardable(const uint8_t *evt_data, size_t remaining)
{
	if (remaining <= sizeof(struct bt_hci_evt_hdr)) {
		return false;
	}
	if (evt_data[0] != BT_HCI_EVT_LE_META_EVENT) {
		return false;
	}

	switch (evt_data[sizeof(struct bt_hci_evt_hdr)]) {
	case BT_HCI_EVT_LE_ADVERTISING_REPORT:
		return true;
	default:
		return false;
	}
}

/* Build the host buffer of an event. Returns -ENOMEM when the host has no
 * buffer yet, any other error when the packet must be dropped.
 */
static int hci_ncp_evt_recv(const uint8_t *data, size_t remaining, struct net_buf **out)
{
	struct bt_hci_evt_hdr hdr;
	struct net_buf *buf;
	bool discardable;

	if (remaining < sizeof(hdr)) {
		LOG_ERR("Not enough data for event header");
		return -EBADMSG;
	}

	discardable = hci_ncp_evt_discardable(data, remaining);
	memcpy(&hdr, data, sizeof(hdr));
	data += sizeof(hdr);
	remaining -= sizeof(hdr);

	if (remaining != hdr.len) {
		LOG_ERR("Event payload length mismatch (%u != %u)", remaining, hdr.len);
		return -EBADMSG;
	}

	buf = bt_buf_get_evt(hdr.evt, discardable, K_NO_WAIT);
	if (!buf) {
		if (discardable) {
			LOG_DBG("No event buffer, discarding");
			return -ENOBUFS;
		}
		return -ENOMEM;
	}

	net_buf_add_mem(buf, &hdr, sizeof(hdr));
	if (net_buf_tailroom(buf) < remaining) {
		LOG_ERR("Not enough room in event buffer");
		net_buf_unref(buf);
		return -EMSGSIZE;
	}
	net_buf_add_mem(buf, data, remaining);
	*out = buf;

	return 0;
}

static int hci_ncp_acl_recv(const uint8_t *data, size_t remaining, struct net_buf **out)
{
	struct bt_hci_acl_hdr hdr;
	struct net_buf *buf;

	if (remaining < sizeof(hdr)) {
		LOG_ERR("Not enough data for ACL header");
		return -EBADMSG;
	}

	memcpy(&hdr, data, sizeof(hdr));
	data += sizeof(hdr);
	remaining -= sizeof(hdr);

	if (remaining != sys_le16_to_cpu(hdr.len)) {
		LOG_ERR("ACL payload length mismatch");
		return -EBADMSG;
	}

	buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_NO_WAIT);
	if (!buf) {
		return -ENOMEM;
	}

	net_buf_add_mem(buf, &hdr, sizeof(hdr));
	if (net_buf_tailroom(buf) < remaining) {
		LOG_ERR("Not enough room in ACL buffer");
		net_buf_unref(buf);
		return -EMSGSIZE;
	}
	net_buf_add_mem(buf, data, remaining);
	*out = buf;

	return 0;
}

/* Deliver one H4 packet to the host. Returns -ENOMEM when the host has no
 * buffer: the packet must be kept and delivered later. Any other outcome
 * means the CPC buffer can be returned.
 */
static int hci_ncp_deliver(const struct device *dev, const uint8_t *payload, uint16_t len)
{
	struct net_buf *buf = NULL;
	uint8_t h4_type;
	int ret;

	if (len < 1) {
		LOG_WRN("Empty packet, dropping");
		return -EBADMSG;
	}

	h4_type = payload[0];
	payload++;
	len--;

	switch (h4_type) {
	case BT_HCI_H4_EVT:
		ret = hci_ncp_evt_recv(payload, len, &buf);
		break;
	case BT_HCI_H4_ACL:
		ret = hci_ncp_acl_recv(payload, len, &buf);
		break;
	default:
		LOG_ERR("Unknown HCI packet type %u", h4_type);
		return -EBADMSG;
	}

	if (!ret) {
		bt_hci_recv(dev, buf);
	}

	return ret;
}

/* Deliver the pending packets in order, until the host runs out of buffers.
 * Runs in the CPC context and on the system work queue.
 */
static void hci_ncp_rx_process(const struct device *dev)
{
	struct hci_ncp_data *data = dev->data;
	struct hci_ncp_rx_slot *slot;
	k_spinlock_key_t key;
	sys_snode_t *node;
	int ret;

	while (true) {
		key = k_spin_lock(&data->rx_lock);
		node = sys_slist_peek_head(&data->rx_pending);
		k_spin_unlock(&data->rx_lock, key);
		if (!node || !data->opened) {
			return;
		}
		slot = CONTAINER_OF(node, struct hci_ncp_rx_slot, node);

		ret = hci_ncp_deliver(dev, slot->buf.ptr, slot->buf.len);
		if (ret == -ENOMEM) {
			/* bt_buf_rx_freed_cb() resumes us */
			return;
		}

		key = k_spin_lock(&data->rx_lock);
		sys_slist_remove(&data->rx_pending, NULL, node);
		k_spin_unlock(&data->rx_lock, key);
		/* Give the buffer back to CPC */
		sl_cpc_ep_push_recv_buf(&data->ep, &slot->buf);
	}
}

static void hci_ncp_rx_work(struct k_work *work)
{
	struct hci_ncp_data *data = CONTAINER_OF(work, struct hci_ncp_data, rx_work);

	hci_ncp_rx_process(data->dev);
}

/* The host freed a RX buffer, may be in any context */
static const struct device *hci_ncp_rx_freed_dev;

static void hci_ncp_rx_freed(enum bt_buf_type type_mask)
{
	const struct device *dev = hci_ncp_rx_freed_dev;

	ARG_UNUSED(type_mask);

	if (dev) {
		struct hci_ncp_data *data = dev->data;

		k_work_submit(&data->rx_work);
	}
}

/* Runs in the CPC context: keep it short */
static void hci_ncp_ep_event(sl_cpc_ep_t *ep, sl_cpc_ep_event_type_t type,
			     const sl_cpc_ep_event_t *event, void *arg)
{
	const struct device *dev = arg;
	const struct hci_ncp_config *cfg = dev->config;
	struct hci_ncp_data *data = dev->data;
	struct hci_ncp_rx_slot *rx;
	struct hci_ncp_tx_slot *tx;
	k_spinlock_key_t key;

	ARG_UNUSED(ep);

	switch (type) {
	case SL_CPC_EP_EVENT_RECV:
		rx = CONTAINER_OF(event->recv.buf, struct hci_ncp_rx_slot, buf);
		key = k_spin_lock(&data->rx_lock);
		sys_slist_append(&data->rx_pending, &rx->node);
		k_spin_unlock(&data->rx_lock, key);
		hci_ncp_rx_process(dev);
		break;
	case SL_CPC_EP_EVENT_SEND_DONE:
		tx = CONTAINER_OF(event->send_done.frame, struct hci_ncp_tx_slot, frame);
		if (event->send_done.status) {
			LOG_ERR("%s: send failed: %d", dev->name,
				event->send_done.status);
		}
		k_mem_slab_free(cfg->tx_slab, tx);
		break;
	case SL_CPC_EP_EVENT_CONNECTED:
		k_sem_give(&data->connected);
		break;
	case SL_CPC_EP_EVENT_CLOSED:
		k_sem_give(&data->closed);
		break;
	case SL_CPC_EP_EVENT_ERROR:
		LOG_ERR("%s: endpoint error: %d", dev->name,
			event->error.status);
		break;
	default:
		break;
	}
}

static int hci_ncp_send(const struct device *dev, struct net_buf *buf)
{
	const struct hci_ncp_config *cfg = dev->config;
	struct hci_ncp_data *data = dev->data;
	struct hci_ncp_tx_slot *slot;
	int status;
	int ret;

	if (!data->opened) {
		return -ENOTCONN;
	}

	/* The H4 packet type is the first byte of the buffer */
	if (buf->len < 1 || buf->len > HCI_TX_MTU) {
		return -EINVAL;
	}

	ret = k_mem_slab_alloc(cfg->tx_slab, (void **)&slot, K_SECONDS(1));
	if (ret < 0) {
		LOG_ERR("%s: no TX slot", dev->name);
		return -ENOMEM;
	}

	memcpy(slot->data, buf->data, buf->len);
	sl_cpc_buf_init(&slot->buf, slot->data, buf->len);
	memset(&slot->frame, 0, sizeof(slot->frame));

	status = sl_cpc_ep_send(&data->ep, &slot->buf, &slot->frame, NULL);
	if (status) {
		LOG_ERR("%s: send failed: %d", dev->name, status);
		k_mem_slab_free(cfg->tx_slab, slot);
		return -EIO;
	}

	net_buf_unref(buf);

	return 0;
}

static int hci_ncp_open(const struct device *dev)
{
	const struct hci_ncp_config *cfg = dev->config;
	struct hci_ncp_data *data = dev->data;
	int status;

	if (!device_is_ready(cfg->parent)) {
		LOG_ERR("%s: co-processor not ready", dev->name);
		return -ENODEV;
	}

	status =
		sl_cpc_ep_init(&data->ep, HCI_NCP_EP_ID, HCI_RX_MTU, hci_ncp_ep_event, (void *)dev);
	if (status) {
		LOG_ERR("%s: endpoint init failed: %d", dev->name, status);
		return -EIO;
	}

	for (size_t i = 0; i < ARRAY_SIZE(data->rx_slots); i++) {
		struct hci_ncp_rx_slot *slot = &data->rx_slots[i];

		sl_cpc_buf_init(&slot->buf, slot->data, sizeof(slot->data));
		sl_cpc_ep_push_recv_buf(&data->ep, &slot->buf);
	}

	sys_slist_init(&data->rx_pending);
	hci_ncp_rx_freed_dev = dev;
	bt_buf_rx_freed_cb_set(hci_ncp_rx_freed);

	k_sem_reset(&data->connected);
	status = sl_cpc_ep_connect(&data->ep, mfd_silabs_ncp_cpc_bus(cfg->parent));
	if (status) {
		LOG_ERR("%s: endpoint connect failed: %d", dev->name, status);
		bt_buf_rx_freed_cb_set(NULL);
		sl_cpc_ep_deinit(&data->ep);
		return -EIO;
	}

	if (k_sem_take(&data->connected, HCI_CONNECT_TIMEOUT)) {
		LOG_ERR("%s: endpoint connection timed out", dev->name);
		bt_buf_rx_freed_cb_set(NULL);
		k_sem_reset(&data->closed);
		sl_cpc_ep_close(&data->ep);
		if (!k_sem_take(&data->closed, HCI_CONNECT_TIMEOUT)) {
			sl_cpc_ep_deinit(&data->ep);
		}
		return -ETIMEDOUT;
	}

	data->opened = true;
	LOG_INF("%s: HCI endpoint %u connected", dev->name, HCI_NCP_EP_ID);

	return 0;
}

static int hci_ncp_close(const struct device *dev)
{
	struct hci_ncp_data *data = dev->data;

	if (!data->opened) {
		return 0;
	}

	data->opened = false;
	bt_buf_rx_freed_cb_set(NULL);
	k_work_cancel(&data->rx_work);
	k_sem_reset(&data->closed);
	sl_cpc_ep_close(&data->ep);
	if (k_sem_take(&data->closed, HCI_CONNECT_TIMEOUT)) {
		LOG_WRN("%s: endpoint close timed out", dev->name);
		return -ETIMEDOUT;
	}
	sl_cpc_ep_deinit(&data->ep);

	return 0;
}

static int hci_ncp_init(const struct device *dev)
{
	struct hci_ncp_data *data = dev->data;

	data->dev = dev;
	k_sem_init(&data->connected, 0, 1);
	k_sem_init(&data->closed, 0, 1);
	sys_slist_init(&data->rx_pending);
	k_work_init(&data->rx_work, hci_ncp_rx_work);

	return 0;
}

static DEVICE_API(bt_hci, hci_ncp_api) = {
	.open = hci_ncp_open,
	.close = hci_ncp_close,
	.send = hci_ncp_send,
};

#define HCI_NCP_DEFINE(inst)                                                                       \
	K_MEM_SLAB_DEFINE_STATIC(hci_ncp_tx_slab_##inst, sizeof(struct hci_ncp_tx_slot),           \
				 CONFIG_BT_HCI_SILABS_NCP_TX_COUNT, SL_CPC_BUF_MIN_ALIGNMENT);     \
	static struct hci_ncp_data hci_ncp_data_##inst;                                            \
	static const struct hci_ncp_config hci_ncp_config_##inst = {                               \
		.common = BT_DT_HCI_DRIVER_CONFIG_INST_GET(inst),                                  \
		.parent = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                     \
		.tx_slab = &hci_ncp_tx_slab_##inst,                                                \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, hci_ncp_init, NULL, &hci_ncp_data_##inst,                      \
			      &hci_ncp_config_##inst, POST_KERNEL,                                 \
			      CONFIG_BT_HCI_SILABS_NCP_INIT_PRIORITY, &hci_ncp_api);

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) <= 1, "A single HCI controller is supported");

DT_INST_FOREACH_STATUS_OKAY(HCI_NCP_DEFINE)
