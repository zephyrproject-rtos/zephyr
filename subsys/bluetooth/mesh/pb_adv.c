/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>
#include <string.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/mesh.h>
#include <zephyr/net/buf.h>
#include "host/testing.h"
#include "net.h"
#include "crypto.h"
#include "beacon.h"
#include "prov.h"

#include "common/bt_str.h"

#define LOG_LEVEL CONFIG_BT_MESH_PROV_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_mesh_pb_adv);

#define GPCF(gpc)           (gpc & 0x03)
#define GPC_START(last_seg) (((last_seg) << 2) | 0x00)
#define GPC_ACK             0x01
#define GPC_CONT(seg_id)    (((seg_id) << 2) | 0x02)
#define GPC_CTL(op)         (((op) << 2) | 0x03)

#define START_PAYLOAD_MAX 20
#define CONT_PAYLOAD_MAX  23
#define RX_BUFFER_MAX     65

#define START_LAST_SEG(gpc) (gpc >> 2)
#define CONT_SEG_INDEX(gpc) (gpc >> 2)

#define BEARER_CTL(gpc) (gpc >> 2)
#define LINK_OPEN       0x00
#define LINK_ACK        0x01
#define LINK_CLOSE      0x02

#define XACT_SEG_OFFSET(_seg) (20 + ((_seg - 1) * 23))
#define XACT_SEG_DATA(_seg) (&link.rx.buf->data[XACT_SEG_OFFSET(_seg)])
#define XACT_SEG_RECV(_seg) (link.rx.seg &= ~(1 << (_seg)))

#define XACT_ID_MAX  0x7f
#define XACT_ID_NVAL 0xff
#define SEG_NVAL     0xff

#define RETRANSMIT_TIMEOUT  K_MSEC(CONFIG_BT_MESH_PB_ADV_RETRANS_TIMEOUT)
#define BUF_TIMEOUT         K_MSEC(400)
#define CLOSING_TIMEOUT     3
#define TRANSACTION_TIMEOUT 30

/* Acked messages, will do retransmissions manually, taking acks into account:
 */
#define RETRANSMITS_RELIABLE   CONFIG_BT_MESH_PB_ADV_TRANS_PDU_RETRANSMIT_COUNT
/* PDU acks: */
#define RETRANSMITS_ACK        CONFIG_BT_MESH_PB_ADV_TRANS_ACK_RETRANSMIT_COUNT
/* Link close retransmits: */
#define RETRANSMITS_LINK_CLOSE CONFIG_BT_MESH_PB_ADV_LINK_CLOSE_RETRANSMIT_COUNT

enum {
	ADV_LINK_ACTIVE,    	/* Link has been opened */
	ADV_LINK_ACK_RECVD, 	/* Ack for link has been received */
	ADV_LINK_CLOSING,   	/* Link is closing down */
	ADV_LINK_INVALID,   	/* Error occurred during provisioning */
	ADV_ACK_PENDING,    	/* An acknowledgment is being sent */
	ADV_PROVISIONER,    	/* The link was opened as provisioner */

	ADV_NUM_FLAGS,
};

enum {
	DELAYABLE_ADV_CTX_IN_USE,
	DELAYABLE_ADV_CTX_RETRANSMIT,

	DELAYABLE_ADV_CTX_NUM_FLAGS,
};

struct delayable_adv_ctx {
	sys_snode_t node;
	struct bt_mesh_adv *adv;
	const struct bt_mesh_send_cb *cb;
	void *cb_data;

	ATOMIC_DEFINE(flags, DELAYABLE_ADV_CTX_NUM_FLAGS);
} delayable_advs_ctx[CONFIG_BT_MESH_PB_ADV_DELAYABLE_ADV_COUNT];

struct pb_adv {
	uint32_t id; /* Link ID */

	ATOMIC_DEFINE(flags, ADV_NUM_FLAGS);

	const struct prov_bearer_cb *cb;
	void *cb_data;

	struct {
		uint8_t id;       /* Most recent transaction ID */
		uint8_t seg;      /* Bit-field of unreceived segments */
		uint8_t last_seg; /* Last segment (to check length) */
		uint8_t fcs;      /* Expected FCS value */
		struct net_buf_simple *buf;
	} rx;

	struct {
		/* Start timestamp of the transaction */
		int64_t start;

		/* Transaction id */
		uint8_t id;

		/* Current ack id */
		uint8_t pending_ack;

		/* Transaction timeout in seconds */
		uint8_t timeout;

		/* Pending outgoing adv(s) */
		struct bt_mesh_adv *adv[3];

		prov_bearer_send_complete_t cb;

		void *cb_data;

		/* Retransmit timer */
		struct k_work_delayable retransmit;
	} tx;

	/* Protocol timeout */
	struct k_work_delayable prot_timer;

	/* Transmit random delay timer */
	struct k_work_delayable random_delay;

	/* List of delayed advertisements */
	sys_slist_t delayable_advs;
};

struct prov_rx {
	uint32_t link_id;
	uint8_t xact_id;
	uint8_t gpc;
};

NET_BUF_SIMPLE_DEFINE_STATIC(rx_buf, RX_BUFFER_MAX);

static struct pb_adv link = { .rx = { .buf = &rx_buf } };

static void gen_prov_ack_send(uint8_t xact_id);
static void link_open(struct prov_rx *rx, struct net_buf_simple *buf);
static void link_ack(struct prov_rx *rx, struct net_buf_simple *buf);
static void link_close(struct prov_rx *rx, struct net_buf_simple *buf);
static void prov_link_close(enum prov_bearer_link_status status);
static void close_link(enum prov_bearer_link_status status);
static void delayable_adv_ctx_clear(struct bt_mesh_adv *adv);
static bool delayable_adv_ctx_push(void);
static struct delayable_adv_ctx *delayable_adv_ctx_head_get(void);
static void delayable_adv_ctx_release(struct delayable_adv_ctx *ctx);

static void buf_sent(int err, void *user_data)
{
	enum prov_bearer_link_status reason = (enum prov_bearer_link_status)(int)user_data;

	if (atomic_test_and_clear_bit(link.flags, ADV_LINK_CLOSING)) {
		close_link(reason);
		return;
	}
}

static void buf_start(uint16_t duration, int err, void *user_data)
{
	if (err) {
		buf_sent(err, user_data);
	}
}

static struct bt_mesh_send_cb buf_sent_cb = {
	.start = buf_start,
	.end = buf_sent,
};

static uint8_t last_seg(uint16_t len)
{
	if (len <= START_PAYLOAD_MAX) {
		return 0;
	}

	len -= START_PAYLOAD_MAX;

	return 1 + (len / CONT_PAYLOAD_MAX);
}

static void free_segments(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(link.tx.adv); i++) {
		struct bt_mesh_adv *adv = link.tx.adv[i];

		if (!adv) {
			break;
		}

		link.tx.adv[i] = NULL;

		/* Terminate active adv */
		if (adv->ctx.busy == 0U) {
			bt_mesh_adv_terminate(adv);
		} else {
			/* Mark as canceled */
			adv->ctx.busy = 0U;
		}

		bt_mesh_adv_unref(adv);

		/* Active adv is terminated, remove it from the list. */
		delayable_adv_ctx_clear(adv);
	}
}

static uint8_t next_transaction_id(uint8_t id)
{
	return (((id + 1) & XACT_ID_MAX) | (id & (XACT_ID_MAX+1)));
}

static void prov_clear_tx(void)
{
	LOG_DBG("");

	/* If this fails, the work handler will not find any advs to send,
	 * and return without rescheduling. The work handler also checks the
	 * LINK_ACTIVE flag, so if this call is part of reset_adv_link, it'll
	 * exit early.
	 */
	(void)k_work_cancel_delayable(&link.tx.retransmit);

	free_segments();
}

static void reset_adv_link(void)
{
	struct delayable_adv_ctx *ctx;

	LOG_DBG("");

	/* Clear all pending delayable advs. */
	ctx = delayable_adv_ctx_head_get();
	while (ctx) {
		delayable_adv_ctx_release(ctx);
		ctx = delayable_adv_ctx_head_get();
	}

	prov_clear_tx();

	/* If this fails, the work handler will exit early on the LINK_ACTIVE
	 * check.
	 */
	(void)k_work_cancel_delayable(&link.prot_timer);
	(void)k_work_cancel_delayable(&link.random_delay);

	if (atomic_test_bit(link.flags, ADV_PROVISIONER)) {
		/* Clear everything except the random_delay, retransmit
		 * and protocol timer delayed work objects.
		 */
		(void)memset(&link, 0, offsetof(struct pb_adv, tx.retransmit));
		link.rx.id = XACT_ID_NVAL;
	} else {
		/* If provisioned, reset the link callback to stop receiving provisioning advs,
		 * otherwise keep the callback to accept another provisioning attempt.
		 */
		if (bt_mesh_is_provisioned()) {
			link.cb = NULL;
		}

		link.id = 0;
		atomic_clear(link.flags);
		link.rx.id = XACT_ID_MAX;
		link.tx.id = XACT_ID_NVAL;
	}

	link.tx.pending_ack = XACT_ID_NVAL;
	link.rx.buf = &rx_buf;
	net_buf_simple_reset(link.rx.buf);
}

static void close_link(enum prov_bearer_link_status reason)
{
	const struct prov_bearer_cb *cb = link.cb;
	void *cb_data = link.cb_data;

	reset_adv_link();
	cb->link_closed(&bt_mesh_pb_adv, cb_data, reason);
}

static struct bt_mesh_adv *adv_create(uint8_t retransmits)
{
	struct bt_mesh_adv *adv;

	adv = bt_mesh_adv_create(BT_MESH_ADV_PROV, BT_MESH_ADV_TAG_PROV,
				 BT_MESH_TRANSMIT(retransmits, 20),
				 BUF_TIMEOUT);
	if (!adv) {
		LOG_ERR("Out of provisioning advs");
		return NULL;
	}

	return adv;
}

static void ack_complete(uint16_t duration, int err, void *user_data)
{
	LOG_DBG("xact 0x%x complete", (uint8_t)link.tx.pending_ack);
	atomic_clear_bit(link.flags, ADV_ACK_PENDING);
}

static bool ack_pending(void)
{
	return atomic_test_bit(link.flags, ADV_ACK_PENDING);
}

static void prov_failed(uint8_t err)
{
	LOG_DBG("%u", err);
	link.cb->error(&bt_mesh_pb_adv, link.cb_data, err);
	atomic_set_bit(link.flags, ADV_LINK_INVALID);
}

static void prov_msg_recv(void)
{
	k_work_reschedule(&link.prot_timer, bt_mesh_prov_protocol_timeout_get());

	if (!bt_mesh_fcs_check(link.rx.buf, link.rx.fcs)) {
		LOG_ERR("Incorrect FCS");
		return;
	}

	gen_prov_ack_send(link.rx.id);

	if (atomic_test_bit(link.flags, ADV_LINK_INVALID)) {
		LOG_WRN("Unexpected msg 0x%02x on invalidated link", link.rx.buf->data[0]);
		prov_failed(PROV_ERR_UNEXP_PDU);
		return;
	}

	link.cb->recv(&bt_mesh_pb_adv, link.cb_data, link.rx.buf);
}

static void protocol_timeout(struct k_work *work)
{
	if (!atomic_test_bit(link.flags, ADV_LINK_ACTIVE)) {
		return;
	}

	LOG_DBG("");

	link.rx.seg = 0U;
	prov_link_close(PROV_BEARER_LINK_STATUS_TIMEOUT);
}

static struct delayable_adv_ctx *delayable_adv_ctx_get_unused(void)
{
	for (int i = 0; i < CONFIG_BT_MESH_PB_ADV_DELAYABLE_ADV_COUNT; i++) {
		if (!atomic_test_bit(delayable_advs_ctx[i].flags, DELAYABLE_ADV_CTX_IN_USE)) {
			return &delayable_advs_ctx[i];
		}
	}

	return NULL;
}

static struct delayable_adv_ctx *delayable_adv_ctx_assign(struct bt_mesh_adv *adv,
							  const struct bt_mesh_send_cb *cb,
							  void *cb_data, bool retransmit)
{
	struct delayable_adv_ctx *ctx;

	ctx = delayable_adv_ctx_get_unused();
	if (!ctx) {
		LOG_WRN("Purge pending delayed advertisement.");
		if (!delayable_adv_ctx_push()) {
			return NULL;
		}

		ctx = delayable_adv_ctx_get_unused();
		if (!ctx) {
			return NULL;
		}
	}

	atomic_set_bit(ctx->flags, DELAYABLE_ADV_CTX_IN_USE);
	ctx->adv = adv;
	ctx->cb = cb;
	ctx->cb_data = cb_data;
	if (retransmit) {
		atomic_set_bit(ctx->flags, DELAYABLE_ADV_CTX_RETRANSMIT);
	}

	return ctx;
}

static void delayable_adv_ctx_release(struct delayable_adv_ctx *ctx)
{
	if (!atomic_test_and_clear_bit(ctx->flags, DELAYABLE_ADV_CTX_RETRANSMIT)) {
		bt_mesh_adv_unref(ctx->adv);
	}

	(void)sys_slist_find_and_remove(&link.delayable_advs, &ctx->node);
	atomic_clear_bit(ctx->flags, DELAYABLE_ADV_CTX_IN_USE);

}

static struct delayable_adv_ctx *delayable_adv_ctx_head_get(void)
{
	sys_snode_t *node = sys_slist_peek_head(&link.delayable_advs);

	if (!node) {
		return NULL;
	}

	return CONTAINER_OF(node, struct delayable_adv_ctx, node);
}

static void delayable_adv_ctx_reschedule(void)
{
	uint16_t random_delay;
	k_timeout_t delay;

	if (sys_slist_is_empty(&link.delayable_advs)) {
		return;
	}

	(void)bt_rand(&random_delay, sizeof(random_delay));
	random_delay = 20 + (random_delay % 30);
	delay = K_MSEC(random_delay);

	LOG_DBG("delay %u", random_delay);

	k_work_schedule(&link.random_delay, delay);
}

static bool delayable_adv_ctx_push(void)
{
	struct delayable_adv_ctx *ctx;

	ctx = delayable_adv_ctx_head_get();

	if (!ctx) {
		return false;
	}

	LOG_DBG("%u bytes: %s", ctx->adv->b.len, bt_hex(ctx->adv->b.data, ctx->adv->b.len));
	bt_mesh_adv_send(ctx->adv, ctx->cb, ctx->cb_data);

	delayable_adv_ctx_release(ctx);
	return true;
}

static void delayable_adv_handler(struct k_work *work)
{
	if (delayable_adv_ctx_push()) {
		delayable_adv_ctx_reschedule();
	}
}

static void adv_delay_send(struct bt_mesh_adv *adv, const struct bt_mesh_send_cb *cb,
			   void *cb_data, bool retransmit)
{
	struct delayable_adv_ctx *ctx;

	ctx = delayable_adv_ctx_assign(adv, cb, cb_data, retransmit);

	if (!ctx) {
		LOG_WRN("Unable to allocate delayable advertiser context, sending immediately.");
		bt_mesh_adv_send(adv, cb, cb_data);
		return;
	}

	sys_slist_append(&link.delayable_advs, &ctx->node);
	delayable_adv_ctx_reschedule();
}

static void delayable_adv_ctx_clear(struct bt_mesh_adv *adv)
{
	struct delayable_adv_ctx *current;

	SYS_SLIST_FOR_EACH_CONTAINER(&link.delayable_advs, current, node) {
		if (current->adv == adv) {
			delayable_adv_ctx_release(current);
			return;
		}
	}
}

/*******************************************************************************
 * Generic provisioning
 ******************************************************************************/

static void gen_prov_ack_send(uint8_t xact_id)
{
	static const struct bt_mesh_send_cb cb = {
		.start = ack_complete,
	};
	const struct bt_mesh_send_cb *complete;
	struct bt_mesh_adv *adv;
	bool pending = atomic_test_and_set_bit(link.flags, ADV_ACK_PENDING);

	LOG_DBG("xact_id 0x%x", xact_id);

	if (pending && link.tx.pending_ack == xact_id) {
		LOG_DBG("Not sending duplicate ack");
		return;
	}

	adv = adv_create(RETRANSMITS_ACK);
	if (!adv) {
		atomic_clear_bit(link.flags, ADV_ACK_PENDING);
		return;
	}

	if (pending) {
		complete = NULL;
	} else {
		link.tx.pending_ack = xact_id;
		complete = &cb;
	}

	net_buf_simple_add_be32(&adv->b, link.id);
	net_buf_simple_add_u8(&adv->b, xact_id);
	net_buf_simple_add_u8(&adv->b, GPC_ACK);

	adv_delay_send(adv, complete, NULL, false);
}

static void gen_prov_cont(struct prov_rx *rx, struct net_buf_simple *buf)
{
	uint8_t seg = CONT_SEG_INDEX(rx->gpc);

	LOG_DBG("len %u, seg_index %u", buf->len, seg);

	if (!link.rx.seg && link.rx.id == rx->xact_id) {
		if (!ack_pending()) {
			LOG_DBG("Resending ack");
			gen_prov_ack_send(rx->xact_id);
		}

		return;
	} else if (rx->xact_id == next_transaction_id(link.rx.id) && link.tx.adv[0]) {
		LOG_WRN("xact 0x%x equal next transaction id 0x%x, but tx buf is not empty",
			 rx->xact_id, next_transaction_id(link.rx.id));
		return;
	}

	if (!link.rx.seg &&
	    next_transaction_id(link.rx.id) == rx->xact_id) {
		LOG_DBG("Start segment lost");

		link.rx.id = rx->xact_id;

		net_buf_simple_reset(link.rx.buf);

		link.rx.seg = SEG_NVAL;
		link.rx.last_seg = SEG_NVAL;

		prov_clear_tx();
	} else if (rx->xact_id != link.rx.id) {
		LOG_WRN("Data for unknown transaction (0x%x != 0x%x)", rx->xact_id, link.rx.id);
		return;
	}

	if (seg > link.rx.last_seg) {
		LOG_ERR("Invalid segment index %u", seg);
		prov_failed(PROV_ERR_NVAL_FMT);
		return;
	}

	if (!(link.rx.seg & BIT(seg))) {
		LOG_DBG("Ignoring already received segment");
		return;
	}

	if (XACT_SEG_OFFSET(seg) + buf->len > RX_BUFFER_MAX) {
		LOG_WRN("Rx buffer overflow. Malformed generic prov frame?");
		return;
	}

	memcpy(XACT_SEG_DATA(seg), buf->data, buf->len);
	XACT_SEG_RECV(seg);

	if (seg == link.rx.last_seg && !(link.rx.seg & BIT(0))) {
		uint8_t expect_len;

		expect_len = (link.rx.buf->len - 20U -
				((link.rx.last_seg - 1) * 23U));
		if (expect_len != buf->len) {
			LOG_ERR("Incorrect last seg len: %u != %u", expect_len, buf->len);
			prov_failed(PROV_ERR_NVAL_FMT);
			return;
		}
	}

	if (!link.rx.seg) {
		prov_msg_recv();
	}
}

static void gen_prov_ack(struct prov_rx *rx, struct net_buf_simple *buf)
{
	LOG_DBG("len %u", buf->len);

	if (!link.tx.adv[0]) {
		return;
	}

	if (rx->xact_id == link.tx.id) {
		/* Don't clear resending of link_close messages */
		if (!atomic_test_bit(link.flags, ADV_LINK_CLOSING)) {
			prov_clear_tx();
		}

		if (link.tx.cb) {
			link.tx.cb(0, link.tx.cb_data);
		}
	}
}

static void gen_prov_start(struct prov_rx *rx, struct net_buf_simple *buf)
{
	uint8_t seg = SEG_NVAL;

	if (rx->xact_id == link.rx.id) {
		if (!link.rx.seg) {
			if (!ack_pending()) {
				LOG_DBG("Resending ack");
				gen_prov_ack_send(rx->xact_id);
			}

			return;
		}

		if (!(link.rx.seg & BIT(0))) {
			LOG_DBG("Ignoring duplicate segment");
			return;
		}
	} else if (rx->xact_id != next_transaction_id(link.rx.id)) {
		LOG_WRN("Unexpected xact 0x%x, expected 0x%x", rx->xact_id,
			next_transaction_id(link.rx.id));
		return;
	} else if (rx->xact_id == next_transaction_id(link.rx.id) && link.tx.adv[0]) {
		LOG_WRN("xact 0x%x equal next transaction id 0x%x, but tx buf is not empty",
			 rx->xact_id, next_transaction_id(link.rx.id));
		return;
	}

	net_buf_simple_reset(link.rx.buf);
	link.rx.buf->len = net_buf_simple_pull_be16(buf);
	link.rx.id = rx->xact_id;
	link.rx.fcs = net_buf_simple_pull_u8(buf);

	LOG_DBG("len %u last_seg %u total_len %u fcs 0x%02x", buf->len, START_LAST_SEG(rx->gpc),
		link.rx.buf->len, link.rx.fcs);

	if (link.rx.buf->len < 1) {
		LOG_ERR("Ignoring zero-length provisioning PDU");
		prov_failed(PROV_ERR_NVAL_FMT);
		return;
	}

	if (link.rx.buf->len > link.rx.buf->size) {
		LOG_ERR("Too large provisioning PDU (%u bytes)", link.rx.buf->len);
		prov_failed(PROV_ERR_NVAL_FMT);
		return;
	}

	if (START_LAST_SEG(rx->gpc) > 0 && link.rx.buf->len <= 20U) {
		LOG_ERR("Too small total length for multi-segment PDU");
		prov_failed(PROV_ERR_NVAL_FMT);
		return;
	}

	if (START_LAST_SEG(rx->gpc) != last_seg(link.rx.buf->len)) {
		LOG_ERR("Invalid SegN (%u, calculated %u)", START_LAST_SEG(rx->gpc),
			last_seg(link.rx.buf->len));
		prov_failed(PROV_ERR_NVAL_FMT);
		return;
	}

	prov_clear_tx();

	link.rx.last_seg = START_LAST_SEG(rx->gpc);

	if ((link.rx.seg & BIT(0)) &&
	    (find_msb_set((~link.rx.seg) & SEG_NVAL) - 1 > link.rx.last_seg)) {
		LOG_ERR("Invalid segment index %u", seg);
		prov_failed(PROV_ERR_NVAL_FMT);
		return;
	}

	if (link.rx.seg) {
		seg = link.rx.seg;
	}

	link.rx.seg = seg & ((1 << (START_LAST_SEG(rx->gpc) + 1)) - 1);
	memcpy(link.rx.buf->data, buf->data, buf->len);
	XACT_SEG_RECV(0);

	if (!link.rx.seg) {
		prov_msg_recv();
	}
}

static void gen_prov_ctl(struct prov_rx *rx, struct net_buf_simple *buf)
{
	LOG_DBG("op 0x%02x len %u", BEARER_CTL(rx->gpc), buf->len);

	switch (BEARER_CTL(rx->gpc)) {
	case LINK_OPEN:
		link_open(rx, buf);
		break;
	case LINK_ACK:
		if (!atomic_test_bit(link.flags, ADV_LINK_ACTIVE)) {
			return;
		}

		link_ack(rx, buf);
		break;
	case LINK_CLOSE:
		if (!atomic_test_bit(link.flags, ADV_LINK_ACTIVE)) {
			return;
		}

		link_close(rx, buf);
		break;
	default:
		LOG_ERR("Unknown bearer opcode: 0x%02x", BEARER_CTL(rx->gpc));

		if (IS_ENABLED(CONFIG_BT_TESTING)) {
			bt_test_mesh_prov_invalid_bearer(BEARER_CTL(rx->gpc));
		}

		return;
	}
}

static const struct {
	void (*func)(struct prov_rx *rx, struct net_buf_simple *buf);
	bool require_link;
	uint8_t min_len;
} gen_prov[] = {
	{ gen_prov_start, true, 3 },
	{ gen_prov_ack, true, 0 },
	{ gen_prov_cont, true, 0 },
	{ gen_prov_ctl, false, 0 },
};

static void gen_prov_recv(struct prov_rx *rx, struct net_buf_simple *buf)
{
	if (buf->len < gen_prov[GPCF(rx->gpc)].min_len) {
		LOG_ERR("Too short GPC message type %u", GPCF(rx->gpc));
		return;
	}

	if (!atomic_test_bit(link.flags, ADV_LINK_ACTIVE) &&
	    gen_prov[GPCF(rx->gpc)].require_link) {
		LOG_DBG("Ignoring message that requires active link");
		return;
	}

	gen_prov[GPCF(rx->gpc)].func(rx, buf);
}

/*******************************************************************************
 * TX
 ******************************************************************************/

static void send_reliable(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(link.tx.adv); i++) {
		struct bt_mesh_adv *adv = link.tx.adv[i];

		if (!adv) {
			break;
		}

		if (adv->ctx.busy) {
			continue;
		}

		LOG_DBG("%u bytes: %s", adv->b.len, bt_hex(adv->b.data, adv->b.len));

		adv_delay_send(adv, NULL, NULL, true);
	}

	k_work_reschedule(&link.tx.retransmit, RETRANSMIT_TIMEOUT);
}

static void prov_retransmit(struct k_work *work)
{
	LOG_DBG("");

	if (!atomic_test_bit(link.flags, ADV_LINK_ACTIVE)) {
		LOG_WRN("Link not active");
		return;
	}

	if (k_uptime_get() - link.tx.start > link.tx.timeout * MSEC_PER_SEC) {
		LOG_WRN("Giving up transaction");
		prov_link_close(PROV_BEARER_LINK_STATUS_TIMEOUT);
		return;
	}

	send_reliable();
}

static struct bt_mesh_adv *ctl_adv_create(uint8_t op, const void *data, uint8_t data_len,
					  uint8_t retransmits)
{
	struct bt_mesh_adv *adv;

	LOG_DBG("op 0x%02x data_len %u", op, data_len);

	adv = adv_create(retransmits);
	if (!adv) {
		return NULL;
	}

	net_buf_simple_add_be32(&adv->b, link.id);
	/* Transaction ID, always 0 for Bearer messages */
	net_buf_simple_add_u8(&adv->b, 0x00);
	net_buf_simple_add_u8(&adv->b, GPC_CTL(op));
	net_buf_simple_add_mem(&adv->b, data, data_len);

	return adv;
}

static int bearer_ctl_send(struct bt_mesh_adv *adv)
{
	if (!adv) {
		return -ENOMEM;
	}

	prov_clear_tx();
	k_work_reschedule(&link.prot_timer, bt_mesh_prov_protocol_timeout_get());

	link.tx.start = k_uptime_get();
	link.tx.adv[0] = adv;
	send_reliable();

	return 0;
}

static int bearer_ctl_send_unacked(struct bt_mesh_adv *adv, void *user_data)
{
	if (!adv) {
		return -ENOMEM;
	}

	prov_clear_tx();
	k_work_reschedule(&link.prot_timer, bt_mesh_prov_protocol_timeout_get());

	adv_delay_send(adv, &buf_sent_cb, user_data, false);

	return 0;
}

static int prov_send_adv(struct net_buf_simple *msg,
			 prov_bearer_send_complete_t cb, void *cb_data)
{
	struct bt_mesh_adv *start, *adv;
	uint8_t seg_len, seg_id;

	prov_clear_tx();
	k_work_reschedule(&link.prot_timer, bt_mesh_prov_protocol_timeout_get());

	start = adv_create(RETRANSMITS_RELIABLE);
	if (!start) {
		return -ENOBUFS;
	}

	link.tx.id = next_transaction_id(link.tx.id);
	net_buf_simple_add_be32(&start->b, link.id);
	net_buf_simple_add_u8(&start->b, link.tx.id);

	net_buf_simple_add_u8(&start->b, GPC_START(last_seg(msg->len)));
	net_buf_simple_add_be16(&start->b, msg->len);
	net_buf_simple_add_u8(&start->b, bt_mesh_fcs_calc(msg->data, msg->len));

	link.tx.adv[0] = start;
	link.tx.cb = cb;
	link.tx.cb_data = cb_data;
	link.tx.start = k_uptime_get();

	LOG_DBG("xact_id: 0x%x len: %u", link.tx.id, msg->len);

	seg_len = MIN(msg->len, START_PAYLOAD_MAX);
	LOG_DBG("seg 0 len %u: %s", seg_len, bt_hex(msg->data, seg_len));
	net_buf_simple_add_mem(&start->b, msg->data, seg_len);
	net_buf_simple_pull(msg, seg_len);

	adv = start;
	for (seg_id = 1U; msg->len > 0; seg_id++) {
		if (seg_id >= ARRAY_SIZE(link.tx.adv)) {
			LOG_ERR("Too big message");
			free_segments();
			return -E2BIG;
		}

		adv = adv_create(RETRANSMITS_RELIABLE);
		if (!adv) {
			free_segments();
			return -ENOBUFS;
		}

		link.tx.adv[seg_id] = adv;

		seg_len = MIN(msg->len, CONT_PAYLOAD_MAX);

		LOG_DBG("seg %u len %u: %s", seg_id, seg_len, bt_hex(msg->data, seg_len));

		net_buf_simple_add_be32(&adv->b, link.id);
		net_buf_simple_add_u8(&adv->b, link.tx.id);
		net_buf_simple_add_u8(&adv->b, GPC_CONT(seg_id));
		net_buf_simple_add_mem(&adv->b, msg->data, seg_len);
		net_buf_simple_pull(msg, seg_len);
	}

	send_reliable();

	return 0;
}

/*******************************************************************************
 * Link management rx
 ******************************************************************************/

static void link_open(struct prov_rx *rx, struct net_buf_simple *buf)
{
	int err;

	LOG_DBG("len %u", buf->len);

	if (buf->len < 16) {
		LOG_ERR("Too short bearer open message (len %u)", buf->len);
		return;
	}

	if (atomic_test_bit(link.flags, ADV_LINK_ACTIVE)) {
		/* Send another link ack if the provisioner missed the last */
		if (link.id != rx->link_id) {
			LOG_DBG("Ignoring bearer open: link already active");
			return;
		}

		LOG_DBG("Resending link ack");
		/* Ignore errors, message will be attempted again if we keep receiving link open: */
		(void)bearer_ctl_send_unacked(
			ctl_adv_create(LINK_ACK, NULL, 0, RETRANSMITS_ACK),
			(void *)PROV_BEARER_LINK_STATUS_SUCCESS);
		return;
	}

	if (memcmp(buf->data, bt_mesh_prov_get()->uuid, 16)) {
		LOG_DBG("Bearer open message not for us");
		return;
	}

	link.id = rx->link_id;
	atomic_set_bit(link.flags, ADV_LINK_ACTIVE);
	net_buf_simple_reset(link.rx.buf);

	err = bearer_ctl_send_unacked(
		ctl_adv_create(LINK_ACK, NULL, 0, RETRANSMITS_ACK),
		(void *)PROV_BEARER_LINK_STATUS_SUCCESS);
	if (err) {
		reset_adv_link();
		return;
	}

	link.cb->link_opened(&bt_mesh_pb_adv, link.cb_data);
}

static void link_ack(struct prov_rx *rx, struct net_buf_simple *buf)
{
	LOG_DBG("len %u", buf->len);

	if (atomic_test_bit(link.flags, ADV_PROVISIONER)) {
		if (atomic_test_and_set_bit(link.flags, ADV_LINK_ACK_RECVD)) {
			return;
		}

		prov_clear_tx();

		link.tx.timeout = TRANSACTION_TIMEOUT;

		link.cb->link_opened(&bt_mesh_pb_adv, link.cb_data);
	}
}

static void link_close(struct prov_rx *rx, struct net_buf_simple *buf)
{
	LOG_DBG("len %u", buf->len);

	if (buf->len != 1) {
		return;
	}

	close_link(net_buf_simple_pull_u8(buf));
}

/*******************************************************************************
 * Higher level functionality
 ******************************************************************************/

void bt_mesh_pb_adv_recv(struct net_buf_simple *buf)
{
	struct prov_rx rx;

	if (!link.cb) {
		return;
	}

	if (buf->len < 6) {
		LOG_WRN("Too short provisioning packet (len %u)", buf->len);
		return;
	}

	rx.link_id = net_buf_simple_pull_be32(buf);
	rx.xact_id = net_buf_simple_pull_u8(buf);
	rx.gpc = net_buf_simple_pull_u8(buf);

	if (atomic_test_bit(link.flags, ADV_LINK_ACTIVE) && link.id != rx.link_id) {
		return;
	}

	LOG_DBG("link_id 0x%08x xact_id 0x%x", rx.link_id, rx.xact_id);

	gen_prov_recv(&rx, buf);
}

static int prov_link_open(const uint8_t uuid[16], uint8_t timeout,
			  const struct prov_bearer_cb *cb, void *cb_data)
{
	int err;

	LOG_DBG("uuid %s", bt_hex(uuid, 16));

	err = bt_mesh_adv_enable();
	if (err) {
		LOG_ERR("Failed enabling advertiser");
		return err;
	}

	if (atomic_test_and_set_bit(link.flags, ADV_LINK_ACTIVE)) {
		return -EBUSY;
	}

	atomic_set_bit(link.flags, ADV_PROVISIONER);

	bt_rand(&link.id, sizeof(link.id));
	link.tx.id = XACT_ID_MAX;
	link.rx.id = XACT_ID_NVAL;
	link.cb = cb;
	link.cb_data = cb_data;

	/* The link open time is configurable, but this will be changed to TRANSACTION_TIMEOUT once
	 * the link is established.
	 */
	link.tx.timeout = timeout;

	net_buf_simple_reset(link.rx.buf);

	return bearer_ctl_send(ctl_adv_create(LINK_OPEN, uuid, 16, RETRANSMITS_RELIABLE));
}

static int prov_link_accept(const struct prov_bearer_cb *cb, void *cb_data)
{
	int err;

	err = bt_mesh_adv_enable();
	if (err) {
		LOG_ERR("Failed enabling advertiser");
		return err;
	}

	if (atomic_test_bit(link.flags, ADV_LINK_ACTIVE)) {
		return -EBUSY;
	}

	link.rx.id = XACT_ID_MAX;
	link.tx.id = XACT_ID_NVAL;
	link.cb = cb;
	link.cb_data = cb_data;
	link.tx.timeout = TRANSACTION_TIMEOUT;

	/* Make sure we're scanning for provisioning invitations */
	bt_mesh_scan_enable();
	/* Enable unprovisioned beacon sending */
	bt_mesh_beacon_enable();

	return 0;
}

static void prov_link_close(enum prov_bearer_link_status status)
{
	if (atomic_test_and_set_bit(link.flags, ADV_LINK_CLOSING)) {
		return;
	}

	/*
	 * According to MshPRTv1.1: 5.3.1.4.3, the close message should
	 * be restransmitted at least three times. Retransmit the LINK_CLOSE
	 * message until CLOSING_TIMEOUT has elapsed.
	 */
	link.tx.timeout = CLOSING_TIMEOUT;
	/* Ignore errors, the link will time out eventually if this doesn't get sent */
	bearer_ctl_send_unacked(
		ctl_adv_create(LINK_CLOSE, &status, 1, RETRANSMITS_LINK_CLOSE),
		(void *)status);
}

void bt_mesh_pb_adv_init(void)
{
	k_work_init_delayable(&link.prot_timer, protocol_timeout);
	k_work_init_delayable(&link.random_delay, delayable_adv_handler);
	k_work_init_delayable(&link.tx.retransmit, prov_retransmit);
	sys_slist_init(&link.delayable_advs);
	for (int i = 0; i < CONFIG_BT_MESH_PB_ADV_DELAYABLE_ADV_COUNT; i++) {
		atomic_clear(delayable_advs_ctx[i].flags);
	}
}

void bt_mesh_pb_adv_reset(void)
{
	reset_adv_link();
}

const struct prov_bearer bt_mesh_pb_adv = {
	.type = BT_MESH_PROV_ADV,
	.link_open = prov_link_open,
	.link_accept = prov_link_accept,
	.link_close = prov_link_close,
	.send = prov_send_adv,
	.clear_tx = prov_clear_tx,
};
