/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <string.h>
#include <misc/printk.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/mesh.h>
#include <bluetooth/hci.h>

#include "mesh.h"
#include "board.h"

#define MOD_LF         0x0000
#define OP_LF          0xbb
#define OP_VENDOR_MSG  BT_MESH_MODEL_OP_3(OP_LF, BT_COMP_ID_LF)

#define DEFAULT_TTL    7
#define GROUP_ADDR     0xc123
#define NET_IDX        0x000
#define APP_IDX        0x000
#define FLAGS          0

static struct k_work publish_work;
static struct k_work mesh_start_work;

static void heartbeat(u8_t hops, u16_t feat)
{
	board_show_text("Heartbeat Received", false, K_SECONDS(2));
}

static struct bt_mesh_cfg_srv cfg_srv = {
	.relay = BT_MESH_RELAY_ENABLED,
	.beacon = BT_MESH_BEACON_ENABLED,
	.default_ttl = DEFAULT_TTL,

	/* 3 transmissions with 20ms interval */
	.net_transmit = BT_MESH_TRANSMIT(2, 20),
	.relay_retransmit = BT_MESH_TRANSMIT(3, 20),

	.hb_sub.func = heartbeat,
};

static struct bt_mesh_cfg_cli cfg_cli = {
};

static void attention_on(struct bt_mesh_model *model)
{
	board_show_text("Attention!", false, K_SECONDS(2));
}

static void attention_off(struct bt_mesh_model *model)
{
	board_refresh_display();
}

static const struct bt_mesh_health_srv_cb health_srv_cb = {
	.attn_on = attention_on,
	.attn_off = attention_off,
};

static struct bt_mesh_health_srv health_srv = {
	.cb = &health_srv_cb,
};

BT_MESH_HEALTH_PUB_DEFINE(health_pub, 0);

static struct bt_mesh_model root_models[] = {
	BT_MESH_MODEL_CFG_SRV(&cfg_srv),
	BT_MESH_MODEL_CFG_CLI(&cfg_cli),
	BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub),
};

static void vnd_message(struct bt_mesh_model *model,
			struct bt_mesh_msg_ctx *ctx,
			struct net_buf_simple *buf)
{
	char str[32];
	size_t len;

	printk("Vendor message from 0x%04x\n", ctx->addr);

	if (ctx->addr == bt_mesh_model_elem(model)->addr) {
		printk("Ignoring message from self\n");
		return;
	}

	len = min(buf->len, 8);
	memcpy(str, buf->data, len);
	strcpy(str + len, " says hi!");

	board_show_text(str, false, K_SECONDS(3));

	board_blink_leds();
}

static const struct bt_mesh_model_op vnd_ops[] = {
	{ OP_VENDOR_MSG, 1, vnd_message },
	BT_MESH_MODEL_OP_END,
};

/* Limit the payload to 11 bytes so that it doesn't get segmented */
BT_MESH_MODEL_PUB_DEFINE(vnd_pub, NULL, 3 + 8);

static struct bt_mesh_model vnd_models[] = {
	BT_MESH_MODEL_VND(BT_COMP_ID_LF, MOD_LF, vnd_ops, &vnd_pub, NULL),
};

static struct bt_mesh_elem elements[] = {
	BT_MESH_ELEM(0, root_models, vnd_models),
};

static const struct bt_mesh_comp comp = {
	.cid = BT_COMP_ID_LF,
	.elem = elements,
	.elem_count = ARRAY_SIZE(elements),
};

static void publish_message(struct k_work *work)
{
	if (bt_mesh_model_publish(&vnd_models[0]) == 0) {
		board_show_text("Saying \"hi!\" to everyone", false,
				K_SECONDS(2));
	} else {
		board_show_text("Sending Failed!", false, K_SECONDS(2));
	}
}

void mesh_publish(void)
{
	k_work_submit(&publish_work);
}

static int provision_and_configure(void)
{
	static const u8_t net_key[16] = {
		0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc,
		0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc,
	};
	static const u8_t app_key[16] = {
		0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
		0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
	};
	static const u16_t iv_index;
	struct bt_mesh_cfg_mod_pub pub = {
		.addr = GROUP_ADDR,
		.app_idx = APP_IDX,
		.ttl = DEFAULT_TTL,
	};
	u8_t dev_key[16];
	u16_t addr;
	int err;

	err = bt_rand(dev_key, sizeof(dev_key));
	if (err) {
		return err;
	}

	do {
		err = bt_rand(&addr, sizeof(addr));
		if (err) {
			return err;
		}
	} while (!addr);

	/* Make sure it's a unicast address (highest bit unset) */
	addr &= ~0x8000;

	err = bt_mesh_provision(net_key, NET_IDX, FLAGS, iv_index, addr,
				dev_key);
	if (err) {
		return err;
	}

	printk("Configuring...\n");

	/* Add Application Key */
	bt_mesh_cfg_app_key_add(NET_IDX, addr, NET_IDX, APP_IDX, app_key, NULL);

	/* Bind to vendor model */
	bt_mesh_cfg_mod_app_bind_vnd(NET_IDX, addr, addr, APP_IDX,
				     MOD_LF, BT_COMP_ID_LF, NULL);

	/* Bind to Health model */
	bt_mesh_cfg_mod_app_bind(NET_IDX, addr, addr, APP_IDX,
				 BT_MESH_MODEL_ID_HEALTH_SRV, NULL);

	/* Add model subscription */
	bt_mesh_cfg_mod_sub_add_vnd(NET_IDX, addr, addr, GROUP_ADDR,
				    MOD_LF, BT_COMP_ID_LF, NULL);

	bt_mesh_cfg_mod_pub_set_vnd(NET_IDX, addr, addr, MOD_LF, BT_COMP_ID_LF,
				    &pub, NULL);

	printk("Configuration complete\n");

	return addr;
}

static void start_mesh(struct k_work *work)
{
	int err;

	err = provision_and_configure();
	if (err < 0) {
		board_show_text("Starting Mesh Failed", false,
				K_SECONDS(2));
	} else {
		char buf[32];

		snprintk(buf, sizeof(buf),
			 "Mesh Started\nAddr: 0x%04x", err);
		board_show_text(buf, false, K_SECONDS(4));
	}
}

void mesh_start(void)
{
	k_work_submit(&mesh_start_work);
}

void mesh_set_name(const char *name, size_t len)
{
	bt_mesh_model_msg_init(vnd_pub.msg, OP_VENDOR_MSG);
	len = min(len, net_buf_simple_tailroom(vnd_pub.msg));
	net_buf_simple_add_mem(vnd_pub.msg, name, len);
}

bool mesh_is_initialized(void)
{
	return elements[0].addr != BT_MESH_ADDR_UNASSIGNED;
}

int mesh_init(void)
{
	static const u8_t dev_uuid[16] = { 0xc0, 0xff, 0xee };
	static const struct bt_mesh_prov prov = {
		.uuid = dev_uuid,
	};

	k_work_init(&publish_work, publish_message);
	k_work_init(&mesh_start_work, start_mesh);

	return bt_mesh_init(&prov, &comp);
}
