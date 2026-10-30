/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Basalte bv
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/math_extras.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/zassert.h>

#include "ipc_link_core.h"

ZASSERT_MODULE(IPC_SERVICE_LINK);

#define IPC_LINK_HDR_VER     0
#define IPC_LINK_HDR_FLAGS   1
#define IPC_LINK_HDR_SEQ     2
#define IPC_LINK_HDR_ACK     3
#define IPC_LINK_HDR_CREDITS 4
#define IPC_LINK_HDR_RSVD    5
#define IPC_LINK_HDR_LEN     6
#define IPC_LINK_HDR_MTU     8
#define IPC_LINK_HDR_CRC     10

#define IPC_LINK_MSG_ADDR  0
#define IPC_LINK_MSG_FLAGS 1
#define IPC_LINK_MSG_LEN   2

#define IPC_LINK_CRC_SEED 0xffffU

#ifdef CONFIG_IPC_SERVICE_LINK_STATS
#define IPC_LINK_STAT_ADD(core, name, n) ((core)->stats.name += (n))
#else
#define IPC_LINK_STAT_ADD(core, name, n) ARG_UNUSED(n)
#endif
#define IPC_LINK_STAT_INC(core, name) IPC_LINK_STAT_ADD(core, name, 1U)

static bool ipc_link_state(const struct ipc_link_core *core, uint8_t state)
{
	return (core->state & state) != 0U;
}

static uint8_t ipc_link_slot_of(const struct ipc_link_core *core, uint8_t seq)
{
	return (core->base_slot + (uint8_t)(seq - core->base)) %
	       IPC_LINK_TX_SLOTS(core->cfg.window);
}

static uint8_t *ipc_link_tx_buf_of(const struct ipc_link_core *core, uint8_t slot)
{
	return &core->cfg.tx_bufs[slot * core->cfg.mtu];
}

static uint8_t *ipc_link_rx_buf_of(const struct ipc_link_core *core, uint8_t idx)
{
	return &core->cfg.rx_bufs[idx * core->cfg.mtu];
}

static uint16_t ipc_link_payload_max(const struct ipc_link_core *core)
{
	return IPC_LINK_PAYLOAD_MAX(core->cfg.mtu);
}

static uint8_t ipc_link_credits(const struct ipc_link_core *core)
{
	return (uint8_t)POPCOUNT(core->rx_free);
}

static uint8_t ipc_link_in_flight(const struct ipc_link_core *core)
{
	return (uint8_t)(core->next - core->base);
}

static uint8_t ipc_link_committed(const struct ipc_link_core *core)
{
	return (uint8_t)(core->head - core->base);
}

static void ipc_link_reset_session(struct ipc_link_core *core)
{
	core->base = 0U;
	core->next = 0U;
	core->head = 0U;
	core->base_slot = 0U;
	core->fill_len = 0U;
	core->fill_crc = IPC_LINK_CRC_SEED;
	core->peer_credits = 0U;
	core->retx_count = 0U;
	core->send_timed = false;
	core->expected = 0U;
}

/* The session ends, however it ends: what the caller still holds of it belongs to the past */
static void ipc_link_end_session(struct ipc_link_core *core)
{
	ipc_link_reset_session(core);
	core->session++;
}

void ipc_link_core_init(struct ipc_link_core *core, const struct ipc_link_core_cfg *cfg)
{
	ZASSERT(cfg->rx_count >= 2U && cfg->rx_count <= IPC_LINK_RX_BUFS_MAX, "%u receive buffers",
		cfg->rx_count);
	ZASSERT(cfg->window >= 1U && cfg->window <= IPC_LINK_WINDOW_MAX, "window of %u",
		cfg->window);
	ZASSERT(cfg->mtu >= IPC_LINK_MTU_MIN && cfg->mtu % IPC_LINK_MSG_ALIGN == 0U,
		"frame size of %u", cfg->mtu);

	memset(core, 0, sizeof(*core));

	core->cfg = *cfg;

	core->fill_crc = IPC_LINK_CRC_SEED;
	core->rx_armed = 0U;
	core->rx_free = BIT_MASK(cfg->rx_count) & ~BIT(0);
	core->state = IPC_LINK_STATE_BOOTING;
}

static bool ipc_link_can_send(const struct ipc_link_core *core)
{
	if (ipc_link_state(core, IPC_LINK_STATE_BOOTING) ||
	    core->peer_credits <= ipc_link_in_flight(core)) {
		return false;
	}

	if (core->next != core->head) {
		return true;
	}

	return core->fill_len > 0U && ipc_link_committed(core) < core->cfg.window;
}

static bool ipc_link_boot_due(const struct ipc_link_core *core, uint32_t now)
{
	return ipc_link_state(core, IPC_LINK_STATE_BOOTING) &&
	       (!ipc_link_state(core, IPC_LINK_STATE_BOOT_SENT) ||
		now - core->boot_sent_at >= core->cfg.retx_timeout);
}

bool ipc_link_core_pending(const struct ipc_link_core *core, uint32_t now)
{
	/* A lost header may have carried the only update of the peer's credits */
	if (core->owed != 0U) {
		return true;
	}

	if (ipc_link_boot_due(core, now)) {
		return true;
	}

	return ipc_link_can_send(core);
}

static void ipc_link_commit(struct ipc_link_core *core)
{
	uint8_t slot = ipc_link_slot_of(core, core->head);
	uint8_t *buf = ipc_link_tx_buf_of(core, slot);

	sys_put_le16(core->fill_crc, &buf[IPC_LINK_HDR_SIZE + core->fill_len]);

	core->tx_len[slot] = core->fill_len;
	core->slot_fresh |= BIT(slot);
	core->head++;
	core->fill_len = 0U;
	core->fill_crc = IPC_LINK_CRC_SEED;
}

/* The oldest payload was sent, and waits for its acknowledgment or to be sent again */
static bool ipc_link_waiting_ack(const struct ipc_link_core *core)
{
	return ipc_link_committed(core) > 0U &&
	       (core->slot_fresh & BIT(ipc_link_slot_of(core, core->base))) == 0U;
}

static bool ipc_link_lost(const struct ipc_link_core *core, uint32_t now)
{
	return ipc_link_waiting_ack(core) &&
	       now - core->sent_at[ipc_link_slot_of(core, core->base)] >= core->cfg.retx_timeout;
}

/* Payloads wait for the peer's credits, with none in flight */
static bool ipc_link_blocked(const struct ipc_link_core *core)
{
	return ipc_link_state(core, IPC_LINK_STATE_CONNECTED) &&
	       !ipc_link_state(core, IPC_LINK_STATE_BOOTING) && core->next == core->base &&
	       (core->next != core->head || core->fill_len > 0U) && core->peer_credits == 0U;
}

/*
 * A timeout without an answer from the peer, while a payload waits for its acknowledgment or
 * for credits. Every one counts, whether the transport could send again in between or not: a
 * peer that takes no frame is as gone as one that answers none. Any valid header from the peer
 * starts the count over.
 */
static uint32_t ipc_link_timed_out(struct ipc_link_core *core)
{
	if (core->cfg.max_retx > 0U && core->retx_count >= core->cfg.max_retx) {
		ipc_link_end_session(core);
		core->state = IPC_LINK_STATE_BOOTING;
		core->owed &= ~IPC_LINK_OWED_BOOT_ACK;
		IPC_LINK_STAT_INC(core, disconnects);

		return IPC_LINK_CORE_EVT_DISCONNECTED;
	}

	core->retx_count++;

	return 0U;
}

/* A payload found lost is sent again with the ones after it */
static uint32_t ipc_link_go_back(struct ipc_link_core *core, uint32_t now)
{
	uint32_t evt = ipc_link_timed_out(core);

	if (evt != 0U) {
		return evt;
	}

	IPC_LINK_STAT_ADD(core, retransmits, ipc_link_in_flight(core));

	core->next = core->base;
	core->sent_at[ipc_link_slot_of(core, core->base)] = now;

	return 0U;
}

const uint8_t *ipc_link_core_tx_prepare(struct ipc_link_core *core, uint32_t now, size_t *len)
{
	uint8_t *buf = ipc_link_tx_buf_of(core, IPC_LINK_TX_SLOTS(core->cfg.window));
	uint16_t payload_len = 0U;
	uint8_t seq = 0U;
	uint8_t flags = 0U;

	/* The transport sends: what waited to be sent no longer does */
	core->send_timed = false;

	if (ipc_link_can_send(core)) {
		uint8_t slot;

		if (core->next == core->head) {
			ipc_link_commit(core);
		}

		seq = core->next++;
		slot = ipc_link_slot_of(core, seq);
		buf = ipc_link_tx_buf_of(core, slot);
		payload_len = core->tx_len[slot];

		if ((core->slot_fresh & BIT(slot)) != 0U) {
			IPC_LINK_STAT_INC(core, tx_payloads);
			core->slot_fresh &= ~BIT(slot);
		}

		core->sent_at[slot] = now;
	}

	if (ipc_link_state(core, IPC_LINK_STATE_BOOTING)) {
		flags |= IPC_LINK_FLAG_BOOT;
		core->state |= IPC_LINK_STATE_BOOT_SENT;
		core->boot_sent_at = now;
	}

	if ((core->owed & IPC_LINK_OWED_BOOT_ACK) != 0U) {
		flags |= IPC_LINK_FLAG_BOOT_ACK;
	}

	if ((core->owed & IPC_LINK_OWED_HDR_ERR) != 0U) {
		flags |= IPC_LINK_FLAG_HDR_ERR;
	}

	if ((core->owed & IPC_LINK_OWED_POLL) != 0U) {
		flags |= IPC_LINK_FLAG_POLL;
	}

	core->owed = 0U;
	core->credits_sent = ipc_link_credits(core);

	buf[IPC_LINK_HDR_VER] = IPC_LINK_VERSION;
	buf[IPC_LINK_HDR_FLAGS] = flags;
	buf[IPC_LINK_HDR_SEQ] = seq;
	buf[IPC_LINK_HDR_ACK] = core->expected;
	buf[IPC_LINK_HDR_CREDITS] = core->credits_sent;
	buf[IPC_LINK_HDR_RSVD] = 0U;
	sys_put_le16(payload_len, &buf[IPC_LINK_HDR_LEN]);
	sys_put_le16(core->cfg.mtu, &buf[IPC_LINK_HDR_MTU]);
	sys_put_le16(crc16_ccitt(IPC_LINK_CRC_SEED, buf, IPC_LINK_HDR_CRC), &buf[IPC_LINK_HDR_CRC]);

	*len = IPC_LINK_HDR_SIZE + (payload_len > 0U ? payload_len + IPC_LINK_PCRC_SIZE : 0U);

	return buf;
}

uint8_t *ipc_link_core_rx_buf(struct ipc_link_core *core)
{
	return ipc_link_rx_buf_of(core, core->rx_armed);
}

static bool ipc_link_msg_next(const uint8_t *payload, uint16_t len, size_t *offset,
			      struct ipc_link_msg *msg)
{
	uint16_t msg_len;

	if (*offset + IPC_LINK_MSG_HDR_SIZE > len) {
		return false;
	}

	msg_len = sys_get_le16(&payload[*offset + IPC_LINK_MSG_LEN]);
	if (*offset + IPC_LINK_MSG_HDR_SIZE + msg_len > len) {
		return false;
	}

	msg->addr = payload[*offset + IPC_LINK_MSG_ADDR];
	msg->len = msg_len;
	msg->data = &payload[*offset + IPC_LINK_MSG_HDR_SIZE];

	*offset += ROUND_UP(IPC_LINK_MSG_HDR_SIZE + msg_len, IPC_LINK_MSG_ALIGN);

	return true;
}

static bool ipc_link_payload_layout_valid(const uint8_t *payload, uint16_t len)
{
	struct ipc_link_msg msg;
	size_t offset = 0U;

	while (ipc_link_msg_next(payload, len, &offset, &msg)) {
	}

	return offset == len;
}

uint8_t ipc_link_core_rx_check(const struct ipc_link_core *core, size_t len, bool ok)
{
	const uint8_t *buf = ipc_link_rx_buf_of(core, core->rx_armed);
	const uint8_t *payload = &buf[IPC_LINK_HDR_SIZE];
	uint8_t checked = 0U;
	uint16_t payload_len;

	if (!ok || len < IPC_LINK_HDR_SIZE || len > core->cfg.mtu ||
	    crc16_ccitt(IPC_LINK_CRC_SEED, buf, IPC_LINK_HDR_CRC) !=
		    sys_get_le16(&buf[IPC_LINK_HDR_CRC])) {
		return checked;
	}

	checked |= IPC_LINK_RX_HDR_OK;

	payload_len = sys_get_le16(&buf[IPC_LINK_HDR_LEN]);
	if (payload_len == 0U) {
		return checked;
	}

	if (payload_len > ipc_link_payload_max(core) ||
	    IPC_LINK_HDR_SIZE + payload_len + IPC_LINK_PCRC_SIZE > len ||
	    crc16_ccitt(IPC_LINK_CRC_SEED, payload, payload_len) !=
		    sys_get_le16(&payload[payload_len])) {
		return checked;
	}

	checked |= IPC_LINK_RX_PAYLOAD_OK;

	if (ipc_link_payload_layout_valid(payload, payload_len)) {
		checked |= IPC_LINK_RX_LAYOUT_OK;
	}

	return checked;
}

static uint32_t ipc_link_process_ack(struct ipc_link_core *core, const uint8_t *buf, uint32_t now)
{
	uint8_t acked = buf[IPC_LINK_HDR_ACK] - core->base;
	uint32_t evt = 0U;

	/* Only payloads that were sent can be acknowledged: after going back, that is more than
	 * the ones in flight. Beyond the committed ones it is from before a session reset.
	 */
	if (acked > ipc_link_committed(core)) {
		return 0U;
	}

	if (acked > 0U) {
		if (acked > ipc_link_in_flight(core)) {
			core->next = buf[IPC_LINK_HDR_ACK];
		}

		core->base_slot = (core->base_slot + acked) % IPC_LINK_TX_SLOTS(core->cfg.window);
		core->base = buf[IPC_LINK_HDR_ACK];
	}

	/* The peer built this header after it had our payload for a while without acknowledging
	 * it: lost, send it and the ones after it again
	 */
	if (ipc_link_lost(core, now)) {
		evt |= ipc_link_go_back(core, now);
	}

	return evt;
}

static uint32_t ipc_link_process_payload(struct ipc_link_core *core, const uint8_t *buf,
					 uint8_t checked, uint8_t *rx_idx)
{
	if (sys_get_le16(&buf[IPC_LINK_HDR_LEN]) == 0U) {
		return 0U;
	}

	if ((checked & IPC_LINK_RX_PAYLOAD_OK) == 0U) {
		IPC_LINK_STAT_INC(core, payload_errors);
		return 0U;
	}

	/* Any payload asks for an acknowledgment, also a repeated one whose acknowledgment got
	 * lost
	 */
	core->owed |= IPC_LINK_OWED_ACK;

	if (buf[IPC_LINK_HDR_SEQ] != core->expected || core->rx_free == 0U) {
		IPC_LINK_STAT_INC(core, rx_dropped);
		return 0U;
	}

	core->expected++;

	/* Sending it again won't fix it, acknowledge and drop it */
	if ((checked & IPC_LINK_RX_LAYOUT_OK) == 0U) {
		IPC_LINK_STAT_INC(core, payload_errors);
		return 0U;
	}

	IPC_LINK_STAT_INC(core, rx_payloads);

	*rx_idx = core->rx_armed;
	core->rx_armed = (uint8_t)u32_count_trailing_zeros(core->rx_free);
	core->rx_free &= ~BIT(core->rx_armed);

	return IPC_LINK_CORE_EVT_RX;
}

uint32_t ipc_link_core_rx(struct ipc_link_core *core, uint8_t checked, uint32_t now,
			  uint8_t *rx_idx)
{
	const uint8_t *buf = ipc_link_rx_buf_of(core, core->rx_armed);
	uint32_t evt = 0U;
	bool peer_boot;

	IPC_LINK_STAT_INC(core, rx_frames);

	if ((checked & IPC_LINK_RX_HDR_OK) == 0U) {
		IPC_LINK_STAT_INC(core, header_errors);
		core->owed |= IPC_LINK_OWED_HDR_ERR;

		return IPC_LINK_CORE_EVT_HDR_ERR;
	}

	/* A sound header from a peer that can't talk to us: retrying won't help. A peer that
	 * came back that way ends the session it was in, as one that fell silent.
	 */
	if (buf[IPC_LINK_HDR_VER] != IPC_LINK_VERSION ||
	    sys_get_le16(&buf[IPC_LINK_HDR_MTU]) != core->cfg.mtu) {
		IPC_LINK_STAT_INC(core, peer_mismatches);

		if (ipc_link_state(core, IPC_LINK_STATE_CONNECTED)) {
			ipc_link_end_session(core);
			core->state = IPC_LINK_STATE_BOOTING;
			core->owed = 0U;
			IPC_LINK_STAT_INC(core, disconnects);
			evt |= IPC_LINK_CORE_EVT_DISCONNECTED;
		}

		return evt | IPC_LINK_CORE_EVT_PEER_MISMATCH;
	}

	/* The peer answers */
	core->retx_count = 0U;

	peer_boot = (buf[IPC_LINK_HDR_FLAGS] & IPC_LINK_FLAG_BOOT) != 0U;
	if (peer_boot && !ipc_link_state(core, IPC_LINK_STATE_PEER_BOOTING)) {
		if (ipc_link_state(core, IPC_LINK_STATE_CONNECTED)) {
			IPC_LINK_STAT_INC(core, peer_resets);
		}

		ipc_link_end_session(core);
		core->state |= IPC_LINK_STATE_CONNECTED;
		evt |= IPC_LINK_CORE_EVT_CONNECTED;
	}

	if (peer_boot) {
		core->state |= IPC_LINK_STATE_PEER_BOOTING;
		/* Once per announcement: the peer repeats it until it has the acknowledgment */
		core->owed |= IPC_LINK_OWED_BOOT_ACK;
	} else {
		core->state &= ~IPC_LINK_STATE_PEER_BOOTING;
	}

	if (ipc_link_state(core, IPC_LINK_STATE_BOOTING)) {
		/* Until the peer acknowledges the boot, its state belongs to an earlier session */
		if ((buf[IPC_LINK_HDR_FLAGS] & IPC_LINK_FLAG_BOOT_ACK) == 0U) {
			return evt;
		}

		core->state &= ~IPC_LINK_STATE_BOOTING;

		if (!ipc_link_state(core, IPC_LINK_STATE_CONNECTED)) {
			core->state |= IPC_LINK_STATE_CONNECTED;
			core->session++;
			evt |= IPC_LINK_CORE_EVT_CONNECTED;
		}
	}

	/* The peer missed our last header or waits for our credits: give it a header */
	if ((buf[IPC_LINK_HDR_FLAGS] & (IPC_LINK_FLAG_HDR_ERR | IPC_LINK_FLAG_POLL)) != 0U) {
		core->owed |= IPC_LINK_OWED_ACK;
	}

	evt |= ipc_link_process_ack(core, buf, now);
	if ((evt & IPC_LINK_CORE_EVT_DISCONNECTED) != 0U) {
		/* The rest of the frame belongs to the session that just ended */
		return evt;
	}

	core->peer_credits = buf[IPC_LINK_HDR_CREDITS];
	/* Fresh credits, don't ask for them before a retransmission timeout */
	core->poll_sent_at = now;
	evt |= ipc_link_process_payload(core, buf, checked, rx_idx);

	return evt;
}

/* Time until the timeout of something that happened at since */
static uint32_t ipc_link_remaining(const struct ipc_link_core *core, uint32_t since, uint32_t now)
{
	uint32_t elapsed = now - since;

	return elapsed >= core->cfg.retx_timeout ? 0U : core->cfg.retx_timeout - elapsed;
}

uint32_t ipc_link_core_timeout(struct ipc_link_core *core, uint32_t now)
{
	uint32_t timeout = UINT32_MAX;

	/* A payload the transport does not send is as unanswered as one the peer does not take */
	if (ipc_link_can_send(core)) {
		if (!core->send_timed) {
			core->send_timed = true;
			core->send_since = now;
		}

		timeout = ipc_link_remaining(core, core->send_since, now);
	}

	/* An announcement due is pending, not timed: the transport sends it when it can */
	if (ipc_link_state(core, IPC_LINK_STATE_BOOTING) &&
	    ipc_link_state(core, IPC_LINK_STATE_BOOT_SENT) && !ipc_link_boot_due(core, now)) {
		timeout = ipc_link_remaining(core, core->boot_sent_at, now);
	}

	if (ipc_link_waiting_ack(core)) {
		timeout =
			MIN(timeout,
			    ipc_link_remaining(
				    core, core->sent_at[ipc_link_slot_of(core, core->base)], now));
	}

	if (ipc_link_blocked(core)) {
		timeout = MIN(timeout, ipc_link_remaining(core, core->poll_sent_at, now));
	}

	return timeout;
}

uint32_t ipc_link_core_check(struct ipc_link_core *core, uint32_t now)
{
	uint32_t evt = 0U;

	if (ipc_link_lost(core, now)) {
		return ipc_link_go_back(core, now);
	}

	/* The header returning the credits may have been lost: ask for one, up to the same limit
	 * as a payload sent again
	 */
	if (ipc_link_blocked(core) && now - core->poll_sent_at >= core->cfg.retx_timeout) {
		evt = ipc_link_timed_out(core);
		if (evt == 0U) {
			core->owed |= IPC_LINK_OWED_POLL;
			core->poll_sent_at = now;
		}

		return evt;
	}

	/* A payload waiting for the transport, which cannot send: a peer that takes no frame */
	if (ipc_link_can_send(core) && core->send_timed &&
	    now - core->send_since >= core->cfg.retx_timeout) {
		evt = ipc_link_timed_out(core);
		core->send_since = now;
	}

	return evt;
}

bool ipc_link_core_msg_next(const struct ipc_link_core *core, uint8_t rx_idx, size_t *offset,
			    struct ipc_link_msg *msg)
{
	const uint8_t *buf = ipc_link_rx_buf_of(core, rx_idx);

	return ipc_link_msg_next(&buf[IPC_LINK_HDR_SIZE], sys_get_le16(&buf[IPC_LINK_HDR_LEN]),
				 offset, msg);
}

int ipc_link_core_rx_idx(const struct ipc_link_core *core, const void *ptr)
{
	const uint8_t *p = ptr;

	if (p < core->cfg.rx_bufs || p >= core->cfg.rx_bufs + core->cfg.rx_count * core->cfg.mtu) {
		return -1;
	}

	return (int)((p - core->cfg.rx_bufs) / core->cfg.mtu);
}

void ipc_link_core_rx_release(struct ipc_link_core *core, uint8_t rx_idx)
{
	ZASSERT(rx_idx < core->cfg.rx_count && rx_idx != core->rx_armed, "receive buffer %u",
		rx_idx);

	core->rx_free |= BIT(rx_idx);

	/* The peer stops sending at zero credits, tell it there's room again */
	if (core->credits_sent == 0U) {
		core->owed |= IPC_LINK_OWED_CREDITS;
	}
}

uint16_t ipc_link_core_max_msg_len(const struct ipc_link_core *core)
{
	return IPC_LINK_MSG_LEN_MAX(core->cfg.mtu);
}

static uint16_t ipc_link_msg_size(uint16_t len)
{
	return ROUND_UP(IPC_LINK_MSG_HDR_SIZE + len, IPC_LINK_MSG_ALIGN);
}

bool ipc_link_core_has_room(const struct ipc_link_core *core, uint16_t len)
{
	return ipc_link_state(core, IPC_LINK_STATE_CONNECTED) &&
	       len <= ipc_link_core_max_msg_len(core) &&
	       (core->fill_len + ipc_link_msg_size(len) <= ipc_link_payload_max(core) ||
		ipc_link_committed(core) < core->cfg.window);
}

bool ipc_link_core_connected(const struct ipc_link_core *core)
{
	return ipc_link_state(core, IPC_LINK_STATE_CONNECTED);
}

bool ipc_link_core_booting(const struct ipc_link_core *core)
{
	return ipc_link_state(core, IPC_LINK_STATE_BOOTING);
}

int ipc_link_core_send(struct ipc_link_core *core, uint8_t addr, const void *data, uint16_t len)
{
	uint16_t size = ipc_link_msg_size(len);
	uint8_t *buf;

	if (!ipc_link_state(core, IPC_LINK_STATE_CONNECTED)) {
		return -ENOTCONN;
	}

	if (len > ipc_link_core_max_msg_len(core)) {
		return -EMSGSIZE;
	}

	if (core->fill_len + size > ipc_link_payload_max(core)) {
		if (ipc_link_committed(core) >= core->cfg.window) {
			return -EAGAIN;
		}

		ipc_link_commit(core);
	}

	buf = ipc_link_tx_buf_of(core, ipc_link_slot_of(core, core->head));
	buf += IPC_LINK_HDR_SIZE + core->fill_len;

	buf[IPC_LINK_MSG_ADDR] = addr;
	buf[IPC_LINK_MSG_FLAGS] = 0U;
	sys_put_le16(len, &buf[IPC_LINK_MSG_LEN]);
	if (len > 0U) {
		memcpy(&buf[IPC_LINK_MSG_HDR_SIZE], data, len);
	}
	/* The padding is covered by the CRC, keep it defined */
	memset(&buf[IPC_LINK_MSG_HDR_SIZE + len], 0, size - IPC_LINK_MSG_HDR_SIZE - len);

	/* The CRC grows with the payload: committing a frame costs nothing more */
	core->fill_crc = crc16_ccitt(core->fill_crc, buf, size);
	core->fill_len += size;

	return 0;
}
