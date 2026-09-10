/*
 * Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
 * SPDX-License-Identifier: Apache-2.0
 *
 * LoRaWAN OTAA Join — frame formats
 *
 * Join Request (23 bytes, unencrypted):
 *
 *       +--------------------+
 *    0  | MHDR (1)           |  0x00 (Join Request)
 *       +--------------------+
 *    1  | JoinEUI (8)        |  Big-endian application identifier
 *       +--------------------+
 *    9  | DevEUI (8)         |  Big-endian device identifier
 *       +--------------------+
 *   17  | DevNonce (2)       |  Replay protection counter
 *       +--------------------+
 *   19  | MIC (4)            |  aes128_cmac(NwkKey, MHDR..DevNonce)[0:3]
 *       +--------------------+
 *
 * Join Accept (raw frame: 17 or 33 bytes):
 *
 *       +--------------------+
 *    0  | MHDR (1)           |  0x20 (Join Accept)
 *       +--------------------+
 *    1  | encrypted payload  |  aes128_encrypt(NwkKey) — NOT decrypt
 *       | (16 or 32)         |
 *       +--------------------+
 *
 *   Decrypted payload (offsets relative to payload start):
 *
 *       +--------------------+
 *    0  | JoinNonce (3)      |  Server-side nonce
 *       +--------------------+
 *    3  | NetID (3)          |  Network identifier
 *       +--------------------+
 *    6  | DevAddr (4)        |  Assigned device address
 *       +--------------------+
 *   10  | DLSettings (1)     |  RX1DRoffset[6:4] | RX2DataRate[3:0]
 *       +--------------------+
 *   11  | RxDelay (1)        |  Seconds (0 means 1)
 *       +--------------------+
 *   12  | [CFList] (0/16)    |  Optional channel list
 *       +--------------------+
 *       | MIC (4)            |  aes128_cmac(NwkKey, MHDR..last field)[0:3]
 *       +--------------------+
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <string.h>

#include "mac_internal.h"
#include <engine.h>
#include <crypto/crypto.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(lorawan_native_mac, CONFIG_LORAWAN_LOG_LEVEL);

#define MHDR_JOIN_REQUEST	0x00
#define MHDR_JOIN_ACCEPT	0x20

#define JA_CFLIST_SIZE		16

#define DL_RX1_DR_OFFSET_MASK	GENMASK(6, 4)
#define DL_RX2_DATARATE_MASK	GENMASK(3, 0)

#define JOIN_ACCEPT_DELAY1_MS	5000

struct pkt_join_request {
	uint8_t mhdr;
	uint8_t join_eui[EUI_SIZE];
	uint8_t dev_eui[EUI_SIZE];
	uint8_t dev_nonce[DEV_NONCE_SIZE];
	uint8_t mic[LWAN_MIC_SIZE];
} __packed;

struct pkt_join_accept {
	uint8_t join_nonce[JOIN_NONCE_SIZE];
	uint8_t net_id[NET_ID_SIZE];
	uint8_t dev_addr[DEV_ADDR_SIZE];
	uint8_t dl_settings;
	uint8_t rx_delay;
} __packed;

#define JA_MIN_SIZE		(sizeof(struct pkt_join_accept) + LWAN_MIC_SIZE)
#define JA_MAX_SIZE		(sizeof(struct pkt_join_accept) + JA_CFLIST_SIZE + LWAN_MIC_SIZE)

struct join_rx_ctx {
	psa_key_id_t nwk_cmac;
	psa_key_id_t nwk_ecb;
	uint16_t dev_nonce;
};

struct join_state {
	const struct lwan_join_req *req;
	struct join_rx_ctx rx_ctx;
	uint8_t frame[sizeof(struct pkt_join_request)];
	size_t frame_len;
	uint32_t tx_freq;
	uint8_t tx_dr_idx;
};

static void mac_write_join_request_fields(struct pkt_join_request *pkt,
					  const struct lwan_join_req *req)
{
	pkt->mhdr = MHDR_JOIN_REQUEST;
	sys_memcpy_swap(pkt->join_eui, req->join_eui, EUI_SIZE);
	sys_memcpy_swap(pkt->dev_eui, req->dev_eui, EUI_SIZE);
	sys_put_le16(req->dev_nonce, pkt->dev_nonce);
}

static int mac_write_join_request_mic(psa_key_id_t nwk_cmac,
				      uint8_t *frame)
{
	struct pkt_join_request *pkt = (struct pkt_join_request *)frame;
	size_t mic_off = offsetof(struct pkt_join_request, mic);
	int ret;

	/* MIC = cmac(NwkKey, MHDR | JoinEUI | DevEUI | DevNonce) */
	ret = lwan_crypto_compute_mic(nwk_cmac, frame, mic_off, pkt->mic);
	if (ret != 0) {
		LOG_ERR("Failed to compute join request MIC: %d", ret);
	}

	return ret;
}

static int mac_build_join_request(const struct lwan_join_req *req,
				  psa_key_id_t nwk_cmac,
				  uint8_t *frame, size_t *frame_len)
{
	struct pkt_join_request *pkt = (struct pkt_join_request *)frame;
	int ret;

	if (*frame_len < sizeof(*pkt)) {
		return -ENOMEM;
	}

	mac_write_join_request_fields(pkt, req);

	ret = mac_write_join_request_mic(nwk_cmac, frame);
	if (ret != 0) {
		return ret;
	}

	*frame_len = sizeof(*pkt);

	LOG_DBG("Join request built: dev_nonce=%u", req->dev_nonce);

	return 0;
}

static int mac_apply_dl_settings(struct lwan_ctx *ctx, uint8_t dl_settings)
{
	uint8_t rx1_dr_offset = FIELD_GET(DL_RX1_DR_OFFSET_MASK, dl_settings);
	uint8_t rx2_datarate = FIELD_GET(DL_RX2_DATARATE_MASK, dl_settings);
	int ret;

	ret = ctx->region->validate_dl_settings(rx1_dr_offset, rx2_datarate);
	if (ret != 0) {
		LOG_ERR("Invalid DLSettings: RX1DRoff=%u RX2DR=%u",
			rx1_dr_offset, rx2_datarate);
		return ret;
	}

	ctx->session.rx1_dr_offset = rx1_dr_offset;
	ctx->session.rx2_datarate = rx2_datarate;

	return 0;
}

static void mac_reset_channel_plan(struct lwan_ctx *ctx,
				    const uint8_t *payload, size_t payload_len)
{
	const struct lwan_region_ops *region = ctx->region;
	const uint8_t *cflist = payload + sizeof(struct pkt_join_accept);

	region->get_default_channels(ctx->channels, &ctx->channel_count);

	if (payload_len > JA_MIN_SIZE) {
		region->apply_cflist(cflist, ctx->channels, &ctx->channel_count);
	}
}

static void mac_read_join_accept_session(struct lwan_session *sess,
					 const struct pkt_join_accept *ja)
{
	sess->dev_addr = sys_get_le32(ja->dev_addr);
	sess->rx_delay = ja->rx_delay;
}

static void mac_log_join_accept(const struct lwan_session *sess)
{
	LOG_INF("Join accept: DevAddr=0x%08X RX1DRoff=%u RX2DR=%u RxDelay=%u",
		sess->dev_addr, sess->rx1_dr_offset,
		sess->rx2_datarate, sess->rx_delay);
}

static void mac_destroy_session_keys(struct lwan_session *sess)
{
	/* Destroy previous session keys if re-joining */
	psa_destroy_key(sess->app_s_key);
	psa_destroy_key(sess->fnwk_s_int_cmac);
	psa_destroy_key(sess->fnwk_s_int_ecb);
}

static int mac_derive_join_session_keys(struct lwan_session *sess,
					const struct pkt_join_accept *ja,
					psa_key_id_t nwk_ecb,
					uint16_t dev_nonce)
{
	int ret;

	ret = lwan_crypto_derive_session_keys(nwk_ecb,
					      ja->join_nonce,
					      ja->net_id,
					      dev_nonce,
					      &sess->app_s_key,
					      &sess->fnwk_s_int_cmac,
					      &sess->fnwk_s_int_ecb);
	if (ret != 0) {
		LOG_ERR("Session key derivation failed: %d", ret);
	}

	return ret;
}

static void mac_finish_join_accept(struct lwan_ctx *ctx,
				   const uint8_t *frame, size_t frame_len)
{
	struct lwan_session *sess = &ctx->session;

	sess->fcnt_up = 0;
	sess->fcnt_down = LWAN_FCNT_NONE;

	mac_reset_channel_plan(ctx, &frame[MHDR_SIZE], frame_len - MHDR_SIZE);

	atomic_set_bit(ctx->flags, LWAN_FLAG_JOINED);
}

static int mac_apply_join_accept(struct lwan_ctx *ctx,
				  const uint8_t *frame, size_t frame_len,
				  psa_key_id_t nwk_ecb, uint16_t dev_nonce)
{
	const struct pkt_join_accept *ja =
		(const struct pkt_join_accept *)&frame[MHDR_SIZE];
	struct lwan_session *sess = &ctx->session;
	int ret;

	mac_read_join_accept_session(sess, ja);

	ret = mac_apply_dl_settings(ctx, ja->dl_settings);
	if (ret != 0) {
		return ret;
	}

	mac_log_join_accept(sess);
	mac_destroy_session_keys(sess);

	ret = mac_derive_join_session_keys(sess, ja, nwk_ecb, dev_nonce);
	if (ret != 0) {
		return ret;
	}

	mac_finish_join_accept(ctx, frame, frame_len);

	return 0;
}

static int mac_verify_join_accept_mic(psa_key_id_t nwk_cmac,
				       const uint8_t *frame, size_t frame_len)
{
	uint8_t computed_mic[LWAN_MIC_SIZE];
	int ret;

	ret = lwan_crypto_compute_mic(nwk_cmac, frame,
				      frame_len - LWAN_MIC_SIZE, computed_mic);
	if (ret != 0) {
		return ret;
	}

	if (memcmp(computed_mic, &frame[frame_len - LWAN_MIC_SIZE],
		   LWAN_MIC_SIZE) != 0) {
		LOG_ERR("Join accept MIC mismatch");
		return -EBADMSG;
	}

	return 0;
}

static int mac_validate_join_accept_size(size_t rx_len, size_t *payload_len)
{
	*payload_len = rx_len - MHDR_SIZE;
	if (*payload_len != JA_MIN_SIZE &&
	    *payload_len != JA_MAX_SIZE) {
		LOG_ERR("Invalid join accept size: %zu", rx_len);
		return -EINVAL;
	}

	return 0;
}

static int mac_decrypt_join_accept_frame(psa_key_id_t nwk_ecb,
					 uint8_t *frame, size_t payload_len)
{
	int ret;

	ret = lwan_crypto_decrypt_join_accept(nwk_ecb, &frame[MHDR_SIZE],
					      payload_len);
	if (ret != 0) {
		LOG_ERR("Join accept decrypt failed: %d", ret);
	}

	return ret;
}

static int mac_parse_join_accept(struct lwan_ctx *ctx,
				 const uint8_t *rx_buf, size_t rx_len,
				 psa_key_id_t nwk_cmac, psa_key_id_t nwk_ecb,
				 uint16_t dev_nonce)
{
	uint8_t frame[MHDR_SIZE + JA_MAX_SIZE];
	size_t payload_len;
	int ret;

	ret = mac_validate_join_accept_size(rx_len, &payload_len);
	if (ret != 0) {
		return ret;
	}

	memcpy(frame, rx_buf, rx_len);

	ret = mac_decrypt_join_accept_frame(nwk_ecb, frame, payload_len);
	if (ret != 0) {
		return ret;
	}

	ret = mac_verify_join_accept_mic(nwk_cmac, frame, rx_len);
	if (ret != 0) {
		return ret;
	}

	return mac_apply_join_accept(ctx, frame, rx_len, nwk_ecb, dev_nonce);
}

static enum mac_rx_result join_rx_handler(struct lwan_ctx *ctx,
					   const uint8_t *rx_buf, size_t rx_len,
					   int16_t rssi, int8_t snr,
					   void *user_data)
{
	const struct join_rx_ctx *jctx = user_data;

	ARG_UNUSED(rssi);
	ARG_UNUSED(snr);

	if ((rx_buf[0] & MHDR_TYPE_MASK) != MHDR_JOIN_ACCEPT) {
		return MAC_RX_CONTINUE;
	}

	if ((rx_buf[0] & MHDR_MAJOR_MASK) != MHDR_MAJOR_R1) {
		return MAC_RX_CONTINUE;
	}

	if (mac_parse_join_accept(ctx, rx_buf, rx_len,
				  jctx->nwk_cmac, jctx->nwk_ecb,
				  jctx->dev_nonce) != 0) {
		return MAC_RX_CONTINUE;
	}

	return MAC_RX_DONE;
}

static int select_join_channel_wait(struct lwan_ctx *ctx, uint32_t *freq,
				    uint8_t *dr)
{
	const struct lwan_region_ops *region = ctx->region;
	int32_t delay_ms;
	int ret;

	do {
		ret = region->select_join_channel(ctx->channels,
						  ctx->channel_count,
						  freq, dr, &delay_ms);
		if (ret == -ENOBUFS) {
			LOG_INF("Duty cycle: waiting %d ms", delay_ms);
			k_msleep(delay_ms);
		}
	} while (ret == -ENOBUFS);

	return ret;
}

static void join_state_init(struct join_state *state,
			    const struct lwan_join_req *req)
{
	state->req = req;
	state->rx_ctx = (struct join_rx_ctx){
		.dev_nonce = req->dev_nonce,
	};
	state->frame_len = sizeof(state->frame);
}

static int join_import_nwk_key(struct join_state *state)
{
	state->rx_ctx.nwk_cmac =
		lwan_crypto_import_cmac_key(state->req->nwk_key);
	state->rx_ctx.nwk_ecb =
		lwan_crypto_import_ecb_key(state->req->nwk_key);

	if (state->rx_ctx.nwk_cmac == 0 || state->rx_ctx.nwk_ecb == 0) {
		LOG_ERR("Failed to import NwkKey");
		return -EIO;
	}

	return 0;
}

static int join_prepare_frame(struct join_state *state)
{
	return mac_build_join_request(state->req, state->rx_ctx.nwk_cmac,
				      state->frame, &state->frame_len);
}

static int join_select_channel(struct lwan_ctx *ctx, struct join_state *state)
{
	int ret;

	ret = select_join_channel_wait(ctx, &state->tx_freq,
				       &state->tx_dr_idx);
	if (ret != 0) {
		LOG_ERR("No join channel available: %d", ret);
	}

	return ret;
}

static struct mac_tx_params join_tx_params(struct join_state *state)
{
	return (struct mac_tx_params){
		.frame = state->frame,
		.frame_len = state->frame_len,
		.tx_freq = state->tx_freq,
		.tx_dr_idx = state->tx_dr_idx,
		.rx1_delay_ms = JOIN_ACCEPT_DELAY1_MS,
		.post_tx = NULL,
		.rx_handler = join_rx_handler,
		.user_data = &state->rx_ctx,
	};
}

static int join_tx_rx(struct lwan_ctx *ctx, struct join_state *state)
{
	struct mac_tx_params tx_params = join_tx_params(state);

	return mac_do_tx_rx(ctx, &tx_params);
}

static void join_log_result(int ret)
{
	if (ret == 0) {
		LOG_INF("Joined successfully");
	} else if (ret == -ETIMEDOUT) {
		LOG_WRN("Join failed: no join accept received");
	} else {
		LOG_ERR("Join failed: %d", ret);
	}
}

static void join_destroy_keys(struct join_state *state)
{
	psa_destroy_key(state->rx_ctx.nwk_cmac);
	psa_destroy_key(state->rx_ctx.nwk_ecb);
}

void mac_do_join(struct lwan_ctx *ctx, const struct lwan_req *req)
{
	const struct lwan_join_req *join_req = req->data;
	struct join_state state;
	int ret;

	join_state_init(&state, join_req);

	/* Import NwkKey into PSA — plaintext is not stored */
	ret = join_import_nwk_key(&state);
	if (ret != 0) {
		goto done;
	}

	ret = join_prepare_frame(&state);
	if (ret != 0) {
		goto done;
	}

	ret = join_select_channel(ctx, &state);
	if (ret != 0) {
		goto done;
	}

	ret = join_tx_rx(ctx, &state);
	join_log_result(ret);

done:
	join_destroy_keys(&state);
	engine_signal_result(req, ret);
}
