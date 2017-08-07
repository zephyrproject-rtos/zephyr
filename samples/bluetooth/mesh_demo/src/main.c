/* main.c - Application main entry point */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <misc/printk.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/mesh.h>

#include "board.h"

#define CID_INTEL 0x0002
#define MOD_INTEL 0x0000

#define GROUP_ADDR 0xc000
#define PROV_ADDR  0x000f

#define OP_VENDOR_BUTTON BT_MESH_MODEL_OP_3(0x00, CID_INTEL)

static const u8_t net_key[16] = {
	0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
	0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
};
static const u8_t dev_key[16] = {
	0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
	0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
};
static const u8_t app_key[16] = {
	0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
	0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
};
static const u16_t net_idx;
static const u16_t app_idx;
static const u32_t iv_index;
static u8_t flags;
#if defined(NODE_ADDR)
static u16_t addr = NODE_ADDR;
#else
static u16_t addr = 0x0b0c;
#endif
static u32_t seq;

#define PROVISIONER_ADDR 0x0001

static void heartbeat(u8_t hops, u16_t feat)
{
	board_heartbeat(hops, feat);
	board_play("100H");
}

static struct bt_mesh_cfg cfg_srv = {
#if defined(CONFIG_BOARD_BBC_MICROBIT)
	.relay = BT_MESH_RELAY_DISABLED,
	.beacon = BT_MESH_BEACON_DISABLED,
#else
	.relay = BT_MESH_RELAY_ENABLED,
	.beacon = BT_MESH_BEACON_ENABLED,
#endif
	.frnd = BT_MESH_FRIEND_NOT_SUPPORTED,
	.default_ttl = 7,

	/* 3 transmissions with 20ms interval */
	.net_transmit = BT_MESH_TRANSMIT(2, 20),
	.relay_retransmit = BT_MESH_TRANSMIT(3, 20),

	.hb_sub.func = heartbeat,
};

static void attention_on(struct bt_mesh_model *model)
{
	printk("attention_on()\n");
	board_attention(true);
	board_play("100H100C100H100C100H100C");
}

static void attention_off(struct bt_mesh_model *model)
{
	printk("attention_off()\n");
	board_attention(false);
}

static struct bt_mesh_health health_srv = {
	.attention.on = attention_on,
	.attention.off = attention_off,
};

static struct bt_mesh_model root_models[] = {
	BT_MESH_MODEL_CFG_SRV(&cfg_srv),
	BT_MESH_MODEL_HEALTH_SRV(&health_srv),
};

static void vnd_publish(struct bt_mesh_model *mod)
{
	printk("Vendor publish\n");
}

static struct bt_mesh_model_pub vnd_pub = {
	.func = vnd_publish,
};

static void vnd_button_pressed(struct bt_mesh_model *model,
			       struct bt_mesh_msg_ctx *ctx,
			       struct net_buf_simple *buf)
{
	printk("src 0x%04x\n", ctx->addr);

	if (ctx->addr == model->elem->addr) {
		return;
	}

	board_other_dev_pressed(ctx->addr);
	board_play("100G200 100G");
}

static const struct bt_mesh_model_op vnd_ops[] = {
	{ OP_VENDOR_BUTTON, 0, vnd_button_pressed },
	BT_MESH_MODEL_OP_END,
};

static struct bt_mesh_model vnd_models[] = {
	BT_MESH_MODEL_VND(CID_INTEL, MOD_INTEL, vnd_ops, &vnd_pub, NULL),
};

static struct bt_mesh_elem elements[] = {
	BT_MESH_ELEM(0, root_models, vnd_models),
};

static const struct bt_mesh_comp comp = {
	.cid = CID_INTEL,
	.elem = elements,
	.elem_count = ARRAY_SIZE(elements),
};

#define OP_APP_KEY_ADD        BT_MESH_MODEL_OP_1(0x00)
#define OP_MOD_SUB_ADD        BT_MESH_MODEL_OP_2(0x80, 0x1b)
#define OP_HEARTBEAT_SUB_SET  BT_MESH_MODEL_OP_2(0x80, 0x3b)
#define OP_MOD_APP_BIND       BT_MESH_MODEL_OP_2(0x80, 0x3d)

static inline void key_idx_pack(struct net_buf_simple *buf,
				u16_t idx1, u16_t idx2)
{
	net_buf_simple_add_le16(buf, idx1 | ((idx2 & 0x00f) << 12));
	net_buf_simple_add_u8(buf, idx2 >> 4);
}

static void configure(void)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(24);
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net_idx,
		.app_idx = BT_MESH_KEY_DEV,
		.addr = elements[0].addr,
	};

	printk("Configuring...\n");

	/* Add App Key */
	bt_mesh_model_msg_init(msg, OP_APP_KEY_ADD);

	key_idx_pack(msg, net_idx, app_idx);
	net_buf_simple_add_mem(msg, app_key, sizeof(app_key));

	if (bt_mesh_model_send(&root_models[0], &ctx, msg, NULL, NULL)) {
		printk("Unable to send App Key Add message\n");
		return;
	}

	/* Bind to vendor model */
	bt_mesh_model_msg_init(msg, OP_MOD_APP_BIND);
	net_buf_simple_add_le16(msg, elements[0].addr);
	net_buf_simple_add_le16(msg, app_idx);
	net_buf_simple_add_le16(msg, CID_INTEL);
	net_buf_simple_add_le16(msg, MOD_INTEL);

	if (bt_mesh_model_send(&root_models[0], &ctx, msg, NULL, NULL)) {
		printk("Unable to send Model App Key Bind message\n");
		return;
	}

	/* Bind to Health model */
	bt_mesh_model_msg_init(msg, OP_MOD_APP_BIND);
	net_buf_simple_add_le16(msg, elements[0].addr);
	net_buf_simple_add_le16(msg, app_idx);
	net_buf_simple_add_le16(msg, BT_MESH_MODEL_ID_HEALTH_SRV);

	if (bt_mesh_model_send(&root_models[0], &ctx, msg, NULL, NULL)) {
		printk("Unable to send Model App Key Bind message\n");
		return;
	}

	/* Bind to Health model */
	bt_mesh_model_msg_init(msg, OP_MOD_SUB_ADD);
	net_buf_simple_add_le16(msg, elements[0].addr);
	net_buf_simple_add_le16(msg, GROUP_ADDR);
	net_buf_simple_add_le16(msg, CID_INTEL);
	net_buf_simple_add_le16(msg, MOD_INTEL);

	if (bt_mesh_model_send(&root_models[0], &ctx, msg, NULL, NULL)) {
		printk("Unable to send Model Subscription Add message\n");
		return;
	}

	/* Heartbeat subscription */
	bt_mesh_model_msg_init(msg, OP_HEARTBEAT_SUB_SET);
	net_buf_simple_add_le16(msg, PROV_ADDR);
	net_buf_simple_add_le16(msg, GROUP_ADDR);
	net_buf_simple_add_u8(msg, 0x10);

	if (bt_mesh_model_send(&root_models[0], &ctx, msg, NULL, NULL)) {
		printk("Unable to send Model App Key Bind message\n");
		return;
	}

	printk("Configuration complete\n");

	board_play("100C100D100E100F100G100A100H");
}

static void bt_ready(int err)
{
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	err = bt_mesh_init(NULL, &comp);
	if (err) {
		printk("Initializing mesh failed (err %d)\n", err);
		return;
	}

	printk("Mesh initialized\n");

	err = bt_mesh_provision(net_key, net_idx, flags, iv_index, seq, addr,
				dev_key);
	if (err) {
		printk("Provisioning failed (err %d)\n", err);
		return;
	}

	printk("Provisioning completed\n");

	configure();
}

void board_button_1_pressed(void)
{
	struct net_buf_simple *msg = NET_BUF_SIMPLE(3 + 4);
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = net_idx,
		.app_idx = app_idx,
		.addr = GROUP_ADDR,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};

	/* Bind to Health model */
	bt_mesh_model_msg_init(msg, OP_VENDOR_BUTTON);

	if (bt_mesh_model_send(&vnd_models[0], &ctx, msg, NULL, NULL)) {
		printk("Unable to send Vendor Button message\n");
	}

	printk("Button message sent with OpCode 0x%08x\n", OP_VENDOR_BUTTON);
}

bool board_toggle_relay(void)
{
	if (cfg_srv.relay == BT_MESH_RELAY_ENABLED) {
		cfg_srv.relay = BT_MESH_RELAY_DISABLED;
		return false;
	}

	cfg_srv.relay = BT_MESH_RELAY_ENABLED;
	return true;
}

static struct k_sem tune_sem = _K_SEM_INITIALIZER(tune_sem, 0, 1);
static const char *tune_str;

void board_play(const char *str)
{
	tune_str = str;
	k_sem_give(&tune_sem);
}

void main(void)
{
	int err;

	printk("Initializing...\n");

	board_init(&addr, &seq);

	printk("Unicast address: 0x%04x, seq 0x%06x\n", addr, seq);

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
	}

	while (1) {
		k_sem_take(&tune_sem, K_FOREVER);
		board_play_tune(tune_str);
	}

}
