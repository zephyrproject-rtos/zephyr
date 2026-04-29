/*
 * Copyright (c) 2026 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief RTP (Real-time Transport Protocol) API
 */

#ifndef ZEPHYR_INCLUDE_NET_RTP_H_
#define ZEPHYR_INCLUDE_NET_RTP_H_

#include <zephyr/kernel.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_if.h>
#ifdef CONFIG_NET_PKT_TIMESTAMP
#include <zephyr/net/ptp_time.h>
#endif
#include <zephyr/net/net_context.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup rtp RTP (Real-time Transport Protocol)
 * @version 0.0.1
 * @ingroup networking
 * @{
 */

/** RTP protocol version 2 as defined in RFC 3550. */
#define RTP_VERSION 2

/** Minimum RTP header length in bytes, excluding any CSRC list or extension. */
#define RTP_MIN_HEADER_LEN 12

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
	/** Union to hold first part of the RTP header */
	union {
		struct {
			/** Payload type. */
			uint8_t pt: 7;
			/** Marker bit. */
			uint8_t m: 1;
			/** CSRC count; number of CSRC identifiers that follow the fixed header. */
			uint8_t cc: 4;
			/** Extension bit; set when a header extension is present. */
			uint8_t x: 1;
			/** Padding bit; set when the packet contains padding octets. */
			uint8_t p: 1;
			/** RTP version; must equal @ref RTP_VERSION. */
			uint8_t version: 2;
		} __packed;
		/** Raw encoding of the version, padding, extension, CC, marker, and payload type
		 * fields.
		 */
		uint16_t vpxccmpt;
	};
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
	uint32_t csrc[CONFIG_RTP_MAX_CSRC_COUNT];
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
 * @param[in] session   Pointer to the RTP session that received the packet.
 * @param[in] packet    Pointer to the received RTP packet.
 * @param[in] user_data Opaque user data supplied during session initialization.
 */
typedef void (*rtp_rx_cb_t)(struct rtp_session *session, struct rtp_packet *packet,
			    void *user_data);

/** RTP session configuration context. */
struct rtp_session_context {
	/** Network interface used by the session. */
	struct net_if *iface;

	/* Transmitter configuration
	 */

	/** Source address and port for transmitted packets. */
	struct net_sockaddr source_addr;
	/** RTP payload type field value used when transmitting. */
	uint8_t payload_type;
	/** Marker bit value used when transmitting. */
	uint8_t marker;

	/* Receiver configuration
	 */

	/** Local address and port the session listens on. */
	struct net_sockaddr sink_addr;
	/** Callback invoked upon packet reception. */
	rtp_rx_cb_t callback;
	/** Opaque user data forwarded to @p callback. */
	void *user_data;
};

/**
 * RTP session state.
 *
 * Declare with @ref RTP_SESSION_DEFINE and configure with @ref rtp_session_init
 * before use.
 */
struct rtp_session {
	/** Network connection handle for RTP traffic. */
	struct net_conn_handle *net_handle_rtp;
	/** Network connection handle for RTCP traffic. */
	struct net_conn_handle *net_handle_rtcp;
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
	uint32_t csrc[CONFIG_RTP_MAX_CSRC_COUNT];
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
 * @param _local_port Local UDP port used when transmitting packets. Set to 0
 *                    to let the stack select a port automatically (see
 *                    @kconfig{CONFIG_RTP_LOCAL_PORT_BASE}).
 */
#define RTP_SESSION_DEFINE(_name, _local_port)                                                     \
	struct rtp_session _name = {.local_port = _local_port, RTP_SESSION_NAME(_name)}

/**
 * @brief Initialize an RTP header extension.
 *
 * @param[out] hdr_x      Pointer to the header extension struct to initialize.
 * @param[in]  definition Profile-defined extension header identifier.
 * @param[in]  data       Pointer to the extension data.
 * @param[in]  len        Length of @p data in bytes; must be aligned to a
 *                        32-bit word boundary.
 *
 * @retval 0        On success.
 * @retval negative Errno value on failure.
 */
int rtp_init_header_extension(struct rtp_header_extension *hdr_x, uint16_t definition,
			      uint8_t *data, size_t len);

/**
 * @brief Initialize an RTP session.
 *
 * Configure a session for transmit, receive, or both. Pass NULL for
 * @p sink_addr to disable receive functionality, or NULL for @p source_addr
 * to disable transmit functionality.
 *
 * @param[in,out] session      Pointer to the RTP session to initialize.
 * @param[in]     iface        Network interface to use.
 * @param[in]     sink_addr    Local address and port to listen on for incoming
 *                             packets, or NULL to disable receive functionality.
 * @param[in]     source_addr  Remote address and port to send packets to, or
 *                             NULL to disable transmit functionality.
 * @param[in]     payload_type RTP payload type field value used when
 *                             transmitting; ignored if @p source_addr is NULL.
 * @param[in]     marker       Marker bit value used when transmitting; ignored
 *                             if @p source_addr is NULL.
 * @param[in]     callback     Callback invoked on packet reception; ignored if
 *                             @p sink_addr is NULL.
 * @param[in]     user_data    Opaque user data forwarded to @p callback.
 *
 * @retval 0        On success.
 * @retval negative Errno value on failure.
 */
int rtp_session_init(struct rtp_session *session, struct net_if *iface,
		     struct net_sockaddr *sink_addr, struct net_sockaddr *source_addr,
		     uint8_t payload_type, uint8_t marker, rtp_rx_cb_t callback, void *user_data);

/**
 * @brief Initialize a receive-only RTP session.
 *
 * Convenience wrapper around @ref rtp_session_init that configures the session
 * for receive only.
 *
 * @param[in,out] session   Pointer to the RTP session to initialize.
 * @param[in]     iface     Network interface to use.
 * @param[in]     sink_addr Local address and port to listen on.
 * @param[in]     callback  Callback invoked when a packet is received.
 * @param[in]     user_data Opaque user data forwarded to @p callback.
 *
 * @retval 0        On success.
 * @retval negative Errno value on failure.
 */
static inline int rtp_session_init_rx(struct rtp_session *session, struct net_if *iface,
				      struct net_sockaddr *sink_addr, rtp_rx_cb_t callback,
				      void *user_data)
{
	return rtp_session_init(session, iface, sink_addr, NULL, 0, 0, callback, user_data);
}

/**
 * @brief Initialize a transmit-only RTP session.
 *
 * Convenience wrapper around @ref rtp_session_init that configures the session
 * for transmit only.
 *
 * @param[in,out] session      Pointer to the RTP session to initialize.
 * @param[in]     iface        Network interface to use.
 * @param[in]     source_addr  Remote address and port to send packets to.
 * @param[in]     payload_type RTP payload type field value.
 * @param[in]     marker       Marker bit value.
 *
 * @retval 0        On success.
 * @retval negative Errno value on failure.
 */
static inline int rtp_session_init_tx(struct rtp_session *session, struct net_if *iface,
				      struct net_sockaddr *source_addr, uint8_t payload_type,
				      uint8_t marker)
{

	return rtp_session_init(session, iface, NULL, source_addr, payload_type, marker, NULL,
				NULL);
}

/**
 * @brief Start an RTP session.
 *
 * Open the network connections and begin processing packets. The session must
 * have been configured with @ref rtp_session_init before calling this function.
 *
 * @param[in,out] session Pointer to the RTP session.
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
 * @param[in,out] session Pointer to the RTP session.
 *
 * @retval 0        On success.
 * @retval negative Errno value on failure.
 */
int rtp_session_stop(struct rtp_session *session);

/**
 * @brief Add a contributing source (CSRC) to the session.
 *
 * @param[in,out] session Pointer to the RTP session.
 * @param[in]     csrc    CSRC identifier to add.
 *
 * @retval 0        On success.
 * @retval negative Errno value on failure.
 */
int rtp_session_add_csrc(struct rtp_session *session, uint32_t csrc);

/**
 * @brief Remove a contributing source (CSRC) from the session.
 *
 * @param[in,out] session Pointer to the RTP session.
 * @param[in]     csrc    CSRC identifier to remove.
 *
 * @retval 0        On success.
 * @retval negative Errno value on failure.
 */
int rtp_session_remove_csrc(struct rtp_session *session, uint32_t csrc);

/**
 * @brief Send an RTP packet with an optional header extension and optional padding.
 *
 * @param[in,out] session  Pointer to the RTP session.
 * @param[in]     data     Pointer to the payload data to send.
 * @param[in]     len      Length of @p data in bytes.
 * @param[in]     delta_ts Timestamp increment to apply; for audio this is
 *                         typically the number of sample periods covered by
 *                         the payload.
 * @param[in]     padding  Number of padding bytes to append (0–255); pass 0
 *                         for no padding.
 * @param[in]     hdr_x    Pointer to a header extension, or NULL for no
 *                         extension.
 *
 * @retval 0        On success.
 * @retval negative Errno value on failure.
 */
int rtp_session_send(struct rtp_session *session, void *data, size_t len, uint32_t delta_ts,
		     size_t padding, struct rtp_header_extension *hdr_x);

/**
 * @brief Send an RTP packet without a header extension or padding.
 *
 * Convenience wrapper around @ref rtp_session_send.
 *
 * @param[in,out] session  Pointer to the RTP session.
 * @param[in]     data     Pointer to the payload data to send.
 * @param[in]     len      Length of @p data in bytes.
 * @param[in]     delta_ts Timestamp increment to apply; for audio this is
 *                         typically the number of sample periods covered by
 *                         the payload.
 *
 * @retval 0        On success.
 * @retval negative Errno value on failure.
 */
static inline int rtp_session_send_simple(struct rtp_session *session, void *data, size_t len,
					  uint32_t delta_ts)
{
	return rtp_session_send(session, data, len, delta_ts, 0, NULL);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_RTP_H_ */
