/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/net/buf.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/mesh.h>

#include "crypto.h"
#include "mesh.h"
#include "net.h"
#include "transport.h"
#include "heartbeat.h"
#include "access.h"
#include "beacon.h"
#include "foundation.h"
#include "lpn.h"

#define LOG_LEVEL CONFIG_BT_MESH_LOW_POWER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_mesh_lpn);

/**
 * Log modes other than the deferred may cause unintended delays during processing of log messages.
 * This in turns will affect scheduling of the receive delay and receive window.
 */
#if !defined(CONFIG_TEST) && !defined(CONFIG_ARCH_POSIX) && \
	defined(CONFIG_LOG) && !defined(CONFIG_LOG_MODE_DEFERRED) && \
	(LOG_LEVEL >= LOG_LEVEL_INF)
#warning Frienship feature may work unstable when non-deferred log mode is selected. Use the \
	 CONFIG_LOG_MODE_DEFERRED Kconfig option when Low Power node feature is enabled.
#endif

#if defined(CONFIG_BT_MESH_ADV_LEGACY)
#define RX_DELAY_CORRECTION(lpn) ((lpn)->adv_duration)
#else
#define RX_DELAY_CORRECTION(lpn) 0
#endif

#if defined(CONFIG_BT_MESH_LPN_AUTO)
#define LPN_AUTO_TIMEOUT (CONFIG_BT_MESH_LPN_AUTO_TIMEOUT * MSEC_PER_SEC)
#else
#define LPN_AUTO_TIMEOUT 0
#endif

#define LPN_RECV_DELAY            CONFIG_BT_MESH_LPN_RECV_DELAY
#define SCAN_LATENCY              MIN(CONFIG_BT_MESH_LPN_SCAN_LATENCY, \
				      LPN_RECV_DELAY)

#define FRIEND_REQ_RETRY_TIMEOUT  K_SECONDS(CONFIG_BT_MESH_LPN_RETRY_TIMEOUT)

#define FRIEND_REQ_WAIT           100
#define FRIEND_REQ_SCAN           (1 * MSEC_PER_SEC)
#define FRIEND_REQ_TIMEOUT        (FRIEND_REQ_WAIT + FRIEND_REQ_SCAN)

#define POLL_RETRY_TIMEOUT        100

#define REQ_RETRY_DURATION(lpn)   (LPN_RECV_DELAY + (lpn)->adv_duration + \
				      (lpn)->recv_win + POLL_RETRY_TIMEOUT)

#define POLL_TIMEOUT_INIT         (CONFIG_BT_MESH_LPN_INIT_POLL_TIMEOUT * 100)

#define POLL_TIMEOUT              (CONFIG_BT_MESH_LPN_POLL_TIMEOUT * 100)

#define REQ_ATTEMPTS_MAX          6
#define REQ_ATTEMPTS(lpn)         MIN(REQ_ATTEMPTS_MAX, \
			  POLL_TIMEOUT / REQ_RETRY_DURATION(lpn))

#define POLL_TIMEOUT_MAX(lpn)     (POLL_TIMEOUT - \
			  (REQ_ATTEMPTS(lpn) * REQ_RETRY_DURATION(lpn)))

#define CLEAR_ATTEMPTS            3

#define LPN_CRITERIA ((CONFIG_BT_MESH_LPN_MIN_QUEUE_SIZE) | \
		      (CONFIG_BT_MESH_LPN_RSSI_FACTOR << 3) | \
		      (CONFIG_BT_MESH_LPN_RECV_WIN_FACTOR << 5))

#define POLL_TO(to) { (uint8_t)((to) >> 16), (uint8_t)((to) >> 8), (uint8_t)(to) }
#define LPN_POLL_TO POLL_TO(CONFIG_BT_MESH_LPN_POLL_TIMEOUT)

/* 1 transmission, 20ms interval */
#define POLL_XMIT BT_MESH_TRANSMIT(0, 20)

#if defined(CONFIG_BT_MESH_LOW_POWER_LOG_LEVEL_DBG)
static const char *state2str(int state)
{
	switch (state) {
	case BT_MESH_LPN_DISABLED:
		return "disabled";
	case BT_MESH_LPN_CLEAR:
		return "clear";
	case BT_MESH_LPN_TIMER:
		return "timer";
	case BT_MESH_LPN_ENABLED:
		return "enabled";
	case BT_MESH_LPN_REQ_WAIT:
		return "req wait";
	case BT_MESH_LPN_WAIT_OFFER:
		return "wait offer";
	case BT_MESH_LPN_ESTABLISHED:
		return "established";
	case BT_MESH_LPN_RECV_DELAY:
		return "recv delay";
	case BT_MESH_LPN_WAIT_UPDATE:
		return "wait update";
	default:
		return "(unknown)";
	}
}
#endif /* CONFIG_BT_MESH_LOW_POWER_LOG_LEVEL_DBG */

static int32_t poll_timeout(struct bt_mesh_lpn *lpn)
{
	/* If we're waiting for segment acks keep polling at high freq */
	if (bt_mesh_tx_in_progress()) {
		LOG_DBG("Tx is in progress. Keep polling");
		return MIN(POLL_TIMEOUT_MAX(lpn), 1 * MSEC_PER_SEC);
	}

	if (lpn->poll_timeout < POLL_TIMEOUT_MAX(lpn)) {
		lpn->poll_timeout *= 2;
		lpn->poll_timeout =
			MIN(lpn->poll_timeout, POLL_TIMEOUT_MAX(lpn));
	}

	LOG_DBG("Poll Timeout is %ums", lpn->poll_timeout);

	return lpn->poll_timeout;
}

static inline void lpn_set_state(int state)
{
#if defined(CONFIG_BT_MESH_LOW_POWER_LOG_LEVEL_DBG)
	LOG_DBG("%s -> %s", state2str(bt_mesh.lpn.state), state2str(state));
#endif
	bt_mesh.lpn.state = state;
}

static inline void group_zero(atomic_t *target)
{
#if CONFIG_BT_MESH_LPN_GROUPS > 32
	int i;

	for (i = 0; i < ARRAY_SIZE(bt_mesh.lpn.added); i++) {
		atomic_set(&target[i], 0);
	}
#else
	atomic_set(target, 0);
#endif
}

static inline void group_set(atomic_t *target, atomic_t *source)
{
#if CONFIG_BT_MESH_LPN_GROUPS > 32
	int i;

	for (i = 0; i < ARRAY_SIZE(bt_mesh.lpn.added); i++) {
		(void)atomic_or(&target[i], atomic_get(&source[i]));
	}
#else
	(void)atomic_or(target, atomic_get(source));
#endif
}

static inline void group_clear(atomic_t *target, atomic_t *source)
{
#if CONFIG_BT_MESH_LPN_GROUPS > 32
	int i;

	for (i = 0; i < ARRAY_SIZE(bt_mesh.lpn.added); i++) {
		(void)atomic_and(&target[i], ~atomic_get(&source[i]));
	}
#else
	(void)atomic_and(target, ~atomic_get(source));
#endif
}

static void clear_friendship(bool force, bool disable);

static void friend_clear_sent(int err, void *user_data)
{
	struct bt_mesh_lpn *lpn = &bt_mesh.lpn;

	bt_mesh_scan_enable();

	lpn->req_attempts++;

	if (err) {
		LOG_ERR("Sending Friend Request failed (err %d)", err);
		lpn_set_state(BT_MESH_LPN_ENABLED);
		clear_friendship(false, lpn->disable);
		return;
	}

	lpn_set_state(BT_MESH_LPN_CLEAR);
	k_work_reschedule(&lpn->timer, K_MSEC(FRIEND_REQ_TIMEOUT));
}

static const struct bt_mesh_send_cb clear_sent_cb = {
	.end = friend_clear_sent,
};

static int send_friend_clear(void)
{
	struct bt_mesh_msg_ctx ctx = {
		.net_idx     = bt_mesh.lpn.sub->net_idx,
		.app_idx     = BT_MESH_KEY_UNUSED,
		.addr        = bt_mesh.lpn.frnd,
		.send_ttl    = 0,
	};
	struct bt_mesh_net_tx tx = {
		.sub = bt_mesh.lpn.sub,
		.ctx = &ctx,
		.src = bt_mesh_primary_addr(),
		.xmit = bt_mesh_net_transmit_get(),
	};
	struct bt_mesh_ctl_friend_clear req = {
		.lpn_addr    = sys_cpu_to_be16(tx.src),
		.lpn_counter = sys_cpu_to_be16(bt_mesh.lpn.lpn_counter),
	};

	LOG_DBG("");

	return bt_mesh_ctl_send(&tx, TRANS_CTL_OP_FRIEND_CLEAR, &req,
				sizeof(req), &clear_sent_cb, NULL);
}

static void clear_friendship(bool force, bool disable)
{
	struct bt_mesh_lpn *lpn = &bt_mesh.lpn;
	bool was_established = lpn->established;
	uint16_t frnd = lpn->frnd;
	uint16_t net_idx = lpn->sub->net_idx;

	LOG_DBG("force %u disable %u", force, disable);

	if (!force && lpn->established && !lpn->clear_success &&
	    lpn->req_attempts < CLEAR_ATTEMPTS) {
		send_friend_clear();
		lpn->disable = disable;
		return;
	}

	bt_mesh_rx_reset();

	/* Disable LPN while clearing, in case the work handler gets a chance to fire. */
	lpn_set_state(BT_MESH_LPN_DISABLED);
	/* The timer handler returns without any actions if this fails. */
	(void)k_work_cancel_delayable(&lpn->timer);

	if (IS_ENABLED(CONFIG_BT_MESH_LPN_ESTABLISHMENT) || disable) {
		bt_mesh_scan_disable();
	}

	if (lpn->clear_success) {
		lpn->old_friend = BT_MESH_ADDR_UNASSIGNED;
	} else {
		lpn->old_friend = lpn->frnd;
	}

	for (int i = 0; i < ARRAY_SIZE(lpn->cred); i++) {
		if (lpn->sub->keys[i].valid) {
			bt_mesh_friend_cred_destroy(&lpn->cred[i]);
		}
	}

	lpn->frnd = BT_MESH_ADDR_UNASSIGNED;
	lpn->fsn = 0U;
	lpn->req_attempts = 0U;
	lpn->recv_win = 0U;
	lpn->queue_size = 0U;
	lpn->disable = 0U;
	lpn->sent_req = 0U;
	lpn->established = 0U;
	lpn->clear_success = 0U;
	lpn->sub = NULL;

	group_zero(lpn->added);
	group_zero(lpn->pending);
	group_zero(lpn->to_remove);

	/* Set this to 1 to force group subscription when the next
	 * Friendship is created, in case lpn->groups doesn't get
	 * modified meanwhile.
	 */
	lpn->groups_changed = 1U;

	bt_mesh_hb_feature_changed(BT_MESH_FEAT_LOW_POWER);

	if (!disable) {
		lpn_set_state(BT_MESH_LPN_ENABLED);

		k_work_reschedule(&lpn->timer, FRIEND_REQ_RETRY_TIMEOUT);

		if (!IS_ENABLED(CONFIG_BT_MESH_LPN_ESTABLISHMENT)) {
			bt_mesh_scan_enable();
		}
	}

	if (was_established) {
		STRUCT_SECTION_FOREACH(bt_mesh_lpn_cb, cb) {
			if (cb->terminated) {
				cb->terminated(net_idx, frnd);
			}
		}
	}
}

static void friend_req_send_end(int err, void *user_data)
{
	struct bt_mesh_lpn *lpn = &bt_mesh.lpn;

	if (lpn->state != BT_MESH_LPN_ENABLED) {
		return;
	}

	if (err) {
		LOG_ERR("Sending Friend Request failed (err %d)", err);
		return;
	}

	lpn->adv_duration = k_uptime_get_32() - lpn->adv_start_time;

	if (IS_ENABLED(CONFIG_BT_MESH_LPN_ESTABLISHMENT)) {
		k_work_reschedule(&lpn->timer,
				  K_MSEC(FRIEND_REQ_WAIT - (int32_t)lpn->adv_duration));
		lpn_set_state(BT_MESH_LPN_REQ_WAIT);
	} else {
		k_work_reschedule(&lpn->timer, K_MSEC(FRIEND_REQ_TIMEOUT));
		lpn_set_state(BT_MESH_LPN_WAIT_OFFER);
	}
}

static void friend_req_send_start(uint16_t duration, int err, void *user_data)
{
	struct bt_mesh_lpn *lpn = &bt_mesh.lpn;

	lpn->adv_start_time = k_uptime_get_32();

	if (err) {
		friend_req_send_end(err, user_data);
	}
}

static const struct bt_mesh_send_cb friend_req_send_cb = {
	.start = friend_req_send_start,
	.end = friend_req_send_end,
};

static int send_friend_req(struct bt_mesh_lpn *lpn)
{
	const struct bt_mesh_comp *comp = bt_mesh_comp_get();
	struct bt_mesh_msg_ctx ctx = {
		.app_idx  = BT_MESH_KEY_UNUSED,
		.addr     = BT_MESH_ADDR_FRIENDS,
		.send_ttl = 0,
	};
	struct bt_mesh_net_tx tx = {
		.ctx = &ctx,
		.src = bt_mesh_primary_addr(),
		.xmit = POLL_XMIT,
	};

	lpn->lpn_counter++;

	struct bt_mesh_ctl_friend_req req = {
		.criteria    = LPN_CRITERIA,
		.recv_delay  = LPN_RECV_DELAY,
		.poll_to     = LPN_POLL_TO,
		.prev_addr   = sys_cpu_to_be16(lpn->old_friend),
		.num_elem    = comp->elem_count,
		.lpn_counter = sys_cpu_to_be16(lpn->lpn_counter),
	};

	LOG_DBG("");

	lpn->sub = bt_mesh_subnet_next(NULL);
	if (!lpn->sub) {
		LOG_ERR("No subnets, can't start LPN mode");
		return -ENOENT;
	}

	ctx.net_idx = lpn->sub->net_idx;
	tx.sub = lpn->sub;

	return bt_mesh_ctl_send(&tx, TRANS_CTL_OP_FRIEND_REQ, &req,
				sizeof(req), &friend_req_send_cb, NULL);
}

static void req_send_end(int err, void *user_data)
{
	struct bt_mesh_lpn *lpn = &bt_mesh.lpn;
	bool retry;

	if (lpn->state == BT_MESH_LPN_DISABLED) {
		return;
	}

	lpn->adv_duration = k_uptime_get_32() - lpn->adv_start_time;

#if defined(CONFIG_BT_MESH_LOW_POWER_LOG_LEVEL_DBG)
	LOG_DBG("req 0x%02x duration %u err %d state %s", lpn->sent_req, lpn->adv_duration, err,
		state2str(lpn->state));
#endif

	if (err) {
		LOG_ERR("Sending request failed (err %d)", err);
		lpn->sent_req = 0U;
		group_zero(lpn->pending);
		return;
	}

	retry = (lpn->req_attempts > 0);

	lpn->req_attempts++;

	if (lpn->established || IS_ENABLED(CONFIG_BT_MESH_LPN_ESTABLISHMENT)) {
		lpn_set_state(BT_MESH_LPN_RECV_DELAY);
		/* We start scanning a bit early to eliminate risk of missing
		 * response data due to HCI and other latencies.
		 */
		k_work_reschedule(&lpn->timer,
				  K_MSEC(LPN_RECV_DELAY - SCAN_LATENCY - RX_DELAY_CORRECTION(lpn)));
	} else {
		lpn_set_state(BT_MESH_LPN_WAIT_UPDATE);
		k_work_reschedule(&lpn->timer, K_MSEC(LPN_RECV_DELAY + lpn->recv_win));
	}

	STRUCT_SECTION_FOREACH(bt_mesh_lpn_cb, cb) {
		if (cb->polled) {
			cb->polled(lpn->sub->net_idx, lpn->frnd, retry);
		}
	}
}

static void req_send_start(uint16_t duration, int err, void *user_data)
{
	struct bt_mesh_lpn *lpn = &bt_mesh.lpn;

	lpn->adv_start_time = k_uptime_get_32();

	if (err) {
		req_send_end(err, user_data);
	}
}

static const struct bt_mesh_send_cb req_send_cb = {
	.start = req_send_start,
	.end = req_send_end,
};

static int send_friend_poll(void)
{
	struct bt_mesh_msg_ctx ctx = {
		.net_idx     = bt_mesh.lpn.sub->net_idx,
		.app_idx     = BT_MESH_KEY_UNUSED,
		.addr        = bt_mesh.lpn.frnd,
		.send_ttl    = 0,
	};
	struct bt_mesh_net_tx tx = {
		.sub = bt_mesh.lpn.sub,
		.ctx = &ctx,
		.src = bt_mesh_primary_addr(),
		.xmit = POLL_XMIT,
		.friend_cred = true,
	};
	struct bt_mesh_lpn *lpn = &bt_mesh.lpn;
	uint8_t fsn = lpn->fsn;
	int err;

	LOG_DBG("lpn->sent_req 0x%02x", lpn->sent_req);

	if (lpn->sent_req) {
		if (lpn->sent_req != TRANS_CTL_OP_FRIEND_POLL) {
			lpn->pending_poll = 1U;
		}

		return 0;
	}

	err = bt_mesh_ctl_send(&tx, TRANS_CTL_OP_FRIEND_POLL, &fsn, 1,
			       &req_send_cb, NULL);
	if (err == 0) {
		lpn->pending_poll = 0U;
		lpn->sent_req = TRANS_CTL_OP_FRIEND_POLL;
	}

	return err;
}

void bt_mesh_lpn_disable(bool force)
{
	if (bt_mesh.lpn.state == BT_MESH_LPN_DISABLED) {
		return;
	}

	clear_friendship(force, true);
}

int bt_mesh_lpn_set(bool enable)
{
	struct bt_mesh_lpn *lpn = &bt_mesh.lpn;

	if (enable) {
		if (lpn->state != BT_MESH_LPN_DISABLED) {
			return 0;
		}
	} else {
		if (lpn->state == BT_MESH_LPN_DISABLED) {
			return 0;
		}
	}

	if (!bt_mesh_is_provisioned()) {
		if (enable) {
			lpn_set_state(BT_MESH_LPN_ENABLED);
		} else {
			lpn_set_state(BT_MESH_LPN_DISABLED);
		}

		return 0;
	}

	if (enable) {
		lpn_set_state(BT_MESH_LPN_ENABLED);

		if (IS_ENABLED(CONFIG_BT_MESH_LPN_ESTABLISHMENT)) {
			bt_mesh_scan_disable();
		}

		send_friend_req(lpn);
	} else {
		if (IS_ENABLED(CONFIG_BT_MESH_LPN_AUTO) &&
		    lpn->state == BT_MESH_LPN_TIMER) {
			/* If this fails, the work handler will just exit on the
			 * next timeout.
			 */
			(void)k_work_cancel_delayable(&lpn->timer);
			lpn_set_state(BT_MESH_LPN_DISABLED);
		} else {
			bt_mesh_lpn_disable(false);
		}
	}

	return 0;
}

void bt_mesh_lpn_friendship_end(void)
{
	struct bt_mesh_lpn *lpn = &bt_mesh.lpn;

	if (!lpn->established) {
		return;
	}

	clear_friendship(true, false);
}

static void friend_response_received(struct bt_mesh_lpn *lpn)
{
	LOG_DBG("lpn->sent_req 0x%02x", lpn->sent_req);

	if (lpn->sent_req == TRANS_CTL_OP_FRIEND_POLL) {
		lpn->fsn++;
	}

	lpn_set_state(BT_MESH_LPN_ESTABLISHED);
	lpn->req_attempts = 0U;
	lpn->sent_req = 0U;

	/* Schedule the next poll. This may be overridden by additional
	 * processing of the received response.
	 */
	int32_t timeout = poll_timeout(lpn);

	k_work_reschedule(&lpn->timer, K_MSEC(timeout));
	bt_mesh_scan_disable();
}

void bt_mesh_lpn_msg_received(struct bt_mesh_net_rx *rx)
{
	struct bt_mesh_lpn *lpn = &bt_mesh.lpn;

	if (lpn->state == BT_MESH_LPN_TIMER) {
		LOG_DBG("Restarting establishment timer");
		k_work_reschedule(&lpn->timer, K_MSEC(LPN_AUTO_TIMEOUT));
		return;
	}

	/* If the message was a Friend control message, it's possible that a
	 * Poll was already queued for sending. In this case, we're already in
	 * a different state.
	 */
	if (lpn->state != BT_MESH_LPN_WAIT_UPDATE) {
		return;
	}

	if (lpn->sent_req != TRANS_CTL_OP_FRIEND_POLL) {
		LOG_WRN("Unexpected message without a preceding Poll");
		return;
	}

	friend_response_received(lpn);

	LOG_DBG("Requesting more messages from Friend");

	send_friend_poll();
}

static int friend_cred_create(struct bt_mesh_net_cred *cred, const struct bt_mesh_key *key)
{
	struct bt_mesh_lpn *lpn = &bt_mesh.lpn;

	return bt_mesh_friend_cred_create(cred, bt_mesh_primary_addr(),
					  lpn->frnd, lpn->lpn_counter,
					  lpn->frnd_counter, key);
}

int bt_mesh_lpn_friend_offer(struct bt_mesh_net_rx *rx,
			     struct net_buf_simple *buf)
{
	struct bt_mesh_ctl_friend_offer *msg = (void *)buf->data;
	struct bt_mesh_lpn *lpn = &bt_mesh.lpn;
	uint16_t frnd_counter;
	int err;

	if (buf->len < sizeof(*msg)) {
		LOG_WRN("Too short Friend Offer");
		return -EBADMSG;
	}

	if (lpn->state != BT_MESH_LPN_WAIT_OFFER) {
		LOG_WRN("Ignoring unexpected Friend Offer");
		return 0;
	}

	if (!msg->recv_win) {
		LOG_WRN("Prohibited ReceiveWindow value");
		return -EBADMSG;
	}

	frnd_counter = sys_be16_to_cpu(msg->frnd_counter);

	LOG_DBG("recv_win %u queue_size %u sub_list_size %u rssi %d counter %u", msg->recv_win,
		msg->queue_size, msg->sub_list_size, msg->rssi, frnd_counter);

	lpn->frnd_counter = frnd_counter;
	lpn->frnd = rx->ctx.addr;

	/* Create friend credentials for each of the valid keys in the
	 * friendship subnet:
	 */
	for (int i = 0; i < ARRAY_SIZE(lpn->cred); i++) {
		if (!lpn->sub->keys[i].valid) {
			continue;
		}

		err = friend_cred_create(&lpn->cred[i], &lpn->sub->keys[i].net);
		if (err) {
			lpn->frnd = BT_MESH_ADDR_UNASSIGNED;
			return err;
		}
	}

	/* TODO: Add offer acceptance criteria check */

	lpn->recv_win = msg->recv_win;
	lpn->queue_size = msg->queue_size;

	err = send_friend_poll();
	if (err) {
		LOG_WRN("LPN didn't succeed poll sending (err %d)", err);
		for (int i = 0; i < ARRAY_SIZE(lpn->cred); i++) {
			if (lpn->sub->keys[i].valid) {
				bt_mesh_friend_cred_destroy(&lpn->cred[i]);
			}
		}

		lpn->sub = NULL;
		lpn->frnd = BT_MESH_ADDR_UNASSIGNED;
		lpn->recv_win = 0U;
		lpn->queue_size = 0U;
	}

	return 0;
}

int bt_mesh_lpn_friend_clear_cfm(struct bt_mesh_net_rx *rx,
				 struct net_buf_simple *buf)
{
	struct bt_mesh_ctl_friend_clear_confirm *msg = (void *)buf->data;
	struct bt_mesh_lpn *lpn = &bt_mesh.lpn;
	uint16_t addr, counter;

	if (buf->len < sizeof(*msg)) {
		LOG_WRN("Too short Friend Clear Confirm");
		return -EBADMSG;
	}

	if (lpn->state != BT_MESH_LPN_CLEAR) {
		LOG_WRN("Ignoring unexpected Friend Clear Confirm");
		return 0;
	}

	addr = sys_be16_to_cpu(msg->lpn_addr);
	counter = sys_be16_to_cpu(msg->lpn_counter);

	LOG_DBG("LPNAddress 0x%04x LPNCounter 0x%04x", addr, counter);

	if (addr != bt_mesh_primary_addr() || counter != lpn->lpn_counter) {
		LOG_WRN("Invalid parameters in Friend Clear Confirm");
		return 0;
	}

	lpn->clear_success = 1U;
	clear_friendship(false, lpn->disable);

	return 0;
}

static void lpn_group_add(uint16_t group)
{
	struct bt_mesh_lpn *lpn = &bt_mesh.lpn;
	uint16_t *free_slot = NULL;
	int i;

	for (i = 0; i < ARRAY_SIZE(lpn->groups); i++) {
		if (lpn->groups[i] == group) {
			atomic_clear_bit(lpn->to_remove, i);
			return;
		}

		if (!free_slot && lpn->groups[i] == BT_MESH_ADDR_UNASSIGNED) {
			free_slot = &lpn->groups[i];
		}
	}

	if (!free_slot) {
		LOG_WRN("Friend Subscription List exceeded!");
		return;
	}

	*free_slot = group;
	lpn->groups_changed = 1U;
}

static void lpn_group_del(uint16_t group)
{
	struct bt_mesh_lpn *lpn = &bt_mesh.lpn;
	int i;

	for (i = 0; i < ARRAY_SIZE(lpn->groups); i++) {
		if (lpn->groups[i] == group) {
			if (atomic_test_bit(lpn->added, i) ||
			    atomic_test_bit(lpn->pending, i)) {
				atomic_set_bit(lpn->to_remove, i);
				lpn->groups_changed = 1U;
			} else {
				lpn->groups[i] = BT_MESH_ADDR_UNASSIGNED;
			}
		}
	}
}

static inline int group_popcount(atomic_t *target)
{
#if CONFIG_BT_MESH_LPN_GROUPS > 32
	int i, count = 0;

	for (i = 0; i < ARRAY_SIZE(bt_mesh.lpn.added); i++) {
		count += POPCOUNT(atomic_get(&target[i]));
	}
#else
	return POPCOUNT(atomic_get(target));
#endif
}

static bool sub_update(uint8_t op)
{
	struct bt_mesh_lpn *lpn = &bt_mesh.lpn;
	int added_count = group_popcount(lpn->added);
	struct bt_mesh_msg_ctx ctx = {
		.net_idx     = lpn->sub->net_idx,
		.app_idx     = BT_MESH_KEY_UNUSED,
		.addr        = lpn->frnd,
		.send_ttl    = 0,
	};
	struct bt_mesh_net_tx tx = {
		.sub = lpn->sub,
		.ctx = &ctx,
		.src = bt_mesh_primary_addr(),
		.xmit = POLL_XMIT,
		.friend_cred = true,
	};
	struct bt_mesh_ctl_friend_sub req;
	size_t i, g;

	LOG_DBG("op 0x%02x sent_req 0x%02x", op, lpn->sent_req);

	if (lpn->sent_req) {
		return false;
	}

	for (i = 0, g = 0; i < ARRAY_SIZE(lpn->groups); i++) {
		if (lpn->groups[i] == BT_MESH_ADDR_UNASSIGNED) {
			continue;
		}

		if (op == TRANS_CTL_OP_FRIEND_SUB_ADD) {
			if (atomic_test_bit(lpn->added, i)) {
				continue;
			}
		} else {
			if (!atomic_test_bit(lpn->to_remove, i)) {
				continue;
			}
		}

		if (added_count + g >= lpn->queue_size) {
			LOG_WRN("Friend Queue Size exceeded");
			break;
		}

		req.addr_list[g++] = sys_cpu_to_be16(lpn->groups[i]);
		atomic_set_bit(lpn->pending, i);

		if (g == ARRAY_SIZE(req.addr_list)) {
			break;
		}
	}

	if (g == 0) {
		group_zero(lpn->pending);
		return false;
	}

	req.xact = lpn->xact_next++;

	if (bt_mesh_ctl_send(&tx, op, &req, 1 + g * 2,
			     &req_send_cb, NULL) < 0) {
		group_zero(lpn->pending);
		return false;
	}

	lpn->xact_pending = req.xact;
	lpn->sent_req = op;
	return true;
}

static void update_timeout(struct bt_mesh_lpn *lpn)
{
	if (lpn->established) {
		LOG_WRN("No response from Friend during ReceiveWindow");
		lpn_set_state(BT_MESH_LPN_ESTABLISHED);
		k_work_reschedule(&lpn->timer, K_MSEC(POLL_RETRY_TIMEOUT));
		bt_mesh_scan_disable();
	} else {
		if (IS_ENABLED(CONFIG_BT_MESH_LPN_ESTABLISHMENT)) {
			bt_mesh_scan_disable();
		}

		if (lpn->req_attempts < REQ_ATTEMPTS(lpn)) {
			LOG_WRN("Retrying first Friend Poll");
			lpn->sent_req = 0U;
			if (send_friend_poll() == 0) {
				return;
			}
		}

		LOG_ERR("Timed out waiting for first Friend Update");
		clear_friendship(false, false);
	}
}

static void lpn_timeout(struct k_work *work)
{
	struct bt_mesh_lpn *lpn = &bt_mesh.lpn;

#if defined(CONFIG_BT_MESH_LOW_POWER_LOG_LEVEL_DBG)
	LOG_DBG("state: %s", state2str(lpn->state));
#endif

	switch (lpn->state) {
	case BT_MESH_LPN_DISABLED:
		break;
	case BT_MESH_LPN_CLEAR:
		clear_friendship(false, bt_mesh.lpn.disable);
		break;
	case BT_MESH_LPN_TIMER:
		LOG_DBG("Starting to look for Friend nodes");
		lpn_set_state(BT_MESH_LPN_ENABLED);
		if (IS_ENABLED(CONFIG_BT_MESH_LPN_ESTABLISHMENT)) {
			bt_mesh_scan_disable();
		}
		__fallthrough;
	case BT_MESH_LPN_ENABLED:
		send_friend_req(lpn);
		break;
	case BT_MESH_LPN_REQ_WAIT:
		k_work_reschedule(&lpn->timer, K_MSEC(lpn->adv_duration + FRIEND_REQ_SCAN));
		lpn_set_state(BT_MESH_LPN_WAIT_OFFER);
		bt_mesh_scan_enable();
		break;
	case BT_MESH_LPN_WAIT_OFFER:
		LOG_WRN("No acceptable Friend Offers received");
		lpn_set_state(BT_MESH_LPN_ENABLED);
		lpn->sent_req = 0U;
		k_work_reschedule(&lpn->timer, FRIEND_REQ_RETRY_TIMEOUT);

		if (IS_ENABLED(CONFIG_BT_MESH_LPN_ESTABLISHMENT)) {
			bt_mesh_scan_disable();
		}
		break;
	case BT_MESH_LPN_ESTABLISHED:
		if (lpn->req_attempts < REQ_ATTEMPTS(lpn)) {
			uint8_t req = lpn->sent_req;

			lpn->sent_req = 0U;

			if (!req || req == TRANS_CTL_OP_FRIEND_POLL) {
				send_friend_poll();
			} else {
				sub_update(req);
			}

			break;
		}

		LOG_ERR("No response from Friend after %u retries", lpn->req_attempts);
		lpn->req_attempts = 0U;
		clear_friendship(true, false);
		break;
	case BT_MESH_LPN_RECV_DELAY:
		k_work_reschedule(&lpn->timer,
				  K_MSEC(SCAN_LATENCY + lpn->recv_win + RX_DELAY_CORRECTION(lpn)));
		lpn_set_state(BT_MESH_LPN_WAIT_UPDATE);
		bt_mesh_scan_enable();
		break;
	case BT_MESH_LPN_WAIT_UPDATE:
		update_timeout(lpn);
		break;
	default:
		__ASSERT(0, "Unhandled LPN state");
		break;
	}
}

void bt_mesh_lpn_group_add(uint16_t group)
{
	LOG_DBG("group 0x%04x", group);

	lpn_group_add(group);

	if (!bt_mesh_lpn_established() || bt_mesh.lpn.sent_req) {
		return;
	}

	sub_update(TRANS_CTL_OP_FRIEND_SUB_ADD);
}

void bt_mesh_lpn_group_del(const uint16_t *groups, size_t group_count)
{
	int i;

	for (i = 0; i < group_count; i++) {
		if (groups[i] != BT_MESH_ADDR_UNASSIGNED) {
			LOG_DBG("group 0x%04x", groups[i]);
			lpn_group_del(groups[i]);
		}
	}

	if (!bt_mesh_lpn_established() || bt_mesh.lpn.sent_req) {
		return;
	}

	sub_update(TRANS_CTL_OP_FRIEND_SUB_REM);
}

int bt_mesh_lpn_friend_sub_cfm(struct bt_mesh_net_rx *rx,
			       struct net_buf_simple *buf)
{
	struct bt_mesh_ctl_friend_sub_confirm *msg = (void *)buf->data;
	struct bt_mesh_lpn *lpn = &bt_mesh.lpn;

	if (buf->len < sizeof(*msg)) {
		LOG_WRN("Too short Friend Subscription Confirm");
		return -EBADMSG;
	}

	LOG_DBG("xact 0x%02x", msg->xact);

	if (!lpn->sent_req) {
		LOG_WRN("No pending subscription list message");
		return 0;
	}

	if (msg->xact != lpn->xact_pending) {
		LOG_WRN("Transaction mismatch (0x%02x != 0x%02x)", msg->xact, lpn->xact_pending);
		return 0;
	}

	if (lpn->sent_req == TRANS_CTL_OP_FRIEND_SUB_ADD) {
		group_set(lpn->added, lpn->pending);
		group_zero(lpn->pending);
	} else if (lpn->sent_req == TRANS_CTL_OP_FRIEND_SUB_REM) {
		int i;

		group_clear(lpn->added, lpn->pending);

		for (i = 0; i < ARRAY_SIZE(lpn->groups); i++) {
			if (atomic_test_and_clear_bit(lpn->pending, i) &&
			    atomic_test_and_clear_bit(lpn->to_remove, i)) {
				lpn->groups[i] = BT_MESH_ADDR_UNASSIGNED;
			}
		}
	} else {
		LOG_WRN("Unexpected Friend Subscription Confirm");
		return 0;
	}

	friend_response_received(lpn);

	if (lpn->groups_changed) {
		sub_update(TRANS_CTL_OP_FRIEND_SUB_ADD);
		sub_update(TRANS_CTL_OP_FRIEND_SUB_REM);

		if (!lpn->sent_req) {
			lpn->groups_changed = 0U;
		}
	}

	if (lpn->pending_poll) {
		send_friend_poll();
	}

	return 0;
}

int bt_mesh_lpn_friend_update(struct bt_mesh_net_rx *rx,
			      struct net_buf_simple *buf)
{
	struct bt_mesh_ctl_friend_update *msg = (void *)buf->data;
	struct bt_mesh_lpn *lpn = &bt_mesh.lpn;
	struct bt_mesh_subnet *sub = rx->sub;
	uint32_t iv_index;
	bool established = false;

	if (buf->len < sizeof(*msg)) {
		LOG_WRN("Too short Friend Update");
		return -EBADMSG;
	}

	if (lpn->sent_req != TRANS_CTL_OP_FRIEND_POLL) {
		LOG_WRN("Unexpected friend update");
		return 0;
	}

	if (sub->kr_phase == BT_MESH_KR_PHASE_2 && !rx->new_key) {
		LOG_WRN("Ignoring Phase 2 KR Update secured using old key");
		return 0;
	}

	if (atomic_test_bit(bt_mesh.flags, BT_MESH_IVU_INITIATOR) &&
	    (atomic_test_bit(bt_mesh.flags, BT_MESH_IVU_IN_PROGRESS) ==
	     BT_MESH_IV_UPDATE(msg->flags))) {
		bt_mesh_beacon_ivu_initiator(false);
	}

	if (!lpn->established) {
		/* This is normally checked on the transport layer, however
		 * in this state we're also still accepting flooding
		 * credentials so we need to ensure the right ones (Friend
		 * Credentials) were used for this message.
		 */
		if (!rx->friend_cred) {
			LOG_WRN("Friend Update with wrong credentials");
			return -EINVAL;
		}

		lpn->established = 1U;

		LOG_INF("Friendship established with 0x%04x", lpn->frnd);

		bt_mesh_hb_feature_changed(BT_MESH_FEAT_LOW_POWER);

		/* Set initial poll timeout */
		lpn->poll_timeout = MIN(POLL_TIMEOUT_MAX(lpn),
					POLL_TIMEOUT_INIT);

		established = true;
	}

	friend_response_received(lpn);

	iv_index = sys_be32_to_cpu(msg->iv_index);

	LOG_DBG("flags 0x%02x iv_index 0x%08x md %u", msg->flags, iv_index, msg->md);

	bt_mesh_kr_update(sub, BT_MESH_KEY_REFRESH(msg->flags), rx->new_key);
	bt_mesh_net_iv_update(iv_index, BT_MESH_IV_UPDATE(msg->flags));

	if (lpn->groups_changed) {
		sub_update(TRANS_CTL_OP_FRIEND_SUB_ADD);
		sub_update(TRANS_CTL_OP_FRIEND_SUB_REM);

		if (!lpn->sent_req) {
			lpn->groups_changed = 0U;
		}
	}

	if (msg->md) {
		LOG_DBG("Requesting for more messages");
		send_friend_poll();
	}

	if (established) {
		STRUCT_SECTION_FOREACH(bt_mesh_lpn_cb, cb) {
			if (cb->established) {
				cb->established(lpn->sub->net_idx, lpn->frnd, lpn->queue_size,
						lpn->recv_win);
			}
		}
	}

	return 0;
}

int bt_mesh_lpn_poll(void)
{
	if (!bt_mesh.lpn.established) {
		return -EAGAIN;
	}

	LOG_DBG("Requesting more messages");

	return send_friend_poll();
}

static void subnet_evt(struct bt_mesh_subnet *sub, enum bt_mesh_key_evt evt)
{
	switch (evt) {
	case BT_MESH_KEY_DELETED:
		if (sub == bt_mesh.lpn.sub) {
			LOG_DBG("NetKey deleted");
			clear_friendship(true, false);
		}
		break;
	case BT_MESH_KEY_UPDATED:
		LOG_DBG("NetKey updated");
		friend_cred_create(&bt_mesh.lpn.cred[1], &sub->keys[1].net);
		break;
	default:
		break;
	}
}

BT_MESH_SUBNET_CB_DEFINE(lpn) = {
	.evt_handler = subnet_evt,
};

int bt_mesh_lpn_init(void)
{
	struct bt_mesh_lpn *lpn = &bt_mesh.lpn;

	LOG_DBG("");

	k_work_init_delayable(&lpn->timer, lpn_timeout);

	if (lpn->state == BT_MESH_LPN_ENABLED) {
		if (IS_ENABLED(CONFIG_BT_MESH_LPN_ESTABLISHMENT)) {
			bt_mesh_scan_disable();
		} else {
			bt_mesh_scan_enable();
		}

		send_friend_req(lpn);
	} else {
		bt_mesh_scan_enable();

		if (IS_ENABLED(CONFIG_BT_MESH_LPN_AUTO)) {
			LOG_DBG("Waiting %u ms for messages", LPN_AUTO_TIMEOUT);
			lpn_set_state(BT_MESH_LPN_TIMER);
			k_work_reschedule(&lpn->timer,
					  K_MSEC(LPN_AUTO_TIMEOUT));
		}
	}

	return 0;
}
