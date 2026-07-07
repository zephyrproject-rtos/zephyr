/*
 * Copyright (c) 2025-2026 Conclusive Engineering sp. z o. o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/logging/log.h>
#include <ble_if.h>
#include <ble_transport.h>

/* Needs to happen after CC35xx Wi-Fi driver init. */
#define HCI_CC35XX_INIT_PRIORITY 81

#define DT_DRV_COMPAT ti_cc35xx_bt_hci

LOG_MODULE_REGISTER(DT_DRV_COMPAT, CONFIG_BT_HCI_DRIVER_LOG_LEVEL);

struct hci_cc35xx_priv {
	bt_hci_recv_t recv;
};

static int hci_cc35xx_setup(const struct device *dev, const struct bt_hci_setup_params *params)
{
	const bt_addr_t *addr = &params->public_addr;

	if (!bt_addr_eq(addr, BT_ADDR_ANY)) {
		BleIf_SetBdAddr(addr->val);
	}

	return 0;
}

static int hci_cc35xx_open(const struct device *dev, bt_hci_recv_t recv)
{
	struct hci_cc35xx_priv *priv = dev->data;

	BleIf_VendorSpecificEventFormat(VENDOR_SPECIFIC_FORMAT_NIMBLE);

	priv->recv = recv;

	return BleIf_EnableBLE();
}

static int hci_cc35xx_close(const struct device *dev)
{
	struct hci_cc35xx_priv *priv = dev->data;

	priv->recv = NULL;

	return 0;
}

static int hci_cc35xx_send(const struct device *dev, struct net_buf *buf)
{
	int ret;
	uint8_t type = bt_buf_type_from_h4(net_buf_pull_u8(buf), BT_BUF_OUT);

	switch (type) {
	case BT_BUF_ACL_OUT:
		net_buf_push_u8(buf, BT_HCI_H4_ACL);
		break;
	case BT_BUF_CMD:
		net_buf_push_u8(buf, BT_HCI_H4_CMD);
		break;
	default:
		LOG_ERR("Unsupported BT type");
		net_buf_unref(buf);
		ret = -EINVAL;
		goto out;
	}

	ret = BleIf_SendCommand(buf->data, buf->len);

out:
	net_buf_unref(buf);

	return ret;
}

static DEVICE_API(bt_hci, drv) = {
	.setup = hci_cc35xx_setup,
	.open = hci_cc35xx_open,
	.close = hci_cc35xx_close,
	.send = hci_cc35xx_send,
};

static bool hci_cc35xx_is_evt_discardable(uint8_t *data)
{
	struct bt_hci_evt_hdr *evt = (void *)data;

	if (evt->evt != BT_HCI_EVT_LE_META_EVENT || !evt->len) {
		return false;
	}

	switch (data[BT_HCI_EVT_HDR_SIZE]) {
	case BT_HCI_EVT_LE_ADVERTISING_REPORT:
	/* Fall-through. */
	case BT_HCI_EVT_LE_EXT_ADVERTISING_REPORT:
		return true;
	default:
		return false;
	}
}

static int hci_cc35xx_evt_recv(uint8_t *data, uint16_t len)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(hci0));
	struct hci_cc35xx_priv *priv = dev->data;
	struct net_buf *buf;
	uint8_t pkt_type;
	bool discardable;

	if (len == 0) {
		return -EINVAL;
	}

	pkt_type = data[0];

	/* Skip over the HCI packet indicator byte. */
	data++;
	len--;

	switch (pkt_type) {
	case BT_HCI_H4_EVT:
		if (len < BT_HCI_EVT_HDR_SIZE) {
			LOG_ERR("Event header is missing");
			return -EINVAL;
		}

		discardable = hci_cc35xx_is_evt_discardable(data);
		buf = bt_buf_get_evt(data[0], discardable, discardable ? K_NO_WAIT : K_FOREVER);
		break;
	case BT_HCI_H4_ACL:
		if (len < BT_HCI_ACL_HDR_SIZE) {
			LOG_ERR("ACL header is missing");
			return -EINVAL;
		}

		buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_FOREVER);
		break;
	default:
		LOG_ERR("Unknown HCI packet type: %d", pkt_type);
		return -ENOTSUP;
	}

	if (!buf) {
		return -ENOMEM;
	}

	if (len > net_buf_tailroom(buf)) {
		LOG_ERR("Not enough space in RX buffer");
		net_buf_unref(buf);
		return -EINVAL;
	}

	net_buf_add_mem(buf, data, len);
	priv->recv(dev, buf);

	return 0;
}

static int hci_cc35xx_init(const struct device *dev)
{
	BleIf_OpenTransport();
	BleIf_EventCbRegister(hci_cc35xx_evt_recv);

	return 0;
}

#define HCI_DEVICE_INIT(inst)                                                                      \
	static struct hci_cc35xx_priv hci_data_##inst = {};                                        \
	DEVICE_DT_INST_DEFINE(inst, hci_cc35xx_init, NULL, &hci_data_##inst, NULL, POST_KERNEL,    \
			      HCI_CC35XX_INIT_PRIORITY, &drv)

HCI_DEVICE_INIT(0)
