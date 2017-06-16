/*  Bluetooth Mesh */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr.h>
#include <misc/byteorder.h>

#include <net/buf.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/mesh.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BLUETOOTH_MESH_DEBUG_FRIEND)
#include "common/log.h"

#include "crypto.h"
#include "adv.h"
#include "mesh.h"
#include "net.h"
#include "transport.h"
#include "access.h"
#include "friend.h"

static int send_friend_update(void)
{
	struct bt_mesh_msg_ctx ctx = {
		.net_idx     = bt_mesh.sub[0].net_idx,
		.app_idx     = BT_MESH_KEY_UNUSED,
		.addr        = bt_mesh.frnd.lpn,
		.send_ttl    = 0,
		.friend_cred = 1,
	};
	struct bt_mesh_net_tx tx = {
		.sub = &bt_mesh.sub[0],
		.ctx = &ctx,
		.src = bt_mesh_primary_addr(),
	};
	struct bt_mesh_ctl_friend_update upd = {
		.flags = 0,
		.iv_index = sys_cpu_to_be32(bt_mesh.iv_index),
		.md = !k_fifo_is_empty(&bt_mesh.frnd.queue),
	};

	BT_DBG("");

	return bt_mesh_ctl_send(&tx, TRANS_CTL_OP_FRIEND_UPDATE, &upd,
				sizeof(upd), NULL);
}

int bt_mesh_friend_poll(struct bt_mesh_net_rx *rx, struct net_buf_simple *buf)
{
	struct bt_mesh_ctl_friend_poll *msg = (void *)buf->data;
	struct bt_mesh_friend *frnd = &bt_mesh.frnd;

	if (buf->len < sizeof(*msg)) {
		BT_WARN("Too short Friend Update");
		return -EINVAL;
	}

	BT_DBG("msg->fsn 0x%02x frnd->fsn 0x%02x", msg->fsn, frnd->fsn);

	if (msg->fsn != frnd->fsn) {
		frnd->send_last = 1;
	}

	frnd->fsn++;
	frnd->send_update = 1;

	k_delayed_work_submit(&frnd->timer, frnd->recv_delay);

	return 0;
}

static int send_friend_offer(s8_t rssi)
{
	struct bt_mesh_msg_ctx ctx = {
		.net_idx     = bt_mesh.sub[0].net_idx,
		.app_idx     = BT_MESH_KEY_UNUSED,
		.addr        = bt_mesh.frnd.lpn,
		.send_ttl    = 0,
	};
	struct bt_mesh_net_tx tx = {
		.sub = &bt_mesh.sub[0],
		.ctx = &ctx,
		.src = bt_mesh_primary_addr(),
	};
	struct bt_mesh_ctl_friend_offer off = {
		.recv_win = CONFIG_BLUETOOTH_MESH_FRIEND_RECV_WIN,
		.queue_size = CONFIG_BLUETOOTH_MESH_FRIEND_QUEUE_SIZE,
		.rssi = rssi,
		.frnd_counter = bt_mesh.frnd.counter++,
	};

	BT_DBG("");

	return bt_mesh_ctl_send(&tx, TRANS_CTL_OP_FRIEND_OFFER, &off,
				sizeof(off), NULL);
}

int bt_mesh_friend_req(struct bt_mesh_net_rx *rx, struct net_buf_simple *buf)
{
	struct bt_mesh_ctl_friend_req *msg = (void *)buf->data;
	struct bt_mesh_friend *frnd = &bt_mesh.frnd;
	struct bt_mesh_subnet *sub = rx->sub;

	if (buf->len < sizeof(*msg)) {
		BT_WARN("Too short Friend Request");
		return -EINVAL;
	}

	frnd->lpn = rx->ctx.addr;
	frnd->rssi = rx->rssi;
	frnd->recv_delay = msg->recv_delay;
	frnd->poll_to = (((u32_t)msg->poll_to[0] << 16) |
			 ((u32_t)msg->poll_to[1] << 8) |
			 ((u32_t)msg->poll_to[2]));
	frnd->poll_to *= 100;
	frnd->lpn_counter = sys_be16_to_cpu(msg->lpn_counter);

	BT_DBG("LPN 0x%04x rssi %d recv_delay %u poll_to %ums",
	       frnd->lpn, frnd->rssi, frnd->recv_delay, frnd->poll_to);

	bt_mesh_friend_cred_add(sub->net_idx, sub->keys[0].net, 0,
				frnd->lpn, frnd->lpn_counter, frnd->counter);

	frnd->send_offer = 1;

	k_delayed_work_submit(&frnd->timer, K_MSEC(100));

	return 0;
}

static void friend_timeout(struct k_work *work)
{
	struct bt_mesh_friend *frnd = &bt_mesh.frnd;

	BT_DBG("send_offer %u send_update %u", frnd->send_offer,
	       frnd->send_update);

	if (frnd->send_offer) {
		frnd->send_offer = 0;
		send_friend_offer(frnd->rssi);
		return;
	}

	if (!frnd->send_update) {
		BT_WARN("Friendship lost");
		bt_mesh_friend_cred_del(bt_mesh.sub[0].net_idx, frnd->lpn);
		return;
	}

	frnd->send_update = 0;

	if (frnd->send_last && frnd->last) {
		frnd->send_last = 0;
		bt_mesh_adv_send(frnd->last, NULL);
		return;
	}

	if (frnd->last) {
		net_buf_unref(frnd->last);
	}

	frnd->last = net_buf_get(&frnd->queue, K_NO_WAIT);
	if (frnd->last) {
		bt_mesh_adv_send(frnd->last, NULL);
	} else {
		send_friend_update();
	}

	k_delayed_work_submit(&frnd->timer, frnd->poll_to);
}

int bt_mesh_friend_init(void)
{
	struct bt_mesh_friend *frnd = &bt_mesh.frnd;

	k_fifo_init(&frnd->queue);

	k_delayed_work_init(&frnd->timer, friend_timeout);

	return 0;
}

bool bt_mesh_friend_enqueue(struct net_buf *buf, u16_t dst)
{
	/* FIXME: Add support for multiple LPNs and group addresses */
	if (!bt_mesh_friend_dst_is_lpn(dst)) {
		return false;
	}

	if (BT_MESH_ADDR_IS_UNICAST(dst)) {
		net_buf_put(&bt_mesh.frnd.queue, net_buf_ref(buf));
	} else {
		struct net_buf *clone = net_buf_clone(buf, K_NO_WAIT);

		if (clone) {
			net_buf_put(&bt_mesh.frnd.queue, clone);
		} else {
			BT_WARN("Unable to allocate buffer for friend queue");
			return false;
		}
	}

	BT_DBG("Queued message for LPN");

	return true;
}
