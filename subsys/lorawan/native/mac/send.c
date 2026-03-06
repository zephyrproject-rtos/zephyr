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
#include "engine.h"
#include "crypto/crypto.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(lorawan_native_mac, CONFIG_LORAWAN_LOG_LEVEL);

#define MHDR_UNCONF_DATA_UP	0x40
#define MHDR_UNCONF_DATA_DOWN	0x60
#define MHDR_CONF_DATA_UP	0x80
#define MHDR_CONF_DATA_DOWN	0xA0

#define DIR_UPLINK		0
#define DIR_DOWNLINK		1
#define MAX_FRAME_SIZE		256

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
};

static K_SEM_DEFINE(dl_work_sem, 1, 1);

static void dl_work_handler(struct k_work *work)
{
	struct dl_work_ctx *wctx =
		CONTAINER_OF(work, struct dl_work_ctx, work);
	struct lorawan_downlink_cb *cb;

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

static int mac_build_data_frame(struct lwan_ctx *ctx,
				const struct lwan_send_req *req,
				uint8_t *frame, size_t *frame_len)
{
	struct pkt_data_hdr *hdr = (struct pkt_data_hdr *)frame;
	struct lwan_session *sess = &ctx->session;
	size_t fhdr_end = sizeof(*hdr);
	size_t msg_len;
	size_t pos;
	int ret;

	pos = DF_MIN_SIZE;
	if (req->len > 0) {
		pos += FPORT_SIZE + req->len;
	}

	if (pos > *frame_len) {
		return -EMSGSIZE;
	}

	hdr->mhdr = req->type == LORAWAN_MSG_CONFIRMED
		   ? MHDR_CONF_DATA_UP : MHDR_UNCONF_DATA_UP;
	hdr->fctrl = ctx->pending & LWAN_PENDING_ACK ? FCTRL_ACK : 0x00;
	sys_put_le32(sess->dev_addr, hdr->dev_addr);
	sys_put_le16((uint16_t)(sess->fcnt_up & FCNT_LOW_MASK), hdr->fcnt);

	if (req->len > 0) {
		ret = mac_encrypt_ul_payload(sess, req->port, req->data,
					     req->len, frame, fhdr_end);
		if (ret != 0) {
			LOG_ERR("Payload encrypt failed: %d", ret);
			return ret;
		}
	}

	msg_len = pos - LWAN_MIC_SIZE;

	ret = mac_compute_data_mic(sess->fnwk_s_int_cmac, sess->dev_addr,
				   sess->fcnt_up, DIR_UPLINK,
				   frame, msg_len, &frame[msg_len]);
	if (ret != 0) {
		LOG_ERR("Data MIC compute failed: %d", ret);
		return ret;
	}

	*frame_len = pos;

	LOG_DBG("Data frame built: port=%u len=%u fcnt=%u total=%zu",
		req->port, req->len, sess->fcnt_up, *frame_len);

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

	if (data != NULL && len > 0) {
		memcpy(dl_work_ctx.data, data, len);
	}

	k_work_submit(&dl_work_ctx.work);
}

static int mac_process_dl_payload(struct lwan_ctx *ctx,
				  const uint8_t *rx_buf, size_t rx_len,
				  uint8_t fopts_len, uint32_t dev_addr,
				  uint32_t fcnt_down, uint8_t flags,
				  int16_t rssi, int8_t snr,
				  bool ack_received)
{
	struct lwan_session *sess = &ctx->session;
	size_t fhdr_end = sizeof(struct pkt_data_hdr) + fopts_len;
	size_t msg_len = rx_len - LWAN_MIC_SIZE;
	size_t payload_start = fhdr_end + FPORT_SIZE;
	size_t payload_len;
	uint8_t port;

	if (rx_len <= fhdr_end + LWAN_MIC_SIZE) {
		/* No FPort/payload — notify ACK-only if applicable */
		if (ack_received) {
			mac_dispatch_downlink(ctx, 0, flags, rssi, snr,
					      NULL, 0);
		}
		return 0;
	}

	port = rx_buf[fhdr_end];

	payload_len = msg_len - payload_start;

	if (payload_len > 0) {
		uint8_t dl_payload[MAX_RX_BUF_SIZE];
		psa_key_id_t ecb_key;
		int ret;

		memcpy(dl_payload, &rx_buf[payload_start], payload_len);

		ecb_key = (port == 0) ? sess->fnwk_s_int_ecb : sess->app_s_key;

		ret = lwan_crypto_payload_encrypt(ecb_key, dev_addr,
						      fcnt_down,
						      DIR_DOWNLINK,
						      dl_payload,
						      payload_len);
		if (ret != 0) {
			return ret;
		}

		LOG_INF("Downlink: port=%u len=%zu fcnt=%u",
			port, payload_len, fcnt_down);

		mac_dispatch_downlink(ctx, port, flags, rssi, snr,
				      dl_payload, (uint8_t)payload_len);
	}

	return 0;
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

static int mac_parse_downlink(struct lwan_ctx *ctx, const uint8_t *rx_buf,
			      size_t rx_len, int16_t rssi, int8_t snr,
			      bool *ack_received)
{
	const struct pkt_data_hdr *hdr = (const struct pkt_data_hdr *)rx_buf;
	struct lwan_session *sess = &ctx->session;
	uint32_t dev_addr;
	uint16_t fcnt16;
	uint32_t fcnt_down;
	uint8_t fopts_len;
	uint8_t flags = 0;
	int ret;

	*ack_received = false;

	ret = mac_validate_dl_header(rx_buf, rx_len, sess->dev_addr);
	if (ret != 0) {
		return ret;
	}

	dev_addr = sys_get_le32(hdr->dev_addr);
	fcnt16 = sys_get_le16(hdr->fcnt);
	fcnt_down = mac_reconstruct_fcnt_down(fcnt16, sess->fcnt_down);
	fopts_len = FIELD_GET(FCTRL_FOPTS_LEN_MASK, hdr->fctrl);

	ret = mac_verify_data_mic(sess->fnwk_s_int_cmac, dev_addr,
				  fcnt_down, rx_buf, rx_len);
	if (ret != 0) {
		return ret;
	}

	if (sess->fcnt_down != LWAN_FCNT_NONE && fcnt_down <= sess->fcnt_down) {
		LOG_WRN("Downlink replay (fcnt %u <= %u)", fcnt_down,
			sess->fcnt_down);
		return -EAGAIN;
	}

	if (hdr->fctrl & FCTRL_ACK) {
		*ack_received = true;
	}

	if (hdr->fctrl & FCTRL_FPENDING) {
		flags |= LORAWAN_DATA_PENDING;
	}

	sess->fcnt_down = fcnt_down;

	/* Confirmed downlinks require ACK in our next uplink */
	if ((hdr->mhdr & MHDR_TYPE_MASK) == MHDR_CONF_DATA_DOWN) {
		ctx->pending |= LWAN_PENDING_ACK;
	}

	return mac_process_dl_payload(ctx, rx_buf, rx_len, fopts_len, dev_addr,
				      fcnt_down, flags, rssi, snr,
				      *ack_received);
}

static enum mac_rx_result send_rx_handler(struct lwan_ctx *ctx,
					   const uint8_t *rx_buf, size_t rx_len,
					   int16_t rssi, int8_t snr,
					   void *user_data)
{
	const struct lwan_send_req *req = user_data;
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

static void send_post_tx(struct lwan_ctx *ctx)
{
	ctx->session.fcnt_up++;
	ctx->pending &= ~LWAN_PENDING_ACK;
}

void mac_do_send(struct lwan_ctx *ctx, const struct lwan_send_req *req)
{
	const struct lwan_region_ops *region = ctx->region;
	struct lwan_session *sess = &ctx->session;
	struct mac_tx_params tx_params;
	struct lwan_dr_params dr_params;
	uint8_t tx_frame[MAX_FRAME_SIZE];
	size_t tx_frame_len = sizeof(tx_frame);
	uint32_t rx1_delay_ms;
	uint32_t tx_freq;
	uint8_t tx_dr_idx;
	uint8_t tries;
	int8_t tx_power;
	int ret;

	tx_dr_idx = (uint8_t)ctx->current_dr;

	ret = region->get_tx_params(tx_dr_idx, &dr_params, &tx_power);
	if (ret != 0) {
		LOG_ERR("Invalid datarate DR%u: %d", tx_dr_idx, ret);
		goto done;
	}

	if (req->len > dr_params.max_payload) {
		LOG_ERR("Payload too large for DR%u: %u > %u", tx_dr_idx,
			req->len, dr_params.max_payload);
		ret = -EMSGSIZE;
		goto done;
	}

	rx1_delay_ms = (sess->rx_delay == 0 ? 1 : sess->rx_delay) * 1000U;

	tries = (req->type == LORAWAN_MSG_CONFIRMED) ? ctx->conf_tries : 1;

	for (uint8_t attempt = 0; attempt < tries; attempt++) {
		tx_frame_len = sizeof(tx_frame);
		ret = mac_build_data_frame(ctx, req, tx_frame, &tx_frame_len);
		if (ret != 0) {
			goto done;
		}

		LOG_INF("Send: port=%u len=%u fcnt=%u attempt=%u/%u",
			req->port, req->len, sess->fcnt_up, attempt + 1, tries);

		ret = select_data_channel_wait(ctx, tx_dr_idx, &tx_freq);
		if (ret != 0) {
			LOG_ERR("No channel available for DR%u: %d",
				tx_dr_idx, ret);
			goto done;
		}

		tx_params = (struct mac_tx_params){
			.frame = tx_frame,
			.frame_len = tx_frame_len,
			.tx_freq = tx_freq,
			.tx_dr_idx = tx_dr_idx,
			.rx1_delay_ms = rx1_delay_ms,
			.post_tx = send_post_tx,
			.rx_handler = send_rx_handler,
			.user_data = (void *)req,
		};

		ret = mac_do_tx_rx(ctx, &tx_params);
		if (ret == 0) {
			goto done;
		}

		if (ret != -ETIMEDOUT) {
			LOG_ERR("TX/RX transaction failed: %d", ret);
			goto done;
		}

		if (req->type != LORAWAN_MSG_CONFIRMED) {
			/* Unconfirmed: no downlink is normal */
			ret = 0;
			goto done;
		}

		LOG_WRN("Confirmed uplink: no ACK (attempt %u/%u)",
			attempt + 1, tries);

		if (attempt < tries - 1) {
			uint32_t backoff_ms = 1000 + (sys_rand32_get() % 2001);

			LOG_DBG("Retry backoff: %u ms", backoff_ms);
			k_msleep(backoff_ms);
		}
	}

	ret = -ETIMEDOUT;

done:
	engine_signal_send_result(ret);
}
