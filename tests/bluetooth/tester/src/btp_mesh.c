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
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_BTTESTER_LOG_LEVEL);

#include "btp/btp.h"

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


static uint8_t supported_commands(const void *cmd, uint16_t cmd_len,
				  void *rsp, uint16_t *rsp_len)
{
	struct btp_mesh_read_supported_commands_rp *rp = rsp;

	/* octet 0 */
	tester_set_bit(rp->data, BTP_MESH_READ_SUPPORTED_COMMANDS);
	tester_set_bit(rp->data, BTP_MESH_CONFIG_PROVISIONING);
	tester_set_bit(rp->data, BTP_MESH_PROVISION_NODE);
	tester_set_bit(rp->data, BTP_MESH_INIT);
	tester_set_bit(rp->data, BTP_MESH_RESET);
	tester_set_bit(rp->data, BTP_MESH_INPUT_NUMBER);
	tester_set_bit(rp->data, BTP_MESH_INPUT_STRING);

	/* octet 1 */
	tester_set_bit(rp->data, BTP_MESH_IVU_TEST_MODE);
	tester_set_bit(rp->data, BTP_MESH_IVU_TOGGLE_STATE);
	tester_set_bit(rp->data, BTP_MESH_NET_SEND);
	tester_set_bit(rp->data, BTP_MESH_HEALTH_GENERATE_FAULTS);
	tester_set_bit(rp->data, BTP_MESH_HEALTH_CLEAR_FAULTS);
	tester_set_bit(rp->data, BTP_MESH_LPN);
	tester_set_bit(rp->data, BTP_MESH_LPN_POLL);
	tester_set_bit(rp->data, BTP_MESH_MODEL_SEND);

	/* octet 2 */
#if defined(CONFIG_BT_TESTING)
	tester_set_bit(rp->data, BTP_MESH_LPN_SUBSCRIBE);
	tester_set_bit(rp->data, BTP_MESH_LPN_UNSUBSCRIBE);
	tester_set_bit(rp->data, BTP_MESH_RPL_CLEAR);
#endif /* CONFIG_BT_TESTING */
	tester_set_bit(rp->data, BTP_MESH_PROXY_IDENTITY);
	tester_set_bit(rp->data, BTP_MESH_COMP_DATA_GET);
	tester_set_bit(rp->data, BTP_MESH_CFG_BEACON_GET);
	tester_set_bit(rp->data, BTP_MESH_CFG_BEACON_SET);

	/* octet 3 */
	tester_set_bit(rp->data, BTP_MESH_CFG_DEFAULT_TTL_GET);
	tester_set_bit(rp->data, BTP_MESH_CFG_DEFAULT_TTL_SET);
	tester_set_bit(rp->data, BTP_MESH_CFG_GATT_PROXY_GET);
	tester_set_bit(rp->data, BTP_MESH_CFG_GATT_PROXY_SET);
	tester_set_bit(rp->data, BTP_MESH_CFG_FRIEND_GET);
	tester_set_bit(rp->data, BTP_MESH_CFG_FRIEND_SET);
	tester_set_bit(rp->data, BTP_MESH_CFG_RELAY_GET);
	tester_set_bit(rp->data, BTP_MESH_CFG_RELAY_SET);

	/* octet 4 */
	tester_set_bit(rp->data, BTP_MESH_CFG_MODEL_PUB_GET);
	tester_set_bit(rp->data, BTP_MESH_CFG_MODEL_PUB_SET);
	tester_set_bit(rp->data, BTP_MESH_CFG_MODEL_SUB_ADD);
	tester_set_bit(rp->data, BTP_MESH_CFG_MODEL_SUB_DEL);
	tester_set_bit(rp->data, BTP_MESH_CFG_NETKEY_ADD);
	tester_set_bit(rp->data, BTP_MESH_CFG_NETKEY_GET);
	tester_set_bit(rp->data, BTP_MESH_CFG_NETKEY_DEL);
	tester_set_bit(rp->data, BTP_MESH_CFG_APPKEY_ADD);

	/* octet 5 */
	tester_set_bit(rp->data, BTP_MESH_CFG_APPKEY_DEL);
	tester_set_bit(rp->data, BTP_MESH_CFG_APPKEY_GET);
	tester_set_bit(rp->data, BTP_MESH_CFG_MODEL_APP_BIND);
	tester_set_bit(rp->data, BTP_MESH_CFG_MODEL_APP_UNBIND);
	tester_set_bit(rp->data, BTP_MESH_CFG_MODEL_APP_GET);
	tester_set_bit(rp->data, BTP_MESH_CFG_MODEL_APP_VND_GET);
	tester_set_bit(rp->data, BTP_MESH_CFG_HEARTBEAT_PUB_SET);
	tester_set_bit(rp->data, BTP_MESH_CFG_HEARTBEAT_PUB_GET);

	/* octet 6 */
	tester_set_bit(rp->data, BTP_MESH_CFG_HEARTBEAT_SUB_SET);
	tester_set_bit(rp->data, BTP_MESH_CFG_HEARTBEAT_SUB_GET);
	tester_set_bit(rp->data, BTP_MESH_CFG_NET_TRANS_GET);
	tester_set_bit(rp->data, BTP_MESH_CFG_NET_TRANS_SET);
	tester_set_bit(rp->data, BTP_MESH_CFG_MODEL_SUB_OVW);
	tester_set_bit(rp->data, BTP_MESH_CFG_MODEL_SUB_DEL_ALL);
	tester_set_bit(rp->data, BTP_MESH_CFG_MODEL_SUB_GET);
	tester_set_bit(rp->data, BTP_MESH_CFG_MODEL_SUB_GET_VND);

	/* octet 7 */
	tester_set_bit(rp->data, BTP_MESH_CFG_MODEL_SUB_VA_ADD);
	tester_set_bit(rp->data, BTP_MESH_CFG_MODEL_SUB_VA_DEL);
	tester_set_bit(rp->data, BTP_MESH_CFG_MODEL_SUB_VA_OVW);
	tester_set_bit(rp->data, BTP_MESH_CFG_NETKEY_UPDATE);
	tester_set_bit(rp->data, BTP_MESH_CFG_APPKEY_UPDATE);
	tester_set_bit(rp->data, BTP_MESH_CFG_NODE_IDT_SET);
	tester_set_bit(rp->data, BTP_MESH_CFG_NODE_IDT_GET);
	tester_set_bit(rp->data, BTP_MESH_CFG_NODE_RESET);

	/* octet 8 */
	tester_set_bit(rp->data, BTP_MESH_CFG_LPN_TIMEOUT_GET);
	tester_set_bit(rp->data, BTP_MESH_CFG_MODEL_APP_BIND_VND);
	tester_set_bit(rp->data, BTP_MESH_HEALTH_FAULT_GET);
	tester_set_bit(rp->data, BTP_MESH_HEALTH_FAULT_CLEAR);
	tester_set_bit(rp->data, BTP_MESH_HEALTH_PERIOD_GET);
	tester_set_bit(rp->data, BTP_MESH_HEALTH_PERIOD_SET);

	/* octet 9 */
	tester_set_bit(rp->data, BTP_MESH_HEALTH_ATTENTION_GET);
	tester_set_bit(rp->data, BTP_MESH_HEALTH_ATTENTION_SET);
	tester_set_bit(rp->data, BTP_MESH_PROVISION_ADV);
	tester_set_bit(rp->data, BTP_MESH_CFG_KRP_GET);
	tester_set_bit(rp->data, BTP_MESH_CFG_KRP_SET);

	*rsp_len = sizeof(*rp) + 10;

	return BTP_STATUS_SUCCESS;
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
	struct btp_mesh_prov_link_open_ev ev;

	LOG_DBG("bearer 0x%02x", bearer);

	switch (bearer) {
	case BT_MESH_PROV_ADV:
		ev.bearer = BTP_MESH_PROV_BEARER_PB_ADV;
		break;
	case BT_MESH_PROV_GATT:
		ev.bearer = BTP_MESH_PROV_BEARER_PB_GATT;
		break;
	default:
		LOG_ERR("Invalid bearer");

		return;
	}

	tester_event(BTP_SERVICE_ID_MESH, BTP_MESH_EV_PROV_LINK_OPEN, &ev, sizeof(ev));
}

static void link_close(bt_mesh_prov_bearer_t bearer)
{
	struct btp_mesh_prov_link_closed_ev ev;

	LOG_DBG("bearer 0x%02x", bearer);

	switch (bearer) {
	case BT_MESH_PROV_ADV:
		ev.bearer = BTP_MESH_PROV_BEARER_PB_ADV;
		break;
	case BT_MESH_PROV_GATT:
		ev.bearer = BTP_MESH_PROV_BEARER_PB_GATT;
		break;
	default:
		LOG_ERR("Invalid bearer");

		return;
	}

	tester_event(BTP_SERVICE_ID_MESH, BTP_MESH_EV_PROV_LINK_CLOSED, &ev, sizeof(ev));
}

static int output_number(bt_mesh_output_action_t action, uint32_t number)
{
	struct btp_mesh_out_number_action_ev ev;

	LOG_DBG("action 0x%04x number 0x%08x", action, number);

	ev.action = sys_cpu_to_le16(action);
	ev.number = sys_cpu_to_le32(number);

	tester_event(BTP_SERVICE_ID_MESH, BTP_MESH_EV_OUT_NUMBER_ACTION, &ev, sizeof(ev));

	return 0;
}

static int output_string(const char *str)
{
	struct btp_mesh_out_string_action_ev *ev;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(BTP_DATA_MAX_SIZE);

	LOG_DBG("str %s", str);

	net_buf_simple_init(buf, 0);

	ev = net_buf_simple_add(buf, sizeof(*ev));
	ev->string_len = strlen(str);

	net_buf_simple_add_mem(buf, str, ev->string_len);

	tester_event(BTP_SERVICE_ID_MESH, BTP_MESH_EV_OUT_STRING_ACTION,
		    buf->data, buf->len);

	return 0;
}

static int input(bt_mesh_input_action_t action, uint8_t size)
{
	struct btp_mesh_in_action_ev ev;

	LOG_DBG("action 0x%04x number 0x%02x", action, size);

	input_size = size;

	ev.action = sys_cpu_to_le16(action);
	ev.size = size;

	tester_event(BTP_SERVICE_ID_MESH, BTP_MESH_EV_IN_ACTION, &ev, sizeof(ev));

	return 0;
}

static void prov_complete(uint16_t net_idx, uint16_t addr)
{
	LOG_DBG("net_idx 0x%04x addr 0x%04x", net_idx, addr);

	net.net_idx = net_idx,
	net.local = addr;
	net.dst = addr;

	tester_event(BTP_SERVICE_ID_MESH, BTP_MESH_EV_PROVISIONED, NULL, 0);
}

static void prov_node_added(uint16_t net_idx, uint8_t uuid[16], uint16_t addr,
			    uint8_t num_elem)
{
	struct btp_mesh_prov_node_added_ev ev;

	LOG_DBG("net_idx 0x%04x addr 0x%04x num_elem %d", net_idx, addr,
		num_elem);

	ev.net_idx = net_idx;
	ev.addr = addr;
	ev.num_elems = num_elem;
	memcpy(&ev.uuid, uuid, sizeof(ev.uuid));

	tester_event(BTP_SERVICE_ID_MESH, BTP_MESH_EV_PROV_NODE_ADDED, &ev, sizeof(ev));
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

static uint8_t config_prov(const void *cmd, uint16_t cmd_len,
			   void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_config_provisioning_cmd *cp = cmd;
	const struct btp_mesh_config_provisioning_cmd_v2 *cp2 = cmd;
	int err = 0;

	/* TODO consider fix BTP commands to avoid this */
	if (cmd_len != sizeof(*cp) && cmd_len != (sizeof(*cp2))) {
		return BTP_STATUS_FAILED;
	}

	LOG_DBG("");

	memcpy(dev_uuid, cp->uuid, sizeof(dev_uuid));
	memcpy(static_auth, cp->static_auth, sizeof(static_auth));

	prov.output_size = cp->out_size;
	prov.output_actions = sys_le16_to_cpu(cp->out_actions);
	prov.input_size = cp->in_size;
	prov.input_actions = sys_le16_to_cpu(cp->in_actions);

	if (cmd_len == sizeof(*cp2)) {
		memcpy(pub_key, cp2->set_pub_key, sizeof(cp2->set_pub_key));
		memcpy(priv_key, cp2->set_priv_key, sizeof(cp2->set_priv_key));
		prov.public_key_be = pub_key;
		prov.private_key_be = priv_key;
	}

	if (cp->auth_method == AUTH_METHOD_OUTPUT) {
		err = bt_mesh_auth_method_set_output(prov.output_actions, prov.output_size);
	} else if (cp->auth_method == AUTH_METHOD_INPUT) {
		err = bt_mesh_auth_method_set_input(prov.input_actions, prov.input_size);
	} else if (cp->auth_method == AUTH_METHOD_STATIC) {
		err = bt_mesh_auth_method_set_static(static_auth, sizeof(static_auth));
	}

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t provision_node(const void *cmd, uint16_t cmd_len,
			      void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_provision_node_cmd *cp = cmd;
	const struct btp_mesh_provision_node_cmd_v2 *cp2 = cmd;
	int err;

	/* TODO consider fix BTP commands to avoid this */
	if (cmd_len != sizeof(*cp) && cmd_len != (sizeof(*cp2))) {
		return BTP_STATUS_FAILED;
	}

	LOG_DBG("");

	memcpy(dev_key, cp->dev_key, sizeof(dev_key));
	memcpy(net_key, cp->net_key, sizeof(net_key));

	addr = sys_le16_to_cpu(cp->addr);
	flags = cp->flags;
	iv_index = sys_le32_to_cpu(cp->iv_index);
	net_key_idx = sys_le16_to_cpu(cp->net_key_idx);

	if (cmd_len == sizeof(*cp2)) {
		memcpy(pub_key, cp2->pub_key, sizeof(pub_key));

		err = bt_mesh_prov_remote_pub_key_set(pub_key);
		if (err) {
			LOG_ERR("err %d", err);
			return BTP_STATUS_FAILED;
		}
	}
#if defined(CONFIG_BT_MESH_PROVISIONER)
	err = bt_mesh_cdb_create(net_key);
	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}
#endif
	err = bt_mesh_provision(net_key, net_key_idx, flags, iv_index, addr,
				dev_key);
	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t provision_adv(const void *cmd, uint16_t cmd_len,
			     void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_provision_adv_cmd *cp = cmd;
	int err;

	LOG_DBG("");

	err = bt_mesh_provision_adv(cp->uuid, sys_le16_to_cpu(cp->net_idx),
				    sys_le16_to_cpu(cp->address),
				    cp->attention_duration);
	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t init(const void *cmd, uint16_t cmd_len,
		    void *rsp, uint16_t *rsp_len)
{
	int err;

	LOG_DBG("");

	err = bt_mesh_init(&prov, &comp);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	if (addr) {
		err = bt_mesh_provision(net_key, net_key_idx, flags, iv_index,
					addr, dev_key);
		if (err) {
			return BTP_STATUS_FAILED;
		}
	} else {
		err = bt_mesh_prov_enable(BT_MESH_PROV_ADV | BT_MESH_PROV_GATT);
		if (err) {
			return BTP_STATUS_FAILED;
		}
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t reset(const void *cmd, uint16_t cmd_len,
		     void *rsp, uint16_t *rsp_len)
{
	LOG_DBG("");

	bt_mesh_reset();

	return BTP_STATUS_SUCCESS;
}

static uint8_t input_number(const void *cmd, uint16_t cmd_len,
			    void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_input_number_cmd *cp = cmd;
	uint32_t number;
	int err;

	number = sys_le32_to_cpu(cp->number);

	LOG_DBG("number 0x%04x", number);

	err = bt_mesh_input_number(number);
	if (err) {
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t input_string(const void *cmd, uint16_t cmd_len,
			    void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_input_string_cmd *cp = cmd;
	uint8_t status = BTP_STATUS_SUCCESS;
	int err;

	LOG_DBG("");

	if (cmd_len < sizeof(*cp) &&
	    cmd_len != (sizeof(*cp) + cp->string_len)) {
		return BTP_STATUS_FAILED;
	}

	/* for historical reasons this commands must send NULL terminated
	 * string
	 */
	if (cp->string[cp->string_len] != '\0') {
		return BTP_STATUS_FAILED;
	}

	if (strlen(cp->string) < input_size) {
		LOG_ERR("Too short input (%u chars required)", input_size);
		return BTP_STATUS_FAILED;
	}

	err = bt_mesh_input_string(cp->string);
	if (err) {
		status = BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t ivu_test_mode(const void *cmd, uint16_t cmd_len,
			     void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_ivu_test_mode_cmd *cp = cmd;

	LOG_DBG("enable 0x%02x", cp->enable);

	bt_mesh_iv_update_test(cp->enable ? true : false);

	return BTP_STATUS_SUCCESS;
}

static uint8_t ivu_toggle_state(const void *cmd, uint16_t cmd_len,
				void *rsp, uint16_t *rsp_len)
{
	bool result;

	LOG_DBG("");

	result = bt_mesh_iv_update();
	if (!result) {
		LOG_ERR("Failed to toggle the IV Update state");
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t lpn(const void *cmd, uint16_t cmd_len,
		   void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_lpn_set_cmd *cp = cmd;
	int err;

	LOG_DBG("enable 0x%02x", cp->enable);

	err = bt_mesh_lpn_set(cp->enable ? true : false);
	if (err) {
		LOG_ERR("Failed to toggle LPN (err %d)", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t lpn_poll(const void *cmd, uint16_t cmd_len,
			void *rsp, uint16_t *rsp_len)
{
	int err;

	LOG_DBG("");

	err = bt_mesh_lpn_poll();
	if (err) {
		LOG_ERR("Failed to send poll msg (err %d)", err);
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t net_send(const void *cmd, uint16_t cmd_len,
			void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_net_send_cmd *cp = cmd;
	NET_BUF_SIMPLE_DEFINE(msg, UINT8_MAX);
	int err;

	if (cmd_len < sizeof(*cp) &&
	    cmd_len != (sizeof(*cp) + cp->payload_len)) {
		return BTP_STATUS_FAILED;
	}

	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = vnd_app_key_idx,
		.addr = sys_le16_to_cpu(cp->dst),
		.send_ttl = cp->ttl,
	};

	LOG_DBG("ttl 0x%02x dst 0x%04x payload_len %d", ctx.send_ttl,
		ctx.addr, cp->payload_len);

	if (!bt_mesh_app_key_exists(vnd_app_key_idx)) {
		(void)bt_mesh_app_key_add(vnd_app_key_idx, net.net_idx,
					  vnd_app_key);
		vnd_models[0].keys[0] = vnd_app_key_idx;
	}

	net_buf_simple_add_mem(&msg, cp->payload, cp->payload_len);

	err = bt_mesh_model_send(&vnd_models[0], &ctx, &msg, NULL, NULL);
	if (err) {
		LOG_ERR("Failed to send (err %d)", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t health_generate_faults(const void *cmd, uint16_t cmd_len,
				      void *rsp, uint16_t *rsp_len)
{
	struct btp_mesh_health_generate_faults_rp *rp = rsp;
	uint8_t some_faults[] = { 0x01, 0x02, 0x03, 0xff, 0x06 };
	uint8_t cur_faults_count;
	uint8_t reg_faults_count;

	cur_faults_count = MIN(sizeof(cur_faults), sizeof(some_faults));
	memcpy(cur_faults, some_faults, cur_faults_count);
	memcpy(rp->current_faults, cur_faults, cur_faults_count);
	rp->cur_faults_count = cur_faults_count;

	reg_faults_count = MIN(sizeof(reg_faults), sizeof(some_faults));
	memcpy(reg_faults, some_faults, reg_faults_count);
	memcpy(rp->registered_faults + cur_faults_count, reg_faults, reg_faults_count);
	rp->reg_faults_count = reg_faults_count;

	bt_mesh_health_srv_fault_update(&elements[0]);

	*rsp_len = sizeof(*rp) + cur_faults_count + reg_faults_count;

	return BTP_STATUS_SUCCESS;
}

static uint8_t health_clear_faults(const void *cmd, uint16_t cmd_len,
				   void *rsp, uint16_t *rsp_len)
{
	LOG_DBG("");

	(void)memset(cur_faults, 0, sizeof(cur_faults));
	(void)memset(reg_faults, 0, sizeof(reg_faults));

	bt_mesh_health_srv_fault_update(&elements[0]);

	return BTP_STATUS_SUCCESS;
}

static uint8_t model_send(const void *cmd, uint16_t cmd_len,
			  void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_model_send_cmd *cp = cmd;
	NET_BUF_SIMPLE_DEFINE(msg, UINT8_MAX);
	struct bt_mesh_model *model = NULL;
	uint16_t src;
	int err;

	if (cmd_len < sizeof(*cp) &&
	    cmd_len != (sizeof(*cp) + cp->payload_len)) {
		return BTP_STATUS_FAILED;
	}

	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.addr = sys_le16_to_cpu(cp->dst),
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};

	src = sys_le16_to_cpu(cp->src);

	/* Lookup source address */
	for (int i = 0; i < ARRAY_SIZE(model_bound); i++) {
		if (bt_mesh_model_elem(model_bound[i].model)->addr == src) {
			model = model_bound[i].model;
			ctx.app_idx = model_bound[i].appkey_idx;

			break;
		}
	}

	if (!model) {
		LOG_ERR("Model not found");
		return BTP_STATUS_FAILED;
	}

	LOG_DBG("src 0x%04x dst 0x%04x model %p payload_len %d", src,
		ctx.addr, model, cp->payload_len);

	net_buf_simple_add_mem(&msg, cp->payload, cp->payload_len);

	err = bt_mesh_model_send(model, &ctx, &msg, NULL, NULL);
	if (err) {
		LOG_ERR("Failed to send (err %d)", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

#if defined(CONFIG_BT_TESTING)
static uint8_t lpn_subscribe(const void *cmd, uint16_t cmd_len,
			     void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_lpn_subscribe_cmd *cp = cmd;
	uint16_t address = sys_le16_to_cpu(cp->address);
	int err;

	LOG_DBG("address 0x%04x", address);

	err = bt_test_mesh_lpn_group_add(address);
	if (err) {
		LOG_ERR("Failed to subscribe (err %d)", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t lpn_unsubscribe(const void *cmd, uint16_t cmd_len,
			       void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_lpn_unsubscribe_cmd *cp = cmd;
	uint16_t address = sys_le16_to_cpu(cp->address);
	int err;

	LOG_DBG("address 0x%04x", address);

	err = bt_test_mesh_lpn_group_remove(&address, 1);
	if (err) {
		LOG_ERR("Failed to unsubscribe (err %d)", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t rpl_clear(const void *cmd, uint16_t cmd_len,
			 void *rsp, uint16_t *rsp_len)
{
	int err;

	LOG_DBG("");

	err = bt_test_mesh_rpl_clear();
	if (err) {
		LOG_ERR("Failed to clear RPL (err %d)", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}
#endif /* CONFIG_BT_TESTING */

static uint8_t proxy_identity_enable(const void *cmd, uint16_t cmd_len,
				     void *rsp, uint16_t *rsp_len)
{
	int err;

	LOG_DBG("");

	err = bt_mesh_proxy_identity_enable();
	if (err) {
		LOG_ERR("Failed to enable proxy identity (err %d)", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t composition_data_get(const void *cmd, uint16_t cmd_len,
				    void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_comp_data_get_cmd *cp = cmd;
	struct btp_mesh_comp_data_get_rp *rp = rsp;
	uint8_t page;
	struct net_buf_simple *comp = NET_BUF_SIMPLE(128);
	int err;

	LOG_DBG("");

	bt_mesh_cfg_cli_timeout_set(10 * MSEC_PER_SEC);

	net_buf_simple_init(comp, 0);

	err = bt_mesh_cfg_cli_comp_data_get(sys_le16_to_cpu(cp->net_idx),
					    sys_le16_to_cpu(cp->address),
					    cp->page, &page, comp);
	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	memcpy(rp->data, comp->data, comp->len);
	*rsp_len = comp->len;

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_krp_get(const void *cmd, uint16_t cmd_len,
			      void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_krp_get_cmd *cp = cmd;
	struct btp_mesh_cfg_krp_get_rp *rp = rsp;
	uint8_t status;
	uint8_t phase;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_krp_get(sys_le16_to_cpu(cp->net_idx),
				      sys_le16_to_cpu(cp->address),
				      sys_le16_to_cpu(cp->key_net_idx),
				      &status, &phase);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	rp->phase = phase;

	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_krp_set(const void *cmd, uint16_t cmd_len,
			      void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_krp_set_cmd *cp = cmd;
	struct btp_mesh_cfg_krp_set_rp *rp = rsp;
	uint8_t status;
	uint8_t phase;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_krp_set(sys_le16_to_cpu(cp->net_idx),
				      sys_le16_to_cpu(cp->address),
				      sys_le16_to_cpu(cp->key_net_idx),
				      cp->transition, &status, &phase);
	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	rp->phase = phase;

	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_beacon_get(const void *cmd, uint16_t cmd_len,
				 void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_beacon_get_cmd *cp = cmd;
	struct btp_mesh_cfg_beacon_get_rp *rp = rsp;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_beacon_get(sys_le16_to_cpu(cp->net_idx),
					 sys_le16_to_cpu(cp->address), &status);
	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_beacon_set(const void *cmd, uint16_t cmd_len,
				 void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_beacon_set_cmd *cp = cmd;
	struct btp_mesh_cfg_beacon_set_rp *rp = rsp;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_beacon_set(sys_le16_to_cpu(cp->net_idx),
					 sys_le16_to_cpu(cp->address), cp->val,
					 &status);
	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_default_ttl_get(const void *cmd, uint16_t cmd_len,
				      void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_default_ttl_get_cmd *cp = cmd;
	struct btp_mesh_cfg_default_ttl_get_rp *rp = rsp;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_ttl_get(sys_le16_to_cpu(cp->net_idx),
				      sys_le16_to_cpu(cp->address), &status);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_default_ttl_set(const void *cmd, uint16_t cmd_len,
				      void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_default_ttl_set_cmd *cp = cmd;
	struct btp_mesh_cfg_default_ttl_set_rp *rp = rsp;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_ttl_set(sys_le16_to_cpu(cp->net_idx),
				      sys_le16_to_cpu(cp->address), cp->val,
				      &status);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_gatt_proxy_get(const void *cmd, uint16_t cmd_len,
				     void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_gatt_proxy_get_cmd *cp = cmd;
	struct btp_mesh_cfg_gatt_proxy_get_rp *rp = rsp;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_gatt_proxy_get(sys_le16_to_cpu(cp->net_idx),
					     sys_le16_to_cpu(cp->address),
					     &status);
	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_gatt_proxy_set(const void *cmd, uint16_t cmd_len,
				     void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_gatt_proxy_set_cmd *cp = cmd;
	struct btp_mesh_cfg_gatt_proxy_set_rp *rp = rsp;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_gatt_proxy_set(sys_le16_to_cpu(cp->net_idx),
					     sys_le16_to_cpu(cp->address),
					     cp->val, &status);
	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_friend_get(const void *cmd, uint16_t cmd_len,
				 void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_friend_get_cmd *cp = cmd;
	struct btp_mesh_cfg_friend_get_rp *rp = rsp;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_friend_get(sys_le16_to_cpu(cp->net_idx),
					 sys_le16_to_cpu(cp->address),
					 &status);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_friend_set(const void *cmd, uint16_t cmd_len,
				 void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_friend_set_cmd *cp = cmd;
	struct btp_mesh_cfg_friend_set_rp *rp = rsp;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_friend_set(sys_le16_to_cpu(cp->net_idx),
					 sys_le16_to_cpu(cp->address),
					 cp->val, &status);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_relay_get(const void *cmd, uint16_t cmd_len,
				void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_relay_get_cmd *cp = cmd;
	struct btp_mesh_cfg_relay_get_rp *rp = rsp;
	uint8_t status;
	uint8_t transmit;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_relay_get(sys_le16_to_cpu(cp->net_idx),
					sys_le16_to_cpu(cp->address), &status,
					&transmit);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_relay_set(const void *cmd, uint16_t cmd_len,
				void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_relay_set_cmd *cp = cmd;
	struct btp_mesh_cfg_relay_set_rp *rp = rsp;
	uint8_t status;
	uint8_t transmit;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_relay_set(sys_le16_to_cpu(cp->net_idx),
					sys_le16_to_cpu(cp->address),
					cp->new_relay, cp->new_transmit,
					&status, &transmit);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_mod_pub_get(const void *cmd, uint16_t cmd_len,
				  void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_model_pub_get_cmd *cp = cmd;
	struct btp_mesh_cfg_model_pub_get_rp *rp = rsp;
	struct bt_mesh_cfg_cli_mod_pub pub;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_mod_pub_get(sys_le16_to_cpu(cp->net_idx),
					  sys_le16_to_cpu(cp->address),
					  sys_le16_to_cpu(cp->elem_address),
					  sys_le16_to_cpu(cp->model_id),
					  &pub, &status);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_mod_pub_set(const void *cmd, uint16_t cmd_len,
				  void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_model_pub_set_cmd *cp = cmd;
	struct btp_mesh_cfg_model_pub_set_rp *rp = rsp;
	struct bt_mesh_cfg_cli_mod_pub pub;
	uint8_t status;
	int err;

	LOG_DBG("");

	pub.addr = sys_le16_to_cpu(cp->pub_addr);
	pub.uuid = NULL;
	pub.app_idx = sys_le16_to_cpu(cp->app_idx);
	pub.cred_flag = cp->cred_flag;
	pub.ttl = cp->ttl;
	pub.period = cp->period;
	pub.transmit = cp->transmit;

	err = bt_mesh_cfg_cli_mod_pub_set(sys_le16_to_cpu(cp->net_idx),
					  sys_le16_to_cpu(cp->address),
					  sys_le16_to_cpu(cp->elem_address),
					  sys_le16_to_cpu(cp->model_id),
					  &pub, &status);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_mod_pub_va_set(const void *cmd, uint16_t cmd_len,
				     void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_model_pub_va_set_cmd *cp = cmd;
	struct btp_mesh_cfg_model_pub_va_set_rp *rp = rsp;
	uint8_t status;
	struct bt_mesh_cfg_cli_mod_pub pub;
	int err;

	LOG_DBG("");

	pub.uuid = cp->uuid;
	pub.app_idx = sys_le16_to_cpu(cp->app_idx);
	pub.cred_flag = cp->cred_flag;
	pub.ttl = cp->ttl;
	pub.period = cp->period;
	pub.transmit = cp->transmit;

	err = bt_mesh_cfg_cli_mod_pub_set(sys_le16_to_cpu(cp->net_idx),
					  sys_le16_to_cpu(cp->address),
					  sys_le16_to_cpu(cp->elem_address),
					  sys_le16_to_cpu(cp->model_id),
					  &pub, &status);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_mod_sub_add(const void *cmd, uint16_t cmd_len,
				  void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_model_sub_add_cmd *cp = cmd;
	struct btp_mesh_cfg_model_sub_add_rp *rp = rsp;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_mod_sub_add(sys_le16_to_cpu(cp->net_idx),
					  sys_le16_to_cpu(cp->address),
					  sys_le16_to_cpu(cp->elem_address),
					  sys_le16_to_cpu(cp->sub_addr),
					  sys_le16_to_cpu(cp->model_id),
					  &status);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_mod_sub_ovw(const void *cmd, uint16_t cmd_len,
				  void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_model_sub_ovw_cmd *cp = cmd;
	struct btp_mesh_cfg_model_sub_add_rp *rp = rsp;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_mod_sub_overwrite(sys_le16_to_cpu(cp->net_idx),
						sys_le16_to_cpu(cp->address),
						sys_le16_to_cpu(cp->elem_address),
						sys_le16_to_cpu(cp->sub_addr),
						sys_le16_to_cpu(cp->model_id),
						&status);
	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_mod_sub_del(const void *cmd, uint16_t cmd_len,
				  void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_model_sub_del_cmd *cp = cmd;
	struct btp_mesh_cfg_model_sub_del_rp *rp = rsp;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_mod_sub_del(sys_le16_to_cpu(cp->net_idx),
					  sys_le16_to_cpu(cp->address),
					  sys_le16_to_cpu(cp->elem_address),
					  sys_le16_to_cpu(cp->sub_addr),
					  sys_le16_to_cpu(cp->model_id),
					  &status);
	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_mod_sub_del_all(const void *cmd, uint16_t cmd_len,
				      void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_model_sub_del_all_cmd *cp = cmd;
	struct btp_mesh_cfg_model_sub_del_all_rp *rp = rsp;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_mod_sub_del_all(sys_le16_to_cpu(cp->net_idx),
					      sys_le16_to_cpu(cp->address),
					      sys_le16_to_cpu(cp->elem_address),
					      sys_le16_to_cpu(cp->model_id),
					      &status);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_mod_sub_get(const void *cmd, uint16_t cmd_len,
				  void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_model_sub_get_cmd *cp = cmd;
	struct btp_mesh_cfg_model_sub_get_rp *rp = rsp;
	uint8_t status;
	int16_t subs;
	size_t sub_cn;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_mod_sub_get(sys_le16_to_cpu(cp->net_idx),
					  sys_le16_to_cpu(cp->address),
					  sys_le16_to_cpu(cp->elem_address),
					  sys_le16_to_cpu(cp->model_id),
					  &status, &subs, &sub_cn);
	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_mod_sub_get_vnd(const void *cmd, uint16_t cmd_len,
				      void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_model_sub_get_vnd_cmd *cp = cmd;
	struct btp_mesh_cfg_model_sub_get_vnd_rp *rp = rsp;
	uint8_t status;
	uint16_t subs;
	size_t sub_cn;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_mod_sub_get_vnd(sys_le16_to_cpu(cp->net_idx),
					      sys_le16_to_cpu(cp->address),
					      sys_le16_to_cpu(cp->elem_address),
					      sys_le16_to_cpu(cp->model_id),
					      sys_le16_to_cpu(cp->cid),
					      &status, &subs, &sub_cn);
	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_mod_sub_va_add(const void *cmd, uint16_t cmd_len,
				     void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_model_sub_va_add_cmd *cp = cmd;
	struct btp_mesh_cfg_model_sub_va_add_rp *rp = rsp;
	uint8_t status;
	uint16_t virt_addr_rcv;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_mod_sub_va_add(sys_le16_to_cpu(cp->net_idx),
					     sys_le16_to_cpu(cp->address),
					     sys_le16_to_cpu(cp->elem_address),
					     sys_le16_to_cpu(cp->uuid),
					     sys_le16_to_cpu(cp->model_id),
					     &virt_addr_rcv, &status);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_mod_sub_va_del(const void *cmd, uint16_t cmd_len,
				     void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_model_sub_va_del_cmd *cp = cmd;
	struct btp_mesh_cfg_model_sub_va_del_rp *rp = rsp;
	uint8_t status;
	uint16_t virt_addr_rcv;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_mod_sub_va_del(sys_le16_to_cpu(cp->net_idx),
					     sys_le16_to_cpu(cp->address),
					     sys_le16_to_cpu(cp->elem_address),
					     sys_le16_to_cpu(cp->uuid),
					     sys_le16_to_cpu(cp->model_id),
					     &virt_addr_rcv, &status);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_mod_sub_va_ovw(const void *cmd, uint16_t cmd_len,
				     void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_model_sub_va_ovw_cmd *cp = cmd;
	struct btp_mesh_cfg_model_sub_va_ovw_rp *rp = rsp;
	uint8_t status;
	uint16_t virt_addr_rcv;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_mod_sub_va_overwrite(sys_le16_to_cpu(cp->net_idx),
						   sys_le16_to_cpu(cp->address),
						   sys_le16_to_cpu(cp->elem_address),
						   sys_le16_to_cpu(cp->uuid),
						   sys_le16_to_cpu(cp->model_id),
						   &virt_addr_rcv, &status);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_netkey_add(const void *cmd, uint16_t cmd_len,
				 void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_netkey_add_cmd *cp = cmd;
	struct btp_mesh_cfg_netkey_add_rp *rp = rsp;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_net_key_add(sys_le16_to_cpu(cp->net_idx),
					  sys_le16_to_cpu(cp->address),
					  sys_le16_to_cpu(cp->net_key_idx),
					  cp->net_key, &status);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_netkey_update(const void *cmd, uint16_t cmd_len,
				    void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_netkey_update_cmd *cp = cmd;
	struct btp_mesh_cfg_netkey_update_rp *rp = rsp;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_net_key_update(sys_le16_to_cpu(cp->net_idx),
					     sys_le16_to_cpu(cp->address),
					     sys_le16_to_cpu(cp->net_key_idx),
					     cp->net_key,
					     &status);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_netkey_get(const void *cmd, uint16_t cmd_len,
				 void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_netkey_get_cmd *cp = cmd;
	struct btp_mesh_cfg_netkey_get_rp *rp = rsp;
	size_t key_cnt = 1;
	uint16_t keys;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_net_key_get(sys_le16_to_cpu(cp->net_idx),
					  sys_le16_to_cpu(cp->address),
					  &keys, &key_cnt);
	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	/* for historical reasons this command has status in response */
	rp->status = 0;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_netkey_del(const void *cmd, uint16_t cmd_len,
				 void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_netkey_del_cmd *cp = cmd;
	struct btp_mesh_cfg_netkey_del_rp *rp = rsp;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_net_key_del(sys_le16_to_cpu(cp->net_idx),
					  sys_le16_to_cpu(cp->address),
					  sys_le16_to_cpu(cp->net_key_idx),
					  &status);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_appkey_add(const void *cmd, uint16_t cmd_len,
				 void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_appkey_add_cmd *cp = cmd;
	struct btp_mesh_cfg_appkey_add_rp *rp = rsp;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_app_key_add(sys_le16_to_cpu(cp->net_idx),
					  sys_le16_to_cpu(cp->address),
					  sys_le16_to_cpu(cp->net_key_idx),
					  sys_le16_to_cpu(cp->app_key_idx),
					  sys_le16_to_cpu(cp->app_key),
					  &status);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_appkey_update(const void *cmd, uint16_t cmd_len,
				    void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_appkey_update_cmd *cp = cmd;
	struct btp_mesh_cfg_appkey_update_rp *rp = rsp;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_app_key_update(sys_le16_to_cpu(cp->net_idx),
					     sys_le16_to_cpu(cp->address),
					     sys_le16_to_cpu(cp->net_key_idx),
					     sys_le16_to_cpu(cp->app_key_idx),
					     sys_le16_to_cpu(cp->app_key),
					     &status);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_appkey_del(const void *cmd, uint16_t cmd_len,
				 void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_appkey_del_cmd *cp = cmd;
	struct btp_mesh_cfg_appkey_del_rp *rp = rsp;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_app_key_del(sys_le16_to_cpu(cp->net_idx),
					  sys_le16_to_cpu(cp->address),
					  sys_le16_to_cpu(cp->net_key_idx),
					  sys_le16_to_cpu(cp->app_key_idx),
					  &status);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_appkey_get(const void *cmd, uint16_t cmd_len,
				 void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_appkey_get_cmd *cp = cmd;
	struct btp_mesh_cfg_appkey_get_rp *rp = rsp;
	uint8_t status;
	uint16_t keys;
	size_t key_cnt = 1;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_app_key_get(sys_le16_to_cpu(cp->net_idx),
					  sys_le16_to_cpu(cp->address),
					  sys_le16_to_cpu(cp->net_key_idx),
					  &status, &keys, &key_cnt);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}


static uint8_t config_model_app_bind(const void *cmd, uint16_t cmd_len,
				 void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_model_app_bind_cmd *cp = cmd;
	struct btp_mesh_cfg_model_app_bind_rp *rp = rsp;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_mod_app_bind(sys_le16_to_cpu(cp->net_idx),
					   sys_le16_to_cpu(cp->address),
					   sys_le16_to_cpu(cp->elem_address),
					   sys_le16_to_cpu(cp->app_key_idx),
					   sys_le16_to_cpu(cp->mod_id),
					   &status);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_model_app_bind_vnd(const void *cmd, uint16_t cmd_len,
					 void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_model_app_bind_vnd_cmd *cp = cmd;
	struct btp_mesh_cfg_model_app_bind_vnd_rp *rp = rsp;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_mod_app_bind_vnd(sys_le16_to_cpu(cp->net_idx),
					       sys_le16_to_cpu(cp->address),
					       sys_le16_to_cpu(cp->elem_address),
					       sys_le16_to_cpu(cp->app_key_idx),
					       sys_le16_to_cpu(cp->mod_id),
					       sys_le16_to_cpu(cp->cid),
					       &status);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_model_app_unbind(const void *cmd, uint16_t cmd_len,
				       void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_model_app_unbind_cmd *cp = cmd;
	struct btp_mesh_cfg_model_app_unbind_rp *rp = rsp;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_mod_app_unbind(sys_le16_to_cpu(cp->net_idx),
					     sys_le16_to_cpu(cp->address),
					     sys_le16_to_cpu(cp->elem_address),
					     sys_le16_to_cpu(cp->app_key_idx),
					     sys_le16_to_cpu(cp->mod_id),
					     &status);
	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}


static uint8_t config_model_app_get(const void *cmd, uint16_t cmd_len,
				    void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_model_app_get_cmd *cp = cmd;
	struct btp_mesh_cfg_model_app_get_rp *rp = rsp;
	uint8_t status;
	uint16_t apps;
	size_t app_cnt;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_mod_app_get(sys_le16_to_cpu(cp->net_idx),
					  sys_le16_to_cpu(cp->address),
					  sys_le16_to_cpu(cp->elem_address),
					  sys_le16_to_cpu(cp->mod_id),
					  &status, &apps, &app_cnt);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_model_app_vnd_get(const void *cmd, uint16_t cmd_len,
				    void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_model_app_vnd_get_cmd *cp = cmd;
	struct btp_mesh_cfg_model_app_vnd_get_rp *rp = rsp;
	uint8_t status;
	uint16_t apps;
	size_t app_cnt;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_mod_app_get_vnd(sys_le16_to_cpu(cp->net_idx),
					      sys_le16_to_cpu(cp->address),
					      sys_le16_to_cpu(cp->elem_address),
					      sys_le16_to_cpu(cp->mod_id),
					      sys_le16_to_cpu(cp->cid),
					      &status, &apps, &app_cnt);
	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_hb_pub_set(const void *cmd, uint16_t cmd_len,
				 void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_heartbeat_pub_set_cmd *cp = cmd;
	struct btp_mesh_cfg_heartbeat_pub_set_rp *rp = rsp;
	uint8_t status;
	struct bt_mesh_cfg_cli_hb_pub pub;
	int err;

	LOG_DBG("");

	pub.net_idx = sys_le16_to_cpu(cp->net_key_idx);
	pub.dst = sys_le16_to_cpu(cp->destination);
	pub.count = cp->count_log;
	pub.period = cp->period_log;
	pub.ttl = cp->ttl;
	pub.feat = sys_le16_to_cpu(cp->features);

	err = bt_mesh_cfg_cli_hb_pub_set(sys_le16_to_cpu(cp->net_idx),
					 sys_le16_to_cpu(cp->address),
					 &pub, &status);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_hb_pub_get(const void *cmd, uint16_t cmd_len,
				 void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_heartbeat_pub_get_cmd *cp = cmd;
	struct btp_mesh_cfg_heartbeat_pub_get_rp *rp = rsp;
	uint8_t status;
	struct bt_mesh_cfg_cli_hb_pub pub;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_hb_pub_get(sys_le16_to_cpu(cp->net_idx),
					 sys_le16_to_cpu(cp->address),
					 &pub, &status);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_hb_sub_set(const void *cmd, uint16_t cmd_len,
				 void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_heartbeat_sub_set_cmd *cp = cmd;
	struct btp_mesh_cfg_heartbeat_sub_set_rp *rp = rsp;
	uint8_t status;
	struct bt_mesh_cfg_cli_hb_sub sub;
	int err;

	LOG_DBG("");

	sub.src = sys_le16_to_cpu(cp->source);
	sub.dst = sys_le16_to_cpu(cp->destination);
	sub.period = cp->period_log;

	err = bt_mesh_cfg_cli_hb_sub_set(sys_le16_to_cpu(cp->net_idx),
					 sys_le16_to_cpu(cp->address),
					 &sub, &status);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_hb_sub_get(const void *cmd, uint16_t cmd_len,
				 void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_heartbeat_sub_get_cmd *cp = cmd;
	struct btp_mesh_cfg_heartbeat_sub_get_rp *rp = rsp;
	uint8_t status;
	struct bt_mesh_cfg_cli_hb_sub sub;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_hb_sub_get(sys_le16_to_cpu(cp->net_idx),
					 sys_le16_to_cpu(cp->address),
					 &sub, &status);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_net_trans_get(const void *cmd, uint16_t cmd_len,
				    void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_net_trans_get_cmd *cp = cmd;
	struct btp_mesh_cfg_net_trans_get_rp *rp = rsp;
	uint8_t transmit;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_net_transmit_get(sys_le16_to_cpu(cp->net_idx),
					       sys_le16_to_cpu(cp->address),
					       &transmit);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->transmit = transmit;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_net_trans_set(const void *cmd, uint16_t cmd_len,
				    void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_net_trans_set_cmd *cp = cmd;
	struct btp_mesh_cfg_net_trans_set_rp *rp = rsp;
	uint8_t transmit;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_net_transmit_set(sys_le16_to_cpu(cp->net_idx),
					       sys_le16_to_cpu(cp->address),
					       cp->transmit, &transmit);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->transmit = transmit;
	*rsp_len = sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t config_node_identity_set(const void *cmd, uint16_t cmd_len,
				    void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_node_idt_set_cmd *cp = cmd;
	struct btp_mesh_cfg_node_idt_set_rp *rp = rsp;
	uint8_t identity;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_node_identity_set(sys_le16_to_cpu(cp->net_idx),
						sys_le16_to_cpu(cp->address),
						sys_le16_to_cpu(cp->net_key_idx),
						cp->new_identity,
						&status, &identity);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	rp->identity = identity;

	*rsp_len = sizeof(*rp);
	return BTP_STATUS_SUCCESS;
}

static uint8_t config_node_identity_get(const void *cmd, uint16_t cmd_len,
				    void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_node_idt_get_cmd *cp = cmd;
	struct btp_mesh_cfg_node_idt_get_rp *rp = rsp;
	uint8_t identity;
	uint8_t status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_node_identity_get(sys_le16_to_cpu(cp->net_idx),
						sys_le16_to_cpu(cp->address),
						sys_le16_to_cpu(cp->net_key_idx),
						&status, &identity);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;
	rp->identity = identity;

	*rsp_len = sizeof(*rp);
	return BTP_STATUS_SUCCESS;
}

static uint8_t config_node_reset(const void *cmd, uint16_t cmd_len,
				 void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_node_reset_cmd *cp = cmd;
	struct btp_mesh_cfg_node_reset_rp *rp = rsp;
	bool status;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_node_reset(sys_le16_to_cpu(cp->net_idx),
					 sys_le16_to_cpu(cp->address),
					 &status);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->status = status;

	*rsp_len = sizeof(*rp);
	return BTP_STATUS_SUCCESS;
}

static uint8_t config_lpn_timeout_get(const void *cmd, uint16_t cmd_len,
				      void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_cfg_lpn_timeout_cmd *cp = cmd;
	struct btp_mesh_cfg_lpn_timeout_rp *rp = rsp;
	int32_t polltimeout;
	int err;

	LOG_DBG("");

	err = bt_mesh_cfg_cli_lpn_timeout_get(sys_le16_to_cpu(cp->net_idx),
					      sys_le16_to_cpu(cp->address),
					      sys_le16_to_cpu(cp->unicast_addr),
					      &polltimeout);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	rp->timeout = sys_cpu_to_le32(polltimeout);

	*rsp_len = sizeof(*rp);
	return BTP_STATUS_SUCCESS;
}

static uint8_t health_fault_get(const void *cmd, uint16_t cmd_len,
				void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_health_fault_get_cmd *cp = cmd;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.addr = sys_le16_to_cpu(cp->address),
		.app_idx = sys_le16_to_cpu(cp->app_idx),
	};
	uint8_t test_id;
	size_t fault_count = 16;
	uint8_t faults[fault_count];
	int err;

	LOG_DBG("");

	err = bt_mesh_health_cli_fault_get(&health_cli, &ctx,
					   sys_le16_to_cpu(cp->cid), &test_id,
					   faults, &fault_count);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t health_fault_clear(const void *cmd, uint16_t cmd_len,
				  void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_health_fault_clear_cmd *cp = cmd;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.addr = sys_le16_to_cpu(cp->address),
		.app_idx = sys_le16_to_cpu(cp->app_idx),
	};
	uint8_t test_id;
	size_t fault_count = 16;
	uint8_t faults[fault_count];
	int err;

	LOG_DBG("");

	if (cp->ack) {
		err = bt_mesh_health_cli_fault_clear(&health_cli, &ctx,
						     sys_le16_to_cpu(cp->cid),
						     &test_id, faults,
						     &fault_count);
	} else {
		err = bt_mesh_health_cli_fault_clear_unack(&health_cli, &ctx,
							   sys_le16_to_cpu(cp->cid));
	}

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	if (cp->ack) {
		struct btp_mesh_health_fault_clear_rp *rp = rsp;

		rp->test_id = test_id;
		*rsp_len = sizeof(*rp);
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t health_fault_test(const void *cmd, uint16_t cmd_len,
				 void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_health_fault_test_cmd *cp = cmd;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.addr = sys_le16_to_cpu(cp->address),
		.app_idx = sys_le16_to_cpu(cp->app_idx),
	};
	size_t fault_count = 16;
	uint8_t faults[fault_count];
	uint8_t test_id;
	uint16_t cid;
	int err;

	LOG_DBG("");

	test_id = cp->test_id;
	cid = sys_le16_to_cpu(cp->cid);

	if (cp->ack) {
		err = bt_mesh_health_cli_fault_test(&health_cli, &ctx, cid, test_id, faults,
						    &fault_count);
	} else {
		err = bt_mesh_health_cli_fault_test_unack(&health_cli, &ctx, cid, test_id);
	}

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	if (cp->ack) {
		struct btp_mesh_health_fault_test_rp *rp = rsp;

		rp->test_id = test_id;
		rp->cid = sys_cpu_to_le16(cid);
		(void)memcpy(rp->faults, faults, fault_count);

		*rsp_len = sizeof(*rp) + fault_count;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t health_period_get(const void *cmd, uint16_t cmd_len,
				 void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_health_period_get_cmd *cp = cmd;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.addr = sys_le16_to_cpu(cp->address),
		.app_idx = sys_le16_to_cpu(cp->app_idx),
	};
	uint8_t divisor;
	int err;

	LOG_DBG("");

	err = bt_mesh_health_cli_period_get(&health_cli, &ctx, &divisor);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t health_period_set(const void *cmd, uint16_t cmd_len,
				 void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_health_period_set_cmd *cp = cmd;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.addr = sys_le16_to_cpu(cp->address),
		.app_idx = sys_le16_to_cpu(cp->app_idx),
	};
	uint8_t updated_divisor;
	int err;

	LOG_DBG("");

	if (cp->ack) {
		err = bt_mesh_health_cli_period_set(&health_cli, &ctx, cp->divisor,
						    &updated_divisor);
	} else {
		err = bt_mesh_health_cli_period_set_unack(&health_cli, &ctx, cp->divisor);
	}

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	if (cp->ack) {
		struct btp_mesh_health_period_set_rp *rp = rsp;

		rp->divisor = updated_divisor;

		*rsp_len = sizeof(*rp);
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t health_attention_get(const void *cmd, uint16_t cmd_len,
				    void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_health_attention_get_cmd *cp = cmd;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.addr = sys_le16_to_cpu(cp->address),
		.app_idx = sys_le16_to_cpu(cp->app_idx),
	};
	uint8_t attention;
	int err;

	LOG_DBG("");

	err = bt_mesh_health_cli_attention_get(&health_cli, &ctx, &attention);

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t health_attention_set(const void *cmd, uint16_t cmd_len,
				    void *rsp, uint16_t *rsp_len)
{
	const struct btp_mesh_health_attention_set_cmd *cp = cmd;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.addr = sys_le16_to_cpu(cp->address),
		.app_idx = sys_le16_to_cpu(cp->app_idx),
	};
	uint8_t updated_attention;
	int err;

	LOG_DBG("");

	if (cp->ack) {
		err = bt_mesh_health_cli_attention_set(&health_cli, &ctx, cp->attention,
						       &updated_attention);
	} else {
		err = bt_mesh_health_cli_attention_set_unack(&health_cli, &ctx, cp->attention);
	}

	if (err) {
		LOG_ERR("err %d", err);
		return BTP_STATUS_FAILED;
	}

	if (cp->ack) {
		struct btp_mesh_health_attention_set_rp *rp = rsp;

		rp->attention = updated_attention;

		*rsp_len = sizeof(*rp);
	}

	return BTP_STATUS_SUCCESS;
}

static const struct btp_handler handlers[] = {
	{
		.opcode = BTP_MESH_READ_SUPPORTED_COMMANDS,
		.index = BTP_INDEX_NONE,
		.expect_len = 0,
		.func = supported_commands,
	},
	{
		.opcode = BTP_MESH_CONFIG_PROVISIONING,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = config_prov,
	},
	{
		.opcode = BTP_MESH_PROVISION_NODE,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = provision_node,
	},
	{
		.opcode = BTP_MESH_INIT,
		.expect_len = 0,
		.func = init,
	},
	{
		.opcode = BTP_MESH_RESET,
		.expect_len = 0,
		.func = reset,
	},
	{
		.opcode = BTP_MESH_INPUT_NUMBER,
		.expect_len = sizeof(struct btp_mesh_input_number_cmd),
		.func = input_number,
	},
	{
		.opcode = BTP_MESH_INPUT_STRING,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = input_string,
	},
	{
		.opcode = BTP_MESH_IVU_TEST_MODE,
		.expect_len = sizeof(struct btp_mesh_ivu_test_mode_cmd),
		.func = ivu_test_mode,
	},
	{
		.opcode = BTP_MESH_IVU_TOGGLE_STATE,
		.expect_len = 0,
		.func = ivu_toggle_state,
	},
	{
		.opcode = BTP_MESH_LPN,
		.expect_len = sizeof(struct btp_mesh_lpn_set_cmd),
		.func = lpn,
	},
	{
		.opcode = BTP_MESH_LPN_POLL,
		.expect_len = 0,
		.func = lpn_poll,
	},
	{
		.opcode = BTP_MESH_NET_SEND,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = net_send,
	},
	{
		.opcode = BTP_MESH_HEALTH_GENERATE_FAULTS,
		.expect_len = 0,
		.func = health_generate_faults,
	},
	{
		.opcode = BTP_MESH_HEALTH_CLEAR_FAULTS,
		.expect_len = 0,
		.func = health_clear_faults,
	},
	{
		.opcode = BTP_MESH_MODEL_SEND,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = model_send,
	},
	{
		.opcode = BTP_MESH_COMP_DATA_GET,
		.expect_len = sizeof(struct btp_mesh_comp_data_get_cmd),
		.func = composition_data_get,
	},
	{
		.opcode = BTP_MESH_CFG_BEACON_GET,
		.expect_len = sizeof(struct btp_mesh_cfg_beacon_get_cmd),
		.func = config_beacon_get,
	},
	{
		.opcode = BTP_MESH_CFG_BEACON_SET,
		.expect_len = sizeof(struct btp_mesh_cfg_beacon_set_cmd),
		.func = config_beacon_set,
	},
	{
		.opcode = BTP_MESH_CFG_DEFAULT_TTL_GET,
		.expect_len = sizeof(struct btp_mesh_cfg_default_ttl_get_cmd),
		.func = config_default_ttl_get,
	},
	{
		.opcode = BTP_MESH_CFG_DEFAULT_TTL_SET,
		.expect_len = sizeof(struct btp_mesh_cfg_default_ttl_set_cmd),
		.func = config_default_ttl_set,
	},
	{
		.opcode = BTP_MESH_CFG_GATT_PROXY_GET,
		.expect_len = sizeof(struct btp_mesh_cfg_gatt_proxy_get_cmd),
		.func = config_gatt_proxy_get,
	},
	{
		.opcode = BTP_MESH_CFG_GATT_PROXY_SET,
		.expect_len = sizeof(struct btp_mesh_cfg_gatt_proxy_set_cmd),
		.func = config_gatt_proxy_set,
	},
	{
		.opcode = BTP_MESH_CFG_FRIEND_GET,
		.expect_len = sizeof(struct btp_mesh_cfg_friend_get_cmd),
		.func = config_friend_get,
	},
	{
		.opcode = BTP_MESH_CFG_FRIEND_SET,
		.expect_len = sizeof(struct btp_mesh_cfg_friend_set_cmd),
		.func = config_friend_set,
	},
	{
		.opcode = BTP_MESH_CFG_RELAY_GET,
		.expect_len = sizeof(struct btp_mesh_cfg_relay_get_cmd),
		.func = config_relay_get,
	},
	{
		.opcode = BTP_MESH_CFG_RELAY_SET,
		.expect_len = sizeof(struct btp_mesh_cfg_relay_set_cmd),
		.func = config_relay_set,
	},
	{
		.opcode = BTP_MESH_CFG_MODEL_PUB_GET,
		.expect_len = sizeof(struct btp_mesh_cfg_model_pub_get_cmd),
		.func = config_mod_pub_get,
	},
	{
		.opcode = BTP_MESH_CFG_MODEL_PUB_SET,
		.expect_len = sizeof(struct btp_mesh_cfg_model_pub_set_cmd),
		.func = config_mod_pub_set,
	},
	{
		.opcode = BTP_MESH_CFG_MODEL_SUB_ADD,
		.expect_len = sizeof(struct btp_mesh_cfg_model_sub_add_cmd),
		.func = config_mod_sub_add,
	},
	{
		.opcode = BTP_MESH_CFG_MODEL_SUB_DEL,
		.expect_len = sizeof(struct btp_mesh_cfg_model_sub_del_cmd),
		.func = config_mod_sub_del,
	},
	{
		.opcode = BTP_MESH_CFG_MODEL_SUB_OVW,
		.expect_len = sizeof(struct btp_mesh_cfg_model_sub_ovw_cmd),
		.func = config_mod_sub_ovw,
	},
	{
		.opcode = BTP_MESH_CFG_MODEL_SUB_DEL_ALL,
		.expect_len = sizeof(struct btp_mesh_cfg_model_sub_del_all_cmd),
		.func = config_mod_sub_del_all,
	},
	{
		.opcode = BTP_MESH_CFG_MODEL_SUB_GET,
		.expect_len = sizeof(struct btp_mesh_cfg_model_sub_get_cmd),
		.func = config_mod_sub_get,
	},
	{
		.opcode = BTP_MESH_CFG_MODEL_SUB_GET_VND,
		.expect_len = sizeof(struct btp_mesh_cfg_model_sub_get_vnd_cmd),
		.func = config_mod_sub_get_vnd,
	},
	{
		.opcode = BTP_MESH_CFG_MODEL_SUB_VA_ADD,
		.expect_len = sizeof(struct btp_mesh_cfg_model_sub_va_add_cmd),
		.func = config_mod_sub_va_add,
	},
	{
		.opcode = BTP_MESH_CFG_MODEL_SUB_VA_DEL,
		.expect_len = sizeof(struct btp_mesh_cfg_model_sub_va_del_cmd),
		.func = config_mod_sub_va_del,
	},
	{
		.opcode = BTP_MESH_CFG_MODEL_SUB_VA_OVW,
		.expect_len = sizeof(struct btp_mesh_cfg_model_sub_va_ovw_cmd),
		.func = config_mod_sub_va_ovw,
	},
	{
		.opcode = BTP_MESH_CFG_NETKEY_ADD,
		.expect_len = sizeof(struct btp_mesh_cfg_netkey_add_cmd),
		.func = config_netkey_add,
	},
	{
		.opcode = BTP_MESH_CFG_NETKEY_GET,
		.expect_len = sizeof(struct btp_mesh_cfg_netkey_get_cmd),
		.func = config_netkey_get,
	},
	{
		.opcode = BTP_MESH_CFG_NETKEY_DEL,
		.expect_len = sizeof(struct btp_mesh_cfg_netkey_del_cmd),
		.func = config_netkey_del,
	},
	{
		.opcode = BTP_MESH_CFG_NETKEY_UPDATE,
		.expect_len = sizeof(struct btp_mesh_cfg_netkey_update_cmd),
		.func = config_netkey_update,
	},
	{
		.opcode = BTP_MESH_CFG_APPKEY_ADD,
		.expect_len = sizeof(struct btp_mesh_cfg_appkey_add_cmd),
		.func = config_appkey_add,
	},
	{
		.opcode = BTP_MESH_CFG_APPKEY_GET,
		.expect_len = sizeof(struct btp_mesh_cfg_appkey_get_cmd),
		.func = config_appkey_get,
	},
	{
		.opcode = BTP_MESH_CFG_APPKEY_DEL,
		.expect_len = sizeof(struct btp_mesh_cfg_appkey_del_cmd),
		.func = config_appkey_del,
	},
	{
		.opcode = BTP_MESH_CFG_APPKEY_UPDATE,
		.expect_len = sizeof(struct btp_mesh_cfg_appkey_update_cmd),
		.func = config_appkey_update,
	},
	{
		.opcode = BTP_MESH_CFG_MODEL_APP_BIND,
		.expect_len = sizeof(struct btp_mesh_cfg_model_app_bind_cmd),
		.func = config_model_app_bind,
	},
	{
		.opcode = BTP_MESH_CFG_MODEL_APP_UNBIND,
		.expect_len = sizeof(struct btp_mesh_cfg_model_app_unbind_cmd),
		.func = config_model_app_unbind,
	},
	{
		.opcode = BTP_MESH_CFG_MODEL_APP_GET,
		.expect_len = sizeof(struct btp_mesh_cfg_model_app_get_cmd),
		.func = config_model_app_get,
	},
	{
		.opcode = BTP_MESH_CFG_MODEL_APP_VND_GET,
		.expect_len = sizeof(struct btp_mesh_cfg_model_app_vnd_get_cmd),
		.func = config_model_app_vnd_get,
	},
	{
		.opcode = BTP_MESH_CFG_HEARTBEAT_PUB_SET,
		.expect_len = sizeof(struct btp_mesh_cfg_heartbeat_pub_set_cmd),
		.func = config_hb_pub_set,
	},
	{
		.opcode = BTP_MESH_CFG_HEARTBEAT_PUB_GET,
		.expect_len = sizeof(struct btp_mesh_cfg_heartbeat_pub_get_cmd),
		.func = config_hb_pub_get,
	},
	{
		.opcode = BTP_MESH_CFG_HEARTBEAT_SUB_SET,
		.expect_len = sizeof(struct btp_mesh_cfg_heartbeat_sub_set_cmd),
		.func = config_hb_sub_set,
	},
	{
		.opcode = BTP_MESH_CFG_HEARTBEAT_SUB_GET,
		.expect_len = sizeof(struct btp_mesh_cfg_heartbeat_sub_get_cmd),
		.func = config_hb_sub_get,
	},
	{
		.opcode = BTP_MESH_CFG_NET_TRANS_GET,
		.expect_len = sizeof(struct btp_mesh_cfg_net_trans_get_cmd),
		.func = config_net_trans_get,
	},
	{
		.opcode = BTP_MESH_CFG_NET_TRANS_SET,
		.expect_len = sizeof(struct btp_mesh_cfg_net_trans_set_cmd),
		.func = config_net_trans_set,
	},
	{
		.opcode = BTP_MESH_CFG_NODE_IDT_SET,
		.expect_len = sizeof(struct btp_mesh_cfg_node_idt_set_cmd),
		.func = config_node_identity_set,
	},
	{
		.opcode = BTP_MESH_CFG_NODE_IDT_GET,
		.expect_len = sizeof(struct btp_mesh_cfg_node_idt_get_cmd),
		.func = config_node_identity_get,
	},
	{
		.opcode = BTP_MESH_CFG_NODE_RESET,
		.expect_len = sizeof(struct btp_mesh_cfg_node_reset_cmd),
		.func = config_node_reset,
	},
	{
		.opcode = BTP_MESH_CFG_LPN_TIMEOUT_GET,
		.expect_len = sizeof(struct btp_mesh_cfg_lpn_timeout_cmd),
		.func = config_lpn_timeout_get,
	},
	{
		.opcode = BTP_MESH_CFG_MODEL_PUB_VA_SET,
		.expect_len = sizeof(struct btp_mesh_cfg_model_pub_va_set_cmd),
		.func = config_mod_pub_va_set,
	},
	{
		.opcode = BTP_MESH_CFG_MODEL_APP_BIND_VND,
		.expect_len = sizeof(struct btp_mesh_cfg_model_app_bind_vnd_cmd),
		.func = config_model_app_bind_vnd,
	},
	{
		.opcode = BTP_MESH_HEALTH_FAULT_GET,
		.expect_len = sizeof(struct btp_mesh_health_fault_get_cmd),
		.func = health_fault_get,
	},
	{
		.opcode = BTP_MESH_HEALTH_FAULT_CLEAR,
		.expect_len = sizeof(struct btp_mesh_health_fault_clear_cmd),
		.func = health_fault_clear,
	},
	{
		.opcode = BTP_MESH_HEALTH_FAULT_TEST,
		.expect_len = sizeof(struct btp_mesh_health_fault_test_cmd),
		.func = health_fault_test,
	},
	{
		.opcode = BTP_MESH_HEALTH_PERIOD_GET,
		.expect_len = sizeof(struct btp_mesh_health_period_get_cmd),
		.func = health_period_get,
	},
	{
		.opcode = BTP_MESH_HEALTH_PERIOD_SET,
		.expect_len = sizeof(struct btp_mesh_health_period_set_cmd),
		.func = health_period_set,
	},
	{
		.opcode = BTP_MESH_HEALTH_ATTENTION_GET,
		.expect_len = sizeof(struct btp_mesh_health_attention_get_cmd),
		.func = health_attention_get,
	},
	{
		.opcode = BTP_MESH_HEALTH_ATTENTION_SET,
		.expect_len = sizeof(struct btp_mesh_health_attention_set_cmd),
		.func = health_attention_set,
	},
	{
		.opcode = BTP_MESH_PROVISION_ADV,
		.expect_len = sizeof(struct btp_mesh_provision_adv_cmd),
		.func = provision_adv,
	},
	{
		.opcode = BTP_MESH_CFG_KRP_GET,
		.expect_len = sizeof(struct btp_mesh_cfg_krp_get_cmd),
		.func = config_krp_get,
	},
	{
		.opcode = BTP_MESH_CFG_KRP_SET,
		.expect_len = sizeof(struct btp_mesh_cfg_krp_set_cmd),
		.func = config_krp_set,
	},
#if defined(CONFIG_BT_TESTING)
	{
		.opcode = BTP_MESH_LPN_SUBSCRIBE,
		.expect_len = sizeof(struct btp_mesh_lpn_subscribe_cmd),
		.func = lpn_subscribe,
	},
	{
		.opcode = BTP_MESH_LPN_UNSUBSCRIBE,
		.expect_len = sizeof(struct btp_mesh_lpn_unsubscribe_cmd),
		.func = lpn_unsubscribe,
	},
	{
		.opcode = BTP_MESH_RPL_CLEAR,
		.expect_len = 0,
		.func = rpl_clear,
	},
#endif /* CONFIG_BT_TESTING */
	{
		.opcode = BTP_MESH_PROXY_IDENTITY,
		.expect_len = 0,
		.func = proxy_identity_enable,
	},
};

void net_recv_ev(uint8_t ttl, uint8_t ctl, uint16_t src, uint16_t dst, const void *payload,
		 size_t payload_len)
{
	NET_BUF_SIMPLE_DEFINE(buf, UINT8_MAX);
	struct btp_mesh_net_recv_ev *ev;

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

	tester_event(BTP_SERVICE_ID_MESH, BTP_MESH_EV_NET_RECV, buf.data, buf.len);
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
	struct btp_mesh_invalid_bearer_ev ev = {
		.opcode = opcode,
	};

	LOG_DBG("opcode 0x%02x", opcode);

	tester_event(BTP_SERVICE_ID_MESH, BTP_MESH_EV_INVALID_BEARER, &ev, sizeof(ev));
}

static void incomp_timer_exp_cb(void)
{
	tester_event(BTP_SERVICE_ID_MESH, BTP_MESH_EV_INCOMP_TIMER_EXP, NULL, 0);
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
	struct btp_mesh_frnd_established_ev ev = { net_idx, lpn_addr, recv_delay,
					       polltimeout };

	LOG_DBG("Friendship (as Friend) established with "
			"LPN 0x%04x Receive Delay %u Poll Timeout %u",
			lpn_addr, recv_delay, polltimeout);


	tester_event(BTP_SERVICE_ID_MESH, BTP_MESH_EV_FRND_ESTABLISHED, &ev, sizeof(ev));
}

static void friend_terminated(uint16_t net_idx, uint16_t lpn_addr)
{
	struct btp_mesh_frnd_terminated_ev ev = { net_idx, lpn_addr };

	LOG_DBG("Friendship (as Friend) lost with LPN "
			"0x%04x", lpn_addr);

	tester_event(BTP_SERVICE_ID_MESH, BTP_MESH_EV_FRND_TERMINATED, &ev, sizeof(ev));
}

BT_MESH_FRIEND_CB_DEFINE(friend_cb) = {
	.established = friend_established,
	.terminated = friend_terminated,
};

static void lpn_established(uint16_t net_idx, uint16_t friend_addr,
					uint8_t queue_size, uint8_t recv_win)
{
	struct btp_mesh_lpn_established_ev ev = { net_idx, friend_addr, queue_size,
					      recv_win };

	LOG_DBG("Friendship (as LPN) established with "
			"Friend 0x%04x Queue Size %d Receive Window %d",
			friend_addr, queue_size, recv_win);

	tester_event(BTP_SERVICE_ID_MESH, BTP_MESH_EV_LPN_ESTABLISHED, &ev, sizeof(ev));
}

static void lpn_terminated(uint16_t net_idx, uint16_t friend_addr)
{
	struct btp_mesh_lpn_polled_ev ev = { net_idx, friend_addr };

	LOG_DBG("Friendship (as LPN) lost with Friend "
			"0x%04x", friend_addr);

	tester_event(BTP_SERVICE_ID_MESH, BTP_MESH_EV_LPN_TERMINATED, &ev, sizeof(ev));
}

static void lpn_polled(uint16_t net_idx, uint16_t friend_addr, bool retry)
{
	struct btp_mesh_lpn_polled_ev ev = { net_idx, friend_addr, (uint8_t)retry };

	LOG_DBG("LPN polled 0x%04x %s", friend_addr, retry ? "(retry)" : "");

	tester_event(BTP_SERVICE_ID_MESH, BTP_MESH_EV_LPN_POLLED, &ev, sizeof(ev));
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

	tester_register_command_handlers(BTP_SERVICE_ID_MESH, handlers,
					 ARRAY_SIZE(handlers));

	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_mesh(void)
{
	return BTP_STATUS_SUCCESS;
}
