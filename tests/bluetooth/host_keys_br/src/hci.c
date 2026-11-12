/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/device.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

#define DT_DRV_COMPAT zephyr_bt_hci_test

static int driver_open(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int driver_send(const struct device *dev, struct net_buf *buf)
{
	struct bt_hci_cmd_hdr *cmd;
	struct bt_hci_evt_hdr *hdr;
	struct bt_hci_evt_cmd_complete *cc;
	struct net_buf *evt;
	uint16_t opcode;
	uint8_t *rp;

	zassert_equal(net_buf_pull_u8(buf), BT_HCI_H4_CMD);
	cmd = net_buf_pull_mem(buf, sizeof(*cmd));
	opcode = sys_le16_to_cpu(cmd->opcode);

	evt = bt_buf_get_evt(BT_HCI_EVT_CMD_COMPLETE, false, K_FOREVER);
	zassert_not_null(evt);
	hdr = net_buf_add(evt, sizeof(*hdr));
	hdr->evt = BT_HCI_EVT_CMD_COMPLETE;
	hdr->len = sizeof(*cc) + 64;
	cc = net_buf_add(evt, sizeof(*cc));
	cc->ncmd = 1;
	cc->opcode = sys_cpu_to_le16(opcode);
	rp = net_buf_add(evt, 64);
	memset(rp, 0, 64);

	switch (opcode) {
	case BT_HCI_OP_READ_LOCAL_FEATURES: {
		struct bt_hci_rp_read_local_features *features = (void *)rp;

		features->features[4] = BIT(6); /* Dual-mode controller. */
		break;
	}
	case BT_HCI_OP_READ_BUFFER_SIZE: {
		struct bt_hci_rp_read_buffer_size *size = (void *)rp;

		size->acl_max_len = sys_cpu_to_le16(64);
		size->acl_max_num = sys_cpu_to_le16(2);
		break;
	}
	case BT_HCI_OP_LE_READ_BUFFER_SIZE: {
		struct bt_hci_rp_le_read_buffer_size *size = (void *)rp;

		size->le_max_len = sys_cpu_to_le16(64);
		size->le_max_num = 2;
		break;
	}
	case BT_HCI_OP_READ_BD_ADDR: {
		struct bt_hci_rp_read_bd_addr *addr = (void *)rp;

		addr->bdaddr = (bt_addr_t){{6, 5, 4, 3, 2, 1}};
		break;
	}
	case BT_HCI_OP_READ_LOCAL_VERSION_INFO:
	case BT_HCI_OP_READ_SUPPORTED_COMMANDS:
	case BT_HCI_OP_LE_READ_LOCAL_FEATURES:
	case BT_HCI_OP_LE_WRITE_LE_HOST_SUPP:
	case BT_HCI_OP_LE_SET_EVENT_MASK:
	case BT_HCI_OP_SET_EVENT_MASK:
	case BT_HCI_OP_WRITE_SSP_MODE:
	case BT_HCI_OP_WRITE_INQUIRY_MODE:
	case BT_HCI_OP_WRITE_LOCAL_NAME:
	case BT_HCI_OP_WRITE_CLASS_OF_DEVICE:
	case BT_HCI_OP_WRITE_PAGE_TIMEOUT:
	case BT_HCI_OP_READ_DEFAULT_LINK_POLICY_SETTINGS:
	case BT_HCI_OP_WRITE_DEFAULT_LINK_POLICY_SETTINGS:
		break;
	default:
		zassert_unreachable("Unexpected HCI command 0x%04x", opcode);
	}

	net_buf_unref(buf);
	bt_hci_recv(dev, evt);
	return 0;
}

static DEVICE_API(bt_hci, driver_api) = {
	.open = driver_open,
	.send = driver_send,
};

static struct bt_hci_driver_data driver_data;
static const struct bt_hci_driver_config driver_config = BT_DT_HCI_DRIVER_CONFIG_INST_GET(0);

DEVICE_DT_INST_DEFINE(0, NULL, NULL, &driver_data, &driver_config, POST_KERNEL,
		      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &driver_api);
