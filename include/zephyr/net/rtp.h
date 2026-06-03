/*
 *  SPDX-FileCopyrightText: 2026 Basalte bv
 *
 *  SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief RTP (Real-time Transport Protocol) API
 */

#ifndef ZEPHYR_INCLUDE_NET_RTP_H_
#define ZEPHYR_INCLUDE_NET_RTP_H_

#include <zephyr/kernel.h>
#include <zephyr/logging/log_core.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/socket.h>
#ifdef CONFIG_NET_PKT_TIMESTAMP
#include <zephyr/net/ptp_time.h>
#endif
#include <zephyr/net/net_context.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup rtp RTP (Real-time Transport Protocol)
 * @version 0.1.0
 * @ingroup networking
 * @{
 */

/** RTP protocol version 2 as defined in RFC 3550. */
#define RTP_VERSION 2

/** Minimum RTP header length in bytes, excluding any CSRC list or extension. */
#define RTP_MIN_HEADER_LEN 12

/** Maximum value for the payload type in an RTP packet */
#define RTP_PAYLOAD_TYPE_MAX 127

/** Convenience value for the @p marker argument of @ref rtp_session_send that sets the
 *  RTP marker bit. Pass @c 0 to clear the bit.
 */
#define RTP_MARKER 1

/** Mask for the version (V) field within @ref rtp_header.vpxcc. */
#define RTP_HDR_V_MASK  GENMASK(7, 6)
/** Mask for the padding (P) flag within @ref rtp_header.vpxcc. */
#define RTP_HDR_P_MASK  BIT(5)
/** Mask for the extension (X) flag within @ref rtp_header.vpxcc. */
#define RTP_HDR_X_MASK  BIT(4)
/** Mask for the CSRC count (CC) field within @ref rtp_header.vpxcc. */
#define RTP_HDR_CC_MASK GENMASK(3, 0)
/** Mask for the marker (M) bit within @ref rtp_header.mpt. */
#define RTP_HDR_M_MASK  BIT(7)
/** Mask for the payload type (PT) field within @ref rtp_header.mpt. */
#define RTP_HDR_PT_MASK GENMASK(6, 0)

#if defined(CONFIG_RTP_MAX_CSRC_COUNT) || defined(__DOXYGEN__)
/** Maximum number of csrc's that can be stored */
#define RTP_MAX_CSRC_COUNT CONFIG_RTP_MAX_CSRC_COUNT
#else
#define RTP_MAX_CSRC_COUNT 0
#endif

/** RTP header extension as defined in RFC 3550 section 5.3.1. */
struct rtp_header_extension {
	/** Profile-defined extension header identifier. */
	uint16_t definition;
	/** Length of the extension data in 32-bit words. */
	uint16_t length;
	/** Pointer to the extension data. */
	uint8_t *data;
};

/** RTP fixed header as defined in RFC 3550 section 5.1. */
struct rtp_header {
	/** Raw encoding of the version, padding, extension, and CC fields. */
	uint8_t vpxcc;
	/** Raw encoding of the marker, and payload type fields. */
	uint8_t mpt;
	/** Sequence number, incremented by one for each RTP data packet sent. */
	uint16_t seq;
	/** Timestamp reflecting the sampling instant of the first octet in the payload. */
	uint32_t ts;
	/** Synchronization source (SSRC) identifier, chosen randomly and intended to be globally
	 * unique.
	 */
	uint32_t ssrc;
	/** Contributing source (CSRC) identifiers, up to @kconfig{CONFIG_RTP_MAX_CSRC_COUNT}
	 * entries.
	 */
	uint32_t csrc[RTP_MAX_CSRC_COUNT];
	/** Optional header extension; valid only when the @c x bit is set. */
	struct rtp_header_extension header_extension;
};

/** Decoded RTP packet with header and payload separated. */
struct rtp_packet {
	/** Parsed RTP header. */
	struct rtp_header header;
	/** Pointer to the packet payload. Valid only for the duration of the receive callback. */
	uint8_t *payload;
	/** Length of the payload in bytes. */
	size_t payload_len;

#ifdef CONFIG_NET_PKT_TIMESTAMP
	/** Network packet reception timestamp. Only available when
	 * @kconfig{CONFIG_NET_PKT_TIMESTAMP} is enabled.
	 */
	struct net_ptp_time timestamp;
#endif
};

struct rtp_session;

/**
 * @brief RTP packet receive callback type.
 *
 * Called from the network receive context when an RTP packet arrives. Keep
 * the callback short and non-blocking. The @p packet payload pointer is only
 * valid for the duration of the callback.
 *
 * The callback may still be invoked concurrently with, or immediately after,
 * a stop of the session's receive path from another thread; a session being
 * stopped is not a guarantee that no further callback is in flight or about
 * to be dispatched. Implementations must tolerate a call landing after they
 * consider the session stopped.
 *
 * @param session   Pointer to the RTP session that received the packet.
 * @param packet    Pointer to the received RTP packet.
 * @param user_data Opaque user data supplied during session initialization.
 */
typedef void (*rtp_rx_cb_t)(struct rtp_session *session, struct rtp_packet *packet,
			    void *user_data);

/** Role of an RTP session. */
enum rtp_role {
	/** Session receives packets only. */
	RTP_ROLE_SINK,
	/** Session transmits packets only. */
	RTP_ROLE_SOURCE,
	/** Session both transmits and receives packets. */
	RTP_ROLE_BOTH,
};

/** RTP session configuration context. */
struct rtp_session_context {
	/** Network interface used by the session. */
	struct net_if *iface;

	/** Session address and port (multicast group address or unicast peer). */
	struct net_sockaddr_storage sock_addr;
	/** Role of the session: sink, source, or both. */
	enum rtp_role role;

	/** RTP payload type field value used when transmitting. */
	uint8_t payload_type;

	/** Callback invoked upon packet reception; required when role is @ref RTP_ROLE_SINK or
	 *  @ref RTP_ROLE_BOTH.
	 */
	rtp_rx_cb_t callback;
	/** Opaque user data forwarded to @p callback. */
	void *user_data;
};

/** Transport backend used by an RTP session. Passed to @ref rtp_session_init. */
enum rtp_transport_type {
#ifdef CONFIG_RTP_TRANSPORT_SOCKET
	/** Socket-based transport using the Zephyr BSD socket API with a background socket
	 *  service for polling. Suitable for general use.
	 */
	RTP_TRANSPORT_SOCKET,
#endif /* CONFIG_RTP_TRANSPORT_SOCKET */

	/** Shall not be used as a transport type.
	 *  Indicator of maximum transport types possible.
	 */
	RTP_TRANSPORT_NUM,
};

BUILD_ASSERT(RTP_TRANSPORT_NUM > 0, "Please select at least 1 RTP transport backend");

/** Internal transport state embedded in @ref rtp_session. Holds the active transport type
 *  and all transport-specific handles needed to send and receive packets.
 */
struct rtp_transport {
	/** Active transport backend for this session. */
	enum rtp_transport_type type;

	/** Union to hold transport related data */
	union {
#ifdef CONFIG_RTP_TRANSPORT_SOCKET
		struct {
			/** RX socket file descriptor for socket transport; -1 when not open. */
			int socket_rx_fd;
			/** TX socket file descriptor for socket transport; -1 when not open. */
			int socket_tx_fd;
		};
#endif /* CONFIG_RTP_TRANSPORT_SOCKET */
	};
};

/**
 * RTP session state.
 *
 * Declare with @ref RTP_SESSION_DEFINE and configure with @ref rtp_session_init
 * before use.
 */
struct rtp_session {
	/** Transport info */
	struct rtp_transport transport;
	/** Session configuration context. */
	struct rtp_session_context rtp_context;

	/** Mutex protecting concurrent access to session state. */
	struct k_mutex lock;

	/** Synchronization source (SSRC) identifier for this session. */
	uint32_t ssrc;
	/** Current RTP timestamp, advanced with each transmitted packet. */
	uint32_t timestamp;
	/** Sequence number of the next packet to be transmitted. */
	uint16_t sequence_number;

	/** Local UDP port used when transmitting; 0 to auto-select (see
	 * @kconfig{CONFIG_RTP_LOCAL_PORT_BASE}).
	 */
	uint16_t local_port;

	/** Contributing source (CSRC) identifier list. */
	uint32_t csrc[RTP_MAX_CSRC_COUNT];
	/** Number of active entries in @p csrc. */
	size_t csrc_len;

#if CONFIG_RTP_LOG_LEVEL >= LOG_LEVEL_DBG
	/** Human-readable session name used in debug log messages. */
	const char *name;
#endif
};

/** @cond INTERNAL_HIDDEN */
#if CONFIG_RTP_LOG_LEVEL >= LOG_LEVEL_DBG
#define RTP_SESSION_NAME(_name) .name = STRINGIFY(_name),
#else
#define RTP_SESSION_NAME(_name)
#endif
/** @endcond */

/**
 * @brief Define an RTP session.
 *
 * Declares and zero-initializes a @ref rtp_session variable. Call
 * @ref rtp_session_init to configure the session before starting it with
 * @ref rtp_session_start.
 *
 * @param _name       Name of the @ref rtp_session variable to define.
 * @param _local_port Local UDP port used when transmitting packets.
 */
#define RTP_SESSION_DEFINE(_name, _local_port)                                                     \
	struct rtp_session _name = {.local_port = _local_port, RTP_SESSION_NAME(_name)}

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
int rtp_init_header_extension(struct rtp_header_extension *hdr_x, uint16_t definition,
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
 * @param transport_type Transport backend to use; see @ref rtp_transport_type.
 *
 * @retval 0        On success.
 * @retval negative Errno value on failure.
 */
int rtp_session_init(struct rtp_session *session, struct net_if *iface,
		     struct net_sockaddr *sock_addr, enum rtp_role role, uint8_t payload_type,
		     rtp_rx_cb_t callback, void *user_data, enum rtp_transport_type transport_type);

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
 * @param transport_type Transport backend to use; see @ref rtp_transport_type.
 *
 * @retval 0        On success.
 * @retval negative Errno value on failure.
 */
static inline int rtp_session_init_rx(struct rtp_session *session, struct net_if *iface,
				      struct net_sockaddr *sock_addr, rtp_rx_cb_t callback,
				      void *user_data, enum rtp_transport_type transport_type)
{
	return rtp_session_init(session, iface, sock_addr, RTP_ROLE_SINK, 0, callback, user_data,
				transport_type);
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
 * @param transport_type Transport backend to use; see @ref rtp_transport_type.
 *
 * @retval 0        On success.
 * @retval negative Errno value on failure.
 */
static inline int rtp_session_init_tx(struct rtp_session *session, struct net_if *iface,
				      struct net_sockaddr *sock_addr, uint8_t payload_type,
				      enum rtp_transport_type transport_type)
{
	return rtp_session_init(session, iface, sock_addr, RTP_ROLE_SOURCE, payload_type, NULL,
				NULL, transport_type);
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
int rtp_session_start(struct rtp_session *session);

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
int rtp_session_add_csrc(struct rtp_session *session, uint32_t csrc);

/**
 * @brief Remove a contributing source (CSRC) from the session.
 *
 * @param session Pointer to the RTP session.
 * @param csrc    CSRC identifier to remove.
 *
 * @retval 0        On success.
 * @retval negative Errno value on failure.
 */
int rtp_session_remove_csrc(struct rtp_session *session, uint32_t csrc);

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
int rtp_session_send(struct rtp_session *session, void *data, size_t len, uint32_t delta_ts,
		     uint8_t padding, uint8_t marker, struct rtp_header_extension *hdr_x,
		     uint32_t *timestamp);

/**
 * @brief Send an RTP packet without a header extension, padding or marker.
 *
 * Convenience wrapper around @ref rtp_session_send.
 *
 * @param session  Pointer to the RTP session.
 * @param data     Pointer to the payload data to send.
 * @param len      Length of @p data in bytes.
 * @param delta_ts Timestamp increment to apply; for audio this is typically the number of
 *                 sample periods covered by the payload.
 *
 * @retval 0        On success.
 * @retval negative Errno value on failure.
 */
static inline int rtp_session_send_simple(struct rtp_session *session, void *data, size_t len,
					  uint32_t delta_ts)
{
	return rtp_session_send(session, data, len, delta_ts, 0, 0, NULL, NULL);
}

/**
 * @brief Get the RTP version (V) field.
 *
 * @param h RTP header to read.
 *
 * @return Version field value.
 */
static inline uint8_t rtp_header_get_v(const struct rtp_header *h)
{
	__ASSERT_NO_MSG(h != NULL);

	return FIELD_GET(RTP_HDR_V_MASK, h->vpxcc);
}

/**
 * @brief Get the padding (P) flag.
 *
 * @param h RTP header to read.
 *
 * @return Padding flag value.
 */
static inline uint8_t rtp_header_get_p(const struct rtp_header *h)
{
	__ASSERT_NO_MSG(h != NULL);

	return FIELD_GET(RTP_HDR_P_MASK, h->vpxcc);
}

/**
 * @brief Get the extension (X) flag.
 *
 * @param h RTP header to read.
 *
 * @return Extension flag value.
 */
static inline uint8_t rtp_header_get_x(const struct rtp_header *h)
{
	__ASSERT_NO_MSG(h != NULL);

	return FIELD_GET(RTP_HDR_X_MASK, h->vpxcc);
}

/**
 * @brief Get the CSRC count (CC) field.
 *
 * @param h RTP header to read.
 *
 * @return CSRC count field value.
 */
static inline uint8_t rtp_header_get_cc(const struct rtp_header *h)
{
	__ASSERT_NO_MSG(h != NULL);

	return FIELD_GET(RTP_HDR_CC_MASK, h->vpxcc);
}

/**
 * @brief Get the marker (M) bit.
 *
 * @param h RTP header to read.
 *
 * @return Marker bit value.
 */
static inline uint8_t rtp_header_get_m(const struct rtp_header *h)
{
	__ASSERT_NO_MSG(h != NULL);

	return FIELD_GET(RTP_HDR_M_MASK, h->mpt);
}

/**
 * @brief Get the payload type (PT) field.
 *
 * @param h RTP header to read.
 *
 * @return Payload type field value.
 */
static inline uint8_t rtp_header_get_pt(const struct rtp_header *h)
{
	__ASSERT_NO_MSG(h != NULL);

	return FIELD_GET(RTP_HDR_PT_MASK, h->mpt);
}

/**
 * @brief Set the RTP version (V) field.
 *
 * @param h     RTP header to modify.
 * @param value Version field value.
 */
static inline void rtp_header_set_v(struct rtp_header *h, uint8_t value)
{
	__ASSERT_NO_MSG(h != NULL);

	h->vpxcc &= ~RTP_HDR_V_MASK;
	h->vpxcc |= FIELD_PREP(RTP_HDR_V_MASK, value);
}

/**
 * @brief Set the padding (P) flag.
 *
 * @param h     RTP header to modify.
 * @param value Padding flag value.
 */
static inline void rtp_header_set_p(struct rtp_header *h, uint8_t value)
{
	__ASSERT_NO_MSG(h != NULL);

	h->vpxcc &= ~RTP_HDR_P_MASK;
	h->vpxcc |= FIELD_PREP(RTP_HDR_P_MASK, value);
}

/**
 * @brief Set the extension (X) flag.
 *
 * @param h     RTP header to modify.
 * @param value Extension flag value.
 */
static inline void rtp_header_set_x(struct rtp_header *h, uint8_t value)
{
	__ASSERT_NO_MSG(h != NULL);

	h->vpxcc &= ~RTP_HDR_X_MASK;
	h->vpxcc |= FIELD_PREP(RTP_HDR_X_MASK, value);
}

/**
 * @brief Set the CSRC count (CC) field.
 *
 * @param h     RTP header to modify.
 * @param value CSRC count field value.
 */
static inline void rtp_header_set_cc(struct rtp_header *h, uint8_t value)
{
	__ASSERT_NO_MSG(h != NULL);

	h->vpxcc &= ~RTP_HDR_CC_MASK;
	h->vpxcc |= FIELD_PREP(RTP_HDR_CC_MASK, value);
}

/**
 * @brief Set the marker (M) bit.
 *
 * @param h     RTP header to modify.
 * @param value Marker bit value.
 */
static inline void rtp_header_set_m(struct rtp_header *h, uint8_t value)
{
	__ASSERT_NO_MSG(h != NULL);

	h->mpt &= ~RTP_HDR_M_MASK;
	h->mpt |= FIELD_PREP(RTP_HDR_M_MASK, value);
}

/**
 * @brief Set the payload type (PT) field.
 *
 * @param h     RTP header to modify.
 * @param value Payload type field value.
 */
static inline void rtp_header_set_pt(struct rtp_header *h, uint8_t value)
{
	__ASSERT_NO_MSG(h != NULL);

	h->mpt &= ~RTP_HDR_PT_MASK;
	h->mpt |= FIELD_PREP(RTP_HDR_PT_MASK, value);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_RTP_H_ */
