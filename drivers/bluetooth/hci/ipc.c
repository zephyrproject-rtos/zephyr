/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/drivers/bluetooth.h>

#include <zephyr/device.h>
#include <zephyr/ipc/ipc_service.h>

#define LOG_LEVEL CONFIG_BT_HCI_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_hci_driver);

#define DT_DRV_COMPAT zephyr_bt_hci_ipc

#define IPC_BOUND_TIMEOUT_IN_MS K_MSEC(1000)

struct ipc_data {
	bt_hci_recv_t recv;
	struct ipc_ept hci_ept;
	struct ipc_ept_cfg hci_ept_cfg;
	struct k_sem bound_sem;
	const struct device *ipc;
};

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

static struct net_buf *bt_ipc_evt_recv(const uint8_t *data, size_t remaining)
{
	bool discardable;
	struct bt_hci_evt_hdr hdr;
	struct net_buf *buf;
	size_t buf_tailroom;

	if (remaining < sizeof(hdr)) {
		LOG_ERR("Not enough data (%u) for event header (%zu)", remaining, sizeof(hdr));
		return NULL;
	}

	discardable = is_hci_event_discardable(data);

	memcpy((void *)&hdr, data, sizeof(hdr));
	data += sizeof(hdr);
	remaining -= sizeof(hdr);

	if (remaining != hdr.len) {
		LOG_ERR("Event payload length is not correct (%u != %u)", remaining, hdr.len);
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

static struct net_buf *bt_ipc_acl_recv(const uint8_t *data, size_t remaining)
{
	struct bt_hci_acl_hdr hdr;
	struct net_buf *buf;
	size_t buf_tailroom;

	if (remaining < sizeof(hdr)) {
		LOG_ERR("Not enough data (%u) for ACL header (%zu)", remaining, sizeof(hdr));
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
		LOG_ERR("ACL payload length is not correct (%u != %u)", remaining,
			sys_le16_to_cpu(hdr.len));
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

static struct net_buf *bt_ipc_iso_recv(const uint8_t *data, size_t remaining)
{
	struct bt_hci_iso_hdr hdr;
	static size_t fail_cnt;
	struct net_buf *buf;
	size_t buf_tailroom;

	if (remaining < sizeof(hdr)) {
		LOG_ERR("Not enough data (%u) for ISO header (%zu)", remaining, sizeof(hdr));
		return NULL;
	}

	buf = bt_buf_get_rx(BT_BUF_ISO_IN, K_NO_WAIT);
	if (buf) {
		memcpy((void *)&hdr, data, sizeof(hdr));
		data += sizeof(hdr);
		remaining -= sizeof(hdr);

		net_buf_add_mem(buf, &hdr, sizeof(hdr));

		fail_cnt = 0U;
	} else {
		if ((fail_cnt % 100U) == 0U) {
			LOG_ERR("No available ISO buffers (%zu)!", fail_cnt);
		}

		fail_cnt++;

		return NULL;
	}

	if (remaining != bt_iso_hdr_len(sys_le16_to_cpu(hdr.len))) {
		LOG_ERR("ISO payload length is not correct (%u != %lu)", remaining,
			bt_iso_hdr_len(sys_le16_to_cpu(hdr.len)));
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

static void bt_ipc_rx(const struct device *dev, const uint8_t *data, size_t len)
{
	struct ipc_data *ipc = dev->data;
	uint8_t pkt_indicator;
	struct net_buf *buf = NULL;
	size_t remaining = len;

	LOG_HEXDUMP_DBG(data, len, "ipc data:");

	pkt_indicator = *data++;
	remaining -= sizeof(pkt_indicator);

	switch (pkt_indicator) {
	case BT_HCI_H4_EVT:
		buf = bt_ipc_evt_recv(data, remaining);
		break;

	case BT_HCI_H4_ACL:
		buf = bt_ipc_acl_recv(data, remaining);
		break;

	case BT_HCI_H4_ISO:
		buf = bt_ipc_iso_recv(data, remaining);
		break;

	default:
		LOG_ERR("Unknown HCI type %u", pkt_indicator);
		return;
	}

	if (buf) {
		LOG_DBG("Calling bt_recv(%p)", buf);
		ipc->recv(dev, buf);

		LOG_HEXDUMP_DBG(buf->data, buf->len, "RX buf payload:");
	}
}

static int bt_ipc_send(const struct device *dev, struct net_buf *buf)
{
	struct ipc_data *data = dev->data;
	int err;
	uint8_t pkt_indicator;

	LOG_DBG("buf %p type %u len %u", buf, bt_buf_get_type(buf), buf->len);

	switch (bt_buf_get_type(buf)) {
	case BT_BUF_ACL_OUT:
		pkt_indicator = BT_HCI_H4_ACL;
		break;
	case BT_BUF_CMD:
		pkt_indicator = BT_HCI_H4_CMD;
		break;
	case BT_BUF_ISO_OUT:
		pkt_indicator = BT_HCI_H4_ISO;
		break;
	default:
		LOG_ERR("Unknown type %u", bt_buf_get_type(buf));
		goto done;
	}
	net_buf_push_u8(buf, pkt_indicator);

	LOG_HEXDUMP_DBG(buf->data, buf->len, "Final HCI buffer:");
	err = ipc_service_send(&data->hci_ept, buf->data, buf->len);
	if (err < 0) {
		LOG_ERR("Failed to send (err %d)", err);
	}

done:
	net_buf_unref(buf);
	return 0;
}

static void hci_ept_bound(void *priv)
{
	const struct device *dev = priv;
	struct ipc_data *ipc = dev->data;

	k_sem_give(&ipc->bound_sem);
}

static void hci_ept_recv(const void *data, size_t len, void *priv)
{
	const struct device *dev = priv;

	bt_ipc_rx(dev, data, len);
}

int __weak bt_hci_transport_setup(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

int __weak bt_hci_transport_teardown(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

static int bt_ipc_open(const struct device *dev, bt_hci_recv_t recv)
{
	struct ipc_data *ipc = dev->data;
	int err;

	err = bt_hci_transport_setup(NULL);
	if (err) {
		LOG_ERR("HCI transport setup failed with: %d\n", err);
		return err;
	}

	LOG_DBG("");

	err = ipc_service_open_instance(ipc->ipc);
	if (err && (err != -EALREADY)) {
		LOG_ERR("IPC service instance initialization failed: %d\n", err);
		return err;
	}

	err = ipc_service_register_endpoint(ipc->ipc, &ipc->hci_ept, &ipc->hci_ept_cfg);
	if (err) {
		LOG_ERR("Registering endpoint failed with %d", err);
		return err;
	}

	err = k_sem_take(&ipc->bound_sem, IPC_BOUND_TIMEOUT_IN_MS);
	if (err) {
		LOG_ERR("Endpoint binding failed with %d", err);
		return err;
	}

	ipc->recv = recv;

	return 0;
}

static int bt_ipc_close(const struct device *dev)
{
	struct ipc_data *ipc = dev->data;
	int err;

	if (IS_ENABLED(CONFIG_BT_HCI_HOST)) {
		err = bt_hci_cmd_send_sync(BT_HCI_OP_RESET, NULL, NULL);
		if (err) {
			LOG_ERR("Sending reset command failed with: %d", err);
			return err;
		}
	}

	err = ipc_service_deregister_endpoint(&ipc->hci_ept);
	if (err) {
		LOG_ERR("Deregistering HCI endpoint failed with: %d", err);
		return err;
	}

	err = ipc_service_close_instance(ipc->ipc);
	if (err) {
		LOG_ERR("Closing IPC service failed with: %d", err);
		return err;
	}

	err = bt_hci_transport_teardown(NULL);
	if (err) {
		LOG_ERR("HCI transport teardown failed with: %d", err);
		return err;
	}

	ipc->recv = NULL;

	return 0;
}

static DEVICE_API(bt_hci, drv) = {
	.open		= bt_ipc_open,
	.close		= bt_ipc_close,
	.send		= bt_ipc_send,
};

#define IPC_DEVICE_INIT(inst) \
	static struct ipc_data ipc_data_##inst = { \
		.bound_sem = Z_SEM_INITIALIZER(ipc_data_##inst.bound_sem, 0, 1), \
		.hci_ept_cfg = { \
			.name = DT_INST_PROP(inst, bt_hci_ipc_name), \
			.cb = { \
				.bound    = hci_ept_bound, \
				.received = hci_ept_recv, \
			}, \
			.priv = (void *)DEVICE_DT_INST_GET(inst), \
		}, \
		.ipc = DEVICE_DT_GET(DT_INST_PARENT(inst)), \
	}; \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, &ipc_data_##inst, NULL, \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &drv)

DT_INST_FOREACH_STATUS_OKAY(IPC_DEVICE_INIT)
