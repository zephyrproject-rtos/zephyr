/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

static inline uint16_t bt_mesh_hb_pwr2(uint8_t val)
{
	if (!val) {
		return 0x0000;
	} else if (val == 0xff) {
		return 0xffff;
	} else {
		return (1 << (val - 1));
	}
}

static inline uint8_t bt_mesh_hb_log(uint32_t val)
{
	if (!val) {
		return 0x00;
	} else {
		return 32 - __builtin_clz(val);
	}
}

void bt_mesh_hb_init(void);
void bt_mesh_hb_start(void);
void bt_mesh_hb_suspend(void);
void bt_mesh_hb_resume(void);

int bt_mesh_hb_recv(struct bt_mesh_net_rx *rx, struct net_buf_simple *buf);
void bt_mesh_hb_feature_changed(uint16_t features);

uint8_t bt_mesh_hb_pub_set(struct bt_mesh_hb_pub *hb_pub);
uint8_t bt_mesh_hb_sub_set(uint16_t src, uint16_t dst, uint32_t period);
void bt_mesh_hb_sub_reset_count(void);
void bt_mesh_hb_pub_pending_store(void);
