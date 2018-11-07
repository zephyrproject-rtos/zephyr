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

#include <sensor.h>

#include "mesh.h"
#include "board.h"

#define MOD_LF            0x0000
#define OP_HELLO          0xbb
#define OP_HEARTBEAT      0xbc
#define OP_VND_HELLO      BT_MESH_MODEL_OP_3(OP_HELLO, BT_COMP_ID_LF)
#define OP_VND_HEARTBEAT  BT_MESH_MODEL_OP_3(OP_HEARTBEAT, BT_COMP_ID_LF)

#define DEFAULT_TTL       31
#define GROUP_ADDR        0xc123
#define NET_IDX           0x000
#define APP_IDX           0x000
#define FLAGS             0

/* Maximum characters in "hello" message */
#define HELLO_MAX         8

static struct k_work hello_work;
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

static void vnd_hello(struct bt_mesh_model *model,
		      struct bt_mesh_msg_ctx *ctx,
		      struct net_buf_simple *buf)
{
	char str[32];

	printk("Hello message from 0x%04x\n", ctx->addr);

	if (ctx->addr == bt_mesh_model_elem(model)->addr) {
		printk("Ignoring message from self\n");
		return;
	}

	strncpy(str, buf->data, HELLO_MAX);
	str[HELLO_MAX] = '\0';

	board_add_hello(ctx->addr, str);

	strcat(str, " says hi!");
	board_show_text(str, false, K_SECONDS(3));

	board_blink_leds();
}

static void vnd_heartbeat(struct bt_mesh_model *model,
			  struct bt_mesh_msg_ctx *ctx,
			  struct net_buf_simple *buf)
{
	u8_t init_ttl, hops;

	/* Ignore messages from self */
	if (ctx->addr == bt_mesh_model_elem(model)->addr) {
		return;
	}

	init_ttl = net_buf_simple_pull_u8(buf);
	hops = init_ttl - ctx->recv_ttl + 1;

	printk("Heartbeat from 0x%04x over %u hop%s\n", ctx->addr,
	       hops, hops == 1 ? "" : "s");

	board_add_heartbeat(ctx->addr, hops);
}

static const struct bt_mesh_model_op vnd_ops[] = {
	{ OP_VND_HELLO, 1, vnd_hello },
	{ OP_VND_HEARTBEAT, 1, vnd_heartbeat },
	BT_MESH_MODEL_OP_END,
};

static int pub_update(struct bt_mesh_model *mod)
{
	struct net_buf_simple *msg = mod->pub->msg;

	printk("Preparing to send heartbeat\n");

	bt_mesh_model_msg_init(msg, OP_VND_HEARTBEAT);
	net_buf_simple_add_u8(msg, DEFAULT_TTL);

	return 0;
}

BT_MESH_MODEL_PUB_DEFINE(vnd_pub, pub_update, 3 + 1);

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

static size_t first_name_len(const char *name)
{
	size_t len;

	for (len = 0; *name; name++, len++) {
		switch (*name) {
		case ' ':
		case ',':
		case '\n':
			return len;
		}
	}

	return len;
}

static void send_hello(struct k_work *work)
{
	NET_BUF_SIMPLE_DEFINE(msg, 3 + HELLO_MAX + 4);
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = NET_IDX,
		.app_idx = APP_IDX,
		.addr = GROUP_ADDR,
		.send_ttl = DEFAULT_TTL,
	};
	const char *name = bt_get_name();

	bt_mesh_model_msg_init(&msg, OP_VND_HELLO);
	net_buf_simple_add_mem(&msg, name, first_name_len(name));

	if (bt_mesh_model_send(&vnd_models[0], &ctx, &msg, NULL, NULL) == 0) {
		board_show_text("Saying \"hi!\" to everyone", false,
				K_SECONDS(2));
	} else {
		board_show_text("Sending Failed!", false, K_SECONDS(2));
	}
}

void mesh_send_hello(void)
{
	k_work_submit(&hello_work);
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
		.period = BT_MESH_PUB_PERIOD_SEC(10),
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

bool mesh_is_initialized(void)
{
	return elements[0].addr != BT_MESH_ADDR_UNASSIGNED;
}

u16_t mesh_get_addr(void)
{
	return elements[0].addr;
}

int mesh_init(void)
{
	static const u8_t dev_uuid[16] = { 0xc0, 0xff, 0xee };
	static const struct bt_mesh_prov prov = {
		.uuid = dev_uuid,
	};

	k_work_init(&hello_work, send_hello);
	k_work_init(&mesh_start_work, start_mesh);

	return bt_mesh_init(&prov, &comp);
}
