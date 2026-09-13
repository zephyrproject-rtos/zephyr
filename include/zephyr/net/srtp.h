/*
 * SPDX-FileCopyrightText: 2026 Sayed Naser Moravej
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_SRTP_H_
#define ZEPHYR_INCLUDE_NET_SRTP_H_

#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/rtp.h>

#include <srtp.h>
#ifdef __cplusplus
extern "C" {
#endif
#define MAX_KEY_LEN  96

typedef struct policy_params_t {
    bool gcm_on;
    size_t key_size;
    size_t tag_size;
    srtp_sec_serv_t sec_servs;
} policy_params_t;

srtp_err_status_t create_policy_from_params(srtp_policy_t *policy,
                                            const policy_params_t *params);
					    
struct srtp_session {
	srtp_ctx_t *srtp_ctx;
	srtp_policy_t policy;
	char key[MAX_KEY_LEN];
	size_t key_len;
	size_t salt_len;
	struct rtp_session *rtp_se;
};

/**
 * @brief Initialize an RTP header extension.
 *
 * @param hdr_x      Pointer to the header extension struct to initialize.
 * @param definition Profile-defined extension header identifier.
 * @param data       Pointer to the extension data.
 * @param len        Length of @p data in bytes; must be aligned to a 32-bit word boundary.
 *
 * @retval 0        On success.
 * @retval negative Errno value on failure.
 */
int srtp_init_header_extension(struct rtp_header_extension *hdr_x, uint16_t definition,
			      uint8_t *data, size_t len);

/**
 * @brief Initialize an RTP session.
 *
 * Configure a session for transmit, receive, or both, as determined by @p role.
 *
 * @param session        Pointer to the RTP session to initialize.
 * @param iface          Network interface to use.
 * @param sock_addr      Session address and port (multicast group or unicast peer). The application
 *                       should be aware of the fact that unicast sinks accept RTP packets from any
 *                       sender.
 * @param role           Whether the session acts as a sink, source, or both.
 * @param payload_type   RTP payload type field value used when transmitting; ignored
 *                       when @p role is @ref RTP_ROLE_SINK.
 * @param callback       Callback invoked on packet reception; required when @p role is
 *                       @ref RTP_ROLE_SINK or @ref RTP_ROLE_BOTH.
 * @param user_data      Opaque user data forwarded to @p callback.
 *
 * @retval 0        On success.
 * @retval negative Errno value on failure.
 */
int srtp_session_init(struct srtp_session *session, struct net_if *iface,
		     struct net_sockaddr *sock_addr, enum rtp_role role, uint8_t payload_type,
		     rtp_rx_cb_t callback, void *user_data);

/**
 * @brief Initialize a receive-only RTP session.
 *
 * Convenience wrapper around @ref rtp_session_init that configures the session
 * for receive only.
 *
 * @param session        Pointer to the RTP session to initialize.
 * @param iface          Network interface to use.
 * @param sock_addr      Session address and port to listen on.
 * @param callback       Callback invoked when a packet is received.
 * @param user_data      Opaque user data forwarded to @p callback.
 *
 * @retval 0        On success.
 * @retval negative Errno value on failure.
 */
static inline int srtp_session_init_rx(struct srtp_session *session, struct net_if *iface,
				      struct net_sockaddr *sock_addr, rtp_rx_cb_t callback,
				      void *user_data)
{
	return srtp_session_init(session, iface, sock_addr, RTP_ROLE_SINK, 0, callback, user_data);
}

/**
 * @brief Initialize a transmit-only RTP session.
 *
 * Convenience wrapper around @ref rtp_session_init that configures the session
 * for transmit only.
 *
 * @param session        Pointer to the RTP session to initialize.
 * @param iface          Network interface to use.
 * @param sock_addr      Session address and port to send packets to.
 * @param payload_type   RTP payload type field value.
 *
 * @retval 0        On success.
 * @retval negative Errno value on failure.
 */
static inline int srtp_session_init_tx(struct srtp_session *session, struct net_if *iface,
				      struct net_sockaddr *sock_addr, uint8_t payload_type)
{
	return srtp_session_init(session, iface, sock_addr, RTP_ROLE_SOURCE, payload_type, NULL,
				NULL);
}

/**
 * @brief Start an RTP session.
 *
 * Open the network connections and begin processing packets. The session must
 * have been configured with @ref rtp_session_init before calling this function.
 *
 * @param session Pointer to the RTP session.
 *
 * @retval 0        On success.
 * @retval negative Errno value on failure.
 */
int srtp_session_start(struct srtp_session *session);

/**
 * @brief Stop an RTP session.
 *
 * Close the network connections and stop processing packets.
 *
 * @param session Pointer to the RTP session.
 *
 * @retval 0        On success.
 * @retval negative Errno value on failure.
 */
int rtp_session_stop(struct rtp_session *session);

/**
 * @brief Add a contributing source (CSRC) to the session.
 *
 * @param session Pointer to the RTP session.
 * @param csrc    CSRC identifier to add.
 *
 * @retval 0        On success.
 * @retval negative Errno value on failure.
 */
int srtp_session_add_csrc(struct srtp_session *session, uint32_t csrc);

/**
 * @brief Remove a contributing source (CSRC) from the session.
 *
 * @param session Pointer to the RTP session.
 * @param csrc    CSRC identifier to remove.
 *
 * @retval 0        On success.
 * @retval negative Errno value on failure.
 */
int srtp_session_remove_csrc(struct srtp_session *session, uint32_t csrc);

/**
 * @brief Send an RTP packet with an optional header extension and optional padding.
 *
 * @param session        Pointer to the RTP session.
 * @param data           Pointer to the payload data to send.
 * @param len            Length of @p data in bytes.
 * @param delta_ts       Timestamp increment to apply; for audio this is typically the number of
 *                       sample periods covered by the payload.
 * @param padding        Number of padding bytes to append (0–255); pass 0 for no padding.
 * @param marker         Marker bit value; use @ref RTP_MARKER to set the bit or @c 0 to clear it.
 * @param hdr_x          Pointer to a header extension, or NULL for no extension.
 * @param[out] timestamp Pointer to store the RTP timestamp used in the packet. Can be left
 *                       NULL if the timestamp is of no interest for the caller.
 *
 * @retval 0        On success.
 * @retval negative Errno value on failure.
 */
int srtp_session_send(struct srtp_session *session, void *data, size_t len, uint32_t delta_ts,
		     uint8_t padding, uint8_t marker, struct rtp_header_extension *hdr_x,
		     uint32_t *timestamp);

/**
 * @brief Send an RTP packet without a header extension, padding or marker.
 *
 * Convenience wrapper around @ref srtp_session_send.
 *
 * @param session  Pointer to the SRTP session.
 * @param data     Pointer to the payload data to send.
 * @param len      Length of @p data in bytes.
 * @param delta_ts Timestamp increment to apply; for audio this is typically the number of
 *                 sample periods covered by the payload.
 *
 * @retval 0        On success.
 * @retval negative Errno value on failure.
 */
static inline int srtp_session_send_simple(struct srtp_session *session, void *data, size_t len,
					  uint32_t delta_ts)
{
	return srtp_session_send(session, data, len, delta_ts, 0, 0, NULL, NULL);
}

// /**
//  * @brief Get the RTP version (V) field.
//  *
//  * @param h RTP header to read.
//  *
//  * @return Version field value.
//  */
// static inline uint8_t rtp_header_get_v(const struct rtp_header *h)
// {
// 	__ASSERT_NO_MSG(h != NULL);

// 	return FIELD_GET(RTP_HDR_V_MASK, h->vpxcc);
// }

// /**
//  * @brief Get the padding (P) flag.
//  *
//  * @param h RTP header to read.
//  *
//  * @return Padding flag value.
//  */
// static inline uint8_t rtp_header_get_p(const struct rtp_header *h)
// {
// 	__ASSERT_NO_MSG(h != NULL);

// 	return FIELD_GET(RTP_HDR_P_MASK, h->vpxcc);
// }

// /**
//  * @brief Get the extension (X) flag.
//  *
//  * @param h RTP header to read.
//  *
//  * @return Extension flag value.
//  */
// static inline uint8_t rtp_header_get_x(const struct rtp_header *h)
// {
// 	__ASSERT_NO_MSG(h != NULL);

// 	return FIELD_GET(RTP_HDR_X_MASK, h->vpxcc);
// }

// /**
//  * @brief Get the CSRC count (CC) field.
//  *
//  * @param h RTP header to read.
//  *
//  * @return CSRC count field value.
//  */
// static inline uint8_t rtp_header_get_cc(const struct rtp_header *h)
// {
// 	__ASSERT_NO_MSG(h != NULL);

// 	return FIELD_GET(RTP_HDR_CC_MASK, h->vpxcc);
// }

// /**
//  * @brief Get the marker (M) bit.
//  *
//  * @param h RTP header to read.
//  *
//  * @return Marker bit value.
//  */
// static inline uint8_t rtp_header_get_m(const struct rtp_header *h)
// {
// 	__ASSERT_NO_MSG(h != NULL);

// 	return FIELD_GET(RTP_HDR_M_MASK, h->mpt);
// }

// /**
//  * @brief Get the payload type (PT) field.
//  *
//  * @param h RTP header to read.
//  *
//  * @return Payload type field value.
//  */
// static inline uint8_t rtp_header_get_pt(const struct rtp_header *h)
// {
// 	__ASSERT_NO_MSG(h != NULL);

// 	return FIELD_GET(RTP_HDR_PT_MASK, h->mpt);
// }

// /**
//  * @brief Set the RTP version (V) field.
//  *
//  * @param h     RTP header to modify.
//  * @param value Version field value.
//  */
// static inline void rtp_header_set_v(struct rtp_header *h, uint8_t value)
// {
// 	__ASSERT_NO_MSG(h != NULL);

// 	h->vpxcc &= ~RTP_HDR_V_MASK;
// 	h->vpxcc |= FIELD_PREP(RTP_HDR_V_MASK, value);
// }

// /**
//  * @brief Set the padding (P) flag.
//  *
//  * @param h     RTP header to modify.
//  * @param value Padding flag value.
//  */
// static inline void rtp_header_set_p(struct rtp_header *h, uint8_t value)
// {
// 	__ASSERT_NO_MSG(h != NULL);

// 	h->vpxcc &= ~RTP_HDR_P_MASK;
// 	h->vpxcc |= FIELD_PREP(RTP_HDR_P_MASK, value);
// }

// /**
//  * @brief Set the extension (X) flag.
//  *
//  * @param h     RTP header to modify.
//  * @param value Extension flag value.
//  */
// static inline void rtp_header_set_x(struct rtp_header *h, uint8_t value)
// {
// 	__ASSERT_NO_MSG(h != NULL);

// 	h->vpxcc &= ~RTP_HDR_X_MASK;
// 	h->vpxcc |= FIELD_PREP(RTP_HDR_X_MASK, value);
// }

// /**
//  * @brief Set the CSRC count (CC) field.
//  *
//  * @param h     RTP header to modify.
//  * @param value CSRC count field value.
//  */
// static inline void rtp_header_set_cc(struct rtp_header *h, uint8_t value)
// {
// 	__ASSERT_NO_MSG(h != NULL);

// 	h->vpxcc &= ~RTP_HDR_CC_MASK;
// 	h->vpxcc |= FIELD_PREP(RTP_HDR_CC_MASK, value);
// }

// /**
//  * @brief Set the marker (M) bit.
//  *
//  * @param h     RTP header to modify.
//  * @param value Marker bit value.
//  */
// static inline void rtp_header_set_m(struct rtp_header *h, uint8_t value)
// {
// 	__ASSERT_NO_MSG(h != NULL);

// 	h->mpt &= ~RTP_HDR_M_MASK;
// 	h->mpt |= FIELD_PREP(RTP_HDR_M_MASK, value);
// }

// /**
//  * @brief Set the payload type (PT) field.
//  *
//  * @param h     RTP header to modify.
//  * @param value Payload type field value.
//  */
// static inline void rtp_header_set_pt(struct rtp_header *h, uint8_t value)
// {
// 	__ASSERT_NO_MSG(h != NULL);

// 	h->mpt &= ~RTP_HDR_PT_MASK;
// 	h->mpt |= FIELD_PREP(RTP_HDR_PT_MASK, value);
// }
#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_SRTP_H_ */
