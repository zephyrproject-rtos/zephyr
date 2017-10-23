/* mesh.c - Bluetooth MESH Tester */

/*
 * Copyright (c) 2017 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <bluetooth/bluetooth.h>

#include <errno.h>
#include <bluetooth/mesh.h>
#include <misc/byteorder.h>
#include "bttester.h"

#define CONTROLLER_INDEX 0

#define CID 0xffff

/* Configure provisioning data */
static u8_t dev_uuid[16];
static u8_t static_auth[16];

/* Provision node data */
static u8_t net_key[16];
static u16_t net_key_idx;
static u8_t flags;
static u32_t iv_index;
static u32_t seq_num;
static u16_t addr;
static u8_t dev_key[16];

static void supported_commands(u8_t *data, u16_t len)
{
	u8_t cmds[1];
	struct mesh_read_supported_commands_rp *rp = (void *) cmds;

	memset(cmds, 0, sizeof(cmds));

	tester_set_bit(cmds, MESH_READ_SUPPORTED_COMMANDS);
	tester_set_bit(cmds, MESH_CONFIG_PROVISIONING);
	tester_set_bit(cmds, MESH_PROVISION_NODE);
	tester_set_bit(cmds, MESH_INIT);
	tester_set_bit(cmds, MESH_RESET);
	tester_set_bit(cmds, MESH_INPUT_NUMBER);
	tester_set_bit(cmds, MESH_INPUT_STRING);

	tester_send(BTP_SERVICE_ID_MESH, MESH_READ_SUPPORTED_COMMANDS,
		    CONTROLLER_INDEX, (u8_t *) rp, sizeof(cmds));
}

static struct bt_mesh_cfg_srv cfg_srv = {
	.relay = BT_MESH_RELAY_DISABLED,
	.beacon = BT_MESH_BEACON_ENABLED,
	.frnd = BT_MESH_FRIEND_NOT_SUPPORTED,
	.gatt_proxy = BT_MESH_GATT_PROXY_NOT_SUPPORTED,
	.default_ttl = 7,

	/* 3 transmissions with 20ms interval */
	.net_transmit = BT_MESH_TRANSMIT(2, 20),
	.relay_retransmit = BT_MESH_TRANSMIT(2, 20),
};

static struct bt_mesh_health_srv health_srv = {
};

static struct bt_mesh_model_pub health_pub = {
	.msg = BT_MESH_HEALTH_FAULT_MSG(0),
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
	SYS_LOG_DBG("Provisioning link opened");
}

static void link_close(bt_mesh_prov_bearer_t bearer)
{
	SYS_LOG_DBG("Provisioning link closed");
}

static int output_number(bt_mesh_output_action_t action, uint32_t number)
{
	struct mesh_out_number_action_ev ev;

	ev.action = sys_cpu_to_le16(action);
	ev.number = sys_cpu_to_le32(number);

	tester_send(BTP_SERVICE_ID_MESH, MESH_EV_OUT_NUMBER_ACTION,
		    CONTROLLER_INDEX, (u8_t *) &ev, sizeof(ev));

	return 0;
}

static int output_string(const char *str)
{
	u8_t string_len = strlen(str);
	u8_t buf[sizeof(struct mesh_out_string_action_ev) + string_len];
	struct mesh_out_string_action_ev *ev = (void *) buf;

	memset(buf, 0, sizeof(buf));

	ev->string_len = string_len;
	strcpy(ev->string, str);

	tester_send(BTP_SERVICE_ID_MESH, MESH_EV_OUT_STRING_ACTION,
		    CONTROLLER_INDEX, buf, sizeof(buf));

	return 0;
}

static int input(bt_mesh_input_action_t action, u8_t size)
{
	struct mesh_in_action_ev ev;

	ev.action = sys_cpu_to_le16(action);
	ev.size = size;

	tester_send(BTP_SERVICE_ID_MESH, MESH_EV_IN_ACTION, CONTROLLER_INDEX,
		    (u8_t *) &ev, sizeof(ev));

	return 0;
}

static void prov_complete(u16_t net_idx, u16_t addr)
{
	SYS_LOG_DBG("Local node provisioned, net_idx 0x%04x address 0x%04x",
		     net_idx, addr);

	tester_send(BTP_SERVICE_ID_MESH, MESH_EV_PROVISIONED, CONTROLLER_INDEX,
		    NULL, 0);
}

static void prov_reset(void)
{
	SYS_LOG_DBG("The local node has been reset and needs reprovisioning");
}

static const struct bt_mesh_comp comp = {
	.cid = CID,
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
		   BTP_STATUS_SUCCESS);
}

static void reset(u8_t *data, u16_t len)
{
	bt_mesh_reset();

	tester_rsp(BTP_SERVICE_ID_MESH, MESH_RESET, CONTROLLER_INDEX,
		   BTP_STATUS_SUCCESS);
}

static void input_number(u8_t *data, u16_t len)
{
	const struct mesh_input_number_cmd *cmd = (void *) data;
	u8_t status = BTP_STATUS_SUCCESS;
	int err;

	err = bt_mesh_input_number(sys_le32_to_cpu(cmd->number));
	if (err) {
		status = BTP_STATUS_FAILED;
	}

	tester_rsp(BTP_SERVICE_ID_MESH, MESH_INPUT_NUMBER, CONTROLLER_INDEX,
		   status);
}

static void input_string(u8_t *data, u16_t len)
{
	const struct mesh_input_string_cmd *cmd = (void *) data;
	char input_str[cmd->string_len + 1];
	u8_t status = BTP_STATUS_SUCCESS;
	int err;

	strncpy(input_str, cmd->string, cmd->string_len);
	input_str[cmd->string_len] = '\0';

	err = bt_mesh_input_string(input_str);
	if (err) {
		status = BTP_STATUS_FAILED;
	}

	tester_rsp(BTP_SERVICE_ID_MESH, MESH_INPUT_STRING, CONTROLLER_INDEX,
		   status);
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
