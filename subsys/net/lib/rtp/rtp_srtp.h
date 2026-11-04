/*
 *  SPDX-FileCopyrightText: 2026 Basalte bv
 *
 *  SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file rtp_srtp.h
 * @brief Internal SRTP hooks for the RTP transports
 */

#ifndef ZEPHYR_SUBSYS_NET_LIB_RTP_RTP_SRTP_H_
#define ZEPHYR_SUBSYS_NET_LIB_RTP_RTP_SRTP_H_

#include <zephyr/net/rtp.h>

#ifdef CONFIG_SRTP

#include <zephyr/net/srtp.h>

/* rtp_session_clear_srtp() unpublishes session->srtp under the session lock
 * and may then destroy the context. The transmit helpers run under that
 * lock, held by rtp_session_send(), and read the pointer directly. The
 * receive path runs without it and takes a hold on the context with
 * rtp_srtp_rx_get() instead, which the teardown waits for.
 */

static inline bool rtp_srtp_tx_enabled(const struct rtp_session *session)
{
	const struct srtp_session_ctx *srtp_ctx = session->srtp;

	return srtp_ctx != NULL && srtp_ctx->tx_enabled;
}

/**
 * @brief Protect a serialized RTP packet with the session's transmit stream.
 *
 * @param session  RTP session with SRTP enabled for transmit.
 * @param buf      Buffer holding the serialized RTP packet.
 * @param pkt_len  Length of the RTP packet in bytes.
 * @param buf_size Size of @p buf; must allow @ref SRTP_TRAILER_MAX_LEN bytes
 *                 of growth beyond @p pkt_len.
 * @param out_len  Location to store the protected packet length.
 *
 * @retval 0        On success.
 * @retval -ENOENT  When SRTP was concurrently cleared from the session.
 * @retval negative Other errno value on failure.
 */
int rtp_srtp_protect(struct rtp_session *session, uint8_t *buf, size_t pkt_len, size_t buf_size,
		     size_t *out_len);

/**
 * @brief Take a hold on the session's SRTP context for receiving a packet.
 *
 * The hold keeps the context, its streams and its locks valid until it is
 * released with rtp_srtp_rx_put(); rtp_session_clear_srtp() waits for all
 * holds to drain before destroying the context.
 *
 * @param session  RTP session.
 * @param srtp_ctx Location to store the held context.
 *
 * @retval 0        On success.
 * @retval -ENOENT  When the session has no receive protection installed or
 *                  it is being cleared; the packet is plain RTP.
 * @retval negative Other errno value when the session lock is unavailable.
 */
int rtp_srtp_rx_get(struct rtp_session *session, struct srtp_session_ctx **srtp_ctx);

/**
 * @brief Release a hold taken with rtp_srtp_rx_get().
 *
 * @param srtp_ctx Held SRTP context.
 */
void rtp_srtp_rx_put(struct srtp_session_ctx *srtp_ctx);

/**
 * @brief Unprotect a serialized SRTP packet, late binding receive streams.
 *
 * Looks up the receive stream by the packet's SSRC, instantiating one from
 * the session's receive policy for previously unseen sources. A stream bound
 * by a packet that subsequently fails authentication is released again, so
 * unauthenticated traffic cannot pin receive stream slots.
 *
 * @param srtp_ctx SRTP context held with rtp_srtp_rx_get().
 * @param buf      Buffer holding the serialized SRTP packet.
 * @param pkt_len  Length of the SRTP packet in bytes.
 * @param out_len  Location to store the plain RTP packet length.
 *
 * @retval 0        On success.
 * @retval negative Errno value on failure; the packet must be dropped.
 */
int rtp_srtp_ctx_unprotect(struct srtp_session_ctx *srtp_ctx, uint8_t *buf, size_t pkt_len,
			   size_t *out_len);

/**
 * @brief Unprotect a serialized SRTP packet received on a session.
 *
 * Holds the session's SRTP context for the duration of the call, see
 * rtp_srtp_rx_get() and rtp_srtp_ctx_unprotect().
 *
 * @param session RTP session.
 * @param buf     Buffer holding the serialized SRTP packet.
 * @param pkt_len Length of the SRTP packet in bytes.
 * @param out_len Location to store the plain RTP packet length.
 *
 * @retval 0        On success.
 * @retval -ENOENT  When the session has no receive protection installed or
 *                  it is being cleared.
 * @retval negative Other errno value on failure; the packet must be dropped.
 */
int rtp_srtp_unprotect(struct rtp_session *session, uint8_t *buf, size_t pkt_len, size_t *out_len);

/**
 * @brief Check whether the session's SRTP transmit stream has sent packets.
 *
 * @param session Pointer to the RTP session.
 *
 * @return true when SRTP is installed for transmit and at least one packet
 *         was protected, meaning the RTP sequence number must not be reset.
 */
bool rtp_srtp_tx_started(const struct rtp_session *session);

#else

static inline bool rtp_srtp_tx_started(const struct rtp_session *session)
{
	ARG_UNUSED(session);

	return false;
}

static inline bool rtp_srtp_tx_enabled(const struct rtp_session *session)
{
	ARG_UNUSED(session);

	return false;
}

#endif /* CONFIG_SRTP */

#endif /* ZEPHYR_SUBSYS_NET_LIB_RTP_RTP_SRTP_H_ */
