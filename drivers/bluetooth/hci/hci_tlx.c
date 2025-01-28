/*
 * Copyright (c) 2024, Telink Semiconductor (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT telink_tlx_bt

#include <zephyr/drivers/bluetooth.h>
#include <zephyr/sys/byteorder.h>
#include <tlx_bt.h>

#define LOG_LEVEL CONFIG_BT_HCI_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_hci_driver_tlx);

#define HCI_BT_TLX_TIMEOUT      K_MSEC(2000)
#define HCI_CMD                 0x01
#define HCI_ACL                 0x02
#define HCI_EVT                 0x04

static K_SEM_DEFINE(hci_send_sem, 1, 1);
static const struct device *hci_dev;
static bt_hci_recv_t hci_recv;

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

static struct net_buf *bt_tlx_evt_recv(uint8_t *data, size_t len)
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

static struct net_buf *bt_tlx_acl_recv(uint8_t *data, size_t len)
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

static void hci_tlx_host_rcv_pkt(uint8_t *data, uint16_t len)
{
	uint8_t pkt_indicator;
	struct net_buf *buf;

	LOG_HEXDUMP_DBG(data, len, "host packet data:");

	pkt_indicator = *data++;
	len -= sizeof(pkt_indicator);

	switch (pkt_indicator) {
	case HCI_EVT:
		buf = bt_tlx_evt_recv(data, len);
		break;

	case HCI_ACL:
		buf = bt_tlx_acl_recv(data, len);
		break;

	default:
		buf = NULL;
		LOG_ERR("Unknown HCI type %u", pkt_indicator);
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

static void hci_tlx_controller_rcv_pkt_ready(void)
{
	k_sem_give(&hci_send_sem);
}

static tlx_bt_host_callback_t vhci_host_cb = {
	.host_send_available = hci_tlx_controller_rcv_pkt_ready,
	.host_read_packet = hci_tlx_host_rcv_pkt
};

static int hci_tlx_open(const struct device *dev, bt_hci_recv_t recv)
{
#if CONFIG_IEEE802154_TELINK_TLX
	extern volatile bool tlx_rf_zigbee_250K_mode;

	tlx_rf_zigbee_250K_mode = false;
#endif
	int status;

	status = tlx_bt_controller_init();
	if (status) {
		LOG_ERR("Bluetooth controller init failed %d", status);
		return status;
	}

	hci_dev = dev;
	hci_recv = recv;
	tlx_bt_host_callback_register(&vhci_host_cb);

#if CONFIG_SOC_RISCV_TELINK_B91
	LOG_DBG("B91 BT started");
#elif CONFIG_SOC_RISCV_TELINK_B92
	LOG_DBG("B92 BT started");
#endif

	return 0;
}

static int bt_tlx_send(const struct device *dev, struct net_buf *buf)
{
	(void) dev;
	int err = 0;
	uint8_t type;

	LOG_DBG("buf %p type %u len %u", buf, bt_buf_get_type(buf), buf->len);

	switch (bt_buf_get_type(buf)) {
	case BT_BUF_ACL_OUT:
		type = HCI_ACL;
		break;

	case BT_BUF_CMD:
		type = HCI_CMD;
		break;

	default:
		LOG_ERR("Unknown type %u", bt_buf_get_type(buf));
		goto done;
	}

	LOG_HEXDUMP_DBG(buf->data, buf->len, "Final HCI buffer:");

	if (k_sem_take(&hci_send_sem, HCI_BT_TLX_TIMEOUT) == 0) {
		tlx_bt_host_send_packet(type, buf->data, buf->len);
	} else {
		LOG_ERR("Send packet timeout error");
		err = -ETIMEDOUT;
	}

done:
	net_buf_unref(buf);
	k_sem_give(&hci_send_sem);

	return err;
}

static int hci_tlx_close(const struct device *dev)
{
	(void) dev;
#if defined(CONFIG_BT_HCI_HOST) && defined(CONFIG_BT_BROADCASTER)
	bt_le_adv_stop();
#endif /* CONFIG_BT_HCI_HOST && CONFIG_BT_BROADCASTER */
	tlx_bt_controller_deinit();
#if CONFIG_IEEE802154_TELINK_TLX
	extern volatile bool tlx_rf_zigbee_250K_mode;

	tlx_rf_zigbee_250K_mode = false;
#endif
	return 0;
}

static int tlx_bt_hci_init(const struct device *dev)
{
	return 0;
}

static const struct bt_hci_driver_api tlx_bt_hci_api = {
	.open  = hci_tlx_open,
	.send  = bt_tlx_send,
	.close = hci_tlx_close
};

/* BT HCI driver registration */
#define TLX_BT_HCI_INIT(n)                                              \
	DEVICE_DT_INST_DEFINE(n, tlx_bt_hci_init,                           \
		NULL,                                                           \
		NULL,                                                           \
		NULL,                                                           \
		POST_KERNEL,                                                    \
		CONFIG_KERNEL_INIT_PRIORITY_DEVICE,                             \
		&tlx_bt_hci_api);

DT_INST_FOREACH_STATUS_OKAY(TLX_BT_HCI_INIT)

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) > 1
#error only one HCI controller is supported
#endif /* DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) > 1 */
