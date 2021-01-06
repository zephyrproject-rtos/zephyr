/*  Bluetooth Mesh */

/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2020 Lingao Meng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

struct bt_mesh_rpl {
	uint16_t src;
	bool  old_iv;
#if defined(CONFIG_BT_SETTINGS)
	bool  store;
#endif
	uint32_t seq;
};

typedef void (*bt_mesh_rpl_func_t)(struct bt_mesh_rpl *rpl,
					void *user_data);

void bt_mesh_rpl_reset(void);
bool bt_mesh_rpl_check(struct bt_mesh_net_rx *rx,
			struct bt_mesh_rpl **match);
void bt_mesh_rpl_clear(void);
void bt_mesh_rpl_update(struct bt_mesh_rpl *rpl,
			struct bt_mesh_net_rx *rx);
void bt_mesh_rpl_pending_store(void);
