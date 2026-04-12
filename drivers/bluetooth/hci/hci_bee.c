/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/bluetooth/buf.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/logging/log.h>

#include "rtl_bt_hci.h"

LOG_MODULE_REGISTER(bt_driver, CONFIG_BT_HCI_DRIVER_LOG_LEVEL);
#define DT_DRV_COMPAT realtek_bee_bt_hci

/* Min 20 required by BT controller. Set to 40 for margin. */
#define BT_BEE_RX_BUF_COUNT 40

struct k_thread rx_thread_data;
static K_KERNEL_STACK_DEFINE(rx_thread_stack, CONFIG_BT_RX_STACK_SIZE);

struct rtl_bt_rx_buf {
	intptr_t _unused;
	uint8_t *buf;
	uint32_t len;
};

K_MEM_SLAB_DEFINE(rx_slab, sizeof(struct rtl_bt_rx_buf), BT_BEE_RX_BUF_COUNT, 4);

static struct {
	struct k_fifo fifo;
} rx = {
	.fifo = Z_FIFO_INITIALIZER(rx.fifo),
};

struct bt_bee_data {
	bt_hci_recv_t recv;
};

static bool bt_hci_bee_check_hci_event_discardable(const uint8_t *event_data)
{
	uint8_t event_type = event_data[0];

	switch (event_type) {
#if defined(CONFIG_BT_CLASSIC)
	case BT_HCI_EVT_INQUIRY_RESULT_WITH_RSSI:
	case BT_HCI_EVT_EXTENDED_INQUIRY_RESULT:
		return true;
#endif
	case BT_HCI_EVT_LE_META_EVENT: {
		uint8_t sub_event_type = event_data[sizeof(struct bt_hci_evt_hdr)];

		switch (sub_event_type) {
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

static bool bt_hci_bee_recv_cb(T_RTL_BT_HCI_EVT evt, bool status, uint8_t *buf, uint32_t len)
{
	int ret = 0;

	LOG_DBG("evt %u status %u, type %u, len %u", evt, status, buf[0], len);
	switch (evt) {
	case BT_HCI_EVT_OPENED: {
		LOG_DBG("BT_HCI_EVT_OPENED");
		if (status == false) {
			ret = -EXDEV;
		}
	} break;

	case BT_HCI_EVT_DATA_IND: {
		struct rtl_bt_rx_buf *rx_buf;
		int err;

		err = k_mem_slab_alloc(&rx_slab, (void **)&rx_buf, K_NO_WAIT);
		if (err == 0) {
			rx_buf->buf = buf;
			rx_buf->len = len;
			k_fifo_put(&rx.fifo, rx_buf);
			break;
		}
		LOG_ERR("Failed to alloc RX slab, err: %d", err);
		rtl_bt_hci_ack(buf);
	} break;

	default:
		ret = -EINVAL;
		break;
	}

	if (ret != 0) {
		LOG_ERR("Error, evt %u status %u, type %u, len %u, ret %d", evt, status, buf[0],
			len, ret);
		return false;
	}

	return true;
}

void bt_hci_bee_handle_rx_data(const struct device *dev, struct rtl_bt_rx_buf *rx_buf)
{
	struct bt_bee_data *hci = dev->data;
	struct net_buf *z_buf = NULL;
	size_t buf_tailroom;

	/* First byte is packet type */
	switch (rx_buf->buf[0]) {
	case H4_EVT: {
		bool discardable = false;
		struct bt_hci_evt_hdr hdr;

		memcpy((void *)&hdr, &rx_buf->buf[1], sizeof(hdr));

		discardable = bt_hci_bee_check_hci_event_discardable(&rx_buf->buf[1]);

		z_buf = bt_buf_get_evt(hdr.evt, discardable, K_NO_WAIT);
		if (z_buf != NULL) {
			buf_tailroom = net_buf_tailroom(z_buf);

			if (buf_tailroom >= (hdr.len + sizeof(hdr))) {
				net_buf_add_mem(z_buf, &rx_buf->buf[1], hdr.len + sizeof(hdr));
				LOG_DBG("H4_EVT: event 0x%x", hdr.evt);
				hci->recv(dev, z_buf);
				break;
			}
			net_buf_unref(z_buf);
		}
		LOG_ERR("H4_EVT: event 0x%x, len %d, alloc failed", hdr.evt, hdr.len);
	} break;

	case H4_ACL: {
		struct bt_hci_acl_hdr hdr;

		memcpy((void *)&hdr, &rx_buf->buf[1], sizeof(hdr));

		z_buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_NO_WAIT);
		if (z_buf != NULL) {
			buf_tailroom = net_buf_tailroom(z_buf);
			if (buf_tailroom >= (hdr.len + sizeof(hdr))) {
				net_buf_add_mem(z_buf, &rx_buf->buf[1], hdr.len + sizeof(hdr));
				LOG_DBG("H4_ACL: handle 0x%x, Calling bt_recv(%p)", hdr.handle,
					z_buf);
				hci->recv(dev, z_buf);
				break;
			}
			net_buf_unref(z_buf);
		}
		LOG_ERR("H4_ACL: handle 0x%x, len %d, alloc failed", hdr.handle, hdr.len);
	} break;

	case H4_ISO: {
		struct bt_hci_iso_hdr hdr;

		memcpy((void *)&hdr, &rx_buf->buf[1], sizeof(hdr));

		z_buf = bt_buf_get_rx(BT_BUF_ISO_IN, K_NO_WAIT);
		if (z_buf != NULL) {
			buf_tailroom = net_buf_tailroom(z_buf);
			if (buf_tailroom >= (hdr.len + sizeof(hdr))) {
				net_buf_add_mem(z_buf, &rx_buf->buf[1], hdr.len + sizeof(hdr));
				LOG_DBG("H4_ISO: handle 0x%x, Calling bt_recv(%p)", hdr.handle,
					z_buf);
				hci->recv(dev, z_buf);
				break;
			}
			net_buf_unref(z_buf);
		}
		LOG_ERR("H4_ISO: handle 0x%x, len %d, alloc failed", hdr.handle, hdr.len);
	} break;

	default:
		LOG_ERR("rtl_rx_thread: invalid type %d", rx_buf->buf[0]);
		break;
	}
	rtl_bt_hci_ack(rx_buf->buf);
	k_mem_slab_free(&rx_slab, rx_buf);
}

static void rtl_rx_thread(void *p1, void *p2, void *p3)
{
	struct rtl_bt_rx_buf *rx_buf;
	const struct device *dev = (const struct device *)p1;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (1) {
		rx_buf = k_fifo_get(&rx.fifo, K_FOREVER);
		do {
			bt_hci_bee_handle_rx_data(dev, rx_buf);

			/* Give other threads a chance to run if the ISR
			 * is receiving data so fast that rx.fifo never
			 * or very rarely goes empty.
			 */
			k_yield();

			rx_buf = k_fifo_get(&rx.fifo, K_NO_WAIT);
		} while (rx_buf);
	}
}

static int bt_hci_bee_send(const struct device *dev, struct net_buf *buf)
{
	int ret = 0;
	T_RTL_BT_HCI_BUF hci_buf = {};
	uint8_t packet_type = net_buf_pull_u8(buf);

	switch (packet_type) {
	case BT_HCI_H4_ACL:
	case BT_HCI_H4_CMD:
	case BT_HCI_H4_ISO:
		break;

	default:
		LOG_ERR("Unknown packet type: 0x%02x", packet_type);
		ret = -EINVAL;
		goto done;
	}

	if (rtl_bt_hci_h2c_buf_alloc(&hci_buf, packet_type, buf->len) == false) {
		ret = -EINVAL;
		goto done;
	}

	if (rtl_bt_hci_h2c_buf_add(&hci_buf, buf->data, buf->len) == false) {
		rtl_bt_hci_h2c_buf_rel(hci_buf);
		ret = -EINVAL;
		goto done;
	}

	if (rtl_bt_hci_send(hci_buf) == false) {
		rtl_bt_hci_h2c_buf_rel(hci_buf);
		ret = -EIO;
	}

done:
	net_buf_unref(buf);
	if (ret != 0) {
		LOG_ERR("Error, type %d, len %u, ret %d", packet_type, buf->len, ret);
	} else {
		LOG_DBG("Sent, type %d, len %u", packet_type, buf->len);
	}

	return ret;
}

static int bt_hci_bee_open(const struct device *dev, bt_hci_recv_t recv)
{
	k_tid_t tid;

	tid = k_thread_create(&rx_thread_data, rx_thread_stack,
			      K_KERNEL_STACK_SIZEOF(rx_thread_stack), rtl_rx_thread, (void *)dev,
			      NULL, NULL, 0, 0, K_NO_WAIT);
	k_thread_name_set(tid, "rtl_rx_thread");

	struct bt_bee_data *hci = dev->data;

	if (rtl_bt_hci_h2c_pool_init(CONFIG_BT_BEE_HCI_H2C_POOL_SIZE)) {
		if (rtl_bt_hci_open(bt_hci_bee_recv_cb)) {
			hci->recv = recv;
			LOG_DBG("Bee BT started");
			return 0;
		}
	}
	return -EINVAL;
}

static int bt_hci_bee_close(const struct device *dev)
{
	struct bt_bee_data *hci = dev->data;

	hci->recv = NULL;

	LOG_DBG("Bee BT stopped");

	return 0;
}

static DEVICE_API(bt_hci, drv) = {
	.open = bt_hci_bee_open,
	.send = bt_hci_bee_send,
	.close = bt_hci_bee_close,
};

#define BT_HCI_BEE_DEVICE_INIT(inst)                                                               \
	static struct bt_bee_data bt_bee_data_##inst = {};                                         \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, &bt_bee_data_##inst, NULL, POST_KERNEL,            \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &drv)

/* Only one instance supported */
BT_HCI_BEE_DEVICE_INIT(0)
