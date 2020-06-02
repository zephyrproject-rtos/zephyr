/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

void bt_mesh_beacon_enable(void);
void bt_mesh_beacon_disable(void);
void bt_mesh_beacon_cache_clear(struct bt_mesh_subnet *sub);
void bt_mesh_beacon_ivu_initiator(bool enable);

void bt_mesh_beacon_recv(struct net_buf_simple *buf);

int bt_mesh_beacon_create(struct bt_mesh_subnet *sub,
			  struct net_buf_simple *buf);

void bt_mesh_beacon_init(void);
void bt_mesh_beacon_update(struct bt_mesh_subnet *sub);
void bt_mesh_beacon_priv_random_get(uint8_t *random, size_t size);
