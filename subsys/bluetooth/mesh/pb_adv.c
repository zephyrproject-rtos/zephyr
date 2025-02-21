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
#include <zephyr/net_buf.h>
#include "testing.h"
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
	ADV_LINK_ACK_SENDING,	/* Link Ack tx was scheduled but not finished. */
	ADV_SENDING,		/* Sending any PDU */

	ADV_NUM_FLAGS,
};

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

		/* Pending outgoing adv(s) (Link Open, Gen Trans Start and Gen Trans Cont) */
		struct bt_mesh_adv *adv[3];

		/* Index of the next adv to be sent */
		int next;

		prov_bearer_send_complete_t cb;

		void *cb_data;

		/* Retransmit timer */
		struct k_work_delayable retransmit;

		/* Unacked adv buffers (Link Ack, Link Close and Gen Trans Ack). Array size is
		 * hardcoded to 2 allowing to send Gen Trans Ack and Link Close at the same time.
		 */
		struct unacked_adv_ctx {
			struct bt_mesh_adv *adv;
			prov_bearer_send_complete_t cb;
			void *cb_data;
		} unacked[2];

		/* Last sent unacked[] buffer */
		int last_unacked;
	} tx;

	/* Protocol timeout */
	struct k_work_delayable prot_timer;
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

static void tx_work_handler(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(tx_work, tx_work_handler);

static void tx_schedule(void)
{
	uint16_t random_delay;

	if (atomic_test_bit(link.flags, ADV_SENDING)) {
		LOG_DBG("Another tx is in progress");
		return;
	}

	(void)bt_rand(&random_delay, sizeof(random_delay));
	random_delay = 20 + (random_delay % 30);

	LOG_DBG("Next PDU delayed by %ums", random_delay);

	(void)k_work_schedule(&tx_work, K_MSEC(random_delay));
}

static int send_unacked(struct bt_mesh_adv *adv, prov_bearer_send_complete_t cb,
			 void *cb_data)
{
	for (int i = 0; i < ARRAY_SIZE(link.tx.unacked); i++) {
		if (link.tx.unacked[i].adv != NULL) {
			continue;
		}

		link.tx.unacked[i].adv = adv;
		link.tx.unacked[i].cb = cb;
		link.tx.unacked[i].cb_data = cb_data;

		tx_schedule();

		return 0;
	}

	LOG_WRN("No memory to send unacked PDU: %s", bt_hex(adv->b.data, adv->b.len));
	return -ENOMEM;
}

static void send_reliable(void)
{
	/* Dropping next tx adv index to start transmission from the first adv buffer. */
	link.tx.next = 0;

	tx_schedule();
}

static void delayed_adv_send_end(int err, void *user_data)
{
	bool unacked = (bool)user_data;
	struct unacked_adv_ctx *unacked_adv = &link.tx.unacked[link.tx.last_unacked];

	if (unacked && unacked_adv->adv != NULL) {
		if (unacked_adv->cb) {
			unacked_adv->cb(err, unacked_adv->cb_data);
		}

		bt_mesh_adv_unref(unacked_adv->adv);
		unacked_adv->adv = NULL;
	}

	atomic_clear_bit(link.flags, ADV_SENDING);
	tx_schedule();
}

static void delayed_adv_send_start(uint16_t duration, int err, void *user_data)
{
	if (err) {
		delayed_adv_send_end(err, user_data);
	}
}

static const struct bt_mesh_send_cb delayed_adv_send_cb = {
	.start = delayed_adv_send_start,
	.end = delayed_adv_send_end,
};

static void tx_work_handler(struct k_work *work)
{
	int i;

	/* Send Link Ack, Link Close and Gen Trans Ack first. */
	for (i = 0; i < ARRAY_SIZE(link.tx.unacked); i++) {
		int idx = (i + link.tx.last_unacked) % ARRAY_SIZE(link.tx.unacked);
		struct unacked_adv_ctx *unacked = &link.tx.unacked[idx];

		if (!unacked->adv) {
			continue;
		}

		atomic_set_bit(link.flags, ADV_SENDING);
		bt_mesh_adv_send(unacked->adv, &delayed_adv_send_cb, (void *)true);

		link.tx.last_unacked = idx;

		return;
	}

	/* Send Trans Start, Trans Cont and Link Open */
	if (link.tx.next >= ARRAY_SIZE(link.tx.adv) || link.tx.adv[link.tx.next] == NULL) {
		LOG_DBG("All PDUs were sent");
		return;
	}

	atomic_set_bit(link.flags, ADV_SENDING);
	bt_mesh_adv_send(link.tx.adv[link.tx.next], &delayed_adv_send_cb, (void *)false);

	link.tx.next++;

	if (link.tx.next == ARRAY_SIZE(link.tx.adv) || link.tx.adv[link.tx.next] == NULL) {
		/* All ack-able PDUs are sent. Now we can run the retransmit timer. */
		LOG_DBG("Starting retransmit timer");
		k_work_reschedule(&link.tx.retransmit, RETRANSMIT_TIMEOUT);
	}
}

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
	bool canceled = false;

	for (i = 0; i < ARRAY_SIZE(link.tx.adv); i++) {
		struct bt_mesh_adv *adv = link.tx.adv[i];
		int err;

		if (!adv) {
			break;
		}

		link.tx.adv[i] = NULL;

		/* Terminate active adv */
		if (adv->ctx.busy == 0U) {
			err = bt_mesh_adv_terminate(adv);
			if (err == 0) {
				canceled = true;
			}
		} else {
			/* Mark as canceled */
			adv->ctx.busy = 0U;
			canceled = true;
		}

		bt_mesh_adv_unref(adv);
	}

	if (canceled) {
		atomic_clear_bit(link.flags, ADV_SENDING);
		tx_schedule();
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
	LOG_DBG("");
	prov_clear_tx();

	/* If this fails, the work handler will exit early on the LINK_ACTIVE
	 * check.
	 */
	(void)k_work_cancel_delayable(&link.prot_timer);

	if (atomic_test_bit(link.flags, ADV_PROVISIONER)) {
		/* Clear everything except the retransmit and protocol timer
		 * delayed work objects.
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

static void ack_complete(int err, void *user_data)
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
/*******************************************************************************
 * Generic provisioning
 ******************************************************************************/

static void gen_prov_ack_send(uint8_t xact_id)
{
	struct bt_mesh_adv *adv;
	bool pending = atomic_test_and_set_bit(link.flags, ADV_ACK_PENDING);
	int err;

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

	if (!pending) {
		link.tx.pending_ack = xact_id;
	}

	net_buf_simple_add_be32(&adv->b, link.id);
	net_buf_simple_add_u8(&adv->b, xact_id);
	net_buf_simple_add_u8(&adv->b, GPC_ACK);

	err = send_unacked(adv, pending ? NULL : ack_complete, NULL);
	if (err) {
		atomic_clear_bit(link.flags, ADV_ACK_PENDING);
	}
}

static void gen_prov_cont(struct prov_rx *rx, struct net_buf_simple *buf)
{
	uint8_t seg = CONT_SEG_INDEX(rx->gpc);

	if (link.tx.adv[0]) {
		LOG_DBG("Ongoing tx transaction has not been completed yet");
		return;
	}

	LOG_DBG("len %u, seg_index %u", buf->len, seg);

	if (!link.rx.seg && link.rx.id == rx->xact_id) {
		if (!ack_pending()) {
			LOG_DBG("Resending ack");
			gen_prov_ack_send(rx->xact_id);
		}

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

	if (link.tx.adv[0]) {
		LOG_DBG("Ongoing tx transaction has not been completed yet");
		return;
	}

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

	/* This (BIT(0) is set) can happen if we received a Transaction Continuation PDU, before
	 * receiving a Transaction Start PDU (see `gen_prov_cont`). Now we received the Transaction
	 * Start PDU and we can extract the last segment number. Knowing this, we check if
	 * previously received segment exceeds the last segment number. If so, we reject the
	 * Transaction Start PDU.
	 */
	if ((link.rx.seg & BIT(0)) &&
	    ((link.rx.seg & SEG_NVAL) != SEG_NVAL) &&
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
			bt_mesh_test_prov_invalid_bearer(BEARER_CTL(rx->gpc));
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

static void buf_sent(int err, void *user_data)
{
	enum prov_bearer_link_status reason = (enum prov_bearer_link_status)(int)user_data;

	atomic_clear_bit(link.flags, ADV_LINK_ACK_SENDING);

	if (atomic_test_and_clear_bit(link.flags, ADV_LINK_CLOSING)) {
		close_link(reason);
		return;
	}
}

static int bearer_ctl_send_unacked(struct bt_mesh_adv *adv, void *user_data)
{
	int err;

	if (!adv) {
		return -ENOMEM;
	}

	prov_clear_tx();
	k_work_reschedule(&link.prot_timer, bt_mesh_prov_protocol_timeout_get());

	err = send_unacked(adv, &buf_sent, user_data);
	if (err) {
		bt_mesh_adv_unref(adv);
	}

	return err;
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

		if (atomic_test_bit(link.flags, ADV_LINK_ACK_SENDING)) {
			LOG_DBG("Still sending Link Ack");
			return;
		}

		/* Ignore errors, message will be attempted again if we keep receiving link open: */
		atomic_set_bit(link.flags, ADV_LINK_ACK_SENDING);
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

	atomic_set_bit(link.flags, ADV_LINK_ACK_SENDING);
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
	int err;

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
	err = bearer_ctl_send_unacked(
		ctl_adv_create(LINK_CLOSE, &status, 1, RETRANSMITS_LINK_CLOSE),
		(void *)status);
	if (err) {
		close_link(status);
	}
}

void bt_mesh_pb_adv_init(void)
{
	k_work_init_delayable(&link.prot_timer, protocol_timeout);
	k_work_init_delayable(&link.tx.retransmit, prov_retransmit);
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
