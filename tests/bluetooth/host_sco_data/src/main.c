/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/classic/sco.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "host/conn_internal.h"
#include "host/hci_core.h"
#include "host/classic/sco_internal.h"

#define DT_DRV_COMPAT zephyr_bt_hci_test

#define TEST_HANDLE      0x0001U
#define GENERIC_RP_SIZE  64U
#define LE_ACL_MAX_LEN   27U
#define LE_ACL_MAX_NUM   3U
#define SCO_MAX_LEN      60U
#define SCO_MAX_NUM      3U

struct cmd_handler {
	uint16_t opcode;
	uint8_t len;
	void (*handler)(struct net_buf *buf, struct net_buf **evt, uint8_t len, uint16_t opcode);
};

static void evt_create(struct net_buf *buf, uint8_t evt, uint8_t len)
{
	struct bt_hci_evt_hdr *hdr;

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->evt = evt;
	hdr->len = len;
}

static void *cmd_complete(struct net_buf **buf, uint8_t len, uint16_t opcode)
{
	struct bt_hci_evt_cmd_complete *cc;

	*buf = bt_buf_get_evt(BT_HCI_EVT_CMD_COMPLETE, false, K_FOREVER);
	evt_create(*buf, BT_HCI_EVT_CMD_COMPLETE, sizeof(*cc) + len);
	cc = net_buf_add(*buf, sizeof(*cc));
	cc->ncmd = 1U;
	cc->opcode = sys_cpu_to_le16(opcode);

	return net_buf_add(*buf, len);
}

static void generic_success(struct net_buf *buf, struct net_buf **evt, uint8_t len, uint16_t opcode)
{
	struct bt_hci_evt_cc_status *rp;

	ARG_UNUSED(buf);

	rp = cmd_complete(evt, len, opcode);
	(void)memset(rp, 0, len);
	rp->status = BT_HCI_ERR_SUCCESS;
}

static void read_local_features(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				uint16_t opcode)
{
	struct bt_hci_rp_read_local_features *rp;

	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(rp->features, 0, sizeof(rp->features));
	rp->features[4] = BIT(6);
}

static void read_supported_commands(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				    uint16_t opcode)
{
	struct bt_hci_rp_read_supported_commands *rp;

	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(rp->commands, 0xFF, sizeof(rp->commands));
}

static void read_bd_addr(struct net_buf *buf, struct net_buf **evt, uint8_t len, uint16_t opcode)
{
	struct bt_hci_rp_read_bd_addr *rp;

	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(&rp->bdaddr, 0x11, sizeof(rp->bdaddr));
}

static void read_buffer_size(struct net_buf *buf, struct net_buf **evt, uint8_t len,
			     uint16_t opcode)
{
	struct bt_hci_rp_read_buffer_size *rp;

	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	rp->acl_max_len = sys_cpu_to_le16(LE_ACL_MAX_LEN);
	rp->sco_max_len = SCO_MAX_LEN;
	rp->acl_max_num = sys_cpu_to_le16(LE_ACL_MAX_NUM);
	rp->sco_max_num = sys_cpu_to_le16(SCO_MAX_NUM);
}

static void le_read_local_features(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				   uint16_t opcode)
{
	struct bt_hci_rp_le_read_local_features *rp;

	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(rp->features, 0, sizeof(rp->features));
}

static void le_read_buffer_size(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				uint16_t opcode)
{
	struct bt_hci_rp_le_read_buffer_size *rp;

	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	rp->le_max_len = sys_cpu_to_le16(LE_ACL_MAX_LEN);
	rp->le_max_num = LE_ACL_MAX_NUM;
}

static void le_read_supp_states(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				uint16_t opcode)
{
	struct bt_hci_rp_le_read_supp_states *rp;

	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(rp->le_states, 0xFF, sizeof(rp->le_states));
}

static const struct cmd_handler cmds[] = {
	{BT_HCI_OP_READ_LOCAL_FEATURES, sizeof(struct bt_hci_rp_read_local_features),
	 read_local_features},
	{BT_HCI_OP_READ_SUPPORTED_COMMANDS, sizeof(struct bt_hci_rp_read_supported_commands),
	 read_supported_commands},
	{BT_HCI_OP_READ_BD_ADDR, sizeof(struct bt_hci_rp_read_bd_addr), read_bd_addr},
	{BT_HCI_OP_READ_BUFFER_SIZE, sizeof(struct bt_hci_rp_read_buffer_size), read_buffer_size},
	{BT_HCI_OP_LE_READ_LOCAL_FEATURES, sizeof(struct bt_hci_rp_le_read_local_features),
	 le_read_local_features},
	{BT_HCI_OP_LE_READ_BUFFER_SIZE, sizeof(struct bt_hci_rp_le_read_buffer_size),
	 le_read_buffer_size},
	{BT_HCI_OP_LE_READ_SUPP_STATES, sizeof(struct bt_hci_rp_le_read_supp_states),
	 le_read_supp_states},
};

static struct net_buf *sent_buf;
static int data_err;

static void cmd_handle(const struct device *dev, struct net_buf *cmd)
{
	struct bt_hci_cmd_hdr *hdr;
	struct net_buf *evt = NULL;
	uint16_t opcode;

	hdr = net_buf_pull_mem(cmd, sizeof(*hdr));
	opcode = sys_le16_to_cpu(hdr->opcode);

	for (size_t i = 0U; i < ARRAY_SIZE(cmds); i++) {
		if (cmds[i].opcode == opcode) {
			cmds[i].handler(cmd, &evt, cmds[i].len, opcode);
			bt_hci_recv(dev, evt);
			return;
		}
	}

	generic_success(cmd, &evt, GENERIC_RP_SIZE, opcode);
	bt_hci_recv(dev, evt);
}

static int driver_open(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int driver_send(const struct device *dev, struct net_buf *buf)
{
	uint8_t type = buf->data[0];

	if (type == BT_HCI_H4_SCO) {
		if (data_err != 0) {
			return data_err;
		}

		zassert_is_null(sent_buf, "Previous transmitted buffer was not released");
		sent_buf = buf;
		return 0;
	}

	zassert_equal(net_buf_pull_u8(buf), BT_HCI_H4_CMD);
	cmd_handle(dev, buf);
	net_buf_unref(buf);

	return 0;
}

static DEVICE_API(bt_hci, driver_api) = {
	.open = driver_open,
	.send = driver_send,
};

#define TEST_DEVICE_INIT(inst)                                                                     \
	static struct bt_hci_driver_data driver_data_##inst = {0};                                 \
	static const struct bt_hci_driver_config driver_config_##inst =                            \
		BT_DT_HCI_DRIVER_CONFIG_INST_GET(inst);                                            \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, &driver_data_##inst, &driver_config_##inst,        \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &driver_api)

DT_INST_FOREACH_STATUS_OKAY(TEST_DEVICE_INIT)

static const uint8_t test_payload[] = {0x01U, 0x02U, 0x03U, 0x04U};

static struct bt_sco_chan sco_chan;
static struct bt_conn *acl_conn;
static struct bt_conn *sco_conn;
static struct net_buf *received_buf;
static K_SEM_DEFINE(recv_sem, 0, 1);

static void sco_recv(struct bt_conn *conn, struct net_buf *buf)
{
	zassert_equal(conn, sco_conn);
	zassert_mem_equal(buf->data, test_payload, sizeof(test_payload));
	zassert_equal(buf->len, sizeof(test_payload));

	received_buf = buf;
	k_sem_give(&recv_sem);
}

static void *setup(void)
{
	static const bt_addr_t addr = {.val = {0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U}};
	int err;

	err = bt_enable(NULL);
	zassert_ok(err, "Bluetooth init failed");

	acl_conn = bt_conn_add_br(&addr);
	zassert_not_null(acl_conn);

	sco_conn = bt_conn_add_sco(&addr, BT_HCI_SCO);
	zassert_not_null(sco_conn);

	sco_conn->handle = TEST_HANDLE;
	sco_conn->state = BT_CONN_CONNECTED;
	sco_conn->sco.chan = &sco_chan;
	sco_chan.sco = sco_conn;

	return NULL;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	sco_chan.recv = NULL;
	sent_buf = NULL;
	received_buf = NULL;
	data_err = 0;
	bt_dev.br.sco_mtu = SCO_MAX_LEN;
	k_sem_reset(&recv_sem);
}

ZTEST_SUITE(sco_data, NULL, setup, before, NULL, NULL);

ZTEST(sco_data, test_buf_type_mapping)
{
	zassert_equal(bt_buf_type_to_h4(BT_BUF_SCO_IN), BT_HCI_H4_SCO);
	zassert_equal(bt_buf_type_to_h4(BT_BUF_SCO_OUT), BT_HCI_H4_SCO);
	zassert_equal(bt_buf_type_from_h4(BT_HCI_H4_SCO, BT_BUF_IN), BT_BUF_SCO_IN);
	zassert_equal(bt_buf_type_from_h4(BT_HCI_H4_SCO, BT_BUF_OUT), BT_BUF_SCO_OUT);
}

ZTEST(sco_data, test_alloc_and_send)
{
	struct bt_hci_sco_hdr *hdr;
	struct net_buf *buf;
	int err;

	buf = bt_sco_buf_alloc(K_NO_WAIT);
	zassert_not_null(buf);
	zassert_equal(net_buf_headroom(buf), BT_BUF_SCO_SIZE(0));
	zassert_is_null(bt_sco_buf_alloc(K_NO_WAIT));

	net_buf_add_mem(buf, test_payload, sizeof(test_payload));
	err = bt_sco_send(sco_conn, buf);
	zassert_ok(err);
	zassert_not_null(sent_buf);

	zassert_equal(net_buf_pull_u8(sent_buf), BT_HCI_H4_SCO);
	hdr = net_buf_pull_mem(sent_buf, sizeof(*hdr));
	zassert_equal(bt_sco_handle(sys_le16_to_cpu(hdr->handle)), TEST_HANDLE);
	zassert_equal(hdr->len, sizeof(test_payload));
	zassert_mem_equal(sent_buf->data, test_payload, sizeof(test_payload));

	net_buf_unref(sent_buf);
	sent_buf = NULL;
}

ZTEST(sco_data, test_send_validation_error_retains_buffer)
{
	struct net_buf_pool *pool;
	struct net_buf *buf;
	uint16_t available;
	int err;

	buf = bt_sco_buf_alloc(K_NO_WAIT);
	zassert_not_null(buf);

	pool = net_buf_pool_get(buf->pool_id);
	available = net_buf_get_available(pool);
	net_buf_add_mem(buf, test_payload, sizeof(test_payload));

	err = bt_sco_send(NULL, buf);
	zassert_equal(err, -EINVAL);
	zassert_equal(net_buf_get_available(pool), available);
	zassert_mem_equal(buf->data, test_payload, sizeof(test_payload));

	err = bt_sco_send(acl_conn, buf);
	zassert_equal(err, -ENOTCONN);
	zassert_equal(net_buf_get_available(pool), available);
	zassert_mem_equal(buf->data, test_payload, sizeof(test_payload));

	bt_dev.br.sco_mtu = sizeof(test_payload) - 1U;

	err = bt_sco_send(sco_conn, buf);
	zassert_equal(err, -EMSGSIZE);
	zassert_equal(net_buf_get_available(pool), available);
	zassert_is_null(sent_buf);

	net_buf_unref(buf);
	zassert_equal(net_buf_get_available(pool), available + 1U);
}

ZTEST(sco_data, test_send_transport_error_retains_buffer)
{
	struct net_buf *buf;
	int err;

	buf = bt_sco_buf_alloc(K_NO_WAIT);
	zassert_not_null(buf);
	net_buf_add_mem(buf, test_payload, sizeof(test_payload));

	data_err = -EIO;
	err = bt_sco_send(sco_conn, buf);
	zassert_equal(err, -EIO);
	zassert_is_null(sent_buf);
	zassert_is_null(bt_sco_buf_alloc(K_NO_WAIT));

	net_buf_unref(buf);
	buf = bt_sco_buf_alloc(K_NO_WAIT);
	zassert_not_null(buf);
	net_buf_unref(buf);
}

ZTEST(sco_data, test_recv_callback_set)
{
	struct bt_sco_chan *chan = sco_conn->sco.chan;
	int err;

	err = bt_sco_recv_cb_set(NULL, sco_recv);
	zassert_equal(err, -EINVAL);

	err = bt_sco_recv_cb_set(acl_conn, sco_recv);
	zassert_equal(err, -EINVAL);

	sco_conn->sco.chan = NULL;
	err = bt_sco_recv_cb_set(sco_conn, sco_recv);
	zassert_equal(err, -ENOTCONN);
	sco_conn->sco.chan = chan;

	err = bt_sco_recv_cb_set(sco_conn, sco_recv);
	zassert_ok(err);
	zassert_equal(sco_chan.recv, sco_recv);
}

ZTEST(sco_data, test_recv)
{
	const struct device *dev = DEVICE_DT_GET(DT_DRV_INST(0));
	struct bt_hci_sco_hdr *hdr;
	struct net_buf *other;
	struct net_buf *buf;
	int err;

	err = bt_sco_recv_cb_set(sco_conn, sco_recv);
	zassert_ok(err);

	buf = bt_buf_get_rx(BT_BUF_SCO_IN, K_NO_WAIT);
	zassert_not_null(buf);
	zassert_equal(buf->data[0], BT_HCI_H4_SCO);

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->handle = sys_cpu_to_le16(bt_sco_handle_pack(TEST_HANDLE, 0U));
	hdr->len = sizeof(test_payload);
	net_buf_add_mem(buf, test_payload, sizeof(test_payload));

	bt_hci_recv(dev, buf);
	err = k_sem_take(&recv_sem, K_SECONDS(1));
	zassert_ok(err, "SCO receive callback was not called");
	zassert_not_null(received_buf);
	zassert_is_null(bt_buf_get_rx(BT_BUF_SCO_IN, K_NO_WAIT));

	net_buf_unref(received_buf);
	received_buf = NULL;
	other = bt_buf_get_rx(BT_BUF_SCO_IN, K_NO_WAIT);
	zassert_not_null(other);
	net_buf_unref(other);
}

ZTEST(sco_data, test_recv_without_callback_releases_buffer)
{
	const struct device *dev = DEVICE_DT_GET(DT_DRV_INST(0));
	struct bt_hci_sco_hdr *hdr;
	struct net_buf *other;
	struct net_buf *buf;

	buf = bt_buf_get_rx(BT_BUF_SCO_IN, K_NO_WAIT);
	zassert_not_null(buf);

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->handle = sys_cpu_to_le16(bt_sco_handle_pack(TEST_HANDLE, 0U));
	hdr->len = sizeof(test_payload);
	net_buf_add_mem(buf, test_payload, sizeof(test_payload));

	bt_hci_recv(dev, buf);
	other = bt_buf_get_rx(BT_BUF_SCO_IN, K_SECONDS(1));
	zassert_not_null(other, "SCO buffer without a callback was not released");
	zassert_is_null(received_buf);
	net_buf_unref(other);
}

ZTEST(sco_data, test_recv_length_error_releases_buffer)
{
	const struct device *dev = DEVICE_DT_GET(DT_DRV_INST(0));
	struct bt_hci_sco_hdr *hdr;
	struct net_buf *other;
	struct net_buf *buf;

	buf = bt_buf_get_rx(BT_BUF_SCO_IN, K_NO_WAIT);
	zassert_not_null(buf);
	zassert_equal(buf->data[0], BT_HCI_H4_SCO);

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->handle = sys_cpu_to_le16(bt_sco_handle_pack(TEST_HANDLE, 0U));
	hdr->len = sizeof(test_payload);
	net_buf_add_mem(buf, test_payload, sizeof(test_payload) - 1U);

	bt_hci_recv(dev, buf);
	other = bt_buf_get_rx(BT_BUF_SCO_IN, K_SECONDS(1));
	zassert_not_null(other, "Malformed SCO buffer was not released");
	zassert_is_null(received_buf);
	net_buf_unref(other);
}
