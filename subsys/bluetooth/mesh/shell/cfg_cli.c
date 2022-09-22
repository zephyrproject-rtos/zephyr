/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/shell/shell.h>
#include <zephyr/bluetooth/mesh.h>

#include "../net.h"
#include "../access.h"
#include "utils.h"
#include <zephyr/bluetooth/mesh/shell.h>

#define CID_NVAL 0xffff

/* Default net & app key values, unless otherwise specified */
static const uint8_t default_key[16] = {
	0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
	0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
};

static int cmd_reset(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	bool reset = false;

	err = bt_mesh_cfg_cli_node_reset(bt_mesh_shell_target_ctx.net_idx,
					 bt_mesh_shell_target_ctx.dst, &reset);
	if (err) {
		shell_error(sh, "Unable to send Remote Node Reset (err %d)", err);
		return 0;
	}

	if (IS_ENABLED(CONFIG_BT_MESH_CDB)) {
		struct bt_mesh_cdb_node *node = bt_mesh_cdb_node_get(bt_mesh_shell_target_ctx.dst);

		if (node) {
			bt_mesh_cdb_node_del(node, true);
		}
	}

	shell_print(sh, "Remote node reset complete");

	return 0;
}

static int cmd_timeout(const struct shell *sh, size_t argc, char *argv[])
{
	int32_t timeout_ms;
	int err = 0;

	if (argc == 2) {
		int32_t timeout_s = shell_strtol(argv[1], 0, &err);

		if (err) {
			shell_warn(sh, "Unable to parse input string argument");
			return err;
		}

		if (timeout_s < 0 || timeout_s > (INT32_MAX / 1000)) {
			timeout_ms = SYS_FOREVER_MS;
		} else {
			timeout_ms = timeout_s * MSEC_PER_SEC;
		}

		bt_mesh_cfg_cli_timeout_set(timeout_ms);
	}

	timeout_ms = bt_mesh_cfg_cli_timeout_get();
	if (timeout_ms == SYS_FOREVER_MS) {
		shell_print(sh, "Message timeout: forever");
	} else {
		shell_print(sh, "Message timeout: %u seconds", timeout_ms / 1000);
	}

	return 0;
}

static int cmd_get_comp(const struct shell *sh, size_t argc, char *argv[])
{
	NET_BUF_SIMPLE_DEFINE(buf, BT_MESH_RX_SDU_MAX);
	struct bt_mesh_comp_p0_elem elem;
	struct bt_mesh_comp_p0 comp;
	uint8_t page = 0x00;
	int err = 0;

	if (argc > 1) {
		page = shell_strtoul(argv[1], 0, &err);
		if (err) {
			shell_warn(sh, "Unable to parse input string argument");
			return err;
		}
	}

	err = bt_mesh_cfg_cli_comp_data_get(bt_mesh_shell_target_ctx.net_idx,
					bt_mesh_shell_target_ctx.dst, page, &page, &buf);
	if (err) {
		shell_error(sh, "Getting composition failed (err %d)", err);
		return 0;
	}

	if (page != 0x00) {
		shell_print(sh, "Got page 0x%02x. No parser available.", page);
		return 0;
	}

	err = bt_mesh_comp_p0_get(&comp, &buf);
	if (err) {
		shell_error(sh, "Couldn't parse Composition data (err %d)", err);
		return 0;
	}

	shell_print(sh, "Got Composition Data for 0x%04x:", bt_mesh_shell_target_ctx.dst);
	shell_print(sh, "\tCID      0x%04x", comp.cid);
	shell_print(sh, "\tPID      0x%04x", comp.pid);
	shell_print(sh, "\tVID      0x%04x", comp.vid);
	shell_print(sh, "\tCRPL     0x%04x", comp.crpl);
	shell_print(sh, "\tFeatures 0x%04x", comp.feat);

	while (bt_mesh_comp_p0_elem_pull(&comp, &elem)) {
		int i;

		shell_print(sh, "\tElement @ 0x%04x:", elem.loc);

		if (elem.nsig) {
			shell_print(sh, "\t\tSIG Models:");
		} else {
			shell_print(sh, "\t\tNo SIG Models");
		}

		for (i = 0; i < elem.nsig; i++) {
			uint16_t mod_id = bt_mesh_comp_p0_elem_mod(&elem, i);

			shell_print(sh, "\t\t\t0x%04x", mod_id);
		}

		if (elem.nvnd) {
			shell_print(sh, "\t\tVendor Models:");
		} else {
			shell_print(sh, "\t\tNo Vendor Models");
		}

		for (i = 0; i < elem.nvnd; i++) {
			struct bt_mesh_mod_id_vnd mod = bt_mesh_comp_p0_elem_mod_vnd(&elem, i);

			shell_print(sh, "\t\t\tCompany 0x%04x: 0x%04x", mod.company, mod.id);
		}
	}

	if (buf.len) {
		shell_print(sh, "\t\t...truncated data!");
	}

	return 0;
}

static int cmd_beacon(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t status;
	int err = 0;

	if (argc < 2) {
		err = bt_mesh_cfg_cli_beacon_get(bt_mesh_shell_target_ctx.net_idx,
					     bt_mesh_shell_target_ctx.dst, &status);
	} else {
		uint8_t val = shell_strtobool(argv[1], 0, &err);

		if (err) {
			shell_warn(sh, "Unable to parse input string argument");
			return err;
		}

		err = bt_mesh_cfg_cli_beacon_set(bt_mesh_shell_target_ctx.net_idx,
					     bt_mesh_shell_target_ctx.dst, val, &status);
	}

	if (err) {
		shell_error(sh, "Unable to send Beacon Get/Set message (err %d)", err);
		return 0;
	}

	shell_print(sh, "Beacon state is 0x%02x", status);

	return 0;
}

static int cmd_ttl(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t ttl;
	int err = 0;

	if (argc < 2) {
		err = bt_mesh_cfg_cli_ttl_get(bt_mesh_shell_target_ctx.net_idx,
					  bt_mesh_shell_target_ctx.dst, &ttl);
	} else {
		uint8_t val = shell_strtoul(argv[1], 0, &err);

		if (err) {
			shell_warn(sh, "Unable to parse input string argument");
			return err;
		}

		err = bt_mesh_cfg_cli_ttl_set(bt_mesh_shell_target_ctx.net_idx,
					  bt_mesh_shell_target_ctx.dst, val, &ttl);
	}

	if (err) {
		shell_error(sh, "Unable to send Default TTL Get/Set (err %d)", err);
		return 0;
	}

	shell_print(sh, "Default TTL is 0x%02x", ttl);

	return 0;
}

static int cmd_friend(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t frnd;
	int err = 0;

	if (argc < 2) {
		err = bt_mesh_cfg_cli_friend_get(bt_mesh_shell_target_ctx.net_idx,
					     bt_mesh_shell_target_ctx.dst, &frnd);
	} else {
		uint8_t val = shell_strtobool(argv[1], 0, &err);

		if (err) {
			shell_warn(sh, "Unable to parse input string argument");
			return err;
		}

		err = bt_mesh_cfg_cli_friend_set(bt_mesh_shell_target_ctx.net_idx,
					     bt_mesh_shell_target_ctx.dst, val, &frnd);
	}

	if (err) {
		shell_error(sh, "Unable to send Friend Get/Set (err %d)", err);
		return 0;
	}

	shell_print(sh, "Friend is set to 0x%02x", frnd);

	return 0;
}

static int cmd_gatt_proxy(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t proxy;
	int err = 0;

	if (argc < 2) {
		err = bt_mesh_cfg_cli_gatt_proxy_get(bt_mesh_shell_target_ctx.net_idx,
						 bt_mesh_shell_target_ctx.dst, &proxy);
	} else {
		uint8_t val = shell_strtobool(argv[1], 0, &err);

		if (err) {
			shell_warn(sh, "Unable to parse input string argument");
			return err;
		}

		err = bt_mesh_cfg_cli_gatt_proxy_set(bt_mesh_shell_target_ctx.net_idx,
						 bt_mesh_shell_target_ctx.dst, val, &proxy);
	}

	if (err) {
		shell_print(sh, "Unable to send GATT Proxy Get/Set (err %d)", err);
		return 0;
	}

	shell_print(sh, "GATT Proxy is set to 0x%02x", proxy);

	return 0;
}

static int cmd_polltimeout_get(const struct shell *sh, size_t argc, char *argv[])
{
	uint16_t lpn_address;
	int32_t poll_timeout;
	int err = 0;

	lpn_address = shell_strtoul(argv[1], 0, &err);
	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	err = bt_mesh_cfg_cli_lpn_timeout_get(bt_mesh_shell_target_ctx.net_idx,
					  bt_mesh_shell_target_ctx.dst, lpn_address, &poll_timeout);
	if (err) {
		shell_error(sh, "Unable to send LPN PollTimeout Get (err %d)", err);
		return 0;
	}

	shell_print(sh, "PollTimeout value %d", poll_timeout);

	return 0;
}

static int cmd_net_transmit(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t transmit;
	int err = 0;

	if (argc < 2) {
		err = bt_mesh_cfg_cli_net_transmit_get(bt_mesh_shell_target_ctx.net_idx,
						   bt_mesh_shell_target_ctx.dst, &transmit);
	} else {
		if (argc != 3) {
			shell_warn(sh, "Wrong number of input arguments"
					"(2 arguments are required)");
			return -EINVAL;
		}

		uint8_t count, interval, new_transmit;

		count = shell_strtoul(argv[1], 0, &err);
		interval = shell_strtoul(argv[2], 0, &err);
		if (err) {
			shell_warn(sh, "Unable to parse input string argument");
			return err;
		}

		new_transmit = BT_MESH_TRANSMIT(count, interval);

		err = bt_mesh_cfg_cli_net_transmit_set(bt_mesh_shell_target_ctx.net_idx,
						   bt_mesh_shell_target_ctx.dst, new_transmit,
						   &transmit);
	}

	if (err) {
		shell_error(sh, "Unable to send network transmit Get/Set (err %d)", err);
		return 0;
	}

	shell_print(sh, "Transmit 0x%02x (count %u interval %ums)", transmit,
		    BT_MESH_TRANSMIT_COUNT(transmit), BT_MESH_TRANSMIT_INT(transmit));

	return 0;
}

static int cmd_relay(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t relay, transmit;
	int err = 0;

	if (argc < 2) {
		err = bt_mesh_cfg_cli_relay_get(bt_mesh_shell_target_ctx.net_idx,
					    bt_mesh_shell_target_ctx.dst, &relay, &transmit);
	} else {
		uint8_t count, interval, new_transmit;
		uint8_t val = shell_strtobool(argv[1], 0, &err);

		if (val) {
			if (argc > 2) {
				count = shell_strtoul(argv[2], 0, &err);
			} else {
				count = 2U;
			}

			if (argc > 3) {
				interval = shell_strtoul(argv[3], 0, &err);
			} else {
				interval = 20U;
			}

			new_transmit = BT_MESH_TRANSMIT(count, interval);
		} else {
			new_transmit = 0U;
		}

		if (err) {
			shell_warn(sh, "Unable to parse input string argument");
			return err;
		}

		err = bt_mesh_cfg_cli_relay_set(bt_mesh_shell_target_ctx.net_idx,
					    bt_mesh_shell_target_ctx.dst, val, new_transmit, &relay,
					    &transmit);
	}

	if (err) {
		shell_error(sh, "Unable to send Relay Get/Set (err %d)", err);
		return 0;
	}

	shell_print(sh, "Relay is 0x%02x, Transmit 0x%02x (count %u interval %ums)", relay,
		    transmit, BT_MESH_TRANSMIT_COUNT(transmit), BT_MESH_TRANSMIT_INT(transmit));

	return 0;
}

static int cmd_net_key_add(const struct shell *sh, size_t argc, char *argv[])
{
	bool has_key_val = (argc > 2);
	uint8_t key_val[16];
	uint16_t key_net_idx;
	uint8_t status;
	int err = 0;

	key_net_idx = shell_strtoul(argv[1], 0, &err);
	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	if (has_key_val) {
		size_t len;

		len = hex2bin(argv[3], strlen(argv[3]), key_val, sizeof(key_val));
		(void)memset(key_val, 0, sizeof(key_val) - len);
	} else {
		memcpy(key_val, default_key, sizeof(key_val));
	}

	if (IS_ENABLED(CONFIG_BT_MESH_CDB)) {
		struct bt_mesh_cdb_subnet *subnet;

		subnet = bt_mesh_cdb_subnet_get(key_net_idx);
		if (subnet) {
			if (has_key_val) {
				shell_error(sh, "Subnet 0x%03x already has a value", key_net_idx);
				return 0;
			}

			memcpy(key_val, subnet->keys[0].net_key, 16);
		} else {
			subnet = bt_mesh_cdb_subnet_alloc(key_net_idx);
			if (!subnet) {
				shell_error(sh, "No space for subnet in cdb");
				return 0;
			}

			memcpy(subnet->keys[0].net_key, key_val, 16);
			bt_mesh_cdb_subnet_store(subnet);
		}
	}

	err = bt_mesh_cfg_cli_net_key_add(bt_mesh_shell_target_ctx.net_idx,
				      bt_mesh_shell_target_ctx.dst, key_net_idx, key_val, &status);
	if (err) {
		shell_print(sh, "Unable to send NetKey Add (err %d)", err);
		return 0;
	}

	if (status) {
		shell_print(sh, "NetKeyAdd failed with status 0x%02x", status);
	} else {
		shell_print(sh, "NetKey added with NetKey Index 0x%03x", key_net_idx);
	}

	return 0;
}

static int cmd_net_key_update(const struct shell *sh, size_t argc, char *argv[])
{
	bool has_key_val = (argc > 2);
	uint8_t key_val[16];
	uint16_t key_net_idx;
	uint8_t status;
	int err = 0;

	key_net_idx = shell_strtoul(argv[1], 0, &err);
	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	if (has_key_val) {
		size_t len;

		len = hex2bin(argv[2], strlen(argv[2]), key_val, sizeof(key_val));
		(void)memset(key_val, 0, sizeof(key_val) - len);
	} else {
		memcpy(key_val, default_key, sizeof(key_val));
	}

	err = bt_mesh_cfg_cli_net_key_update(bt_mesh_shell_target_ctx.net_idx,
					 bt_mesh_shell_target_ctx.dst, key_net_idx, key_val,
					 &status);
	if (err) {
		shell_print(sh, "Unable to send NetKey Update (err %d)", err);
		return 0;
	}

	if (status) {
		shell_print(sh, "NetKeyUpdate failed with status 0x%02x", status);
	} else {
		shell_print(sh, "NetKey updated with NetKey Index 0x%03x", key_net_idx);
	}

	return 0;
}

static int cmd_net_key_get(const struct shell *sh, size_t argc, char *argv[])
{
	uint16_t keys[16];
	size_t cnt;
	int err, i;

	cnt = ARRAY_SIZE(keys);

	err = bt_mesh_cfg_cli_net_key_get(bt_mesh_shell_target_ctx.net_idx,
				      bt_mesh_shell_target_ctx.dst, keys, &cnt);
	if (err) {
		shell_print(sh, "Unable to send NetKeyGet (err %d)", err);
		return 0;
	}

	shell_print(sh, "NetKeys known by 0x%04x:", bt_mesh_shell_target_ctx.dst);
	for (i = 0; i < cnt; i++) {
		shell_print(sh, "\t0x%03x", keys[i]);
	}

	return 0;
}

static int cmd_net_key_del(const struct shell *sh, size_t argc, char *argv[])
{
	uint16_t key_net_idx;
	uint8_t status;
	int err = 0;

	key_net_idx = shell_strtoul(argv[1], 0, &err);
	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	err = bt_mesh_cfg_cli_net_key_del(bt_mesh_shell_target_ctx.net_idx,
				      bt_mesh_shell_target_ctx.dst, key_net_idx, &status);
	if (err) {
		shell_print(sh, "Unable to send NetKeyDel (err %d)", err);
		return 0;
	}

	if (status) {
		shell_print(sh, "NetKeyDel failed with status 0x%02x", status);
	} else {
		shell_print(sh, "NetKey 0x%03x deleted", key_net_idx);
	}

	return 0;
}

static int cmd_app_key_add(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t key_val[16];
	uint16_t key_net_idx, key_app_idx;
	bool has_key_val = (argc > 3);
	uint8_t status;
	int err = 0;

	key_net_idx = shell_strtoul(argv[1], 0, &err);
	key_app_idx = shell_strtoul(argv[2], 0, &err);
	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	if (has_key_val) {
		size_t len;

		len = hex2bin(argv[3], strlen(argv[3]), key_val, sizeof(key_val));
		(void)memset(key_val + len, 0, sizeof(key_val) - len);
	} else {
		memcpy(key_val, default_key, sizeof(key_val));
	}

	if (IS_ENABLED(CONFIG_BT_MESH_CDB)) {
		struct bt_mesh_cdb_app_key *app_key;

		app_key = bt_mesh_cdb_app_key_get(key_app_idx);
		if (app_key) {
			if (has_key_val) {
				shell_error(sh, "App key 0x%03x already has a value", key_app_idx);
				return 0;
			}

			memcpy(key_val, app_key->keys[0].app_key, 16);
		} else {
			app_key = bt_mesh_cdb_app_key_alloc(key_net_idx, key_app_idx);
			if (!app_key) {
				shell_error(sh, "No space for app key in cdb");
				return 0;
			}

			memcpy(app_key->keys[0].app_key, key_val, 16);
			bt_mesh_cdb_app_key_store(app_key);
		}
	}

	err = bt_mesh_cfg_cli_app_key_add(bt_mesh_shell_target_ctx.net_idx,
				      bt_mesh_shell_target_ctx.dst, key_net_idx, key_app_idx,
				      key_val, &status);
	if (err) {
		shell_error(sh, "Unable to send App Key Add (err %d)", err);
		return 0;
	}

	if (status) {
		shell_print(sh, "AppKeyAdd failed with status 0x%02x", status);
	} else {
		shell_print(sh, "AppKey added, NetKeyIndex 0x%04x AppKeyIndex 0x%04x", key_net_idx,
			    key_app_idx);
	}

	return 0;
}

static int cmd_app_key_upd(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t key_val[16];
	uint16_t key_net_idx, key_app_idx;
	bool has_key_val = (argc > 3);
	uint8_t status;
	int err = 0;

	key_net_idx = shell_strtoul(argv[1], 0, &err);
	key_app_idx = shell_strtoul(argv[2], 0, &err);
	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	if (has_key_val) {
		size_t len;

		len = hex2bin(argv[3], strlen(argv[3]), key_val, sizeof(key_val));
		(void)memset(key_val, 0, sizeof(key_val) - len);
	} else {
		memcpy(key_val, default_key, sizeof(key_val));
	}

	err = bt_mesh_cfg_cli_app_key_update(bt_mesh_shell_target_ctx.net_idx,
					 bt_mesh_shell_target_ctx.dst, key_net_idx, key_app_idx,
					 key_val, &status);
	if (err) {
		shell_error(sh, "Unable to send App Key Update (err %d)", err);
		return 0;
	}

	if (status) {
		shell_print(sh, "AppKey update failed with status 0x%02x", status);
	} else {
		shell_print(sh, "AppKey updated, NetKeyIndex 0x%04x AppKeyIndex 0x%04x",
			    key_net_idx, key_app_idx);
	}

	return 0;
}

static int cmd_app_key_get(const struct shell *sh, size_t argc, char *argv[])
{
	uint16_t net_idx;
	uint16_t keys[16];
	size_t cnt;
	uint8_t status;
	int err = 0;
	int i;

	cnt = ARRAY_SIZE(keys);
	net_idx = shell_strtoul(argv[1], 0, &err);
	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	err = bt_mesh_cfg_cli_app_key_get(bt_mesh_shell_target_ctx.net_idx,
				      bt_mesh_shell_target_ctx.dst, net_idx, &status, keys, &cnt);
	if (err) {
		shell_print(sh, "Unable to send AppKeyGet (err %d)", err);
		return 0;
	}

	if (status) {
		shell_print(sh, "AppKeyGet failed with status 0x%02x", status);
		return 0;
	}

	shell_print(sh, "AppKeys for NetKey 0x%03x known by 0x%04x:", net_idx,
		    bt_mesh_shell_target_ctx.dst);
	for (i = 0; i < cnt; i++) {
		shell_print(sh, "\t0x%03x", keys[i]);
	}

	return 0;
}

static int cmd_node_id(const struct shell *sh, size_t argc, char *argv[])
{
	uint16_t net_idx;
	uint8_t status, identify;
	int err = 0;

	net_idx = shell_strtoul(argv[1], 0, &err);
	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	if (argc <= 2) {
		printk("ANders\n");
		err = bt_mesh_cfg_cli_node_identity_get(bt_mesh_shell_target_ctx.net_idx,
						    bt_mesh_shell_target_ctx.dst, net_idx, &status,
						    &identify);
		if (err) {
			shell_print(sh, "Unable to send Node Identify Get (err %d)", err);
			return 0;
		}
	} else {
		uint8_t new_identify = shell_strtoul(argv[2], 0, &err);

		if (err) {
			shell_warn(sh, "Unable to parse input string argument");
			return err;
		}

		err = bt_mesh_cfg_cli_node_identity_set(bt_mesh_shell_target_ctx.net_idx,
						    bt_mesh_shell_target_ctx.dst, net_idx,
						    new_identify, &status, &identify);
		if (err) {
			shell_print(sh, "Unable to send Node Identify Set (err %d)", err);
			return 0;
		}
	}

	if (status) {
		shell_print(sh, "Node Identify Get/Set failed with status 0x%02x", status);
	} else {
		shell_print(sh, "Node Identify Get/Set successful with identify 0x%02x", identify);
	}

	return 0;
}

static int cmd_app_key_del(const struct shell *sh, size_t argc, char *argv[])
{
	uint16_t key_net_idx, key_app_idx;
	uint8_t status;
	int err = 0;

	key_net_idx = shell_strtoul(argv[1], 0, &err);
	key_app_idx = shell_strtoul(argv[2], 0, &err);
	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	err = bt_mesh_cfg_cli_app_key_del(bt_mesh_shell_target_ctx.net_idx,
				      bt_mesh_shell_target_ctx.dst, key_net_idx, key_app_idx,
				      &status);
	if (err) {
		shell_error(sh, "Unable to send App Key del(err %d)", err);
		return 0;
	}

	if (status) {
		shell_print(sh, "AppKeyDel failed with status 0x%02x", status);
	} else {
		shell_print(sh, "AppKey deleted, NetKeyIndex 0x%04x AppKeyIndex 0x%04x",
			    key_net_idx, key_app_idx);
	}

	return 0;
}

static int cmd_mod_app_bind(const struct shell *sh, size_t argc, char *argv[])
{
	uint16_t elem_addr, mod_app_idx, mod_id, cid;
	uint8_t status;
	int err = 0;

	elem_addr = shell_strtoul(argv[1], 0, &err);
	mod_app_idx = shell_strtoul(argv[2], 0, &err);
	mod_id = shell_strtoul(argv[3], 0, &err);
	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	if (argc > 4) {
		cid = shell_strtoul(argv[4], 0, &err);
		if (err) {
			shell_warn(sh, "Unable to parse input string argument");
			return err;
		}

		err = bt_mesh_cfg_cli_mod_app_bind_vnd(bt_mesh_shell_target_ctx.net_idx,
						   bt_mesh_shell_target_ctx.dst, elem_addr,
						   mod_app_idx, mod_id, cid, &status);
	} else {
		err = bt_mesh_cfg_cli_mod_app_bind(bt_mesh_shell_target_ctx.net_idx,
					       bt_mesh_shell_target_ctx.dst, elem_addr, mod_app_idx,
					       mod_id, &status);
	}

	if (err) {
		shell_error(sh, "Unable to send Model App Bind (err %d)", err);
		return 0;
	}

	if (status) {
		shell_print(sh, "Model App Bind failed with status 0x%02x", status);
	} else {
		shell_print(sh, "AppKey successfully bound");
	}

	return 0;
}

static int cmd_mod_app_unbind(const struct shell *sh, size_t argc, char *argv[])
{
	uint16_t elem_addr, mod_app_idx, mod_id, cid;
	uint8_t status;
	int err = 0;

	elem_addr = shell_strtoul(argv[1], 0, &err);
	mod_app_idx = shell_strtoul(argv[2], 0, &err);
	mod_id = shell_strtoul(argv[3], 0, &err);
	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	if (argc > 4) {
		cid = shell_strtoul(argv[4], 0, &err);
		if (err) {
			shell_warn(sh, "Unable to parse input string argument");
			return err;
		}

		err = bt_mesh_cfg_cli_mod_app_unbind_vnd(bt_mesh_shell_target_ctx.net_idx,
						     bt_mesh_shell_target_ctx.dst, elem_addr,
						     mod_app_idx, mod_id, cid, &status);
	} else {
		err = bt_mesh_cfg_cli_mod_app_unbind(bt_mesh_shell_target_ctx.net_idx,
						 bt_mesh_shell_target_ctx.dst, elem_addr,
						 mod_app_idx, mod_id, &status);
	}

	if (err) {
		shell_error(sh, "Unable to send Model App Unbind (err %d)", err);
		return 0;
	}

	if (status) {
		shell_print(sh, "Model App Unbind failed with status 0x%02x", status);
	} else {
		shell_print(sh, "AppKey successfully unbound");
	}

	return 0;
}

static int cmd_mod_app_get(const struct shell *sh, size_t argc, char *argv[])
{
	uint16_t elem_addr, mod_id, cid;
	uint16_t apps[16];
	uint8_t status;
	size_t cnt;
	int err = 0;
	int i;

	cnt = ARRAY_SIZE(apps);
	elem_addr = shell_strtoul(argv[1], 0, &err);
	mod_id = shell_strtoul(argv[2], 0, &err);
	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	if (argc > 3) {
		cid = shell_strtoul(argv[3], 0, &err);
		if (err) {
			shell_warn(sh, "Unable to parse input string argument");
			return err;
		}

		err = bt_mesh_cfg_cli_mod_app_get_vnd(bt_mesh_shell_target_ctx.net_idx,
						  bt_mesh_shell_target_ctx.dst, elem_addr, mod_id,
						  cid, &status, apps, &cnt);
	} else {
		err = bt_mesh_cfg_cli_mod_app_get(bt_mesh_shell_target_ctx.net_idx,
					      bt_mesh_shell_target_ctx.dst, elem_addr, mod_id,
					      &status, apps, &cnt);
	}

	if (err) {
		shell_error(sh, "Unable to send Model App Get (err %d)", err);
		return 0;
	}

	if (status) {
		shell_print(sh, "Model App Get failed with status 0x%02x", status);
	} else {
		shell_print(sh, "Apps bound to Element 0x%04x, Model 0x%04x %s:", elem_addr, mod_id,
			    argc > 3 ? argv[3] : "(SIG)");

		if (!cnt) {
			shell_print(sh, "\tNone.");
		}

		for (i = 0; i < cnt; i++) {
			shell_print(sh, "\t0x%04x", apps[i]);
		}
	}

	return 0;
}

static int cmd_mod_sub_add(const struct shell *sh, size_t argc, char *argv[])
{
	uint16_t elem_addr, sub_addr, mod_id, cid;
	uint8_t status;
	int err = 0;

	elem_addr = shell_strtoul(argv[1], 0, &err);
	sub_addr = shell_strtoul(argv[2], 0, &err);
	mod_id = shell_strtoul(argv[3], 0, &err);
	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	if (argc > 4) {
		cid = shell_strtoul(argv[4], 0, &err);
		if (err) {
			shell_warn(sh, "Unable to parse input string argument");
			return err;
		}

		err = bt_mesh_cfg_cli_mod_sub_add_vnd(bt_mesh_shell_target_ctx.net_idx,
						  bt_mesh_shell_target_ctx.dst, elem_addr, sub_addr,
						  mod_id, cid, &status);
	} else {
		err = bt_mesh_cfg_cli_mod_sub_add(bt_mesh_shell_target_ctx.net_idx,
					      bt_mesh_shell_target_ctx.dst, elem_addr, sub_addr,
					      mod_id, &status);
	}

	if (err) {
		shell_error(sh, "Unable to send Model Subscription Add (err %d)", err);
		return 0;
	}

	if (status) {
		shell_print(sh, "Model Subscription Add failed with status 0x%02x", status);
	} else {
		shell_print(sh, "Model subscription was successful");
	}

	return 0;
}

static int cmd_mod_sub_del(const struct shell *sh, size_t argc, char *argv[])
{
	uint16_t elem_addr, sub_addr, mod_id, cid;
	uint8_t status;
	int err = 0;

	elem_addr = shell_strtoul(argv[1], 0, &err);
	sub_addr = shell_strtoul(argv[2], 0, &err);
	mod_id = shell_strtoul(argv[3], 0, &err);
	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	if (argc > 4) {
		cid = shell_strtoul(argv[4], 0, &err);
		if (err) {
			shell_warn(sh, "Unable to parse input string argument");
			return err;
		}

		err = bt_mesh_cfg_cli_mod_sub_del_vnd(bt_mesh_shell_target_ctx.net_idx,
						  bt_mesh_shell_target_ctx.dst, elem_addr, sub_addr,
						  mod_id, cid, &status);
	} else {
		err = bt_mesh_cfg_cli_mod_sub_del(bt_mesh_shell_target_ctx.net_idx,
					      bt_mesh_shell_target_ctx.dst, elem_addr, sub_addr,
					      mod_id, &status);
	}

	if (err) {
		shell_error(sh, "Unable to send Model Subscription Delete (err %d)", err);
		return 0;
	}

	if (status) {
		shell_print(sh, "Model Subscription Delete failed with status 0x%02x", status);
	} else {
		shell_print(sh, "Model subscription deltion was successful");
	}

	return 0;
}

static int cmd_mod_sub_add_va(const struct shell *sh, size_t argc, char *argv[])
{
	uint16_t elem_addr, sub_addr, mod_id, cid;
	uint8_t label[16];
	uint8_t status;
	size_t len;
	int err = 0;

	elem_addr = shell_strtoul(argv[1], 0, &err);

	len = hex2bin(argv[2], strlen(argv[2]), label, sizeof(label));
	(void)memset(label + len, 0, sizeof(label) - len);

	mod_id = shell_strtoul(argv[3], 0, &err);
	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	if (argc > 4) {
		cid = shell_strtoul(argv[4], 0, &err);
		if (err) {
			shell_warn(sh, "Unable to parse input string argument");
			return err;
		}

		err = bt_mesh_cfg_cli_mod_sub_va_add_vnd(bt_mesh_shell_target_ctx.net_idx,
						     bt_mesh_shell_target_ctx.dst, elem_addr, label,
						     mod_id, cid, &sub_addr, &status);
	} else {
		err = bt_mesh_cfg_cli_mod_sub_va_add(bt_mesh_shell_target_ctx.net_idx,
						 bt_mesh_shell_target_ctx.dst, elem_addr, label,
						 mod_id, &sub_addr, &status);
	}

	if (err) {
		shell_error(sh, "Unable to send Mod Sub VA Add (err %d)", err);
		return 0;
	}

	if (status) {
		shell_print(sh, "Mod Sub VA Add failed with status 0x%02x", status);
	} else {
		shell_print(sh, "0x%04x subscribed to Label UUID %s (va 0x%04x)", elem_addr,
			    argv[2], sub_addr);
	}

	return 0;
}

static int cmd_mod_sub_del_va(const struct shell *sh, size_t argc, char *argv[])
{
	uint16_t elem_addr, sub_addr, mod_id, cid;
	uint8_t label[16];
	uint8_t status;
	size_t len;
	int err = 0;

	elem_addr = shell_strtoul(argv[1], 0, &err);

	len = hex2bin(argv[2], strlen(argv[2]), label, sizeof(label));
	(void)memset(label + len, 0, sizeof(label) - len);

	mod_id = shell_strtoul(argv[3], 0, &err);
	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	if (argc > 4) {
		cid = shell_strtoul(argv[4], 0, &err);
		if (err) {
			shell_warn(sh, "Unable to parse input string argument");
			return err;
		}

		err = bt_mesh_cfg_cli_mod_sub_va_del_vnd(bt_mesh_shell_target_ctx.net_idx,
						     bt_mesh_shell_target_ctx.dst, elem_addr, label,
						     mod_id, cid, &sub_addr, &status);
	} else {
		err = bt_mesh_cfg_cli_mod_sub_va_del(bt_mesh_shell_target_ctx.net_idx,
						 bt_mesh_shell_target_ctx.dst, elem_addr, label,
						 mod_id, &sub_addr, &status);
	}

	if (err) {
		shell_error(sh, "Unable to send Model Subscription Delete (err %d)", err);
		return 0;
	}

	if (status) {
		shell_print(sh, "Model Subscription Delete failed with status 0x%02x", status);
	} else {
		shell_print(sh, "0x%04x unsubscribed from Label UUID %s (va 0x%04x)", elem_addr,
			    argv[2], sub_addr);
	}

	return 0;
}

static int cmd_mod_sub_ow(const struct shell *sh, size_t argc, char *argv[])
{
	uint16_t elem_addr, sub_addr, mod_id, cid;
	uint8_t status;
	int err = 0;

	elem_addr = shell_strtoul(argv[1], 0, &err);
	sub_addr = shell_strtoul(argv[2], 0, &err);
	mod_id = shell_strtoul(argv[3], 0, &err);
	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	if (argc > 4) {
		cid = shell_strtoul(argv[4], 0, &err);
		if (err) {
			shell_warn(sh, "Unable to parse input string argument");
			return err;
		}

		err = bt_mesh_cfg_cli_mod_sub_overwrite_vnd(bt_mesh_shell_target_ctx.net_idx,
							bt_mesh_shell_target_ctx.dst, elem_addr,
							sub_addr, mod_id, cid, &status);
	} else {
		err = bt_mesh_cfg_cli_mod_sub_overwrite(bt_mesh_shell_target_ctx.net_idx,
						    bt_mesh_shell_target_ctx.dst, elem_addr,
						    sub_addr, mod_id, &status);
	}

	if (err) {
		shell_error(sh, "Unable to send Model Subscription Overwrite (err %d)", err);
		return 0;
	}

	if (status) {
		shell_print(sh, "Model Subscription Overwrite failed with status 0x%02x", status);
	} else {
		shell_print(sh, "Model subscription overwrite was successful");
	}

	return 0;
}

static int cmd_mod_sub_ow_va(const struct shell *sh, size_t argc, char *argv[])
{
	uint16_t elem_addr, sub_addr, mod_id, cid;
	uint8_t label[16];
	uint8_t status;
	size_t len;
	int err = 0;

	elem_addr = shell_strtoul(argv[1], 0, &err);

	len = hex2bin(argv[2], strlen(argv[2]), label, sizeof(label));
	(void)memset(label + len, 0, sizeof(label) - len);

	mod_id = shell_strtoul(argv[3], 0, &err);
	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	if (argc > 4) {
		cid = shell_strtoul(argv[4], 0, &err);
		if (err) {
			shell_warn(sh, "Unable to parse input string argument");
			return err;
		}

		err = bt_mesh_cfg_cli_mod_sub_va_overwrite_vnd(bt_mesh_shell_target_ctx.net_idx,
							   bt_mesh_shell_target_ctx.dst, elem_addr,
							   label, mod_id, cid, &sub_addr, &status);
	} else {
		err = bt_mesh_cfg_cli_mod_sub_va_overwrite(bt_mesh_shell_target_ctx.net_idx,
						       bt_mesh_shell_target_ctx.dst, elem_addr,
						       label, mod_id, &sub_addr, &status);
	}

	if (err) {
		shell_error(sh, "Unable to send Mod Sub VA Overwrite (err %d)", err);
		return 0;
	}

	if (status) {
		shell_print(sh, "Mod Sub VA Overwrite failed with status 0x%02x", status);
	} else {
		shell_print(sh, "0x%04x overwrite to Label UUID %s (va 0x%04x)", elem_addr, argv[2],
			    sub_addr);
	}

	return 0;
}

static int cmd_mod_sub_del_all(const struct shell *sh, size_t argc, char *argv[])
{
	uint16_t elem_addr, mod_id, cid;
	uint8_t status;
	int err = 0;

	elem_addr = shell_strtoul(argv[1], 0, &err);
	mod_id = shell_strtoul(argv[2], 0, &err);
	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	if (argc > 3) {
		cid = shell_strtoul(argv[3], 0, &err);
		if (err) {
			shell_warn(sh, "Unable to parse input string argument");
			return err;
		}

		err = bt_mesh_cfg_cli_mod_sub_del_all_vnd(bt_mesh_shell_target_ctx.net_idx,
						      bt_mesh_shell_target_ctx.dst, elem_addr,
						      mod_id, cid, &status);
	} else {
		err = bt_mesh_cfg_cli_mod_sub_del_all(bt_mesh_shell_target_ctx.net_idx,
						  bt_mesh_shell_target_ctx.dst, elem_addr, mod_id,
						  &status);
	}

	if (err) {
		shell_error(sh, "Unable to send Model Subscription Delete All (err %d)", err);
		return 0;
	}

	if (status) {
		shell_print(sh, "Model Subscription Delete All failed with status 0x%02x", status);
	} else {
		shell_print(sh, "Model subscription deltion all was successful");
	}

	return 0;
}

static int cmd_mod_sub_get(const struct shell *sh, size_t argc, char *argv[])
{
	uint16_t elem_addr, mod_id, cid;
	uint16_t subs[16];
	uint8_t status;
	size_t cnt;
	int err = 0;
	int i;

	cnt = ARRAY_SIZE(subs);
	elem_addr = shell_strtoul(argv[1], 0, &err);
	mod_id = shell_strtoul(argv[2], 0, &err);
	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	if (argc > 3) {
		cid = shell_strtoul(argv[3], 0, &err);
		if (err) {
			shell_warn(sh, "Unable to parse input string argument");
			return err;
		}

		err = bt_mesh_cfg_cli_mod_sub_get_vnd(bt_mesh_shell_target_ctx.net_idx,
						  bt_mesh_shell_target_ctx.dst, elem_addr, mod_id,
						  cid, &status, subs, &cnt);
	} else {
		err = bt_mesh_cfg_cli_mod_sub_get(bt_mesh_shell_target_ctx.net_idx,
					      bt_mesh_shell_target_ctx.dst, elem_addr, mod_id,
					      &status, subs, &cnt);
	}

	if (err) {
		shell_error(sh, "Unable to send Model Subscription Get (err %d)", err);
		return 0;
	}

	if (status) {
		shell_print(sh, "Model Subscription Get failed with status 0x%02x", status);
	} else {
		shell_print(sh,
			    "Model Subscriptions for Element 0x%04x, Model 0x%04x %s:", elem_addr,
			    mod_id, argc > 3 ? argv[3] : "(SIG)");

		if (!cnt) {
			shell_print(sh, "\tNone.");
		}

		for (i = 0; i < cnt; i++) {
			shell_print(sh, "\t0x%04x", subs[i]);
		}
	}

	return 0;
}

static int cmd_krp(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t status, phase;
	uint16_t key_net_idx;
	int err = 0;

	key_net_idx = shell_strtoul(argv[1], 0, &err);
	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	if (argc < 3) {
		err = bt_mesh_cfg_cli_krp_get(bt_mesh_shell_target_ctx.net_idx,
					  bt_mesh_shell_target_ctx.dst, key_net_idx, &status,
					  &phase);
	} else {
		uint16_t trans = shell_strtoul(argv[2], 0, &err);

		if (err) {
			shell_warn(sh, "Unable to parse input string argument");
			return err;
		}

		err = bt_mesh_cfg_cli_krp_set(bt_mesh_shell_target_ctx.net_idx,
					  bt_mesh_shell_target_ctx.dst, key_net_idx, trans, &status,
					  &phase);
	}

	if (err) {
		shell_error(sh, "Unable to send key refresh phase Get/Set (err %d)", err);
		return 0;
	}

	shell_print(sh, "Key refresh phase Get/Set with status 0x%02x and phase 0x%02x", status,
		    phase);

	return 0;
}

static int mod_pub_get(const struct shell *sh, uint16_t addr, uint16_t mod_id, uint16_t cid)
{
	struct bt_mesh_cfg_cli_mod_pub pub;
	uint8_t status;
	int err;

	if (cid == CID_NVAL) {
		err = bt_mesh_cfg_cli_mod_pub_get(bt_mesh_shell_target_ctx.net_idx,
					      bt_mesh_shell_target_ctx.dst, addr, mod_id, &pub,
					      &status);
	} else {
		err = bt_mesh_cfg_cli_mod_pub_get_vnd(bt_mesh_shell_target_ctx.net_idx,
						  bt_mesh_shell_target_ctx.dst, addr, mod_id, cid,
						  &pub, &status);
	}

	if (err) {
		shell_error(sh, "Model Publication Get failed (err %d)", err);
		return 0;
	}

	if (status) {
		shell_print(sh, "Model Publication Get failed (status 0x%02x)", status);
		return 0;
	}

	shell_print(sh,
		    "Model Publication for Element 0x%04x, Model 0x%04x:\n"
		    "\tPublish Address:                0x%04x\n"
		    "\tAppKeyIndex:                    0x%04x\n"
		    "\tCredential Flag:                %u\n"
		    "\tPublishTTL:                     %u\n"
		    "\tPublishPeriod:                  0x%02x\n"
		    "\tPublishRetransmitCount:         %u\n"
		    "\tPublishRetransmitInterval:      %ums",
		    addr, mod_id, pub.addr, pub.app_idx, pub.cred_flag, pub.ttl, pub.period,
		    BT_MESH_PUB_TRANSMIT_COUNT(pub.transmit),
		    BT_MESH_PUB_TRANSMIT_INT(pub.transmit));

	return 0;
}

static int mod_pub_set(const struct shell *sh, uint16_t addr, bool is_va, uint16_t mod_id,
		       uint16_t cid, char *argv[])
{
	struct bt_mesh_cfg_cli_mod_pub pub;
	uint8_t status, count;
	uint16_t interval;
	uint8_t uuid[16];
	uint8_t len;
	int err = 0;

	if (!is_va) {
		pub.addr = shell_strtoul(argv[0], 0, &err);
		pub.uuid = NULL;
	} else {
		len = hex2bin(argv[0], strlen(argv[0]), uuid, sizeof(uuid));
		memset(uuid + len, 0, sizeof(uuid) - len);
		pub.uuid = (const uint8_t *)&uuid;
	}

	pub.app_idx = shell_strtoul(argv[1], 0, &err);
	pub.cred_flag = shell_strtobool(argv[2], 0, &err);
	pub.ttl = shell_strtoul(argv[3], 0, &err);
	pub.period = shell_strtoul(argv[4], 0, &err);

	count = shell_strtoul(argv[5], 0, &err);
	if (count > 7) {
		shell_print(sh, "Invalid retransmit count");
		return -EINVAL;
	}

	interval = shell_strtoul(argv[6], 0, &err);
	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	if (interval > (31 * 50) || (interval % 50)) {
		shell_print(sh, "Invalid retransmit interval %u", interval);
		return -EINVAL;
	}

	pub.transmit = BT_MESH_PUB_TRANSMIT(count, interval);

	if (cid == CID_NVAL) {
		err = bt_mesh_cfg_cli_mod_pub_set(bt_mesh_shell_target_ctx.net_idx,
					      bt_mesh_shell_target_ctx.dst, addr, mod_id, &pub,
					      &status);
	} else {
		err = bt_mesh_cfg_cli_mod_pub_set_vnd(bt_mesh_shell_target_ctx.net_idx,
						  bt_mesh_shell_target_ctx.dst, addr, mod_id, cid,
						  &pub, &status);
	}

	if (err) {
		shell_error(sh, "Model Publication Set failed (err %d)", err);
		return 0;
	}

	if (status) {
		shell_print(sh, "Model Publication Set failed (status 0x%02x)", status);
	} else {
		shell_print(sh, "Model Publication successfully set");
	}

	return 0;
}

static int cmd_mod_pub(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;
	uint16_t addr, mod_id, cid;

	addr = shell_strtoul(argv[1], 0, &err);
	mod_id = shell_strtoul(argv[2], 0, &err);

	argc -= 3;
	argv += 3;

	if (argc == 1 || argc == 8) {
		cid = shell_strtoul(argv[0], 0, &err);
		argc--;
		argv++;
	} else {
		cid = CID_NVAL;
	}

	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	if (argc > 0) {
		if (argc < 7) {
			shell_warn(sh, "Invalid number of argument");
			return -EINVAL;
		}

		return mod_pub_set(sh, addr, false, mod_id, cid, argv);
	} else {
		return mod_pub_get(sh, addr, mod_id, cid);
	}
}

static int cmd_mod_pub_va(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;
	uint16_t addr, mod_id, cid = CID_NVAL;

	addr = shell_strtoul(argv[1], 0, &err);
	mod_id = shell_strtoul(argv[9], 0, &err);

	if (argc > 10) {
		cid = shell_strtoul(argv[10], 0, &err);
	}

	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	argv += 2;

	return mod_pub_set(sh, addr, true, mod_id, cid, argv);
}

static void hb_sub_print(const struct shell *sh, struct bt_mesh_cfg_cli_hb_sub *sub)
{
	shell_print(sh,
		    "Heartbeat Subscription:\n"
		    "\tSource:      0x%04x\n"
		    "\tDestination: 0x%04x\n"
		    "\tPeriodLog:   0x%02x\n"
		    "\tCountLog:    0x%02x\n"
		    "\tMinHops:     %u\n"
		    "\tMaxHops:     %u",
		    sub->src, sub->dst, sub->period, sub->count, sub->min, sub->max);
}

static int hb_sub_get(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_mesh_cfg_cli_hb_sub sub;
	uint8_t status;
	int err;

	err = bt_mesh_cfg_cli_hb_sub_get(bt_mesh_shell_target_ctx.net_idx,
					 bt_mesh_shell_target_ctx.dst, &sub, &status);
	if (err) {
		shell_error(sh, "Heartbeat Subscription Get failed (err %d)", err);
		return 0;
	}

	if (status) {
		shell_print(sh, "Heartbeat Subscription Get failed (status 0x%02x)", status);
	} else {
		hb_sub_print(sh, &sub);
	}

	return 0;
}

static int hb_sub_set(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_mesh_cfg_cli_hb_sub sub;
	uint8_t status;
	int err = 0;

	sub.src = shell_strtoul(argv[1], 0, &err);
	sub.dst = shell_strtoul(argv[2], 0, &err);
	sub.period = shell_strtoul(argv[3], 0, &err);
	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	err = bt_mesh_cfg_cli_hb_sub_set(bt_mesh_shell_target_ctx.net_idx,
					 bt_mesh_shell_target_ctx.dst, &sub, &status);
	if (err) {
		shell_error(sh, "Heartbeat Subscription Set failed (err %d)", err);
		return 0;
	}

	if (status) {
		shell_print(sh, "Heartbeat Subscription Set failed (status 0x%02x)", status);
	} else {
		hb_sub_print(sh, &sub);
	}

	return 0;
}

static int cmd_hb_sub(const struct shell *sh, size_t argc, char *argv[])
{
	if (argc > 1) {
		if (argc < 4) {
			shell_warn(sh, "Invalid number of argument");
			return -EINVAL;
		}

		return hb_sub_set(sh, argc, argv);
	} else {
		return hb_sub_get(sh, argc, argv);
	}
}

static int hb_pub_get(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_mesh_cfg_cli_hb_pub pub;
	uint8_t status;
	int err;

	err = bt_mesh_cfg_cli_hb_pub_get(bt_mesh_shell_target_ctx.net_idx,
					 bt_mesh_shell_target_ctx.dst, &pub, &status);
	if (err) {
		shell_error(sh, "Heartbeat Publication Get failed (err %d)", err);
		return 0;
	}

	if (status) {
		shell_print(sh, "Heartbeat Publication Get failed (status 0x%02x)", status);
		return 0;
	}

	shell_print(sh, "Heartbeat publication:");
	shell_print(sh, "\tdst 0x%04x count 0x%02x period 0x%02x", pub.dst, pub.count, pub.period);
	shell_print(sh, "\tttl 0x%02x feat 0x%04x net_idx 0x%04x", pub.ttl, pub.feat, pub.net_idx);

	return 0;
}

static int hb_pub_set(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_mesh_cfg_cli_hb_pub pub;
	uint8_t status;
	int err = 0;

	pub.dst = shell_strtoul(argv[1], 0, &err);
	pub.count = shell_strtoul(argv[2], 0, &err);
	pub.period = shell_strtoul(argv[3], 0, &err);
	pub.ttl = shell_strtoul(argv[4], 0, &err);
	pub.feat = shell_strtoul(argv[5], 0, &err);
	pub.net_idx = shell_strtoul(argv[6], 0, &err);
	if (err) {
		shell_warn(sh, "Unable to parse input string argument");
		return err;
	}

	err = bt_mesh_cfg_cli_hb_pub_set(bt_mesh_shell_target_ctx.net_idx,
					 bt_mesh_shell_target_ctx.dst, &pub, &status);
	if (err) {
		shell_error(sh, "Heartbeat Publication Set failed (err %d)", err);
		return 0;
	}

	if (status) {
		shell_print(sh, "Heartbeat Publication Set failed (status 0x%02x)", status);
	} else {
		shell_print(sh, "Heartbeat publication successfully set");
	}

	return 0;
}

static int cmd_hb_pub(const struct shell *sh, size_t argc, char *argv[])
{
	if (argc > 1) {
		if (argc < 7) {
			shell_warn(sh, "Invalid number of argument");
			return -EINVAL;
		}

		return hb_pub_set(sh, argc, argv);
	} else {
		return hb_pub_get(sh, argc, argv);
	}
}

SHELL_STATIC_SUBCMD_SET_CREATE(model_cmds,
	SHELL_CMD_ARG(app-bind, NULL, "<Addr> <AppIndex> <Model ID> [Company ID]",
		      cmd_mod_app_bind, 4, 1),
	SHELL_CMD_ARG(app-get, NULL, "<Elem addr> <Model ID> [Company ID]", cmd_mod_app_get,
		      3, 1),
	SHELL_CMD_ARG(app-unbind, NULL, "<Addr> <AppIndex> <Model ID> [Company ID]",
		      cmd_mod_app_unbind, 4, 1),
	SHELL_CMD_ARG(pub, NULL,
		      "<Addr> <Model ID> [Company ID] [<PubAddr> "
		      "<AppKeyIndex> <Cred: off, on> <TTL> <Period> <Count> <Interval>]",
		      cmd_mod_pub, 3, 1 + 7),
	SHELL_CMD_ARG(pub-va, NULL,
		      "<Addr> <UUID: 16 hex values> "
		      "<AppKeyIndex> <Cred: off, on> <TTL> <Period> <Count> <Interval> "
		      "<Model ID> [Company ID]",
		      cmd_mod_pub_va, 10, 1),
	SHELL_CMD_ARG(sub-add, NULL, "<Elem addr> <Sub addr> <Model ID> [Company ID]",
		      cmd_mod_sub_add, 4, 1),
	SHELL_CMD_ARG(sub-del, NULL, "<Elem addr> <Sub addr> <Model ID> [Company ID]",
		      cmd_mod_sub_del, 4, 1),
	SHELL_CMD_ARG(sub-add-va, NULL,
		      "<Elem addr> <Label UUID> <Model ID> [Company ID]", cmd_mod_sub_add_va, 4, 1),
	SHELL_CMD_ARG(sub-del-va, NULL,
		      "<Elem addr> <Label UUID> <Model ID> [Company ID]", cmd_mod_sub_del_va, 4, 1),
	SHELL_CMD_ARG(sub-ow, NULL, "<Elem addr> <Sub addr> <Model ID> [Company ID]",
		      cmd_mod_sub_ow, 4, 1),
	SHELL_CMD_ARG(sub-ow-va, NULL, "<Elem addr> <Label UUID> <Model ID> [Company ID]",
		      cmd_mod_sub_ow_va, 4, 1),
	SHELL_CMD_ARG(sub-del-all, NULL, "<Elem addr> <Model ID> [Company ID]",
		      cmd_mod_sub_del_all, 3, 1),
	SHELL_CMD_ARG(sub-get, NULL, "<Elem addr> <Model ID> [Company ID]", cmd_mod_sub_get,
		      3, 1),
	SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(netkey_cmds,
	SHELL_CMD_ARG(add, NULL, "<NetKeyIndex> [Val]", cmd_net_key_add, 2, 1),
	SHELL_CMD_ARG(upd, NULL, "<NetKeyIndex> [Val]", cmd_net_key_update, 2, 1),
	SHELL_CMD_ARG(get, NULL, NULL, cmd_net_key_get, 1, 0),
	SHELL_CMD_ARG(del, NULL, "<NetKeyIndex>", cmd_net_key_del, 2, 0),
	SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(appkey_cmds,
	SHELL_CMD_ARG(add, NULL, "<NetKeyIndex> <AppKeyIndex> [Val]", cmd_app_key_add,
		      3, 1),
	SHELL_CMD_ARG(upd, NULL, "<NetKeyIndex> <AppKeyIndex> [Val]", cmd_app_key_upd,
		      3, 1),
	SHELL_CMD_ARG(del, NULL, "<NetKeyIndex> <AppKeyIndex>", cmd_app_key_del, 3, 0),
	SHELL_CMD_ARG(get, NULL, "<NetKeyIndex>", cmd_app_key_get, 2, 0),
	SHELL_SUBCMD_SET_END);


SHELL_STATIC_SUBCMD_SET_CREATE(
	cfg_cli_cmds,
	/* Configuration Client Model operations */
	SHELL_CMD_ARG(reset, NULL, NULL, cmd_reset, 1, 0),
	SHELL_CMD_ARG(timeout, NULL, "[Timeout in seconds]", cmd_timeout, 1, 1),
	SHELL_CMD_ARG(get-comp, NULL, "[Page]", cmd_get_comp, 1, 1),
	SHELL_CMD_ARG(beacon, NULL, "[Val: off, on]", cmd_beacon, 1, 1),
	SHELL_CMD_ARG(ttl, NULL, "[TTL: 0x00, 0x02-0x7f]", cmd_ttl, 1, 1),
	SHELL_CMD_ARG(friend, NULL, "[Val: off, on]", cmd_friend, 1, 1),
	SHELL_CMD_ARG(gatt-proxy, NULL, "[Val: off, on]", cmd_gatt_proxy, 1, 1),
	SHELL_CMD_ARG(relay, NULL, "[<Val: off, on> [<Count: 0-7> [Interval: 10-320]]]", cmd_relay,
		      1, 3),
	SHELL_CMD_ARG(node-id, NULL, "<NetKeyIndex> [Identify]", cmd_node_id, 2, 1),
	SHELL_CMD_ARG(polltimeout-get, NULL, "<LPN Address>", cmd_polltimeout_get, 2, 0),
	SHELL_CMD_ARG(net-transmit-param, NULL,
		      "[<Count: 0-7>"
		      " <Interval: 10-320>]",
		      cmd_net_transmit, 1, 2),
	SHELL_CMD_ARG(krp, NULL, "<NetKeyIndex> [Phase]", cmd_krp, 2, 1),
	SHELL_CMD_ARG(hb-sub, NULL, "[<Src> <Dst> <Period>]", cmd_hb_sub, 1, 3),
	SHELL_CMD_ARG(hb-pub, NULL, "[<Dst> <Count> <Period> <TTL> <Features> <NetKeyIndex>]",
		      cmd_hb_pub, 1, 6),
	SHELL_CMD(appkey, &appkey_cmds, "Appkey config commands", bt_mesh_shell_mdl_cmds_help),
	SHELL_CMD(netkey, &netkey_cmds, "Netkey config commands", bt_mesh_shell_mdl_cmds_help),
	SHELL_CMD(model, &model_cmds, "Model config commands", bt_mesh_shell_mdl_cmds_help),
	SHELL_SUBCMD_SET_END);

SHELL_SUBCMD_ADD((mesh, models), cfg, &cfg_cli_cmds, "Config Cli commands",
		 bt_mesh_shell_mdl_cmds_help, 1, 1);
