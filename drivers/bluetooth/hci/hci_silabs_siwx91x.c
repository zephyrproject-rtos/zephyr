/*
 * Copyright (c) 2024 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT silabs_siwx91x_bt_hci

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/bluetooth.h>

#include "siwx91x_nwp.h"
#include "siwx91x_nwp_bus.h"
#include "siwx91x_nwp_api.h"
#include "device/silabs/si91x/wireless/ble/inc/rsi_ble_common_config.h"
#include "device/silabs/si91x/wireless/ble/inc/rsi_ble.h"

LOG_MODULE_REGISTER(siwx91x_bt, CONFIG_BT_HCI_DRIVER_LOG_LEVEL);

struct siwx91x_bt_config {
	struct siwx91x_nwp_bt_cb nwp_ops;
	const struct device *self_dev;
	const struct device *nwp_dev;
	struct net_buf_pool *mempool;
};

struct siwx91x_bt_data {
	bt_hci_recv_t on_recv;
};

static void siwx91x_bt_on_rx(const struct siwx91x_nwp_bt_cb *ctxt, uint8_t packet_type,
			     void *payload, size_t len)
{
	struct siwx91x_bt_config *config = container_of(ctxt, struct siwx91x_bt_config, nwp_ops);
	const struct device *dev = config->self_dev;
	struct siwx91x_bt_data *data = dev->data;
	struct net_buf *bt_buf;

	switch (packet_type) {
	case BT_HCI_H4_EVT: {
		struct bt_hci_evt_hdr *hdr = payload;

		__ASSERT(hdr->len + sizeof(*hdr) == len, "Corrupted data");
		bt_buf = bt_buf_get_evt(hdr->evt, false, K_FOREVER);
		break;
	}
	case BT_HCI_H4_ACL: {
		__maybe_unused struct bt_hci_acl_hdr *hdr = payload;

		__ASSERT(hdr->len + sizeof(*hdr) == len, "Corrupted data");
		bt_buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_FOREVER);
		break;
	}
	default:
		LOG_ERR("Unknown/Unhandled HCI type: %d", packet_type);
		return;
	}
	if (!bt_buf) {
		LOG_ERR("No memory");
		return;
	}
	/* net_buf_add_mem() assert if bt_buf is too small. we are fine with that */
	net_buf_add_mem(bt_buf, payload, len);
	data->on_recv(config->self_dev, bt_buf);
}

static int siwx91x_bt_send(const struct device *dev, struct net_buf *bt_buf)
{
	const struct siwx91x_bt_config *config = dev->config;
	struct net_buf *desc_buf = net_buf_alloc(config->mempool, K_NO_WAIT);
	struct siwx91x_frame_desc *desc = net_buf_add(desc_buf, sizeof(struct siwx91x_frame_desc));

	__ASSERT(desc_buf, "Corrupted mempool size");
	__ASSERT(desc, "Corrupted mempool format");
	memset(desc, 0, sizeof(struct siwx91x_frame_desc));
	desc->reserved[10] = bt_buf->data[0];
	net_buf_frag_add(desc_buf, bt_buf);
	siwx91x_nwp_send_frame(config->nwp_dev, desc_buf, RSI_BLE_REQ_HCI_RAW, SLI_BT_Q,
			       SIWX91X_FRAME_FLAG_SHIFT_PAYLOAD_1_BYTE |
			       SIWX91X_FRAME_FLAG_NO_HDR_RESET |
			       SIWX91X_FRAME_FLAG_ASYNC);
	net_buf_unref(desc_buf);
	return 0;
}

static int siwx91x_bt_open(const struct device *dev, bt_hci_recv_t recv)
{
	struct siwx91x_bt_data *data = dev->data;

	data->on_recv = recv;
	return 0;
}

#define SIWX91X_BLE_RF_POWER_INDEX     0x0006
#define SIWX91X_BLE_MODE               2
static int siwx91x_bt_set_tx_power(const struct device *dev, uint8_t protocol_mode,
				   uint8_t le_tx_power_index)
{
	struct net_buf *buf;
	int ret;

	buf = bt_hci_cmd_alloc(K_FOREVER);
	if (!buf) {
		return -ENOMEM;
	}
	net_buf_add_u8(buf, protocol_mode);
	net_buf_add_u8(buf, le_tx_power_index);
	LOG_DBG("Sending RF Power Mode command (OCF 0x%04X) with power index %d",
		SIWX91X_BLE_RF_POWER_INDEX, le_tx_power_index);

	ret = bt_hci_cmd_send_sync(BT_OP(BT_OGF_VS, SIWX91X_BLE_RF_POWER_INDEX), buf, NULL);
	if (ret) {
		return ret;
	}

	return 0;
}

static int siwx91x_bt_setup(const struct device *dev, const struct bt_hci_setup_params *params)
{
	const struct siwx91x_bt_config *config = dev->config;
	int ret;

	ret = siwx91x_bt_set_tx_power(dev, SIWX91X_BLE_MODE, RSI_BLE_PWR_INX);
	if (ret) {
		return ret;
	}
	ret = siwx91x_nwp_apply_power_profile(config->nwp_dev);
	if (ret < 0) {
		return ret;
	}

	return 0;

}

static int siwx91x_bt_init(const struct device *dev)
{
	const struct siwx91x_bt_config *config = dev->config;

	if (!device_is_ready(config->nwp_dev)) {
		return -ENODEV;
	}
	siwx91x_nwp_register_bt(config->nwp_dev, &config->nwp_ops);

	return 0;
}

static DEVICE_API(bt_hci, siwx91x_bt_api) = {
	.send = siwx91x_bt_send,
	.open = siwx91x_bt_open,
	.setup = siwx91x_bt_setup,
};

#define SIWX91X_BT_DEFINE(inst)                                                                    \
	NET_BUF_POOL_FIXED_DEFINE(siwx91x_bt_pool##inst, CONFIG_BT_BUF_CMD_TX_COUNT,               \
				  sizeof(struct siwx91x_frame_desc), sizeof(void *), NULL);        \
	static struct siwx91x_bt_config siwx91x_bt_config##inst = {                                \
		.nwp_ops.on_rx = siwx91x_bt_on_rx,                                                 \
		.self_dev = DEVICE_DT_GET(DT_DRV_INST(inst)),                                      \
		.nwp_dev = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                    \
		.mempool = &siwx91x_bt_pool##inst,                                                 \
	};                                                                                         \
	static struct siwx91x_bt_data siwx91x_bt_data##inst = { };                                 \
												   \
	DEVICE_DT_INST_DEFINE(inst, siwx91x_bt_init, NULL, &siwx91x_bt_data##inst,                 \
			      &siwx91x_bt_config##inst, POST_KERNEL,                               \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &siwx91x_bt_api)

/* Only one instance supported right now */
SIWX91X_BT_DEFINE(0)
