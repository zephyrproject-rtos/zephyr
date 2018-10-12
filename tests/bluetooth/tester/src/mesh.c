/* mesh.c - Bluetooth Mesh Tester */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <bluetooth/bluetooth.h>

#include <errno.h>
#include <bluetooth/mesh.h>
#include <bluetooth/testing.h>
#include <misc/byteorder.h>
#include "bttester.h"

#define CONTROLLER_INDEX 0
#define CID_LOCAL 0xffff

/* Health server data */
#define CUR_FAULTS_MAX 4
#define HEALTH_TEST_ID 0x00

static u8_t cur_faults[CUR_FAULTS_MAX];
static u8_t reg_faults[CUR_FAULTS_MAX * 2];

/* Provision node data */
static u8_t net_key[16];
static u16_t net_key_idx;
static u8_t flags;
static u32_t iv_index;
static u16_t addr;
static u8_t dev_key[16];
static u8_t input_size;

/* Configured provisioning data */
static u8_t dev_uuid[16];
static u8_t static_auth[16];

/* Vendor Model data */
#define VND_MODEL_ID_1 0x1234

/* Model send data */
#define MODEL_BOUNDS_MAX 2

static struct model_data {
	struct bt_mesh_model *model;
	u16_t addr;
	u16_t appkey_idx;
} model_bound[MODEL_BOUNDS_MAX];

static struct {
	u16_t local;
	u16_t dst;
	u16_t net_idx;
} net = {
	.local = BT_MESH_ADDR_UNASSIGNED,
	.dst = BT_MESH_ADDR_UNASSIGNED,
};

static void supported_commands(u8_t *data, u16_t len)
{
	struct net_buf_simple *buf = NET_BUF_SIMPLE(BTP_DATA_MAX_SIZE);

	net_buf_simple_init(buf, 0);

	/* 1st octet */
	(void)memset(net_buf_simple_add(buf, 1), 0, 1);
	tester_set_bit(buf->data, MESH_READ_SUPPORTED_COMMANDS);
	tester_set_bit(buf->data, MESH_CONFIG_PROVISIONING);
	tester_set_bit(buf->data, MESH_PROVISION_NODE);
	tester_set_bit(buf->data, MESH_INIT);
	tester_set_bit(buf->data, MESH_RESET);
	tester_set_bit(buf->data, MESH_INPUT_NUMBER);
	tester_set_bit(buf->data, MESH_INPUT_STRING);
	/* 2nd octet */
	tester_set_bit(buf->data, MESH_IVU_TEST_MODE);
	tester_set_bit(buf->data, MESH_IVU_TOGGLE_STATE);
	tester_set_bit(buf->data, MESH_NET_SEND);
	tester_set_bit(buf->data, MESH_HEALTH_GENERATE_FAULTS);
	tester_set_bit(buf->data, MESH_HEALTH_CLEAR_FAULTS);
	tester_set_bit(buf->data, MESH_LPN);
	tester_set_bit(buf->data, MESH_LPN_POLL);
	tester_set_bit(buf->data, MESH_MODEL_SEND);
	/* 3rd octet */
	(void)memset(net_buf_simple_add(buf, 1), 0, 1);
#if defined(CONFIG_BT_TESTING)
	tester_set_bit(buf->data, MESH_LPN_SUBSCRIBE);
	tester_set_bit(buf->data, MESH_LPN_UNSUBSCRIBE);
	tester_set_bit(buf->data, MESH_RPL_CLEAR);
#endif /* CONFIG_BT_TESTING */
	tester_set_bit(buf->data, MESH_PROXY_IDENTITY);

	tester_send(BTP_SERVICE_ID_MESH, MESH_READ_SUPPORTED_COMMANDS,
		    CONTROLLER_INDEX, buf->data, buf->len);
}

static struct bt_mesh_cfg_srv cfg_srv = {
	.relay = BT_MESH_RELAY_ENABLED,
	.beacon = BT_MESH_BEACON_ENABLED,
#if defined(CONFIG_BT_MESH_FRIEND)
	.frnd = BT_MESH_FRIEND_DISABLED,
#else
	.frnd = BT_MESH_FRIEND_NOT_SUPPORTED,
#endif
#if defined(CONFIG_BT_MESH_GATT_PROXY)
	.gatt_proxy = BT_MESH_GATT_PROXY_ENABLED,
#else
	.gatt_proxy = BT_MESH_GATT_PROXY_NOT_SUPPORTED,
#endif
	.default_ttl = 7,

	/* 3 transmissions with 20ms interval */
	.net_transmit = BT_MESH_TRANSMIT(2, 20),
	.relay_retransmit = BT_MESH_TRANSMIT(2, 20),
};

static void get_faults(u8_t *faults, u8_t faults_size, u8_t *dst, u8_t *count)
{
	u8_t i, limit = *count;

	for (i = 0, *count = 0; i < faults_size && *count < limit; i++) {
		if (faults[i]) {
			*dst++ = faults[i];
			(*count)++;
		}
	}
}

static int fault_get_cur(struct bt_mesh_model *model, u8_t *test_id,
			 u16_t *company_id, u8_t *faults, u8_t *fault_count)
{
	SYS_LOG_DBG("");

	*test_id = HEALTH_TEST_ID;
	*company_id = CID_LOCAL;

	get_faults(cur_faults, sizeof(cur_faults), faults, fault_count);

	return 0;
}

static int fault_get_reg(struct bt_mesh_model *model, u16_t company_id,
			 u8_t *test_id, u8_t *faults, u8_t *fault_count)
{
	SYS_LOG_DBG("company_id 0x%04x", company_id);

	if (company_id != CID_LOCAL) {
		return -EINVAL;
	}

	*test_id = HEALTH_TEST_ID;

	get_faults(reg_faults, sizeof(reg_faults), faults, fault_count);

	return 0;
}

static int fault_clear(struct bt_mesh_model *model, uint16_t company_id)
{
	SYS_LOG_DBG("company_id 0x%04x", company_id);

	if (company_id != CID_LOCAL) {
		return -EINVAL;
	}

	(void)memset(reg_faults, 0, sizeof(reg_faults));

	return 0;
}

static int fault_test(struct bt_mesh_model *model, uint8_t test_id,
		      uint16_t company_id)
{
	SYS_LOG_DBG("test_id 0x%02x company_id 0x%04x", test_id, company_id);

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

void show_faults(u8_t test_id, u16_t cid, u8_t *faults, size_t fault_count)
{
	size_t i;

	if (!fault_count) {
		SYS_LOG_DBG("Health Test ID 0x%02x Company ID 0x%04x: "
			    "no faults", test_id, cid);
		return;
	}

	SYS_LOG_DBG("Health Test ID 0x%02x Company ID 0x%04x Fault Count %zu: ",
		    test_id, cid, fault_count);

	for (i = 0; i < fault_count; i++) {
		SYS_LOG_DBG("0x%02x", faults[i]);
	}
}

static void health_current_status(struct bt_mesh_health_cli *cli, u16_t addr,
				  u8_t test_id, u16_t cid, u8_t *faults,
				  size_t fault_count)
{
	SYS_LOG_DBG("Health Current Status from 0x%04x", addr);
	show_faults(test_id, cid, faults, fault_count);
}

static struct bt_mesh_health_cli health_cli = {
	.current_status = health_current_status,
};

static struct bt_mesh_model root_models[] = {
	BT_MESH_MODEL_CFG_SRV(&cfg_srv),
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

	SYS_LOG_DBG("bearer 0x%02x", bearer);

	switch (bearer) {
	case BT_MESH_PROV_ADV:
		ev.bearer = MESH_PROV_BEARER_PB_ADV;
		break;
	case BT_MESH_PROV_GATT:
		ev.bearer = MESH_PROV_BEARER_PB_GATT;
		break;
	default:
		SYS_LOG_ERR("Invalid bearer");

		return;
	}

	tester_send(BTP_SERVICE_ID_MESH, MESH_EV_PROV_LINK_OPEN,
		    CONTROLLER_INDEX, (u8_t *) &ev, sizeof(ev));
}

static void link_close(bt_mesh_prov_bearer_t bearer)
{
	struct mesh_prov_link_closed_ev ev;

	SYS_LOG_DBG("bearer 0x%02x", bearer);

	switch (bearer) {
	case BT_MESH_PROV_ADV:
		ev.bearer = MESH_PROV_BEARER_PB_ADV;
		break;
	case BT_MESH_PROV_GATT:
		ev.bearer = MESH_PROV_BEARER_PB_GATT;
		break;
	default:
		SYS_LOG_ERR("Invalid bearer");

		return;
	}

	tester_send(BTP_SERVICE_ID_MESH, MESH_EV_PROV_LINK_CLOSED,
		    CONTROLLER_INDEX, (u8_t *) &ev, sizeof(ev));
}

static int output_number(bt_mesh_output_action_t action, u32_t number)
{
	struct mesh_out_number_action_ev ev;

	SYS_LOG_DBG("action 0x%04x number 0x%08x", action, number);

	ev.action = sys_cpu_to_le16(action);
	ev.number = sys_cpu_to_le32(number);

	tester_send(BTP_SERVICE_ID_MESH, MESH_EV_OUT_NUMBER_ACTION,
		    CONTROLLER_INDEX, (u8_t *) &ev, sizeof(ev));

	return 0;
}

static int output_string(const char *str)
{
	struct mesh_out_string_action_ev *ev;
	struct net_buf_simple *buf = NET_BUF_SIMPLE(BTP_DATA_MAX_SIZE);

	SYS_LOG_DBG("str %s", str);

	net_buf_simple_init(buf, 0);

	ev = net_buf_simple_add(buf, sizeof(*ev));
	ev->string_len = strlen(str);

	net_buf_simple_add_mem(buf, str, ev->string_len);

	tester_send(BTP_SERVICE_ID_MESH, MESH_EV_OUT_STRING_ACTION,
		    CONTROLLER_INDEX, buf->data, buf->len);

	return 0;
}

static int input(bt_mesh_input_action_t action, u8_t size)
{
	struct mesh_in_action_ev ev;

	SYS_LOG_DBG("action 0x%04x number 0x%02x", action, size);

	input_size = size;

	ev.action = sys_cpu_to_le16(action);
	ev.size = size;

	tester_send(BTP_SERVICE_ID_MESH, MESH_EV_IN_ACTION, CONTROLLER_INDEX,
		    (u8_t *) &ev, sizeof(ev));

	return 0;
}

static void prov_complete(u16_t net_idx, u16_t addr)
{
	SYS_LOG_DBG("net_idx 0x%04x addr 0x%04x", net_idx, addr);

	net.net_idx = net_idx,
	net.local = addr;
	net.dst = addr;

	tester_send(BTP_SERVICE_ID_MESH, MESH_EV_PROVISIONED, CONTROLLER_INDEX,
		    NULL, 0);
}

static void prov_reset(void)
{
	SYS_LOG_DBG("");

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
	.reset = prov_reset,
};

static void config_prov(u8_t *data, u16_t len)
{
	const struct mesh_config_provisioning_cmd *cmd = (void *) data;

	SYS_LOG_DBG("");

	memcpy(dev_uuid, cmd->uuid, sizeof(dev_uuid));
	memcpy(static_auth, cmd->static_auth, sizeof(static_auth));

	prov.output_size = cmd->out_size;
	prov.output_actions = sys_le16_to_cpu(cmd->out_actions);
	prov.input_size = cmd->in_size;
	prov.input_actions = sys_le16_to_cpu(cmd->in_actions);

	tester_rsp(BTP_SERVICE_ID_MESH, MESH_CONFIG_PROVISIONING,
		   CONTROLLER_INDEX, BTP_STATUS_SUCCESS);
}

static void provision_node(u8_t *data, u16_t len)
{
	const struct mesh_provision_node_cmd *cmd = (void *) data;

	SYS_LOG_DBG("");

	memcpy(dev_key, cmd->dev_key, sizeof(dev_key));
	memcpy(net_key, cmd->net_key, sizeof(net_key));

	addr = sys_le16_to_cpu(cmd->addr);
	flags = cmd->flags;
	iv_index = sys_le32_to_cpu(cmd->iv_index);
	net_key_idx = sys_le16_to_cpu(cmd->net_key_idx);

	tester_rsp(BTP_SERVICE_ID_MESH, MESH_PROVISION_NODE,
		   CONTROLLER_INDEX, BTP_STATUS_SUCCESS);
}

static void init(u8_t *data, u16_t len)
{
	u8_t status = BTP_STATUS_SUCCESS;
	int err;

	SYS_LOG_DBG("");

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

	/* Set device key for vendor model */
	vnd_models[0].keys[0] = BT_MESH_KEY_DEV;

rsp:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_INIT, CONTROLLER_INDEX,
		   status);
}

static void reset(u8_t *data, u16_t len)
{
	SYS_LOG_DBG("");

	bt_mesh_reset();

	tester_rsp(BTP_SERVICE_ID_MESH, MESH_RESET, CONTROLLER_INDEX,
		   BTP_STATUS_SUCCESS);
}

static void input_number(u8_t *data, u16_t len)
{
	const struct mesh_input_number_cmd *cmd = (void *) data;
	u8_t status = BTP_STATUS_SUCCESS;
	u32_t number;
	int err;

	number = sys_le32_to_cpu(cmd->number);

	SYS_LOG_DBG("number 0x%04x", number);

	err = bt_mesh_input_number(number);
	if (err) {
		status = BTP_STATUS_FAILED;
	}

	tester_rsp(BTP_SERVICE_ID_MESH, MESH_INPUT_NUMBER, CONTROLLER_INDEX,
		   status);
}

static void input_string(u8_t *data, u16_t len)
{
	const struct mesh_input_string_cmd *cmd = (void *) data;
	u8_t status = BTP_STATUS_SUCCESS;
	u8_t str_auth[16];
	int err;

	SYS_LOG_DBG("");

	if (cmd->string_len > sizeof(str_auth)) {
		SYS_LOG_ERR("Too long input (%u chars required)", input_size);
		status = BTP_STATUS_FAILED;
		goto rsp;
	} else if (cmd->string_len < input_size) {
		SYS_LOG_ERR("Too short input (%u chars required)", input_size);
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

static void ivu_test_mode(u8_t *data, u16_t len)
{
	const struct mesh_ivu_test_mode_cmd *cmd = (void *) data;

	SYS_LOG_DBG("enable 0x%02x", cmd->enable);

	bt_mesh_iv_update_test(cmd->enable ? true : false);

	tester_rsp(BTP_SERVICE_ID_MESH, MESH_IVU_TEST_MODE, CONTROLLER_INDEX,
		   BTP_STATUS_SUCCESS);
}

static void ivu_toggle_state(u8_t *data, u16_t len)
{
	bool result;

	SYS_LOG_DBG("");

	result = bt_mesh_iv_update();
	if (!result) {
		SYS_LOG_ERR("Failed to toggle the IV Update state");
	}

	tester_rsp(BTP_SERVICE_ID_MESH, MESH_IVU_TOGGLE_STATE, CONTROLLER_INDEX,
		   result ? BTP_STATUS_SUCCESS : BTP_STATUS_FAILED);
}

static void lpn(u8_t *data, u16_t len)
{
	struct mesh_lpn_set_cmd *cmd = (void *) data;
	bool enable;
	int err;

	SYS_LOG_DBG("enable 0x%02x", cmd->enable);

	enable = cmd->enable ? true : false;
	err = bt_mesh_lpn_set(enable);
	if (err) {
		SYS_LOG_ERR("Failed to toggle LPN (err %d)", err);
	}

	tester_rsp(BTP_SERVICE_ID_MESH, MESH_LPN, CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void lpn_poll(u8_t *data, u16_t len)
{
	int err;

	SYS_LOG_DBG("");

	err = bt_mesh_lpn_poll();
	if (err) {
		SYS_LOG_ERR("Failed to send poll msg (err %d)", err);
	}

	tester_rsp(BTP_SERVICE_ID_MESH, MESH_LPN_POLL, CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void net_send(u8_t *data, u16_t len)
{
	struct mesh_net_send_cmd *cmd = (void *) data;
	NET_BUF_SIMPLE_DEFINE(msg, UINT8_MAX);
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net.net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.addr = sys_le16_to_cpu(cmd->dst),
		.send_ttl = cmd->ttl,
	};
	int err;

	SYS_LOG_DBG("ttl 0x%02x dst 0x%04x payload_len %d", ctx.send_ttl,
		    ctx.addr, cmd->payload_len);

	net_buf_simple_add_mem(&msg, cmd->payload, cmd->payload_len);

	err = bt_mesh_model_send(&vnd_models[0], &ctx, &msg, NULL, NULL);
	if (err) {
		SYS_LOG_ERR("Failed to send (err %d)", err);
	}

	tester_rsp(BTP_SERVICE_ID_MESH, MESH_NET_SEND, CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void health_generate_faults(u8_t *data, u16_t len)
{
	struct mesh_health_generate_faults_rp *rp;
	NET_BUF_SIMPLE_DEFINE(buf, sizeof(*rp) + sizeof(cur_faults) +
			      sizeof(reg_faults));
	u8_t some_faults[] = { 0x01, 0x02, 0x03, 0xff, 0x06 };
	u8_t cur_faults_count, reg_faults_count;

	rp = net_buf_simple_add(&buf, sizeof(*rp));

	cur_faults_count = min(sizeof(cur_faults), sizeof(some_faults));
	memcpy(cur_faults, some_faults, cur_faults_count);
	net_buf_simple_add_mem(&buf, cur_faults, cur_faults_count);
	rp->cur_faults_count = cur_faults_count;

	reg_faults_count = min(sizeof(reg_faults), sizeof(some_faults));
	memcpy(reg_faults, some_faults, reg_faults_count);
	net_buf_simple_add_mem(&buf, reg_faults, reg_faults_count);
	rp->reg_faults_count = reg_faults_count;

	bt_mesh_fault_update(&elements[0]);

	tester_send(BTP_SERVICE_ID_MESH, MESH_HEALTH_GENERATE_FAULTS,
		    CONTROLLER_INDEX, buf.data, buf.len);
}

static void health_clear_faults(u8_t *data, u16_t len)
{
	SYS_LOG_DBG("");

	(void)memset(cur_faults, 0, sizeof(cur_faults));
	(void)memset(reg_faults, 0, sizeof(reg_faults));

	bt_mesh_fault_update(&elements[0]);

	tester_rsp(BTP_SERVICE_ID_MESH, MESH_HEALTH_CLEAR_FAULTS,
		   CONTROLLER_INDEX, BTP_STATUS_SUCCESS);
}

static void model_send(u8_t *data, u16_t len)
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
	u16_t src = sys_le16_to_cpu(cmd->src);

	/* Lookup source address */
	for (i = 0; i < ARRAY_SIZE(model_bound); i++) {
		if (bt_mesh_model_elem(model_bound[i].model)->addr == src) {
			model = model_bound[i].model;
			ctx.app_idx = model_bound[i].appkey_idx;

			break;
		}
	}

	if (!model) {
		SYS_LOG_ERR("Model not found");
		err = -EINVAL;

		goto fail;
	}

	SYS_LOG_DBG("src 0x%04x dst 0x%04x model %p payload_len %d", src,
		    ctx.addr, model, cmd->payload_len);

	net_buf_simple_add_mem(&msg, cmd->payload, cmd->payload_len);

	err = bt_mesh_model_send(model, &ctx, &msg, NULL, NULL);
	if (err) {
		SYS_LOG_ERR("Failed to send (err %d)", err);
	}

fail:
	tester_rsp(BTP_SERVICE_ID_MESH, MESH_MODEL_SEND, CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

#if defined(CONFIG_BT_TESTING)
static void lpn_subscribe(u8_t *data, u16_t len)
{
	struct mesh_lpn_subscribe_cmd *cmd = (void *) data;
	u16_t address = sys_le16_to_cpu(cmd->address);
	int err;

	SYS_LOG_DBG("address 0x%04x", address);

	err = bt_test_mesh_lpn_group_add(address);
	if (err) {
		SYS_LOG_ERR("Failed to subscribe (err %d)", err);
	}

	tester_rsp(BTP_SERVICE_ID_MESH, MESH_LPN_SUBSCRIBE, CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void lpn_unsubscribe(u8_t *data, u16_t len)
{
	struct mesh_lpn_unsubscribe_cmd *cmd = (void *) data;
	u16_t address = sys_le16_to_cpu(cmd->address);
	int err;

	SYS_LOG_DBG("address 0x%04x", address);

	err = bt_test_mesh_lpn_group_remove(&address, 1);
	if (err) {
		SYS_LOG_ERR("Failed to unsubscribe (err %d)", err);
	}

	tester_rsp(BTP_SERVICE_ID_MESH, MESH_LPN_UNSUBSCRIBE, CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

static void rpl_clear(u8_t *data, u16_t len)
{
	int err;

	SYS_LOG_DBG("");

	err = bt_test_mesh_rpl_clear();
	if (err) {
		SYS_LOG_ERR("Failed to clear RPL (err %d)", err);
	}

	tester_rsp(BTP_SERVICE_ID_MESH, MESH_RPL_CLEAR, CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}
#endif /* CONFIG_BT_TESTING */

static void proxy_identity_enable(u8_t *data, u16_t len)
{
	int err;

	SYS_LOG_DBG("");

	err = bt_mesh_proxy_identity_enable();
	if (err) {
		SYS_LOG_ERR("Failed to enable proxy identity (err %d)", err);
	}

	tester_rsp(BTP_SERVICE_ID_MESH, MESH_PROXY_IDENTITY, CONTROLLER_INDEX,
		   err ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS);
}

void tester_handle_mesh(u8_t opcode, u8_t index, u8_t *data, u16_t len)
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

void net_recv_ev(u8_t ttl, u8_t ctl, u16_t src, u16_t dst, const void *payload,
		 size_t payload_len)
{
	NET_BUF_SIMPLE_DEFINE(buf, UINT8_MAX);
	struct mesh_net_recv_ev *ev;

	SYS_LOG_DBG("ttl 0x%02x ctl 0x%02x src 0x%04x dst 0x%04x "
		    "payload_len %d", ttl, ctl, src, dst, payload_len);

	if (payload_len > net_buf_simple_tailroom(&buf)) {
		SYS_LOG_ERR("Payload size exceeds buffer size");

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

static void model_bound_cb(u16_t addr, struct bt_mesh_model *model,
			   u16_t key_idx)
{
	int i;

	SYS_LOG_DBG("remote addr 0x%04x key_idx 0x%04x model %p",
		    addr, key_idx, model);

	for (i = 0; i < ARRAY_SIZE(model_bound); i++) {
		if (!model_bound[i].model) {
			model_bound[i].model = model;
			model_bound[i].addr = addr;
			model_bound[i].appkey_idx = key_idx;

			return;
		}
	}

	SYS_LOG_ERR("model_bound is full");
}

static void model_unbound_cb(u16_t addr, struct bt_mesh_model *model,
			     u16_t key_idx)
{
	int i;

	SYS_LOG_DBG("remote addr 0x%04x key_idx 0x%04x model %p",
		    addr, key_idx, model);

	for (i = 0; i < ARRAY_SIZE(model_bound); i++) {
		if (model_bound[i].model == model) {
			model_bound[i].model = NULL;
			model_bound[i].addr = 0x0000;
			model_bound[i].appkey_idx = BT_MESH_KEY_UNUSED;

			return;
		}
	}

	SYS_LOG_INF("model not found");
}

static void invalid_bearer_cb(u8_t opcode)
{
	struct mesh_invalid_bearer_ev ev = {
		.opcode = opcode,
	};

	SYS_LOG_DBG("opcode 0x%02x", opcode);

	tester_send(BTP_SERVICE_ID_MESH, MESH_EV_INVALID_BEARER,
		    CONTROLLER_INDEX, (u8_t *) &ev, sizeof(ev));
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

u8_t tester_init_mesh(void)
{
	if (IS_ENABLED(CONFIG_BT_TESTING)) {
		bt_test_cb_register(&bt_test_cb);
	}

	return BTP_STATUS_SUCCESS;
}

u8_t tester_unregister_mesh(void)
{
	return BTP_STATUS_SUCCESS;
}
