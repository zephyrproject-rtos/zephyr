/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/bluetooth.h>

#include <assert.h>
#include <errno.h>
#include <zephyr/bluetooth/mesh.h>
#include <zephyr/bluetooth/testing.h>
#include <zephyr/bluetooth/mesh/cfg.h>
#include <zephyr/sys/byteorder.h>
#include <app_keys.h>

#include <zephyr/logging/log.h>
#define LOG_MODULE_NAME bttester_mesh
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include "bttester.h"

#define CONTROLLER_INDEX 0
#define CID_LOCAL 0x05F1

/* Health server data */
#define CUR_FAULTS_MAX 4
#define HEALTH_TEST_ID 0x00

static uint8_t cur_faults[CUR_FAULTS_MAX];
static uint8_t reg_faults[CUR_FAULTS_MAX * 2];

/* Provision node data */
static uint8_t net_key[16];
static uint16_t net_key_idx;
static uint8_t flags;
static uint32_t iv_index;
static uint16_t addr;
static uint8_t dev_key[16];
static uint8_t input_size;
static uint8_t pub_key[64];
static uint8_t priv_key[32];

/* Configured provisioning data */
static uint8_t dev_uuid[16];
static uint8_t static_auth[16];

/* Vendor Model data */
#define VND_MODEL_ID_1 0x1234
static uint8_t vnd_app_key[16];
static uint16_t vnd_app_key_idx = 0x000f;

/* Model send data */
#define MODEL_BOUNDS_MAX 2

/* Model Authentication Method */
#define AUTH_METHOD_STATIC 0x01
#define AUTH_METHOD_OUTPUT 0x02
#define AUTH_METHOD_INPUT 0x03

static struct model_data {
	struct bt_mesh_model *model;
	uint16_t addr;
	uint16_t appkey_idx;
} model_bound[MODEL_BOUNDS_MAX];

static struct {
	uint16_t local;
	uint16_t dst;
	uint16_t net_idx;
} net = {
	.local = BT_MESH_ADDR_UNASSIGNED,
	.dst = BT_MESH_ADDR_UNASSIGNED,
};

static void supported_commands(uint8_t *data, uint16_t len)
{
	struct net_buf_simple *buf = NET_BUF_SIMPLE(BTP_DATA_MAX_SIZE);

	net_buf_simple_init(buf, 0);
	net_buf_simple_add_u8(buf, MESH_READ_SUPPORTED_COMMANDS);
	net_buf_simple_add_u8(buf, MESH_CONFIG_PROVISIONING);
	net_buf_simple_add_u8(buf, MESH_PROVISION_NODE);
	net_buf_simple_add_u8(buf, MESH_INIT);
	net_buf_simple_add_u8(buf, MESH_RESET);
	net_buf_simple_add_u8(buf, MESH_INPUT_NUMBER);
	net_buf_simple_add_u8(buf, MESH_INPUT_STRING);
	net_buf_simple_add_u8(buf, MESH_IVU_TEST_MODE);
	net_buf_simple_add_u8(buf, MESH_IVU_TOGGLE_STATE);
	net_buf_simple_add_u8(buf, MESH_NET_SEND);
	net_buf_simple_add_u8(buf, MESH_HEALTH_GENERATE_FAULTS);
	net_buf_simple_add_u8(buf, MESH_HEALTH_CLEAR_FAULTS);
	net_buf_simple_add_u8(buf, MESH_LPN);
	net_buf_simple_add_u8(buf, MESH_LPN_POLL);
	net_buf_simple_add_u8(buf, MESH_MODEL_SEND);
#if defined(CONFIG_BT_TESTING)
	net_buf_simple_add_u8(buf, MESH_LPN_SUBSCRIBE);
	net_buf_simple_add_u8(buf, MESH_LPN_UNSUBSCRIBE);
	net_buf_simple_add_u8(buf, MESH_RPL_CLEAR);
#endif /* CONFIG_BT_TESTING */
	net_buf_simple_add_u8(buf, MESH_PROXY_IDENTITY);
	net_buf_simple_add_u8(buf, MESH_COMP_DATA_GET);
	net_buf_simple_add_u8(buf, MESH_CFG_BEACON_GET);
	net_buf_simple_add_u8(buf, MESH_CFG_BEACON_SET);
	net_buf_simple_add_u8(buf, MESH_CFG_DEFAULT_TTL_GET);
	net_buf_simple_add_u8(buf, MESH_CFG_DEFAULT_TTL_SET);
	net_buf_simple_add_u8(buf, MESH_CFG_GATT_PROXY_GET);
	net_buf_simple_add_u8(buf, MESH_CFG_GATT_PROXY_SET);
	net_buf_simple_add_u8(buf, MESH_CFG_FRIEND_GET);
	net_buf_simple_add_u8(buf, MESH_CFG_FRIEND_SET);
	net_buf_simple_add_u8(buf, MESH_CFG_RELAY_GET);
	net_buf_simple_add_u8(buf, MESH_CFG_RELAY_SET);
	net_buf_simple_add_u8(buf, MESH_CFG_MODEL_PUB_GET);
	net_buf_simple_add_u8(buf, MESH_CFG_MODEL_PUB_SET);
	net_buf_simple_add_u8(buf, MESH_CFG_MODEL_SUB_ADD);
	net_buf_simple_add_u8(buf, MESH_CFG_MODEL_SUB_DEL);
	net_buf_simple_add_u8(buf, MESH_CFG_NETKEY_ADD);
	net_buf_simple_add_u8(buf, MESH_CFG_NETKEY_GET);
	net_buf_simple_add_u8(buf, MESH_CFG_NETKEY_DEL);
	net_buf_simple_add_u8(buf, MESH_CFG_APPKEY_ADD);
	net_buf_simple_add_u8(buf, MESH_CFG_APPKEY_DEL);
	net_buf_simple_add_u8(buf, MESH_CFG_APPKEY_GET);
	net_buf_simple_add_u8(buf, MESH_CFG_MODEL_APP_BIND);
	net_buf_simple_add_u8(buf, MESH_CFG_MODEL_APP_UNBIND);
	net_buf_simple_add_u8(buf, MESH_CFG_MODEL_APP_GET);
	net_buf_simple_add_u8(buf, MESH_CFG_MODEL_APP_VND_GET);
	net_buf_simple_add_u8(buf, MESH_CFG_HEARTBEAT_PUB_SET);
	net_buf_simple_add_u8(buf, MESH_CFG_HEARTBEAT_PUB_GET);
	net_buf_simple_add_u8(buf, MESH_CFG_HEARTBEAT_SUB_SET);
	net_buf_simple_add_u8(buf, MESH_CFG_HEARTBEAT_SUB_GET);
	net_buf_simple_add_u8(buf, MESH_CFG_NET_TRANS_GET);
	net_buf_simple_add_u8(buf, MESH_CFG_NET_TRANS_SET);
	net_buf_simple_add_u8(buf, MESH_CFG_MODEL_SUB_OVW);
	net_buf_simple_add_u8(buf, MESH_CFG_MODEL_SUB_DEL_ALL);
	net_buf_simple_add_u8(buf, MESH_CFG_MODEL_SUB_GET);
	net_buf_simple_add_u8(buf, MESH_CFG_MODEL_SUB_GET_VND);
	net_buf_simple_add_u8(buf, MESH_CFG_MODEL_SUB_VA_ADD);
	net_buf_simple_add_u8(buf, MESH_CFG_MODEL_SUB_VA_DEL);
	net_buf_simple_add_u8(buf, MESH_CFG_MODEL_SUB_VA_OVW);
	net_buf_simple_add_u8(buf, MESH_CFG_NETKEY_UPDATE);
	net_buf_simple_add_u8(buf, MESH_CFG_APPKEY_UPDATE);
	net_buf_simple_add_u8(buf, MESH_CFG_NODE_IDT_SET);
	net_buf_simple_add_u8(buf, MESH_CFG_NODE_IDT_GET);
	net_buf_simple_add_u8(buf, MESH_CFG_NODE_RESET);
	net_buf_simple_add_u8(buf, MESH_CFG_LPN_TIMEOUT_GET);
	net_buf_simple_add_u8(buf, MESH_CFG_MODEL_APP_BIND_VND);
	net_buf_simple_add_u8(buf, MESH_HEALTH_FAULT_GET);
	net_buf_simple_add_u8(buf, MESH_HEALTH_FAULT_CLEAR);
	net_buf_simple_add_u8(buf, MESH_HEALTH_PERIOD_GET);
	net_buf_simple_add_u8(buf, MESH_HEALTH_PERIOD_SET);
	net_buf_simple_add_u8(buf, MESH_HEALTH_ATTENTION_GET);
	net_buf_simple_add_u8(buf, MESH_HEALTH_ATTENTION_SET);
	net_buf_simple_add_u8(buf, MESH_PROVISION_ADV);
	net_buf_simple_add_u8(buf, MESH_CFG_KRP_GET);
	net_buf_simple_add_u8(buf, MESH_CFG_KRP_SET);

	tester_send(BTP_SERVICE_ID_MESH, MESH_READ_SUPPORTED_COMMANDS,
		    CONTROLLER_INDEX, buf->data, buf->len);
}

static void get_faults(uint8_t *faults, uint8_t faults_size, uint8_t *dst, uint8_t *count)
{
	uint8_t i, limit = *count;

	for (i = 0U, *count = 0U; i < faults_size && *count < limit; i++) {
		if (faults[i]) {
			*dst++ = faults[i];
			(*count)++;
		}
	}
}

static int fault_get_cur(struct bt_mesh_model *model, uint8_t *test_id,
			 uint16_t *company_id, uint8_t *faults, uint8_t *fault_count)
{
	LOG_DBG("");

	*test_id = HEALTH_TEST_ID;
	*company_id = CID_LOCAL;

	get_faults(cur_faults, sizeof(cur_faults), faults, fault_count);

	return 0;
}

static int fault_get_reg(struct bt_mesh_model *model, uint16_t company_id,
			 uint8_t *test_id, uint8_t *faults, uint8_t *fault_count)
{
	LOG_DBG("company_id 0x%04x", company_id);

	if (company_id != CID_LOCAL) {
		return -EINVAL;
	}

	*test_id = HEALTH_TEST_ID;

	get_faults(reg_faults, sizeof(reg_faults), faults, fault_count);

	return 0;
}

static int fault_clear(struct bt_mesh_model *model, uint16_t company_id)
{
	LOG_DBG("company_id 0x%04x", company_id);

	if (company_id != CID_LOCAL) {
		return -EINVAL;
	}

	(void)memset(reg_faults, 0, sizeof(reg_faults));

	return 0;
}

static int fault_test(struct bt_mesh_model *model, uint8_t test_id,
		      uint16_t company_id)
{
	LOG_DBG("test_id 0x%02x company_id 0x%04x", test_id, company_id);

	if (company_id != CID_LOCAL || test_id != HEALTH_TEST_ID) {
		return -EINVAL;
	}

	return 0;
}

static const struct bt_mesh_health_srv_cb health_srv_cb = {
	.fault_get_cur = fault_get_cur,
	.fault_get_reg = fault_get_reg,
	.fault_clear = fault_clear,
	.fault_test = fault_test,
};

static struct bt_mesh_health_srv health_srv = {
	.cb = &health_srv_cb,
};

BT_MESH_HEALTH_PUB_DEFINE(health_pub, CUR_FAULTS_MAX);

static struct bt_mesh_cfg_cli cfg_cli = {
};

static void show_faults(uint8_t test_id, uint16_t cid, uint8_t *faults, size_t fault_count)
{
	size_t i;

	if (!fault_count) {
		LOG_DBG("Health Test ID 0x%02x Company ID 0x%04x: no faults",
			test_id, cid);
		return;
	}

	LOG_DBG("Health Test ID 0x%02x Company ID 0x%04x Fault Count %zu: ",
		test_id, cid, fault_count);

	for (i = 0; i < fault_count; i++) {
		LOG_DBG("0x%02x", faults[i]);
	}
}

static void health_current_status(struct bt_mesh_health_cli *cli, uint16_t addr,
				  uint8_t test_id, uint16_t cid, uint8_t *faults,
				  size_t fault_count)
{
	LOG_DBG("Health Current Status from 0x%04x", addr);
	show_faults(test_id, cid, faults, fault_count);
}

static struct bt_mesh_health_cli health_cli = {
	.current_status = health_current_status,
};

static struct bt_mesh_model root_models[] = {
	BT_MESH_MODEL_CFG_SRV,
	BT_MESH_MODEL_CFG_CLI(&cfg_cli),
	BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub),
	BT_MESH_MODEL_HEALTH_CLI(&health_cli),
};

static struct bt_mesh_model vnd_models[] = {
	BT_MESH_MODEL_VND(CID_LOCAL, VND_MODEL_ID_1, BT_MESH_MODEL_NO_OPS, NULL,
			  NULL),
};

static struct bt_mesh_elem elements[] = {
	BT_MESH_ELEM(0, root_models, vnd_models),
};

static void link_open(bt_mesh_prov_bearer_t bearer)
{
	struct mesh_prov_link_open_ev ev;

	LOG_DBG("bearer 0x%02x", bearer);

	switch (bearer) {
	case BT_MESH_PROV_ADV:
		ev.bearer = MESH_PROV_BEARER_PB_ADV;
		break;
	case BT_MESH_PROV_GATT:
		ev.bearer = MESH_PROV_BEARER_PB_GATT;
		break;
	default:
		LOG_ERR("Invalid bearer");

		return;
	}

	tester_send(BTP_SERVICE_ID_MESH, MESH_EV_PROV_LINK_OPEN,
		    CONTROLLER_INDEX, (uint8_t *) &ev, sizeof(ev));
}

static void link_close(bt_mesh_prov_bearer_t bearer)
{
	struct mesh_prov_link_closed_ev ev;

	LOG_DBG("bearer 0x%02x", bearer);

	switch (bearer) {
	case BT_MESH_PROV_ADV:
		ev.bearer = MESH_PROV_BEARER_PB_ADV;
		break;
	case BT_MESH_PROV_GATT:
		ev.bearer = MESH_PROV_BEARER_PB_GATT;
		break;
	default:
		LOG_ERR("Invalid bearer");

		return;
	}

	tester_send(BTP_SERVICE_ID_MESH, MESH_EV_PROV_LINK_CLOSED,
		    CONTROLLER_INDEX, (uint8_t *) &ev, sizeof(ev));
}

static int output_number(bt_mesh_output_action_t action, uint32_t number)
{
	struct mesh_out_number_action_ev ev;

	LOG_DBG("action 0x%04x number 0x%08x", action, number);

	ev.action = sys_cpu_to_le16(action);
	ev.number = sys_cpu_to_le32(number);

	tester_send(BTP_SERVICE_ID_MESH, MESH_EV_OUT_NUMBER_ACTION,
		    CONTROLLER_INDEX, (uint8_t *) &ev, sizeof(ev));

	return 0;
}

static int output_string(const char *str)
{
	struct mesh_out_string_action_ev *ev;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(BTP_DATA_MAX_SIZE);

	LOG_DBG("str %s", str);

	net_buf_simple_init(buf, 0);

	ev = net_buf_simple_add(buf, sizeof(*ev));
	ev->string_len = strlen(str);

	net_buf_simple_add_mem(buf, str, ev->string_len);

	tester_send(BTP_SERVICE_ID_MESH, MESH_EV_OUT_STRING_ACTION,
		    CONTROLLER_INDEX, buf->data, buf->len);

	return 0;
}

static int input(bt_mesh_input_action_t action, uint8_t size)
{
	struct mesh_in_action_ev ev;

	LOG_DBG("action 0x%04x number 0x%02x", action, size);

	input_size = size;

	ev.action = sys_cpu_to_le16(action);
	ev.size = size;

	tester_send(BTP_SERVICE_ID_MESH, MESH_EV_IN_ACTION, CONTROLLER_INDEX,
		    (uint8_t *) &ev, sizeof(ev));

	return 0;
}

static void prov_complete(uint16_t net_idx, uint16_t addr)
{
	LOG_DBG("net_idx 0x%04x addr 0x%04x", net_idx, addr);

	net.net_idx = net_idx,
	net.local = addr;
	net.dst = addr;

	tester_send(BTP_SERVICE_ID_MESH, MESH_EV_PROVISIONED, CONTROLLER_INDEX,
		    NULL, 0);
}

static void prov_node_added(uint16_t net_idx, uint8_t uuid[16], uint16_t addr,
			    uint8_t num_elem)
{
	struct mesh_prov_node_added_ev ev;

	LOG_DBG("net_idx 0x%04x addr 0x%04x num_elem %d", net_idx, addr,
		num_elem);

	ev.net_idx = net_idx;
	ev.addr = addr;
	ev.num_elems = num_elem;
	memcpy(&ev.uuid, uuid, sizeof(ev.uuid));

	tester_send(BTP_SERVICE_ID_MESH, MESH_EV_PROV_NODE_ADDED,
		    CONTROLLER_INDEX, (void *)&ev, sizeof(ev));
}

static void prov_reset(void)
{
	LOG_DBG("");

	bt_mesh_prov_enable(BT_MESH_PROV_ADV | BT_MESH_PROV_GATT);
}

static const struct bt_mesh_comp comp = {
	.cid = CID_LOCAL,
	.elem = elements,
	.elem_count = ARRAY_SIZE(elements),
};

static struct bt_mesh_prov prov = {
	.uuid = dev_uuid,
	.static_val = static_auth,
	.static_val_len = sizeof(static_auth),
	.output_number = output_number,
	.output_string = output_string,
	.input = input,
	.link_open = link_open,
	.link_close = link_close,
	.complete = prov_complete,
	.node_added = prov_node_added,
	.reset = prov_reset,
};

static void config_prov(uint8_t *data, uint16_t len)
{
	const struct mesh_config_provisioning_cmd *cmd = (void *) data;
	int err = 0;

	LOG_DBG("");

	memcpy(dev_uuid, cmd->uuid, sizeof(dev_uuid));
	memcpy(static_auth, cmd->static_auth, sizeof(static_auth));

	prov.output_size = cmd->out_size;
	prov.output_actions = sys_le16_to_cpu(cmd->out_actions);
	prov.input_size = cmd->in_size;
	prov.input_actions = sys_le16_to_cpu(cmd->in_actions);

	if (cmd->auth_method == AUTH_METHOD_OUTPUT) {
		err = bt_mesh_auth_method_set_output(prov.output_actions, prov.output_size);
	} else if (cmd->auth_method == AUTH_METHOD_INPUT) {
		err = bt_mesh_auth_method_set_input(prov.input_actions, prov.input_size);
	} else if (cmd->auth_method == AUTH_METHOD_STATIC) {
		err = bt_mesh_auth_method_set_static(static_auth, sizeof(static_auth));
	}

	if (len > sizeof(*cmd)) {
		memcpy(pub_key, cmd->set_keys->pub_key, sizeof(cmd->set_keys->pub_key));
		memcpy(priv_key, cmd->set_keys->priv_key, sizeof(cmd->set_keys->priv_key));
		prov.public_key_be = pub_key;
		prov.private_key_be = priv_key;
	}

	if (err) {
		LOG_ERR("err %d", err);
	}

	tester_rsp(BTP_SERVICE_ID_MESH, MESH_CONFIG_PROVISIONING, CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void provision_node(uint8_t *data, uint16_t len)
{
	const struct mesh_provision_node_cmd *cmd = (void *)data;
	int err;

	LOG_DBG("");

	memcpy(dev_key, cmd->dev_key, sizeof(dev_key));
	memcpy(net_key, cmd->net_key, sizeof(net_key));

	addr = sys_le16_to_cpu(cmd->addr);
	flags = cmd->flags;
	iv_index = sys_le32_to_cpu(cmd->iv_index);
	net_key_idx = sys_le16_to_cpu(cmd->net_key_idx);

	if (len > sizeof(*cmd)) {
		memcpy(pub_key, cmd->pub_key, sizeof(pub_key));

		err = bt_mesh_prov_remote_pub_key_set(pub_key);
		if (err) {
			LOG_ERR("err %d", err);
			goto fail;
		}
	}
#if defined(CONFIG_BT_MESH_PROVISIONER)
	err = bt_mesh_cdb_create(net_key);
	if (err) {
		LOG_ERR("err %d", err);
		goto fail;
	}
#endif
	err = bt_mesh_provision(net_key, net_key_idx, flags, iv_index, addr,
				dev_key);
	if (err) {
		LOG_ERR("err %d", err);
		goto fail;
	}

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_PROVISION_NODE, CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void provision_adv(uint8_t *data, uint16_t len)
{
	const struct mesh_provision_adv_cmd *cmd = (void *)data;
	int err;

	LOG_DBG("");

	err = bt_mesh_provision_adv(cmd->uuid, cmd->net_idx, cmd->address,
				    cmd->attention_duration);
	if (err) {
		LOG_ERR("err %d", err);
		goto fail;
	}

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_PROVISION_ADV, CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void init(uint8_t *data, uint16_t len)
{
	uint8_t status = BTP_STATUS_SUCCESS;
	int err;

	LOG_DBG("");

	err = bt_mesh_init(&prov, &comp);
	if (err) {
		status = BTP_STATUS_FAILED;

		goto rsp;
	}

	if (addr) {
		err = bt_mesh_provision(net_key, net_key_idx, flags, iv_index,
					addr, dev_key);
		if (err) {
			status = BTP_STATUS_FAILED;
		}
	} else {
		err = bt_mesh_prov_enable(BT_MESH_PROV_ADV | BT_MESH_PROV_GATT);
		if (err) {
			status = BTP_STATUS_FAILED;
		}
	}

rsp:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_INIT, CONTROLLER_INDEX,
		   status);
}

static void reset(uint8_t *data, uint16_t len)
{
	LOG_DBG("");

	bt_mesh_reset();

	tester_rsp(BTP_SERVICE_ID_MESH, MESH_RESET, CONTROLLER_INDEX,
		   BTP_STATUS_SUCCESS);
}

static void input_number(uint8_t *data, uint16_t len)
{
	const struct mesh_input_number_cmd *cmd = (void *) data;
	uint8_t status = BTP_STATUS_SUCCESS;
	uint32_t number;
	int err;

	number = sys_le32_to_cpu(cmd->number);

	LOG_DBG("number 0x%04x", number);

	err = bt_mesh_input_number(number);
	if (err) {
		status = BTP_STATUS_FAILED;
	}

	tester_rsp(BTP_SERVICE_ID_MESH, MESH_INPUT_NUMBER, CONTROLLER_INDEX,
		   status);
}

static void input_string(uint8_t *data, uint16_t len)
{
	const struct mesh_input_string_cmd *cmd = (void *) data;
	uint8_t status = BTP_STATUS_SUCCESS;
	uint8_t str_auth[16];
	int err;

	LOG_DBG("");

	if (cmd->string_len > sizeof(str_auth)) {
		LOG_ERR("Too long input (%u chars required)", input_size);
		status = BTP_STATUS_FAILED;
		goto rsp;
	} else if (cmd->string_len < input_size) {
		LOG_ERR("Too short input (%u chars required)", input_size);
		status = BTP_STATUS_FAILED;
		goto rsp;
	}

	strncpy(str_auth, cmd->string, cmd->string_len);

	err = bt_mesh_input_string(str_auth);
	if (err) {
		status = BTP_STATUS_FAILED;
	}

rsp:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_INPUT_STRING, CONTROLLER_INDEX,
		   status);
}

static void ivu_test_mode(uint8_t *data, uint16_t len)
{
	const struct mesh_ivu_test_mode_cmd *cmd = (void *) data;

	LOG_DBG("enable 0x%02x", cmd->enable);

	bt_mesh_iv_update_test(cmd->enable ? true : false);

	tester_rsp(BTP_SERVICE_ID_MESH, MESH_IVU_TEST_MODE, CONTROLLER_INDEX,
		   BTP_STATUS_SUCCESS);
}

static void ivu_toggle_state(uint8_t *data, uint16_t len)
{
	bool result;

	LOG_DBG("");

	result = bt_mesh_iv_update();
	if (!result) {
		LOG_ERR("Failed to toggle the IV Update state");
	}

	tester_rsp(BTP_SERVICE_ID_MESH, MESH_IVU_TOGGLE_STATE, CONTROLLER_INDEX,
		   result ? BTP_STATUS_SUCCESS : BTP_STATUS_FAILED);
}

static void lpn(uint8_t *data, uint16_t len)
{
	struct mesh_lpn_set_cmd *cmd = (void *) data;
	bool enable;
	int err;

	LOG_DBG("enable 0x%02x", cmd->enable);

	enable = cmd->enable ? true : false;
	err = bt_mesh_lpn_set(enable);
	if (err) {
		LOG_ERR("Failed to toggle LPN (err %d)", err);
	}

	tester_rsp(BTP_SERVICE_ID_MESH, MESH_LPN, CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void lpn_poll(uint8_t *data, uint16_t len)
{
	int err;

	LOG_DBG("");

	err = bt_mesh_lpn_poll();
	if (err) {
		LOG_ERR("Failed to send poll msg (err %d)", err);
	}

	tester_rsp(BTP_SERVICE_ID_MESH, MESH_LPN_POLL, CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void net_send(uint8_t *data, uint16_t len)
{
	struct mesh_net_send_cmd *cmd = (void *) data;
	NET_BUF_SIMPLE_DEFINE(msg, UINT8_MAX);
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = vnd_app_key_idx,
		.addr = sys_le16_to_cpu(cmd->dst),
		.send_ttl = cmd->ttl,
	};
	int err;

	LOG_DBG("ttl 0x%02x dst 0x%04x payload_len %d", ctx.send_ttl,
		ctx.addr, cmd->payload_len);

	if (!bt_mesh_app_key_exists(vnd_app_key_idx)) {
		(void)bt_mesh_app_key_add(vnd_app_key_idx, net.net_idx,
					  vnd_app_key);
		vnd_models[0].keys[0] = vnd_app_key_idx;
	}

	net_buf_simple_add_mem(&msg, cmd->payload, cmd->payload_len);

	err = bt_mesh_model_send(&vnd_models[0], &ctx, &msg, NULL, NULL);
	if (err) {
		LOG_ERR("Failed to send (err %d)", err);
	}

	tester_rsp(BTP_SERVICE_ID_MESH, MESH_NET_SEND, CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void health_generate_faults(uint8_t *data, uint16_t len)
{
	struct mesh_health_generate_faults_rp *rp;
	NET_BUF_SIMPLE_DEFINE(buf, sizeof(*rp) + sizeof(cur_faults) +
			      sizeof(reg_faults));
	uint8_t some_faults[] = { 0x01, 0x02, 0x03, 0xff, 0x06 };
	uint8_t cur_faults_count, reg_faults_count;

	rp = net_buf_simple_add(&buf, sizeof(*rp));

	cur_faults_count = MIN(sizeof(cur_faults), sizeof(some_faults));
	memcpy(cur_faults, some_faults, cur_faults_count);
	net_buf_simple_add_mem(&buf, cur_faults, cur_faults_count);
	rp->cur_faults_count = cur_faults_count;

	reg_faults_count = MIN(sizeof(reg_faults), sizeof(some_faults));
	memcpy(reg_faults, some_faults, reg_faults_count);
	net_buf_simple_add_mem(&buf, reg_faults, reg_faults_count);
	rp->reg_faults_count = reg_faults_count;

	bt_mesh_health_srv_fault_update(&elements[0]);

	tester_send(BTP_SERVICE_ID_MESH, MESH_HEALTH_GENERATE_FAULTS,
		    CONTROLLER_INDEX, buf.data, buf.len);
}

static void health_clear_faults(uint8_t *data, uint16_t len)
{
	LOG_DBG("");

	(void)memset(cur_faults, 0, sizeof(cur_faults));
	(void)memset(reg_faults, 0, sizeof(reg_faults));

	bt_mesh_health_srv_fault_update(&elements[0]);

	tester_rsp(BTP_SERVICE_ID_MESH, MESH_HEALTH_CLEAR_FAULTS,
		   CONTROLLER_INDEX, BTP_STATUS_SUCCESS);
}

static void model_send(uint8_t *data, uint16_t len)
{
	struct mesh_model_send_cmd *cmd = (void *) data;
	NET_BUF_SIMPLE_DEFINE(msg, UINT8_MAX);
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.addr = sys_le16_to_cpu(cmd->dst),
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	struct bt_mesh_model *model = NULL;
	int err, i;
	uint16_t src = sys_le16_to_cpu(cmd->src);

	/* Lookup source address */
	for (i = 0; i < ARRAY_SIZE(model_bound); i++) {
		if (bt_mesh_model_elem(model_bound[i].model)->addr == src) {
			model = model_bound[i].model;
			ctx.app_idx = model_bound[i].appkey_idx;

			break;
		}
	}

	if (!model) {
		LOG_ERR("Model not found");
		err = -EINVAL;

		goto fail;
	}

	LOG_DBG("src 0x%04x dst 0x%04x model %p payload_len %d", src,
		ctx.addr, model, cmd->payload_len);

	net_buf_simple_add_mem(&msg, cmd->payload, cmd->payload_len);

	err = bt_mesh_model_send(model, &ctx, &msg, NULL, NULL);
	if (err) {
		LOG_ERR("Failed to send (err %d)", err);
	}

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_MODEL_SEND, CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

#if defined(CONFIG_BT_TESTING)
static void lpn_subscribe(uint8_t *data, uint16_t len)
{
	struct mesh_lpn_subscribe_cmd *cmd = (void *) data;
	uint16_t address = sys_le16_to_cpu(cmd->address);
	int err;

	LOG_DBG("address 0x%04x", address);

	err = bt_test_mesh_lpn_group_add(address);
	if (err) {
		LOG_ERR("Failed to subscribe (err %d)", err);
	}

	tester_rsp(BTP_SERVICE_ID_MESH, MESH_LPN_SUBSCRIBE, CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void lpn_unsubscribe(uint8_t *data, uint16_t len)
{
	struct mesh_lpn_unsubscribe_cmd *cmd = (void *) data;
	uint16_t address = sys_le16_to_cpu(cmd->address);
	int err;

	LOG_DBG("address 0x%04x", address);

	err = bt_test_mesh_lpn_group_remove(&address, 1);
	if (err) {
		LOG_ERR("Failed to unsubscribe (err %d)", err);
	}

	tester_rsp(BTP_SERVICE_ID_MESH, MESH_LPN_UNSUBSCRIBE, CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void rpl_clear(uint8_t *data, uint16_t len)
{
	int err;

	LOG_DBG("");

	err = bt_test_mesh_rpl_clear();
	if (err) {
		LOG_ERR("Failed to clear RPL (err %d)", err);
	}

	tester_rsp(BTP_SERVICE_ID_MESH, MESH_RPL_CLEAR, CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}
#endif /* CONFIG_BT_TESTING */

static void proxy_identity_enable(uint8_t *data, uint16_t len)
{
	int err;

	LOG_DBG("");

	err = bt_mesh_proxy_identity_enable();
	if (err) {
		LOG_ERR("Failed to enable proxy identity (err %d)", err);
	}

	tester_rsp(BTP_SERVICE_ID_MESH, MESH_PROXY_IDENTITY, CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void composition_data_get(uint8_t *data, uint16_t len)
{
	struct mesh_comp_data_get_cmd *cmd = (void *)data;
	uint8_t page;
	struct net_buf_simple *comp = NET_BUF_SIMPLE(128);
	int err;

	LOG_DBG("");

	bt_mesh_cfg_cli_timeout_set(10 * MSEC_PER_SEC);

	net_buf_simple_init(comp, 0);

	err = bt_mesh_cfg_cli_comp_data_get(cmd->net_idx, cmd->address, cmd->page,
					&page, comp);
	if (err) {
		LOG_ERR("err %d", err);
		goto fail;
	}

	tester_send(BTP_SERVICE_ID_MESH, MESH_COMP_DATA_GET, CONTROLLER_INDEX,
		    comp->data, comp->len);
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_COMP_DATA_GET, CONTROLLER_INDEX, BTP_STATUS_FAILED);
}

static void config_krp_get(uint8_t *data, uint16_t len)
{
	struct mesh_cfg_krp_get_cmd *cmd = (void *)data;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(2);
	uint8_t status;
	uint8_t phase;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_krp_get(cmd->net_idx, cmd->address, cmd->key_net_idx, &status,
				      &phase);

	if (err) {
		LOG_ERR("err %d", err);
		goto fail;
	}

	net_buf_simple_init(buf, 0);
	net_buf_simple_add_u8(buf, status);
	net_buf_simple_add_u8(buf, phase);

	tester_send(BTP_SERVICE_ID_MESH, MESH_CFG_KRP_GET, CONTROLLER_INDEX, buf->data, buf->len);
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_CFG_KRP_GET, CONTROLLER_INDEX, BTP_STATUS_FAILED);
}

static void config_krp_set(uint8_t *data, uint16_t len)
{
	struct mesh_cfg_krp_set_cmd *cmd = (void *)data;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(2);
	uint8_t status;
	uint8_t phase;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_krp_set(cmd->net_idx, cmd->address, cmd->key_net_idx, cmd->transition,
				  &status, &phase);
	if (err) {
		LOG_ERR("err %d", err);
		goto fail;
	}

	net_buf_simple_init(buf, 0);
	net_buf_simple_add_u8(buf, status);
	net_buf_simple_add_u8(buf, phase);

	tester_send(BTP_SERVICE_ID_MESH, MESH_CFG_KRP_SET, CONTROLLER_INDEX, buf->data, buf->len);
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_CFG_KRP_SET, CONTROLLER_INDEX, BTP_STATUS_FAILED);
}

static void config_beacon_get(uint8_t *data, uint16_t len)
{
	struct mesh_cfg_val_get_cmd *cmd = (void *)data;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_beacon_get(cmd->net_idx, cmd->address, &status);
	if (err) {
		LOG_ERR("err %d", err);
		goto fail;
	}

	tester_send(BTP_SERVICE_ID_MESH, MESH_CFG_BEACON_GET, CONTROLLER_INDEX,
		    &status, sizeof(status));
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_CFG_BEACON_GET, CONTROLLER_INDEX, BTP_STATUS_FAILED);
}

static void config_beacon_set(uint8_t *data, uint16_t len)
{
	struct mesh_cfg_beacon_set_cmd *cmd = (void *)data;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_beacon_set(cmd->net_idx, cmd->address, cmd->val,
				     &status);
	if (err) {
		LOG_ERR("err %d", err);
		goto fail;
	}

	tester_send(BTP_SERVICE_ID_MESH, MESH_CFG_BEACON_SET, CONTROLLER_INDEX,
		    &status, sizeof(status));
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_CFG_BEACON_SET, CONTROLLER_INDEX, BTP_STATUS_FAILED);
}

static void config_default_ttl_get(uint8_t *data, uint16_t len)
{
	struct mesh_cfg_val_get_cmd *cmd = (void *)data;
	uint8_t status;
	int err;

	LOG_DBG("");
	err = bt_mesh_cfg_cli_ttl_get(cmd->net_idx, cmd->address, &status);

	if (err) {
		LOG_ERR("err %d", err);
		goto fail;
	}

	tester_send(BTP_SERVICE_ID_MESH, MESH_CFG_DEFAULT_TTL_GET,
		    CONTROLLER_INDEX, &status, sizeof(status));
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_CFG_DEFAULT_TTL_GET, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void config_default_ttl_set(uint8_t *data, uint16_t len)
{
	struct mesh_cfg_default_ttl_set_cmd *cmd = (void *)data;
	uint8_t status;
	int err;

	LOG_DBG("");
	err = bt_mesh_cfg_cli_ttl_set(cmd->net_idx, cmd->address, cmd->val,
				  &status);

	if (err) {
		LOG_ERR("err %d", err);
		goto fail;
	}

	tester_send(BTP_SERVICE_ID_MESH, MESH_CFG_DEFAULT_TTL_SET,
		    CONTROLLER_INDEX, &status, sizeof(status));
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_CFG_DEFAULT_TTL_SET, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void config_gatt_proxy_get(uint8_t *data, uint16_t len)
{
	struct mesh_cfg_val_get_cmd *cmd = (void *)data;
	uint8_t status;
	int err;

	LOG_DBG("");
	err = bt_mesh_cfg_cli_gatt_proxy_get(cmd->net_idx, cmd->address, &status);

	if (err) {
		LOG_ERR("err %d", err);
		goto fail;
	}

	tester_send(BTP_SERVICE_ID_MESH, MESH_CFG_GATT_PROXY_GET,
		    CONTROLLER_INDEX, &status, sizeof(status));
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_CFG_GATT_PROXY_GET, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void config_gatt_proxy_set(uint8_t *data, uint16_t len)
{
	struct mesh_cfg_gatt_proxy_set_cmd *cmd = (void *)data;
	uint8_t status;
	int err;

	LOG_DBG("");
	err = bt_mesh_cfg_cli_gatt_proxy_set(cmd->net_idx, cmd->address, cmd->val,
					 &status);

	if (err) {
		LOG_ERR("err %d", err);
		goto fail;
	}

	tester_send(BTP_SERVICE_ID_MESH, MESH_CFG_GATT_PROXY_SET,
		    CONTROLLER_INDEX, &status, sizeof(status));
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_CFG_GATT_PROXY_SET, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void config_friend_get(uint8_t *data, uint16_t len)
{
	struct mesh_cfg_val_get_cmd *cmd = (void *)data;
	uint8_t status;
	int err;

	LOG_DBG("");
	err = bt_mesh_cfg_cli_friend_get(cmd->net_idx, cmd->address, &status);

	if (err) {
		LOG_ERR("err %d", err);
		goto fail;
	}

	tester_send(BTP_SERVICE_ID_MESH, MESH_CFG_FRIEND_GET, CONTROLLER_INDEX,
		    &status, sizeof(status));
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_CFG_FRIEND_GET, CONTROLLER_INDEX, BTP_STATUS_FAILED);
}

static void config_friend_set(uint8_t *data, uint16_t len)
{
	struct mesh_cfg_friend_set_cmd *cmd = (void *)data;
	uint8_t status;
	int err;

	LOG_DBG("");
	err = bt_mesh_cfg_cli_friend_set(cmd->net_idx, cmd->address, cmd->val,
				     &status);

	if (err) {
		LOG_ERR("err %d", err);
		goto fail;
	}

	tester_send(BTP_SERVICE_ID_MESH, MESH_CFG_FRIEND_SET, CONTROLLER_INDEX,
		    &status, sizeof(status));
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_CFG_FRIEND_SET, CONTROLLER_INDEX, BTP_STATUS_FAILED);
}

static void config_relay_get(uint8_t *data, uint16_t len)
{
	struct mesh_cfg_val_get_cmd *cmd = (void *)data;
	uint8_t status;
	uint8_t transmit;
	int err;

	LOG_DBG("");
	err = bt_mesh_cfg_cli_relay_get(cmd->net_idx, cmd->address, &status,
				    &transmit);

	if (err) {
		LOG_ERR("err %d", err);
		goto fail;
	}

	tester_send(BTP_SERVICE_ID_MESH, MESH_CFG_RELAY_GET, CONTROLLER_INDEX,
		    &status, sizeof(status));
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_CFG_RELAY_GET, CONTROLLER_INDEX, BTP_STATUS_FAILED);
}

static void config_relay_set(uint8_t *data, uint16_t len)
{
	struct mesh_cfg_relay_set_cmd *cmd = (void *)data;
	uint8_t status;
	uint8_t transmit;
	int err;

	LOG_DBG("");
	err = bt_mesh_cfg_cli_relay_set(cmd->net_idx, cmd->address, cmd->new_relay,
				    cmd->new_transmit, &status, &transmit);

	if (err) {
		LOG_ERR("err %d", err);
		goto fail;
	}

	tester_send(BTP_SERVICE_ID_MESH, MESH_CFG_RELAY_SET, CONTROLLER_INDEX,
		    &status, sizeof(status));
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_CFG_RELAY_SET, CONTROLLER_INDEX, BTP_STATUS_FAILED);
}

static void config_mod_pub_get(uint8_t *data, uint16_t len)
{
	struct mesh_cfg_model_pub_get_cmd *cmd = (void *)data;
	struct bt_mesh_cfg_cli_mod_pub pub;
	uint8_t status;
	int err;

	LOG_DBG("");
	err = bt_mesh_cfg_cli_mod_pub_get(cmd->net_idx, cmd->address,
				      cmd->elem_address, cmd->model_id, &pub,
				      &status);

	if (err) {
		LOG_ERR("err %d", err);
		goto fail;
	}

	tester_send(BTP_SERVICE_ID_MESH, MESH_CFG_MODEL_PUB_GET,
		    CONTROLLER_INDEX, &status, sizeof(status));
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_CFG_MODEL_PUB_GET, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void config_mod_pub_set(uint8_t *data, uint16_t len)
{
	struct mesh_cfg_model_pub_set_cmd *cmd = (void *)data;
	uint8_t status;
	struct bt_mesh_cfg_cli_mod_pub pub;
	int err;

	LOG_DBG("");

	pub.addr = cmd->pub_addr;
	pub.uuid = NULL;
	pub.app_idx = cmd->app_idx;
	pub.cred_flag = cmd->cred_flag;
	pub.ttl = cmd->ttl;
	pub.period = cmd->period;
	pub.transmit = cmd->transmit;

	err = bt_mesh_cfg_cli_mod_pub_set(cmd->net_idx, cmd->address,
				      cmd->elem_address, cmd->model_id, &pub,
				      &status);

	if (err) {
		LOG_ERR("err %d", err);
		goto fail;
	}

	tester_send(BTP_SERVICE_ID_MESH, MESH_CFG_MODEL_PUB_SET,
		    CONTROLLER_INDEX, &status, sizeof(status));
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_CFG_MODEL_PUB_SET, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void config_mod_pub_va_set(uint8_t *data, uint16_t len)
{
	struct mesh_cfg_model_pub_va_set_cmd *cmd = (void *)data;
	uint8_t status;
	struct bt_mesh_cfg_cli_mod_pub pub;
	int err;

	LOG_DBG("");

	pub.uuid = cmd->uuid;
	pub.app_idx = cmd->app_idx;
	pub.cred_flag = cmd->cred_flag;
	pub.ttl = cmd->ttl;
	pub.period = cmd->period;
	pub.transmit = cmd->transmit;

	err = bt_mesh_cfg_cli_mod_pub_set(cmd->net_idx, cmd->address,
				      cmd->elem_address, cmd->model_id,
				      &pub, &status);

	if (err) {
		LOG_ERR("err %d", err);
		goto fail;
	}

	tester_send(BTP_SERVICE_ID_MESH, MESH_CFG_MODEL_PUB_VA_SET,
		    CONTROLLER_INDEX, &status, sizeof(status));
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_CFG_MODEL_PUB_VA_SET, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void config_mod_sub_add(uint8_t *data, uint16_t len)
{
	struct mesh_cfg_model_sub_cmd *cmd = (void *)data;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_mod_sub_add(cmd->net_idx, cmd->address,
				      cmd->elem_address, cmd->sub_addr,
				      cmd->model_id, &status);

	if (err) {
		LOG_ERR("err %d", err);
		goto fail;
	}

	tester_send(BTP_SERVICE_ID_MESH, MESH_CFG_MODEL_SUB_ADD,
		    CONTROLLER_INDEX, &status, sizeof(status));
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_CFG_MODEL_SUB_ADD, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void config_mod_sub_ovw(uint8_t *data, uint16_t len)
{
	struct mesh_cfg_model_sub_cmd *cmd = (void *)data;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_mod_sub_overwrite(cmd->net_idx, cmd->address,
					    cmd->elem_address, cmd->sub_addr,
					    cmd->model_id, &status);

	if (err) {
		LOG_ERR("err %d", err);
		goto fail;
	}

	tester_send(BTP_SERVICE_ID_MESH, MESH_CFG_MODEL_SUB_OVW,
		    CONTROLLER_INDEX, &status, sizeof(status));
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_CFG_MODEL_SUB_OVW, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void config_mod_sub_del(uint8_t *data, uint16_t len)
{
	struct mesh_cfg_model_sub_cmd *cmd = (void *)data;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_mod_sub_del(cmd->net_idx, cmd->address,
				      cmd->elem_address, cmd->sub_addr,
				      cmd->model_id, &status);

	if (err) {
		LOG_ERR("err %d", err);
		goto fail;
	}

	tester_send(BTP_SERVICE_ID_MESH, MESH_CFG_MODEL_SUB_DEL,
		    CONTROLLER_INDEX, &status, sizeof(status));
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_CFG_MODEL_SUB_DEL, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void config_mod_sub_del_all(uint8_t *data, uint16_t len)
{
	struct mesh_cfg_model_sub_del_all_cmd *cmd = (void *)data;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_mod_sub_del_all(cmd->net_idx, cmd->address,
					  cmd->elem_address, cmd->model_id,
					  &status);

	if (err) {
		LOG_ERR("err %d", err);
		goto fail;
	}

	tester_send(BTP_SERVICE_ID_MESH, MESH_CFG_MODEL_SUB_DEL_ALL,
		    CONTROLLER_INDEX, &status, sizeof(status));
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_CFG_MODEL_SUB_DEL_ALL, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void config_mod_sub_get(uint8_t *data, uint16_t len)
{
	struct mesh_cfg_model_sub_get_cmd *cmd = (void *)data;
	uint8_t status;
	int16_t subs;
	size_t sub_cn;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_mod_sub_get(cmd->net_idx, cmd->address,
				      cmd->elem_address, cmd->model_id, &status,
				      &subs, &sub_cn);

	if (err) {
		LOG_ERR("err %d", err);
		goto fail;
	}

	tester_send(BTP_SERVICE_ID_MESH, MESH_CFG_MODEL_SUB_GET,
		    CONTROLLER_INDEX, &status, sizeof(status));
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_CFG_MODEL_SUB_GET, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void config_mod_sub_get_vnd(uint8_t *data, uint16_t len)
{
	struct mesh_cfg_model_sub_get_vnd_cmd *cmd = (void *)data;
	uint8_t status;
	uint16_t subs;
	size_t sub_cn;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_mod_sub_get_vnd(cmd->net_idx, cmd->address,
					  cmd->elem_address, cmd->model_id,
					  cmd->cid, &status, &subs, &sub_cn);

	if (err) {
		LOG_ERR("err %d", err);
		goto fail;
	}

	tester_send(BTP_SERVICE_ID_MESH, MESH_CFG_MODEL_SUB_GET_VND,
		    CONTROLLER_INDEX, &status, sizeof(status));
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_CFG_MODEL_SUB_GET_VND, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void config_mod_sub_va_add(uint8_t *data, uint16_t len)
{
	struct mesh_cfg_model_sub_va_cmd *cmd = (void *)data;
	uint8_t status;
	uint16_t virt_addr_rcv;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_mod_sub_va_add(cmd->net_idx, cmd->address,
					 cmd->elem_address, cmd->uuid,
					 cmd->model_id, &virt_addr_rcv,
					 &status);

	if (err) {
		LOG_ERR("err %d", err);
		goto fail;
	}

	tester_send(BTP_SERVICE_ID_MESH, MESH_CFG_MODEL_SUB_VA_ADD,
		    CONTROLLER_INDEX, &status, sizeof(status));
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_CFG_MODEL_SUB_VA_ADD, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void config_mod_sub_va_del(uint8_t *data, uint16_t len)
{
	struct mesh_cfg_model_sub_va_cmd *cmd = (void *)data;
	uint8_t status;
	uint16_t virt_addr_rcv;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_mod_sub_va_del(cmd->net_idx, cmd->address,
					 cmd->elem_address, cmd->uuid,
					 cmd->model_id, &virt_addr_rcv,
					 &status);

	if (err) {
		LOG_ERR("err %d", err);
		goto fail;
	}

	tester_send(BTP_SERVICE_ID_MESH, MESH_CFG_MODEL_SUB_VA_DEL,
		    CONTROLLER_INDEX, &status, sizeof(status));
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_CFG_MODEL_SUB_VA_DEL, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void config_mod_sub_va_ovw(uint8_t *data, uint16_t len)
{
	struct mesh_cfg_model_sub_va_cmd *cmd = (void *)data;
	uint8_t status;
	uint16_t virt_addr_rcv;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_mod_sub_va_overwrite(cmd->net_idx, cmd->address,
					       cmd->elem_address,
					       cmd->uuid, cmd->model_id,
					       &virt_addr_rcv, &status);

	if (err) {
		LOG_ERR("err %d", err);
		goto fail;
	}

	tester_send(BTP_SERVICE_ID_MESH, MESH_CFG_MODEL_SUB_VA_OVW,
		    CONTROLLER_INDEX, &status, sizeof(status));
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_CFG_MODEL_SUB_VA_OVW, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void config_netkey_add(uint8_t *data, uint16_t len)
{
	struct mesh_cfg_netkey_add_cmd *cmd = (void *)data;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_net_key_add(cmd->net_idx, cmd->address,
				      cmd->net_key_idx, cmd->net_key, &status);

	if (err) {
		LOG_ERR("err %d", err);
		goto fail;
	}

	tester_send(BTP_SERVICE_ID_MESH, MESH_CFG_NETKEY_ADD, CONTROLLER_INDEX,
		    &status, sizeof(status));
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_CFG_NETKEY_ADD, CONTROLLER_INDEX, BTP_STATUS_FAILED);
}

static void config_netkey_update(uint8_t *data, uint16_t len)
{
	struct mesh_cfg_netkey_add_cmd *cmd = (void *)data;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_net_key_update(cmd->net_idx, cmd->address,
					 cmd->net_key_idx, cmd->net_key,
					 &status);

	if (err) {
		LOG_ERR("err %d", err);
		goto fail;
	}

	tester_send(BTP_SERVICE_ID_MESH, MESH_CFG_NETKEY_UPDATE,
		    CONTROLLER_INDEX, &status, sizeof(status));
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_CFG_NETKEY_UPDATE, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void config_netkey_get(uint8_t *data, uint16_t len)
{
	struct mesh_cfg_val_get_cmd *cmd = (void *)data;
	uint16_t keys;
	size_t key_cnt;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_net_key_get(cmd->net_idx, cmd->address, &keys,
				      &key_cnt);

	if (err) {
		LOG_ERR("err %d", err);
		goto fail;
	}

	tester_send(BTP_SERVICE_ID_MESH, MESH_CFG_NETKEY_GET, CONTROLLER_INDEX,
		    (uint8_t *)&keys, key_cnt);
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_CFG_NETKEY_GET, CONTROLLER_INDEX, BTP_STATUS_FAILED);
}

static void config_netkey_del(uint8_t *data, uint16_t len)
{
	struct mesh_cfg_netkey_del_cmd *cmd = (void *)data;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_net_key_del(cmd->net_idx, cmd->address,
				      cmd->net_key_idx, &status);

	if (err) {
		LOG_ERR("err %d", err);
		goto fail;
	}

	tester_send(BTP_SERVICE_ID_MESH, MESH_CFG_NETKEY_DEL, CONTROLLER_INDEX,
		    &status, sizeof(status));
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_CFG_NETKEY_DEL, CONTROLLER_INDEX, BTP_STATUS_FAILED);
}

static void config_appkey_add(uint8_t *data, uint16_t len)
{
	struct mesh_cfg_appkey_add_cmd *cmd = (void *)data;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_app_key_add(cmd->net_idx, cmd->address,
				      cmd->net_key_idx, cmd->app_key_idx,
				      cmd->app_key, &status);

	if (err) {
		LOG_ERR("err %d", err);
		goto fail;
	}

	tester_send(BTP_SERVICE_ID_MESH, MESH_CFG_APPKEY_ADD, CONTROLLER_INDEX,
		    &status, sizeof(status));
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_CFG_APPKEY_ADD, CONTROLLER_INDEX, BTP_STATUS_FAILED);
}

static void config_appkey_update(uint8_t *data, uint16_t len)
{
	struct mesh_cfg_appkey_add_cmd *cmd = (void *)data;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_app_key_update(cmd->net_idx, cmd->address,
					 cmd->net_key_idx, cmd->app_key_idx,
					 cmd->app_key, &status);

	if (err) {
		LOG_ERR("err %d", err);
		goto fail;
	}

	tester_send(BTP_SERVICE_ID_MESH, MESH_CFG_APPKEY_UPDATE,
		    CONTROLLER_INDEX, &status, sizeof(status));
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_CFG_APPKEY_UPDATE, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void config_appkey_del(uint8_t *data, uint16_t len)
{
	struct mesh_cfg_appkey_del_cmd *cmd = (void *)data;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_app_key_del(cmd->net_idx, cmd->address,
				      cmd->net_key_idx, cmd->app_key_idx,
				      &status);

	if (err) {
		LOG_ERR("err %d", err);
		goto fail;
	}

	tester_send(BTP_SERVICE_ID_MESH, MESH_CFG_APPKEY_DEL, CONTROLLER_INDEX,
		    &status, sizeof(status));
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_CFG_APPKEY_DEL, CONTROLLER_INDEX, BTP_STATUS_FAILED);
}

static void config_appkey_get(uint8_t *data, uint16_t len)
{
	struct mesh_cfg_appkey_get_cmd *cmd = (void *)data;
	uint8_t status;
	uint16_t keys;
	size_t key_cnt;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_app_key_get(cmd->net_idx, cmd->address,
				      cmd->net_key_idx, &status, &keys,
				      &key_cnt);

	if (err) {
		LOG_ERR("err %d", err);
		goto fail;
	}

	tester_send(BTP_SERVICE_ID_MESH, MESH_CFG_APPKEY_GET, CONTROLLER_INDEX,
		    &status, sizeof(status));
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_CFG_APPKEY_GET, CONTROLLER_INDEX, BTP_STATUS_FAILED);
}

static void config_model_app_bind(uint8_t *data, uint16_t len)
{
	struct mesh_cfg_model_app_bind_cmd *cmd = (void *)data;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_mod_app_bind(cmd->net_idx, cmd->address,
				       cmd->elem_address, cmd->app_key_idx,
				       cmd->mod_id, &status);

	if (err) {
		LOG_ERR("err %d", err);
		goto fail;
	}

	tester_send(BTP_SERVICE_ID_MESH, MESH_CFG_MODEL_APP_BIND,
		    CONTROLLER_INDEX, &status, sizeof(status));
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_CFG_MODEL_APP_BIND, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void config_model_app_bind_vnd(uint8_t *data, uint16_t len)
{
	struct mesh_cfg_model_app_bind_vnd_cmd *cmd = (void *)data;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_mod_app_bind_vnd(cmd->net_idx, cmd->address,
					   cmd->elem_address, cmd->app_key_idx,
					   cmd->mod_id, cmd->cid, &status);

	if (err) {
		LOG_ERR("err %d", err);
		goto fail;
	}

	tester_send(BTP_SERVICE_ID_MESH, MESH_CFG_MODEL_APP_BIND_VND,
		    CONTROLLER_INDEX, &status, sizeof(status));
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_CFG_MODEL_APP_BIND_VND, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void config_model_app_unbind(uint8_t *data, uint16_t len)
{
	struct mesh_cfg_model_app_bind_cmd *cmd = (void *)data;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_mod_app_unbind(cmd->net_idx, cmd->address,
					 cmd->elem_address, cmd->app_key_idx,
					 cmd->mod_id, &status);

	if (err) {
		LOG_ERR("err %d", err);
		goto fail;
	}

	tester_send(BTP_SERVICE_ID_MESH, MESH_CFG_MODEL_APP_UNBIND,
		    CONTROLLER_INDEX, &status, sizeof(status));
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_CFG_MODEL_APP_UNBIND, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void config_model_app_get(uint8_t *data, uint16_t len)
{
	struct mesh_cfg_model_app_get_cmd *cmd = (void *)data;
	uint8_t status;
	uint16_t apps;
	size_t app_cnt;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_mod_app_get(cmd->net_idx, cmd->address,
				      cmd->elem_address, cmd->mod_id, &status,
				      &apps, &app_cnt);

	if (err) {
		LOG_ERR("err %d", err);
		goto fail;
	}

	tester_send(BTP_SERVICE_ID_MESH, MESH_CFG_MODEL_APP_GET,
		    CONTROLLER_INDEX, &status, sizeof(status));
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_CFG_MODEL_APP_GET, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void config_model_app_vnd_get(uint8_t *data, uint16_t len)
{
	struct mesh_cfg_model_app_get_cmd *cmd = (void *)data;
	uint8_t status;
	uint16_t apps;
	size_t app_cnt;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_mod_app_get_vnd(cmd->net_idx, cmd->address,
					  cmd->elem_address, cmd->mod_id,
					  cmd->cid, &status, &apps, &app_cnt);

	if (err) {
		LOG_ERR("err %d", err);
		goto fail;
	}

	tester_send(BTP_SERVICE_ID_MESH, MESH_CFG_MODEL_APP_VND_GET,
		    CONTROLLER_INDEX, &status, sizeof(status));
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_CFG_MODEL_APP_VND_GET, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void config_hb_pub_set(uint8_t *data, uint16_t len)
{
	struct mesh_cfg_heartbeat_pub_set_cmd *cmd = (void *)data;
	uint8_t status;
	struct bt_mesh_cfg_cli_hb_pub pub;
	int err;

	LOG_DBG("");

	pub.net_idx = cmd->net_key_idx;
	pub.dst = cmd->destination;
	pub.count = cmd->count_log;
	pub.period = cmd->period_log;
	pub.ttl = cmd->ttl;
	pub.feat = cmd->features;

	err = bt_mesh_cfg_cli_hb_pub_set(cmd->net_idx, cmd->address, &pub, &status);

	if (err) {
		LOG_ERR("err %d", err);
		goto fail;
	}

	tester_send(BTP_SERVICE_ID_MESH, MESH_CFG_HEARTBEAT_PUB_SET,
		    CONTROLLER_INDEX, &status, sizeof(status));
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_CFG_HEARTBEAT_PUB_SET, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void config_hb_pub_get(uint8_t *data, uint16_t len)
{
	struct mesh_cfg_val_get_cmd *cmd = (void *)data;
	uint8_t status;
	struct bt_mesh_cfg_cli_hb_pub pub;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_hb_pub_get(cmd->net_idx, cmd->address, &pub, &status);

	if (err) {
		LOG_ERR("err %d", err);
		goto fail;
	}

	tester_send(BTP_SERVICE_ID_MESH, MESH_CFG_HEARTBEAT_PUB_GET,
		    CONTROLLER_INDEX, &status, sizeof(status));
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_CFG_HEARTBEAT_PUB_GET, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void config_hb_sub_set(uint8_t *data, uint16_t len)
{
	struct mesh_cfg_heartbeat_sub_set_cmd *cmd = (void *)data;
	uint8_t status;
	struct bt_mesh_cfg_cli_hb_sub sub;
	int err;

	LOG_DBG("");

	sub.src = cmd->source;
	sub.dst = cmd->destination;
	sub.period = cmd->period_log;

	err = bt_mesh_cfg_cli_hb_sub_set(cmd->net_idx, cmd->address, &sub, &status);

	if (err) {
		LOG_ERR("err %d", err);
		goto fail;
	}

	tester_send(BTP_SERVICE_ID_MESH, MESH_CFG_HEARTBEAT_SUB_SET,
		    CONTROLLER_INDEX, &status, sizeof(status));
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_CFG_HEARTBEAT_SUB_SET, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void config_hb_sub_get(uint8_t *data, uint16_t len)
{
	struct mesh_cfg_val_get_cmd *cmd = (void *)data;
	uint8_t status;
	struct bt_mesh_cfg_cli_hb_sub sub;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_hb_sub_get(cmd->net_idx, cmd->address, &sub, &status);

	if (err) {
		LOG_ERR("err %d", err);
		goto fail;
	}

	tester_send(BTP_SERVICE_ID_MESH, MESH_CFG_HEARTBEAT_SUB_GET,
		    CONTROLLER_INDEX, &status, sizeof(status));
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_CFG_HEARTBEAT_SUB_GET, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void config_net_trans_get(uint8_t *data, uint16_t len)
{
	struct mesh_cfg_val_get_cmd *cmd = (void *)data;
	uint8_t transmit;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_net_transmit_get(cmd->net_idx, cmd->address,
					   &transmit);

	if (err) {
		LOG_ERR("err %d", err);
		goto fail;
	}

	tester_send(BTP_SERVICE_ID_MESH, MESH_CFG_NET_TRANS_GET,
		    CONTROLLER_INDEX, &transmit, sizeof(transmit));
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_CFG_NET_TRANS_GET, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void config_net_trans_set(uint8_t *data, uint16_t len)
{
	struct mesh_cfg_net_trans_set_cmd *cmd = (void *)data;
	uint8_t transmit;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_net_transmit_set(cmd->net_idx, cmd->address,
					   cmd->transmit, &transmit);

	if (err) {
		LOG_ERR("err %d", err);
		goto fail;
	}

	tester_send(BTP_SERVICE_ID_MESH, MESH_CFG_NET_TRANS_SET,
		    CONTROLLER_INDEX, &transmit, sizeof(transmit));
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_CFG_NET_TRANS_SET, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void config_node_identity_set(uint8_t *data, uint16_t len)
{
	struct mesh_cfg_node_idt_set_cmd *cmd = (void *)data;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(2);
	uint8_t identity;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_node_identity_set(cmd->net_idx, cmd->address,
					    cmd->net_key_idx, cmd->new_identity,
					    &status, &identity);

	if (err) {
		LOG_ERR("err %d", err);
		goto fail;
	}

	net_buf_simple_init(buf, 0);
	net_buf_simple_add_u8(buf, status);
	net_buf_simple_add_u8(buf, identity);

	tester_send(BTP_SERVICE_ID_MESH, MESH_CFG_NODE_IDT_SET,
		    CONTROLLER_INDEX, buf->data, buf->len);
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_CFG_NODE_IDT_SET, CONTROLLER_INDEX, BTP_STATUS_FAILED);
}

static void config_node_identity_get(uint8_t *data, uint16_t len)
{
	struct mesh_cfg_node_idt_get_cmd *cmd = (void *)data;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(2);
	uint8_t identity;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_node_identity_get(cmd->net_idx, cmd->address,
					    cmd->net_key_idx, &status,
					    &identity);

	if (err) {
		LOG_ERR("err %d", err);
		goto fail;
	}

	net_buf_simple_init(buf, 0);
	net_buf_simple_add_u8(buf, status);
	net_buf_simple_add_u8(buf, identity);

	tester_send(BTP_SERVICE_ID_MESH, MESH_CFG_NODE_IDT_GET,
		    CONTROLLER_INDEX, buf->data, buf->len);
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_CFG_NODE_IDT_GET, CONTROLLER_INDEX, BTP_STATUS_FAILED);
}

static void config_node_reset(uint8_t *data, uint16_t len)
{
	struct mesh_cfg_node_reset_cmd *cmd = (void *)data;
	bool status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_node_reset(cmd->net_idx, cmd->address, &status);

	if (err) {
		LOG_ERR("err %d", err);
		goto fail;
	}

	tester_send(BTP_SERVICE_ID_MESH, MESH_CFG_NODE_RESET, CONTROLLER_INDEX,
		    (uint8_t *)&status, sizeof(status));
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_CFG_NODE_RESET, CONTROLLER_INDEX, BTP_STATUS_FAILED);
}

static void config_lpn_timeout_get(uint8_t *data, uint16_t len)
{
	struct mesh_cfg_lpn_timeout_cmd *cmd = (void *)data;
	int32_t polltimeout;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_lpn_timeout_get(cmd->net_idx, cmd->address,
					  cmd->unicast_addr, &polltimeout);

	if (err) {
		LOG_ERR("err %d", err);
		goto fail;
	}

	tester_send(BTP_SERVICE_ID_MESH, MESH_CFG_LPN_TIMEOUT_GET,
		    CONTROLLER_INDEX, (uint8_t *)&polltimeout,
		    sizeof(polltimeout));
	return;

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_CFG_LPN_TIMEOUT_GET, CONTROLLER_INDEX,
		   BTP_STATUS_FAILED);
}

static void health_fault_get(uint8_t *data, uint16_t len)
{
	struct mesh_health_fault_get_cmd *cmd = (void *)data;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.addr = cmd->address,
		.app_idx = cmd->app_idx,
	};
	uint8_t test_id;
	size_t fault_count = 16;
	uint8_t faults[fault_count];
	int err;

	LOG_DBG("");

	err = bt_mesh_health_cli_fault_get(&health_cli, &ctx, cmd->cid, &test_id, faults,
					   &fault_count);

	if (err) {
		LOG_ERR("err %d", err);
		goto fail;
	}

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_HEALTH_FAULT_GET, CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void health_fault_clear(uint8_t *data, uint16_t len)
{
	struct mesh_health_fault_clear_cmd *cmd = (void *)data;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.addr = cmd->address,
		.app_idx = cmd->app_idx,
	};
	uint8_t test_id;
	size_t fault_count = 16;
	uint8_t faults[fault_count];
	int err;

	LOG_DBG("");

	if (cmd->ack) {
		err = bt_mesh_health_cli_fault_clear(&health_cli, &ctx, cmd->cid, &test_id, faults,
						     &fault_count);
	} else {
		err = bt_mesh_health_cli_fault_clear_unack(&health_cli, &ctx, cmd->cid);
	}

	if (err) {
		LOG_ERR("err %d", err);
		goto fail;
	}

	if (cmd->ack) {
		tester_send(BTP_SERVICE_ID_MESH, MESH_HEALTH_FAULT_CLEAR,
			    CONTROLLER_INDEX, &test_id, sizeof(test_id));
		return;
	}

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_HEALTH_FAULT_CLEAR,
		   CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void health_fault_test(uint8_t *data, uint16_t len)
{
	struct mesh_health_fault_test_cmd *cmd = (void *)data;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(19);
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.addr = cmd->address,
		.app_idx = cmd->app_idx,
	};
	size_t fault_count = 16;
	uint8_t faults[fault_count];
	uint8_t test_id;
	uint16_t cid;
	int err;

	LOG_DBG("");

	test_id = cmd->test_id;
	cid = cmd->cid;

	if (cmd->ack) {
		err = bt_mesh_health_cli_fault_test(&health_cli, &ctx, cid, test_id, faults,
						    &fault_count);
	} else {
		err = bt_mesh_health_cli_fault_test_unack(&health_cli, &ctx, cid, test_id);
	}

	if (err) {
		LOG_ERR("err %d", err);
		goto fail;
	}

	if (cmd->ack) {
		net_buf_simple_init(buf, 0);
		net_buf_simple_add_u8(buf, test_id);
		net_buf_simple_add_le16(buf, cid);
		net_buf_simple_add_mem(buf, faults, fault_count);

		tester_send(BTP_SERVICE_ID_MESH, MESH_HEALTH_FAULT_TEST,
			    CONTROLLER_INDEX, buf->data, buf->len);
		return;
	}

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_HEALTH_FAULT_TEST,
		   CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void health_period_get(uint8_t *data, uint16_t len)
{
	struct mesh_health_period_get_cmd *cmd = (void *)data;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.addr = cmd->address,
		.app_idx = cmd->app_idx,
	};
	uint8_t divisor;
	int err;

	LOG_DBG("");

	err = bt_mesh_health_cli_period_get(&health_cli, &ctx, &divisor);

	if (err) {
		LOG_ERR("err %d", err);
		goto fail;
	}

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_HEALTH_PERIOD_GET,
		   CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void health_period_set(uint8_t *data, uint16_t len)
{
	struct mesh_health_period_set_cmd *cmd = (void *)data;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.addr = cmd->address,
		.app_idx = cmd->app_idx,
	};
	uint8_t updated_divisor;
	int err;

	LOG_DBG("");

	if (cmd->ack) {
		err = bt_mesh_health_cli_period_set(&health_cli, &ctx, cmd->divisor,
						    &updated_divisor);
	} else {
		err = bt_mesh_health_cli_period_set_unack(&health_cli, &ctx, cmd->divisor);
	}

	if (err) {
		LOG_ERR("err %d", err);
		goto fail;
	}

	if (cmd->ack) {
		tester_send(BTP_SERVICE_ID_MESH, MESH_HEALTH_PERIOD_SET,
			    CONTROLLER_INDEX, &updated_divisor,
			    sizeof(updated_divisor));
		return;
	}

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_HEALTH_PERIOD_SET,
		   CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void health_attention_get(uint8_t *data, uint16_t len)
{
	struct mesh_health_attention_get_cmd *cmd = (void *)data;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.addr = cmd->address,
		.app_idx = cmd->app_idx,
	};
	uint8_t attention;
	int err;

	LOG_DBG("");

	err = bt_mesh_health_cli_attention_get(&health_cli, &ctx, &attention);

	if (err) {
		LOG_ERR("err %d", err);
		goto fail;
	}

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_HEALTH_ATTENTION_GET,
		   CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void health_attention_set(uint8_t *data, uint16_t len)
{
	struct mesh_health_attention_set_cmd *cmd = (void *)data;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.addr = cmd->address,
		.app_idx = cmd->app_idx,
	};
	uint8_t updated_attention;
	int err;

	LOG_DBG("");

	if (cmd->ack) {
		err = bt_mesh_health_cli_attention_set(&health_cli, &ctx, cmd->attention,
						       &updated_attention);
	} else {
		err = bt_mesh_health_cli_attention_set_unack(&health_cli, &ctx, cmd->attention);
	}

	if (err) {
		LOG_ERR("err %d", err);
		goto fail;
	}

	if (cmd->ack) {
		tester_send(BTP_SERVICE_ID_MESH, MESH_HEALTH_ATTENTION_SET,
			    CONTROLLER_INDEX, &updated_attention,
			    sizeof(updated_attention));
		return;
	}

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_HEALTH_ATTENTION_SET,
		   CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

void tester_handle_mesh(uint8_t opcode, uint8_t index, uint8_t *data, uint16_t len)
{
	switch (opcode) {
	case MESH_READ_SUPPORTED_COMMANDS:
		supported_commands(data, len);
		break;
	case MESH_CONFIG_PROVISIONING:
		config_prov(data, len);
		break;
	case MESH_PROVISION_NODE:
		provision_node(data, len);
		break;
	case MESH_INIT:
		init(data, len);
		break;
	case MESH_RESET:
		reset(data, len);
		break;
	case MESH_INPUT_NUMBER:
		input_number(data, len);
		break;
	case MESH_INPUT_STRING:
		input_string(data, len);
		break;
	case MESH_IVU_TEST_MODE:
		ivu_test_mode(data, len);
		break;
	case MESH_IVU_TOGGLE_STATE:
		ivu_toggle_state(data, len);
		break;
	case MESH_LPN:
		lpn(data, len);
		break;
	case MESH_LPN_POLL:
		lpn_poll(data, len);
		break;
	case MESH_NET_SEND:
		net_send(data, len);
		break;
	case MESH_HEALTH_GENERATE_FAULTS:
		health_generate_faults(data, len);
		break;
	case MESH_HEALTH_CLEAR_FAULTS:
		health_clear_faults(data, len);
		break;
	case MESH_MODEL_SEND:
		model_send(data, len);
		break;
	case MESH_COMP_DATA_GET:
		composition_data_get(data, len);
		break;
	case MESH_CFG_BEACON_GET:
		config_beacon_get(data, len);
		break;
	case MESH_CFG_BEACON_SET:
		config_beacon_set(data, len);
		break;
	case MESH_CFG_DEFAULT_TTL_GET:
		config_default_ttl_get(data, len);
		break;
	case MESH_CFG_DEFAULT_TTL_SET:
		config_default_ttl_set(data, len);
		break;
	case MESH_CFG_GATT_PROXY_GET:
		config_gatt_proxy_get(data, len);
		break;
	case MESH_CFG_GATT_PROXY_SET:
		config_gatt_proxy_set(data, len);
		break;
	case MESH_CFG_FRIEND_GET:
		config_friend_get(data, len);
		break;
	case MESH_CFG_FRIEND_SET:
		config_friend_set(data, len);
		break;
	case MESH_CFG_RELAY_GET:
		config_relay_get(data, len);
		break;
	case MESH_CFG_RELAY_SET:
		config_relay_set(data, len);
		break;
	case MESH_CFG_MODEL_PUB_GET:
		config_mod_pub_get(data, len);
		break;
	case MESH_CFG_MODEL_PUB_SET:
		config_mod_pub_set(data, len);
		break;
	case MESH_CFG_MODEL_SUB_ADD:
		config_mod_sub_add(data, len);
		break;
	case MESH_CFG_MODEL_SUB_DEL:
		config_mod_sub_del(data, len);
		break;
	case MESH_CFG_MODEL_SUB_OVW:
		config_mod_sub_ovw(data, len);
		break;
	case MESH_CFG_MODEL_SUB_DEL_ALL:
		config_mod_sub_del_all(data, len);
		break;
	case MESH_CFG_MODEL_SUB_GET:
		config_mod_sub_get(data, len);
		break;
	case MESH_CFG_MODEL_SUB_GET_VND:
		config_mod_sub_get_vnd(data, len);
		break;
	case MESH_CFG_MODEL_SUB_VA_ADD:
		config_mod_sub_va_add(data, len);
		break;
	case MESH_CFG_MODEL_SUB_VA_DEL:
		config_mod_sub_va_del(data, len);
		break;
	case MESH_CFG_MODEL_SUB_VA_OVW:
		config_mod_sub_va_ovw(data, len);
		break;
	case MESH_CFG_NETKEY_ADD:
		config_netkey_add(data, len);
		break;
	case MESH_CFG_NETKEY_GET:
		config_netkey_get(data, len);
		break;
	case MESH_CFG_NETKEY_DEL:
		config_netkey_del(data, len);
		break;
	case MESH_CFG_NETKEY_UPDATE:
		config_netkey_update(data, len);
		break;
	case MESH_CFG_APPKEY_ADD:
		config_appkey_add(data, len);
		break;
	case MESH_CFG_APPKEY_DEL:
		config_appkey_del(data, len);
		break;
	case MESH_CFG_APPKEY_GET:
		config_appkey_get(data, len);
		break;
	case MESH_CFG_APPKEY_UPDATE:
		config_appkey_update(data, len);
		break;
	case MESH_CFG_MODEL_APP_BIND:
		config_model_app_bind(data, len);
		break;
	case MESH_CFG_MODEL_APP_UNBIND:
		config_model_app_unbind(data, len);
		break;
	case MESH_CFG_MODEL_APP_GET:
		config_model_app_get(data, len);
		break;
	case MESH_CFG_MODEL_APP_VND_GET:
		config_model_app_vnd_get(data, len);
		break;
	case MESH_CFG_HEARTBEAT_PUB_SET:
		config_hb_pub_set(data, len);
		break;
	case MESH_CFG_HEARTBEAT_PUB_GET:
		config_hb_pub_get(data, len);
		break;
	case MESH_CFG_HEARTBEAT_SUB_SET:
		config_hb_sub_set(data, len);
		break;
	case MESH_CFG_HEARTBEAT_SUB_GET:
		config_hb_sub_get(data, len);
		break;
	case MESH_CFG_NET_TRANS_GET:
		config_net_trans_get(data, len);
		break;
	case MESH_CFG_NET_TRANS_SET:
		config_net_trans_set(data, len);
		break;
	case MESH_CFG_NODE_IDT_SET:
		config_node_identity_set(data, len);
		break;
	case MESH_CFG_NODE_IDT_GET:
		config_node_identity_get(data, len);
		break;
	case MESH_CFG_NODE_RESET:
		config_node_reset(data, len);
		break;
	case MESH_CFG_LPN_TIMEOUT_GET:
		config_lpn_timeout_get(data, len);
		break;
	case MESH_CFG_MODEL_PUB_VA_SET:
		config_mod_pub_va_set(data, len);
		break;
	case MESH_CFG_MODEL_APP_BIND_VND:
		config_model_app_bind_vnd(data, len);
		break;
	case MESH_HEALTH_FAULT_GET:
		health_fault_get(data, len);
		break;
	case MESH_HEALTH_FAULT_CLEAR:
		health_fault_clear(data, len);
		break;
	case MESH_HEALTH_FAULT_TEST:
		health_fault_test(data, len);
		break;
	case MESH_HEALTH_PERIOD_GET:
		health_period_get(data, len);
		break;
	case MESH_HEALTH_PERIOD_SET:
		health_period_set(data, len);
		break;
	case MESH_HEALTH_ATTENTION_GET:
		health_attention_get(data, len);
		break;
	case MESH_HEALTH_ATTENTION_SET:
		health_attention_set(data, len);
		break;
	case MESH_PROVISION_ADV:
		provision_adv(data, len);
		break;
	case MESH_CFG_KRP_GET:
		config_krp_get(data, len);
		break;
	case MESH_CFG_KRP_SET:
		config_krp_set(data, len);
		break;
#if defined(CONFIG_BT_TESTING)
	case MESH_LPN_SUBSCRIBE:
		lpn_subscribe(data, len);
		break;
	case MESH_LPN_UNSUBSCRIBE:
		lpn_unsubscribe(data, len);
		break;
	case MESH_RPL_CLEAR:
		rpl_clear(data, len);
		break;
#endif /* CONFIG_BT_TESTING */
	case MESH_PROXY_IDENTITY:
		proxy_identity_enable(data, len);
		break;
	default:
		tester_rsp(BTP_SERVICE_ID_MESH, opcode, index,
			   BTP_STATUS_UNKNOWN_CMD);
		break;
	}
}

void net_recv_ev(uint8_t ttl, uint8_t ctl, uint16_t src, uint16_t dst, const void *payload,
		 size_t payload_len)
{
	NET_BUF_SIMPLE_DEFINE(buf, UINT8_MAX);
	struct mesh_net_recv_ev *ev;

	LOG_DBG("ttl 0x%02x ctl 0x%02x src 0x%04x dst 0x%04x payload_len %zu",
		ttl, ctl, src, dst, payload_len);

	if (payload_len > net_buf_simple_tailroom(&buf)) {
		LOG_ERR("Payload size exceeds buffer size");
		return;
	}

	ev = net_buf_simple_add(&buf, sizeof(*ev));
	ev->ttl = ttl;
	ev->ctl = ctl;
	ev->src = sys_cpu_to_le16(src);
	ev->dst = sys_cpu_to_le16(dst);
	ev->payload_len = payload_len;
	net_buf_simple_add_mem(&buf, payload, payload_len);

	tester_send(BTP_SERVICE_ID_MESH, MESH_EV_NET_RECV, CONTROLLER_INDEX,
		    buf.data, buf.len);
}

static void model_bound_cb(uint16_t addr, struct bt_mesh_model *model,
			   uint16_t key_idx)
{
	int i;

	LOG_DBG("remote addr 0x%04x key_idx 0x%04x model %p",
		addr, key_idx, model);

	for (i = 0; i < ARRAY_SIZE(model_bound); i++) {
		if (!model_bound[i].model) {
			model_bound[i].model = model;
			model_bound[i].addr = addr;
			model_bound[i].appkey_idx = key_idx;

			return;
		}
	}

	LOG_ERR("model_bound is full");
}

static void model_unbound_cb(uint16_t addr, struct bt_mesh_model *model,
			     uint16_t key_idx)
{
	int i;

	LOG_DBG("remote addr 0x%04x key_idx 0x%04x model %p",
		addr, key_idx, model);

	for (i = 0; i < ARRAY_SIZE(model_bound); i++) {
		if (model_bound[i].model == model) {
			model_bound[i].model = NULL;
			model_bound[i].addr = 0x0000;
			model_bound[i].appkey_idx = BT_MESH_KEY_UNUSED;

			return;
		}
	}

	LOG_INF("model not found");
}

static void invalid_bearer_cb(uint8_t opcode)
{
	struct mesh_invalid_bearer_ev ev = {
		.opcode = opcode,
	};

	LOG_DBG("opcode 0x%02x", opcode);

	tester_send(BTP_SERVICE_ID_MESH, MESH_EV_INVALID_BEARER,
		    CONTROLLER_INDEX, (uint8_t *) &ev, sizeof(ev));
}

static void incomp_timer_exp_cb(void)
{
	tester_send(BTP_SERVICE_ID_MESH, MESH_EV_INCOMP_TIMER_EXP,
		    CONTROLLER_INDEX, NULL, 0);
}

static struct bt_test_cb bt_test_cb = {
	.mesh_net_recv = net_recv_ev,
	.mesh_model_bound = model_bound_cb,
	.mesh_model_unbound = model_unbound_cb,
	.mesh_prov_invalid_bearer = invalid_bearer_cb,
	.mesh_trans_incomp_timer_exp = incomp_timer_exp_cb,
};

static void friend_established(uint16_t net_idx, uint16_t lpn_addr,
			       uint8_t recv_delay, uint32_t polltimeout)
{
	struct mesh_frnd_established_ev ev = { net_idx, lpn_addr, recv_delay,
					       polltimeout };

	LOG_DBG("Friendship (as Friend) established with "
			"LPN 0x%04x Receive Delay %u Poll Timeout %u",
			lpn_addr, recv_delay, polltimeout);


	tester_send(BTP_SERVICE_ID_MESH, MESH_EV_FRND_ESTABLISHED,
		    CONTROLLER_INDEX, (uint8_t *) &ev, sizeof(ev));
}

static void friend_terminated(uint16_t net_idx, uint16_t lpn_addr)
{
	struct mesh_frnd_terminated_ev ev = { net_idx, lpn_addr };

	LOG_DBG("Friendship (as Friend) lost with LPN "
			"0x%04x", lpn_addr);

	tester_send(BTP_SERVICE_ID_MESH, MESH_EV_FRND_TERMINATED,
		    CONTROLLER_INDEX, (uint8_t *) &ev, sizeof(ev));
}

BT_MESH_FRIEND_CB_DEFINE(friend_cb) = {
	.established = friend_established,
	.terminated = friend_terminated,
};

static void lpn_established(uint16_t net_idx, uint16_t friend_addr,
					uint8_t queue_size, uint8_t recv_win)
{
	struct mesh_lpn_established_ev ev = { net_idx, friend_addr, queue_size,
					      recv_win };

	LOG_DBG("Friendship (as LPN) established with "
			"Friend 0x%04x Queue Size %d Receive Window %d",
			friend_addr, queue_size, recv_win);

	tester_send(BTP_SERVICE_ID_MESH, MESH_EV_LPN_ESTABLISHED,
		    CONTROLLER_INDEX, (uint8_t *) &ev, sizeof(ev));
}

static void lpn_terminated(uint16_t net_idx, uint16_t friend_addr)
{
	struct mesh_lpn_polled_ev ev = { net_idx, friend_addr };

	LOG_DBG("Friendship (as LPN) lost with Friend "
			"0x%04x", friend_addr);

	tester_send(BTP_SERVICE_ID_MESH, MESH_EV_LPN_TERMINATED,
		    CONTROLLER_INDEX, (uint8_t *) &ev, sizeof(ev));
}

static void lpn_polled(uint16_t net_idx, uint16_t friend_addr, bool retry)
{
	struct mesh_lpn_polled_ev ev = { net_idx, friend_addr, (uint8_t)retry };

	LOG_DBG("LPN polled 0x%04x %s", friend_addr, retry ? "(retry)" : "");

	tester_send(BTP_SERVICE_ID_MESH, MESH_EV_LPN_POLLED,
		    CONTROLLER_INDEX, (uint8_t *) &ev, sizeof(ev));
}

BT_MESH_LPN_CB_DEFINE(lpn_cb) = {
	.established = lpn_established,
	.terminated = lpn_terminated,
	.polled = lpn_polled,
};

uint8_t tester_init_mesh(void)
{
	if (IS_ENABLED(CONFIG_BT_TESTING)) {
		bt_test_cb_register(&bt_test_cb);
	}

	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_mesh(void)
{
	return BTP_STATUS_SUCCESS;
}
