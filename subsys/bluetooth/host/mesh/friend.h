/*  Bluetooth Mesh */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

static inline bool bt_mesh_friend_dst_is_lpn(u16_t dst)
{
#if defined(CONFIG_BT_MESH_FRIEND)
	return (dst == bt_mesh.frnd.lpn);
#else
	return false;
#endif
}

bool bt_mesh_friend_enqueue(struct net_buf *buf, u16_t dst);

int bt_mesh_friend_poll(struct bt_mesh_net_rx *rx, struct net_buf_simple *buf);
int bt_mesh_friend_req(struct bt_mesh_net_rx *rx, struct net_buf_simple *buf);

int bt_mesh_friend_init(void);
