/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Basalte bv
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/math_extras.h>
#include <zephyr/sys/util.h>

#include "ipc_link_priv.h"

LOG_MODULE_REGISTER(ipc_link, CONFIG_IPC_SERVICE_BACKEND_LINK_LOG_LEVEL);

#define IPC_LINK_NUM_EP     CONFIG_IPC_SERVICE_BACKEND_LINK_NUM_EP
#define IPC_LINK_RX_BUFFERS CONFIG_IPC_SERVICE_BACKEND_LINK_RX_BUFFERS

/*
 * Control messages on the control channel: [op][endpoint address], followed by the name for a
 * bind, and by the address of the endpoint bound for its reply
 */
#define IPC_LINK_CTRL_BIND          1U
#define IPC_LINK_CTRL_BIND_ACK      2U
#define IPC_LINK_CTRL_UNBIND        3U
/* The peer unbound the endpoint at the address carried: the address can be reused */
#define IPC_LINK_CTRL_UNBIND_ACK    4U
#define IPC_LINK_CTRL_HDR_SIZE      2U
#define IPC_LINK_CTRL_BIND_ACK_SIZE 3U

#define IPC_LINK_EPT_NAME_MAX CONFIG_IPC_SERVICE_BACKEND_LINK_EPT_NAME_MAX

/* Control messages pending for an endpoint */
#define IPC_LINK_EPT_BIND   BIT(0)
#define IPC_LINK_EPT_REPLY  BIT(1)
#define IPC_LINK_EPT_UNBIND BIT(2)

BUILD_ASSERT(IPC_LINK_NUM_EP <= 254, "Endpoint addresses are one byte");
BUILD_ASSERT(IPC_LINK_RX_BUFFERS <= IPC_LINK_RX_BUFS_MAX);
BUILD_ASSERT(IPC_LINK_WINDOW >= 1 && IPC_LINK_WINDOW <= IPC_LINK_WINDOW_MAX);

#ifdef CONFIG_IPC_SERVICE_BACKEND_LINK_WQ_ENABLE
static K_KERNEL_STACK_DEFINE(ipc_link_wq_stack, CONFIG_IPC_SERVICE_BACKEND_LINK_WQ_STACK_SIZE);
static struct k_work_q ipc_link_wq;

static int ipc_link_wq_init(void)
{
	const struct k_work_queue_config cfg = {
		.name = "ipc_link",
	};

	k_work_queue_start(&ipc_link_wq, ipc_link_wq_stack,
			   K_KERNEL_STACK_SIZEOF(ipc_link_wq_stack),
			   CONFIG_IPC_SERVICE_BACKEND_LINK_WQ_PRIORITY, &cfg);

	return 0;
}

SYS_INIT(ipc_link_wq_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

static void ipc_link_submit_rx(struct ipc_link *link)
{
	(void)k_work_submit_to_queue(&ipc_link_wq, &link->rx_work);
}
#else
static void ipc_link_submit_rx(struct ipc_link *link)
{
	(void)k_work_submit(&link->rx_work);
}
#endif /* CONFIG_IPC_SERVICE_BACKEND_LINK_WQ_ENABLE */

static inline struct ipc_link *ipc_link_get(const struct device *dev)
{
	const struct ipc_link_config *cfg = dev->config;

	return cfg->link;
}

static uint8_t ipc_link_ept_addr(const struct ipc_link *link, const struct ipc_link_ept *ept)
{
	return (uint8_t)(ept - link->epts) + 1U;
}

static uint32_t ipc_link_now(void)
{
	return k_uptime_get_32();
}

/* The timer fires when the core next has a retransmission, a poll or a boot announcement due */
static void ipc_link_schedule_timer(struct ipc_link *link)
{
	k_spinlock_key_t key;
	uint32_t timeout;

	key = k_spin_lock(&link->lock);
	timeout = ipc_link_core_timeout(&link->core, ipc_link_now());
	k_spin_unlock(&link->lock, key);

	if (timeout != UINT32_MAX) {
		(void)k_work_reschedule(&link->timer, K_MSEC(timeout));
	}
}

/*
 * Something is to be sent: the transport sends it, and the link is timed while it waits for the
 * peer's answer
 */
static void ipc_link_kick(const struct device *dev)
{
	const struct ipc_link_config *cfg = dev->config;

	cfg->api->kick(dev);
	ipc_link_schedule_timer(ipc_link_get(dev));
}

static uint16_t ipc_link_ctrl_len(const struct ipc_link_ept *ept, uint8_t op)
{
	switch (op) {
	case IPC_LINK_CTRL_BIND:
		return IPC_LINK_CTRL_HDR_SIZE + strlen(ept->cfg->name);
	case IPC_LINK_CTRL_BIND_ACK:
		return IPC_LINK_CTRL_BIND_ACK_SIZE;
	default:
		return IPC_LINK_CTRL_HDR_SIZE;
	}
}

/* With the lock held: the peer may reuse the address it unbound */
static int ipc_link_send_unbind_ack(struct ipc_link *link, uint8_t addr)
{
	const uint8_t msg[IPC_LINK_CTRL_HDR_SIZE] = {IPC_LINK_CTRL_UNBIND_ACK, addr};

	return ipc_link_core_send(&link->core, IPC_LINK_CTRL_ADDR, msg, sizeof(msg));
}

/* With the lock held: a control message about one of our endpoints */
static int ipc_link_send_ctrl(struct ipc_link *link, struct ipc_link_ept *ept, uint8_t op)
{
	uint8_t msg[IPC_LINK_CTRL_HDR_SIZE + IPC_LINK_EPT_NAME_MAX];
	uint16_t len = ipc_link_ctrl_len(ept, op);

	msg[0] = op;
	msg[1] = ipc_link_ept_addr(link, ept);

	switch (op) {
	case IPC_LINK_CTRL_BIND:
		memcpy(&msg[IPC_LINK_CTRL_HDR_SIZE], ept->cfg->name, len - IPC_LINK_CTRL_HDR_SIZE);
		break;
	case IPC_LINK_CTRL_BIND_ACK:
		/* Names the bind it answers: a reply to a bind withdrawn since is dropped there */
		msg[IPC_LINK_CTRL_HDR_SIZE] = ept->remote_addr;
		break;
	default:
		break;
	}

	return ipc_link_core_send(&link->core, IPC_LINK_CTRL_ADDR, msg, len);
}

/* With the lock held: whether a control message can be queued now */
static bool ipc_link_ctrl_pending(const struct ipc_link *link)
{
	for (size_t i = 0; i < ARRAY_SIZE(link->unbind_ack_due); i++) {
		if (atomic_get(&link->unbind_ack_due[i]) != 0 &&
		    ipc_link_core_has_room(&link->core, IPC_LINK_CTRL_HDR_SIZE)) {
			return true;
		}
	}

	for (size_t i = 0; i < ARRAY_SIZE(link->epts); i++) {
		const struct ipc_link_ept *ept = &link->epts[i];

		if (ept->ctrl == 0U) {
			continue;
		}

		if ((ept->ctrl & IPC_LINK_EPT_UNBIND) != 0U) {
			if (ipc_link_core_has_room(&link->core,
						   ipc_link_ctrl_len(ept, IPC_LINK_CTRL_UNBIND))) {
				return true;
			}
		} else if ((ept->ctrl & IPC_LINK_EPT_REPLY) != 0U) {
			if (ipc_link_core_has_room(
				    &link->core, ipc_link_ctrl_len(ept, IPC_LINK_CTRL_BIND_ACK))) {
				return true;
			}
		} else if (ipc_link_core_has_room(&link->core,
						  ipc_link_ctrl_len(ept, IPC_LINK_CTRL_BIND))) {
			return true;
		}
	}

	return false;
}

/* With the lock held: queue the control messages that fit */
static void ipc_link_flush_ctrl(struct ipc_link *link)
{
	static const struct {
		uint8_t flag;
		uint8_t op;
	} ops[] = {
		{IPC_LINK_EPT_UNBIND, IPC_LINK_CTRL_UNBIND},
		{IPC_LINK_EPT_REPLY, IPC_LINK_CTRL_BIND_ACK},
		{IPC_LINK_EPT_BIND, IPC_LINK_CTRL_BIND},
	};

	/* The peer waits for these to reuse its addresses */
	for (size_t i = 0; i < ARRAY_SIZE(link->unbind_ack_due); i++) {
		atomic_val_t due;

		while ((due = atomic_get(&link->unbind_ack_due[i])) != 0) {
			size_t addr = i * ATOMIC_BITS + u64_count_trailing_zeros((uint64_t)due);

			if (ipc_link_send_unbind_ack(link, (uint8_t)addr) < 0) {
				break;
			}

			atomic_clear_bit(link->unbind_ack_due, addr);
		}
	}

	for (size_t i = 0; i < ARRAY_SIZE(link->epts); i++) {
		struct ipc_link_ept *ept = &link->epts[i];

		for (size_t j = 0; j < ARRAY_SIZE(ops); j++) {
			if ((ept->ctrl & ops[j].flag) == 0U) {
				continue;
			}

			/* One that doesn't fit doesn't block a shorter one after it */
			if (ipc_link_send_ctrl(link, ept, ops[j].op) == 0) {
				ept->ctrl &= ~ops[j].flag;
			}
		}
	}
}

/*
 * A new session, with the lock held: forget the peer's endpoint addresses before anything is
 * sent to them and announce the endpoints again, the delivery calls the unbound callbacks
 */
static void ipc_link_reset_endpoints(struct ipc_link *link)
{
	for (size_t i = 0; i < ARRAY_SIZE(link->epts); i++) {
		struct ipc_link_ept *ept = &link->epts[i];

		/* Until the delivery calls unbound(), through any number of resets */
		ept->was_bound |= ept->remote_addr != 0U;
		ept->remote_addr = 0U;
		ept->ctrl = ept->cfg != NULL ? IPC_LINK_EPT_BIND : 0U;
		/* The peer forgot the address */
		ept->retired = false;
	}

	for (size_t i = 0; i < ARRAY_SIZE(link->unbind_ack_due); i++) {
		(void)atomic_clear(&link->unbind_ack_due[i]);
	}

	atomic_set(&link->session_changed, 1);
}

/* With the lock held: the frame of rx_idx is delivered once the lock is released */
static void ipc_link_handle_events(struct ipc_link *link, uint32_t evt, uint8_t rx_idx,
				   struct ipc_link_rx_item *item)
{
	if ((evt & (IPC_LINK_CORE_EVT_CONNECTED | IPC_LINK_CORE_EVT_DISCONNECTED)) != 0U) {
		ipc_link_reset_endpoints(link);
	}

	if ((evt & IPC_LINK_CORE_EVT_RX) != 0U) {
		item->session = link->core.session;
		item->idx = rx_idx;
	}
}

/*
 * With the lock held: queues the control messages that fit, so that they are timed like any
 * payload the transport does not send, and tells whether a frame is to be sent
 */
static bool ipc_link_pending_locked(struct ipc_link *link)
{
	ipc_link_flush_ctrl(link);

	return ipc_link_core_pending(&link->core, ipc_link_now()) || ipc_link_ctrl_pending(link);
}

/* With the lock held: a receive buffer goes back to the core once neither delivered nor held */
static bool ipc_link_rx_free(struct ipc_link *link, uint8_t idx)
{
	if ((link->rx_delivering & BIT(idx)) != 0U || link->rx_holds[idx] > 0U) {
		return false;
	}

	ipc_link_core_rx_release(&link->core, idx);

	return ipc_link_pending_locked(link);
}

static void ipc_link_queue_rx(struct ipc_link *link, const struct ipc_link_rx_item *item)
{
	if (k_msgq_put(&link->rxq, item, K_NO_WAIT) < 0) {
		k_spinlock_key_t key = k_spin_lock(&link->lock);

		/* Or the buffer stays taken, and its credit lost */
		LOG_ERR("Receive queue full");
		ipc_link_core_rx_release(&link->core, item->idx);
		k_spin_unlock(&link->lock, key);
	}
}

static void ipc_link_timer_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct ipc_link *link = CONTAINER_OF(dwork, struct ipc_link, timer);
	struct ipc_link_rx_item item;
	k_spinlock_key_t key;
	bool pending;
	uint32_t evt;

	key = k_spin_lock(&link->lock);
	evt = ipc_link_core_check(&link->core, ipc_link_now());
	ipc_link_handle_events(link, evt, 0U, &item);
	pending = ipc_link_pending_locked(link);
	k_spin_unlock(&link->lock, key);

	if ((evt & IPC_LINK_CORE_EVT_DISCONNECTED) != 0U) {
		LOG_WRN("Session ended: the peer stopped acknowledging");
		ipc_link_submit_rx(link);
	}

	if (pending) {
		ipc_link_kick(link->dev);
	} else {
		ipc_link_schedule_timer(link);
	}
}

bool ipc_link_pending(const struct device *dev)
{
	struct ipc_link *link = ipc_link_get(dev);
	k_spinlock_key_t key;
	bool pending;

	key = k_spin_lock(&link->lock);
	pending = ipc_link_pending_locked(link);
	k_spin_unlock(&link->lock, key);

	return pending;
}

const uint8_t *ipc_link_tx_prepare(const struct device *dev, size_t *len)
{
	struct ipc_link *link = ipc_link_get(dev);
	const uint8_t *frame;
	k_spinlock_key_t key;

	key = k_spin_lock(&link->lock);
	ipc_link_flush_ctrl(link);
	frame = ipc_link_core_tx_prepare(&link->core, ipc_link_now(), len);
	k_spin_unlock(&link->lock, key);

	/* A payload or an announcement just went out: time its answer */
	ipc_link_schedule_timer(link);

	return frame;
}

uint8_t *ipc_link_rx_buf(const struct device *dev)
{
	struct ipc_link *link = ipc_link_get(dev);
	k_spinlock_key_t key;
	uint8_t *buf;

	key = k_spin_lock(&link->lock);
	buf = ipc_link_core_rx_buf(&link->core);
	k_spin_unlock(&link->lock, key);

	return buf;
}

int ipc_link_stats_get(const struct device *dev, struct ipc_link_stats *stats)
{
#ifdef CONFIG_IPC_SERVICE_LINK_STATS
	struct ipc_link *link = ipc_link_get(dev);
	k_spinlock_key_t key;

	key = k_spin_lock(&link->lock);
	*stats = link->core.stats;
	k_spin_unlock(&link->lock, key);

	return 0;
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(stats);

	return -ENOTSUP;
#endif
}

int ipc_link_stats_reset(const struct device *dev)
{
#ifdef CONFIG_IPC_SERVICE_LINK_STATS
	struct ipc_link *link = ipc_link_get(dev);
	k_spinlock_key_t key;

	key = k_spin_lock(&link->lock);
	memset(&link->core.stats, 0, sizeof(link->core.stats));
	k_spin_unlock(&link->lock, key);

	return 0;
#else
	ARG_UNUSED(dev);

	return -ENOTSUP;
#endif
}

uint32_t ipc_link_rx(const struct device *dev, size_t len, bool ok)
{
	const struct ipc_link_config *cfg = dev->config;
	struct ipc_link *link = ipc_link_get(dev);
	struct ipc_link_rx_item item;
	k_spinlock_key_t key;
	uint32_t session;
	uint32_t ret = 0U;
	uint32_t evt;
	uint8_t checked;
	bool pending;
	uint8_t idx;

	/* The CRCs, before the lock: the transport owns the buffer until the frame is processed */
	checked = ipc_link_core_rx_check(&link->core, len, ok);

	key = k_spin_lock(&link->lock);
	evt = ipc_link_core_rx(&link->core, checked, ipc_link_now(), &idx);
	session = link->core.session;
	ipc_link_handle_events(link, evt, idx, &item);
	pending = ipc_link_pending_locked(link);
	k_spin_unlock(&link->lock, key);

	if ((evt & IPC_LINK_CORE_EVT_HDR_ERR) != 0U) {
		ret |= IPC_LINK_EVT_HDR_ERR;
	}

	if ((evt & IPC_LINK_CORE_EVT_PEER_MISMATCH) != 0U) {
		ret |= IPC_LINK_EVT_PEER_MISMATCH;

		if (!link->mismatch_logged) {
			LOG_ERR("Peer runs another protocol version or frame size than %u B",
				cfg->mtu);
			link->mismatch_logged = true;
		}
	}

	if ((evt & IPC_LINK_CORE_EVT_DISCONNECTED) != 0U) {
		LOG_WRN("Session ended: the peer %s", (evt & IPC_LINK_CORE_EVT_PEER_MISMATCH) != 0U
							      ? "came back incompatible"
							      : "stopped acknowledging");
	}

	if ((evt & IPC_LINK_CORE_EVT_CONNECTED) != 0U) {
		ret |= IPC_LINK_EVT_CONNECTED;
		LOG_INF("Session %u started", session);
		link->mismatch_logged = false;
	}

	if ((evt & IPC_LINK_CORE_EVT_RX) != 0U) {
		ipc_link_queue_rx(link, &item);
	}

	if ((evt & (IPC_LINK_CORE_EVT_RX | IPC_LINK_CORE_EVT_CONNECTED |
		    IPC_LINK_CORE_EVT_DISCONNECTED)) != 0U) {
		ipc_link_submit_rx(link);
	}

	if (pending) {
		ipc_link_kick(dev);
	}

	return ret;
}

/* The delivery, for a session that started or ended */
static void ipc_link_session_changed(struct ipc_link *link)
{
	for (size_t i = 0; i < ARRAY_SIZE(link->epts); i++) {
		struct ipc_link_ept *ept = &link->epts[i];
		const struct ipc_ept_cfg *ept_cfg;
		k_spinlock_key_t key;

		key = k_spin_lock(&link->lock);
		ept_cfg = ept->was_bound ? ept->cfg : NULL;
		ept->was_bound = false;
		k_spin_unlock(&link->lock, key);

		if (ept_cfg != NULL && ept_cfg->cb.unbound != NULL) {
			ept_cfg->cb.unbound(ept_cfg->priv);
		}
	}
}

/* With the lock held */
static struct ipc_link_ept *ipc_link_find_ept(struct ipc_link *link, const char *name, size_t len)
{
	for (size_t i = 0; i < ARRAY_SIZE(link->epts); i++) {
		struct ipc_link_ept *ept = &link->epts[i];

		if (ept->cfg != NULL && strlen(ept->cfg->name) == len &&
		    memcmp(ept->cfg->name, name, len) == 0) {
			return ept;
		}
	}

	return NULL;
}

static void ipc_link_handle_ctrl(struct ipc_link *link, const uint8_t *msg, uint16_t len,
				 uint32_t session)
{
	const struct ipc_ept_cfg *ept_cfg = NULL;
	struct ipc_link_ept *ept = NULL;
	k_spinlock_key_t key;
	bool bound = false;
	bool unbound = false;
	bool pending;

	if (len < IPC_LINK_CTRL_HDR_SIZE || msg[1] == IPC_LINK_CTRL_ADDR ||
	    len - IPC_LINK_CTRL_HDR_SIZE > IPC_LINK_EPT_NAME_MAX) {
		LOG_WRN("Invalid control message");
		return;
	}

	key = k_spin_lock(&link->lock);

	if (session != link->core.session) {
		k_spin_unlock(&link->lock, key);
		return;
	}

	switch (msg[0]) {
	case IPC_LINK_CTRL_BIND:
		/* Without a match, the bind of the endpoint registered later will match */
		ept = ipc_link_find_ept(link, (const char *)&msg[IPC_LINK_CTRL_HDR_SIZE],
					len - IPC_LINK_CTRL_HDR_SIZE);
		if (ept == NULL) {
			break;
		}

		bound = ept->remote_addr == 0U;
		ept->remote_addr = msg[1];

		/* Before any data a bound callback sends */
		if (ipc_link_send_ctrl(link, ept, IPC_LINK_CTRL_BIND_ACK) < 0) {
			ept->ctrl |= IPC_LINK_EPT_REPLY;
		}
		break;
	case IPC_LINK_CTRL_BIND_ACK:
		/* To the endpoint whose bind it answers: one deregistered since, whose address is
		 * retired until the peer acknowledges the unbind, takes no reply
		 */
		if (len != IPC_LINK_CTRL_BIND_ACK_SIZE || msg[2] == IPC_LINK_CTRL_ADDR ||
		    msg[2] > ARRAY_SIZE(link->epts) || link->epts[msg[2] - 1U].cfg == NULL) {
			break;
		}

		ept = &link->epts[msg[2] - 1U];
		if (ept->remote_addr != 0U && ept->remote_addr != msg[1]) {
			ept = NULL;
			break;
		}

		bound = ept->remote_addr == 0U;
		ept->remote_addr = msg[1];
		break;
	case IPC_LINK_CTRL_UNBIND:
		for (size_t i = 0; i < ARRAY_SIZE(link->epts); i++) {
			if (link->epts[i].cfg != NULL && link->epts[i].remote_addr == msg[1]) {
				ept = &link->epts[i];
				ept->remote_addr = 0U;
				/* A reply still queued would answer a bind the peer withdrew */
				ept->ctrl &= ~IPC_LINK_EPT_REPLY;
				unbound = true;
				break;
			}
		}

		/* Bound here or not, nothing is sent to that address from here on */
		atomic_set_bit(link->unbind_ack_due, msg[1]);
		break;
	case IPC_LINK_CTRL_UNBIND_ACK:
		if (msg[1] <= ARRAY_SIZE(link->epts)) {
			link->epts[msg[1] - 1U].retired = false;
		}
		break;
	default:
		LOG_WRN("Unknown control message %u", msg[0]);
		break;
	}

	if (ept != NULL) {
		ept_cfg = ept->cfg;
	}

	pending = ipc_link_pending_locked(link);

	k_spin_unlock(&link->lock, key);

	if (pending) {
		ipc_link_kick(link->dev);
	}

	if (bound && ept_cfg->cb.bound != NULL) {
		ept_cfg->cb.bound(ept_cfg->priv);
	}

	if (unbound && ept_cfg->cb.unbound != NULL) {
		ept_cfg->cb.unbound(ept_cfg->priv);
	}
}

static void ipc_link_rx_delivered(struct ipc_link *link, uint8_t idx)
{
	k_spinlock_key_t key;
	bool pending;

	key = k_spin_lock(&link->lock);
	link->rx_delivering &= ~BIT(idx);
	pending = ipc_link_rx_free(link, idx);
	k_spin_unlock(&link->lock, key);

	if (pending) {
		ipc_link_kick(link->dev);
	}
}

static void ipc_link_deliver(struct ipc_link *link, uint8_t idx, uint32_t session)
{
	const struct ipc_ept_cfg *ept_cfg;
	struct ipc_link_msg msg;
	k_spinlock_key_t key;
	size_t offset = 0;

	while (ipc_link_core_msg_next(&link->core, idx, &offset, &msg)) {
		if (msg.addr == IPC_LINK_CTRL_ADDR) {
			ipc_link_handle_ctrl(link, msg.data, msg.len, session);
			continue;
		}

		ept_cfg = NULL;

		key = k_spin_lock(&link->lock);

		/* Only to an endpoint bound in the session the message was sent in: a slot may
		 * have been reused since
		 */
		if (session == link->core.session && msg.addr <= ARRAY_SIZE(link->epts) &&
		    link->epts[msg.addr - 1U].remote_addr != 0U) {
			ept_cfg = link->epts[msg.addr - 1U].cfg;
		}

		k_spin_unlock(&link->lock, key);

		if (ept_cfg == NULL) {
			LOG_DBG("Message for unbound endpoint %u", msg.addr);
			continue;
		}

		if (ept_cfg->cb.received != NULL) {
			ept_cfg->cb.received(msg.data, msg.len, ept_cfg->priv);
		}
	}
}

static void ipc_link_rx_work_handler(struct k_work *work)
{
	struct ipc_link *link = CONTAINER_OF(work, struct ipc_link, rx_work);
	struct ipc_link_rx_item item;
	k_spinlock_key_t key;

	while (k_msgq_get(&link->rxq, &item, K_NO_WAIT) == 0) {
		/* A frame of a new session was queued after the change was flagged: the unbound
		 * callbacks of the old session come first
		 */
		if (atomic_clear(&link->session_changed) != 0) {
			ipc_link_session_changed(link);
		}

		key = k_spin_lock(&link->lock);
		link->rx_delivering |= BIT(item.idx);
		k_spin_unlock(&link->lock, key);

		/* A session change in between: the messages are dropped, the buffer is given back
		 * all the same
		 */
		ipc_link_deliver(link, item.idx, item.session);
		ipc_link_rx_delivered(link, item.idx);
	}

	if (atomic_clear(&link->session_changed) != 0) {
		ipc_link_session_changed(link);
	}
}

static int ipc_link_open_instance(const struct device *dev)
{
	const struct ipc_link_config *cfg = dev->config;
	struct ipc_link *link = ipc_link_get(dev);
	int ret;

	if (link->opened) {
		return -EALREADY;
	}

	ret = cfg->api->open(dev);
	if (ret < 0) {
		return ret;
	}

	link->opened = true;
	ipc_link_kick(dev);

	return 0;
}

static int ipc_link_close_instance(const struct device *dev)
{
	ARG_UNUSED(dev);

	return -ENOTSUP;
}

static int ipc_link_register_endpoint(const struct device *dev, void **token,
				      const struct ipc_ept_cfg *ept_cfg)
{
	struct ipc_link *link = ipc_link_get(dev);
	struct ipc_link_ept *ept = NULL;
	k_spinlock_key_t key;
	bool pending;

	if (ept_cfg->name == NULL || strlen(ept_cfg->name) == 0 ||
	    strlen(ept_cfg->name) > IPC_LINK_EPT_NAME_MAX) {
		return -EINVAL;
	}

	/* The bind message carries the name, in one frame */
	if (IPC_LINK_CTRL_HDR_SIZE + strlen(ept_cfg->name) >
	    ipc_link_core_max_msg_len(&link->core)) {
		return -EMSGSIZE;
	}

	if (!link->opened) {
		return -ENETDOWN;
	}

	key = k_spin_lock(&link->lock);

	for (size_t i = 0; i < ARRAY_SIZE(link->epts); i++) {
		if (link->epts[i].cfg != NULL &&
		    strcmp(link->epts[i].cfg->name, ept_cfg->name) == 0) {
			k_spin_unlock(&link->lock, key);
			return -EEXIST;
		}

		/* A slot whose unbind is on its way, or not acknowledged yet, isn't free: the
		 * peer may still send to its previous occupant
		 */
		if (ept == NULL && link->epts[i].cfg == NULL && link->epts[i].ctrl == 0U &&
		    !link->epts[i].retired) {
			ept = &link->epts[i];
		}
	}

	if (ept == NULL) {
		k_spin_unlock(&link->lock, key);
		return -ENOMEM;
	}

	ept->cfg = ept_cfg;
	ept->remote_addr = 0U;
	ept->was_bound = false;

	if (ipc_link_core_connected(&link->core)) {
		ept->ctrl = IPC_LINK_EPT_BIND;
	}

	pending = ipc_link_pending_locked(link);

	/* A bind already queued may call bound() as soon as the lock is released */
	*token = ept;

	k_spin_unlock(&link->lock, key);

	if (pending) {
		ipc_link_kick(dev);
	}

	return 0;
}

static int ipc_link_deregister_endpoint(const struct device *dev, void *token)
{
	struct ipc_link *link = ipc_link_get(dev);
	struct ipc_link_ept *ept = token;
	k_spinlock_key_t key;
	bool pending = false;

	if (ept == NULL) {
		return -EINVAL;
	}

	key = k_spin_lock(&link->lock);

	if (ept->cfg == NULL) {
		k_spin_unlock(&link->lock, key);
		return -ENOENT;
	}

	/* The peer may have bound it, or bind it from an announcement on its way: the unbind goes
	 * out with the address alone, and the slot stays taken until the peer acknowledges it
	 */
	ept->ctrl = ipc_link_core_connected(&link->core) ? IPC_LINK_EPT_UNBIND : 0U;
	ept->retired = ept->ctrl != 0U;
	ept->remote_addr = 0U;
	ept->was_bound = false;
	ept->cfg = NULL;

	if (ept->ctrl != 0U) {
		pending = ipc_link_pending_locked(link);
	}

	k_spin_unlock(&link->lock, key);

	if (pending) {
		ipc_link_kick(dev);
	}

	return 0;
}

static int ipc_link_send(const struct device *dev, void *token, const void *msg, size_t len)
{
	struct ipc_link *link = ipc_link_get(dev);
	struct ipc_link_ept *ept = token;
	k_spinlock_key_t key;
	int ret;

	if (ept == NULL || msg == NULL) {
		return -EINVAL;
	}

	if (len > UINT16_MAX) {
		return -EMSGSIZE;
	}

	key = k_spin_lock(&link->lock);

	if (ept->cfg == NULL) {
		ret = -ENOENT;
	} else if (ept->remote_addr == 0U) {
		ret = -EBUSY;
	} else {
		/* The peer only takes data for an endpoint once it has the reply */
		ret = 0;
		if ((ept->ctrl & IPC_LINK_EPT_REPLY) != 0U) {
			ret = ipc_link_send_ctrl(link, ept, IPC_LINK_CTRL_BIND_ACK);
			if (ret == 0) {
				ept->ctrl &= ~IPC_LINK_EPT_REPLY;
			}
		}

		if (ret == 0) {
			ret = ipc_link_core_send(&link->core, ept->remote_addr, msg, (uint16_t)len);
		}
	}

	k_spin_unlock(&link->lock, key);

	if (ret == -EAGAIN) {
		/* Room comes with the peer's acknowledgment */
		return -ENOMEM;
	}

	if (ret < 0) {
		return ret;
	}

	ipc_link_kick(dev);

	return (int)len;
}

static int ipc_link_get_tx_buffer_size(const struct device *dev, void *token)
{
	struct ipc_link *link = ipc_link_get(dev);

	ARG_UNUSED(token);

	return ipc_link_core_max_msg_len(&link->core);
}

static int ipc_link_hold_rx_buffer(const struct device *dev, void *token, void *data)
{
	struct ipc_link *link = ipc_link_get(dev);
	k_spinlock_key_t key;
	int idx;

	ARG_UNUSED(token);

	idx = ipc_link_core_rx_idx(&link->core, data);
	if (idx < 0) {
		return -EINVAL;
	}

	key = k_spin_lock(&link->lock);

	/* Only while being delivered or already held */
	if (((link->rx_delivering & BIT(idx)) == 0U && link->rx_holds[idx] == 0U) ||
	    link->rx_holds[idx] == UINT16_MAX) {
		k_spin_unlock(&link->lock, key);
		return -EINVAL;
	}

	link->rx_holds[idx]++;

	k_spin_unlock(&link->lock, key);

	return 0;
}

static int ipc_link_release_rx_buffer(const struct device *dev, void *token, void *data)
{
	struct ipc_link *link = ipc_link_get(dev);
	k_spinlock_key_t key;
	bool pending;
	int idx;

	ARG_UNUSED(token);

	idx = ipc_link_core_rx_idx(&link->core, data);
	if (idx < 0) {
		return -EINVAL;
	}

	key = k_spin_lock(&link->lock);

	/* A buffer only delivered is not the application's to release: the delivery is */
	if (link->rx_holds[idx] == 0U) {
		k_spin_unlock(&link->lock, key);
		return -EALREADY;
	}

	link->rx_holds[idx]--;
	pending = ipc_link_rx_free(link, (uint8_t)idx);

	k_spin_unlock(&link->lock, key);

	if (pending) {
		ipc_link_kick(dev);
	}

	return 0;
}

const struct ipc_service_backend ipc_link_backend_ops = {
	.open_instance = ipc_link_open_instance,
	.close_instance = ipc_link_close_instance,
	.send = ipc_link_send,
	.register_endpoint = ipc_link_register_endpoint,
	.deregister_endpoint = ipc_link_deregister_endpoint,
	.get_tx_buffer_size = ipc_link_get_tx_buffer_size,
	.hold_rx_buffer = ipc_link_hold_rx_buffer,
	.release_rx_buffer = ipc_link_release_rx_buffer,
};

int ipc_link_init(const struct device *dev)
{
	const struct ipc_link_config *cfg = dev->config;
	struct ipc_link *link = cfg->link;
	struct ipc_link_core_cfg core_cfg = {
		.tx_bufs = cfg->tx_bufs,
		.rx_bufs = cfg->rx_bufs,
		.mtu = cfg->mtu,
		.rx_count = IPC_LINK_RX_BUFFERS,
		.window = IPC_LINK_WINDOW,
		.retx_timeout = cfg->retransmit_timeout_ms,
		.max_retx = CONFIG_IPC_SERVICE_BACKEND_LINK_MAX_RETRANSMITS,
	};

	if (cfg->mtu < IPC_LINK_MTU_MIN || cfg->mtu % IPC_LINK_MSG_ALIGN != 0U ||
	    cfg->retransmit_timeout_ms == 0U) {
		return -EINVAL;
	}

	link->dev = dev;
	k_msgq_init(&link->rxq, link->rxq_buf, sizeof(struct ipc_link_rx_item),
		    sizeof(link->rxq_buf) / sizeof(struct ipc_link_rx_item));
	k_work_init(&link->rx_work, ipc_link_rx_work_handler);
	k_work_init_delayable(&link->timer, ipc_link_timer_handler);
	ipc_link_core_init(&link->core, &core_cfg);

	return 0;
}
