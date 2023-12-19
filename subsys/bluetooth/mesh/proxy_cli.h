/*
 * Copyright (c) 2021 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */


void bt_mesh_proxy_cli_adv_recv(const struct bt_le_scan_recv_info *info,
				struct net_buf_simple *buf);

bool bt_mesh_proxy_cli_relay(struct bt_mesh_adv *adv);

bool bt_mesh_proxy_cli_is_connected(uint16_t net_idx);
