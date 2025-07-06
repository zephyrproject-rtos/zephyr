/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <ctype.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include <zephyr/shell/shell.h>
#include <zephyr/settings/settings.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/mesh.h>
#include <zephyr/bluetooth/mesh/shell.h>

/* Private includes for raw Network & Transport layer access */
#include "mesh/mesh.h"
#include "mesh/net.h"
#include "mesh/rpl.h"
#include "mesh/transport.h"
#include "mesh/foundation.h"
#include "mesh/settings.h"
#include "mesh/access.h"
#include "common/bt_shell_private.h"
#include "utils.h"
#include "dfu.h"
#include "blob.h"

#define CID_NVAL   0xffff
#define COMPANY_ID_LF 0x05F1
#define COMPANY_ID_NORDIC_SEMI 0x05F9

struct bt_mesh_shell_target bt_mesh_shell_target_ctx = {
	.dst = BT_MESH_ADDR_UNASSIGNED,
};

/* Default net, app & dev key values, unless otherwise specified */
const uint8_t bt_mesh_shell_default_key[16] = {
	0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
	0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
};

#if defined(CONFIG_BT_MESH_SHELL_HEALTH_SRV_INSTANCE)
static uint8_t cur_faults[BT_MESH_SHELL_CUR_FAULTS_MAX];
static uint8_t reg_faults[BT_MESH_SHELL_CUR_FAULTS_MAX * 2];

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

static int fault_get_cur(const struct bt_mesh_model *model, uint8_t *test_id,
			 uint16_t *company_id, uint8_t *faults, uint8_t *fault_count)
{
	bt_shell_print("Sending current faults");

	*test_id = 0x00;
	*company_id = BT_COMP_ID_LF;

	get_faults(cur_faults, sizeof(cur_faults), faults, fault_count);

	return 0;
}

static int fault_get_reg(const struct bt_mesh_model *model, uint16_t cid,
			 uint8_t *test_id, uint8_t *faults, uint8_t *fault_count)
{
	if (cid != CONFIG_BT_COMPANY_ID) {
		bt_shell_print("Faults requested for unknown Company ID 0x%04x", cid);
		return -EINVAL;
	}

	bt_shell_print("Sending registered faults");

	*test_id = 0x00;

	get_faults(reg_faults, sizeof(reg_faults), faults, fault_count);

	return 0;
}

static int fault_clear(const struct bt_mesh_model *model, uint16_t cid)
{
	if (cid != CONFIG_BT_COMPANY_ID) {
		return -EINVAL;
	}

	(void)memset(reg_faults, 0, sizeof(reg_faults));

	return 0;
}

static int fault_test(const struct bt_mesh_model *model, uint8_t test_id,
		      uint16_t cid)
{
	if (cid != CONFIG_BT_COMPANY_ID) {
		return -EINVAL;
	}

	if (test_id != 0x00) {
		return -EINVAL;
	}

	return 0;
}

static void attention_on(const struct bt_mesh_model *model)
{
	bt_shell_print("Attention On");
}

static void attention_off(const struct bt_mesh_model *model)
{
	bt_shell_print("Attention Off");
}

static const struct bt_mesh_health_srv_cb health_srv_cb = {
	.fault_get_cur = fault_get_cur,
	.fault_get_reg = fault_get_reg,
	.fault_clear = fault_clear,
	.fault_test = fault_test,
	.attn_on = attention_on,
	.attn_off = attention_off,
};
#endif /* CONFIG_BT_MESH_SHELL_HEALTH_SRV_INSTANCE */

#if defined(CONFIG_BT_MESH_LARGE_COMP_DATA_SRV)
static const uint8_t health_tests[] = {
	BT_MESH_HEALTH_TEST_INFO(COMPANY_ID_LF, 6, 0x01, 0x02, 0x03, 0x04, 0x34, 0x15),
	BT_MESH_HEALTH_TEST_INFO(COMPANY_ID_NORDIC_SEMI, 3, 0x01, 0x02, 0x03),
};

const struct bt_mesh_models_metadata_entry health_srv_meta[] = {
	BT_MESH_HEALTH_TEST_INFO_METADATA(health_tests),
	BT_MESH_MODELS_METADATA_END,
};
#endif

struct bt_mesh_health_srv bt_mesh_shell_health_srv = {
#if defined(CONFIG_BT_MESH_SHELL_HEALTH_SRV_INSTANCE)
	.cb = &health_srv_cb,
#endif
};

#if defined(CONFIG_BT_MESH_SHELL_HEALTH_CLI)
static void show_faults(uint8_t test_id, uint16_t cid, uint8_t *faults, size_t fault_count)
{
	size_t i;

	if (!fault_count) {
		bt_shell_print("Health Test ID 0x%02x Company ID 0x%04x: no faults",
			       test_id, cid);
		return;
	}

	bt_shell_print("Health Test ID 0x%02x Company ID 0x%04x Fault Count %zu:",
		       test_id, cid, fault_count);

	for (i = 0; i < fault_count; i++) {
		bt_shell_print("\t0x%02x", faults[i]);
	}
}

static void health_current_status(struct bt_mesh_health_cli *cli, uint16_t addr,
				  uint8_t test_id, uint16_t cid, uint8_t *faults,
				  size_t fault_count)
{
	bt_shell_print("Health Current Status from 0x%04x", addr);
	show_faults(test_id, cid, faults, fault_count);
}

static void health_fault_status(struct bt_mesh_health_cli *cli, uint16_t addr,
				uint8_t test_id, uint16_t cid, uint8_t *faults,
				size_t fault_count)
{
	bt_shell_print("Health Fault Status from 0x%04x", addr);
	show_faults(test_id, cid, faults, fault_count);
}

static void health_attention_status(struct bt_mesh_health_cli *cli,
				    uint16_t addr, uint8_t attention)
{
	bt_shell_print("Health Attention Status from 0x%04x: %u", addr, attention);
}

static void health_period_status(struct bt_mesh_health_cli *cli, uint16_t addr,
				 uint8_t period)
{
	bt_shell_print("Health Fast Period Divisor Status from 0x%04x: %u", addr, period);
}

struct bt_mesh_health_cli bt_mesh_shell_health_cli = {
	.current_status = health_current_status,
	.fault_status = health_fault_status,
	.attention_status = health_attention_status,
	.period_status = health_period_status,
};
#endif /* CONFIG_BT_MESH_SHELL_HEALTH_CLI */

static int cmd_init(const struct shell *sh, size_t argc, char *argv[])
{
	shell_print(sh, "Mesh shell initialized");

#if defined(CONFIG_BT_MESH_SHELL_DFU_CLI) || defined(CONFIG_BT_MESH_SHELL_DFU_SRV)
	bt_mesh_shell_dfu_cmds_init();
#endif
#if defined(CONFIG_BT_MESH_SHELL_BLOB_CLI) || defined(CONFIG_BT_MESH_SHELL_BLOB_SRV) || \
	defined(CONFIG_BT_MESH_SHELL_BLOB_IO_FLASH)
	bt_mesh_shell_blob_cmds_init();
#endif

	if (IS_ENABLED(CONFIG_BT_MESH_RPR_SRV)) {
		bt_mesh_prov_enable(BT_MESH_PROV_REMOTE);
	}

	return 0;
}

static int cmd_reset(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_BT_MESH_CDB)
	bt_mesh_cdb_clear();
#endif
	bt_mesh_reset();
	shell_print(sh, "Local node reset complete");

	return 0;
}

#if defined(CONFIG_BT_MESH_SHELL_LOW_POWER)
static int cmd_lpn(const struct shell *sh, size_t argc, char *argv[])
{
	static bool enabled;
	bool onoff;
	int err = 0;

	if (argc < 2) {
		shell_print(sh, "%s", enabled ? "enabled" : "disabled");
		return 0;
	}

	onoff = shell_strtobool(argv[1], 0, &err);
	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	if (onoff) {
		if (enabled) {
			shell_print(sh, "LPN already enabled");
			return 0;
		}

		err = bt_mesh_lpn_set(true);
		if (err) {
			shell_error(sh, "Enabling LPN failed (err %d)", err);
		} else {
			enabled = true;
		}
	} else {
		if (!enabled) {
			shell_print(sh, "LPN already disabled");
			return 0;
		}

		err = bt_mesh_lpn_set(false);
		if (err) {
			shell_error(sh, "Enabling LPN failed (err %d)", err);
		} else {
			enabled = false;
		}
	}

	return 0;
}

static int cmd_poll(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	err = bt_mesh_lpn_poll();
	if (err) {
		shell_error(sh, "Friend Poll failed (err %d)", err);
	}

	return 0;
}

static void lpn_established(uint16_t net_idx, uint16_t friend_addr,
					uint8_t queue_size, uint8_t recv_win)
{
	bt_shell_print("Friendship (as LPN) established to "
		       "Friend 0x%04x Queue Size %d Receive Window %d",
		       friend_addr, queue_size, recv_win);
}

static void lpn_terminated(uint16_t net_idx, uint16_t friend_addr)
{
	bt_shell_print("Friendship (as LPN) lost with Friend 0x%04x", friend_addr);
}

BT_MESH_LPN_CB_DEFINE(lpn_cb) = {
	.established = lpn_established,
	.terminated = lpn_terminated,
};
#endif /* CONFIG_BT_MESH_SHELL_LOW_POWER */

#if defined(CONFIG_BT_MESH_SHELL_GATT_PROXY)
#if defined(CONFIG_BT_MESH_GATT_PROXY)
static int cmd_ident(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	err = bt_mesh_proxy_identity_enable();
	if (err) {
		shell_error(sh, "Failed advertise using Node Identity (err "
			    "%d)", err);
	}

	return 0;
}
#endif /* CONFIG_BT_MESH_GATT_PROXY */

#if defined(CONFIG_BT_MESH_PROXY_CLIENT)
static int cmd_proxy_connect(const struct shell *sh, size_t argc,
			     char *argv[])
{
	uint16_t net_idx;
	int err = 0;

	net_idx = shell_strtoul(argv[1], 0, &err);
	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	err = bt_mesh_proxy_connect(net_idx);
	if (err) {
		shell_error(sh, "Proxy connect failed (err %d)", err);
	}

	return 0;
}

static int cmd_proxy_disconnect(const struct shell *sh, size_t argc,
				char *argv[])
{
	uint16_t net_idx;
	int err = 0;

	net_idx = shell_strtoul(argv[1], 0, &err);
	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	err = bt_mesh_proxy_disconnect(net_idx);
	if (err) {
		shell_error(sh, "Proxy disconnect failed (err %d)", err);
	}

	return 0;
}
#endif /* CONFIG_BT_MESH_PROXY_CLIENT */

#if defined(CONFIG_BT_MESH_PROXY_SOLICITATION)
static int cmd_proxy_solicit(const struct shell *sh, size_t argc,
			     char *argv[])
{
	uint16_t net_idx;
	int err = 0;

	net_idx = shell_strtoul(argv[1], 0, &err);
	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	err = bt_mesh_proxy_solicit(net_idx);
	if (err) {
		shell_error(sh, "Failed to advertise solicitation PDU (err %d)",
			    err);
	}

	return err;
}
#endif /* CONFIG_BT_MESH_PROXY_SOLICITATION */
#endif /* CONFIG_BT_MESH_SHELL_GATT_PROXY */

#if defined(CONFIG_BT_MESH_SHELL_PROV)
static int cmd_input_num(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;
	uint32_t val;

	val = shell_strtoul(argv[1], 10, &err);
	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	err = bt_mesh_input_number(val);
	if (err) {
		shell_error(sh, "Numeric input failed (err %d)", err);
	}

	return 0;
}

static int cmd_input_str(const struct shell *sh, size_t argc, char *argv[])
{
	int err = bt_mesh_input_string(argv[1]);

	if (err) {
		shell_error(sh, "String input failed (err %d)", err);
	}

	return 0;
}

static const char *bearer2str(bt_mesh_prov_bearer_t bearer)
{
	switch (bearer) {
	case BT_MESH_PROV_ADV:
		return "PB-ADV";
	case BT_MESH_PROV_GATT:
		return "PB-GATT";
	case BT_MESH_PROV_REMOTE:
		return "PB-REMOTE";
	default:
		return "unknown";
	}
}

#if defined(CONFIG_BT_MESH_SHELL_PROV_CTX_INSTANCE)
static uint8_t dev_uuid[16] = { 0xdd, 0xdd };

static void prov_complete(uint16_t net_idx, uint16_t addr)
{

	bt_shell_print("Local node provisioned, net_idx 0x%04x address 0x%04x", net_idx, addr);

	bt_mesh_shell_target_ctx.net_idx = net_idx;
	bt_mesh_shell_target_ctx.dst = addr;
}

static void reprovisioned(uint16_t addr)
{
	bt_shell_print("Local node re-provisioned, new address 0x%04x", addr);

	if (bt_mesh_shell_target_ctx.dst == bt_mesh_primary_addr()) {
		bt_mesh_shell_target_ctx.dst = addr;
	}
}

static void prov_node_added(uint16_t net_idx, uint8_t uuid[16], uint16_t addr,
			    uint8_t num_elem)
{
	bt_shell_print("Node provisioned, net_idx 0x%04x address 0x%04x elements %d",
		       net_idx, addr, num_elem);

	bt_mesh_shell_target_ctx.net_idx = net_idx;
	bt_mesh_shell_target_ctx.dst = addr;
}

#if defined(CONFIG_BT_MESH_PROVISIONER)
static const char * const output_meth_string[] = {
	"Blink",
	"Beep",
	"Vibrate",
	"Display Number",
	"Display String",
};

static const char *const input_meth_string[] = {
	"Push",
	"Twist",
	"Enter Number",
	"Enter String",
};

static void capabilities(const struct bt_mesh_dev_capabilities *cap)
{
	bt_shell_print("Provisionee capabilities:");
	bt_shell_print("\tStatic OOB is %ssupported", cap->oob_type & 1 ? "" : "not ");

	bt_shell_print("\tAvailable output actions (%d bytes max):%s", cap->output_size,
		       cap->output_actions ? "" : "\n\t\tNone");
	for (int i = 0; i < ARRAY_SIZE(output_meth_string); i++) {
		if (cap->output_actions & BIT(i)) {
			bt_shell_print("\t\t%s", output_meth_string[i]);
		}
	}

	bt_shell_print("\tAvailable input actions (%d bytes max):%s", cap->input_size,
		       cap->input_actions ? "" : "\n\t\tNone");
	for (int i = 0; i < ARRAY_SIZE(input_meth_string); i++) {
		if (cap->input_actions & BIT(i)) {
			bt_shell_print("\t\t%s", input_meth_string[i]);
		}
	}
}
#endif

static void prov_input_complete(void)
{
	bt_shell_print("Input complete");
}

static void prov_reset(void)
{
	bt_shell_print("The local node has been reset and needs reprovisioning");
}

static int output_number(bt_mesh_output_action_t action, uint32_t number)
{
	switch (action) {
	case BT_MESH_BLINK:
		bt_shell_print("OOB blink Number: %u", number);
		break;
	case BT_MESH_BEEP:
		bt_shell_print("OOB beep Number: %u", number);
		break;
	case BT_MESH_VIBRATE:
		bt_shell_print("OOB vibrate Number: %u", number);
		break;
	case BT_MESH_DISPLAY_NUMBER:
		bt_shell_print("OOB display Number: %u", number);
		break;
	default:
		bt_shell_error("Unknown Output action %u (number %u) requested!",
			       action, number);
		return -EINVAL;
	}

	return 0;
}

static int output_string(const char *str)
{
	bt_shell_print("OOB String: %s", str);
	return 0;
}

static int input(bt_mesh_input_action_t act, uint8_t size)
{

	switch (act) {
	case BT_MESH_ENTER_NUMBER:
		bt_shell_print("Enter a number (max %u digits) with: Input-num <num>", size);
		break;
	case BT_MESH_ENTER_STRING:
		bt_shell_print("Enter a string (max %u chars) with: Input-str <str>", size);
		break;
	case BT_MESH_TWIST:
		bt_shell_print("\"Twist\" a number (max %u digits) with: Input-num <num>", size);
		break;
	case BT_MESH_PUSH:
		bt_shell_print("\"Push\" a number (max %u digits) with: Input-num <num>", size);
		break;
	default:
		bt_shell_error("Unknown Input action %u (size %u) requested!", act, size);
		return -EINVAL;
	}

	return 0;
}

static void link_open(bt_mesh_prov_bearer_t bearer)
{
	bt_shell_print("Provisioning link opened on %s", bearer2str(bearer));
}

static void link_close(bt_mesh_prov_bearer_t bearer)
{
	bt_shell_print("Provisioning link closed on %s", bearer2str(bearer));
}

static uint8_t static_val[32];

struct bt_mesh_prov bt_mesh_shell_prov = {
	.uuid = dev_uuid,
	.link_open = link_open,
	.link_close = link_close,
	.complete = prov_complete,
	.reprovisioned = reprovisioned,
	.node_added = prov_node_added,
	.reset = prov_reset,
	.static_val = NULL,
	.static_val_len = 0,
	.output_size = 6,
	.output_actions = (BT_MESH_BLINK | BT_MESH_BEEP | BT_MESH_VIBRATE | BT_MESH_DISPLAY_NUMBER |
			   BT_MESH_DISPLAY_STRING),
	.output_number = output_number,
	.output_string = output_string,
	.input_size = 6,
	.input_actions =
		(BT_MESH_ENTER_NUMBER | BT_MESH_ENTER_STRING | BT_MESH_TWIST | BT_MESH_PUSH),
	.input = input,
	.input_complete = prov_input_complete,
#if defined(CONFIG_BT_MESH_PROVISIONER)
	.capabilities = capabilities
#endif
};

static int cmd_static_oob(const struct shell *sh, size_t argc, char *argv[])
{
	if (argc < 2) {
		bt_mesh_shell_prov.static_val = NULL;
		bt_mesh_shell_prov.static_val_len = 0U;
	} else {
		bt_mesh_shell_prov.static_val_len = hex2bin(argv[1], strlen(argv[1]),
					      static_val, 32);
		if (bt_mesh_shell_prov.static_val_len) {
			bt_mesh_shell_prov.static_val = static_val;
		} else {
			bt_mesh_shell_prov.static_val = NULL;
		}
	}

	if (bt_mesh_shell_prov.static_val) {
		shell_print(sh, "Static OOB value set (length %u)",
			    bt_mesh_shell_prov.static_val_len);
	} else {
		shell_print(sh, "Static OOB value cleared");
	}

	return 0;
}

static int cmd_uuid(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t uuid[16];
	size_t len;

	if (argc < 2) {
		char uuid_hex_str[32 + 1];

		bin2hex(dev_uuid, 16, uuid_hex_str, sizeof(uuid_hex_str));

		bt_shell_print("Device UUID: %s", uuid_hex_str);
		return 0;
	}

	len = hex2bin(argv[1], strlen(argv[1]), uuid, sizeof(uuid));
	if (len < 1) {
		return -EINVAL;
	}

	memcpy(dev_uuid, uuid, len);
	(void)memset(dev_uuid + len, 0, sizeof(dev_uuid) - len);

	shell_print(sh, "Device UUID set");

	return 0;
}

static void print_unprovisioned_beacon(uint8_t uuid[16],
				       bt_mesh_prov_oob_info_t oob_info,
				       uint32_t *uri_hash)
{
	char uuid_hex_str[32 + 1];

	bin2hex(uuid, 16, uuid_hex_str, sizeof(uuid_hex_str));

	bt_shell_print("PB-ADV UUID %s, OOB Info 0x%04x, URI Hash 0x%x",
		       uuid_hex_str, oob_info,
		       (uri_hash == NULL ? 0 : *uri_hash));
}

#if defined(CONFIG_BT_MESH_PB_GATT_CLIENT)
static void pb_gatt_unprovisioned(uint8_t uuid[16],
				  bt_mesh_prov_oob_info_t oob_info)
{
	char uuid_hex_str[32 + 1];

	bin2hex(uuid, 16, uuid_hex_str, sizeof(uuid_hex_str));

	bt_shell_print("PB-GATT UUID %s, OOB Info 0x%04x", uuid_hex_str, oob_info);
}
#endif

static int cmd_beacon_listen(const struct shell *sh, size_t argc,
			     char *argv[])
{
	int err = 0;
	bool val = shell_strtobool(argv[1], 0, &err);

	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	if (!bt_mesh_is_provisioned()) {
		shell_error(sh, "Not yet provisioned");
		return -EINVAL;
	}

	if (val) {
		bt_mesh_shell_prov.unprovisioned_beacon = print_unprovisioned_beacon;
#if defined(CONFIG_BT_MESH_PB_GATT_CLIENT)
		bt_mesh_shell_prov.unprovisioned_beacon_gatt = pb_gatt_unprovisioned;
#endif
	} else {
		bt_mesh_shell_prov.unprovisioned_beacon = NULL;
		bt_mesh_shell_prov.unprovisioned_beacon_gatt = NULL;
	}

	return 0;
}
#endif /* CONFIG_BT_MESH_SHELL_PROV_CTX_INSTANCE */

#if defined(CONFIG_BT_MESH_PB_GATT_CLIENT)
static int cmd_provision_gatt(const struct shell *sh, size_t argc,
			      char *argv[])
{
	static uint8_t uuid[16];
	uint8_t attention_duration;
	uint16_t net_idx;
	uint16_t addr;
	size_t len;
	int err = 0;

	len = hex2bin(argv[1], strlen(argv[1]), uuid, sizeof(uuid));
	(void)memset(uuid + len, 0, sizeof(uuid) - len);

	net_idx = shell_strtoul(argv[2], 0, &err);
	addr = shell_strtoul(argv[3], 0, &err);
	attention_duration = shell_strtoul(argv[4], 0, &err);
	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	err = bt_mesh_provision_gatt(uuid, net_idx, addr, attention_duration);
	if (err) {
		shell_error(sh, "Provisioning failed (err %d)", err);
	}

	return 0;
}
#endif /* CONFIG_BT_MESH_PB_GATT_CLIENT */

#if defined(CONFIG_BT_MESH_PROVISIONEE)
static int cmd_pb(bt_mesh_prov_bearer_t bearer, const struct shell *sh,
		  size_t argc, char *argv[])
{
	int err = 0;
	bool onoff;

	if (argc < 2) {
		return -EINVAL;
	}

	onoff = shell_strtobool(argv[1], 0, &err);
	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	if (onoff) {
		err = bt_mesh_prov_enable(bearer);
		if (err) {
			shell_error(sh, "Failed to enable %s (err %d)",
				    bearer2str(bearer), err);
		} else {
			shell_print(sh, "%s enabled", bearer2str(bearer));
		}
	} else {
		err = bt_mesh_prov_disable(bearer);
		if (err) {
			shell_error(sh, "Failed to disable %s (err %d)",
				    bearer2str(bearer), err);
		} else {
			shell_print(sh, "%s disabled", bearer2str(bearer));
		}
	}

	return 0;
}

#if defined(CONFIG_BT_MESH_PB_ADV)
static int cmd_pb_adv(const struct shell *sh, size_t argc, char *argv[])
{
	return cmd_pb(BT_MESH_PROV_ADV, sh, argc, argv);
}
#endif /* CONFIG_BT_MESH_PB_ADV */

#if defined(CONFIG_BT_MESH_PB_GATT)
static int cmd_pb_gatt(const struct shell *sh, size_t argc, char *argv[])
{
	return cmd_pb(BT_MESH_PROV_GATT, sh, argc, argv);
}
#endif /* CONFIG_BT_MESH_PB_GATT */
#endif /* CONFIG_BT_MESH_PROVISIONEE */

#if defined(CONFIG_BT_MESH_PROVISIONER)
static int cmd_remote_pub_key_set(const struct shell *sh, size_t argc, char *argv[])
{
	size_t len;
	uint8_t pub_key[64];
	int err = 0;

	len = hex2bin(argv[1], strlen(argv[1]), pub_key, sizeof(pub_key));
	if (len < 1) {
		shell_warn(sh, "Unable to parse input string argument");
		return -EINVAL;
	}

	err = bt_mesh_prov_remote_pub_key_set(pub_key);

	if (err) {
		shell_error(sh, "Setting remote pub key failed (err %d)", err);
	}

	return 0;
}

static int cmd_auth_method_set_input(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;
	bt_mesh_input_action_t action = shell_strtoul(argv[1], 10, &err);
	uint8_t size = shell_strtoul(argv[2], 10, &err);

	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	err = bt_mesh_auth_method_set_input(action, size);
	if (err) {
		shell_error(sh, "Setting input OOB authentication action failed (err %d)", err);
	}

	return 0;
}

static int cmd_auth_method_set_output(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;
	bt_mesh_output_action_t action = shell_strtoul(argv[1], 10, &err);
	uint8_t size = shell_strtoul(argv[2], 10, &err);

	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	err = bt_mesh_auth_method_set_output(action, size);
	if (err) {
		shell_error(sh, "Setting output OOB authentication action failed (err %d)", err);
	}
	return 0;
}

static int cmd_auth_method_set_static(const struct shell *sh, size_t argc, char *argv[])
{
	size_t len;
	uint8_t static_oob_auth[32];
	int err = 0;

	len = hex2bin(argv[1], strlen(argv[1]), static_oob_auth, sizeof(static_oob_auth));
	if (len < 1) {
		shell_warn(sh, "Unable to parse input string argument");
		return -EINVAL;
	}

	err = bt_mesh_auth_method_set_static(static_oob_auth, len);
	if (err) {
		shell_error(sh, "Setting static OOB authentication failed (err %d)", err);
	}
	return 0;
}

static int cmd_auth_method_set_none(const struct shell *sh, size_t argc, char *argv[])
{
	int err = bt_mesh_auth_method_set_none();

	if (err) {
		shell_error(sh, "Disabling authentication failed (err %d)", err);
	}
	return 0;
}

static int cmd_provision_adv(const struct shell *sh, size_t argc,
			     char *argv[])
{
	uint8_t uuid[16];
	uint8_t attention_duration;
	uint16_t net_idx;
	uint16_t addr;
	size_t len;
	int err = 0;

	len = hex2bin(argv[1], strlen(argv[1]), uuid, sizeof(uuid));
	(void)memset(uuid + len, 0, sizeof(uuid) - len);

	net_idx = shell_strtoul(argv[2], 0, &err);
	addr = shell_strtoul(argv[3], 0, &err);
	attention_duration = shell_strtoul(argv[4], 0, &err);
	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	err = bt_mesh_provision_adv(uuid, net_idx, addr, attention_duration);
	if (err) {
		shell_error(sh, "Provisioning failed (err %d)", err);
	}

	return 0;
}
#endif /* CONFIG_BT_MESH_PROVISIONER */

static int cmd_provision_local(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t net_key[16];
	uint16_t net_idx, addr;
	uint32_t iv_index;
	int err = 0;

	net_idx = shell_strtoul(argv[1], 0, &err);
	addr = shell_strtoul(argv[2], 0, &err);

	if (argc > 3) {
		iv_index = shell_strtoul(argv[3], 0, &err);
	} else {
		iv_index = 0U;
	}

	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	memcpy(net_key, bt_mesh_shell_default_key, sizeof(net_key));

	if (IS_ENABLED(CONFIG_BT_MESH_CDB)) {
		struct bt_mesh_cdb_subnet *sub;

		sub = bt_mesh_cdb_subnet_get(net_idx);
		if (!sub) {
			shell_error(sh, "No cdb entry for subnet 0x%03x", net_idx);
			return 0;
		}

		if (bt_mesh_cdb_subnet_key_export(sub, SUBNET_KEY_TX_IDX(sub), net_key)) {
			shell_error(sh, "Unable to export key for subnet 0x%03x", net_idx);
			return 0;
		}
	}

	err = bt_mesh_provision(net_key, net_idx, 0, iv_index, addr, bt_mesh_shell_default_key);
	if (err) {
		shell_error(sh, "Provisioning failed (err %d)", err);
	}

	return 0;
}

static int cmd_comp_change(const struct shell *sh, size_t argc, char *argv[])
{
	bt_mesh_comp_change_prepare();
	return 0;
}
#endif /* CONFIG_BT_MESH_SHELL_PROV */

#if defined(CONFIG_BT_MESH_SHELL_TEST)
static int cmd_net_send(const struct shell *sh, size_t argc, char *argv[])
{
	NET_BUF_SIMPLE_DEFINE(msg, 32);

	struct bt_mesh_msg_ctx ctx = BT_MESH_MSG_CTX_INIT(bt_mesh_shell_target_ctx.net_idx,
							  bt_mesh_shell_target_ctx.app_idx,
							  bt_mesh_shell_target_ctx.dst,
							  BT_MESH_TTL_DEFAULT);
	struct bt_mesh_net_tx tx = {
		.ctx = &ctx,
		.src = bt_mesh_primary_addr(),
	};

	size_t len;
	int err;

	len = hex2bin(argv[1], strlen(argv[1]),
		      msg.data, net_buf_simple_tailroom(&msg) - 4);
	net_buf_simple_add(&msg, len);

	err = bt_mesh_trans_send(&tx, &msg, NULL, NULL);
	if (err) {
		shell_error(sh, "Failed to send (err %d)", err);
	}

	return 0;
}

#if defined(CONFIG_BT_MESH_IV_UPDATE_TEST)
static int cmd_iv_update(const struct shell *sh, size_t argc, char *argv[])
{
	if (bt_mesh_iv_update()) {
		shell_print(sh, "Transitioned to IV Update In Progress "
			    "state");
	} else {
		shell_print(sh, "Transitioned to IV Update Normal state");
	}

	shell_print(sh, "IV Index is 0x%08x", bt_mesh.iv_index);

	return 0;
}

static int cmd_iv_update_test(const struct shell *sh, size_t argc,
			      char *argv[])
{
	int err = 0;
	bool enable;

	enable = shell_strtobool(argv[1], 0, &err);
	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	if (enable) {
		shell_print(sh, "Enabling IV Update test mode");
	} else {
		shell_print(sh, "Disabling IV Update test mode");
	}

	bt_mesh_iv_update_test(enable);

	return 0;
}
#endif /* CONFIG_BT_MESH_IV_UPDATE_TEST */

static int cmd_rpl_clear(const struct shell *sh, size_t argc, char *argv[])
{
	bt_mesh_rpl_clear();
	return 0;
}

#if defined(CONFIG_BT_MESH_SHELL_HEALTH_SRV_INSTANCE)
static const struct bt_mesh_elem *primary_element(void)
{
	const struct bt_mesh_comp *comp = bt_mesh_comp_get();

	if (comp) {
		return &comp->elem[0];
	}

	return NULL;
}

static int cmd_add_fault(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t fault_id;
	uint8_t i;
	const struct bt_mesh_elem *elem;
	int err = 0;

	elem = primary_element();
	if (elem == NULL) {
		shell_print(sh, "Element not found!");
		return -EINVAL;
	}

	fault_id = shell_strtoul(argv[1], 0, &err);
	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	if (!fault_id) {
		shell_print(sh, "The Fault ID must be non-zero!");
		return -EINVAL;
	}

	for (i = 0U; i < sizeof(cur_faults); i++) {
		if (!cur_faults[i]) {
			cur_faults[i] = fault_id;
			break;
		}
	}

	if (i == sizeof(cur_faults)) {
		shell_print(sh, "Fault array is full. Use \"del-fault\" to "
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
		shell_print(sh, "No space to store more registered faults");
	}

	bt_mesh_health_srv_fault_update(elem);

	return 0;
}

static int cmd_del_fault(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t fault_id;
	uint8_t i;
	const struct bt_mesh_elem *elem;
	int err = 0;

	elem = primary_element();
	if (elem == NULL) {
		shell_print(sh, "Element not found!");
		return -EINVAL;
	}

	if (argc < 2) {
		(void)memset(cur_faults, 0, sizeof(cur_faults));
		shell_print(sh, "All current faults cleared");
		bt_mesh_health_srv_fault_update(elem);
		return 0;
	}

	fault_id = shell_strtoul(argv[1], 0, &err);
	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	if (!fault_id) {
		shell_print(sh, "The Fault ID must be non-zero!");
		return -EINVAL;
	}

	for (i = 0U; i < sizeof(cur_faults); i++) {
		if (cur_faults[i] == fault_id) {
			cur_faults[i] = 0U;
			shell_print(sh, "Fault cleared");
		}
	}

	bt_mesh_health_srv_fault_update(elem);

	return 0;
}
#endif /* CONFIG_BT_MESH_SHELL_HEALTH_SRV_INSTANCE */
#endif /* CONFIG_BT_MESH_SHELL_TEST */

#if defined(CONFIG_BT_MESH_SHELL_CDB)
static int cmd_cdb_create(const struct shell *sh, size_t argc,
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
		shell_print(sh, "Failed to create CDB (err %d)", err);
	}

	return 0;
}

static int cmd_cdb_clear(const struct shell *sh, size_t argc,
			 char *argv[])
{
	bt_mesh_cdb_clear();

	shell_print(sh, "Cleared CDB");

	return 0;
}

static void cdb_print_nodes(const struct shell *sh)
{
	char key_hex_str[32 + 1], uuid_hex_str[32 + 1];
	struct bt_mesh_cdb_node *node;
	int i, total = 0;
	bool configured;
	uint8_t dev_key[16];

	shell_print(sh, "Address  Elements  Flags  %-32s  DevKey", "UUID");

	for (i = 0; i < ARRAY_SIZE(bt_mesh_cdb.nodes); ++i) {
		node = &bt_mesh_cdb.nodes[i];
		if (node->addr == BT_MESH_ADDR_UNASSIGNED) {
			continue;
		}

		configured = atomic_test_bit(node->flags,
					     BT_MESH_CDB_NODE_CONFIGURED);

		total++;
		bin2hex(node->uuid, 16, uuid_hex_str, sizeof(uuid_hex_str));
		if (bt_mesh_cdb_node_key_export(node, dev_key)) {
			shell_error(sh, "Unable to export key for node 0x%04x", node->addr);
			continue;
		}
		bin2hex(dev_key, 16, key_hex_str, sizeof(key_hex_str));
		shell_print(sh, "0x%04x   %-8d  %-5s  %s  %s", node->addr,
			    node->num_elem, configured ? "C" : "-",
			    uuid_hex_str, key_hex_str);
	}

	shell_print(sh, "> Total nodes: %d", total);
}

static void cdb_print_subnets(const struct shell *sh)
{
	struct bt_mesh_cdb_subnet *subnet;
	char key_hex_str[32 + 1];
	int i, total = 0;
	uint8_t net_key[16];

	shell_print(sh, "NetIdx  NetKey");

	for (i = 0; i < ARRAY_SIZE(bt_mesh_cdb.subnets); ++i) {
		subnet = &bt_mesh_cdb.subnets[i];
		if (subnet->net_idx == BT_MESH_KEY_UNUSED) {
			continue;
		}

		if (bt_mesh_cdb_subnet_key_export(subnet, 0, net_key)) {
			shell_error(sh, "Unable to export key for subnet 0x%03x",
					subnet->net_idx);
			continue;
		}

		total++;
		bin2hex(net_key, 16, key_hex_str, sizeof(key_hex_str));
		shell_print(sh, "0x%03x   %s", subnet->net_idx, key_hex_str);
	}

	shell_print(sh, "> Total subnets: %d", total);
}

static void cdb_print_app_keys(const struct shell *sh)
{
	struct bt_mesh_cdb_app_key *key;
	char key_hex_str[32 + 1];
	int i, total = 0;
	uint8_t app_key[16];

	shell_print(sh, "NetIdx  AppIdx  AppKey");

	for (i = 0; i < ARRAY_SIZE(bt_mesh_cdb.app_keys); ++i) {
		key = &bt_mesh_cdb.app_keys[i];
		if (key->net_idx == BT_MESH_KEY_UNUSED) {
			continue;
		}

		if (bt_mesh_cdb_app_key_export(key, 0, app_key)) {
			shell_error(sh, "Unable to export app key 0x%03x", key->app_idx);
			continue;
		}

		total++;
		bin2hex(app_key, 16, key_hex_str, sizeof(key_hex_str));
		shell_print(sh, "0x%03x   0x%03x   %s", key->net_idx, key->app_idx, key_hex_str);
	}

	shell_print(sh, "> Total app-keys: %d", total);
}

static int cmd_cdb_show(const struct shell *sh, size_t argc,
			char *argv[])
{
	if (!atomic_test_bit(bt_mesh_cdb.flags, BT_MESH_CDB_VALID)) {
		shell_print(sh, "No valid networks");
		return 0;
	}

	shell_print(sh, "Mesh Network Information");
	shell_print(sh, "========================");

	cdb_print_nodes(sh);
	shell_print(sh, "---");
	cdb_print_subnets(sh);
	shell_print(sh, "---");
	cdb_print_app_keys(sh);

	return 0;
}

static int cmd_cdb_node_add(const struct shell *sh, size_t argc,
			    char *argv[])
{
	struct bt_mesh_cdb_node *node;
	uint8_t uuid[16], dev_key[16];
	uint16_t addr, net_idx;
	uint8_t num_elem;
	size_t len;
	int err = 0;

	len = hex2bin(argv[1], strlen(argv[1]), uuid, sizeof(uuid));
	memset(uuid + len, 0, sizeof(uuid) - len);

	addr = shell_strtoul(argv[2], 0, &err);
	num_elem = shell_strtoul(argv[3], 0, &err);
	net_idx = shell_strtoul(argv[4], 0, &err);
	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	if (argc < 6) {
		bt_rand(dev_key, 16);
	} else {
		len = hex2bin(argv[5], strlen(argv[5]), dev_key,
			      sizeof(dev_key));
		memset(dev_key + len, 0, sizeof(dev_key) - len);
	}

	node = bt_mesh_cdb_node_alloc(uuid, addr, num_elem, net_idx);
	if (node == NULL) {
		shell_print(sh, "Failed to allocate node");
		return 0;
	}

	err = bt_mesh_cdb_node_key_import(node, dev_key);
	if (err) {
		shell_warn(sh, "Unable to import device key into cdb");
		return err;
	}

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		bt_mesh_cdb_node_store(node);
	}

	shell_print(sh, "Added node 0x%04x", node->addr);

	return 0;
}

static int cmd_cdb_node_del(const struct shell *sh, size_t argc,
			    char *argv[])
{
	struct bt_mesh_cdb_node *node;
	uint16_t addr;
	int err = 0;

	addr = shell_strtoul(argv[1], 0, &err);
	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	node = bt_mesh_cdb_node_get(addr);
	if (node == NULL) {
		shell_print(sh, "No node with address 0x%04x", addr);
		return 0;
	}

	bt_mesh_cdb_node_del(node, true);

	shell_print(sh, "Deleted node 0x%04x", addr);

	return 0;
}

static int cmd_cdb_subnet_add(const struct shell *sh, size_t argc,
			     char *argv[])
{
	struct bt_mesh_cdb_subnet *sub;
	uint8_t net_key[16];
	uint16_t net_idx;
	size_t len;
	int err = 0;

	net_idx = shell_strtoul(argv[1], 0, &err);
	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	if (argc < 3) {
		bt_rand(net_key, 16);
	} else {
		len = hex2bin(argv[2], strlen(argv[2]), net_key,
			      sizeof(net_key));
		memset(net_key + len, 0, sizeof(net_key) - len);
	}

	sub = bt_mesh_cdb_subnet_alloc(net_idx);
	if (sub == NULL) {
		shell_print(sh, "Could not add subnet");
		return 0;
	}

	if (bt_mesh_cdb_subnet_key_import(sub, 0, net_key)) {
		shell_error(sh, "Unable to import key for subnet 0x%03x", net_idx);
		return 0;
	}

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		bt_mesh_cdb_subnet_store(sub);
	}

	shell_print(sh, "Added Subnet 0x%03x", net_idx);

	return 0;
}

static int cmd_cdb_subnet_del(const struct shell *sh, size_t argc,
			     char *argv[])
{
	struct bt_mesh_cdb_subnet *sub;
	uint16_t net_idx;
	int err = 0;

	net_idx = shell_strtoul(argv[1], 0, &err);
	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	sub = bt_mesh_cdb_subnet_get(net_idx);
	if (sub == NULL) {
		shell_print(sh, "No subnet with NetIdx 0x%03x", net_idx);
		return 0;
	}

	bt_mesh_cdb_subnet_del(sub, true);

	shell_print(sh, "Deleted subnet 0x%03x", net_idx);

	return 0;
}

static int cmd_cdb_app_key_add(const struct shell *sh, size_t argc,
			      char *argv[])
{
	struct bt_mesh_cdb_app_key *key;
	uint16_t net_idx, app_idx;
	uint8_t app_key[16];
	size_t len;
	int err = 0;

	net_idx = shell_strtoul(argv[1], 0, &err);
	app_idx = shell_strtoul(argv[2], 0, &err);
	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	if (argc < 4) {
		bt_rand(app_key, 16);
	} else {
		len = hex2bin(argv[3], strlen(argv[3]), app_key,
			      sizeof(app_key));
		memset(app_key + len, 0, sizeof(app_key) - len);
	}

	key = bt_mesh_cdb_app_key_alloc(net_idx, app_idx);
	if (key == NULL) {
		shell_print(sh, "Could not add AppKey");
		return 0;
	}

	if (bt_mesh_cdb_app_key_import(key, 0, app_key)) {
		shell_error(sh, "Unable to import app key 0x%03x", app_idx);
		return 0;
	}

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		bt_mesh_cdb_app_key_store(key);
	}

	shell_print(sh, "Added AppKey 0x%03x", app_idx);

	return 0;
}

static int cmd_cdb_app_key_del(const struct shell *sh, size_t argc,
			      char *argv[])
{
	struct bt_mesh_cdb_app_key *key;
	uint16_t app_idx;
	int err = 0;

	app_idx = shell_strtoul(argv[1], 0, &err);
	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	key = bt_mesh_cdb_app_key_get(app_idx);
	if (key == NULL) {
		shell_print(sh, "No AppKey 0x%03x", app_idx);
		return 0;
	}

	bt_mesh_cdb_app_key_del(key, true);

	shell_print(sh, "Deleted AppKey 0x%03x", app_idx);

	return 0;
}
#endif /* CONFIG_BT_MESH_SHELL_CDB */

static int cmd_dst(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;

	if (argc < 2) {
		shell_print(sh, "Destination address: 0x%04x%s", bt_mesh_shell_target_ctx.dst,
			    bt_mesh_shell_target_ctx.dst == bt_mesh_primary_addr()
				    ? " (local)"
				    : "");
		return 0;
	}

	if (!strcmp(argv[1], "local")) {
		bt_mesh_shell_target_ctx.dst = bt_mesh_primary_addr();
	} else {
		bt_mesh_shell_target_ctx.dst = shell_strtoul(argv[1], 0, &err);
		if (err) {
			shell_warn(sh, "Unable to parse input string argument");
			return err;
		}
	}

	shell_print(sh, "Destination address set to 0x%04x%s", bt_mesh_shell_target_ctx.dst,
		    bt_mesh_shell_target_ctx.dst == bt_mesh_primary_addr() ? " (local)"
										   : "");
	return 0;
}

static int cmd_netidx(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;

	if (argc < 2) {
		shell_print(sh, "NetIdx: 0x%04x", bt_mesh_shell_target_ctx.net_idx);
		return 0;
	}

	bt_mesh_shell_target_ctx.net_idx = shell_strtoul(argv[1], 0, &err);
	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	shell_print(sh, "NetIdx set to 0x%04x", bt_mesh_shell_target_ctx.net_idx);
	return 0;
}

static int cmd_appidx(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;

	if (argc < 2) {
		shell_print(sh, "AppIdx: 0x%04x", bt_mesh_shell_target_ctx.app_idx);
		return 0;
	}

	bt_mesh_shell_target_ctx.app_idx = shell_strtoul(argv[1], 0, &err);
	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	shell_print(sh, "AppIdx set to 0x%04x", bt_mesh_shell_target_ctx.app_idx);
	return 0;
}

#if defined(CONFIG_BT_MESH_STATISTIC)
static int cmd_stat_get(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_mesh_statistic st;

	bt_mesh_stat_get(&st);

	shell_print(sh, "Received frames over:");
	shell_print(sh, "adv:       %d", st.rx_adv);
	shell_print(sh, "loopback:  %d", st.rx_loopback);
	shell_print(sh, "proxy:     %d", st.rx_proxy);
	shell_print(sh, "unknown:   %d", st.rx_uknown);

	shell_print(sh, "Transmitted frames: <planned> - <succeeded>");
	shell_print(sh, "relay adv:   %d - %d", st.tx_adv_relay_planned, st.tx_adv_relay_succeeded);
	shell_print(sh, "local adv:   %d - %d", st.tx_local_planned, st.tx_local_succeeded);
	shell_print(sh, "friend:      %d - %d", st.tx_friend_planned, st.tx_friend_succeeded);

	return 0;
}

static int cmd_stat_clear(const struct shell *sh, size_t argc, char *argv[])
{
	bt_mesh_stat_reset();

	return 0;
}
#endif

#if defined(CONFIG_BT_MESH_SHELL_CDB)
SHELL_STATIC_SUBCMD_SET_CREATE(
	cdb_cmds,
	/* Mesh Configuration Database Operations */
	SHELL_CMD_ARG(create, NULL, "[NetKey(1-16 hex)]", cmd_cdb_create, 1, 1),
	SHELL_CMD_ARG(clear, NULL, NULL, cmd_cdb_clear, 1, 0),
	SHELL_CMD_ARG(show, NULL, NULL, cmd_cdb_show, 1, 0),
	SHELL_CMD_ARG(node-add, NULL,
		      "<UUID(1-16 hex)> <Addr> <ElemCnt> <NetKeyIdx> [DevKey(1-16 hex)]",
		      cmd_cdb_node_add, 5, 1),
	SHELL_CMD_ARG(node-del, NULL, "<Addr>", cmd_cdb_node_del, 2, 0),
	SHELL_CMD_ARG(subnet-add, NULL, "<NetKeyIdx> [<NetKey(1-16 hex)>]", cmd_cdb_subnet_add, 2,
		      1),
	SHELL_CMD_ARG(subnet-del, NULL, "<NetKeyIdx>", cmd_cdb_subnet_del, 2, 0),
	SHELL_CMD_ARG(app-key-add, NULL, "<NetKeyIdx> <AppKeyIdx> [<AppKey(1-16 hex)>]",
		      cmd_cdb_app_key_add, 3, 1),
	SHELL_CMD_ARG(app-key-del, NULL, "<AppKeyIdx>", cmd_cdb_app_key_del, 2, 0),
	SHELL_SUBCMD_SET_END);
#endif

#if defined(CONFIG_BT_MESH_SHELL_PROV)
#if defined(CONFIG_BT_MESH_PROVISIONER)
SHELL_STATIC_SUBCMD_SET_CREATE(auth_cmds,
	SHELL_CMD_ARG(input, NULL, "<Action> <Size>",
		      cmd_auth_method_set_input, 3, 0),
	SHELL_CMD_ARG(output, NULL, "<Action> <Size>",
		      cmd_auth_method_set_output, 3, 0),
	SHELL_CMD_ARG(static, NULL, "<Val(1-16 hex)>", cmd_auth_method_set_static, 2,
		      0),
	SHELL_CMD_ARG(none, NULL, NULL, cmd_auth_method_set_none, 1, 0),
	SHELL_SUBCMD_SET_END);
#endif

SHELL_STATIC_SUBCMD_SET_CREATE(
	prov_cmds, SHELL_CMD_ARG(input-num, NULL, "<Number>", cmd_input_num, 2, 0),
	SHELL_CMD_ARG(input-str, NULL, "<String>", cmd_input_str, 2, 0),
	SHELL_CMD_ARG(local, NULL, "<NetKeyIdx> <Addr> [IVI]", cmd_provision_local, 3, 1),
#if defined(CONFIG_BT_MESH_SHELL_PROV_CTX_INSTANCE)
	SHELL_CMD_ARG(static-oob, NULL, "[Val]", cmd_static_oob, 2, 1),
	SHELL_CMD_ARG(uuid, NULL, "[UUID(1-16 hex)]", cmd_uuid, 1, 1),
	SHELL_CMD_ARG(beacon-listen, NULL, "<Val(off, on)>", cmd_beacon_listen, 2, 0),
#endif

	SHELL_CMD_ARG(comp-change, NULL, NULL, cmd_comp_change, 1, 0),

/* Provisioning operations */
#if defined(CONFIG_BT_MESH_PROVISIONEE)
#if defined(CONFIG_BT_MESH_PB_GATT)
	SHELL_CMD_ARG(pb-gatt, NULL, "<Val(off, on)>", cmd_pb_gatt, 2, 0),
#endif
#if defined(CONFIG_BT_MESH_PB_ADV)
	SHELL_CMD_ARG(pb-adv, NULL, "<Val(off, on)>", cmd_pb_adv, 2, 0),
#endif
#endif /* CONFIG_BT_MESH_PROVISIONEE */

#if defined(CONFIG_BT_MESH_PROVISIONER)
	SHELL_CMD(auth-method, &auth_cmds, "Authentication methods", bt_mesh_shell_mdl_cmds_help),
	SHELL_CMD_ARG(remote-pub-key, NULL, "<PubKey>", cmd_remote_pub_key_set, 2, 0),
	SHELL_CMD_ARG(remote-adv, NULL,
		      "<UUID(1-16 hex)> <NetKeyIdx> <Addr> "
		      "<AttDur(s)>",
		      cmd_provision_adv, 5, 0),
#endif

#if defined(CONFIG_BT_MESH_PB_GATT_CLIENT)
	SHELL_CMD_ARG(remote-gatt, NULL,
		      "<UUID(1-16 hex)> <NetKeyIdx> <Addr> "
		      "<AttDur(s)>",
		      cmd_provision_gatt, 5, 0),
#endif
	SHELL_SUBCMD_SET_END);
#endif /* CONFIG_BT_MESH_SHELL_PROV */

#if defined(CONFIG_BT_MESH_SHELL_TEST)
#if defined(CONFIG_BT_MESH_SHELL_HEALTH_SRV_INSTANCE)
SHELL_STATIC_SUBCMD_SET_CREATE(health_srv_cmds,
	/* Health Server Model Operations */
	SHELL_CMD_ARG(add-fault, NULL, "<FaultID>", cmd_add_fault, 2, 0),
	SHELL_CMD_ARG(del-fault, NULL, "[FaultID]", cmd_del_fault, 1, 1),
	SHELL_SUBCMD_SET_END);
#endif

SHELL_STATIC_SUBCMD_SET_CREATE(test_cmds,
	/* Commands which access internal APIs, for testing only */
	SHELL_CMD_ARG(net-send, NULL, "<HexString>", cmd_net_send,
		      2, 0),
#if defined(CONFIG_BT_MESH_IV_UPDATE_TEST)
	SHELL_CMD_ARG(iv-update, NULL, NULL, cmd_iv_update, 1, 0),
	SHELL_CMD_ARG(iv-update-test, NULL, "<Val(off, on)>", cmd_iv_update_test, 2, 0),
#endif
	SHELL_CMD_ARG(rpl-clear, NULL, NULL, cmd_rpl_clear, 1, 0),
#if defined(CONFIG_BT_MESH_SHELL_HEALTH_SRV_INSTANCE)
	SHELL_CMD(health-srv, &health_srv_cmds, "Health Server test", bt_mesh_shell_mdl_cmds_help),
#endif
	SHELL_SUBCMD_SET_END);
#endif /* CONFIG_BT_MESH_SHELL_TEST */

#if defined(CONFIG_BT_MESH_SHELL_GATT_PROXY)
SHELL_STATIC_SUBCMD_SET_CREATE(proxy_cmds,
#if defined(CONFIG_BT_MESH_GATT_PROXY)
	SHELL_CMD_ARG(identity-enable, NULL, NULL, cmd_ident, 1, 0),
#endif

#if defined(CONFIG_BT_MESH_PROXY_CLIENT)
	SHELL_CMD_ARG(connect, NULL, "<NetKeyIdx>", cmd_proxy_connect, 2, 0),
	SHELL_CMD_ARG(disconnect, NULL, "<NetKeyIdx>", cmd_proxy_disconnect, 2, 0),
#endif

#if defined(CONFIG_BT_MESH_PROXY_SOLICITATION)
	SHELL_CMD_ARG(solicit, NULL, "<NetKeyIdx>",
		      cmd_proxy_solicit, 2, 0),
#endif
	SHELL_SUBCMD_SET_END);
#endif /* CONFIG_BT_MESH_SHELL_GATT_PROXY */

#if defined(CONFIG_BT_MESH_SHELL_LOW_POWER)
SHELL_STATIC_SUBCMD_SET_CREATE(low_pwr_cmds,
	SHELL_CMD_ARG(set, NULL, "<Val(off, on)>", cmd_lpn, 2, 0),
	SHELL_CMD_ARG(poll, NULL, NULL, cmd_poll, 1, 0),
	SHELL_SUBCMD_SET_END);
#endif

SHELL_STATIC_SUBCMD_SET_CREATE(target_cmds,
	SHELL_CMD_ARG(dst, NULL, "[DstAddr]", cmd_dst, 1, 1),
	SHELL_CMD_ARG(net, NULL, "[NetKeyIdx]", cmd_netidx, 1, 1),
	SHELL_CMD_ARG(app, NULL, "[AppKeyIdx]", cmd_appidx, 1, 1),
	SHELL_SUBCMD_SET_END);

#if defined(CONFIG_BT_MESH_STATISTIC)
SHELL_STATIC_SUBCMD_SET_CREATE(stat_cmds,
	SHELL_CMD_ARG(get, NULL, NULL, cmd_stat_get, 1, 0),
	SHELL_CMD_ARG(clear, NULL, NULL, cmd_stat_clear, 1, 0),
	SHELL_SUBCMD_SET_END);
#endif

/* Placeholder for model shell modules that is configured in the application */
SHELL_SUBCMD_SET_CREATE(model_cmds, (mesh, models));

/* List of Mesh subcommands.
 *
 * Each command is documented in doc/reference/bluetooth/mesh/shell.rst.
 *
 * Please keep the documentation up to date by adding any new commands to the
 * list.
 */
SHELL_STATIC_SUBCMD_SET_CREATE(mesh_cmds,
	SHELL_CMD_ARG(init, NULL, NULL, cmd_init, 1, 0),
	SHELL_CMD_ARG(reset-local, NULL, NULL, cmd_reset, 1, 0),

	SHELL_CMD(models, &model_cmds, "Model commands", bt_mesh_shell_mdl_cmds_help),

#if defined(CONFIG_BT_MESH_SHELL_LOW_POWER)
	SHELL_CMD(lpn, &low_pwr_cmds, "Low Power commands", bt_mesh_shell_mdl_cmds_help),
#endif

#if defined(CONFIG_BT_MESH_SHELL_CDB)
	SHELL_CMD(cdb, &cdb_cmds, "Configuration Database", bt_mesh_shell_mdl_cmds_help),
#endif

#if defined(CONFIG_BT_MESH_SHELL_GATT_PROXY)
	SHELL_CMD(proxy, &proxy_cmds, "Proxy commands", bt_mesh_shell_mdl_cmds_help),
#endif

#if defined(CONFIG_BT_MESH_SHELL_PROV)
	SHELL_CMD(prov, &prov_cmds, "Provisioning commands", bt_mesh_shell_mdl_cmds_help),
#endif

#if defined(CONFIG_BT_MESH_SHELL_TEST)
	SHELL_CMD(test, &test_cmds, "Test commands", bt_mesh_shell_mdl_cmds_help),
#endif
	SHELL_CMD(target, &target_cmds, "Target commands", bt_mesh_shell_mdl_cmds_help),

#if defined(CONFIG_BT_MESH_STATISTIC)
	SHELL_CMD(stat, &stat_cmds, "Statistic commands", bt_mesh_shell_mdl_cmds_help),
#endif

	SHELL_SUBCMD_SET_END
);

SHELL_CMD_ARG_REGISTER(mesh, &mesh_cmds, "Bluetooth Mesh shell commands",
			bt_mesh_shell_mdl_cmds_help, 1, 1);
