/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Tarek Tarek
 * SPDX-License-Identifier: Apache-2.0
 *
 * Failing ztest reproduction for Zephyr issue #125420:
 * - Proxy GATT disconnect during bt_mesh_suspend() leaks proxy connections.
 * - bt_mesh_adv_disable() can deadlock on k_sys_work_q with CONFIG_BT_MESH_WORKQ_SYS.
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/mesh.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "adv.h"
#include "net.h"
#include "proxy.h"

#define DT_DRV_COMPAT zephyr_bt_hci_test

#define UNHANDLED_CMD_RP_SIZE 64U
#define TEST_CONN_HANDLE      0x0040U

static K_SEM_DEFINE(proxy_adv_started_sem, 0, 64);
static K_SEM_DEFINE(proxy_conn_sem, 0, 1);
static K_SEM_DEFINE(suspend_done_sem, 0, 1);

static const struct device *hci_dev;

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

static void *cmd_complete(struct net_buf **buf, uint8_t plen, uint16_t opcode)
{
	struct bt_hci_evt_cmd_complete *cc;

	*buf = bt_buf_get_evt(BT_HCI_EVT_CMD_COMPLETE, false, K_FOREVER);
	evt_create(*buf, BT_HCI_EVT_CMD_COMPLETE, sizeof(*cc) + plen);
	cc = net_buf_add(*buf, sizeof(*cc));
	cc->ncmd = 1U;
	cc->opcode = sys_cpu_to_le16(opcode);

	return net_buf_add(*buf, plen);
}

static void generic_success(struct net_buf *buf, struct net_buf **evt, uint8_t len, uint16_t opcode)
{
	struct bt_hci_evt_cc_status *ccst;

	ARG_UNUSED(buf);

	ccst = cmd_complete(evt, len, opcode);
	(void)memset(ccst, 0, len);
	ccst->status = BT_HCI_ERR_SUCCESS;
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
	rp->features[4] = BIT(5) | BIT(6);
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

static void le_read_local_features(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				   uint16_t opcode)
{
	struct bt_hci_rp_le_read_local_features *rp;

	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(rp->features, 0, sizeof(rp->features));
	rp->features[BT_LE_FEAT_BIT_EXT_ADV / 8] = BIT(BT_LE_FEAT_BIT_EXT_ADV % 8);
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

static void le_read_max_adv_data_len(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				     uint16_t opcode)
{
	struct bt_hci_rp_le_read_max_adv_data_len *rp;

	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	rp->max_adv_data_len = sys_cpu_to_le16(BT_GAP_ADV_MAX_EXT_ADV_DATA_LEN);
}

static void le_read_buffer_size(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				uint16_t opcode)
{
	struct bt_hci_rp_le_read_buffer_size *rp;

	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	rp->le_max_len = sys_cpu_to_le16(251);
	rp->le_max_num = 10;
}

static void le_set_ext_adv_param(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				 uint16_t opcode)
{
	struct bt_hci_rp_le_set_ext_adv_param *rp;

	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	rp->tx_power = 0;
}

static void le_set_ext_adv_enable(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				  uint16_t opcode)
{
	const struct bt_hci_cp_le_set_ext_adv_enable *cp = (void *)buf->data;

	ARG_UNUSED(len);

	if (buf->len >= sizeof(*cp) && cp->enable) {
		k_sem_give(&proxy_adv_started_sem);
	}

	generic_success(buf, evt, sizeof(struct bt_hci_evt_cc_status), opcode);
}

static const struct cmd_handler cmds[] = {
	{ BT_HCI_OP_READ_LOCAL_FEATURES, sizeof(struct bt_hci_rp_read_local_features),
	  read_local_features },
	{ BT_HCI_OP_READ_SUPPORTED_COMMANDS, sizeof(struct bt_hci_rp_read_supported_commands),
	  read_supported_commands },
	{ BT_HCI_OP_LE_READ_LOCAL_FEATURES, sizeof(struct bt_hci_rp_le_read_local_features),
	  le_read_local_features },
	{ BT_HCI_OP_LE_READ_SUPP_STATES, sizeof(struct bt_hci_rp_le_read_supp_states),
	  le_read_supp_states },
	{ BT_HCI_OP_LE_READ_MAX_ADV_DATA_LEN, sizeof(struct bt_hci_rp_le_read_max_adv_data_len),
	  le_read_max_adv_data_len },
	{ BT_HCI_OP_LE_READ_BUFFER_SIZE, sizeof(struct bt_hci_rp_le_read_buffer_size),
	  le_read_buffer_size },
	{ BT_HCI_OP_LE_SET_EXT_ADV_PARAM, sizeof(struct bt_hci_rp_le_set_ext_adv_param),
	  le_set_ext_adv_param },
	{ BT_HCI_OP_LE_SET_EXT_ADV_ENABLE, sizeof(struct bt_hci_evt_cc_status),
	  le_set_ext_adv_enable },
};

static void cmd_handle(const struct device *dev, struct net_buf *cmd)
{
	struct net_buf *evt = NULL;
	struct bt_hci_cmd_hdr *chdr;
	uint16_t opcode;

	chdr = net_buf_pull_mem(cmd, sizeof(*chdr));
	opcode = sys_le16_to_cpu(chdr->opcode);

	switch (opcode) {
	case BT_HCI_OP_HOST_NUM_COMPLETED_PACKETS:
		return;
	default:
		break;
	}

	for (size_t i = 0U; i < ARRAY_SIZE(cmds); i++) {
		if (cmds[i].opcode == opcode) {
			cmds[i].handler(cmd, &evt, cmds[i].len, opcode);
			bt_hci_recv(dev, evt);

			return;
		}
	}

	generic_success(cmd, &evt, UNHANDLED_CMD_RP_SIZE, opcode);
	bt_hci_recv(dev, evt);
}

static int driver_open(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int driver_send(const struct device *dev, struct net_buf *buf)
{
	uint8_t type = net_buf_pull_u8(buf);

	if (type == BT_HCI_H4_CMD) {
		cmd_handle(dev, buf);
	}

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

static void evt_send(uint8_t evt, const void *params, uint8_t len)
{
	struct net_buf *buf;

	buf = bt_buf_get_evt(evt, false, K_FOREVER);
	evt_create(buf, evt, len);
	net_buf_add_mem(buf, params, len);
	bt_hci_recv(hci_dev, buf);
}

static void inject_le_conn_complete(void)
{
	const bt_addr_le_t peer = {
		.type = BT_ADDR_LE_RANDOM,
		.a.val = { 0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0xc6U },
	};
	struct {
		struct bt_hci_evt_le_meta_event meta;
		struct bt_hci_evt_le_conn_complete cc;
	} __packed evt;

	(void)memset(&evt, 0, sizeof(evt));
	evt.meta.subevent = BT_HCI_EVT_LE_CONN_COMPLETE;
	evt.cc.status = BT_HCI_ERR_SUCCESS;
	evt.cc.handle = sys_cpu_to_le16(TEST_CONN_HANDLE);
	evt.cc.role = BT_HCI_ROLE_PERIPHERAL;
	bt_addr_le_copy(&evt.cc.peer_addr, &peer);
	evt.cc.interval = sys_cpu_to_le16(BT_GAP_INIT_CONN_INT_MIN);
	evt.cc.supv_timeout = sys_cpu_to_le16(BT_GAP_MS_TO_CONN_TIMEOUT(4000U));

	evt_send(BT_HCI_EVT_LE_META_EVENT, &evt, sizeof(evt));
}

static void inject_disconn_complete(void)
{
	const struct bt_hci_evt_disconn_complete evt = {
		.status = BT_HCI_ERR_SUCCESS,
		.handle = sys_cpu_to_le16(TEST_CONN_HANDLE),
		.reason = BT_HCI_ERR_REMOTE_USER_TERM_CONN,
	};

	evt_send(BT_HCI_EVT_DISCONN_COMPLETE, &evt, sizeof(evt));
}

static void proxy_conn_cb(struct bt_conn *conn, uint8_t err)
{
	ARG_UNUSED(conn);

	if (err == 0U && bt_mesh_proxy_srv_connected_cnt() > 0U) {
		k_sem_give(&proxy_conn_sem);
	}
}

BT_CONN_CB_DEFINE(proxy_conn_cbs) = {
	.connected = proxy_conn_cb,
};

static const uint8_t net_key[16] = { 0xcc };
static const uint8_t dev_key[16] = { 0x01 };
static uint8_t prov_uuid[16] = { 0x6c, 0x69, 0x6e, 0x67, 0x61, 0xaa };

static const struct bt_mesh_prov prov = {
	.uuid = prov_uuid,
};

static const struct bt_mesh_model dut_models[] = {
	BT_MESH_MODEL_CFG_SRV,
};

static struct bt_mesh_elem dut_elems[] = {
	BT_MESH_ELEM(0, dut_models, BT_MESH_MODEL_NONE),
};

static const struct bt_mesh_comp comp = {
	.cid = BT_COMP_ID_LF,
	.elem = dut_elems,
	.elem_count = ARRAY_SIZE(dut_elems),
};

static int mesh_provision_ready(void)
{
	while (k_sem_take(&proxy_adv_started_sem, K_NO_WAIT) == 0) {
		/* Drain stale signals. */
	}

	zassert_ok(bt_mesh_provision(net_key, 0, 0, 0, 0x0001, dev_key),
		   "bt_mesh_provision failed");

	return k_sem_take(&proxy_adv_started_sem, K_SECONDS(10));
}

static void mesh_suspend_work_fn(struct k_work *work)
{
	ARG_UNUSED(work);

	(void)bt_mesh_proxy_identity_enable();
	(void)bt_mesh_suspend();
	k_sem_give(&suspend_done_sem);
}

static K_WORK_DEFINE(mesh_suspend_work, mesh_suspend_work_fn);

static void *suite_setup(void)
{
	hci_dev = DEVICE_DT_GET(DT_DRV_INST(0));

	zassert_ok(bt_enable(NULL), "bt_enable failed");
	zassert_ok(bt_mesh_init(&prov, &comp), "bt_mesh_init failed");

	return NULL;
}

static void test01_before(void *fixture)
{
	ARG_UNUSED(fixture);

	zassert_ok(mesh_provision_ready(), "Proxy advertising did not start");
}

static void test02_before(void *fixture)
{
	ARG_UNUSED(fixture);

	bt_mesh_reset();
	zassert_ok(bt_mesh_init(&prov, &comp), "bt_mesh_init failed after reset");
	zassert_ok(mesh_provision_ready(), "Proxy advertising did not start (test 2)");
	k_sem_reset(&suspend_done_sem);
}

ZTEST_SUITE(mesh_suspend_deadlock, NULL, suite_setup, NULL, NULL, NULL);

ZTEST(mesh_suspend_deadlock, test_01_proxy_disconnect_during_suspend)
{
	test01_before(NULL);

	inject_le_conn_complete();

	zassert_ok(k_sem_take(&proxy_conn_sem, K_SECONDS(2)), "Proxy connection not established");
	zassert_equal(bt_mesh_proxy_srv_connected_cnt(), 1U, "Expected one proxy client");

	zassert_ok(bt_mesh_suspend(), "bt_mesh_suspend failed");

	inject_disconn_complete();
	k_sleep(K_MSEC(50));

	zexpect_equal(bt_mesh_proxy_srv_connected_cnt(), 0U,
		      "Proxy connection count should be zero after disconnect during suspend "
		      "(Zephyr #125420: gatt_disconnected skips cleanup when "
		      "!service_registered && provisioned)");
	zassert_ok(bt_mesh_resume(),
		   "bt_mesh_resume failed (expected after proxy leak re-registers service)");
}

ZTEST(mesh_suspend_deadlock, test_02_adv_disable_deadlock_on_sys_workq)
{
	test02_before(NULL);

	k_work_submit_to_queue(&k_sys_work_q, &mesh_suspend_work);

	zassert_ok(k_sem_take(&suspend_done_sem, K_SECONDS(3)),
		   "bt_mesh_suspend did not complete (likely k_sys_work_q deadlock in "
		   "bt_mesh_adv_disable() when proxy identity work is queued on sys WQ)");
}
