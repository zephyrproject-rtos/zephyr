/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/drivers/bluetooth.h>

#include <esp_hosted_mcu.h>

#define DT_DRV_COMPAT espressif_bt_hci_esp_hosted_mcu

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(hci_esp_hosted_mcu, CONFIG_BT_HCI_DRIVER_LOG_LEVEL);

struct hci_esp_hosted_mcu_data {
	/* bt_hci_driver_data must be the first member. */
	struct bt_hci_driver_data common;
	const struct device *dev;
	bool rx_started;
};

/*
 * The core delivers received frames on its single shared receive thread, which
 * also drives the Wi-Fi path. Reconstructing an event or ACL net_buf can block
 * on the host buffer pools, so that work must not run on the shared thread or
 * it would stall Wi-Fi receive. Frames are copied into a slab slot and handed
 * to a dedicated thread that performs the blocking allocation and delivery.
 */
#define HCI_ESP_HOSTED_MCU_RX_SLOTS 8

struct hci_esp_hosted_mcu_rx_frame {
	void *fifo_reserved;
	uint16_t len;
	uint8_t data[ESP_HOSTED_MCU_MAX_PAYLOAD];
};

/* Slab block size must be a multiple of the pointer alignment. */
#define HCI_ESP_HOSTED_MCU_RX_SLOT_SIZE                                                            \
	ROUND_UP(sizeof(struct hci_esp_hosted_mcu_rx_frame), sizeof(void *))

K_MEM_SLAB_DEFINE_STATIC(hci_esp_hosted_mcu_rx_slab, HCI_ESP_HOSTED_MCU_RX_SLOT_SIZE,
			 HCI_ESP_HOSTED_MCU_RX_SLOTS, sizeof(void *));
static K_FIFO_DEFINE(hci_esp_hosted_mcu_rx_fifo);
static const struct device *hci_esp_hosted_mcu_rx_dev;

/* LE advertising reports arrive in bursts and may be dropped under pressure. */
static bool hci_esp_hosted_mcu_evt_discardable(const uint8_t *evt_data, size_t remaining)
{
	/* Need the meta event header plus its subevent code byte. */
	if (remaining <= sizeof(struct bt_hci_evt_hdr)) {
		return false;
	}

	if (evt_data[0] != BT_HCI_EVT_LE_META_EVENT) {
		return false;
	}

	switch (evt_data[sizeof(struct bt_hci_evt_hdr)]) {
	case BT_HCI_EVT_LE_ADVERTISING_REPORT:
#if defined(CONFIG_BT_EXT_ADV)
	case BT_HCI_EVT_LE_EXT_ADVERTISING_REPORT:
#endif
		return true;
	default:
		return false;
	}
}

static struct net_buf *hci_esp_hosted_mcu_evt_recv(const uint8_t *data, size_t remaining)
{
	struct bt_hci_evt_hdr hdr;
	struct net_buf *buf;
	bool discardable;

	if (remaining < sizeof(hdr)) {
		LOG_ERR("Not enough data for event header");
		return NULL;
	}

	discardable = hci_esp_hosted_mcu_evt_discardable(data, remaining);
	memcpy((void *)&hdr, data, sizeof(hdr));
	data += sizeof(hdr);
	remaining -= sizeof(hdr);

	if (remaining != hdr.len) {
		LOG_ERR("Event payload length mismatch (%u != %u)", remaining, hdr.len);
		return NULL;
	}

	buf = bt_buf_get_evt(hdr.evt, discardable, discardable ? K_NO_WAIT : K_SECONDS(10));
	if (!buf) {
		LOG_DBG("No event buffer available");
		return NULL;
	}

	net_buf_add_mem(buf, &hdr, sizeof(hdr));
	if (net_buf_tailroom(buf) < remaining) {
		LOG_ERR("Not enough room in event buffer");
		net_buf_unref(buf);
		return NULL;
	}

	net_buf_add_mem(buf, data, remaining);
	return buf;
}

static struct net_buf *hci_esp_hosted_mcu_acl_recv(const uint8_t *data, size_t remaining)
{
	struct bt_hci_acl_hdr hdr;
	struct net_buf *buf;

	if (remaining < sizeof(hdr)) {
		LOG_ERR("Not enough data for ACL header");
		return NULL;
	}

	memcpy((void *)&hdr, data, sizeof(hdr));
	data += sizeof(hdr);
	remaining -= sizeof(hdr);

	/*
	 * Validate before claiming a buffer, as the event path does. A
	 * malformed frame would otherwise take and return an ACL buffer from
	 * a pool that is under pressure exactly when frames go bad.
	 */
	if (remaining != sys_le16_to_cpu(hdr.len)) {
		LOG_ERR("ACL payload length mismatch");
		return NULL;
	}

	buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_NO_WAIT);
	if (!buf) {
		LOG_ERR("No ACL buffer available");
		return NULL;
	}

	net_buf_add_mem(buf, &hdr, sizeof(hdr));

	if (net_buf_tailroom(buf) < remaining) {
		LOG_ERR("Not enough room in ACL buffer");
		net_buf_unref(buf);
		return NULL;
	}

	net_buf_add_mem(buf, data, remaining);
	return buf;
}

/* Rebuild and deliver one queued frame; may block on the host buffer pools. */
static void hci_esp_hosted_mcu_deliver(const struct device *dev,
				       const struct hci_esp_hosted_mcu_rx_frame *frame)
{
	const uint8_t *payload = frame->data;
	uint16_t len = frame->len;
	struct net_buf *buf;
	uint8_t h4_type;

	if (len < 1) {
		return;
	}

	/*
	 * The coprocessor carries the controller frame whole, so the H4 packet-type
	 * indicator is the first payload byte and the HCI body follows.
	 */
	h4_type = payload[0];
	payload++;
	len--;

	switch (h4_type) {
	case BT_HCI_H4_EVT:
		buf = hci_esp_hosted_mcu_evt_recv(payload, len);
		break;
	case BT_HCI_H4_ACL:
		buf = hci_esp_hosted_mcu_acl_recv(payload, len);
		break;
	default:
		LOG_ERR("Unknown HCI packet type %u", h4_type);
		return;
	}

	if (buf != NULL) {
		bt_hci_recv(dev, buf);
	}
}

/*
 * Receive callback registered with the esp-hosted-mcu core for HCI_IF frames. Runs
 * on the shared core receive thread, so it only copies the frame into a slab
 * slot and queues it; the delivery thread does the blocking reconstruction.
 */
static void hci_esp_hosted_mcu_rx(const uint8_t *payload, uint16_t len, uint8_t pkt_type,
				  void *user_data)
{
	struct hci_esp_hosted_mcu_rx_frame *frame;

	ARG_UNUSED(pkt_type);
	ARG_UNUSED(user_data);

	if (len < 1 || len > ESP_HOSTED_MCU_MAX_PAYLOAD) {
		return;
	}

	if (k_mem_slab_alloc(&hci_esp_hosted_mcu_rx_slab, (void **)&frame, K_NO_WAIT) != 0) {
		LOG_WRN("No RX slot, dropping frame");
		return;
	}

	frame->len = len;
	memcpy(frame->data, payload, len);
	k_fifo_put(&hci_esp_hosted_mcu_rx_fifo, frame);
}

static void hci_esp_hosted_mcu_rx_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		struct hci_esp_hosted_mcu_rx_frame *frame =
			k_fifo_get(&hci_esp_hosted_mcu_rx_fifo, K_FOREVER);

		if (hci_esp_hosted_mcu_rx_dev != NULL) {
			hci_esp_hosted_mcu_deliver(hci_esp_hosted_mcu_rx_dev, frame);
		}

		k_mem_slab_free(&hci_esp_hosted_mcu_rx_slab, (void *)frame);
	}
}

K_THREAD_STACK_DEFINE(hci_esp_hosted_mcu_rx_stack, CONFIG_BT_HCI_ESP_HOSTED_MCU_RX_STACK_SIZE);
static struct k_thread hci_esp_hosted_mcu_rx_thread_data;

static int hci_esp_hosted_mcu_send(const struct device *dev, struct net_buf *buf)
{
	uint8_t pkt_type;
	int ret;

	ARG_UNUSED(dev);

	if (buf->len < 1) {
		return -EINVAL;
	}

	/* The leading byte is the H4 packet-type indicator. */
	pkt_type = net_buf_pull_u8(buf);

	ret = esp_hosted_mcu_send_frame(ESP_HOSTED_MCU_HCI_IF, 0, pkt_type, buf->data, buf->len);
	if (ret < 0) {
		LOG_ERR("HCI send failed (%d)", ret);
		return ret;
	}

	net_buf_unref(buf);
	return 0;
}

/* Drive one coprocessor Bluetooth FeatureControl command. */
static int hci_esp_hosted_mcu_feature_bt(RpcFeatureCommand command)
{
	Rpc req = (Rpc)Rpc_init_zero;
	Rpc resp;

	req.msg_id = RpcId_Req_FeatureControl;
	req.which_payload = Rpc_req_feature_control_tag;
	req.payload.req_feature_control.feature = RpcFeature_Feature_Bluetooth;
	req.payload.req_feature_control.command = command;
	req.payload.req_feature_control.option = RpcFeatureOption_Feature_Option_None;

	return esp_hosted_mcu_rpc_call(&req, &resp, ESP_HOSTED_MCU_RPC_TIMEOUT);
}

/*
 * Ask the coprocessor to initialise and enable its Bluetooth controller. Once these
 * succeed the controller is a raw H4 bridge over HCI_IF frames.
 */
static int hci_esp_hosted_mcu_enable_controller(void)
{
	int ret;

	/*
	 * A warm host reboot can leave the controller enabled from a previous
	 * run, which wedges its scan state. Deinitialise first so init starts
	 * from a clean controller; ignore the error when it was not yet up.
	 */
	(void)hci_esp_hosted_mcu_feature_bt(RpcFeatureCommand_Feature_Command_BT_Deinit);

	ret = hci_esp_hosted_mcu_feature_bt(RpcFeatureCommand_Feature_Command_BT_Init);
	if (ret) {
		LOG_ERR("coprocessor BT init failed (%d)", ret);
		return -EIO;
	}

	ret = hci_esp_hosted_mcu_feature_bt(RpcFeatureCommand_Feature_Command_BT_Enable);
	if (ret) {
		LOG_ERR("coprocessor BT enable failed (%d)", ret);
		return -EIO;
	}

	return 0;
}

static int hci_esp_hosted_mcu_open(const struct device *dev)
{
	struct hci_esp_hosted_mcu_data *data = dev->data;
	int ret;

	if (esp_hosted_mcu_get_dev() == NULL) {
		LOG_ERR("esp-hosted-mcu core not ready");
		return -ENODEV;
	}

	/*
	 * Publish the device for the delivery thread and start it before the
	 * receive callback is registered, so a frame can never reach an
	 * unstarted thread.
	 */
	hci_esp_hosted_mcu_rx_dev = dev;
	if (!data->rx_started) {
		k_thread_create(&hci_esp_hosted_mcu_rx_thread_data, hci_esp_hosted_mcu_rx_stack,
				K_THREAD_STACK_SIZEOF(hci_esp_hosted_mcu_rx_stack),
				hci_esp_hosted_mcu_rx_thread, NULL, NULL, NULL,
				K_PRIO_COOP(CONFIG_BT_HCI_ESP_HOSTED_MCU_RX_PRIO), 0, K_NO_WAIT);
		k_thread_name_set(&hci_esp_hosted_mcu_rx_thread_data, "hci_esp_hosted_mcu_rx");
		data->rx_started = true;
	}

	/*
	 * Bring the controller up before routing HCI frames into the host. The
	 * enable sequence runs as control-plane RPCs over the core, not over
	 * this callback, so registering only after it succeeds keeps a stray
	 * controller event (for example a leftover after a warm reboot) from
	 * reaching the host stack before it has sent HCI Reset.
	 */
	ret = hci_esp_hosted_mcu_enable_controller();
	if (ret) {
		return ret;
	}

	/* Route HCI_IF frames from the core demux into this driver. */
	esp_hosted_mcu_register_if(ESP_HOSTED_MCU_HCI_IF, hci_esp_hosted_mcu_rx, (void *)dev);

	data->dev = dev;
	return 0;
}

static int hci_esp_hosted_mcu_close(const struct device *dev)
{
	struct hci_esp_hosted_mcu_data *data = dev->data;
	struct hci_esp_hosted_mcu_rx_frame *frame;

	/* Stop the core from dispatching further HCI frames to this driver. */
	esp_hosted_mcu_unregister_if(ESP_HOSTED_MCU_HCI_IF);
	hci_esp_hosted_mcu_rx_dev = NULL;

	/* Drop any frames already queued for delivery. */
	while ((frame = k_fifo_get(&hci_esp_hosted_mcu_rx_fifo, K_NO_WAIT)) != NULL) {
		k_mem_slab_free(&hci_esp_hosted_mcu_rx_slab, (void *)frame);
	}

	/* Bring the coprocessor controller down so a later open starts clean. */
	(void)hci_esp_hosted_mcu_feature_bt(RpcFeatureCommand_Feature_Command_BT_Disable);
	(void)hci_esp_hosted_mcu_feature_bt(RpcFeatureCommand_Feature_Command_BT_Deinit);

	data->dev = NULL;
	return 0;
}

static DEVICE_API(bt_hci, hci_esp_hosted_mcu_api) = {
	.open = hci_esp_hosted_mcu_open,
	.close = hci_esp_hosted_mcu_close,
	.send = hci_esp_hosted_mcu_send,
};

#define HCI_ESP_HOSTED_MCU_INIT(inst)                                                              \
	static struct hci_esp_hosted_mcu_data hci_esp_hosted_mcu_data_##inst;                      \
	static const struct bt_hci_driver_config hci_esp_hosted_mcu_config_##inst =                \
		BT_DT_HCI_DRIVER_CONFIG_INST_GET(inst);                                            \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, &hci_esp_hosted_mcu_data_##inst,                   \
			      &hci_esp_hosted_mcu_config_##inst, POST_KERNEL,                      \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &hci_esp_hosted_mcu_api);

DT_INST_FOREACH_STATUS_OKAY(HCI_ESP_HOSTED_MCU_INIT)
