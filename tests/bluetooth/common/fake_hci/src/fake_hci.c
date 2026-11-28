/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include <fake_hci.h>

#define DT_DRV_COMPAT zephyr_bt_hci_test

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "The fake controller needs one zephyr,bt-hci-test node");

/* Large enough for the return parameters of any command that the Host sends */
#define GENERIC_RP_SIZE 64U

struct fake_hci_ctlr fake_hci = {
	/* BR/EDR Not Supported and LE Supported (Controller) */
	.features[4] = BIT(5) | BIT(6),
	.le_acl_len = BT_GAP_DATA_LEN_DEFAULT,
	.le_acl_num = 1U,
};

static const struct fake_hci_cb *test_cb;

void fake_hci_cb_set(const struct fake_hci_cb *cb)
{
	test_cb = cb;
}

static void host_recv(struct net_buf *buf)
{
	bt_hci_recv(DEVICE_DT_INST_GET(0), buf);
}

void fake_hci_recv(struct net_buf *buf)
{
	host_recv(buf);
	/* The Host's RX thread runs at a higher priority than the test thread */
	k_yield();
}

struct net_buf *fake_hci_evt(uint8_t evt, uint8_t len)
{
	struct bt_hci_evt_hdr *hdr;
	struct net_buf *buf;

	buf = bt_buf_get_evt(evt, false, K_FOREVER);
	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->evt = evt;
	hdr->len = len;

	return buf;
}

struct net_buf *fake_hci_le_meta_evt(uint8_t subevent, uint8_t len)
{
	struct bt_hci_evt_le_meta_event *meta;
	struct net_buf *buf;

	zassert_true(len <= UINT8_MAX - sizeof(*meta), "Event too long");

	buf = fake_hci_evt(BT_HCI_EVT_LE_META_EVENT, sizeof(*meta) + len);
	meta = net_buf_add(buf, sizeof(*meta));
	meta->subevent = subevent;

	return buf;
}

struct net_buf *fake_hci_cmd_complete(uint16_t opcode, uint8_t len)
{
	struct bt_hci_evt_cmd_complete *cc;
	struct net_buf *buf;

	zassert_true(len <= UINT8_MAX - sizeof(*cc), "Event too long");

	buf = fake_hci_evt(BT_HCI_EVT_CMD_COMPLETE, sizeof(*cc) + len);
	cc = net_buf_add(buf, sizeof(*cc));
	cc->ncmd = 1U;
	cc->opcode = sys_cpu_to_le16(opcode);

	return buf;
}

void fake_hci_cmd_status(uint16_t opcode, uint8_t status)
{
	struct bt_hci_evt_cmd_status *cs;
	struct net_buf *buf;

	buf = fake_hci_evt(BT_HCI_EVT_CMD_STATUS, sizeof(*cs));
	cs = net_buf_add(buf, sizeof(*cs));
	cs->status = status;
	cs->ncmd = 1U;
	cs->opcode = sys_cpu_to_le16(opcode);

	host_recv(buf);
}

static struct net_buf *read_local_features(uint16_t opcode)
{
	struct bt_hci_rp_read_local_features *rp;
	struct net_buf *buf;

	buf = fake_hci_cmd_complete(opcode, sizeof(*rp));
	rp = net_buf_add(buf, sizeof(*rp));
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memcpy(rp->features, fake_hci.features, sizeof(rp->features));

	return buf;
}

static struct net_buf *read_supported_commands(uint16_t opcode)
{
	struct bt_hci_rp_read_supported_commands *rp;
	struct net_buf *buf;

	buf = fake_hci_cmd_complete(opcode, sizeof(*rp));
	rp = net_buf_add(buf, sizeof(*rp));
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(rp->commands, 0xFF, sizeof(rp->commands));

	return buf;
}

static struct net_buf *read_bd_addr(uint16_t opcode)
{
	struct bt_hci_rp_read_bd_addr *rp;
	struct net_buf *buf;

	buf = fake_hci_cmd_complete(opcode, sizeof(*rp));
	rp = net_buf_add(buf, sizeof(*rp));
	rp->status = BT_HCI_ERR_SUCCESS;
	bt_addr_copy(&rp->bdaddr, &fake_hci.bd_addr);

	return buf;
}

static struct net_buf *le_read_local_features(uint16_t opcode)
{
	struct bt_hci_rp_le_read_local_features *rp;
	struct net_buf *buf;

	buf = fake_hci_cmd_complete(opcode, sizeof(*rp));
	rp = net_buf_add(buf, sizeof(*rp));
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memcpy(rp->features, fake_hci.le_features, sizeof(rp->features));

	return buf;
}

static struct net_buf *le_read_buffer_size(uint16_t opcode)
{
	struct bt_hci_rp_le_read_buffer_size *rp;
	struct net_buf *buf;

	buf = fake_hci_cmd_complete(opcode, sizeof(*rp));
	rp = net_buf_add(buf, sizeof(*rp));
	rp->status = BT_HCI_ERR_SUCCESS;
	rp->le_max_len = sys_cpu_to_le16(fake_hci.le_acl_len);
	rp->le_max_num = fake_hci.le_acl_num;

	return buf;
}

static struct net_buf *le_read_supp_states(uint16_t opcode)
{
	struct bt_hci_rp_le_read_supp_states *rp;
	struct net_buf *buf;

	buf = fake_hci_cmd_complete(opcode, sizeof(*rp));
	rp = net_buf_add(buf, sizeof(*rp));
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(rp->le_states, 0xFF, sizeof(rp->le_states));

	return buf;
}

static struct net_buf *le_read_max_adv_data_len(uint16_t opcode)
{
	struct bt_hci_rp_le_read_max_adv_data_len *rp;
	struct net_buf *buf;

	buf = fake_hci_cmd_complete(opcode, sizeof(*rp));
	rp = net_buf_add(buf, sizeof(*rp));
	rp->status = BT_HCI_ERR_SUCCESS;
	rp->max_adv_data_len = sys_cpu_to_le16(BT_GAP_ADV_MAX_EXT_ADV_DATA_LEN);

	return buf;
}

static struct net_buf *generic_success(uint16_t opcode)
{
	struct bt_hci_evt_cc_status *rp;
	struct net_buf *buf;

	buf = fake_hci_cmd_complete(opcode, GENERIC_RP_SIZE);
	rp = net_buf_add(buf, GENERIC_RP_SIZE);
	(void)memset(rp, 0, GENERIC_RP_SIZE);
	rp->status = BT_HCI_ERR_SUCCESS;

	return buf;
}

static void cmd_handle(uint16_t opcode)
{
	struct net_buf *buf;

	switch (opcode) {
	case BT_HCI_OP_HOST_NUM_COMPLETED_PACKETS:
		/* Has no response */
		return;
	case BT_HCI_OP_READ_LOCAL_FEATURES:
		buf = read_local_features(opcode);
		break;
	case BT_HCI_OP_READ_SUPPORTED_COMMANDS:
		buf = read_supported_commands(opcode);
		break;
	case BT_HCI_OP_READ_BD_ADDR:
		buf = read_bd_addr(opcode);
		break;
	case BT_HCI_OP_LE_READ_LOCAL_FEATURES:
		buf = le_read_local_features(opcode);
		break;
	case BT_HCI_OP_LE_READ_BUFFER_SIZE:
		buf = le_read_buffer_size(opcode);
		break;
	case BT_HCI_OP_LE_READ_SUPP_STATES:
		buf = le_read_supp_states(opcode);
		break;
	case BT_HCI_OP_LE_READ_MAX_ADV_DATA_LEN:
		buf = le_read_max_adv_data_len(opcode);
		break;
	default:
		buf = generic_success(opcode);
		break;
	}

	host_recv(buf);
}

static int driver_open(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int driver_close(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int driver_send(const struct device *dev, struct net_buf *buf)
{
	struct bt_hci_cmd_hdr *hdr;
	uint16_t opcode;
	uint8_t type;

	ARG_UNUSED(dev);

	type = net_buf_pull_u8(buf);
	zassert_equal(type, BT_HCI_H4_CMD, "Unexpected packet type %u", type);

	hdr = net_buf_pull_mem(buf, sizeof(*hdr));
	opcode = sys_le16_to_cpu(hdr->opcode);

	if ((test_cb == NULL) || (test_cb->cmd == NULL) || !test_cb->cmd(opcode, buf)) {
		cmd_handle(opcode);
	}

	net_buf_unref(buf);

	return 0;
}

static DEVICE_API(bt_hci, driver_api) = {
	.open = driver_open,
	.close = driver_close,
	.send = driver_send,
};

static struct bt_hci_driver_data driver_data;
static const struct bt_hci_driver_config driver_config = BT_DT_HCI_DRIVER_CONFIG_INST_GET(0);

DEVICE_DT_INST_DEFINE(0, NULL, NULL, &driver_data, &driver_config, POST_KERNEL,
		      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &driver_api);
