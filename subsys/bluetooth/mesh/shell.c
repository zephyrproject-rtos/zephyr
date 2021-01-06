/** @file
 *  @brief Bluetooth Mesh shell
 *
 */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <ctype.h>
#include <zephyr.h>
#include <sys/printk.h>
#include <sys/util.h>

#include <shell/shell.h>
#include <settings/settings.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/mesh.h>

/* Private includes for raw Network & Transport layer access */
#include "mesh.h"
#include "net.h"
#include "rpl.h"
#include "transport.h"
#include "foundation.h"
#include "settings.h"

#define CID_NVAL   0xffff

static const struct shell *ctx_shell;

/* Default net, app & dev key values, unless otherwise specified */
static const uint8_t default_key[16] = {
	0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
	0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
};

static struct {
	uint16_t local;
	uint16_t dst;
	uint16_t net_idx;
	uint16_t app_idx;
} net = {
	.local = BT_MESH_ADDR_UNASSIGNED,
	.dst = BT_MESH_ADDR_UNASSIGNED,
};

#define CUR_FAULTS_MAX 4

static uint8_t cur_faults[CUR_FAULTS_MAX];
static uint8_t reg_faults[CUR_FAULTS_MAX * 2];

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
	shell_print(ctx_shell, "Sending current faults");

	*test_id = 0x00;
	*company_id = BT_COMP_ID_LF;

	get_faults(cur_faults, sizeof(cur_faults), faults, fault_count);

	return 0;
}

static int fault_get_reg(struct bt_mesh_model *model, uint16_t cid,
			 uint8_t *test_id, uint8_t *faults, uint8_t *fault_count)
{
	if (cid != BT_COMP_ID_LF) {
		shell_print(ctx_shell, "Faults requested for unknown Company ID"
			    " 0x%04x", cid);
		return -EINVAL;
	}

	shell_print(ctx_shell, "Sending registered faults");

	*test_id = 0x00;

	get_faults(reg_faults, sizeof(reg_faults), faults, fault_count);

	return 0;
}

static int fault_clear(struct bt_mesh_model *model, uint16_t cid)
{
	if (cid != BT_COMP_ID_LF) {
		return -EINVAL;
	}

	(void)memset(reg_faults, 0, sizeof(reg_faults));

	return 0;
}

static int fault_test(struct bt_mesh_model *model, uint8_t test_id,
		      uint16_t cid)
{
	if (cid != BT_COMP_ID_LF) {
		return -EINVAL;
	}

	if (test_id != 0x00) {
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

void show_faults(uint8_t test_id, uint16_t cid, uint8_t *faults, size_t fault_count)
{
	size_t i;

	if (!fault_count) {
		shell_print(ctx_shell, "Health Test ID 0x%02x Company ID "
			    "0x%04x: no faults", test_id, cid);
		return;
	}

	shell_print(ctx_shell, "Health Test ID 0x%02x Company ID 0x%04x Fault "
		    "Count %zu:", test_id, cid, fault_count);

	for (i = 0; i < fault_count; i++) {
		shell_print(ctx_shell, "\t0x%02x", faults[i]);
	}
}

static void health_current_status(struct bt_mesh_health_cli *cli, uint16_t addr,
				  uint8_t test_id, uint16_t cid, uint8_t *faults,
				  size_t fault_count)
{
	shell_print(ctx_shell, "Health Current Status from 0x%04x", addr);
	show_faults(test_id, cid, faults, fault_count);
}

static struct bt_mesh_health_cli health_cli = {
	.current_status = health_current_status,
};

static uint8_t dev_uuid[16] = { 0xdd, 0xdd };

static struct bt_mesh_model root_models[] = {
	BT_MESH_MODEL_CFG_SRV,
	BT_MESH_MODEL_CFG_CLI(&cfg_cli),
	BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub),
	BT_MESH_MODEL_HEALTH_CLI(&health_cli),
};

static struct bt_mesh_elem elements[] = {
	BT_MESH_ELEM(0, root_models, BT_MESH_MODEL_NONE),
};

static const struct bt_mesh_comp comp = {
	.cid = BT_COMP_ID_LF,
	.elem = elements,
	.elem_count = ARRAY_SIZE(elements),
};

static void prov_complete(uint16_t net_idx, uint16_t addr)
{

	shell_print(ctx_shell, "Local node provisioned, net_idx 0x%04x address "
		    "0x%04x", net_idx, addr);

	net.local = addr;
	net.net_idx = net_idx,
	net.dst = addr;
}

static void prov_node_added(uint16_t net_idx, uint8_t uuid[16], uint16_t addr,
			    uint8_t num_elem)
{
	shell_print(ctx_shell, "Node provisioned, net_idx 0x%04x address "
		    "0x%04x elements %d", net_idx, addr, num_elem);

	net.net_idx = net_idx,
	net.dst = addr;
}

static void prov_input_complete(void)
{
	shell_print(ctx_shell, "Input complete");
}

static void prov_reset(void)
{
	shell_print(ctx_shell, "The local node has been reset and needs "
		    "reprovisioning");
}

static int output_number(bt_mesh_output_action_t action, uint32_t number)
{
	shell_print(ctx_shell, "OOB Number: %u", number);
	return 0;
}

static int output_string(const char *str)
{
	shell_print(ctx_shell, "OOB String: %s", str);
	return 0;
}

static bt_mesh_input_action_t input_act;
static uint8_t input_size;

static int cmd_input_num(const struct shell *shell, size_t argc, char *argv[])
{
	int err;

	if (argc < 2) {
		return -EINVAL;
	}

	if (input_act != BT_MESH_ENTER_NUMBER) {
		shell_print(shell, "A number hasn't been requested!");
		return 0;
	}

	if (strlen(argv[1]) < input_size) {
		shell_print(shell, "Too short input (%u digits required)",
			    input_size);
		return 0;
	}

	err = bt_mesh_input_number(strtoul(argv[1], NULL, 10));
	if (err) {
		shell_error(shell, "Numeric input failed (err %d)", err);
		return 0;
	}

	input_act = BT_MESH_NO_INPUT;
	return 0;
}

static int cmd_input_str(const struct shell *shell, size_t argc, char *argv[])
{
	int err;

	if (argc < 2) {
		return -EINVAL;
	}

	if (input_act != BT_MESH_ENTER_STRING) {
		shell_print(shell, "A string hasn't been requested!");
		return 0;
	}

	if (strlen(argv[1]) < input_size) {
		shell_print(shell, "Too short input (%u characters required)",
		       input_size);
		return 0;
	}

	err = bt_mesh_input_string(argv[1]);
	if (err) {
		shell_error(shell, "String input failed (err %d)", err);
		return 0;
	}

	input_act = BT_MESH_NO_INPUT;
	return 0;
}

static int input(bt_mesh_input_action_t act, uint8_t size)
{
	switch (act) {
	case BT_MESH_ENTER_NUMBER:
		shell_print(ctx_shell, "Enter a number (max %u digits) with: "
			    "input-num <num>", size);
		break;
	case BT_MESH_ENTER_STRING:
		shell_print(ctx_shell, "Enter a string (max %u chars) with: "
			    "input-str <str>", size);
		break;
	default:
		shell_error(ctx_shell, "Unknown input action %u (size %u) "
			    "requested!", act, size);
		return -EINVAL;
	}

	input_act = act;
	input_size = size;
	return 0;
}

static const char *bearer2str(bt_mesh_prov_bearer_t bearer)
{
	switch (bearer) {
	case BT_MESH_PROV_ADV:
		return "PB-ADV";
	case BT_MESH_PROV_GATT:
		return "PB-GATT";
	default:
		return "unknown";
	}
}

static void link_open(bt_mesh_prov_bearer_t bearer)
{
	shell_print(ctx_shell, "Provisioning link opened on %s",
		    bearer2str(bearer));
}

static void link_close(bt_mesh_prov_bearer_t bearer)
{
	shell_print(ctx_shell, "Provisioning link closed on %s",
		    bearer2str(bearer));
}

static uint8_t static_val[16];

static struct bt_mesh_prov prov = {
	.uuid = dev_uuid,
	.link_open = link_open,
	.link_close = link_close,
	.complete = prov_complete,
	.node_added = prov_node_added,
	.reset = prov_reset,
	.static_val = NULL,
	.static_val_len = 0,
	.output_size = 6,
	.output_actions = (BT_MESH_DISPLAY_NUMBER | BT_MESH_DISPLAY_STRING),
	.output_number = output_number,
	.output_string = output_string,
	.input_size = 6,
	.input_actions = (BT_MESH_ENTER_NUMBER | BT_MESH_ENTER_STRING),
	.input = input,
	.input_complete = prov_input_complete,
};

static int cmd_static_oob(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc < 2) {
		prov.static_val = NULL;
		prov.static_val_len = 0U;
	} else {
		prov.static_val_len = hex2bin(argv[1], strlen(argv[1]),
					      static_val, 16);
		if (prov.static_val_len) {
			prov.static_val = static_val;
		} else {
			prov.static_val = NULL;
		}
	}

	if (prov.static_val) {
		shell_print(shell, "Static OOB value set (length %u)",
			    prov.static_val_len);
	} else {
		shell_print(shell, "Static OOB value cleared");
	}

	return 0;
}

static int cmd_uuid(const struct shell *shell, size_t argc, char *argv[])
{
	uint8_t uuid[16];
	size_t len;

	if (argc < 2) {
		return -EINVAL;
	}

	len = hex2bin(argv[1], strlen(argv[1]), uuid, sizeof(uuid));
	if (len < 1) {
		return -EINVAL;
	}

	memcpy(dev_uuid, uuid, len);
	(void)memset(dev_uuid + len, 0, sizeof(dev_uuid) - len);

	shell_print(shell, "Device UUID set");

	return 0;
}

static int cmd_reset(const struct shell *shell, size_t argc, char *argv[])
{
	uint16_t addr;
	if (argc < 2) {
		return -EINVAL;
	}

	addr = strtoul(argv[1], NULL, 0);

	if (addr == net.local) {
		bt_mesh_reset();
		shell_print(shell, "Local node reset complete");
	} else {
		int err;
		bool reset = false;

		err = bt_mesh_cfg_node_reset(net.net_idx, net.dst, &reset);
		if (err) {
			shell_error(shell, "Unable to send "
					"Remote Node Reset (err %d)", err);
			return 0;
		}

		shell_print(shell, "Remote node reset complete");
	}

	return 0;
}

static uint8_t str2u8(const char *str)
{
	if (isdigit((unsigned char)str[0])) {
		return strtoul(str, NULL, 0);
	}

	return (!strcmp(str, "on") || !strcmp(str, "enable"));
}

static bool str2bool(const char *str)
{
	return str2u8(str);
}

#if defined(CONFIG_BT_MESH_LOW_POWER)
static int cmd_lpn(const struct shell *shell, size_t argc, char *argv[])
{
	static bool enabled;
	int err;

	if (argc < 2) {
		shell_print(shell, "%s", enabled ? "enabled" : "disabled");
		return 0;
	}

	if (str2bool(argv[1])) {
		if (enabled) {
			shell_print(shell, "LPN already enabled");
			return 0;
		}

		err = bt_mesh_lpn_set(true);
		if (err) {
			shell_error(shell, "Enabling LPN failed (err %d)", err);
		} else {
			enabled = true;
		}
	} else {
		if (!enabled) {
			shell_print(shell, "LPN already disabled");
			return 0;
		}

		err = bt_mesh_lpn_set(false);
		if (err) {
			shell_error(shell, "Enabling LPN failed (err %d)", err);
		} else {
			enabled = false;
		}
	}

	return 0;
}

static int cmd_poll(const struct shell *shell, size_t argc, char *argv[])
{
	int err;

	err = bt_mesh_lpn_poll();
	if (err) {
		shell_error(shell, "Friend Poll failed (err %d)", err);
	}

	return 0;
}

static void lpn_established(uint16_t net_idx, uint16_t friend_addr,
					uint8_t queue_size, uint8_t recv_win)
{
	shell_print(ctx_shell, "Friendship (as LPN) established to "
			"Friend 0x%04x Queue Size %d Receive Window %d",
			friend_addr, queue_size, recv_win);
}

static void lpn_terminated(uint16_t net_idx, uint16_t friend_addr)
{
	shell_print(ctx_shell, "Friendship (as LPN) lost with Friend "
			"0x%04x", friend_addr);
}

BT_MESH_LPN_CB_DEFINE(lpn_cb) = {
	.established = lpn_established,
	.terminated = lpn_terminated,
};

#endif /* MESH_LOW_POWER */

static int cmd_init(const struct shell *shell, size_t argc, char *argv[])
{
	int err;

	err = bt_enable(NULL);
	if (err && err != -EALREADY) {
		shell_error(shell, "Bluetooth init failed (err %d)", err);
		return 0;
	} else if (!err) {
		shell_print(shell, "Bluetooth initialized");
	}

	err = bt_mesh_init(&prov, &comp);
	if (err) {
		shell_error(shell, "Mesh initialization failed (err %d)", err);
	}

	shell_print(shell, "Mesh initialized");

	ctx_shell = shell;

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	if (bt_mesh_is_provisioned()) {
		shell_print(shell, "Mesh network restored from flash");
	} else {
		shell_print(shell, "Use \"pb-adv on\" or \"pb-gatt on\" to "
			    "enable advertising");
	}

	return 0;
}

#if defined(CONFIG_BT_MESH_GATT_PROXY)
static int cmd_ident(const struct shell *shell, size_t argc, char *argv[])
{
	int err;

	err = bt_mesh_proxy_identity_enable();
	if (err) {
		shell_error(shell, "Failed advertise using Node Identity (err "
			    "%d)", err);
	}

	return 0;
}
#endif /* MESH_GATT_PROXY */

static int cmd_get_comp(const struct shell *shell, size_t argc, char *argv[])
{
	NET_BUF_SIMPLE_DEFINE(comp, 32);
	uint8_t status, page = 0x00;
	int err;

	if (argc > 1) {
		page = strtol(argv[1], NULL, 0);
	}

	err = bt_mesh_cfg_comp_data_get(net.net_idx, net.dst, page,
					&status, &comp);
	if (err) {
		shell_error(shell, "Getting composition failed (err %d)", err);
		return 0;
	}

	if (status != 0x00) {
		shell_print(shell, "Got non-success status 0x%02x", status);
		return 0;
	}

	shell_print(shell, "Got Composition Data for 0x%04x:", net.dst);
	shell_print(shell, "\tCID      0x%04x",
		    net_buf_simple_pull_le16(&comp));
	shell_print(shell, "\tPID      0x%04x",
		    net_buf_simple_pull_le16(&comp));
	shell_print(shell, "\tVID      0x%04x",
		    net_buf_simple_pull_le16(&comp));
	shell_print(shell, "\tCRPL     0x%04x",
		    net_buf_simple_pull_le16(&comp));
	shell_print(shell, "\tFeatures 0x%04x",
		    net_buf_simple_pull_le16(&comp));

	while (comp.len > 4) {
		uint8_t sig, vnd;
		uint16_t loc;
		int i;

		loc = net_buf_simple_pull_le16(&comp);
		sig = net_buf_simple_pull_u8(&comp);
		vnd = net_buf_simple_pull_u8(&comp);

		shell_print(shell, "\tElement @ 0x%04x:", loc);

		if (comp.len < ((sig * 2U) + (vnd * 4U))) {
			shell_print(shell, "\t\t...truncated data!");
			break;
		}

		if (sig) {
			shell_print(shell, "\t\tSIG Models:");
		} else {
			shell_print(shell, "\t\tNo SIG Models");
		}

		for (i = 0; i < sig; i++) {
			uint16_t mod_id = net_buf_simple_pull_le16(&comp);

			shell_print(shell, "\t\t\t0x%04x", mod_id);
		}

		if (vnd) {
			shell_print(shell, "\t\tVendor Models:");
		} else {
			shell_print(shell, "\t\tNo Vendor Models");
		}

		for (i = 0; i < vnd; i++) {
			uint16_t cid = net_buf_simple_pull_le16(&comp);
			uint16_t mod_id = net_buf_simple_pull_le16(&comp);

			shell_print(shell, "\t\t\tCompany 0x%04x: 0x%04x", cid,
				    mod_id);
		}
	}

	return 0;
}

static int cmd_dst(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc < 2) {
		shell_print(shell, "Destination address: 0x%04x%s", net.dst,
			    net.dst == net.local ? " (local)" : "");
		return 0;
	}

	if (!strcmp(argv[1], "local")) {
		net.dst = net.local;
	} else {
		net.dst = strtoul(argv[1], NULL, 0);
	}

	shell_print(shell, "Destination address set to 0x%04x%s", net.dst,
		    net.dst == net.local ? " (local)" : "");
	return 0;
}

static int cmd_netidx(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc < 2) {
		shell_print(shell, "NetIdx: 0x%04x", net.net_idx);
		return 0;
	}

	net.net_idx = strtoul(argv[1], NULL, 0);
	shell_print(shell, "NetIdx set to 0x%04x", net.net_idx);
	return 0;
}

static int cmd_appidx(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc < 2) {
		shell_print(shell, "AppIdx: 0x%04x", net.app_idx);
		return 0;
	}

	net.app_idx = strtoul(argv[1], NULL, 0);
	shell_print(shell, "AppIdx set to 0x%04x", net.app_idx);
	return 0;
}

static int cmd_net_send(const struct shell *shell, size_t argc, char *argv[])
{
	NET_BUF_SIMPLE_DEFINE(msg, 32);
	struct bt_mesh_msg_ctx ctx = {
		.send_ttl = BT_MESH_TTL_DEFAULT,
		.net_idx = net.net_idx,
		.addr = net.dst,
		.app_idx = net.app_idx,

	};
	struct bt_mesh_net_tx tx = {
		.ctx = &ctx,
		.src = net.local,
	};
	size_t len;
	int err;

	if (argc < 2) {
		return -EINVAL;
	}

	len = hex2bin(argv[1], strlen(argv[1]),
		      msg.data, net_buf_simple_tailroom(&msg) - 4);
	net_buf_simple_add(&msg, len);

	err = bt_mesh_trans_send(&tx, &msg, NULL, NULL);
	if (err) {
		shell_error(shell, "Failed to send (err %d)", err);
	}

	return 0;
}

#if defined(CONFIG_BT_MESH_IV_UPDATE_TEST)
static int cmd_iv_update(const struct shell *shell, size_t argc, char *argv[])
{
	if (bt_mesh_iv_update()) {
		shell_print(shell, "Transitioned to IV Update In Progress "
			    "state");
	} else {
		shell_print(shell, "Transitioned to IV Update Normal state");
	}

	shell_print(shell, "IV Index is 0x%08x", bt_mesh.iv_index);

	return 0;
}

static int cmd_iv_update_test(const struct shell *shell, size_t argc,
			      char *argv[])
{
	bool enable;

	if (argc < 2) {
		return -EINVAL;
	}

	enable = str2bool(argv[1]);
	if (enable) {
		shell_print(shell, "Enabling IV Update test mode");
	} else {
		shell_print(shell, "Disabling IV Update test mode");
	}

	bt_mesh_iv_update_test(enable);

	return 0;
}
#endif /* CONFIG_BT_MESH_IV_UPDATE_TEST */

static int cmd_rpl_clear(const struct shell *shell, size_t argc, char *argv[])
{
	bt_mesh_rpl_clear();
	return 0;
}

static int cmd_beacon(const struct shell *shell, size_t argc, char *argv[])
{
	uint8_t status;
	int err;

	if (argc < 2) {
		err = bt_mesh_cfg_beacon_get(net.net_idx, net.dst, &status);
	} else {
		uint8_t val = str2u8(argv[1]);

		err = bt_mesh_cfg_beacon_set(net.net_idx, net.dst, val,
					     &status);
	}

	if (err) {
		shell_error(shell, "Unable to send Beacon Get/Set message "
			    "(err %d)", err);
		return 0;
	}

	shell_print(shell, "Beacon state is 0x%02x", status);

	return 0;
}

static void print_unprovisioned_beacon(uint8_t uuid[16],
				       bt_mesh_prov_oob_info_t oob_info,
				       uint32_t *uri_hash)
{
	char uuid_hex_str[32 + 1];

	bin2hex(uuid, 16, uuid_hex_str, sizeof(uuid_hex_str));

	shell_print(ctx_shell, "UUID %s, OOB Info 0x%04x, URI Hash 0x%x",
		    uuid_hex_str, oob_info,
		    (uri_hash == NULL ? 0 : *uri_hash));
}

static int cmd_beacon_listen(const struct shell *shell, size_t argc,
			     char *argv[])
{
	uint8_t val = str2u8(argv[1]);

	if (val) {
		prov.unprovisioned_beacon = print_unprovisioned_beacon;
	} else {
		prov.unprovisioned_beacon = NULL;
	}

	return 0;
}

static int cmd_ttl(const struct shell *shell, size_t argc, char *argv[])
{
	uint8_t ttl;
	int err;

	if (argc < 2) {
		err = bt_mesh_cfg_ttl_get(net.net_idx, net.dst, &ttl);
	} else {
		uint8_t val = strtoul(argv[1], NULL, 0);

		err = bt_mesh_cfg_ttl_set(net.net_idx, net.dst, val, &ttl);
	}

	if (err) {
		shell_error(shell, "Unable to send Default TTL Get/Set "
			    "(err %d)", err);
		return 0;
	}

	shell_print(shell, "Default TTL is 0x%02x", ttl);

	return 0;
}

static int cmd_friend(const struct shell *shell, size_t argc, char *argv[])
{
	uint8_t frnd;
	int err;

	if (argc < 2) {
		err = bt_mesh_cfg_friend_get(net.net_idx, net.dst, &frnd);
	} else {
		uint8_t val = str2u8(argv[1]);

		err = bt_mesh_cfg_friend_set(net.net_idx, net.dst, val, &frnd);
	}

	if (err) {
		shell_error(shell, "Unable to send Friend Get/Set (err %d)",
			    err);
		return 0;
	}

	shell_print(shell, "Friend is set to 0x%02x", frnd);

	return 0;
}

static int cmd_gatt_proxy(const struct shell *shell, size_t argc, char *argv[])
{
	uint8_t proxy;
	int err;

	if (argc < 2) {
		err = bt_mesh_cfg_gatt_proxy_get(net.net_idx, net.dst, &proxy);
	} else {
		uint8_t val = str2u8(argv[1]);

		err = bt_mesh_cfg_gatt_proxy_set(net.net_idx, net.dst, val,
						 &proxy);
	}

	if (err) {
		shell_print(shell, "Unable to send GATT Proxy Get/Set "
			    "(err %d)", err);
		return 0;
	}

	shell_print(shell, "GATT Proxy is set to 0x%02x", proxy);

	return 0;
}

static int cmd_net_transmit(const struct shell *shell,
		size_t argc, char *argv[])
{
	uint8_t transmit;
	int err;

	if (argc < 2) {
		err = bt_mesh_cfg_net_transmit_get(net.net_idx,
				net.dst, &transmit);
	} else {
		if (argc != 3) {
			shell_error(shell, "Wrong number of input arguments"
						"(2 arguments are required)");
			return -EINVAL;
		}

		uint8_t count, interval, new_transmit;

		count = strtoul(argv[1], NULL, 0);
		interval = strtoul(argv[2], NULL, 0);

		new_transmit = BT_MESH_TRANSMIT(count, interval);

		err = bt_mesh_cfg_net_transmit_set(net.net_idx, net.dst,
				new_transmit, &transmit);
	}

	if (err) {
		shell_error(shell, "Unable to send network transmit"
				" Get/Set (err %d)", err);
		return 0;
	}

	shell_print(shell, "Transmit 0x%02x (count %u interval %ums)",
			transmit, BT_MESH_TRANSMIT_COUNT(transmit),
			BT_MESH_TRANSMIT_INT(transmit));

	return 0;
}

static int cmd_relay(const struct shell *shell, size_t argc, char *argv[])
{
	uint8_t relay, transmit;
	int err;

	if (argc < 2) {
		err = bt_mesh_cfg_relay_get(net.net_idx, net.dst, &relay,
					    &transmit);
	} else {
		uint8_t val = str2u8(argv[1]);
		uint8_t count, interval, new_transmit;

		if (val) {
			if (argc > 2) {
				count = strtoul(argv[2], NULL, 0);
			} else {
				count = 2U;
			}

			if (argc > 3) {
				interval = strtoul(argv[3], NULL, 0);
			} else {
				interval = 20U;
			}

			new_transmit = BT_MESH_TRANSMIT(count, interval);
		} else {
			new_transmit = 0U;
		}

		err = bt_mesh_cfg_relay_set(net.net_idx, net.dst, val,
					    new_transmit, &relay, &transmit);
	}

	if (err) {
		shell_error(shell, "Unable to send Relay Get/Set (err %d)",
			    err);
		return 0;
	}

	shell_print(shell, "Relay is 0x%02x, Transmit 0x%02x (count %u interval"
		    " %ums)", relay, transmit, BT_MESH_TRANSMIT_COUNT(transmit),
		    BT_MESH_TRANSMIT_INT(transmit));

	return 0;
}

static int cmd_net_key_add(const struct shell *shell, size_t argc, char *argv[])
{
	bool has_key_val = (argc > 2);
	uint8_t key_val[16];
	uint16_t key_net_idx;
	uint8_t status;
	int err;

	if (argc < 2) {
		return -EINVAL;
	}

	key_net_idx = strtoul(argv[1], NULL, 0);

	if (has_key_val) {
		size_t len;

		len = hex2bin(argv[3], strlen(argv[3]),
			      key_val, sizeof(key_val));
		(void)memset(key_val, 0, sizeof(key_val) - len);
	} else {
		memcpy(key_val, default_key, sizeof(key_val));
	}

	if (IS_ENABLED(CONFIG_BT_MESH_CDB)) {
		struct bt_mesh_cdb_subnet *subnet;

		subnet = bt_mesh_cdb_subnet_get(key_net_idx);
		if (subnet) {
			if (has_key_val) {
				shell_error(shell,
					    "Subnet 0x%03x already has a value",
					    key_net_idx);
				return 0;
			}

			memcpy(key_val, subnet->keys[0].net_key, 16);
		} else {
			subnet = bt_mesh_cdb_subnet_alloc(key_net_idx);
			if (!subnet) {
				shell_error(shell,
					    "No space for subnet in cdb");
				return 0;
			}

			memcpy(subnet->keys[0].net_key, key_val, 16);
			bt_mesh_cdb_subnet_store(subnet);
		}
	}

	err = bt_mesh_cfg_net_key_add(net.net_idx, net.dst, key_net_idx,
				      key_val, &status);
	if (err) {
		shell_print(shell, "Unable to send NetKey Add (err %d)", err);
		return 0;
	}

	if (status) {
		shell_print(shell, "NetKeyAdd failed with status 0x%02x",
			    status);
	} else {
		shell_print(shell, "NetKey added with NetKey Index 0x%03x",
			    key_net_idx);
	}

	return 0;
}

static int cmd_net_key_get(const struct shell *shell, size_t argc, char *argv[])
{
	uint16_t keys[16];
	size_t cnt;
	int err, i;

	cnt = ARRAY_SIZE(keys);

	err = bt_mesh_cfg_net_key_get(net.net_idx, net.dst, keys, &cnt);
	if (err) {
		shell_print(shell, "Unable to send NetKeyGet (err %d)", err);
		return 0;
	}

	shell_print(shell, "NetKeys known by 0x%04x:", net.dst);
	for (i = 0; i < cnt; i++) {
		shell_print(shell, "\t0x%03x", keys[i]);
	}

	return 0;
}

static int cmd_net_key_del(const struct shell *shell, size_t argc, char *argv[])
{
	uint16_t key_net_idx;
	uint8_t status;
	int err;

	key_net_idx = strtoul(argv[1], NULL, 0);

	err = bt_mesh_cfg_net_key_del(net.net_idx, net.dst, key_net_idx,
				      &status);
	if (err) {
		shell_print(shell, "Unable to send NetKeyDel (err %d)", err);
		return 0;
	}

	if (status) {
		shell_print(shell, "NetKeyDel failed with status 0x%02x",
			    status);
	} else {
		shell_print(shell, "NetKey 0x%03x deleted", key_net_idx);
	}

	return 0;
}

static int cmd_app_key_add(const struct shell *shell, size_t argc, char *argv[])
{
	uint8_t key_val[16];
	uint16_t key_net_idx, key_app_idx;
	bool has_key_val = (argc > 3);
	uint8_t status;
	int err;

	if (argc < 3) {
		return -EINVAL;
	}

	key_net_idx = strtoul(argv[1], NULL, 0);
	key_app_idx = strtoul(argv[2], NULL, 0);

	if (has_key_val) {
		size_t len;

		len = hex2bin(argv[3], strlen(argv[3]),
			      key_val, sizeof(key_val));
		(void)memset(key_val, 0, sizeof(key_val) - len);
	} else {
		memcpy(key_val, default_key, sizeof(key_val));
	}

	if (IS_ENABLED(CONFIG_BT_MESH_CDB)) {
		struct bt_mesh_cdb_app_key *app_key;

		app_key = bt_mesh_cdb_app_key_get(key_app_idx);
		if (app_key) {
			if (has_key_val) {
				shell_error(
					shell,
					"App key 0x%03x already has a value",
					key_app_idx);
				return 0;
			}

			memcpy(key_val, app_key->keys[0].app_key, 16);
		} else {
			app_key = bt_mesh_cdb_app_key_alloc(key_net_idx,
							    key_app_idx);
			if (!app_key) {
				shell_error(shell,
					    "No space for app key in cdb");
				return 0;
			}

			memcpy(app_key->keys[0].app_key, key_val, 16);
			bt_mesh_cdb_app_key_store(app_key);
		}
	}

	err = bt_mesh_cfg_app_key_add(net.net_idx, net.dst, key_net_idx,
				      key_app_idx, key_val, &status);
	if (err) {
		shell_error(shell, "Unable to send App Key Add (err %d)", err);
		return 0;
	}

	if (status) {
		shell_print(shell, "AppKeyAdd failed with status 0x%02x",
			    status);
	} else {
		shell_print(shell, "AppKey added, NetKeyIndex 0x%04x "
			    "AppKeyIndex 0x%04x", key_net_idx, key_app_idx);
	}

	return 0;
}

static int cmd_app_key_get(const struct shell *shell, size_t argc, char *argv[])
{
	uint16_t net_idx;
	uint16_t keys[16];
	size_t cnt;
	uint8_t status;
	int err, i;

	net_idx = strtoul(argv[1], NULL, 0);
	cnt = ARRAY_SIZE(keys);

	err = bt_mesh_cfg_app_key_get(net.net_idx, net.dst, net_idx, &status,
				      keys, &cnt);
	if (err) {
		shell_print(shell, "Unable to send AppKeyGet (err %d)", err);
		return 0;
	}

	if (status) {
		shell_print(shell, "AppKeyGet failed with status 0x%02x",
			    status);
		return 0;
	}

	shell_print(shell,
		    "AppKeys for NetKey 0x%03x known by 0x%04x:", net_idx,
		    net.dst);
	for (i = 0; i < cnt; i++) {
		shell_print(shell, "\t0x%03x", keys[i]);
	}

	return 0;
}

static int cmd_app_key_del(const struct shell *shell, size_t argc, char *argv[])
{
	uint16_t key_net_idx, key_app_idx;
	uint8_t status;
	int err;

	if (argc < 3) {
		return -EINVAL;
	}

	key_net_idx = strtoul(argv[1], NULL, 0);
	key_app_idx = strtoul(argv[2], NULL, 0);

	err = bt_mesh_cfg_app_key_del(net.net_idx, net.dst, key_net_idx,
				      key_app_idx, &status);
	if (err) {
		shell_error(shell, "Unable to send App Key del(err %d)", err);
		return 0;
	}

	if (status) {
		shell_print(shell, "AppKeyDel failed with status 0x%02x",
			    status);
	} else {
		shell_print(shell, "AppKey deleted, NetKeyIndex 0x%04x "
			    "AppKeyIndex 0x%04x", key_net_idx, key_app_idx);
	}

	return 0;
}

static int cmd_mod_app_bind(const struct shell *shell, size_t argc,
			    char *argv[])
{
	uint16_t elem_addr, mod_app_idx, mod_id, cid;
	uint8_t status;
	int err;

	if (argc < 4) {
		return -EINVAL;
	}

	elem_addr = strtoul(argv[1], NULL, 0);
	mod_app_idx = strtoul(argv[2], NULL, 0);
	mod_id = strtoul(argv[3], NULL, 0);

	if (argc > 4) {
		cid = strtoul(argv[4], NULL, 0);
		err = bt_mesh_cfg_mod_app_bind_vnd(net.net_idx, net.dst,
						   elem_addr, mod_app_idx,
						   mod_id, cid, &status);
	} else {
		err = bt_mesh_cfg_mod_app_bind(net.net_idx, net.dst, elem_addr,
					       mod_app_idx, mod_id, &status);
	}

	if (err) {
		shell_error(shell, "Unable to send Model App Bind (err %d)",
			    err);
		return 0;
	}

	if (status) {
		shell_print(shell, "Model App Bind failed with status 0x%02x",
			    status);
	} else {
		shell_print(shell, "AppKey successfully bound");
	}

	return 0;
}


static int cmd_mod_app_unbind(const struct shell *shell, size_t argc,
			    char *argv[])
{
	uint16_t elem_addr, mod_app_idx, mod_id, cid;
	uint8_t status;
	int err;

	if (argc < 4) {
		return -EINVAL;
	}

	elem_addr = strtoul(argv[1], NULL, 0);
	mod_app_idx = strtoul(argv[2], NULL, 0);
	mod_id = strtoul(argv[3], NULL, 0);

	if (argc > 4) {
		cid = strtoul(argv[4], NULL, 0);
		err = bt_mesh_cfg_mod_app_unbind_vnd(net.net_idx, net.dst,
						   elem_addr, mod_app_idx,
						   mod_id, cid, &status);
	} else {
		err = bt_mesh_cfg_mod_app_unbind(net.net_idx, net.dst,
				elem_addr, mod_app_idx, mod_id, &status);
	}

	if (err) {
		shell_error(shell, "Unable to send Model App Unbind (err %d)",
			    err);
		return 0;
	}

	if (status) {
		shell_print(shell, "Model App Unbind failed with status 0x%02x",
			    status);
	} else {
		shell_print(shell, "AppKey successfully unbound");
	}

	return 0;
}

static int cmd_mod_app_get(const struct shell *shell, size_t argc,
			      char *argv[])
{
	uint16_t elem_addr, mod_id, cid;
	uint16_t apps[16];
	uint8_t status;
	size_t cnt;
	int err, i;

	elem_addr = strtoul(argv[1], NULL, 0);
	mod_id = strtoul(argv[2], NULL, 0);
	cnt = ARRAY_SIZE(apps);

	if (argc > 3) {
		cid = strtoul(argv[3], NULL, 0);
		err = bt_mesh_cfg_mod_app_get_vnd(net.net_idx, net.dst,
						  elem_addr, mod_id, cid,
						  &status, apps, &cnt);
	} else {
		err = bt_mesh_cfg_mod_app_get(net.net_idx, net.dst, elem_addr,
					      mod_id, &status, apps, &cnt);
	}

	if (err) {
		shell_error(shell, "Unable to send Model App Get (err %d)",
			    err);
		return 0;
	}

	if (status) {
		shell_print(shell, "Model App Get failed with status 0x%02x",
			    status);
	} else {
		shell_print(
			shell,
			"Apps bound to Element 0x%04x, Model 0x%04x %s:",
			elem_addr, mod_id, argc > 3 ? argv[3] : "(SIG)");

		if (!cnt) {
			shell_print(shell, "\tNone.");
		}

		for (i = 0; i < cnt; i++) {
			shell_print(shell, "\t0x%04x", apps[i]);
		}
	}

	return 0;
}

static int cmd_mod_sub_add(const struct shell *shell, size_t argc, char *argv[])
{
	uint16_t elem_addr, sub_addr, mod_id, cid;
	uint8_t status;
	int err;

	if (argc < 4) {
		return -EINVAL;
	}

	elem_addr = strtoul(argv[1], NULL, 0);
	sub_addr = strtoul(argv[2], NULL, 0);
	mod_id = strtoul(argv[3], NULL, 0);

	if (argc > 4) {
		cid = strtoul(argv[4], NULL, 0);
		err = bt_mesh_cfg_mod_sub_add_vnd(net.net_idx, net.dst,
						  elem_addr, sub_addr, mod_id,
						  cid, &status);
	} else {
		err = bt_mesh_cfg_mod_sub_add(net.net_idx, net.dst, elem_addr,
					      sub_addr, mod_id, &status);
	}

	if (err) {
		shell_error(shell, "Unable to send Model Subscription Add "
			    "(err %d)", err);
		return 0;
	}

	if (status) {
		shell_print(shell, "Model Subscription Add failed with status "
			    "0x%02x", status);
	} else {
		shell_print(shell, "Model subscription was successful");
	}

	return 0;
}

static int cmd_mod_sub_del(const struct shell *shell, size_t argc, char *argv[])
{
	uint16_t elem_addr, sub_addr, mod_id, cid;
	uint8_t status;
	int err;

	if (argc < 4) {
		return -EINVAL;
	}

	elem_addr = strtoul(argv[1], NULL, 0);
	sub_addr = strtoul(argv[2], NULL, 0);
	mod_id = strtoul(argv[3], NULL, 0);

	if (argc > 4) {
		cid = strtoul(argv[4], NULL, 0);
		err = bt_mesh_cfg_mod_sub_del_vnd(net.net_idx, net.dst,
						  elem_addr, sub_addr, mod_id,
						  cid, &status);
	} else {
		err = bt_mesh_cfg_mod_sub_del(net.net_idx, net.dst, elem_addr,
					      sub_addr, mod_id, &status);
	}

	if (err) {
		shell_error(shell, "Unable to send Model Subscription Delete "
			    "(err %d)", err);
		return 0;
	}

	if (status) {
		shell_print(shell, "Model Subscription Delete failed with "
			    "status 0x%02x", status);
	} else {
		shell_print(shell, "Model subscription deltion was successful");
	}

	return 0;
}

static int cmd_mod_sub_add_va(const struct shell *shell, size_t argc,
			      char *argv[])
{
	uint16_t elem_addr, sub_addr, mod_id, cid;
	uint8_t label[16];
	uint8_t status;
	size_t len;
	int err;

	if (argc < 4) {
		return -EINVAL;
	}

	elem_addr = strtoul(argv[1], NULL, 0);

	len = hex2bin(argv[2], strlen(argv[2]), label, sizeof(label));
	(void)memset(label + len, 0, sizeof(label) - len);

	mod_id = strtoul(argv[3], NULL, 0);

	if (argc > 4) {
		cid = strtoul(argv[4], NULL, 0);
		err = bt_mesh_cfg_mod_sub_va_add_vnd(net.net_idx, net.dst,
						     elem_addr, label, mod_id,
						     cid, &sub_addr, &status);
	} else {
		err = bt_mesh_cfg_mod_sub_va_add(net.net_idx, net.dst,
						 elem_addr, label, mod_id,
						 &sub_addr, &status);
	}

	if (err) {
		shell_error(shell, "Unable to send Mod Sub VA Add (err %d)",
			    err);
		return 0;
	}

	if (status) {
		shell_print(shell, "Mod Sub VA Add failed with status 0x%02x",
			    status);
	} else {
		shell_print(shell, "0x%04x subscribed to Label UUID %s "
			    "(va 0x%04x)", elem_addr, argv[2], sub_addr);
	}

	return 0;
}

static int cmd_mod_sub_del_va(const struct shell *shell, size_t argc,
			      char *argv[])
{
	uint16_t elem_addr, sub_addr, mod_id, cid;
	uint8_t label[16];
	uint8_t status;
	size_t len;
	int err;

	if (argc < 4) {
		return -EINVAL;
	}

	elem_addr = strtoul(argv[1], NULL, 0);

	len = hex2bin(argv[2], strlen(argv[2]), label, sizeof(label));
	(void)memset(label + len, 0, sizeof(label) - len);

	mod_id = strtoul(argv[3], NULL, 0);

	if (argc > 4) {
		cid = strtoul(argv[4], NULL, 0);
		err = bt_mesh_cfg_mod_sub_va_del_vnd(net.net_idx, net.dst,
						     elem_addr, label, mod_id,
						     cid, &sub_addr, &status);
	} else {
		err = bt_mesh_cfg_mod_sub_va_del(net.net_idx, net.dst,
						 elem_addr, label, mod_id,
						 &sub_addr, &status);
	}

	if (err) {
		shell_error(shell, "Unable to send Model Subscription Delete "
			    "(err %d)", err);
		return 0;
	}

	if (status) {
		shell_print(shell, "Model Subscription Delete failed with "
			    "status 0x%02x", status);
	} else {
		shell_print(shell, "0x%04x unsubscribed from Label UUID %s "
			    "(va 0x%04x)", elem_addr, argv[2], sub_addr);
	}

	return 0;
}

static int cmd_mod_sub_get(const struct shell *shell, size_t argc,
			      char *argv[])
{
	uint16_t elem_addr, mod_id, cid;
	uint16_t subs[16];
	uint8_t status;
	size_t cnt;
	int err, i;

	elem_addr = strtoul(argv[1], NULL, 0);
	mod_id = strtoul(argv[2], NULL, 0);
	cnt = ARRAY_SIZE(subs);

	if (argc > 3) {
		cid = strtoul(argv[3], NULL, 0);
		err = bt_mesh_cfg_mod_sub_get_vnd(net.net_idx, net.dst,
						  elem_addr, mod_id, cid,
						  &status, subs, &cnt);
	} else {
		err = bt_mesh_cfg_mod_sub_get(net.net_idx, net.dst, elem_addr,
					      mod_id, &status, subs, &cnt);
	}

	if (err) {
		shell_error(shell, "Unable to send Model Subscription Get "
			    "(err %d)", err);
		return 0;
	}

	if (status) {
		shell_print(shell, "Model Subscription Get failed with "
			    "status 0x%02x", status);
	} else {
		shell_print(
			shell,
			"Model Subscriptions for Element 0x%04x, "
			"Model 0x%04x %s:",
			elem_addr, mod_id, argc > 3 ? argv[3] : "(SIG)");

		if (!cnt) {
			shell_print(shell, "\tNone.");
		}

		for (i = 0; i < cnt; i++) {
			shell_print(shell, "\t0x%04x", subs[i]);
		}
	}

	return 0;
}

static int mod_pub_get(const struct shell *shell, uint16_t addr, uint16_t mod_id,
		       uint16_t cid)
{
	struct bt_mesh_cfg_mod_pub pub;
	uint8_t status;
	int err;

	if (cid == CID_NVAL) {
		err = bt_mesh_cfg_mod_pub_get(net.net_idx, net.dst, addr,
					      mod_id, &pub, &status);
	} else {
		err = bt_mesh_cfg_mod_pub_get_vnd(net.net_idx, net.dst, addr,
						  mod_id, cid, &pub, &status);
	}

	if (err) {
		shell_error(shell, "Model Publication Get failed (err %d)",
			    err);
		return 0;
	}

	if (status) {
		shell_print(shell, "Model Publication Get failed "
			    "(status 0x%02x)", status);
		return 0;
	}

	shell_print(shell, "Model Publication for Element 0x%04x, Model 0x%04x:"
		    "\tPublish Address:                0x%04x"
		    "\tAppKeyIndex:                    0x%04x"
		    "\tCredential Flag:                %u"
		    "\tPublishTTL:                     %u"
		    "\tPublishPeriod:                  0x%02x"
		    "\tPublishRetransmitCount:         %u"
		    "\tPublishRetransmitInterval:      %ums",
		    addr, mod_id, pub.addr, pub.app_idx, pub.cred_flag, pub.ttl,
		    pub.period, BT_MESH_PUB_TRANSMIT_COUNT(pub.transmit),
		    BT_MESH_PUB_TRANSMIT_INT(pub.transmit));

	return 0;
}

static int mod_pub_set(const struct shell *shell, uint16_t addr, uint16_t mod_id,
		       uint16_t cid, char *argv[])
{
	struct bt_mesh_cfg_mod_pub pub;
	uint8_t status, count;
	uint16_t interval;
	int err;

	pub.addr = strtoul(argv[0], NULL, 0);
	pub.app_idx = strtoul(argv[1], NULL, 0);
	pub.cred_flag = str2bool(argv[2]);
	pub.ttl = strtoul(argv[3], NULL, 0);
	pub.period = strtoul(argv[4], NULL, 0);

	count = strtoul(argv[5], NULL, 0);
	if (count > 7) {
		shell_print(shell, "Invalid retransmit count");
		return -EINVAL;
	}

	interval = strtoul(argv[6], NULL, 0);
	if (interval > (31 * 50) || (interval % 50)) {
		shell_print(shell, "Invalid retransmit interval %u", interval);
		return -EINVAL;
	}

	pub.transmit = BT_MESH_PUB_TRANSMIT(count, interval);

	if (cid == CID_NVAL) {
		err = bt_mesh_cfg_mod_pub_set(net.net_idx, net.dst, addr,
					      mod_id, &pub, &status);
	} else {
		err = bt_mesh_cfg_mod_pub_set_vnd(net.net_idx, net.dst, addr,
						  mod_id, cid, &pub, &status);
	}

	if (err) {
		shell_error(shell, "Model Publication Set failed (err %d)",
			    err);
		return 0;
	}

	if (status) {
		shell_print(shell, "Model Publication Set failed "
			    "(status 0x%02x)", status);
	} else {
		shell_print(shell, "Model Publication successfully set");
	}

	return 0;
}

static int cmd_mod_pub(const struct shell *shell, size_t argc, char *argv[])
{
	uint16_t addr, mod_id, cid;

	if (argc < 3) {
		return -EINVAL;
	}

	addr = strtoul(argv[1], NULL, 0);
	mod_id = strtoul(argv[2], NULL, 0);

	argc -= 3;
	argv += 3;

	if (argc == 1 || argc == 8) {
		cid = strtoul(argv[0], NULL, 0);
		argc--;
		argv++;
	} else {
		cid = CID_NVAL;
	}

	if (argc > 0) {
		if (argc < 7) {
			return -EINVAL;
		}

		return mod_pub_set(shell, addr, mod_id, cid, argv);
	} else {
		return mod_pub_get(shell, addr, mod_id, cid);
	}
}

static void hb_sub_print(const struct shell *shell,
			 struct bt_mesh_cfg_hb_sub *sub)
{
	shell_print(shell, "Heartbeat Subscription:"
		    "\tSource:      0x%04x"
		    "\tDestination: 0x%04x"
		    "\tPeriodLog:   0x%02x"
		    "\tCountLog:    0x%02x"
		    "\tMinHops:     %u"
		    "\tMaxHops:     %u",
		    sub->src, sub->dst, sub->period, sub->count,
		    sub->min, sub->max);
}

static int hb_sub_get(const struct shell *shell, size_t argc, char *argv[])
{
	struct bt_mesh_cfg_hb_sub sub;
	uint8_t status;
	int err;

	err = bt_mesh_cfg_hb_sub_get(net.net_idx, net.dst, &sub, &status);
	if (err) {
		shell_error(shell, "Heartbeat Subscription Get failed (err %d)",
			    err);
		return 0;
	}

	if (status) {
		shell_print(shell, "Heartbeat Subscription Get failed "
			    "(status 0x%02x)", status);
	} else {
		hb_sub_print(shell, &sub);
	}

	return 0;
}

static int hb_sub_set(const struct shell *shell, size_t argc, char *argv[])
{
	struct bt_mesh_cfg_hb_sub sub;
	uint8_t status;
	int err;

	sub.src = strtoul(argv[1], NULL, 0);
	sub.dst = strtoul(argv[2], NULL, 0);
	sub.period = strtoul(argv[3], NULL, 0);

	err = bt_mesh_cfg_hb_sub_set(net.net_idx, net.dst, &sub, &status);
	if (err) {
		shell_error(shell, "Heartbeat Subscription Set failed (err %d)",
			    err);
		return 0;
	}

	if (status) {
		shell_print(shell, "Heartbeat Subscription Set failed "
			    "(status 0x%02x)", status);
	} else {
		hb_sub_print(shell, &sub);
	}

	return 0;
}

static int cmd_hb_sub(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc > 1) {
		if (argc < 4) {
			return -EINVAL;
		}

		return hb_sub_set(shell, argc, argv);
	} else {
		return hb_sub_get(shell, argc, argv);
	}
}

static int hb_pub_get(const struct shell *shell, size_t argc, char *argv[])
{
	struct bt_mesh_cfg_hb_pub pub;
	uint8_t status;
	int err;

	err = bt_mesh_cfg_hb_pub_get(net.net_idx, net.dst, &pub, &status);
	if (err) {
		shell_error(shell, "Heartbeat Publication Get failed (err %d)",
			    err);
		return 0;
	}

	if (status) {
		shell_print(shell, "Heartbeat Publication Get failed "
			    "(status 0x%02x)", status);
		return 0;
	}

	shell_print(shell, "Heartbeat publication:");
	shell_print(shell, "\tdst 0x%04x count 0x%02x period 0x%02x",
		    pub.dst, pub.count, pub.period);
	shell_print(shell, "\tttl 0x%02x feat 0x%04x net_idx 0x%04x",
		    pub.ttl, pub.feat, pub.net_idx);

	return 0;
}

static int hb_pub_set(const struct shell *shell, size_t argc, char *argv[])
{
	struct bt_mesh_cfg_hb_pub pub;
	uint8_t status;
	int err;

	pub.dst = strtoul(argv[1], NULL, 0);
	pub.count = strtoul(argv[2], NULL, 0);
	pub.period = strtoul(argv[3], NULL, 0);
	pub.ttl = strtoul(argv[4], NULL, 0);
	pub.feat = strtoul(argv[5], NULL, 0);
	pub.net_idx = strtoul(argv[5], NULL, 0);

	err = bt_mesh_cfg_hb_pub_set(net.net_idx, net.dst, &pub, &status);
	if (err) {
		shell_error(shell, "Heartbeat Publication Set failed (err %d)",
			    err);
		return 0;
	}

	if (status) {
		shell_print(shell, "Heartbeat Publication Set failed "
			    "(status 0x%02x)", status);
	} else {
		shell_print(shell, "Heartbeat publication successfully set");
	}

	return 0;
}

static int cmd_hb_pub(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc > 1) {
		if (argc < 7) {
			return -EINVAL;
		}

		return hb_pub_set(shell, argc, argv);
	} else {
		return hb_pub_get(shell, argc, argv);
	}
}

#if defined(CONFIG_BT_MESH_PROV_DEVICE)
static int cmd_pb(bt_mesh_prov_bearer_t bearer, const struct shell *shell,
		  size_t argc, char *argv[])
{
	int err;

	if (argc < 2) {
		return -EINVAL;
	}

	if (str2bool(argv[1])) {
		err = bt_mesh_prov_enable(bearer);
		if (err) {
			shell_error(shell, "Failed to enable %s (err %d)",
				    bearer2str(bearer), err);
		} else {
			shell_print(shell, "%s enabled", bearer2str(bearer));
		}
	} else {
		err = bt_mesh_prov_disable(bearer);
		if (err) {
			shell_error(shell, "Failed to disable %s (err %d)",
				    bearer2str(bearer), err);
		} else {
			shell_print(shell, "%s disabled", bearer2str(bearer));
		}
	}

	return 0;
}
#endif

#if defined(CONFIG_BT_MESH_PB_ADV)
static int cmd_pb_adv(const struct shell *shell, size_t argc, char *argv[])
{
	return cmd_pb(BT_MESH_PROV_ADV, shell, argc, argv);
}

#if defined(CONFIG_BT_MESH_PROVISIONER)
static int cmd_provision_adv(const struct shell *shell, size_t argc,
			     char *argv[])
{
	uint8_t uuid[16];
	uint8_t attention_duration;
	uint16_t net_idx;
	uint16_t addr;
	size_t len;
	int err;

	len = hex2bin(argv[1], strlen(argv[1]), uuid, sizeof(uuid));
	(void)memset(uuid + len, 0, sizeof(uuid) - len);

	net_idx = strtoul(argv[2], NULL, 0);
	addr = strtoul(argv[3], NULL, 0);
	attention_duration = strtoul(argv[4], NULL, 0);

	err = bt_mesh_provision_adv(uuid, net_idx, addr, attention_duration);
	if (err) {
		shell_error(shell, "Provisioning failed (err %d)", err);
	}

	return 0;
}
#endif /* CONFIG_BT_MESH_PROVISIONER */

#endif /* CONFIG_BT_MESH_PB_ADV */

#if defined(CONFIG_BT_MESH_PB_GATT)
static int cmd_pb_gatt(const struct shell *shell, size_t argc, char *argv[])
{
	return cmd_pb(BT_MESH_PROV_GATT, shell, argc, argv);
}
#endif /* CONFIG_BT_MESH_PB_GATT */

static int cmd_provision(const struct shell *shell, size_t argc, char *argv[])
{
	const uint8_t *net_key = default_key;
	uint16_t net_idx, addr;
	uint32_t iv_index;
	int err;

	if (argc < 3) {
		return -EINVAL;
	}

	net_idx = strtoul(argv[1], NULL, 0);
	addr = strtoul(argv[2], NULL, 0);

	if (argc > 3) {
		iv_index = strtoul(argv[3], NULL, 0);
	} else {
		iv_index = 0U;
	}

	if (IS_ENABLED(CONFIG_BT_MESH_CDB)) {
		const struct bt_mesh_cdb_subnet *sub;

		sub = bt_mesh_cdb_subnet_get(net_idx);
		if (!sub) {
			shell_error(shell, "No cdb entry for subnet 0x%03x",
				    net_idx);
			return 0;
		}

		net_key = sub->keys[sub->kr_flag].net_key;
	}

	err = bt_mesh_provision(net_key, net_idx, 0, iv_index, addr,
				default_key);
	if (err) {
		shell_error(shell, "Provisioning failed (err %d)", err);
	}

	return 0;
}

int cmd_timeout(const struct shell *shell, size_t argc, char *argv[])
{
	int32_t timeout_ms;

	if (argc == 2) {
		int32_t timeout_s;

		timeout_s = strtol(argv[1], NULL, 0);
		if (timeout_s < 0 || timeout_s > (INT32_MAX / 1000)) {
			timeout_ms = SYS_FOREVER_MS;
		} else {
			timeout_ms = timeout_s * MSEC_PER_SEC;
		}

		bt_mesh_cfg_cli_timeout_set(timeout_ms);
	}

	timeout_ms = bt_mesh_cfg_cli_timeout_get();
	if (timeout_ms == SYS_FOREVER_MS) {
		shell_print(shell, "Message timeout: forever");
	} else {
		shell_print(shell, "Message timeout: %u seconds",
			    timeout_ms / 1000);
	}

	return 0;
}

static int cmd_fault_get(const struct shell *shell, size_t argc, char *argv[])
{
	uint8_t faults[32];
	size_t fault_count;
	uint8_t test_id;
	uint16_t cid;
	int err;

	cid = strtoul(argv[1], NULL, 0);
	fault_count = sizeof(faults);

	err = bt_mesh_health_fault_get(net.dst, net.app_idx, cid, &test_id,
				 faults, &fault_count);
	if (err) {
		shell_error(shell, "Failed to send Health Fault Get (err %d)",
			    err);
	} else {
		show_faults(test_id, cid, faults, fault_count);
	}

	return 0;
}

static int cmd_fault_clear(const struct shell *shell, size_t argc,
			   char *argv[])
{
	uint8_t faults[32];
	size_t fault_count;
	uint8_t test_id;
	uint16_t cid;
	int err;

	if (argc < 2) {
		return -EINVAL;
	}

	cid = strtoul(argv[1], NULL, 0);
	fault_count = sizeof(faults);

	err = bt_mesh_health_fault_clear(net.dst, net.app_idx, cid,
				 &test_id, faults, &fault_count);
	if (err) {
		shell_error(shell, "Failed to send Health Fault Clear (err %d)",
			    err);
	} else {
		show_faults(test_id, cid, faults, fault_count);
	}

	return 0;
}

static int cmd_fault_clear_unack(const struct shell *shell, size_t argc,
				 char *argv[])
{
	uint16_t cid;
	int err;

	if (argc < 2) {
		return -EINVAL;
	}

	cid = strtoul(argv[1], NULL, 0);

	err = bt_mesh_health_fault_clear(net.dst, net.app_idx, cid,
					 NULL, NULL, NULL);
	if (err) {
		shell_error(shell, "Health Fault Clear Unacknowledged failed "
			    "(err %d)", err);
	}

	return 0;
}

static int cmd_fault_test(const struct shell *shell, size_t argc, char *argv[])
{
	uint8_t faults[32];
	size_t fault_count;
	uint8_t test_id;
	uint16_t cid;
	int err;

	if (argc < 3) {
		return -EINVAL;
	}

	cid = strtoul(argv[1], NULL, 0);
	test_id = strtoul(argv[2], NULL, 0);
	fault_count = sizeof(faults);

	err = bt_mesh_health_fault_test(net.dst, net.app_idx, cid,
				 test_id, faults, &fault_count);
	if (err) {
		shell_error(shell, "Failed to send Health Fault Test (err %d)",
			    err);
	} else {
		show_faults(test_id, cid, faults, fault_count);
	}

	return 0;
}

static int cmd_fault_test_unack(const struct shell *shell, size_t argc,
				char *argv[])
{
	uint16_t cid;
	uint8_t test_id;
	int err;

	if (argc < 3) {
		return -EINVAL;
	}

	cid = strtoul(argv[1], NULL, 0);
	test_id = strtoul(argv[2], NULL, 0);

	err = bt_mesh_health_fault_test(net.dst, net.app_idx, cid,
				 test_id, NULL, NULL);
	if (err) {
		shell_error(shell, "Health Fault Test Unacknowledged failed "
			    "(err %d)", err);
	}

	return 0;
}

static int cmd_period_get(const struct shell *shell, size_t argc, char *argv[])
{
	uint8_t divisor;
	int err;

	err = bt_mesh_health_period_get(net.dst, net.app_idx, &divisor);
	if (err) {
		shell_error(shell, "Failed to send Health Period Get (err %d)",
			    err);
	} else {
		shell_print(shell, "Health FastPeriodDivisor: %u", divisor);
	}

	return 0;
}

static int cmd_period_set(const struct shell *shell, size_t argc, char *argv[])
{
	uint8_t divisor, updated_divisor;
	int err;

	if (argc < 2) {
		return -EINVAL;
	}

	divisor = strtoul(argv[1], NULL, 0);

	err = bt_mesh_health_period_set(net.dst, net.app_idx, divisor,
				 &updated_divisor);
	if (err) {
		shell_error(shell, "Failed to send Health Period Set (err %d)",
			    err);
	} else {
		shell_print(shell, "Health FastPeriodDivisor: %u",
			    updated_divisor);
	}

	return 0;
}

static int cmd_period_set_unack(const struct shell *shell, size_t argc,
				char *argv[])
{
	uint8_t divisor;
	int err;

	if (argc < 2) {
		return -EINVAL;
	}

	divisor = strtoul(argv[1], NULL, 0);

	err = bt_mesh_health_period_set(net.dst, net.app_idx, divisor, NULL);
	if (err) {
		shell_print(shell, "Failed to send Health Period Set (err %d)",
			    err);
	}

	return 0;
}

static int cmd_attention_get(const struct shell *shell, size_t argc,
			     char *argv[])
{
	uint8_t attention;
	int err;

	err = bt_mesh_health_attention_get(net.dst, net.app_idx,
					   &attention);
	if (err) {
		shell_error(shell, "Failed to send Health Attention Get "
			    "(err %d)", err);
	} else {
		shell_print(shell, "Health Attention Timer: %u", attention);
	}

	return 0;
}

static int cmd_attention_set(const struct shell *shell, size_t argc,
			     char *argv[])
{
	uint8_t attention, updated_attention;
	int err;

	if (argc < 2) {
		return -EINVAL;
	}

	attention = strtoul(argv[1], NULL, 0);

	err = bt_mesh_health_attention_set(net.dst, net.app_idx, attention,
				 &updated_attention);
	if (err) {
		shell_error(shell, "Failed to send Health Attention Set "
			    "(err %d)", err);
	} else {
		shell_print(shell, "Health Attention Timer: %u",
			    updated_attention);
	}

	return 0;
}

static int cmd_attention_set_unack(const struct shell *shell, size_t argc,
				   char *argv[])
{
	uint8_t attention;
	int err;

	if (argc < 2) {
		return -EINVAL;
	}

	attention = strtoul(argv[1], NULL, 0);

	err = bt_mesh_health_attention_set(net.dst, net.app_idx, attention,
				 NULL);
	if (err) {
		shell_error(shell, "Failed to send Health Attention Set "
			    "(err %d)", err);
	}

	return 0;
}

static int cmd_add_fault(const struct shell *shell, size_t argc, char *argv[])
{
	uint8_t fault_id;
	uint8_t i;

	if (argc < 2) {
		return -EINVAL;
	}

	fault_id = strtoul(argv[1], NULL, 0);
	if (!fault_id) {
		shell_print(shell, "The Fault ID must be non-zero!");
		return -EINVAL;
	}

	for (i = 0U; i < sizeof(cur_faults); i++) {
		if (!cur_faults[i]) {
			cur_faults[i] = fault_id;
			break;
		}
	}

	if (i == sizeof(cur_faults)) {
		shell_print(shell, "Fault array is full. Use \"del-fault\" to "
			    "clear it");
		return 0;
	}

	for (i = 0U; i < sizeof(reg_faults); i++) {
		if (!reg_faults[i]) {
			reg_faults[i] = fault_id;
			break;
		}
	}

	if (i == sizeof(reg_faults)) {
		shell_print(shell, "No space to store more registered faults");
	}

	bt_mesh_fault_update(&elements[0]);

	return 0;
}

static int cmd_del_fault(const struct shell *shell, size_t argc, char *argv[])
{
	uint8_t fault_id;
	uint8_t i;

	if (argc < 2) {
		(void)memset(cur_faults, 0, sizeof(cur_faults));
		shell_print(shell, "All current faults cleared");
		bt_mesh_fault_update(&elements[0]);
		return 0;
	}

	fault_id = strtoul(argv[1], NULL, 0);
	if (!fault_id) {
		shell_print(shell, "The Fault ID must be non-zero!");
		return -EINVAL;
	}

	for (i = 0U; i < sizeof(cur_faults); i++) {
		if (cur_faults[i] == fault_id) {
			cur_faults[i] = 0U;
			shell_print(shell, "Fault cleared");
		}
	}

	bt_mesh_fault_update(&elements[0]);

	return 0;
}

#if defined(CONFIG_BT_MESH_CDB)
static int cmd_cdb_create(const struct shell *shell, size_t argc,
			  char *argv[])
{
	uint8_t net_key[16];
	size_t len;
	int err;

	if (argc < 2) {
		bt_rand(net_key, 16);
	} else {
		len = hex2bin(argv[1], strlen(argv[1]), net_key,
			      sizeof(net_key));
		memset(net_key + len, 0, sizeof(net_key) - len);
	}

	err = bt_mesh_cdb_create(net_key);
	if (err < 0) {
		shell_print(shell, "Failed to create CDB (err %d)", err);
	}

	return 0;
}

static int cmd_cdb_clear(const struct shell *shell, size_t argc,
			 char *argv[])
{
	bt_mesh_cdb_clear();

	shell_print(shell, "Cleared CDB");

	return 0;
}

static void cdb_print_nodes(const struct shell *shell)
{
	char key_hex_str[32 + 1], uuid_hex_str[32 + 1];
	struct bt_mesh_cdb_node *node;
	int i, total = 0;
	bool configured;

	shell_print(shell, "Address  Elements  Flags  %-32s  DevKey", "UUID");

	for (i = 0; i < ARRAY_SIZE(bt_mesh_cdb.nodes); ++i) {
		node = &bt_mesh_cdb.nodes[i];
		if (node->addr == BT_MESH_ADDR_UNASSIGNED) {
			continue;
		}

		configured = atomic_test_bit(node->flags,
					     BT_MESH_CDB_NODE_CONFIGURED);

		total++;
		bin2hex(node->uuid, 16, uuid_hex_str, sizeof(uuid_hex_str));
		bin2hex(node->dev_key, 16, key_hex_str, sizeof(key_hex_str));
		shell_print(shell, "0x%04x   %-8d  %-5s  %s  %s", node->addr,
			    node->num_elem, configured ? "C" : "-",
			    uuid_hex_str, key_hex_str);
	}

	shell_print(shell, "> Total nodes: %d", total);
}

static void cdb_print_subnets(const struct shell *shell)
{
	struct bt_mesh_cdb_subnet *subnet;
	char key_hex_str[32 + 1];
	int i, total = 0;

	shell_print(shell, "NetIdx  NetKey");

	for (i = 0; i < ARRAY_SIZE(bt_mesh_cdb.subnets); ++i) {
		subnet = &bt_mesh_cdb.subnets[i];
		if (subnet->net_idx == BT_MESH_KEY_UNUSED) {
			continue;
		}

		total++;
		bin2hex(subnet->keys[0].net_key, 16, key_hex_str,
			sizeof(key_hex_str));
		shell_print(shell, "0x%03x   %s", subnet->net_idx,
			    key_hex_str);
	}

	shell_print(shell, "> Total subnets: %d", total);
}

static void cdb_print_app_keys(const struct shell *shell)
{
	struct bt_mesh_cdb_app_key *app_key;
	char key_hex_str[32 + 1];
	int i, total = 0;

	shell_print(shell, "NetIdx  AppIdx  AppKey");

	for (i = 0; i < ARRAY_SIZE(bt_mesh_cdb.app_keys); ++i) {
		app_key = &bt_mesh_cdb.app_keys[i];
		if (app_key->net_idx == BT_MESH_KEY_UNUSED) {
			continue;
		}

		total++;
		bin2hex(app_key->keys[0].app_key, 16, key_hex_str,
			sizeof(key_hex_str));
		shell_print(shell, "0x%03x   0x%03x   %s",
			    app_key->net_idx, app_key->app_idx, key_hex_str);
	}

	shell_print(shell, "> Total app-keys: %d", total);
}

static int cmd_cdb_show(const struct shell *shell, size_t argc,
			char *argv[])
{
	if (!atomic_test_bit(bt_mesh_cdb.flags, BT_MESH_CDB_VALID)) {
		shell_print(shell, "No valid networks");
		return 0;
	}

	shell_print(shell, "Mesh Network Information");
	shell_print(shell, "========================");

	cdb_print_nodes(shell);
	shell_print(shell, "---");
	cdb_print_subnets(shell);
	shell_print(shell, "---");
	cdb_print_app_keys(shell);

	return 0;
}

static int cmd_cdb_node_add(const struct shell *shell, size_t argc,
			    char *argv[])
{
	struct bt_mesh_cdb_node *node;
	uint8_t uuid[16], dev_key[16];
	uint16_t addr, net_idx;
	uint8_t num_elem;
	size_t len;

	len = hex2bin(argv[1], strlen(argv[1]), uuid, sizeof(uuid));
	memset(uuid + len, 0, sizeof(uuid) - len);

	addr = strtoul(argv[2], NULL, 0);
	num_elem = strtoul(argv[3], NULL, 0);
	net_idx = strtoul(argv[4], NULL, 0);

	if (argc < 6) {
		bt_rand(dev_key, 16);
	} else {
		len = hex2bin(argv[5], strlen(argv[5]), dev_key,
			      sizeof(dev_key));
		memset(dev_key + len, 0, sizeof(dev_key) - len);
	}

	node = bt_mesh_cdb_node_alloc(uuid, addr, num_elem, net_idx);
	if (node == NULL) {
		shell_print(shell, "Failed to allocate node");
		return 0;
	}

	memcpy(node->dev_key, dev_key, 16);

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		bt_mesh_cdb_node_store(node);
	}

	shell_print(shell, "Added node 0x%04x", addr);

	return 0;
}

static int cmd_cdb_node_del(const struct shell *shell, size_t argc,
			    char *argv[])
{
	struct bt_mesh_cdb_node *node;
	uint16_t addr;

	addr = strtoul(argv[1], NULL, 0);

	node = bt_mesh_cdb_node_get(addr);
	if (node == NULL) {
		shell_print(shell, "No node with address 0x%04x", addr);
		return 0;
	}

	bt_mesh_cdb_node_del(node, true);

	shell_print(shell, "Deleted node 0x%04x", addr);

	return 0;
}

static int cmd_cdb_subnet_add(const struct shell *shell, size_t argc,
			     char *argv[])
{
	struct bt_mesh_cdb_subnet *sub;
	uint8_t net_key[16];
	uint16_t net_idx;
	size_t len;

	net_idx = strtoul(argv[1], NULL, 0);

	if (argc < 3) {
		bt_rand(net_key, 16);
	} else {
		len = hex2bin(argv[2], strlen(argv[2]), net_key,
			      sizeof(net_key));
		memset(net_key + len, 0, sizeof(net_key) - len);
	}

	sub = bt_mesh_cdb_subnet_alloc(net_idx);
	if (sub == NULL) {
		shell_print(shell, "Could not add subnet");
		return 0;
	}

	memcpy(sub->keys[0].net_key, net_key, 16);

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		bt_mesh_cdb_subnet_store(sub);
	}

	shell_print(shell, "Added Subnet 0x%03x", net_idx);

	return 0;
}

static int cmd_cdb_subnet_del(const struct shell *shell, size_t argc,
			     char *argv[])
{
	struct bt_mesh_cdb_subnet *sub;
	uint16_t net_idx;

	net_idx = strtoul(argv[1], NULL, 0);

	sub = bt_mesh_cdb_subnet_get(net_idx);
	if (sub == NULL) {
		shell_print(shell, "No subnet with NetIdx 0x%03x", net_idx);
		return 0;
	}

	bt_mesh_cdb_subnet_del(sub, true);

	shell_print(shell, "Deleted subnet 0x%03x", net_idx);

	return 0;
}

static int cmd_cdb_app_key_add(const struct shell *shell, size_t argc,
			      char *argv[])
{
	struct bt_mesh_cdb_app_key *key;
	uint16_t net_idx, app_idx;
	uint8_t app_key[16];
	size_t len;

	net_idx = strtoul(argv[1], NULL, 0);
	app_idx = strtoul(argv[2], NULL, 0);

	if (argc < 4) {
		bt_rand(app_key, 16);
	} else {
		len = hex2bin(argv[3], strlen(argv[3]), app_key,
			      sizeof(app_key));
		memset(app_key + len, 0, sizeof(app_key) - len);
	}

	key = bt_mesh_cdb_app_key_alloc(net_idx, app_idx);
	if (key == NULL) {
		shell_print(shell, "Could not add AppKey");
		return 0;
	}

	memcpy(key->keys[0].app_key, app_key, 16);

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		bt_mesh_cdb_app_key_store(key);
	}

	shell_print(shell, "Added AppKey 0x%03x", app_idx);

	return 0;
}

static int cmd_cdb_app_key_del(const struct shell *shell, size_t argc,
			      char *argv[])
{
	struct bt_mesh_cdb_app_key *key;
	uint16_t app_idx;

	app_idx = strtoul(argv[1], NULL, 0);

	key = bt_mesh_cdb_app_key_get(app_idx);
	if (key == NULL) {
		shell_print(shell, "No AppKey 0x%03x", app_idx);
		return 0;
	}

	bt_mesh_cdb_app_key_del(key, true);

	shell_print(shell, "Deleted AppKey 0x%03x", app_idx);

	return 0;
}
#endif

/* List of Mesh subcommands.
 *
 * Each command is documented in doc/reference/bluetooth/mesh/shell.rst.
 *
 * Please keep the documentation up to date by adding any new commands to the
 * list.
 */
SHELL_STATIC_SUBCMD_SET_CREATE(mesh_cmds,
	/* General operations */
	SHELL_CMD_ARG(init, NULL, NULL, cmd_init, 1, 0),
	SHELL_CMD_ARG(reset, NULL, "<addr>", cmd_reset, 2, 0),
#if defined(CONFIG_BT_MESH_LOW_POWER)
	SHELL_CMD_ARG(lpn, NULL, "<value: off, on>", cmd_lpn, 2, 0),
	SHELL_CMD_ARG(poll, NULL, NULL, cmd_poll, 1, 0),
#endif
#if defined(CONFIG_BT_MESH_GATT_PROXY)
	SHELL_CMD_ARG(ident, NULL, NULL, cmd_ident, 1, 0),
#endif
	SHELL_CMD_ARG(dst, NULL, "[destination address]", cmd_dst, 1, 1),
	SHELL_CMD_ARG(netidx, NULL, "[NetIdx]", cmd_netidx, 1, 1),
	SHELL_CMD_ARG(appidx, NULL, "[AppIdx]", cmd_appidx, 1, 1),

	/* Commands which access internal APIs, for testing only */
	SHELL_CMD_ARG(net-send, NULL, "<hex string>", cmd_net_send, 2, 0),
#if defined(CONFIG_BT_MESH_IV_UPDATE_TEST)
	SHELL_CMD_ARG(iv-update, NULL, NULL, cmd_iv_update, 1, 0),
	SHELL_CMD_ARG(iv-update-test, NULL, "<value: off, on>",
		      cmd_iv_update_test, 2, 0),
#endif
	SHELL_CMD_ARG(rpl-clear, NULL, NULL, cmd_rpl_clear, 1, 0),

	/* Provisioning operations */
#if defined(CONFIG_BT_MESH_PB_GATT)
	SHELL_CMD_ARG(pb-gatt, NULL, "<val: off, on>", cmd_pb_gatt, 2, 0),
#endif
#if defined(CONFIG_BT_MESH_PB_ADV)
	SHELL_CMD_ARG(pb-adv, NULL, "<val: off, on>", cmd_pb_adv, 2, 0),
#if defined(CONFIG_BT_MESH_PROVISIONER)
	SHELL_CMD_ARG(provision-adv, NULL, "<UUID> <NetKeyIndex> <addr> "
		      "<AttentionDuration>", cmd_provision_adv, 5, 0),
#endif
#endif
	SHELL_CMD_ARG(uuid, NULL, "<UUID: 1-16 hex values>", cmd_uuid, 2, 0),
	SHELL_CMD_ARG(input-num, NULL, "<number>", cmd_input_num, 2, 0),
	SHELL_CMD_ARG(input-str, NULL, "<string>", cmd_input_str, 2, 0),
	SHELL_CMD_ARG(static-oob, NULL, "[val: 1-16 hex values]",
		      cmd_static_oob, 2, 1),
	SHELL_CMD_ARG(provision, NULL, "<NetKeyIndex> <addr> [IVIndex]",
		      cmd_provision, 3, 1),
	SHELL_CMD_ARG(beacon-listen, NULL, "<val: off, on>", cmd_beacon_listen,
		      2, 0),

	/* Configuration Client Model operations */
	SHELL_CMD_ARG(timeout, NULL, "[timeout in seconds]", cmd_timeout, 1, 1),
	SHELL_CMD_ARG(get-comp, NULL, "[page]", cmd_get_comp, 1, 1),
	SHELL_CMD_ARG(beacon, NULL, "[val: off, on]", cmd_beacon, 2, 1),
	SHELL_CMD_ARG(ttl, NULL, "[ttl: 0x00, 0x02-0x7f]", cmd_ttl, 1, 1),
	SHELL_CMD_ARG(friend, NULL, "[val: off, on]", cmd_friend, 1, 1),
	SHELL_CMD_ARG(gatt-proxy, NULL, "[val: off, on]", cmd_gatt_proxy, 1, 1),
	SHELL_CMD_ARG(relay, NULL,
		      "[<val: off, on> [<count: 0-7> [interval: 10-320]]]",
		      cmd_relay, 1, 3),
	SHELL_CMD_ARG(net-key-add, NULL, "<NetKeyIndex> [val]", cmd_net_key_add,
		      2, 1),
	SHELL_CMD_ARG(net-key-get, NULL, NULL, cmd_net_key_get, 1, 0),
	SHELL_CMD_ARG(net-key-del, NULL, "<NetKeyIndex>", cmd_net_key_del, 2,
		      0),
	SHELL_CMD_ARG(app-key-add, NULL, "<NetKeyIndex> <AppKeyIndex> [val]",
		      cmd_app_key_add, 3, 1),
	SHELL_CMD_ARG(app-key-del, NULL, "<NetKeyIndex> <AppKeyIndex>",
		      cmd_app_key_del, 3, 0),
	SHELL_CMD_ARG(app-key-get, NULL, "<NetKeyIndex>", cmd_app_key_get, 2,
		      0),
	SHELL_CMD_ARG(net-transmit-param, NULL, "[<count: 0-7>"
			" <interval: 10-320>]", cmd_net_transmit, 1, 2),
	SHELL_CMD_ARG(mod-app-bind, NULL,
		      "<addr> <AppIndex> <Model ID> [Company ID]",
		      cmd_mod_app_bind, 4, 1),
	SHELL_CMD_ARG(mod-app-get, NULL,
		      "<elem addr> <Model ID> [Company ID]",
		      cmd_mod_app_get, 3, 1),
	SHELL_CMD_ARG(mod-app-unbind, NULL,
		      "<addr> <AppIndex> <Model ID> [Company ID]",
		      cmd_mod_app_unbind, 4, 1),
	SHELL_CMD_ARG(mod-pub, NULL, "<addr> <mod id> [cid] [<PubAddr> "
		      "<AppKeyIndex> <cred: off, on> <ttl> <period> <count> <interval>]",
		      cmd_mod_pub, 3, 1 + 7),
	SHELL_CMD_ARG(mod-sub-add, NULL,
		      "<elem addr> <sub addr> <Model ID> [Company ID]",
		      cmd_mod_sub_add, 4, 1),
	SHELL_CMD_ARG(mod-sub-del, NULL,
		      "<elem addr> <sub addr> <Model ID> [Company ID]",
		      cmd_mod_sub_del, 4, 1),
	SHELL_CMD_ARG(mod-sub-add-va, NULL,
		      "<elem addr> <Label UUID> <Model ID> [Company ID]",
		      cmd_mod_sub_add_va, 4, 1),
	SHELL_CMD_ARG(mod-sub-del-va, NULL,
		      "<elem addr> <Label UUID> <Model ID> [Company ID]",
		      cmd_mod_sub_del_va, 4, 1),
	SHELL_CMD_ARG(mod-sub-get, NULL,
		      "<elem addr> <Model ID> [Company ID]",
		      cmd_mod_sub_get, 3, 1),
	SHELL_CMD_ARG(hb-sub, NULL, "[<src> <dst> <period>]", cmd_hb_sub, 1, 3),
	SHELL_CMD_ARG(hb-pub, NULL,
		      "[<dst> <count> <period> <ttl> <features> <NetKeyIndex>]",
		      cmd_hb_pub, 1, 6),

	/* Health Client Model Operations */
	SHELL_CMD_ARG(fault-get, NULL, "<Company ID>", cmd_fault_get, 2, 0),
	SHELL_CMD_ARG(fault-clear, NULL, "<Company ID>", cmd_fault_clear, 2, 0),
	SHELL_CMD_ARG(fault-clear-unack, NULL, "<Company ID>",
		      cmd_fault_clear_unack, 2, 0),
	SHELL_CMD_ARG(fault-test, NULL, "<Company ID> <Test ID>",
		      cmd_fault_test, 3, 0),
	SHELL_CMD_ARG(fault-test-unack, NULL, "<Company ID> <Test ID>",
		      cmd_fault_test_unack, 3, 0),
	SHELL_CMD_ARG(period-get, NULL, NULL, cmd_period_get, 1, 0),
	SHELL_CMD_ARG(period-set, NULL, "<divisor>", cmd_period_set, 2, 0),
	SHELL_CMD_ARG(period-set-unack, NULL, "<divisor>", cmd_period_set_unack,
		      2, 0),
	SHELL_CMD_ARG(attention-get, NULL, NULL, cmd_attention_get, 1, 0),
	SHELL_CMD_ARG(attention-set, NULL, "<timer>", cmd_attention_set, 2, 0),
	SHELL_CMD_ARG(attention-set-unack, NULL, "<timer>",
		      cmd_attention_set_unack, 2, 0),

	/* Health Server Model Operations */
	SHELL_CMD_ARG(add-fault, NULL, "<Fault ID>", cmd_add_fault, 2, 0),
	SHELL_CMD_ARG(del-fault, NULL, "[Fault ID]", cmd_del_fault, 1, 1),

#if defined(CONFIG_BT_MESH_CDB)
	/* Mesh Configuration Database Operations */
	SHELL_CMD_ARG(cdb-create, NULL, "[NetKey]", cmd_cdb_create, 1, 1),
	SHELL_CMD_ARG(cdb-clear, NULL, NULL, cmd_cdb_clear, 1, 0),
	SHELL_CMD_ARG(cdb-show, NULL, NULL, cmd_cdb_show, 1, 0),
	SHELL_CMD_ARG(cdb-node-add, NULL, "<UUID> <addr> <num-elem> "
		      "<NetKeyIdx> [DevKey]", cmd_cdb_node_add, 5, 1),
	SHELL_CMD_ARG(cdb-node-del, NULL, "<addr>", cmd_cdb_node_del, 2, 0),
	SHELL_CMD_ARG(cdb-subnet-add, NULL, "<NeyKeyIdx> [<NetKey>]",
		      cmd_cdb_subnet_add, 2, 1),
	SHELL_CMD_ARG(cdb-subnet-del, NULL, "<NetKeyIdx>", cmd_cdb_subnet_del,
		      2, 0),
	SHELL_CMD_ARG(cdb-app-key-add, NULL, "<NetKeyIdx> <AppKeyIdx> "
		      "[<AppKey>]", cmd_cdb_app_key_add, 3, 1),
	SHELL_CMD_ARG(cdb-app-key-del, NULL, "<AppKeyIdx>", cmd_cdb_app_key_del,
		      2, 0),
#endif

	SHELL_SUBCMD_SET_END
);

static int cmd_mesh(const struct shell *shell, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(shell);
		/* shell returns 1 when help is printed */
		return 1;
	}

	shell_error(shell, "%s unknown parameter: %s", argv[0], argv[1]);

	return -EINVAL;
}

SHELL_CMD_ARG_REGISTER(mesh, &mesh_cmds, "Bluetooth Mesh shell commands",
			cmd_mesh, 1, 1);
