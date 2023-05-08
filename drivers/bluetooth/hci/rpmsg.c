/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/drivers/bluetooth/hci_driver.h>

#include <zephyr/device.h>
#include <zephyr/ipc/ipc_service.h>

#define LOG_LEVEL CONFIG_BT_HCI_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_hci_driver);

#define RPMSG_CMD 0x01
#define RPMSG_ACL 0x02
#define RPMSG_SCO 0x03
#define RPMSG_EVT 0x04
#define RPMSG_ISO 0x05

#define IPC_BOUND_TIMEOUT_IN_MS K_MSEC(1000)

static struct ipc_ept hci_ept;
static K_SEM_DEFINE(ipc_bound_sem, 0, 1);

static bool is_hci_event_discardable(const uint8_t *evt_data)
{
	uint8_t evt_type = evt_data[0];

	switch (evt_type) {
#if defined(CONFIG_BT_BREDR)
	case BT_HCI_EVT_INQUIRY_RESULT_WITH_RSSI:
	case BT_HCI_EVT_EXTENDED_INQUIRY_RESULT:
		return true;
#endif
	case BT_HCI_EVT_LE_META_EVENT: {
		uint8_t subevt_type = evt_data[sizeof(struct bt_hci_evt_hdr)];

		switch (subevt_type) {
		case BT_HCI_EVT_LE_ADVERTISING_REPORT:
			return true;
#if defined(CONFIG_BT_EXT_ADV)
		case BT_HCI_EVT_LE_EXT_ADVERTISING_REPORT:
		{
			const struct bt_hci_evt_le_ext_advertising_report *ext_adv =
				(void *)&evt_data[3];

			return (ext_adv->num_reports == 1) &&
				   ((ext_adv->adv_info[0].evt_type &
					 BT_HCI_LE_ADV_EVT_TYPE_LEGACY) != 0);
		}
#endif
		default:
			return false;
		}
	}
	default:
		return false;
	}
}

static struct net_buf *bt_rpmsg_evt_recv(const uint8_t *data, size_t remaining)
{
	bool discardable;
	struct bt_hci_evt_hdr hdr;
	struct net_buf *buf;
	size_t buf_tailroom;

	if (remaining < sizeof(hdr)) {
		LOG_ERR("Not enough data for event header");
		return NULL;
	}

	discardable = is_hci_event_discardable(data);

	memcpy((void *)&hdr, data, sizeof(hdr));
	data += sizeof(hdr);
	remaining -= sizeof(hdr);

	if (remaining != hdr.len) {
		LOG_ERR("Event payload length is not correct");
		return NULL;
	}
	LOG_DBG("len %u", hdr.len);

	do {
		buf = bt_buf_get_evt(hdr.evt, discardable, discardable ? K_NO_WAIT : K_SECONDS(10));
		if (!buf) {
			if (discardable) {
				LOG_DBG("Discardable buffer pool full, ignoring event");
				return buf;
			}
			LOG_WRN("Couldn't allocate a buffer after waiting 10 seconds.");
		}
	} while (!buf);

	net_buf_add_mem(buf, &hdr, sizeof(hdr));

	buf_tailroom = net_buf_tailroom(buf);
	if (buf_tailroom < remaining) {
		LOG_ERR("Not enough space in buffer %zu/%zu", remaining, buf_tailroom);
		net_buf_unref(buf);
		return NULL;
	}

	net_buf_add_mem(buf, data, remaining);

	return buf;
}

static struct net_buf *bt_rpmsg_acl_recv(const uint8_t *data, size_t remaining)
{
	struct bt_hci_acl_hdr hdr;
	struct net_buf *buf;
	size_t buf_tailroom;

	if (remaining < sizeof(hdr)) {
		LOG_ERR("Not enough data for ACL header");
		return NULL;
	}

	buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_NO_WAIT);
	if (buf) {
		memcpy((void *)&hdr, data, sizeof(hdr));
		data += sizeof(hdr);
		remaining -= sizeof(hdr);

		net_buf_add_mem(buf, &hdr, sizeof(hdr));
	} else {
		LOG_ERR("No available ACL buffers!");
		return NULL;
	}

	if (remaining != sys_le16_to_cpu(hdr.len)) {
		LOG_ERR("ACL payload length is not correct");
		net_buf_unref(buf);
		return NULL;
	}

	buf_tailroom = net_buf_tailroom(buf);
	if (buf_tailroom < remaining) {
		LOG_ERR("Not enough space in buffer %zu/%zu", remaining, buf_tailroom);
		net_buf_unref(buf);
		return NULL;
	}

	LOG_DBG("len %u", remaining);
	net_buf_add_mem(buf, data, remaining);

	return buf;
}

static struct net_buf *bt_rpmsg_iso_recv(const uint8_t *data, size_t remaining)
{
	struct bt_hci_iso_hdr hdr;
	struct net_buf *buf;
	size_t buf_tailroom;

	if (remaining < sizeof(hdr)) {
		LOG_ERR("Not enough data for ISO header");
		return NULL;
	}

	buf = bt_buf_get_rx(BT_BUF_ISO_IN, K_NO_WAIT);
	if (buf) {
		memcpy((void *)&hdr, data, sizeof(hdr));
		data += sizeof(hdr);
		remaining -= sizeof(hdr);

		net_buf_add_mem(buf, &hdr, sizeof(hdr));
	} else {
		LOG_ERR("No available ISO buffers!");
		return NULL;
	}

	if (remaining != bt_iso_hdr_len(sys_le16_to_cpu(hdr.len))) {
		LOG_ERR("ISO payload length is not correct");
		net_buf_unref(buf);
		return NULL;
	}

	buf_tailroom = net_buf_tailroom(buf);
	if (buf_tailroom < remaining) {
		LOG_ERR("Not enough space in buffer %zu/%zu", remaining, buf_tailroom);
		net_buf_unref(buf);
		return NULL;
	}

	LOG_DBG("len %zu", remaining);
	net_buf_add_mem(buf, data, remaining);

	return buf;
}

static void bt_rpmsg_rx(const uint8_t *data, size_t len)
{
	uint8_t pkt_indicator;
	struct net_buf *buf = NULL;
	size_t remaining = len;

	LOG_HEXDUMP_DBG(data, len, "RPMsg data:");

	pkt_indicator = *data++;
	remaining -= sizeof(pkt_indicator);

	switch (pkt_indicator) {
	case RPMSG_EVT:
		buf = bt_rpmsg_evt_recv(data, remaining);
		break;

	case RPMSG_ACL:
		buf = bt_rpmsg_acl_recv(data, remaining);
		break;

	case RPMSG_ISO:
		buf = bt_rpmsg_iso_recv(data, remaining);
		break;

	default:
		LOG_ERR("Unknown HCI type %u", pkt_indicator);
		return;
	}

	if (buf) {
		LOG_DBG("Calling bt_recv(%p)", buf);

		bt_recv(buf);

		LOG_HEXDUMP_DBG(buf->data, buf->len, "RX buf payload:");
	}
}

static int bt_rpmsg_send(struct net_buf *buf)
{
	int err;
	uint8_t pkt_indicator;

	LOG_DBG("buf %p type %u len %u", buf, bt_buf_get_type(buf), buf->len);

	switch (bt_buf_get_type(buf)) {
	case BT_BUF_ACL_OUT:
		pkt_indicator = RPMSG_ACL;
		break;
	case BT_BUF_CMD:
		pkt_indicator = RPMSG_CMD;
		break;
	case BT_BUF_ISO_OUT:
		pkt_indicator = RPMSG_ISO;
		break;
	default:
		LOG_ERR("Unknown type %u", bt_buf_get_type(buf));
		goto done;
	}
	net_buf_push_u8(buf, pkt_indicator);

	LOG_HEXDUMP_DBG(buf->data, buf->len, "Final HCI buffer:");
	err = ipc_service_send(&hci_ept, buf->data, buf->len);
	if (err < 0) {
		LOG_ERR("Failed to send (err %d)", err);
	}

done:
	net_buf_unref(buf);
	return 0;
}

static void hci_ept_bound(void *priv)
{
	k_sem_give(&ipc_bound_sem);
}

static void hci_ept_recv(const void *data, size_t len, void *priv)
{
	bt_rpmsg_rx(data, len);
}

static struct ipc_ept_cfg hci_ept_cfg = {
	.name = "nrf_bt_hci",
	.cb = {
		.bound    = hci_ept_bound,
		.received = hci_ept_recv,
	},
};

static int bt_rpmsg_open(void)
{
	int err;
	const struct device *hci_ipc_instance =
		DEVICE_DT_GET(DT_CHOSEN(zephyr_bt_hci_rpmsg_ipc));

	LOG_DBG("");

	err = ipc_service_open_instance(hci_ipc_instance);
	if (err && (err != -EALREADY)) {
		LOG_ERR("IPC service instance initialization failed: %d\n", err);
		return err;
	}

	err = ipc_service_register_endpoint(hci_ipc_instance, &hci_ept, &hci_ept_cfg);
	if (err) {
		LOG_ERR("Registering endpoint failed with %d", err);
		return err;
	}

	err = k_sem_take(&ipc_bound_sem, IPC_BOUND_TIMEOUT_IN_MS);
	if (err) {
		LOG_ERR("Endpoint binding failed with %d", err);
		return err;
	}

	return 0;
}

static const struct bt_hci_driver drv = {
	.name		= "RPMsg",
	.open		= bt_rpmsg_open,
	.send		= bt_rpmsg_send,
	.bus		= BT_HCI_DRIVER_BUS_IPM,
#if defined(CONFIG_BT_DRIVER_QUIRK_NO_AUTO_DLE)
	.quirks         = BT_QUIRK_NO_AUTO_DLE,
#endif
};

static int bt_rpmsg_init(void)
{

	int err;

	err = bt_hci_driver_register(&drv);
	if (err < 0) {
		LOG_ERR("Failed to register BT HIC driver (err %d)", err);
	}

	return err;
}

SYS_INIT(bt_rpmsg_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
