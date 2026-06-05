/*
 * Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
 * SPDX-License-Identifier: Apache-2.0
 *
 * LoRaWAN MAC data frames — uplink and downlink
 *
 * Uplink data frame (unconfirmed 0x40, confirmed 0x80):
 *
 *       +--------------------+
 *    0  | MHDR (1)           |  Frame type + major version
 *       +--------------------+
 *    1  | DevAddr (4)        |  Little-endian device address
 *       +--------------------+
 *    5  | FCtrl (1)          |  UL: ADR|ADRACKReq|ACK|ClassB|FOptsLen[3:0]
 *       |                    |  DL: ADR|RFU|ACK|FPending|FOptsLen[3:0]
 *       +--------------------+
 *    6  | FCnt (2)           |  Frame counter (low 16 bits)
 *       +--------------------+
 *    8  | [FOpts] (0..15)    |  MAC commands
 *       +--------------------+  --- optional from here ---
 *  8+N  | FPort (1)          |  0 = MAC (FNwkSIntKey), 1-223 = app (AppSKey)
 *       +--------------------+
 *       | FRMPayload (M)     |  Encrypted payload
 *       +--------------------+
 *       | MIC (4)            |  aes128_cmac over B0 + MHDR..FRMPayload
 *       +--------------------+
 *
 * MIC B0 block (prepended to CMAC input, never transmitted):
 *
 *       +--------------------+
 *    0  | 0x49 (1)           |
 *       +--------------------+
 *    1  | 0x00 (4)           |  Reserved
 *       +--------------------+
 *    5  | Dir (1)            |  0 = uplink, 1 = downlink
 *       +--------------------+
 *    6  | DevAddr (4)        |
 *       +--------------------+
 *   10  | FCnt (4)           |  Full 32-bit frame counter
 *       +--------------------+
 *   14  | 0x00 (1)           |
 *       +--------------------+
 *   15  | MsgLen (1)         |  MHDR..FRMPayload length (excl. MIC)
 *       +--------------------+
 */

#include <zephyr/kernel.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/byteorder.h>
#include <string.h>

#include "mac_internal.h"
#include "mac_commands.h"
#include <engine.h>
#include <crypto/crypto.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(lorawan_native_mac, CONFIG_LORAWAN_LOG_LEVEL);

#define MHDR_UNCONF_DATA_UP	0x40
#define MHDR_UNCONF_DATA_DOWN	0x60
#define MHDR_CONF_DATA_UP	0x80
#define MHDR_CONF_DATA_DOWN	0xA0

#define DIR_UPLINK		0
#define DIR_DOWNLINK		1
#define MAX_FRAME_SIZE		256

#define FCTRL_ADR		BIT(7)
#define FCTRL_ACK		BIT(5)
#define FCTRL_FPENDING		BIT(4)
#define FCTRL_FOPTS_LEN_MASK	GENMASK(3, 0)

#define MIC_B0_TYPE		0x49

#define FCNT_LOW_MASK		BIT_MASK(16)
#define FCNT_HIGH_MASK		GENMASK(31, 16)
#define FCNT_ROLLOVER		BIT(16)

struct pkt_data_hdr {
	uint8_t mhdr;
	uint8_t dev_addr[DEV_ADDR_SIZE];
	uint8_t fctrl;
	uint8_t fcnt[FCNT_SIZE];
} __packed;

#define DF_MIN_SIZE		(sizeof(struct pkt_data_hdr) + LWAN_MIC_SIZE)

struct mic_b0 {
	uint8_t type;
	uint8_t reserved[4];
	uint8_t dir;
	uint8_t dev_addr[DEV_ADDR_SIZE];
	uint8_t fcnt[4];
	uint8_t pad;
	uint8_t msg_len;
} __packed;

struct dl_work_ctx {
	struct k_work work;
	struct lwan_ctx *ctx;
	uint8_t port;
	uint8_t flags;
	int16_t rssi;
	int8_t snr;
	uint8_t len;
	uint8_t data[MAX_RX_BUF_SIZE];
	bool notify_downlink;
};

struct send_tx_ctx {
	const struct lwan_send_req *req;
	bool tx_done;
};

struct data_frame_layout {
	size_t fopts_len;
	size_t fhdr_end;
	size_t total_len;
};

struct dl_frame_info {
	uint32_t dev_addr;
	uint32_t fcnt_down;
	uint8_t fopts_len;
	uint8_t flags;
	bool ack_received;
};

struct dl_payload_info {
	uint8_t port;
	size_t start;
	size_t len;
};

struct send_state {
	const struct lwan_send_req *req;
	struct send_tx_ctx tx_ctx;
	uint8_t frame[MAX_FRAME_SIZE];
	size_t frame_len;
	uint32_t fcnt;
	uint32_t rx1_delay_ms;
	uint8_t dr_idx;
	uint8_t tries;
};

static K_SEM_DEFINE(dl_work_sem, 1, 1);

static void dl_work_handler(struct k_work *work)
{
	struct dl_work_ctx *wctx =
		CONTAINER_OF(work, struct dl_work_ctx, work);
	struct lorawan_downlink_cb *cb;

	mac_cmd_deliver_link_check_ans(wctx->ctx);

	if (!wctx->notify_downlink) {
		k_sem_give(&dl_work_sem);
		return;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&wctx->ctx->dl_callbacks, cb, node) {
		if (cb->port == LW_RECV_PORT_ANY || cb->port == wctx->port) {
			cb->cb(wctx->port, wctx->flags, wctx->rssi,
			       wctx->snr, wctx->len,
			       wctx->len > 0 ? wctx->data : NULL);
		}
	}

	k_sem_give(&dl_work_sem);
}

static struct dl_work_ctx dl_work_ctx = {
	.work = Z_WORK_INITIALIZER(dl_work_handler),
};

static int mac_compute_data_mic(psa_key_id_t cmac_key, uint32_t dev_addr,
				uint32_t fcnt, uint8_t dir,
				const uint8_t *msg, size_t msg_len,
				uint8_t *mic)
{
	uint8_t mic_input[sizeof(struct mic_b0) + MAX_FRAME_SIZE];
	struct mic_b0 *b0 = (struct mic_b0 *)mic_input;

	memset(b0, 0, sizeof(*b0));
	b0->type = MIC_B0_TYPE;
	b0->dir = dir;
	sys_put_le32(dev_addr, b0->dev_addr);
	sys_put_le32(fcnt, b0->fcnt);
	b0->msg_len = (uint8_t)msg_len;

	memcpy(&mic_input[sizeof(*b0)], msg, msg_len);

	return lwan_crypto_compute_mic(cmac_key, mic_input,
				       sizeof(*b0) + msg_len, mic);
}

static int mac_encrypt_ul_payload(struct lwan_session *sess, uint8_t port,
				  const uint8_t *data, uint8_t len,
				  uint8_t *frame, size_t fhdr_end)
{
	psa_key_id_t key;

	frame[fhdr_end] = port;
	memcpy(&frame[fhdr_end + FPORT_SIZE], data, len);

	key = (port == 0) ? sess->fnwk_s_int_ecb : sess->app_s_key;

	return lwan_crypto_payload_encrypt(key, sess->dev_addr,
					   sess->fcnt_up, DIR_UPLINK,
					   &frame[fhdr_end + FPORT_SIZE], len);
}

static struct data_frame_layout mac_ul_frame_layout(
	const struct lwan_send_req *req, size_t fopts_len)
{
	struct data_frame_layout layout = {
		.fopts_len = fopts_len,
		.fhdr_end = sizeof(struct pkt_data_hdr) + fopts_len,
	};

	layout.total_len = layout.fhdr_end + LWAN_MIC_SIZE;
	if (req->len > 0) {
		layout.total_len += FPORT_SIZE + req->len;
	}

	return layout;
}

static uint8_t mac_ul_fctrl(const struct lwan_ctx *ctx, size_t fopts_len)
{
	uint8_t fctrl = (uint8_t)FIELD_PREP(FCTRL_FOPTS_LEN_MASK, fopts_len);

	if (ctx->pending & LWAN_PENDING_ACK) {
		fctrl |= FCTRL_ACK;
	}

	if (ctx->mac.adr_enabled) {
		fctrl |= FCTRL_ADR;
	}

	return fctrl;
}

static void mac_write_ul_header(struct lwan_ctx *ctx,
				const struct lwan_send_req *req,
				uint8_t *frame, const uint8_t *fopts,
				size_t fopts_len)
{
	struct pkt_data_hdr *hdr = (struct pkt_data_hdr *)frame;
	struct lwan_session *sess = &ctx->session;

	hdr->mhdr = req->type == LORAWAN_MSG_CONFIRMED
		   ? MHDR_CONF_DATA_UP : MHDR_UNCONF_DATA_UP;
	hdr->fctrl = mac_ul_fctrl(ctx, fopts_len);
	sys_put_le32(sess->dev_addr, hdr->dev_addr);
	sys_put_le16((uint16_t)(sess->fcnt_up & FCNT_LOW_MASK), hdr->fcnt);

	if (fopts_len > 0) {
		memcpy(&frame[sizeof(*hdr)], fopts, fopts_len);
	}
}

static int mac_write_ul_payload(struct lwan_session *sess,
				const struct lwan_send_req *req,
				uint8_t *frame, size_t fhdr_end)
{
	int ret;

	if (req->len == 0) {
		return 0;
	}

	ret = mac_encrypt_ul_payload(sess, req->port, req->data,
				     req->len, frame, fhdr_end);
	if (ret != 0) {
		LOG_ERR("Payload encrypt failed: %d", ret);
	}

	return ret;
}

static int mac_write_ul_mic(struct lwan_session *sess, uint8_t *frame,
			    size_t msg_len)
{
	int ret;

	ret = mac_compute_data_mic(sess->fnwk_s_int_cmac, sess->dev_addr,
				   sess->fcnt_up, DIR_UPLINK,
				   frame, msg_len, &frame[msg_len]);
	if (ret != 0) {
		LOG_ERR("Data MIC compute failed: %d", ret);
	}

	return ret;
}

static int mac_build_data_frame(struct lwan_ctx *ctx,
				const struct lwan_send_req *req,
				uint8_t *frame, size_t *frame_len)
{
	struct lwan_session *sess = &ctx->session;
	struct data_frame_layout layout;
	uint8_t fopts[LWAN_MAX_FOPTS_LEN];
	size_t fopts_len;
	size_t msg_len;
	int ret;

	fopts_len = mac_cmd_build_ul_fopts(ctx, fopts, sizeof(fopts));
	layout = mac_ul_frame_layout(req, fopts_len);

	if (layout.total_len > *frame_len) {
		return -EMSGSIZE;
	}

	mac_write_ul_header(ctx, req, frame, fopts, layout.fopts_len);

	ret = mac_write_ul_payload(sess, req, frame, layout.fhdr_end);
	if (ret != 0) {
		return ret;
	}

	msg_len = layout.total_len - LWAN_MIC_SIZE;

	ret = mac_write_ul_mic(sess, frame, msg_len);
	if (ret != 0) {
		return ret;
	}

	*frame_len = layout.total_len;

	LOG_DBG("Data frame built: port=%u len=%u fcnt=%u fopts=%zu total=%zu",
		req->port, req->len, sess->fcnt_up, layout.fopts_len,
		*frame_len);

	return 0;
}

static void mac_dispatch_downlink(struct lwan_ctx *ctx, uint8_t port,
				  uint8_t flags, int16_t rssi, int8_t snr,
				  const uint8_t *data, uint8_t len)
{
	if (k_sem_take(&dl_work_sem, K_NO_WAIT) != 0) {
		LOG_WRN("Downlink dispatch busy, dropping port=%u len=%u",
			port, len);
		return;
	}

	dl_work_ctx.ctx = ctx;
	dl_work_ctx.port = port;
	dl_work_ctx.flags = flags;
	dl_work_ctx.rssi = rssi;
	dl_work_ctx.snr = snr;
	dl_work_ctx.len = len;
	dl_work_ctx.notify_downlink = true;

	if (data != NULL && len > 0) {
		memcpy(dl_work_ctx.data, data, len);
	}

	k_work_submit(&dl_work_ctx.work);
}

static void mac_dispatch_mac_delivery(struct lwan_ctx *ctx)
{
	if (k_sem_take(&dl_work_sem, K_NO_WAIT) != 0) {
		LOG_WRN("MAC notification dispatch busy, dropping");
		return;
	}

	dl_work_ctx.ctx = ctx;
	dl_work_ctx.notify_downlink = false;

	k_work_submit(&dl_work_ctx.work);
}

/* Dispatch payload-less notifications so callbacks run on the workqueue. */
static void mac_dispatch_dl_notify(struct lwan_ctx *ctx,
				   const struct dl_frame_info *frame_info,
				   int16_t rssi, int8_t snr)
{
	if (frame_info->ack_received) {
		mac_dispatch_downlink(ctx, 0, frame_info->flags, rssi, snr,
				      NULL, 0);
	} else if (mac_cmd_has_pending_delivery(ctx)) {
		mac_dispatch_mac_delivery(ctx);
	}
}

static bool mac_get_dl_payload_info(const uint8_t *rx_buf, size_t rx_len,
				    uint8_t fopts_len,
				    struct dl_payload_info *payload)
{
	size_t fhdr_end = sizeof(struct pkt_data_hdr) + fopts_len;
	size_t msg_len = rx_len - LWAN_MIC_SIZE;

	if (rx_len <= fhdr_end + LWAN_MIC_SIZE) {
		return false;
	}

	payload->port = rx_buf[fhdr_end];
	payload->start = fhdr_end + FPORT_SIZE;
	payload->len = msg_len - payload->start;

	return true;
}

static int mac_decrypt_dl_payload(struct lwan_session *sess,
				  const struct dl_frame_info *frame_info,
				  const struct dl_payload_info *payload,
				  const uint8_t *rx_buf, uint8_t *dl_payload)
{
	psa_key_id_t ecb_key = payload->port == 0
			       ? sess->fnwk_s_int_ecb : sess->app_s_key;

	memcpy(dl_payload, &rx_buf[payload->start], payload->len);

	return lwan_crypto_payload_encrypt(ecb_key, frame_info->dev_addr,
					   frame_info->fcnt_down,
					   DIR_DOWNLINK, dl_payload,
					   payload->len);
}

static int mac_dispatch_dl_payload(struct lwan_ctx *ctx,
				   const struct dl_frame_info *frame_info,
				   const struct dl_payload_info *payload,
				   int16_t rssi, int8_t snr,
				   const uint8_t *rx_buf)
{
	uint8_t dl_payload[MAX_RX_BUF_SIZE];
	int ret;

	if (payload->len == 0) {
		return 0;
	}

	ret = mac_decrypt_dl_payload(&ctx->session, frame_info, payload,
				     rx_buf, dl_payload);
	if (ret != 0) {
		return ret;
	}

	LOG_INF("Downlink: port=%u len=%zu fcnt=%u",
		payload->port, payload->len, frame_info->fcnt_down);

	mac_dispatch_downlink(ctx, payload->port, frame_info->flags, rssi, snr,
			      dl_payload, (uint8_t)payload->len);

	return 0;
}

static int mac_process_dl_payload(struct lwan_ctx *ctx,
				  const uint8_t *rx_buf, size_t rx_len,
				  const struct dl_frame_info *frame_info,
				  int16_t rssi, int8_t snr)
{
	struct dl_payload_info payload;

	if (!mac_get_dl_payload_info(rx_buf, rx_len, frame_info->fopts_len,
				     &payload)) {
		/* No FPort/FRMPayload: only an ACK or a queued answer */
		mac_dispatch_dl_notify(ctx, frame_info, rssi, snr);
		return 0;
	}

	return mac_dispatch_dl_payload(ctx, frame_info, &payload, rssi, snr,
				       rx_buf);
}

static uint32_t mac_reconstruct_fcnt_down(uint16_t fcnt16,
					  uint32_t last_fcnt_down)
{
	uint32_t fcnt_down;

	if (last_fcnt_down == LWAN_FCNT_NONE) {
		return fcnt16;
	}

	fcnt_down = (last_fcnt_down & FCNT_HIGH_MASK) | fcnt16;
	if (fcnt_down < last_fcnt_down) {
		fcnt_down += FCNT_ROLLOVER;
	}

	return fcnt_down;
}

static int mac_validate_dl_header(const uint8_t *rx_buf, size_t rx_len,
				  uint32_t expected_dev_addr)
{
	const struct pkt_data_hdr *hdr = (const struct pkt_data_hdr *)rx_buf;
	uint32_t dev_addr;

	if (rx_len < DF_MIN_SIZE) {
		LOG_DBG("Downlink too short: %zu", rx_len);
		return -EINVAL;
	}

	if ((hdr->mhdr & MHDR_TYPE_MASK) != MHDR_UNCONF_DATA_DOWN &&
	    (hdr->mhdr & MHDR_TYPE_MASK) != MHDR_CONF_DATA_DOWN) {
		LOG_DBG("Not a data downlink: MHDR=0x%02X", hdr->mhdr);
		return -EINVAL;
	}

	if ((hdr->mhdr & MHDR_MAJOR_MASK) != MHDR_MAJOR_R1) {
		LOG_DBG("Unsupported Major version: MHDR=0x%02X", hdr->mhdr);
		return -EINVAL;
	}

	dev_addr = sys_get_le32(hdr->dev_addr);
	if (dev_addr != expected_dev_addr) {
		LOG_DBG("DevAddr mismatch: 0x%08X != 0x%08X",
			dev_addr, expected_dev_addr);
		return -EINVAL;
	}

	return 0;
}

static int mac_verify_data_mic(psa_key_id_t cmac_key, uint32_t dev_addr,
			       uint32_t fcnt, const uint8_t *frame,
			       size_t frame_len)
{
	uint8_t computed_mic[LWAN_MIC_SIZE];
	size_t msg_len = frame_len - LWAN_MIC_SIZE;
	int ret;

	ret = mac_compute_data_mic(cmac_key, dev_addr, fcnt, DIR_DOWNLINK,
				   frame, msg_len, computed_mic);
	if (ret != 0) {
		return ret;
	}

	if (memcmp(computed_mic, &frame[msg_len], LWAN_MIC_SIZE) != 0) {
		LOG_WRN("Downlink MIC mismatch");
		return -EBADMSG;
	}

	return 0;
}

static int mac_validate_dl_fopts_len(size_t rx_len, uint8_t fopts_len)
{
	if (rx_len < sizeof(struct pkt_data_hdr) + fopts_len + LWAN_MIC_SIZE) {
		LOG_DBG("Invalid FOptsLen: %u for len %zu", fopts_len, rx_len);
		return -EINVAL;
	}

	return 0;
}

static int mac_read_dl_frame_info(struct lwan_ctx *ctx, const uint8_t *rx_buf,
				  size_t rx_len,
				  struct dl_frame_info *frame_info)
{
	const struct pkt_data_hdr *hdr = (const struct pkt_data_hdr *)rx_buf;
	struct lwan_session *sess = &ctx->session;
	int ret;

	ret = mac_validate_dl_header(rx_buf, rx_len, sess->dev_addr);
	if (ret != 0) {
		return ret;
	}

	frame_info->dev_addr = sys_get_le32(hdr->dev_addr);
	frame_info->fcnt_down =
		mac_reconstruct_fcnt_down(sys_get_le16(hdr->fcnt),
					   sess->fcnt_down);
	frame_info->fopts_len = FIELD_GET(FCTRL_FOPTS_LEN_MASK, hdr->fctrl);

	return mac_validate_dl_fopts_len(rx_len, frame_info->fopts_len);
}

static int mac_validate_dl_replay(struct lwan_session *sess,
				  uint32_t fcnt_down)
{
	if (sess->fcnt_down != LWAN_FCNT_NONE && fcnt_down <= sess->fcnt_down) {
		LOG_WRN("Downlink replay (fcnt %u <= %u)", fcnt_down,
			sess->fcnt_down);
		return -EAGAIN;
	}

	return 0;
}

static void mac_apply_dl_fctrl(struct lwan_ctx *ctx,
			       const struct pkt_data_hdr *hdr,
			       struct dl_frame_info *frame_info)
{
	frame_info->ack_received = (hdr->fctrl & FCTRL_ACK) != 0;
	frame_info->flags = 0;

	if (hdr->fctrl & FCTRL_FPENDING) {
		frame_info->flags |= LORAWAN_DATA_PENDING;
	}

	/* Confirmed downlinks require ACK in our next uplink */
	if ((hdr->mhdr & MHDR_TYPE_MASK) == MHDR_CONF_DATA_DOWN) {
		ctx->pending |= LWAN_PENDING_ACK;
	}
}

static int mac_parse_downlink(struct lwan_ctx *ctx, const uint8_t *rx_buf,
			      size_t rx_len, int16_t rssi, int8_t snr,
			      bool *ack_received)
{
	const struct pkt_data_hdr *hdr = (const struct pkt_data_hdr *)rx_buf;
	struct lwan_session *sess = &ctx->session;
	struct dl_frame_info frame_info;
	int ret;

	*ack_received = false;

	ret = mac_read_dl_frame_info(ctx, rx_buf, rx_len, &frame_info);
	if (ret != 0) {
		return ret;
	}

	ret = mac_verify_data_mic(sess->fnwk_s_int_cmac, frame_info.dev_addr,
				  frame_info.fcnt_down, rx_buf, rx_len);
	if (ret != 0) {
		return ret;
	}

	ret = mac_validate_dl_replay(sess, frame_info.fcnt_down);
	if (ret != 0) {
		return ret;
	}

	mac_cmd_process_dl_fopts(ctx, &rx_buf[sizeof(struct pkt_data_hdr)],
				 frame_info.fopts_len);

	mac_apply_dl_fctrl(ctx, hdr, &frame_info);
	sess->fcnt_down = frame_info.fcnt_down;
	*ack_received = frame_info.ack_received;

	return mac_process_dl_payload(ctx, rx_buf, rx_len, &frame_info,
				      rssi, snr);
}

static enum mac_rx_result send_rx_handler(struct lwan_ctx *ctx,
					   const uint8_t *rx_buf, size_t rx_len,
					   int16_t rssi, int8_t snr,
					   void *user_data)
{
	const struct send_tx_ctx *tx_ctx = user_data;
	const struct lwan_send_req *req = tx_ctx->req;
	bool ack_received;
	int ret;

	ret = mac_parse_downlink(ctx, rx_buf, rx_len, rssi, snr, &ack_received);
	if (ret != 0) {
		return MAC_RX_CONTINUE;
	}

	if (req->type == LORAWAN_MSG_CONFIRMED && !ack_received) {
		LOG_DBG("Downlink without ACK for confirmed uplink");
		return MAC_RX_CONTINUE;
	}

	return MAC_RX_DONE;
}

static int select_data_channel_wait(struct lwan_ctx *ctx, uint8_t dr,
				    uint32_t *freq)
{
	const struct lwan_region_ops *region = ctx->region;
	int32_t delay_ms;
	int ret;

	do {
		ret = region->select_data_channel(ctx->channels,
						  ctx->channel_count,
						  dr, freq, &delay_ms);
		if (ret == -ENOBUFS) {
			LOG_INF("Duty cycle: waiting %d ms", delay_ms);
			k_msleep(delay_ms);
		}
	} while (ret == -ENOBUFS);

	return ret;
}

static void send_post_tx(struct lwan_ctx *ctx, void *user_data)
{
	struct send_tx_ctx *tx_ctx = user_data;

	tx_ctx->tx_done = true;
	ctx->pending &= ~LWAN_PENDING_ACK;
	mac_cmd_commit_ul_fopts(ctx);
}

static uint32_t send_rx1_delay_ms(const struct lwan_session *sess)
{
	return (sess->rx_delay == 0 ? 1 : sess->rx_delay) * 1000U;
}

static uint8_t send_try_count(const struct lwan_ctx *ctx,
			      const struct lwan_send_req *req)
{
	return req->type == LORAWAN_MSG_CONFIRMED ? ctx->conf_tries : 1;
}

static void send_state_init(struct send_state *state, struct lwan_ctx *ctx,
			    const struct lwan_send_req *req)
{
	state->req = req;
	state->tx_ctx = (struct send_tx_ctx){
		.req = req,
	};
	state->frame_len = sizeof(state->frame);
	state->fcnt = ctx->session.fcnt_up;
	state->rx1_delay_ms = send_rx1_delay_ms(&ctx->session);
	state->dr_idx = (uint8_t)ctx->current_dr;
	state->tries = send_try_count(ctx, req);
}

static int send_validate_payload_size(struct lwan_ctx *ctx,
				      const struct send_state *state)
{
	struct lwan_dr_params dr_params;
	uint8_t max_payload;
	int8_t tx_power;
	int ret;

	ret = ctx->region->get_tx_params(state->dr_idx, ctx->mac.tx_power_idx,
					 &dr_params, &tx_power);
	if (ret != 0) {
		LOG_ERR("Invalid datarate DR%u: %d", state->dr_idx, ret);
		return ret;
	}

	/* Pending MAC commands in FOpts eat into the payload budget */
	max_payload = mac_cmd_next_payload_size(ctx, dr_params.max_payload);
	if (state->req->len > max_payload) {
		LOG_ERR("Payload too large for DR%u: %u > %u", state->dr_idx,
			state->req->len, max_payload);
		return -EMSGSIZE;
	}

	return 0;
}

static int send_prepare_frame(struct lwan_ctx *ctx, struct send_state *state)
{
	int ret;

	ret = send_validate_payload_size(ctx, state);
	if (ret != 0) {
		return ret;
	}

	return mac_build_data_frame(ctx, state->req, state->frame,
				    &state->frame_len);
}

static struct mac_tx_params send_tx_params(struct send_state *state,
					   uint32_t tx_freq)
{
	return (struct mac_tx_params){
		.frame = state->frame,
		.frame_len = state->frame_len,
		.tx_freq = tx_freq,
		.tx_dr_idx = state->dr_idx,
		.rx1_delay_ms = state->rx1_delay_ms,
		.post_tx = send_post_tx,
		.rx_handler = send_rx_handler,
		.user_data = &state->tx_ctx,
	};
}

static void send_retry_backoff(void)
{
	uint32_t backoff_ms = 1000 + (sys_rand32_get() % 2001);

	LOG_DBG("Retry backoff: %u ms", backoff_ms);
	k_msleep(backoff_ms);
}

/*
 * Positive retry sentinel: cannot collide with success (0) or an
 * errno value propagated from the layers below.
 */
#define SEND_ATTEMPT_RETRY	1

static int send_handle_attempt_result(const struct send_state *state,
				      uint8_t attempt, int ret)
{
	if (ret == 0) {
		return 0;
	}

	if (ret != -ETIMEDOUT) {
		LOG_ERR("TX/RX transaction failed: %d", ret);
		return ret;
	}

	if (state->req->type != LORAWAN_MSG_CONFIRMED) {
		/* Unconfirmed: no downlink is normal */
		return 0;
	}

	LOG_WRN("Confirmed uplink: no ACK (attempt %u/%u)",
		attempt + 1, state->tries);

	if (attempt < state->tries - 1) {
		send_retry_backoff();
	}

	return SEND_ATTEMPT_RETRY;
}

static int send_one_attempt(struct lwan_ctx *ctx, struct send_state *state,
			    uint8_t attempt)
{
	struct mac_tx_params tx_params;
	uint32_t tx_freq;
	int ret;

	LOG_INF("Send: port=%u len=%u fcnt=%u attempt=%u/%u",
		state->req->port, state->req->len, state->fcnt,
		attempt + 1, state->tries);

	ret = select_data_channel_wait(ctx, state->dr_idx, &tx_freq);
	if (ret != 0) {
		LOG_ERR("No channel available for DR%u: %d",
			state->dr_idx, ret);
		return ret;
	}

	tx_params = send_tx_params(state, tx_freq);

	ret = mac_do_tx_rx(ctx, &tx_params);

	return send_handle_attempt_result(state, attempt, ret);
}

static int send_attempts(struct lwan_ctx *ctx, struct send_state *state)
{
	int ret;

	for (uint8_t attempt = 0; attempt < state->tries; attempt++) {
		ret = send_one_attempt(ctx, state, attempt);
		if (ret != SEND_ATTEMPT_RETRY) {
			return ret;
		}
	}

	return -ETIMEDOUT;
}

void mac_do_send(struct lwan_ctx *ctx, const struct lwan_req *req)
{
	const struct lwan_send_req *send_req = req->data;
	struct lwan_session *sess = &ctx->session;
	struct send_state state;
	int ret;

	send_state_init(&state, ctx, send_req);

	ret = send_prepare_frame(ctx, &state);
	if (ret != 0) {
		goto done;
	}

	ret = send_attempts(ctx, &state);

done:
	if (state.tx_ctx.tx_done) {
		sess->fcnt_up++;
	}

	engine_signal_result(req, ret);
}
