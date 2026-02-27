/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * HCI driver for Bouffalo Lab on-chip BLE controllers.
 *
 * The controller is a precompiled binary blob communicating via an on-chip
 * HCI interface (bt_onchiphci_send / bt_onchiphci_interface_init). This
 * driver translates between Zephyr's HCI net_buf format and the vendor's
 * internal packet structures.
 */

#define DT_DRV_COMPAT bflb_bt_hci

#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <string.h>

#include <hci_onchip.h>

#if defined(CONFIG_BT_BFLB_BL70X)

#include <bflb_soc.h>
#include <ble_lib_api.h>
#include <bl702_rf_public.h>

#define bflb_controller_init(prio)     ble_controller_init(prio)
#define bflb_controller_deinit()       ble_controller_deinit()
#define bflb_rf_set_init_tsen_value(v) rf702_set_init_tsen_value(v)

#elif defined(CONFIG_BT_BFLB_BL70XL)

#include <btble_lib_api.h>
#include <btblecontroller_port.h>
#include <bl702l_rf.h>

#define bflb_controller_init(prio)     btble_controller_init(prio)
#define bflb_controller_deinit()       btble_controller_deinit()
#define bflb_rf_set_init_tsen_value(v) rf_set_init_tsen_value(v)

#endif

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_hci_bflb, CONFIG_BT_HCI_DRIVER_LOG_LEVEL);

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "Exactly one bflb bt-hci instance required");

struct bt_bflb_data {
	bt_hci_recv_t recv;
};

static K_FIFO_DEFINE(rx_fifo);

/* Forward declarations for platform shims */
extern int bl_wireless_mac_addr_set(uint8_t mac[8]);
#if defined(CONFIG_BT_BFLB_BL70X)
extern void bflb_ble_irq_setup(void);
#endif

/*
 * Allocate an event net_buf and stamp the HCI event header.
 *
 * Uses K_NO_WAIT because the caller may be in ISR context. Returns NULL if
 * the pool is exhausted.
 */
static struct net_buf *bflb_evt_alloc(uint8_t evt, uint8_t payload_len)
{
	struct net_buf *buf;
	struct bt_hci_evt_hdr *hdr;

	buf = bt_buf_get_evt(evt, false, K_NO_WAIT);
	if (buf == NULL) {
		return NULL;
	}

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->evt = evt;
	hdr->len = payload_len;

	return buf;
}

/*
 * Controller RX callback — called from controller context (possibly ISR).
 * Build the full HCI frame directly into a net_buf and queue it for the
 * RX thread to hand off to the host; this avoids an intermediate copy.
 *
 * ACL data requires immediate extraction: bt_onchiphci_handle_rx_acl
 * dereferences param as a ke_msg* to access Exchange Memory, and the
 * ke_msg may be freed after this callback returns.
 */
static void controller_rx_cb(uint8_t pkt_type, uint16_t src_id, uint8_t *param, uint8_t param_len)
{
	struct net_buf *buf = NULL;

	switch (pkt_type) {
	case BT_HCI_CMD_CMP_EVT: {
		struct bt_hci_evt_cmd_complete *cc;

		buf = bflb_evt_alloc(BT_HCI_EVT_CMD_COMPLETE, sizeof(*cc) + param_len);
		if (buf == NULL) {
			LOG_ERR("No buf for cmd complete");
			return;
		}

		cc = net_buf_add(buf, sizeof(*cc));
		cc->ncmd = 1U;
		cc->opcode = sys_cpu_to_le16(src_id);

		if (param != NULL && param_len > 0U) {
			net_buf_add_mem(buf, param, param_len);
		}
		break;
	}
	case BT_HCI_CMD_STAT_EVT: {
		struct bt_hci_evt_cmd_status *cs;

		buf = bflb_evt_alloc(BT_HCI_EVT_CMD_STATUS, sizeof(*cs));
		if (buf == NULL) {
			LOG_ERR("No buf for cmd status");
			return;
		}

		cs = net_buf_add(buf, sizeof(*cs));
		cs->status = (param != NULL) ? param[0] : 0U;
		cs->ncmd = 1U;
		cs->opcode = sys_cpu_to_le16(src_id);
		break;
	}
	case BT_HCI_LE_EVT: {
		buf = bflb_evt_alloc(BT_HCI_EVT_LE_META_EVENT, param_len);
		if (buf == NULL) {
			LOG_WRN("No buf for LE event");
			return;
		}

		if (param != NULL && param_len > 0U) {
			net_buf_add_mem(buf, param, param_len);
		}
		break;
	}
	case BT_HCI_EVT: {
		buf = bflb_evt_alloc((uint8_t)src_id, param_len);
		if (buf == NULL) {
			LOG_WRN("No buf for event 0x%02x", src_id);
			return;
		}

		if (param != NULL && param_len > 0U) {
			net_buf_add_mem(buf, param, param_len);
		}
		break;
	}
#if defined(CONFIG_BT_CONN)
	case BT_HCI_ACL_DATA: {
		uint16_t acl_len;

		buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_NO_WAIT);
		if (buf == NULL) {
			LOG_WRN("No buf for ACL data");
			return;
		}

		acl_len = bt_onchiphci_handle_rx_acl(param, net_buf_tail(buf));
		net_buf_add(buf, acl_len);
		break;
	}
#endif
	default:
		LOG_WRN("Unknown pkt type %u", pkt_type);
		return;
	}

	k_fifo_put(&rx_fifo, buf);
}

/*
 * RX thread — drains queued net_bufs and hands them to the host.
 */
static void rx_thread_func(void *p1, void *p2, void *p3)
{
	const struct device *dev = p1;
	struct bt_bflb_data *hci = dev->data;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		struct net_buf *buf = k_fifo_get(&rx_fifo, K_FOREVER);

		if (hci->recv != NULL) {
			hci->recv(dev, buf);
		} else {
			net_buf_unref(buf);
		}
	}
}

static K_THREAD_STACK_DEFINE(rx_thread_stack, CONFIG_BT_BFLB_RX_THREAD_STACK_SIZE);
static struct k_thread rx_thread_data;

/*
 * send — Translate Zephyr HCI net_buf to vendor on-chip HCI packet.
 */
static int bt_bflb_send(const struct device *dev, struct net_buf *buf)
{
	uint8_t h4_type;
	uint8_t pkt_type;
	uint16_t dest_id = 0U;
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

		chdr = (struct bt_hci_cmd_hdr *)buf->data;

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

		acl = (struct bt_hci_acl_hdr *)buf->data;
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
	uint8_t mac[8] = {0U};

	/*
	 * hwinfo returns the MAC in network (big-endian) byte order;
	 * bl_wireless_mac_addr_set expects little-endian order.
	 */
	hwinfo_get_device_id(mac, 6);
	sys_mem_swap(mac, 6);

	bl_wireless_mac_addr_set(mac);
	bflb_rf_set_init_tsen_value(0U);
}

static int bt_bflb_open(const struct device *dev, bt_hci_recv_t recv)
{
	struct bt_bflb_data *hci = dev->data;
	uint8_t hci_ret;

	hci->recv = recv;

	bflb_ble_rf_init();

#if defined(CONFIG_BT_BFLB_BL70X)
	bflb_ble_irq_setup();
#endif
	bflb_controller_init(CONFIG_BT_BFLB_CTLR_TASK_PRIO);

	hci_ret = bt_onchiphci_interface_init(controller_rx_cb);
	if (hci_ret != 0) {
		LOG_ERR("bt_onchiphci_interface_init failed: %u", hci_ret);
		bflb_controller_deinit();
		return -EIO;
	}

	LOG_INF("BLE controller initialized");
	return 0;
}

static int bt_bflb_close(const struct device *dev)
{
	struct bt_bflb_data *hci = dev->data;

	bflb_controller_deinit();
	hci->recv = NULL;

	LOG_INF("BLE controller stopped");
	return 0;
}

static int bt_bflb_init(const struct device *dev)
{
	k_thread_create(&rx_thread_data, rx_thread_stack, K_THREAD_STACK_SIZEOF(rx_thread_stack),
			rx_thread_func, (void *)dev, NULL, NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);
	k_thread_name_set(&rx_thread_data, "bt_bflb_rx");
	return 0;
}

static DEVICE_API(bt_hci, bt_bflb_drv) = {
	.open = bt_bflb_open,
	.send = bt_bflb_send,
	.close = bt_bflb_close,
};

static struct bt_bflb_data bt_bflb_data_0 = {0};

DEVICE_DT_INST_DEFINE(0, bt_bflb_init, NULL, &bt_bflb_data_0, NULL, POST_KERNEL,
		      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &bt_bflb_drv);
