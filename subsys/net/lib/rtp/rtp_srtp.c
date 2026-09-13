/*
 *  SPDX-FileCopyrightText: 2026 Basalte bv
 *
 *  SPDX-License-Identifier: Apache-2.0
 */

/** @file rtp_srtp.c
 *
 * @brief SRTP integration for RTP sessions.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(rtp_srtp, CONFIG_RTP_LOG_LEVEL);

#include <zephyr/net/net_log.h>
#include <zephyr/net/rtp.h>
#include <zephyr/net/srtp.h>
#include <zephyr/sys/byteorder.h>

#include "rtp_srtp.h"
#include "srtp_crypto.h"

#define BIND_LOCK_TIMEOUT K_MSEC(100)

int rtp_session_set_srtp(struct rtp_session *session, const struct srtp_policy *tx_policy,
			 const struct srtp_policy *rx_policy, struct srtp_session_ctx *srtp_ctx)
{
	int ret;

	if (session == NULL || srtp_ctx == NULL || (tx_policy == NULL && rx_policy == NULL)) {
		return -EINVAL;
	}

	if (session->srtp != NULL) {
		return -EALREADY;
	}

	memset(srtp_ctx, 0, sizeof(*srtp_ctx));
	k_mutex_init(&srtp_ctx->lock);
#ifdef CONFIG_RTP_TRANSPORT_NET_PKT
	k_mutex_init(&srtp_ctx->net_pkt_lock);
#endif /* CONFIG_RTP_TRANSPORT_NET_PKT */

	if (tx_policy != NULL) {
		ret = srtp_stream_init(&srtp_ctx->tx, tx_policy, session->ssrc);
		if (ret < 0) {
			return ret;
		}

		srtp_ctx->tx_enabled = true;
	}

	if (rx_policy != NULL) {
		ret = srtp_policy_validate(rx_policy);
		if (ret < 0) {
			goto fail;
		}

		/* Keep a copy of the policy and key material for late binding
		 * of receive streams to their SSRC; srtp_policy_validate()
		 * bounds the lengths to the destination sizes.
		 */
		srtp_ctx->rx_policy = *rx_policy;

		if (rx_policy->cipher != SRTP_CIPHER_NULL || rx_policy->auth != SRTP_AUTH_NULL) {
			memcpy(srtp_ctx->rx_master_key, rx_policy->master_key,
			       rx_policy->master_key_len);
			memcpy(srtp_ctx->rx_master_salt, rx_policy->master_salt,
			       rx_policy->master_salt_len);
			srtp_ctx->rx_policy.master_key = srtp_ctx->rx_master_key;
			srtp_ctx->rx_policy.master_salt = srtp_ctx->rx_master_salt;
		} else {
			srtp_ctx->rx_policy.master_key = NULL;
			srtp_ctx->rx_policy.master_salt = NULL;
		}

		if (rx_policy->mki_len > 0) {
			memcpy(srtp_ctx->rx_mki, rx_policy->mki, rx_policy->mki_len);
			srtp_ctx->rx_policy.mki = srtp_ctx->rx_mki;
		} else {
			srtp_ctx->rx_policy.mki = NULL;
		}

		srtp_ctx->rx_enabled = true;
	}

	session->srtp = srtp_ctx;

	return 0;

fail:
	if (srtp_ctx->tx_enabled) {
		(void)srtp_stream_deinit(&srtp_ctx->tx);
	}

	return ret;
}

int rtp_session_clear_srtp(struct rtp_session *session)
{
	struct srtp_session_ctx *srtp_ctx;

	if (session == NULL) {
		return -EINVAL;
	}

	srtp_ctx = session->srtp;
	if (srtp_ctx == NULL) {
		return 0;
	}

	if (srtp_ctx->tx_enabled) {
		(void)srtp_stream_deinit(&srtp_ctx->tx);
	}

	for (size_t i = 0; i < ARRAY_SIZE(srtp_ctx->rx); i++) {
		if (srtp_ctx->rx_bound[i]) {
			(void)srtp_stream_deinit(&srtp_ctx->rx[i]);
		}
	}

	srtp_crypto_zeroize(srtp_ctx->rx_master_key, sizeof(srtp_ctx->rx_master_key));
	srtp_crypto_zeroize(srtp_ctx->rx_master_salt, sizeof(srtp_ctx->rx_master_salt));
#ifdef CONFIG_RTP_TRANSPORT_NET_PKT
	srtp_crypto_zeroize(srtp_ctx->net_pkt_buf, sizeof(srtp_ctx->net_pkt_buf));
#endif /* CONFIG_RTP_TRANSPORT_NET_PKT */
	srtp_ctx->tx_enabled = false;
	srtp_ctx->rx_enabled = false;

	session->srtp = NULL;

	return 0;
}

bool rtp_srtp_tx_started(const struct rtp_session *session)
{
	const struct srtp_session_ctx *srtp_ctx = session->srtp;

	return srtp_ctx != NULL && srtp_ctx->tx_enabled && srtp_ctx->tx.seq_initialized;
}

int rtp_srtp_protect(struct rtp_session *session, uint8_t *buf, size_t pkt_len, size_t buf_size,
		     size_t *out_len)
{
	struct srtp_session_ctx *srtp_ctx = session->srtp;

	if (srtp_ctx == NULL || !srtp_ctx->tx_enabled) {
		return -ENOENT;
	}

	return srtp_protect(&srtp_ctx->tx, buf, pkt_len, buf_size, out_len);
}

static int rtp_srtp_rx_stream_get(struct srtp_session_ctx *srtp_ctx, uint32_t ssrc,
				  struct srtp_stream **stream)
{
	int free_slot = -1;
	int ret;

	*stream = NULL;

	ret = k_mutex_lock(&srtp_ctx->lock, BIND_LOCK_TIMEOUT);
	if (ret < 0) {
		return ret;
	}

	for (size_t i = 0; i < ARRAY_SIZE(srtp_ctx->rx); i++) {
		if (srtp_ctx->rx_bound[i]) {
			if (srtp_ctx->rx[i].ssrc == ssrc) {
				*stream = &srtp_ctx->rx[i];
				break;
			}
		} else if (free_slot < 0) {
			free_slot = i;
		}
	}

	if (*stream == NULL) {
		if (free_slot < 0) {
			NET_DBG("No SRTP receive stream for SSRC %u, consider increasing "
				"CONFIG_SRTP_MAX_RX_STREAMS",
				ssrc);
			ret = -ENOMEM;
		} else {
			ret = srtp_stream_init(&srtp_ctx->rx[free_slot], &srtp_ctx->rx_policy,
					       ssrc);
			if (ret == 0) {
				srtp_ctx->rx_bound[free_slot] = true;
				*stream = &srtp_ctx->rx[free_slot];
				NET_DBG("Bound SRTP receive stream for SSRC %u", ssrc);
			}
		}
	}

	if (*stream != NULL) {
		srtp_ctx->rx_users[*stream - srtp_ctx->rx]++;
	}

	(void)k_mutex_unlock(&srtp_ctx->lock);

	return ret;
}

/* Drop the caller's hold on the stream. The last packet leaving a stream
 * that has yet to authenticate one unbinds it, so unauthenticated traffic
 * cannot pin a slot; a stream a concurrent packet authenticated stays
 * bound with its replay state intact. A held stream is never unbound, so
 * its keys cannot be destroyed or its slot rebound mid-operation.
 *
 * K_FOREVER: giving up would leak the hold and pin the slot; the lock is
 * only ever held for bounded binding-table work.
 */
static void rtp_srtp_rx_stream_put(struct srtp_session_ctx *srtp_ctx, struct srtp_stream *stream)
{
	size_t slot = stream - srtp_ctx->rx;

	(void)k_mutex_lock(&srtp_ctx->lock, K_FOREVER);

	srtp_ctx->rx_users[slot]--;

	if (srtp_ctx->rx_users[slot] == 0U && !stream->seq_initialized) {
		NET_DBG("Unbound SRTP receive stream for SSRC %u", stream->ssrc);
		(void)srtp_stream_deinit(stream);
		srtp_ctx->rx_bound[slot] = false;
	}

	(void)k_mutex_unlock(&srtp_ctx->lock);
}

int rtp_srtp_unprotect(struct rtp_session *session, uint8_t *buf, size_t pkt_len, size_t *out_len)
{
	struct srtp_session_ctx *srtp_ctx = session->srtp;
	struct srtp_stream *stream;
	int ret;

	if (srtp_ctx == NULL || !srtp_ctx->rx_enabled) {
		return -ENOENT;
	}

	if (pkt_len < RTP_MIN_HEADER_LEN) {
		return -EBADMSG;
	}

	ret = rtp_srtp_rx_stream_get(srtp_ctx, sys_get_be32(&buf[8]), &stream);
	if (ret < 0) {
		return ret;
	}

	ret = srtp_unprotect(stream, buf, pkt_len, out_len);

	rtp_srtp_rx_stream_put(srtp_ctx, stream);

	return ret;
}
