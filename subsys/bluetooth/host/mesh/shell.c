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
#include <shell/shell.h>
#include <misc/printk.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/mesh.h>

/* Private includes for raw Network & Transport layer access */
#include "mesh.h"
#include "net.h"
#include "transport.h"
#include "foundation.h"

#define CID_NVAL   0xffff
#define CID_LOCAL  0x0002

/* Default net, app & dev key values, unless otherwise specified */
static const u8_t default_key[16] = {
	0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
	0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
};

static struct {
	u16_t local;
	u16_t dst;
	u16_t net_idx;
	u16_t app_idx;
} net = {
	.local = BT_MESH_ADDR_UNASSIGNED,
	.dst = BT_MESH_ADDR_UNASSIGNED,
};

static struct bt_mesh_cfg_srv cfg_srv = {
	.relay = BT_MESH_RELAY_DISABLED,
	.beacon = BT_MESH_BEACON_DISABLED,
#if defined(CONFIG_BT_MESH_FRIEND)
	.frnd = BT_MESH_FRIEND_DISABLED,
#else
	.frnd = BT_MESH_FRIEND_NOT_SUPPORTED,
#endif
#if defined(CONFIG_BT_MESH_GATT_PROXY)
	.gatt_proxy = BT_MESH_GATT_PROXY_DISABLED,
#else
	.gatt_proxy = BT_MESH_GATT_PROXY_NOT_SUPPORTED,
#endif

	.default_ttl = 7,

	/* 3 transmissions with 20ms interval */
	.net_transmit = BT_MESH_TRANSMIT(2, 20),
	.relay_retransmit = BT_MESH_TRANSMIT(2, 20),
};

#define CUR_FAULTS_MAX 4

static u8_t cur_faults[CUR_FAULTS_MAX];
static u8_t reg_faults[CUR_FAULTS_MAX * 2];

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
	printk("Sending current faults\n");

	*test_id = 0x00;
	*company_id = CID_LOCAL;

	get_faults(cur_faults, sizeof(cur_faults), faults, fault_count);

	return 0;
}

static int fault_get_reg(struct bt_mesh_model *model, u16_t cid,
			 u8_t *test_id, u8_t *faults, u8_t *fault_count)
{
	if (cid != CID_LOCAL) {
		printk("Faults requested for unknown Company ID 0x%04x\n", cid);
		return -EINVAL;
	}

	printk("Sending registered faults\n");

	*test_id = 0x00;

	get_faults(reg_faults, sizeof(reg_faults), faults, fault_count);

	return 0;
}

static int fault_clear(struct bt_mesh_model *model, uint16_t cid)
{
	if (cid != CID_LOCAL) {
		return -EINVAL;
	}

	memset(reg_faults, 0, sizeof(reg_faults));

	return 0;
}

static int fault_test(struct bt_mesh_model *model, uint8_t test_id,
		      uint16_t cid)
{
	if (cid != CID_LOCAL) {
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

static struct bt_mesh_model_pub health_pub = {
	.msg = BT_MESH_HEALTH_FAULT_MSG(CUR_FAULTS_MAX),
};

static struct bt_mesh_cfg_cli cfg_cli = {
};

void show_faults(u8_t test_id, u16_t cid, u8_t *faults, size_t fault_count)
{
	size_t i;

	if (!fault_count) {
		printk("Health Test ID 0x%02x Company ID 0x%04x: no faults\n",
		       test_id, cid);
		return;
	}

	printk("Health Test ID 0x%02x Company ID 0x%04x Fault Count %zu:\n",
	       test_id, cid, fault_count);

	for (i = 0; i < fault_count; i++) {
		printk("\t0x%02x\n", faults[i]);
	}
}

static void health_current_status(struct bt_mesh_health_cli *cli, u16_t addr,
				  u8_t test_id, u16_t cid, u8_t *faults,
				  size_t fault_count)
{
	printk("Health Current Status from 0x%04x\n", addr);
	show_faults(test_id, cid, faults, fault_count);
}

static struct bt_mesh_health_cli health_cli = {
	.current_status = health_current_status,
};

static u8_t dev_uuid[16] = { 0xdd, 0xdd };

static struct bt_mesh_model root_models[] = {
	BT_MESH_MODEL_CFG_SRV(&cfg_srv),
	BT_MESH_MODEL_CFG_CLI(&cfg_cli),
	BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub),
	BT_MESH_MODEL_HEALTH_CLI(&health_cli),
};

static struct bt_mesh_elem elements[] = {
	BT_MESH_ELEM(0, root_models, BT_MESH_MODEL_NONE),
};

static const struct bt_mesh_comp comp = {
	.cid = CID_LOCAL,
	.elem = elements,
	.elem_count = ARRAY_SIZE(elements),
};

static u8_t hex2val(char c)
{
	if (c >= '0' && c <= '9') {
		return c - '0';
	} else if (c >= 'a' && c <= 'f') {
		return c - 'a' + 10;
	} else if (c >= 'A' && c <= 'F') {
		return c - 'A' + 10;
	} else {
		return 0;
	}
}

static size_t hex2bin(const char *hex, u8_t *bin, size_t bin_len)
{
	size_t len = 0;

	while (*hex && len < bin_len) {
		bin[len] = hex2val(*hex++) << 4;

		if (!*hex) {
			len++;
			break;
		}

		bin[len++] |= hex2val(*hex++);
	}

	return len;
}

static void prov_complete(u16_t net_idx, u16_t addr)
{
	printk("Local node provisioned, net_idx 0x%04x address 0x%04x\n",
	       net_idx, addr);
	net.net_idx = net_idx,
	net.local = addr;
	net.dst = addr;
}

static void prov_reset(void)
{
	printk("The local node has been reset and needs reprovisioning\n");
}

static int output_number(bt_mesh_output_action_t action, uint32_t number)
{
	printk("OOB Number: %u\n", number);
	return 0;
}

static int output_string(const char *str)
{
	printk("OOB String: %s\n", str);
	return 0;
}

static bt_mesh_input_action_t input_act;
static u8_t input_size;

static int cmd_input_num(int argc, char *argv[])
{
	int err;

	if (argc < 2) {
		return -EINVAL;
	}

	if (input_act != BT_MESH_ENTER_NUMBER) {
		printk("A number hasn't been requested!\n");
		return 0;
	}

	if (strlen(argv[1]) < input_size) {
		printk("Too short input (%u digits required)\n",
		       input_size);
		return 0;
	}

	err = bt_mesh_input_number(strtoul(argv[1], NULL, 10));
	if (err) {
		printk("Numeric input failed (err %d)\n", err);
		return 0;
	}

	input_act = BT_MESH_NO_INPUT;
	return 0;
}

static int cmd_input_str(int argc, char *argv[])
{
	int err;

	if (argc < 2) {
		return -EINVAL;
	}

	if (input_act != BT_MESH_ENTER_STRING) {
		printk("A string hasn't been requested!\n");
		return 0;
	}

	if (strlen(argv[1]) < input_size) {
		printk("Too short input (%u characters required)\n",
		       input_size);
		return 0;
	}

	err = bt_mesh_input_string(argv[1]);
	if (err) {
		printk("String input failed (err %d)\n", err);
		return 0;
	}

	input_act = BT_MESH_NO_INPUT;
	return 0;
}

static int input(bt_mesh_input_action_t act, u8_t size)
{
	switch (act) {
	case BT_MESH_ENTER_NUMBER:
		printk("Enter a number (max %u digits) with: input-num <num>\n",
		       size);
		break;
	case BT_MESH_ENTER_STRING:
		printk("Enter a string (max %u chars) with: input-str <str>\n",
		       size);
		break;
	default:
		printk("Unknown input action %u (size %u) requested!\n",
		       act, size);
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
	printk("Provisioning link opened on %s\n", bearer2str(bearer));
}

static void link_close(bt_mesh_prov_bearer_t bearer)
{
	printk("Provisioning link closed on %s\n", bearer2str(bearer));
}

static u8_t static_val[16];

static struct bt_mesh_prov prov = {
	.uuid = dev_uuid,
	.link_open = link_open,
	.link_close = link_close,
	.complete = prov_complete,
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
};

static int cmd_static_oob(int argc, char *argv[])
{
	if (argc < 2) {
		prov.static_val = NULL;
		prov.static_val_len = 0;
	} else {
		prov.static_val_len = hex2bin(argv[1], static_val, 16);
		if (prov.static_val_len) {
			prov.static_val = static_val;
		} else {
			prov.static_val = NULL;
		}
	}

	if (prov.static_val) {
		printk("Static OOB value set (length %u)\n",
		       prov.static_val_len);
	} else {
		printk("Static OOB value cleared\n");
	}

	return 0;
}

static int cmd_uuid(int argc, char *argv[])
{
	u8_t uuid[16];
	size_t len;

	if (argc < 2) {
		return -EINVAL;
	}

	len = hex2bin(argv[1], uuid, sizeof(uuid));
	if (len < 1) {
		return -EINVAL;
	}

	memcpy(dev_uuid, uuid, len);
	memset(dev_uuid + len, 0, sizeof(dev_uuid) - len);

	printk("Device UUID set\n");

	return 0;
}

static int cmd_reset(int argc, char *argv[])
{
	bt_mesh_reset();
	printk("Local node reset complete\n");
	return 0;
}

static u8_t str2u8(const char *str)
{
	if (isdigit(str[0])) {
		return strtoul(str, NULL, 0);
	}

	return (!strcmp(str, "on") || !strcmp(str, "enable"));
}

static bool str2bool(const char *str)
{
	return str2u8(str);
}

#if defined(CONFIG_BT_MESH_LOW_POWER)
static int cmd_lpn(int argc, char *argv[])
{
	static bool enabled;
	int err;

	if (argc < 2) {
		printk("%s\n", enabled ? "enabled" : "disabled");
		return 0;
	}

	if (str2bool(argv[1])) {
		if (enabled) {
			printk("LPN already enabled\n");
			return 0;
		}

		err = bt_mesh_lpn_set(true);
		if (err) {
			printk("Enabling LPN failed (err %d)\n", err);
		} else {
			enabled = true;
		}
	} else {
		if (!enabled) {
			printk("LPN already disabled\n");
			return 0;
		}

		err = bt_mesh_lpn_set(false);
		if (err) {
			printk("Enabling LPN failed (err %d)\n", err);
		} else {
			enabled = false;
		}
	}

	return 0;
}

static int cmd_poll(int argc, char *argv[])
{
	int err;

	err = bt_mesh_lpn_poll();
	if (err) {
		printk("Friend Poll failed (err %d)\n", err);
	}

	return 0;
}

static void lpn_cb(u16_t friend_addr, bool established)
{
	if (established) {
		printk("Friendship (as LPN) established to Friend 0x%04x\n",
		       friend_addr);
	} else {
		printk("Friendship (as LPN) lost with Friend 0x%04x\n",
		       friend_addr);
	}
}

#endif /* MESH_LOW_POWER */

static int cmd_init(int argc, char *argv[])
{
	int err;

	err = bt_enable(NULL);
	if (err && err != -EALREADY) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	} else if (!err) {
		printk("Bluetooth initialized\n");
	}

	err = bt_mesh_init(&prov, &comp);
	if (err) {
		printk("Mesh initialization failed (err %d)\n", err);
	}

	printk("Mesh initialized\n");
	printk("Use \"pb-adv on\" or \"pb-gatt on\" to enable advertising\n");

#if IS_ENABLED(CONFIG_BT_MESH_LOW_POWER)
	bt_mesh_lpn_set_cb(lpn_cb);
#endif

	return 0;
}

#if defined(CONFIG_BT_MESH_GATT_PROXY)
static int cmd_ident(int argc, char *argv[])
{
	int err;

	err = bt_mesh_proxy_identity_enable();
	if (err) {
		printk("Failed advertise using Node Identity (err %d)\n", err);
	}

	return 0;
}
#endif /* MESH_GATT_PROXY */

static int cmd_get_comp(int argc, char *argv[])
{
	struct net_buf_simple *comp = NET_BUF_SIMPLE(32);
	u8_t status, page = 0x00;
	int err;

	if (argc > 1) {
		page = strtol(argv[1], NULL, 0);
	}

	net_buf_simple_init(comp, 0);
	err = bt_mesh_cfg_comp_data_get(net.net_idx, net.dst, page,
					&status, comp);
	if (err) {
		printk("Getting composition failed (err %d)\n", err);
		return 0;
	}

	if (status != 0x00) {
		printk("Got non-success status 0x%02x\n", status);
		return 0;
	}

	printk("Got Composition Data for 0x%04x:\n", net.dst);
	printk("\tCID      0x%04x\n", net_buf_simple_pull_le16(comp));
	printk("\tPID      0x%04x\n", net_buf_simple_pull_le16(comp));
	printk("\tVID      0x%04x\n", net_buf_simple_pull_le16(comp));
	printk("\tCRPL     0x%04x\n", net_buf_simple_pull_le16(comp));
	printk("\tFeatures 0x%04x\n", net_buf_simple_pull_le16(comp));

	while (comp->len > 4) {
		u8_t sig, vnd;
		u16_t loc;
		int i;

		loc = net_buf_simple_pull_le16(comp);
		sig = net_buf_simple_pull_u8(comp);
		vnd = net_buf_simple_pull_u8(comp);

		printk("\n\tElement @ 0x%04x:\n", loc);

		if (comp->len < ((sig * 2) + (vnd * 4))) {
			printk("\t\t...truncated data!\n");
			break;
		}

		if (sig) {
			printk("\t\tSIG Models:\n");
		} else {
			printk("\t\tNo SIG Models\n");
		}

		for (i = 0; i < sig; i++) {
			u16_t mod_id = net_buf_simple_pull_le16(comp);

			printk("\t\t\t0x%04x\n", mod_id);
		}

		if (vnd) {
			printk("\t\tVendor Models:\n");
		} else {
			printk("\t\tNo Vendor Models\n");
		}

		for (i = 0; i < vnd; i++) {
			u16_t cid = net_buf_simple_pull_le16(comp);
			u16_t mod_id = net_buf_simple_pull_le16(comp);

			printk("\t\t\tCompany 0x%04x: 0x%04x\n", cid, mod_id);
		}
	}

	return 0;
}

static int cmd_dst(int argc, char *argv[])
{
	if (argc < 2) {
		printk("Destination address: 0x%04x%s\n", net.dst,
		       net.dst == net.local ? " (local)" : "");
		return 0;
	}

	if (!strcmp(argv[1], "local")) {
		net.dst = net.local;
	} else {
		net.dst = strtoul(argv[1], NULL, 0);
	}

	printk("Destination address set to 0x%04x%s\n", net.dst,
	       net.dst == net.local ? " (local)" : "");
	return 0;
}

static int cmd_netidx(int argc, char *argv[])
{
	if (argc < 2) {
		printk("NetIdx: 0x%04x\n", net.net_idx);
		return 0;
	}

	net.net_idx = strtoul(argv[1], NULL, 0);
	printk("NetIdx set to 0x%04x\n", net.net_idx);
	return 0;
}

static int cmd_appidx(int argc, char *argv[])
{
	if (argc < 2) {
		printk("AppIdx: 0x%04x\n", net.app_idx);
		return 0;
	}

	net.app_idx = strtoul(argv[1], NULL, 0);
	printk("AppIdx set to 0x%04x\n", net.app_idx);
	return 0;
}

static int cmd_net_send(int argc, char *argv[])
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(32);
	struct bt_mesh_msg_ctx ctx = {
		.send_ttl = BT_MESH_TTL_DEFAULT,
		.net_idx = net.net_idx,
		.addr = net.dst,
		.app_idx = net.app_idx,

	};
	struct bt_mesh_net_tx tx = {
		.ctx = &ctx,
		.src = net.local,
		.xmit = bt_mesh_net_transmit_get(),
		.sub = bt_mesh_subnet_get(net.net_idx),
	};
	size_t len;
	int err;

	if (argc < 2) {
		return -EINVAL;
	}

	if (!tx.sub) {
		printk("No matching subnet for NetKey Index 0x%04x\n",
		       net.net_idx);
		return 0;
	}

	net_buf_simple_init(msg, 0);
	len = hex2bin(argv[1], msg->data, net_buf_simple_tailroom(msg) - 4);
	net_buf_simple_add(msg, len);

	err = bt_mesh_trans_send(&tx, msg, NULL, NULL);
	if (err) {
		printk("Failed to send (err %d)\n", err);
	}

	return 0;
}

static int cmd_iv_update(int argc, char *argv[])
{
	if (bt_mesh_iv_update()) {
		printk("Transitioned to IV Update In Progress state\n");
	} else {
		printk("Transitioned to IV Update Normal state\n");
	}

	printk("IV Index is 0x%08x\n", bt_mesh.iv_index);

	return 0;
}

static int cmd_iv_update_test(int argc, char *argv[])
{
	bool enable;

	if (argc < 2) {
		return -EINVAL;
	}

	enable = str2bool(argv[1]);
	if (enable) {
		printk("Enabling IV Update test mode\n");
	} else {
		printk("Disabling IV Update test mode\n");
	}

	bt_mesh_iv_update_test(enable);

	return 0;
}

static int cmd_rpl_clear(int argc, char *argv[])
{
	bt_mesh_rpl_clear();
	return 0;
}

static int cmd_beacon(int argc, char *argv[])
{
	u8_t status;
	int err;

	if (argc < 2) {
		err = bt_mesh_cfg_beacon_get(net.net_idx, net.dst, &status);
	} else {
		u8_t val = str2u8(argv[1]);

		err = bt_mesh_cfg_beacon_set(net.net_idx, net.dst, val,
					     &status);
	}

	if (err) {
		printk("Unable to send Beacon Get/Set message (err %d)\n", err);
		return 0;
	}

	printk("Beacon state is 0x%02x\n", status);

	return 0;
}

static int cmd_ttl(int argc, char *argv[])
{
	u8_t ttl;
	int err;

	if (argc < 2) {
		err = bt_mesh_cfg_ttl_get(net.net_idx, net.dst, &ttl);
	} else {
		u8_t val = strtoul(argv[1], NULL, 0);

		err = bt_mesh_cfg_ttl_set(net.net_idx, net.dst, val, &ttl);
	}

	if (err) {
		printk("Unable to send Default TTL Get/Set (err %d)\n", err);
		return 0;
	}

	printk("Default TTL is 0x%02x\n", ttl);

	return 0;
}

static int cmd_friend(int argc, char *argv[])
{
	u8_t frnd;
	int err;

	if (argc < 2) {
		err = bt_mesh_cfg_friend_get(net.net_idx, net.dst, &frnd);
	} else {
		u8_t val = str2u8(argv[1]);

		err = bt_mesh_cfg_friend_set(net.net_idx, net.dst, val, &frnd);
	}

	if (err) {
		printk("Unable to send Friend Get/Set (err %d)\n", err);
		return 0;
	}

	printk("Friend is set to 0x%02x\n", frnd);

	return 0;
}

static int cmd_gatt_proxy(int argc, char *argv[])
{
	u8_t proxy;
	int err;

	if (argc < 2) {
		err = bt_mesh_cfg_gatt_proxy_get(net.net_idx, net.dst, &proxy);
	} else {
		u8_t val = str2u8(argv[1]);

		err = bt_mesh_cfg_gatt_proxy_set(net.net_idx, net.dst, val,
						 &proxy);
	}

	if (err) {
		printk("Unable to send GATT Proxy Get/Set (err %d)\n", err);
		return 0;
	}

	printk("GATT Proxy is set to 0x%02x\n", proxy);

	return 0;
}

static int cmd_relay(int argc, char *argv[])
{
	u8_t relay, transmit;
	int err;

	if (argc < 2) {
		err = bt_mesh_cfg_relay_get(net.net_idx, net.dst, &relay,
					    &transmit);
	} else {
		u8_t val = str2u8(argv[1]);
		u8_t count, interval, new_transmit;

		if (val) {
			if (argc > 2) {
				count = strtoul(argv[2], NULL, 0);
			} else {
				count = 2;
			}

			if (argc > 3) {
				interval = strtoul(argv[3], NULL, 0);
			} else {
				interval = 20;
			}

			new_transmit = BT_MESH_TRANSMIT(count, interval);
		} else {
			new_transmit = 0;
		}

		err = bt_mesh_cfg_relay_set(net.net_idx, net.dst, val,
					    new_transmit, &relay, &transmit);
	}

	if (err) {
		printk("Unable to send Relay Get/Set (err %d)\n", err);
		return 0;
	}

	printk("Relay is 0x%02x, Transmit 0x%02x (count %u interval %ums)\n",
	       relay, transmit, BT_MESH_TRANSMIT_COUNT(transmit),
	       BT_MESH_TRANSMIT_INT(transmit));

	return 0;
}

static int cmd_net_key_add(int argc, char *argv[])
{
	u8_t key_val[16];
	u16_t key_net_idx;
	u8_t status;
	int err;

	if (argc < 2) {
		return -EINVAL;
	}

	key_net_idx = strtoul(argv[1], NULL, 0);

	if (argc > 2) {
		size_t len;

		len = hex2bin(argv[3], key_val, sizeof(key_val));
		memset(key_val, 0, sizeof(key_val) - len);
	} else {
		memcpy(key_val, default_key, sizeof(key_val));
	}

	err = bt_mesh_cfg_net_key_add(net.net_idx, net.dst, key_net_idx,
				      key_val, &status);
	if (err) {
		printk("Unable to send NetKey Add (err %d)\n", err);
		return 0;
	}

	if (status) {
		printk("NetKeyAdd failed with status 0x%02x\n", status);
	} else {
		printk("NetKey added with NetKey Index 0x%03x\n", key_net_idx);
	}

	return 0;
}

static int cmd_app_key_add(int argc, char *argv[])
{
	u8_t key_val[16];
	u16_t key_net_idx, key_app_idx;
	u8_t status;
	int err;

	if (argc < 3) {
		return -EINVAL;
	}

	key_net_idx = strtoul(argv[1], NULL, 0);
	key_app_idx = strtoul(argv[2], NULL, 0);

	if (argc > 3) {
		size_t len;

		len = hex2bin(argv[3], key_val, sizeof(key_val));
		memset(key_val, 0, sizeof(key_val) - len);
	} else {
		memcpy(key_val, default_key, sizeof(key_val));
	}

	err = bt_mesh_cfg_app_key_add(net.net_idx, net.dst, key_net_idx,
				      key_app_idx, key_val, &status);
	if (err) {
		printk("Unable to send App Key Add (err %d)\n", err);
		return 0;
	}

	if (status) {
		printk("AppKeyAdd failed with status 0x%02x\n", status);
	} else {
		printk("AppKey added, NetKeyIndex 0x%04x AppKeyIndex 0x%04x\n",
		       key_net_idx, key_app_idx);
	}

	return 0;
}

static int cmd_mod_app_bind(int argc, char *argv[])
{
	u16_t elem_addr, mod_app_idx, mod_id, cid;
	u8_t status;
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
		printk("Unable to send Model App Bind (err %d)\n", err);
		return 0;
	}

	if (status) {
		printk("Model App Bind failed with status 0x%02x\n", status);
	} else {
		printk("AppKey successfully bound\n");
	}

	return 0;
}

static int cmd_mod_sub_add(int argc, char *argv[])
{
	u16_t elem_addr, sub_addr, mod_id, cid;
	u8_t status;
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
		printk("Unable to send Model Subscription Add (err %d)\n", err);
		return 0;
	}

	if (status) {
		printk("Model Subscription Add failed with status 0x%02x\n",
		       status);
	} else {
		printk("Model subscription was successful\n");
	}

	return 0;
}

static int cmd_mod_sub_del(int argc, char *argv[])
{
	u16_t elem_addr, sub_addr, mod_id, cid;
	u8_t status;
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
		printk("Unable to send Model Subscription Delete (err %d)\n",
		       err);
		return 0;
	}

	if (status) {
		printk("Model Subscription Delete failed with status 0x%02x\n",
		       status);
	} else {
		printk("Model subscription deltion was successful\n");
	}

	return 0;
}

static int cmd_mod_sub_add_va(int argc, char *argv[])
{
	u16_t elem_addr, sub_addr, mod_id, cid;
	u8_t label[16];
	u8_t status;
	size_t len;
	int err;

	if (argc < 4) {
		return -EINVAL;
	}

	elem_addr = strtoul(argv[1], NULL, 0);

	len = hex2bin(argv[2], label, sizeof(label));
	memset(label + len, 0, sizeof(label) - len);

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
		printk("Unable to send Mod Sub VA Add (err %d)\n", err);
		return 0;
	}

	if (status) {
		printk("Mod Sub VA Add failed with status 0x%02x\n",
		       status);
	} else {
		printk("0x%04x subscribed to Label UUID %s (va 0x%04x)\n",
		       elem_addr, argv[2], sub_addr);
	}

	return 0;
}

static int cmd_mod_sub_del_va(int argc, char *argv[])
{
	u16_t elem_addr, sub_addr, mod_id, cid;
	u8_t label[16];
	u8_t status;
	size_t len;
	int err;

	if (argc < 4) {
		return -EINVAL;
	}

	elem_addr = strtoul(argv[1], NULL, 0);

	len = hex2bin(argv[2], label, sizeof(label));
	memset(label + len, 0, sizeof(label) - len);

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
		printk("Unable to send Model Subscription Delete (err %d)\n",
		       err);
		return 0;
	}

	if (status) {
		printk("Model Subscription Delete failed with status 0x%02x\n",
		       status);
	} else {
		printk("0x%04x unsubscribed from Label UUID %s (va 0x%04x)\n",
		       elem_addr, argv[2], sub_addr);
	}

	return 0;
}

static int mod_pub_get(u16_t addr, u16_t mod_id, u16_t cid)
{
	struct bt_mesh_cfg_mod_pub pub;
	u8_t status;
	int err;

	if (cid == CID_NVAL) {
		err = bt_mesh_cfg_mod_pub_get(net.net_idx, net.dst, addr,
					      mod_id, &pub, &status);
	} else {
		err = bt_mesh_cfg_mod_pub_get_vnd(net.net_idx, net.dst, addr,
						  mod_id, cid, &pub, &status);
	}

	if (err) {
		printk("Model Publication Get failed (err %d)\n", err);
		return 0;
	}

	if (status) {
		printk("Model Publication Get failed (status 0x%02x)\n",
		       status);
		return 0;
	}

	printk("Model Publication for Element 0x%04x, Model 0x%04x:\n"
	       "\tPublish Address:                0x%04x\n"
	       "\tAppKeyIndex:                    0x%04x\n"
	       "\tCredential Flag:                %u\n"
	       "\tPublishTTL:                     %u\n"
	       "\tPublishPeriod:                  0x%02x\n"
	       "\tPublishRetransmitCount:         %u\n"
	       "\tPublishRetransmitInterval:      %ums\n",
	       addr, mod_id, pub.addr, pub.app_idx, pub.cred_flag, pub.ttl,
	       pub.period, BT_MESH_PUB_TRANSMIT_COUNT(pub.transmit),
	       BT_MESH_PUB_TRANSMIT_INT(pub.transmit));

	return 0;
}

static int mod_pub_set(u16_t addr, u16_t mod_id, u16_t cid, char *argv[])
{
	struct bt_mesh_cfg_mod_pub pub;
	u8_t status, count;
	u16_t interval;
	int err;

	pub.addr = strtoul(argv[0], NULL, 0);
	pub.app_idx = strtoul(argv[1], NULL, 0);
	pub.cred_flag = str2bool(argv[2]);
	pub.ttl = strtoul(argv[3], NULL, 0);
	pub.period = strtoul(argv[4], NULL, 0);

	count = strtoul(argv[5], NULL, 0);
	if (count > 7) {
		printk("Invalid retransmit count\n");
		return -EINVAL;
	}

	interval = strtoul(argv[6], NULL, 0);
	if (interval > (31 * 50) || (interval % 50)) {
		printk("Invalid retransmit interval %u\n", interval);
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
		printk("Model Publication Set failed (err %d)\n", err);
		return 0;
	}

	if (status) {
		printk("Model Publication Set failed (status 0x%02x)\n",
		       status);
	} else {
		printk("Model Publication successfully set\n");
	}

	return 0;
}

static int cmd_mod_pub(int argc, char *argv[])
{
	u16_t addr, mod_id, cid;

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

		return mod_pub_set(addr, mod_id, cid, argv);
	} else {
		return mod_pub_get(addr, mod_id, cid);
	}
}

static void hb_sub_print(struct bt_mesh_cfg_hb_sub *sub)
{
	printk("Heartbeat Subscription:\n"
	       "\tSource:      0x%04x\n"
	       "\tDestination: 0x%04x\n"
	       "\tPeriodLog:   0x%02x\n"
	       "\tCountLog:    0x%02x\n"
	       "\tMinHops:     %u\n"
	       "\tMaxHops:     %u\n",
	       sub->src, sub->dst, sub->period, sub->count,
	       sub->min, sub->max);
}

static int hb_sub_get(int argc, char *argv[])
{
	struct bt_mesh_cfg_hb_sub sub;
	u8_t status;
	int err;

	err = bt_mesh_cfg_hb_sub_get(net.net_idx, net.dst, &sub, &status);
	if (err) {
		printk("Heartbeat Subscription Get failed (err %d)\n", err);
		return 0;
	}

	if (status) {
		printk("Heartbeat Subscription Get failed (status 0x%02x)\n",
		       status);
	} else {
		hb_sub_print(&sub);
	}

	return 0;
}

static int hb_sub_set(int argc, char *argv[])
{
	struct bt_mesh_cfg_hb_sub sub;
	u8_t status;
	int err;

	sub.src = strtoul(argv[1], NULL, 0);
	sub.dst = strtoul(argv[2], NULL, 0);
	sub.period = strtoul(argv[3], NULL, 0);

	err = bt_mesh_cfg_hb_sub_set(net.net_idx, net.dst, &sub, &status);
	if (err) {
		printk("Heartbeat Subscription Set failed (err %d)\n", err);
		return 0;
	}

	if (status) {
		printk("Heartbeat Subscription Set failed (status 0x%02x)\n",
		       status);
	} else {
		hb_sub_print(&sub);
	}

	return 0;
}

static int cmd_hb_sub(int argc, char *argv[])
{
	if (argc > 1) {
		if (argc < 4) {
			return -EINVAL;
		}

		return hb_sub_set(argc, argv);
	} else {
		return hb_sub_get(argc, argv);
	}
}

static int hb_pub_get(int argc, char *argv[])
{
	struct bt_mesh_cfg_hb_pub pub;
	u8_t status;
	int err;

	err = bt_mesh_cfg_hb_pub_get(net.net_idx, net.dst, &pub, &status);
	if (err) {
		printk("Heartbeat Publication Get failed (err %d)\n", err);
		return 0;
	}

	if (status) {
		printk("Heartbeat Publication Get failed (status 0x%02x)\n",
		       status);
		return 0;
	}

	printk("Heartbeat publication:\n");
	printk("\tdst 0x%04x count 0x%02x period 0x%02x\n",
	       pub.dst, pub.count, pub.period);
	printk("\tttl 0x%02x feat 0x%04x net_idx 0x%04x\n",
	       pub.ttl, pub.feat, pub.net_idx);

	return 0;
}

static int hb_pub_set(int argc, char *argv[])
{
	struct bt_mesh_cfg_hb_pub pub;
	u8_t status;
	int err;

	pub.dst = strtoul(argv[1], NULL, 0);
	pub.count = strtoul(argv[2], NULL, 0);
	pub.period = strtoul(argv[3], NULL, 0);
	pub.ttl = strtoul(argv[4], NULL, 0);
	pub.feat = strtoul(argv[5], NULL, 0);
	pub.net_idx = strtoul(argv[5], NULL, 0);

	err = bt_mesh_cfg_hb_pub_set(net.net_idx, net.dst, &pub, &status);
	if (err) {
		printk("Heartbeat Publication Set failed (err %d)\n", err);
		return 0;
	}

	if (status) {
		printk("Heartbeat Publication Set failed (status 0x%02x)\n",
		       status);
	} else {
		printk("Heartbeat publication successfully set\n");
	}

	return 0;
}

static int cmd_hb_pub(int argc, char *argv[])
{
	if (argc > 1) {
		if (argc < 7) {
			return -EINVAL;
		}

		return hb_pub_set(argc, argv);
	} else {
		return hb_pub_get(argc, argv);
	}
}

#if defined(CONFIG_BT_MESH_PROV)
static int cmd_pb(bt_mesh_prov_bearer_t bearer, int argc, char *argv[])
{
	int err;

	if (argc < 2) {
		return -EINVAL;
	}

	if (str2bool(argv[1])) {
		err = bt_mesh_prov_enable(bearer);
		if (err) {
			printk("Failed to enable %s (err %d)\n",
			       bearer2str(bearer), err);
		} else {
			printk("%s enabled\n", bearer2str(bearer));
		}
	} else {
		err = bt_mesh_prov_disable(bearer);
		if (err) {
			printk("Failed to disable %s (err %d)\n",
			       bearer2str(bearer), err);
		} else {
			printk("%s disabled\n", bearer2str(bearer));
		}
	}

	return 0;
}
#endif

#if defined(CONFIG_BT_MESH_PB_ADV)
static int cmd_pb_adv(int argc, char *argv[])
{
	return cmd_pb(BT_MESH_PROV_ADV, argc, argv);
}
#endif /* CONFIG_BT_MESH_PB_ADV */

#if defined(CONFIG_BT_MESH_PB_GATT)
static int cmd_pb_gatt(int argc, char *argv[])
{
	return cmd_pb(BT_MESH_PROV_GATT, argc, argv);
}
#endif /* CONFIG_BT_MESH_PB_GATT */

static int cmd_provision(int argc, char *argv[])
{
	u16_t net_idx, addr;
	u32_t iv_index;
	int err;

	if (argc < 3) {
		return -EINVAL;
	}

	net_idx = strtoul(argv[1], NULL, 0);
	addr = strtoul(argv[2], NULL, 0);

	if (argc > 3) {
		iv_index = strtoul(argv[1], NULL, 0);
	} else {
		iv_index = 0;
	}

	err = bt_mesh_provision(default_key, net_idx, 0, iv_index, 0, addr,
				default_key);
	if (err) {
		printk("Provisioning failed (err %d)\n", err);
	}

	return 0;
}

int cmd_timeout(int argc, char *argv[])
{
	s32_t timeout;

	if (argc < 2) {
		timeout = bt_mesh_cfg_cli_timeout_get();
		if (timeout == K_FOREVER) {
			printk("Message timeout: forever\n");
		} else {
			printk("Message timeout: %u seconds\n",
			       timeout / 1000);
		}

		return 0;
	}

	timeout = strtol(argv[1], NULL, 0);
	if (timeout < 0 || timeout > (INT32_MAX / 1000)) {
		timeout = K_FOREVER;
	} else {
		timeout = timeout * 1000;
	}

	bt_mesh_cfg_cli_timeout_set(timeout);
	if (timeout == K_FOREVER) {
		printk("Message timeout: forever\n");
	} else {
		printk("Message timeout: %u seconds\n",
		       timeout / 1000);
	}

	return 0;
}

static int cmd_fault_get(int argc, char *argv[])
{
	u8_t faults[32];
	size_t fault_count;
	u8_t test_id;
	u16_t cid;
	int err;

	if (argc < 2) {
		return -EINVAL;
	}

	cid = strtoul(argv[1], NULL, 0);
	fault_count = sizeof(faults);

	err = bt_mesh_health_fault_get(net.net_idx, net.dst, net.app_idx, cid,
				       &test_id, faults, &fault_count);
	if (err) {
		printk("Failed to send Health Fault Get (err %d)\n", err);
	} else {
		show_faults(test_id, cid, faults, fault_count);
	}

	return 0;
}

static int cmd_fault_clear(int argc, char *argv[])
{
	u8_t faults[32];
	size_t fault_count;
	u8_t test_id;
	u16_t cid;
	int err;

	if (argc < 2) {
		return -EINVAL;
	}

	cid = strtoul(argv[1], NULL, 0);
	fault_count = sizeof(faults);

	err = bt_mesh_health_fault_clear(net.net_idx, net.dst, net.app_idx,
					 cid, &test_id, faults, &fault_count);
	if (err) {
		printk("Failed to send Health Fault Clear (err %d)\n", err);
	} else {
		show_faults(test_id, cid, faults, fault_count);
	}

	return 0;
}

static int cmd_fault_clear_unack(int argc, char *argv[])
{
	u16_t cid;
	int err;

	if (argc < 2) {
		return -EINVAL;
	}

	cid = strtoul(argv[1], NULL, 0);

	err = bt_mesh_health_fault_clear(net.net_idx, net.dst, net.app_idx,
					 cid, NULL, NULL, NULL);
	if (err) {
		printk("Health Fault Clear Unacknowledged failed (err %d)\n",
		       err);
	}

	return 0;
}

static int cmd_fault_test(int argc, char *argv[])
{
	u8_t faults[32];
	size_t fault_count;
	u8_t test_id;
	u16_t cid;
	int err;

	if (argc < 3) {
		return -EINVAL;
	}

	cid = strtoul(argv[1], NULL, 0);
	test_id = strtoul(argv[2], NULL, 0);
	fault_count = sizeof(faults);

	err = bt_mesh_health_fault_test(net.net_idx, net.dst, net.app_idx,
					cid, test_id, faults, &fault_count);
	if (err) {
		printk("Failed to send Health Fault Test (err %d)\n", err);
	} else {
		show_faults(test_id, cid, faults, fault_count);
	}

	return 0;
}

static int cmd_fault_test_unack(int argc, char *argv[])
{
	u16_t cid;
	u8_t test_id;
	int err;

	if (argc < 3) {
		return -EINVAL;
	}

	cid = strtoul(argv[1], NULL, 0);
	test_id = strtoul(argv[2], NULL, 0);

	err = bt_mesh_health_fault_test(net.net_idx, net.dst, net.app_idx,
					cid, test_id, NULL, NULL);
	if (err) {
		printk("Health Fault Test Unacknowledged failed (err %d)\n",
		       err);
	}

	return 0;
}

static int cmd_period_get(int argc, char *argv[])
{
	u8_t divisor;
	int err;

	err = bt_mesh_health_period_get(net.net_idx, net.dst, net.app_idx,
					&divisor);
	if (err) {
		printk("Failed to send Health Period Get (err %d)\n", err);
	} else {
		printk("Health FastPeriodDivisor: %u\n", divisor);
	}

	return 0;
}

static int cmd_period_set(int argc, char *argv[])
{
	u8_t divisor, updated_divisor;
	int err;

	if (argc < 2) {
		return -EINVAL;
	}

	divisor = strtoul(argv[1], NULL, 0);

	err = bt_mesh_health_period_set(net.net_idx, net.dst, net.app_idx,
					divisor, &updated_divisor);
	if (err) {
		printk("Failed to send Health Period Set (err %d)\n", err);
	} else {
		printk("Health FastPeriodDivisor: %u\n", updated_divisor);
	}

	return 0;
}

static int cmd_period_set_unack(int argc, char *argv[])
{
	u8_t divisor;
	int err;

	if (argc < 2) {
		return -EINVAL;
	}

	divisor = strtoul(argv[1], NULL, 0);

	err = bt_mesh_health_period_set(net.net_idx, net.dst, net.app_idx,
					divisor, NULL);
	if (err) {
		printk("Failed to send Health Period Set (err %d)\n", err);
	}

	return 0;
}

static int cmd_attention_get(int argc, char *argv[])
{
	u8_t attention;
	int err;

	err = bt_mesh_health_attention_get(net.net_idx, net.dst, net.app_idx,
					   &attention);
	if (err) {
		printk("Failed to send Health Attention Get (err %d)\n", err);
	} else {
		printk("Health Attention Timer: %u\n", attention);
	}

	return 0;
}

static int cmd_attention_set(int argc, char *argv[])
{
	u8_t attention, updated_attention;
	int err;

	if (argc < 2) {
		return -EINVAL;
	}

	attention = strtoul(argv[1], NULL, 0);

	err = bt_mesh_health_attention_set(net.net_idx, net.dst, net.app_idx,
					   attention, &updated_attention);
	if (err) {
		printk("Failed to send Health Attention Set (err %d)\n", err);
	} else {
		printk("Health Attention Timer: %u\n", updated_attention);
	}

	return 0;
}

static int cmd_attention_set_unack(int argc, char *argv[])
{
	u8_t attention;
	int err;

	if (argc < 2) {
		return -EINVAL;
	}

	attention = strtoul(argv[1], NULL, 0);

	err = bt_mesh_health_attention_set(net.net_idx, net.dst, net.app_idx,
					   attention, NULL);
	if (err) {
		printk("Failed to send Health Attention Set (err %d)\n", err);
	}

	return 0;
}

static int cmd_add_fault(int argc, char *argv[])
{
	u8_t fault_id;
	u8_t i;

	if (argc < 2) {
		return -EINVAL;
	}

	fault_id = strtoul(argv[1], NULL, 0);
	if (!fault_id) {
		printk("The Fault ID must be non-zero!\n");
		return -EINVAL;
	}

	for (i = 0; i < sizeof(cur_faults); i++) {
		if (!cur_faults[i]) {
			cur_faults[i] = fault_id;
			break;
		}
	}

	if (i == sizeof(cur_faults)) {
		printk("Fault array is full. Use \"del-fault\" to clear it\n");
		return 0;
	}

	for (i = 0; i < sizeof(reg_faults); i++) {
		if (!reg_faults[i]) {
			reg_faults[i] = fault_id;
			break;
		}
	}

	if (i == sizeof(reg_faults)) {
		printk("No space to store more registered faults\n");
	}

	bt_mesh_fault_update(&elements[0]);

	return 0;
}

static int cmd_del_fault(int argc, char *argv[])
{
	u8_t fault_id;
	u8_t i;

	if (argc < 2) {
		memset(cur_faults, 0, sizeof(cur_faults));
		printk("All current faults cleared\n");
		bt_mesh_fault_update(&elements[0]);
		return 0;
	}

	fault_id = strtoul(argv[1], NULL, 0);
	if (!fault_id) {
		printk("The Fault ID must be non-zero!\n");
		return -EINVAL;
	}

	for (i = 0; i < sizeof(cur_faults); i++) {
		if (cur_faults[i] == fault_id) {
			cur_faults[i] = 0;
			printk("Fault cleared\n");
		}
	}

	bt_mesh_fault_update(&elements[0]);

	return 0;
}

static const struct shell_cmd mesh_commands[] = {
	{ "init", cmd_init, NULL },
	{ "timeout", cmd_timeout, "[timeout in seconds]" },
#if defined(CONFIG_BT_MESH_PB_ADV)
	{ "pb-adv", cmd_pb_adv, "<val: off, on>" },
#endif
#if defined(CONFIG_BT_MESH_PB_GATT)
	{ "pb-gatt", cmd_pb_gatt, "<val: off, on>" },
#endif
	{ "reset", cmd_reset, NULL },
	{ "uuid", cmd_uuid, "<UUID: 1-16 hex values>" },
	{ "input-num", cmd_input_num, "<number>" },
	{ "input-str", cmd_input_str, "<string>" },
	{ "static-oob", cmd_static_oob, "[val: 1-16 hex values]" },

	{ "provision", cmd_provision, "<NetKeyIndex> <addr> [IVIndex]" },
#if defined(CONFIG_BT_MESH_LOW_POWER)
	{ "lpn", cmd_lpn, "<value: off, on>" },
	{ "poll", cmd_poll, NULL },
#endif
#if defined(CONFIG_BT_MESH_GATT_PROXY)
	{ "ident", cmd_ident, NULL },
#endif
	{ "dst", cmd_dst, "[destination address]" },
	{ "netidx", cmd_netidx, "[NetIdx]" },
	{ "appidx", cmd_appidx, "[AppIdx]" },

	/* Commands which access internal APIs, for testing only */
	{ "net-send", cmd_net_send, "<hex string>" },
	{ "iv-update", cmd_iv_update, NULL },
	{ "iv-update-test", cmd_iv_update_test, "<value: off, on>" },
	{ "rpl-clear", cmd_rpl_clear, NULL },

	/* Configuration Client Model operations */
	{ "get-comp", cmd_get_comp, "[page]" },
	{ "beacon", cmd_beacon, "[val: off, on]" },
	{ "ttl", cmd_ttl, "[ttl: 0x00, 0x02-0x7f]" },
	{ "friend", cmd_friend, "[val: off, on]" },
	{ "gatt-proxy", cmd_gatt_proxy, "[val: off, on]" },
	{ "relay", cmd_relay, "[val: off, on] [count: 0-7] [interval: 0-32]" },
	{ "net-key-add", cmd_net_key_add, "<NetKeyIndex> [val]" },
	{ "app-key-add", cmd_app_key_add, "<NetKeyIndex> <AppKeyIndex> [val]" },
	{ "mod-app-bind", cmd_mod_app_bind,
		"<addr> <AppIndex> <Model ID> [Company ID]" },
	{ "mod-pub", cmd_mod_pub, "<addr> <mod id> [cid] [<PubAddr> "
		"<AppKeyIndex> <cred> <ttl> <period> <count> <interval>]" },
	{ "mod-sub-add", cmd_mod_sub_add,
		"<elem addr> <sub addr> <Model ID> [Company ID]" },
	{ "mod-sub-del", cmd_mod_sub_del,
		"<elem addr> <sub addr> <Model ID> [Company ID]" },
	{ "mod-sub-add-va", cmd_mod_sub_add_va,
		"<elem addr> <Label UUID> <Model ID> [Company ID]" },
	{ "mod-sub-del-va", cmd_mod_sub_del_va,
		"<elem addr> <Label UUID> <Model ID> [Company ID]" },
	{ "hb-sub", cmd_hb_sub, "[<src> <dst> <period>]" },
	{ "hb-pub", cmd_hb_pub,
		"[<dst> <count> <period> <ttl> <features> <NetKeyIndex>]" },

	/* Health Client Model Operations */
	{ "fault-get", cmd_fault_get, "<Company ID>" },
	{ "fault-clear", cmd_fault_clear, "<Company ID>" },
	{ "fault-clear-unack", cmd_fault_clear_unack, "<Company ID>" },
	{ "fault-test", cmd_fault_test, "<Company ID> <Test ID>" },
	{ "fault-test-unack", cmd_fault_test_unack, "<Company ID> <Test ID>" },
	{ "period-get", cmd_period_get, NULL },
	{ "period-set", cmd_period_set, "<divisor>" },
	{ "period-set-unack", cmd_period_set_unack, "<divisor>" },
	{ "attention-get", cmd_attention_get, NULL },
	{ "attention-set", cmd_attention_set, "<timer>" },
	{ "attention-set-unack", cmd_attention_set_unack, "<timer>" },

	/* Health Server Model Operations */
	{ "add-fault", cmd_add_fault, "<Fault ID>" },
	{ "del-fault", cmd_del_fault, "[Fault ID]" },

	{ NULL, NULL, NULL}
};

SHELL_REGISTER("mesh", mesh_commands);
