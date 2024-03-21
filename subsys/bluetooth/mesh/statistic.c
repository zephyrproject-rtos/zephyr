/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/mesh.h>

#include "net.h"
#include "statistic.h"

static struct bt_mesh_statistic stat;

void bt_mesh_stat_get(struct bt_mesh_statistic *st)
{
	memcpy(st, &stat, sizeof(struct bt_mesh_statistic));
}

void bt_mesh_stat_reset(void)
{
	memset(&stat, 0, sizeof(struct bt_mesh_statistic));
}

void bt_mesh_stat_planned_count(struct bt_mesh_adv_ctx *ctx)
{
	if (ctx->tag == BT_MESH_ADV_TAG_LOCAL) {
		stat.tx_local_planned++;
	} else if (ctx->tag == BT_MESH_ADV_TAG_RELAY) {
		stat.tx_adv_relay_planned++;
	} else if (ctx->tag == BT_MESH_ADV_TAG_FRIEND) {
		stat.tx_friend_planned++;
	}
}

void bt_mesh_stat_succeeded_count(struct bt_mesh_adv_ctx *ctx)
{
	if (ctx->tag == BT_MESH_ADV_TAG_LOCAL) {
		stat.tx_local_succeeded++;
	} else if (ctx->tag == BT_MESH_ADV_TAG_RELAY) {
		stat.tx_adv_relay_succeeded++;
	} else if (ctx->tag == BT_MESH_ADV_TAG_FRIEND) {
		stat.tx_friend_succeeded++;
	}
}

void bt_mesh_stat_rx(enum bt_mesh_net_if net_if)
{
	switch (net_if) {
	case BT_MESH_NET_IF_ADV:
		stat.rx_adv++;
		break;
	case BT_MESH_NET_IF_LOCAL:
		stat.rx_loopback++;
		break;
	case BT_MESH_NET_IF_PROXY:
	case BT_MESH_NET_IF_PROXY_CFG:
		stat.rx_proxy++;
		break;
	default:
		stat.rx_uknown++;
		break;
	}
}
