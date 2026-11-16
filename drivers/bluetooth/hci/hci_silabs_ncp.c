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
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "sl_cpc.h"

LOG_MODULE_REGISTER(hci_silabs_ncp, CONFIG_BT_HCI_DRIVER_LOG_LEVEL);

/* Largest H4 packet in each direction, as a CPC payload */
#define HCI_RX_MTU                                                                                 \
	ROUND_UP(MAX(BT_BUF_RX_SIZE, BT_BUF_EVT_SIZE(CONFIG_BT_BUF_EVT_DISCARDABLE_SIZE)),         \
		 SL_CPC_BUF_MIN_ALIGNMENT)
#if defined(CONFIG_BT_CONN)
#define HCI_TX_ACL_SIZE BT_BUF_ACL_SIZE(CONFIG_BT_BUF_ACL_TX_SIZE)
#else
#define HCI_TX_ACL_SIZE 0
#endif
#if defined(CONFIG_BT_ISO)
#define HCI_TX_ISO_SIZE BT_BUF_ISO_SIZE(CONFIG_BT_ISO_TX_MTU)
#else
#define HCI_TX_ISO_SIZE 0
#endif
#define HCI_TX_MTU                                                                                 \
	ROUND_UP(MAX(BT_BUF_CMD_TX_SIZE, MAX(HCI_TX_ACL_SIZE, HCI_TX_ISO_SIZE)),                   \
		 SL_CPC_BUF_MIN_ALIGNMENT)

/* An event carries up to 255 bytes, a Command Complete event among them. The
 * host takes such an event whatever its buffer sizes, so the receive buffers
 * must hold one together: CPC spreads a payload longer than a buffer over
 * several of them.
 */
BUILD_ASSERT(CONFIG_BT_HCI_SILABS_NCP_RX_COUNT * HCI_RX_MTU >= BT_BUF_EVT_SIZE(UINT8_MAX),
	     "The receive buffers cannot hold the largest HCI event");

/* The co-processor answers the connection and the closing of the endpoint */
#define HCI_CONNECT_TIMEOUT K_SECONDS(5)
#define HCI_CLOSE_TIMEOUT   K_SECONDS(5)

/* A TX slot is busy until the co-processor acknowledges the packet. CPC
 * retransmits an unacknowledged frame every 100 to 500 ms, so a slot held
 * beyond this timeout means more than the loss of a frame on the link, and
 * the send fails rather than queuing more packets behind it.
 */
#define HCI_TX_SLOT_TIMEOUT K_SECONDS(1)

/* CPC endpoint carrying the HCI packets: the Bluetooth RCP endpoint of CPC */
#define HCI_NCP_EP_ID 14

/* Receive buffer handed to CPC, queued for delivery to the host once filled */
struct hci_ncp_rx_slot {
	void *fifo_reserved;
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
	/* Given by the endpoint events, see hci_ncp_ep_event() */
	struct k_sem connected;
	struct k_sem closed;
	sl_status_t ep_error;
	/* Received packets waiting for a host buffer, oldest first */
	struct k_fifo rx_pending;
	struct k_work rx_work;
	/* The packets are delivered to the host */
	bool opened;
	/* The endpoint was closed and CPC has not released it yet */
	bool closing;
	struct hci_ncp_rx_slot rx_slots[CONFIG_BT_HCI_SILABS_NCP_RX_COUNT];
};

/* A received packet is a chain of buffers when it is longer than one: the
 * total length is in the head, the last buffer is the one whose length is the
 * total length left. The H4 type and the HCI header are always in the head,
 * only the last buffer of a chain is shorter than HCI_RX_MTU.
 */
static const sl_cpc_buf_t *hci_ncp_chain_next(const sl_cpc_buf_t *buf)
{
	if (buf->len == buf->tot_len) {
		return NULL;
	}

	return CONTAINER_OF(buf->node.node, const sl_cpc_buf_t, node);
}

/* Append the data of a chain, past its first skip bytes, to a host buffer */
static void hci_ncp_chain_add(struct net_buf *buf, const sl_cpc_buf_t *chain, size_t skip)
{
	const sl_cpc_buf_t *cur;

	for (cur = chain; cur != NULL; cur = hci_ncp_chain_next(cur)) {
		if (skip >= cur->len) {
			skip -= cur->len;
			continue;
		}
		(void)net_buf_add_mem(buf, (const uint8_t *)cur->ptr + skip, cur->len - skip);
		skip = 0U;
	}
}

/* Same policy as the other HCI drivers: the advertising reports the host can
 * lose are taken from the discardable pool and dropped when it is empty, so
 * that a flood of reports never holds back the events behind them. The
 * extended reports that may be fragmented are not discardable.
 */
static bool hci_ncp_evt_discardable(const uint8_t *data, size_t remaining)
{
	const struct bt_hci_evt_hdr *evt = (const void *)data;
	const struct bt_hci_evt_le_meta_event *meta = (const void *)evt->data;
	const struct bt_hci_evt_le_ext_advertising_report *report = (const void *)meta->data;
	uint16_t adv_evt_type;

	if (remaining < sizeof(*evt)) {
		return false;
	}
	remaining -= sizeof(*evt);

	switch (evt->evt) {
#if defined(CONFIG_BT_CLASSIC)
	case BT_HCI_EVT_INQUIRY_RESULT_WITH_RSSI:
	case BT_HCI_EVT_EXTENDED_INQUIRY_RESULT:
		return true;
#endif
	case BT_HCI_EVT_LE_META_EVENT:
		break;
	default:
		return false;
	}

	if (remaining < sizeof(*meta)) {
		return false;
	}
	remaining -= sizeof(*meta);

	switch (meta->subevent) {
	case BT_HCI_EVT_LE_ADVERTISING_REPORT:
		return true;
#if defined(CONFIG_BT_EXT_ADV)
	case BT_HCI_EVT_LE_EXT_ADVERTISING_REPORT:
		if (remaining < sizeof(*report) + sizeof(report->adv_info[0])) {
			return false;
		}
		if (report->num_reports != 1U) {
			return false;
		}
		adv_evt_type = sys_le16_to_cpu(report->adv_info[0].evt_type);
		return (adv_evt_type & BT_HCI_LE_ADV_EVT_TYPE_LEGACY) != 0U;
#endif
	default:
		return false;
	}
}

/* Build the host buffer of an event. Returns -ENOMEM when the host has no
 * buffer yet, any other error when the packet must be dropped.
 */
static int hci_ncp_evt_recv(const sl_cpc_buf_t *chain, struct net_buf **out)
{
	/* Past the H4 type */
	const uint8_t *data = (const uint8_t *)chain->ptr + 1;
	size_t contiguous = chain->len - 1U;
	size_t remaining = chain->tot_len - 1U;
	struct bt_hci_evt_hdr hdr;
	struct net_buf *buf;
	bool discardable;

	if (remaining < sizeof(hdr)) {
		LOG_ERR("Not enough data for event header");
		return -EBADMSG;
	}

	discardable = hci_ncp_evt_discardable(data, contiguous);
	(void)memcpy(&hdr, data, sizeof(hdr));
	remaining -= sizeof(hdr);

	if (remaining != hdr.len) {
		LOG_ERR("Event payload length mismatch (%u != %u)", remaining, hdr.len);
		return -EBADMSG;
	}

	buf = bt_buf_get_evt(hdr.evt, discardable, K_NO_WAIT);
	if (buf == NULL) {
		if (discardable) {
			LOG_DBG("No event buffer, discarding");
			return -ENOBUFS;
		}
		return -ENOMEM;
	}

	(void)net_buf_add_mem(buf, &hdr, sizeof(hdr));
	if (net_buf_tailroom(buf) < remaining) {
		LOG_ERR("Not enough room in event buffer");
		net_buf_unref(buf);
		return -EMSGSIZE;
	}
	hci_ncp_chain_add(buf, chain, 1U + sizeof(hdr));
	*out = buf;

	return 0;
}

static int hci_ncp_acl_recv(const sl_cpc_buf_t *chain, struct net_buf **out)
{
	const uint8_t *data = (const uint8_t *)chain->ptr + 1;
	size_t remaining = chain->tot_len - 1U;
	struct bt_hci_acl_hdr hdr;
	struct net_buf *buf;

	if (remaining < sizeof(hdr)) {
		LOG_ERR("Not enough data for ACL header");
		return -EBADMSG;
	}

	(void)memcpy(&hdr, data, sizeof(hdr));
	remaining -= sizeof(hdr);

	if (remaining != sys_le16_to_cpu(hdr.len)) {
		LOG_ERR("ACL payload length mismatch");
		return -EBADMSG;
	}

	buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_NO_WAIT);
	if (buf == NULL) {
		return -ENOMEM;
	}

	(void)net_buf_add_mem(buf, &hdr, sizeof(hdr));
	if (net_buf_tailroom(buf) < remaining) {
		LOG_ERR("Not enough room in ACL buffer");
		net_buf_unref(buf);
		return -EMSGSIZE;
	}
	hci_ncp_chain_add(buf, chain, 1U + sizeof(hdr));
	*out = buf;

	return 0;
}

/* An ISO packet is only worth delivering on time: when the host has no buffer
 * for it, it is dropped rather than held back with the packets behind it.
 */
static int hci_ncp_iso_recv(const sl_cpc_buf_t *chain, struct net_buf **out)
{
	const uint8_t *data = (const uint8_t *)chain->ptr + 1;
	size_t remaining = chain->tot_len - 1U;
	struct bt_hci_iso_hdr hdr;
	struct net_buf *buf;

	if (remaining < sizeof(hdr)) {
		LOG_ERR("Not enough data for ISO header");
		return -EBADMSG;
	}

	(void)memcpy(&hdr, data, sizeof(hdr));
	remaining -= sizeof(hdr);

	if (remaining != bt_iso_hdr_len(sys_le16_to_cpu(hdr.len))) {
		LOG_ERR("ISO payload length mismatch");
		return -EBADMSG;
	}

	buf = bt_buf_get_rx(BT_BUF_ISO_IN, K_NO_WAIT);
	if (buf == NULL) {
		LOG_DBG("No ISO buffer, discarding");
		return -ENOBUFS;
	}

	(void)net_buf_add_mem(buf, &hdr, sizeof(hdr));
	if (net_buf_tailroom(buf) < remaining) {
		LOG_ERR("Not enough room in ISO buffer");
		net_buf_unref(buf);
		return -EMSGSIZE;
	}
	hci_ncp_chain_add(buf, chain, 1U + sizeof(hdr));
	*out = buf;

	return 0;
}

/* Deliver one H4 packet to the host. Returns -ENOMEM when the host has no
 * buffer: the packet must be kept and delivered later. Any other outcome
 * means the CPC buffers can be returned.
 */
static int hci_ncp_deliver(const struct device *dev, const sl_cpc_buf_t *chain)
{
	const uint8_t *payload = chain->ptr;
	struct net_buf *buf = NULL;
	int ret;

	if (chain->tot_len < 1U) {
		LOG_WRN("Empty packet, dropping");
		return -EBADMSG;
	}

	switch (payload[0]) {
	case BT_HCI_H4_EVT:
		ret = hci_ncp_evt_recv(chain, &buf);
		break;
	case BT_HCI_H4_ACL:
		ret = hci_ncp_acl_recv(chain, &buf);
		break;
	case BT_HCI_H4_ISO:
		if (!IS_ENABLED(CONFIG_BT_ISO)) {
			LOG_ERR("ISO packet without ISO support");
			return -EBADMSG;
		}
		ret = hci_ncp_iso_recv(chain, &buf);
		break;
	default:
		LOG_ERR("Unknown HCI packet type %u", payload[0]);
		return -EBADMSG;
	}

	if (ret == 0) {
		bt_hci_recv(dev, buf);
	}

	return ret;
}

/* Deliver the pending packets in order, until the host runs out of buffers.
 * Runs from rx_work only: the CPC core delivers the packets from its own
 * thread, and the host frees its buffers from any thread, so both hand over
 * to the work queue of the co-processor instead of racing on rx_pending.
 */
static void hci_ncp_rx_process(const struct device *dev)
{
	struct hci_ncp_data *data = dev->data;
	struct hci_ncp_rx_slot *slot;
	sl_status_t status;
	int ret;

	while (true) {
		slot = k_fifo_peek_head(&data->rx_pending);
		if (slot == NULL || !data->opened) {
			return;
		}

		ret = hci_ncp_deliver(dev, &slot->buf);
		if (ret == -ENOMEM) {
			/* bt_buf_rx_freed_cb() resumes us, the packet stays pending */
			return;
		}

		(void)k_fifo_get(&data->rx_pending, K_NO_WAIT);
		/* Give the buffers back to CPC, the whole chain at once */
		status = sl_cpc_ep_push_recv_buf(&data->ep, &slot->buf);
		if (status != SL_STATUS_OK) {
			LOG_ERR("%s: cannot return a receive buffer: %d", dev->name, status);
		}
	}
}

static void hci_ncp_rx_work(struct k_work *work)
{
	struct hci_ncp_data *data = CONTAINER_OF(work, struct hci_ncp_data, rx_work);

	hci_ncp_rx_process(data->dev);
}

/* The host freed a RX buffer, may be in any context */
static void hci_ncp_rx_freed(enum bt_buf_type type_mask)
{
	const struct device *dev = DEVICE_DT_INST_GET(0);
	const struct hci_ncp_config *cfg = dev->config;
	struct hci_ncp_data *data = dev->data;

	ARG_UNUSED(type_mask);

	(void)mfd_silabs_ncp_wq_submit(cfg->parent, &data->rx_work);
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

	ARG_UNUSED(ep);

	switch (type) {
	case SL_CPC_EP_EVENT_RECV:
		rx = CONTAINER_OF(event->recv.buf, struct hci_ncp_rx_slot, buf);
		k_fifo_put(&data->rx_pending, rx);
		(void)mfd_silabs_ncp_wq_submit(cfg->parent, &data->rx_work);
		break;
	case SL_CPC_EP_EVENT_SEND_DONE:
		tx = CONTAINER_OF(event->send_done.frame, struct hci_ncp_tx_slot, frame);
		if (event->send_done.status != SL_STATUS_OK) {
			LOG_ERR("%s: send failed: %d", dev->name, event->send_done.status);
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
		LOG_ERR("%s: endpoint error: %d", dev->name, event->error.status);
		data->ep_error = event->error.status;
		/* Fails the connection in progress, if any */
		k_sem_give(&data->connected);
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
	sl_status_t status;
	int ret;

	if (!data->opened) {
		return -ENOTCONN;
	}

	/* The H4 packet type is the first byte of the buffer */
	if (buf->len < 1U || buf->len > HCI_TX_MTU) {
		return -EINVAL;
	}

	ret = k_mem_slab_alloc(cfg->tx_slab, (void **)&slot, HCI_TX_SLOT_TIMEOUT);
	if (ret != 0) {
		LOG_ERR("%s: no TX slot", dev->name);
		return -ENOMEM;
	}

	(void)memcpy(slot->data, buf->data, buf->len);
	sl_cpc_buf_init(&slot->buf, slot->data, buf->len);
	(void)memset(&slot->frame, 0, sizeof(slot->frame));

	status = sl_cpc_ep_send(&data->ep, &slot->buf, &slot->frame, NULL);
	if (status != SL_STATUS_OK) {
		LOG_ERR("%s: send failed: %d", dev->name, status);
		k_mem_slab_free(cfg->tx_slab, slot);
		return -EIO;
	}

	net_buf_unref(buf);

	return 0;
}

/* Finish the teardown of the endpoint once CPC has released it. The receive
 * buffers it still held are gone with it, those waiting for the host too.
 */
static int hci_ncp_ep_release(const struct device *dev, k_timeout_t timeout)
{
	struct hci_ncp_data *data = dev->data;

	if (k_sem_take(&data->closed, timeout) != 0) {
		return -EBUSY;
	}

	sl_cpc_ep_deinit(&data->ep);
	while (k_fifo_get(&data->rx_pending, K_NO_WAIT) != NULL) {
	}
	data->closing = false;

	return 0;
}

/* Stop the delivery to the host and close the endpoint. Returns -EBUSY when
 * CPC has not released the endpoint in time: the next open finishes the
 * teardown, nothing can be undone at this point.
 */
static int hci_ncp_ep_shutdown(const struct device *dev)
{
	struct hci_ncp_data *data = dev->data;

	data->opened = false;
	bt_buf_rx_freed_cb_set(NULL);
	/* rx_work may be inside bt_hci_recv(): it must not hand its buffers
	 * back to the endpoint once this one is closed.
	 */
	(void)k_work_cancel_sync(&data->rx_work, &(struct k_work_sync){});

	k_sem_reset(&data->closed);
	data->closing = true;
	sl_cpc_ep_close(&data->ep);

	return hci_ncp_ep_release(dev, HCI_CLOSE_TIMEOUT);
}

static int hci_ncp_open(const struct device *dev)
{
	const struct hci_ncp_config *cfg = dev->config;
	struct hci_ncp_data *data = dev->data;
	sl_status_t status;
	int ret;

	if (!device_is_ready(cfg->parent)) {
		LOG_ERR("%s: co-processor not ready", dev->name);
		return -ENODEV;
	}

	if (data->closing) {
		ret = hci_ncp_ep_release(dev, K_NO_WAIT);
		if (ret < 0) {
			LOG_ERR("%s: endpoint not released yet", dev->name);
			return ret;
		}
	}

	status = sl_cpc_ep_init(&data->ep, HCI_NCP_EP_ID, HCI_RX_MTU, hci_ncp_ep_event,
				(void *)dev);
	if (status != SL_STATUS_OK) {
		LOG_ERR("%s: endpoint init failed: %d", dev->name, status);
		return -EIO;
	}

	for (size_t i = 0; i < ARRAY_SIZE(data->rx_slots); i++) {
		struct hci_ncp_rx_slot *slot = &data->rx_slots[i];

		sl_cpc_buf_init(&slot->buf, slot->data, sizeof(slot->data));
		status = sl_cpc_ep_push_recv_buf(&data->ep, &slot->buf);
		if (status != SL_STATUS_OK) {
			LOG_ERR("%s: cannot queue a receive buffer: %d", dev->name, status);
			sl_cpc_ep_deinit(&data->ep);
			return -EIO;
		}
	}

	/* Deliver from now on: the co-processor may talk before the connection
	 * is reported here.
	 */
	data->opened = true;
	data->ep_error = SL_STATUS_OK;
	bt_buf_rx_freed_cb_set(hci_ncp_rx_freed);
	k_sem_reset(&data->connected);
	status = sl_cpc_ep_connect(&data->ep, mfd_silabs_ncp_cpc_bus(cfg->parent));
	if (status != SL_STATUS_OK) {
		LOG_ERR("%s: endpoint connect failed: %d", dev->name, status);
		data->opened = false;
		bt_buf_rx_freed_cb_set(NULL);
		sl_cpc_ep_deinit(&data->ep);
		return -EIO;
	}

	if (k_sem_take(&data->connected, HCI_CONNECT_TIMEOUT) != 0) {
		LOG_ERR("%s: endpoint connection timed out", dev->name);
		(void)hci_ncp_ep_shutdown(dev);
		return -ETIMEDOUT;
	}
	if (data->ep_error != SL_STATUS_OK) {
		LOG_ERR("%s: endpoint connection failed: %d", dev->name, data->ep_error);
		(void)hci_ncp_ep_shutdown(dev);
		return -EIO;
	}

	LOG_INF("%s: HCI endpoint %u connected", dev->name, HCI_NCP_EP_ID);

	return 0;
}

static int hci_ncp_close(const struct device *dev)
{
	struct hci_ncp_data *data = dev->data;

	if (!data->opened) {
		return 0;
	}

	if (hci_ncp_ep_shutdown(dev) < 0) {
		LOG_WRN("%s: endpoint not released yet, the next open waits for it", dev->name);
	}

	return 0;
}

static int hci_ncp_init(const struct device *dev)
{
	struct hci_ncp_data *data = dev->data;

	data->dev = dev;
	(void)k_sem_init(&data->connected, 0, 1);
	(void)k_sem_init(&data->closed, 0, 1);
	k_fifo_init(&data->rx_pending);
	k_work_init(&data->rx_work, hci_ncp_rx_work);

	return 0;
}

static DEVICE_API(bt_hci, hci_ncp_api) = {
	.open = hci_ncp_open,
	.close = hci_ncp_close,
	.send = hci_ncp_send,
};

/* hci_ncp_rx_freed() has no device argument */
BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) <= 1, "A single HCI controller is supported");

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

DT_INST_FOREACH_STATUS_OKAY(HCI_NCP_DEFINE)
