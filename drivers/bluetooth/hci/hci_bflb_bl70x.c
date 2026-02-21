/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * HCI driver for the Bouffalo Lab BL702 on-chip BLE controller.
 *
 * The controller is a precompiled binary blob communicating via an on-chip
 * HCI interface (bt_onchiphci_send / bt_onchiphci_interface_init). This
 * driver translates between Zephyr's HCI net_buf format and the vendor's
 * internal packet structures.
 */

#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/kernel.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/drivers/syscon.h>
#include <bflb_soc.h>
#include <hci_onchip.h>
#include <ble_lib_api.h>
#include <bl702_rf_public.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_hci_bflb_bl70x, CONFIG_BT_HCI_DRIVER_LOG_LEVEL);

#define DT_DRV_COMPAT bflb_bl70x_bt_hci

#define RX_MSG_PARAM_MAX_LEN UINT8_MAX
#if defined(CONFIG_BT_CONN)
#define RX_MSGQ_SIZE (CONFIG_BT_BUF_EVT_RX_COUNT + CONFIG_BT_BUF_ACL_RX_COUNT_EXTRA)
#else
#define RX_MSGQ_SIZE CONFIG_BT_BUF_EVT_RX_COUNT
#endif

#define EFUSE_WIFI_MAC_LOW_OFFSET  0x14
#define EFUSE_WIFI_MAC_HIGH_OFFSET 0x18

/* Queued RX message for deferred processing */
struct rx_msg {
	uint8_t pkt_type;
	uint16_t src_id;
	uint16_t param_len;
	uint8_t param[RX_MSG_PARAM_MAX_LEN];
};

struct bt_bflb_data {
	bt_hci_recv_t recv;
};

/* The BL70X controller blob exposes a single global on-chip HCI interface. */
K_MSGQ_DEFINE(rx_msgq, sizeof(struct rx_msg), RX_MSGQ_SIZE, 4);

/* Forward declarations for platform shims */
extern int bl_wireless_mac_addr_set(uint8_t mac[8]);
extern void bflb_ble_irq_setup(void);

static void read_mac_from_efuse(uint8_t mac[6])
{
	const struct device *efuse = DEVICE_DT_GET(DT_NODELABEL(efuse));
	uint32_t mac_low, mac_high;

	if (!device_is_ready(efuse) ||
	    syscon_read_reg(efuse, EFUSE_WIFI_MAC_LOW_OFFSET, &mac_low) ||
	    syscon_read_reg(efuse, EFUSE_WIFI_MAC_HIGH_OFFSET, &mac_high)) {
		memset(mac, 0, 6);
		return;
	}

	sys_put_le32(mac_low, &mac[0]);
	sys_put_le16((uint16_t)mac_high, &mac[4]);
}

/*
 * Forward incoming controller packet to the Zephyr BT host.
 */
static void packet_to_host(uint8_t pkt_type, uint16_t src_id, uint8_t *param, uint16_t param_len,
			   const struct device *dev)
{
	struct bt_bflb_data *hci = dev->data;
	struct net_buf *buf;

	switch (pkt_type) {
	case BT_HCI_CMD_CMP_EVT: {
		buf = bt_hci_cmd_complete_create(src_id, param_len);
		if (!buf) {
			LOG_ERR("No buf for cmd complete");
			return;
		}
		net_buf_add_mem(buf, param, param_len);
		break;
	}
	case BT_HCI_CMD_STAT_EVT: {
		uint8_t status = param ? param[0] : 0;

		buf = bt_hci_cmd_status_create(src_id, status);
		if (!buf) {
			LOG_ERR("No buf for cmd status");
			return;
		}
		break;
	}
	case BT_HCI_LE_EVT: {
		buf = bt_hci_evt_create(BT_HCI_EVT_LE_META_EVENT, param_len);
		if (!buf) {
			LOG_WRN("No buf for LE event");
			return;
		}
		net_buf_add_mem(buf, param, param_len);
		break;
	}
	case BT_HCI_EVT: {
		buf = bt_hci_evt_create(src_id, param_len);
		if (!buf) {
			LOG_WRN("No buf for event 0x%02x", src_id);
			return;
		}
		net_buf_add_mem(buf, param, param_len);
		break;
	}
#if defined(CONFIG_BT_CONN)
	case BT_HCI_ACL_DATA: {
		buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_NO_WAIT);
		if (!buf) {
			LOG_WRN("No buf for ACL data");
			return;
		}
		net_buf_add_mem(buf, param, param_len);
		break;
	}
#endif
	default:
		LOG_WRN("Unknown pkt type %u", pkt_type);
		return;
	}

	if (hci->recv) {
		hci->recv(dev, buf);
	} else {
		net_buf_unref(buf);
	}
}

/*
 * RX thread — processes queued messages from the controller callback.
 */
static void rx_thread_func(void *p1, void *p2, void *p3)
{
	const struct device *dev = p1;
	struct rx_msg msg;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		if (k_msgq_get(&rx_msgq, &msg, K_FOREVER) == 0) {
			packet_to_host(msg.pkt_type, msg.src_id, msg.param, msg.param_len, dev);
		}
	}
}

static K_THREAD_STACK_DEFINE(rx_thread_stack, CONFIG_BT_DRV_RX_STACK_SIZE);
static struct k_thread rx_thread_data;

/*
 * Controller RX callback — called from controller context (possibly ISR).
 * Queue message for RX thread to process.
 */
static void controller_rx_cb(uint8_t pkt_type, uint16_t src_id, uint8_t *param, uint8_t param_len)
{
	struct rx_msg msg;
	int ret;

#if defined(CONFIG_BT_CONN)
	if (pkt_type == BT_HCI_ACL_DATA) {
		msg.pkt_type = pkt_type;
		msg.src_id = src_id;
		msg.param_len = bt_onchiphci_handle_rx_acl(param, msg.param);

		ret = k_msgq_put(&rx_msgq, &msg, K_NO_WAIT);
		if (ret != 0) {
			LOG_ERR("RX queue full, dropping ACL packet");
		}
		return;
	}
#endif

	msg.pkt_type = pkt_type;
	msg.src_id = src_id;
	msg.param_len = param_len;
	if (param_len > 0 && param) {
		memcpy(msg.param, param, param_len);
	}

	ret = k_msgq_put(&rx_msgq, &msg, K_NO_WAIT);
	if (ret != 0) {
		LOG_ERR("RX queue full, dropping packet (type=%u len=%u)", pkt_type, param_len);
	}
}

/*
 * send — Translate Zephyr HCI net_buf to vendor on-chip HCI packet.
 */
static int bt_bflb_send(const struct device *dev, struct net_buf *buf)
{
	uint8_t h4_type;
	uint8_t pkt_type;
	uint16_t dest_id = 0;
	hci_pkt_struct pkt;
	int ret;

	ARG_UNUSED(dev);

	h4_type = net_buf_pull_u8(buf);

	switch (h4_type) {
	case BT_HCI_H4_CMD: {
		struct bt_hci_cmd_hdr *chdr;

		if (buf->len < sizeof(*chdr)) {
			ret = -EINVAL;
			goto done;
		}

		chdr = (void *)buf->data;

		pkt_type = BT_HCI_CMD;
		pkt.p.hci_cmd.opcode = sys_le16_to_cpu(chdr->opcode);
		pkt.p.hci_cmd.param_len = chdr->param_len;
		net_buf_pull(buf, sizeof(*chdr));
		pkt.p.hci_cmd.params = buf->data;
		break;
	}
	case BT_HCI_H4_ACL: {
		struct bt_hci_acl_hdr *acl;
		uint16_t connhdl_l2cf;

		if (buf->len < sizeof(*acl)) {
			ret = -EINVAL;
			goto done;
		}

		acl = (void *)buf->data;
		pkt_type = BT_HCI_ACL_DATA;
		connhdl_l2cf = sys_le16_to_cpu(acl->handle);
		dest_id = bt_acl_handle(connhdl_l2cf);

		pkt.p.acl_data.conhdl = dest_id;
		pkt.p.acl_data.pb_bc_flag = bt_acl_flags(connhdl_l2cf);
		pkt.p.acl_data.len = sys_le16_to_cpu(acl->len);
		net_buf_pull(buf, sizeof(*acl));
		pkt.p.acl_data.buffer = buf->data;
		break;
	}
	default:
		ret = -EINVAL;
		goto done;
	}

	ret = bt_onchiphci_send(pkt_type, dest_id, &pkt);
	if (ret != 0) {
		LOG_ERR("bt_onchiphci_send failed: %d", ret);
		ret = (ret < 0) ? ret : -EIO;
	}

done:
	net_buf_unref(buf);
	return ret;
}

static void bflb_ble_rf_init(void)
{
	uint8_t mac[8] = {0};

	read_mac_from_efuse(mac);
	bl_wireless_mac_addr_set(mac);

	rf702_set_init_tsen_value(0);
}

static int bt_bflb_init(const struct device *dev)
{
	k_thread_create(&rx_thread_data, rx_thread_stack, K_THREAD_STACK_SIZEOF(rx_thread_stack),
			rx_thread_func, (void *)dev, NULL, NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);
	k_thread_name_set(&rx_thread_data, "bt_bflb_rx");

	return 0;
}

static int bt_bflb_open(const struct device *dev, bt_hci_recv_t recv)
{
	struct bt_bflb_data *hci = dev->data;
	uint8_t hci_ret;

	hci->recv = recv;

	/* Initialize RF subsystem */
	bflb_ble_rf_init();

	/* Register MSOFT and BLE IRQ handlers before controller init.
	 * The blob triggers MSOFT (IRQ 3) for deferred context switching
	 * and registers the BLE IRQ handler via bl_irq_register/enable.
	 */
	bflb_ble_irq_setup();

	/* Start controller */
	ble_controller_init(CONFIG_BT_BFLB_CTLR_TASK_PRIO);

	/* Register on-chip HCI interface */
	hci_ret = bt_onchiphci_interface_init(controller_rx_cb);

	if (hci_ret != 0) {
		LOG_ERR("bt_onchiphci_interface_init failed: %u", hci_ret);
		ble_controller_deinit();
		return -EIO;
	}

	LOG_INF("BL70X BLE controller initialized");

	return 0;
}

static int bt_bflb_close(const struct device *dev)
{
	struct bt_bflb_data *hci = dev->data;

	ble_controller_deinit();

	k_msgq_purge(&rx_msgq);

	hci->recv = NULL;

	LOG_INF("BL70X BLE controller stopped");
	return 0;
}

static DEVICE_API(bt_hci, bt_bflb_drv) = {
	.open = bt_bflb_open,
	.send = bt_bflb_send,
	.close = bt_bflb_close,
};

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "BL70X BT HCI driver supports exactly one instance");

static struct bt_bflb_data bt_bflb_data_0;

DEVICE_DT_INST_DEFINE(0, bt_bflb_init, NULL, &bt_bflb_data_0, NULL, POST_KERNEL,
		      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &bt_bflb_drv);
