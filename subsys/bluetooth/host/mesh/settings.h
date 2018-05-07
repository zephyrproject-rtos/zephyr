/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

void bt_mesh_store_net(u16_t primary_addr, const u8_t dev_key[16]);
void bt_mesh_store_iv(void);
void bt_mesh_store_seq(void);
void bt_mesh_store_rpl(struct bt_mesh_rpl *rpl);
void bt_mesh_store_subnet(struct bt_mesh_subnet *sub);
void bt_mesh_store_app_key(struct bt_mesh_app_key *key);

void bt_mesh_settings_init(void);
