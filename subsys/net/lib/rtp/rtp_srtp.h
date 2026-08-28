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

/* session->srtp may be set to NULL at any time by rtp_session_clear_srtp().
 * Helpers must load the pointer once and only use that snapshot; re-reading
 * it mid-operation risks a NULL dereference or a half-observed state.
 */

static inline bool rtp_srtp_tx_enabled(const struct rtp_session *session)
{
	const struct srtp_session_ctx *srtp_ctx = session->srtp;

	return srtp_ctx != NULL && srtp_ctx->tx_enabled;
}

static inline bool rtp_srtp_rx_enabled(const struct rtp_session *session)
{
	const struct srtp_session_ctx *srtp_ctx = session->srtp;

	return srtp_ctx != NULL && srtp_ctx->rx_enabled;
}

/**
 * @brief Protect a serialized RTP packet with the session's transmit stream.
 *
 * @param session  RTP session with SRTP enabled for transmit.
 * @param buf      Buffer holding the serialized RTP packet.
 * @param pkt_len  Length of the RTP packet in bytes.
 * @param buf_size Size of @p buf; must allow @ref SRTP_MAX_TRAILER_LEN bytes
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
 * @brief Unprotect a serialized SRTP packet, late binding receive streams.
 *
 * Looks up the receive stream by the packet's SSRC, instantiating one from
 * the session's receive policy for previously unseen sources. A stream bound
 * by a packet that subsequently fails authentication is released again, so
 * unauthenticated traffic cannot pin receive stream slots.
 *
 * @param session RTP session with SRTP enabled for receive.
 * @param buf     Buffer holding the serialized SRTP packet.
 * @param pkt_len Length of the SRTP packet in bytes.
 * @param out_len Location to store the plain RTP packet length.
 *
 * @retval 0        On success.
 * @retval negative Errno value on failure; the packet must be dropped.
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

#endif /* CONFIG_SRTP */

#endif /* ZEPHYR_SUBSYS_NET_LIB_RTP_RTP_SRTP_H_ */
