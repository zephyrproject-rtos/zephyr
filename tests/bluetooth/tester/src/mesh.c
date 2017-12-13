/* mesh.c - Bluetooth Mesh Tester */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <bluetooth/bluetooth.h>

#include <errno.h>
#include <bluetooth/mesh.h>
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
static u32_t seq_num;
static u16_t addr;
static u8_t dev_key[16];
static u8_t input_size;

/* Configured provisioning data */
static u8_t dev_uuid[16];
static u8_t static_auth[16];

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
	memset(net_buf_simple_add(buf, 1), 0, 1);
	tester_set_bit(buf->data, MESH_READ_SUPPORTED_COMMANDS);
	tester_set_bit(buf->data, MESH_CONFIG_PROVISIONING);
	tester_set_bit(buf->data, MESH_PROVISION_NODE);
	tester_set_bit(buf->data, MESH_INIT);
	tester_set_bit(buf->data, MESH_RESET);
	tester_set_bit(buf->data, MESH_INPUT_NUMBER);
	tester_set_bit(buf->data, MESH_INPUT_STRING);
	/* 2nd octet */
	memset(net_buf_simple_add(buf, 1), 0, 1);
	tester_set_bit(buf->data, MESH_IVU_TEST_MODE);
	tester_set_bit(buf->data, MESH_IVU_TOGGLE_STATE);
	tester_set_bit(buf->data, MESH_LPN);
	tester_set_bit(buf->data, MESH_LPN_POLL);

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

	memset(reg_faults, 0, sizeof(reg_faults));

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

static struct bt_mesh_model_pub health_pub = {
	.msg = BT_MESH_HEALTH_FAULT_MSG(CUR_FAULTS_MAX),
};

static struct bt_mesh_model root_models[] = {
	BT_MESH_MODEL_CFG_SRV(&cfg_srv),
	BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub),
};

static struct bt_mesh_elem elements[] = {
	BT_MESH_ELEM(0, root_models, BT_MESH_MODEL_NONE),
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

static int output_number(bt_mesh_output_action_t action, uint32_t number)
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
	seq_num = sys_le32_to_cpu(cmd->seq_num);

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
					seq_num, addr, dev_key);
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
	default:
		tester_rsp(BTP_SERVICE_ID_MESH, opcode, index,
			   BTP_STATUS_UNKNOWN_CMD);
		break;
	}
}

u8_t tester_init_mesh(void)
{
	return BTP_STATUS_SUCCESS;
}
