/** @file
 @brief UDP data handler

 This is not to be included by the application and is only used by
 core IP stack.
 */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __UDP_INTERNAL_H
#define __UDP_INTERNAL_H

#include <zephyr/types.h>

#include <zephyr/net/net_core.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_context.h>

#include "connection.h"

#ifdef __cplusplus
extern "C" {
#endif

/** UDP option kind values (RFC 9868 Section 10, Table 1) */
#define NET_UDP_OPT_KIND_EOL   0   /**< End of Options List */
#define NET_UDP_OPT_KIND_NOP   1   /**< No Operation */
#define NET_UDP_OPT_KIND_APC   2   /**< Additional Payload Checksum */
#define NET_UDP_OPT_KIND_FRAG  3   /**< Fragmentation */
#define NET_UDP_OPT_KIND_MDS   4   /**< Maximum Datagram Size */
#define NET_UDP_OPT_KIND_MRDS  5   /**< Maximum Reassembled Datagram Size */
#define NET_UDP_OPT_KIND_REQ   6   /**< Echo Request */
#define NET_UDP_OPT_KIND_RES   7   /**< Echo Response */
#define NET_UDP_OPT_KIND_TIME  8   /**< Timestamp */
#define NET_UDP_OPT_KIND_AUTH  9   /**< Authentication (RESERVED) */
#define NET_UDP_OPT_KIND_EXP   127 /**< Experimental */
#define NET_UDP_OPT_KIND_UCMP  192 /**< UNSAFE Compression */
#define NET_UDP_OPT_KIND_UENC  193 /**< UNSAFE Encryption */
#define NET_UDP_OPT_KIND_UEXP  254 /**< UNSAFE Experimental (255 is Reserved) */

/** Status/result bits for each parsed option */
#define NET_UDP_OPT_STATUS_PRESENT  BIT(0)
#define NET_UDP_OPT_STATUS_OK       BIT(1)
#define NET_UDP_OPT_STATUS_FAIL     BIT(2)
#define NET_UDP_OPT_STATUS_UNKNOWN  BIT(3)

/** Internal UDP option feature bits used in policy/presence bitmasks */
#define NET_UDP_OPT_F_OCS  BIT(0)
#define NET_UDP_OPT_F_APC  BIT(1)
#define NET_UDP_OPT_F_FRAG BIT(2)
#define NET_UDP_OPT_F_MDS  BIT(3)
#define NET_UDP_OPT_F_MRDS BIT(4)
#define NET_UDP_OPT_F_REQ  BIT(5)
#define NET_UDP_OPT_F_RES  BIT(6)
#define NET_UDP_OPT_F_TIME BIT(7)
#define NET_UDP_OPT_F_AUTH BIT(8)
#define NET_UDP_OPT_F_EXP  BIT(9)
#define NET_UDP_OPT_F_UCMP BIT(10)
#define NET_UDP_OPT_F_UENC BIT(11)
#define NET_UDP_OPT_F_UEXP BIT(12)

#define NET_UDP_OPT_F_ALL (NET_UDP_OPT_F_OCS | NET_UDP_OPT_F_APC | NET_UDP_OPT_F_FRAG | \
			   NET_UDP_OPT_F_MDS | NET_UDP_OPT_F_MRDS | NET_UDP_OPT_F_REQ | \
			   NET_UDP_OPT_F_RES | NET_UDP_OPT_F_TIME | NET_UDP_OPT_F_AUTH | \
			   NET_UDP_OPT_F_EXP | NET_UDP_OPT_F_UCMP | NET_UDP_OPT_F_UENC | \
			   NET_UDP_OPT_F_UEXP)

/** UDP option features that are actually implemented for sending/receiving */
#define NET_UDP_OPT_F_SUPPORTED (NET_UDP_OPT_F_OCS | NET_UDP_OPT_F_APC | \
				 NET_UDP_OPT_F_MDS | NET_UDP_OPT_F_MRDS | \
				 NET_UDP_OPT_F_REQ | NET_UDP_OPT_F_RES | \
				 NET_UDP_OPT_F_TIME)

/** Result of parsing UDP options from a received packet */
struct net_udp_opt_info {
	/** Bitmask of parsed option features (NET_UDP_OPT_F_*) */
	uint32_t present;

	/** OCS validation result (true = passed or unused) */
	bool ocs_valid;

	/** APC (Additional Payload Checksum, RFC 9868) CRC32c value, valid when
	 * APC is present. The UDP options layer does not compute or verify this
	 * checksum; it is carried verbatim between the wire and the application,
	 * which is responsible for computing it on transmit and validating it on
	 * receive.
	 */
	uint32_t apc_crc;

	/** MDS value received from peer */
	uint16_t mds;

	/** MRDS value received from peer */
	struct net_udp_opt_mrds mrds;

	/** Echo Request token (if present) */
	uint32_t req_token;

	/** Echo Response token (if present) */
	uint32_t res_token;

	/** Timestamp value (if present) */
	struct net_udp_opt_time time;

	/** FRAG header fields, if present */
	struct {
		uint16_t frag_start;
		uint32_t identification;
		uint16_t frag_offset;
		bool is_terminal;
	} frag;
};

/**
 * @brief Create UDP packet into net_pkt
 *
 * Note: pkt's cursor should be set a the right position.
 *       (i.e. after IP header)
 *
 * @param pkt Network packet
 * @param src_port Destination port in network byte order.
 * @param dst_port Destination port in network byte order.
 *
 * @return 0 on success, negative errno otherwise.
 */
#if defined(CONFIG_NET_NATIVE_UDP)
int net_udp_create(struct net_pkt *pkt, uint16_t src_port, uint16_t dst_port);
#else
static inline int net_udp_create(struct net_pkt *pkt,
				 uint16_t src_port, uint16_t dst_port)
{
	ARG_UNUSED(pkt);
	ARG_UNUSED(src_port);
	ARG_UNUSED(dst_port);

	return 0;
}
#endif

/**
 * @brief Finalize UDP packet
 *
 * Note: calculates final length and setting up the checksum.
 *
 * @param pkt Network packet
 *
 * @return 0 on success, negative errno otherwise.
 */
#if defined(CONFIG_NET_NATIVE_UDP)
int net_udp_finalize(struct net_pkt *pkt, bool force_chksum);
#else
static inline int net_udp_finalize(struct net_pkt *pkt, bool force_chksum)
{
	ARG_UNUSED(pkt);
	ARG_UNUSED(force_chksum);

	return 0;
}
#endif

/**
 * @brief Get pointer to UDP header in net_pkt
 *
 * @param pkt Network packet
 * @param udp_access Helper variable for accessing UDP header
 *
 * @return UDP header on success, NULL on error
 */
#if defined(CONFIG_NET_NATIVE_UDP)
struct net_udp_hdr *net_udp_input(struct net_pkt *pkt,
				  struct net_pkt_data_access *udp_access);
#else
static inline
struct net_udp_hdr *net_udp_input(struct net_pkt *pkt,
				  struct net_pkt_data_access *udp_access)
{
	ARG_UNUSED(pkt);
	ARG_UNUSED(udp_access);

	return NULL;
}
#endif

/**
 * @brief Register a callback to be called when UDP packet
 * is received corresponding to received packet.
 *
 * @param family Protocol family
 * @param remote_addr Remote address of the connection end point.
 * @param local_addr Local address of the connection end point.
 * @param remote_port Remote port of the connection end point.
 * @param local_port Local port of the connection end point.
 * @param context net_context structure related to the connection.
 * @param cb Callback to be called
 * @param user_data User data supplied by caller.
 * @param handle UDP handle that can be used when unregistering
 *
 * @return Return 0 if the registration succeed, <0 otherwise.
 */
int net_udp_register(uint8_t family,
		     const struct net_sockaddr *remote_addr,
		     const struct net_sockaddr *local_addr,
		     uint16_t remote_port,
		     uint16_t local_port,
		     struct net_context *context,
		     net_conn_cb_t cb,
		     void *user_data,
		     struct net_conn_handle **handle);

/**
 * @brief Unregister UDP handler.
 *
 * @param handle Handle from registering.
 *
 * @return Return 0 if the unregistration succeed, <0 otherwise.
 */
int net_udp_unregister(struct net_conn_handle *handle);

/**
 * @brief Parse UDP options from the surplus area of a received packet.
 *
 * Extracts and validates the OCS and all TLV options from the surplus area
 * (bytes between UDP Length end and IP payload end).
 *
 * @param pkt       Network packet
 * @param info      [out] Parsed option information
 *
 * @return 0 on success (options parsed, results in info)
 * @return -ENOENT if no surplus area exists (no options)
 * @return -EINVAL if surplus area is malformed
 */
int net_udp_opt_parse(struct net_pkt *pkt, struct net_udp_opt_info *info);

/**
 * @brief Compute the total size needed for UDP options to be appended.
 *
 * @param ctx       Network context (carries per-socket option config)
 * @param data_len  Length of UDP user data
 *
 * @return Number of surplus-area bytes needed (including OCS alignment padding)
 */
uint16_t net_udp_opt_surplus_len(struct net_context *ctx, uint16_t data_len);

/**
 * @brief Compute outgoing UDP options surplus size with optional per-packet overrides.
 *
 * This helper mirrors the option merge rules used by net_udp_opt_append():
 * socket-level options from @p ctx are combined with optional per-packet
 * overrides from @p opts (typically parsed from sendmsg() cmsgs).
 *
 * @param ctx       Network context (socket-level UDP options)
 * @param data_len  Length of UDP user data
 * @param opts      Optional per-packet option overrides, or NULL
 *
 * @return Number of surplus-area bytes needed (including alignment padding and OCS)
 */
uint16_t net_udp_opt_surplus_len_with_opts(struct net_context *ctx, uint16_t data_len,
					   const struct net_udp_opt_info *opts);

/**
 * @brief Append UDP options to an outgoing packet's surplus area.
 *
 * This writes the alignment padding, OCS, and enabled TLV options after
 * the UDP user data. The UDP Length in the header is NOT modified (it
 * must still reflect only header + user data).
 *
 * @param pkt       Network packet (cursor should be at end of UDP user data)
 * @param ctx       Network context (carries per-socket option config)
 * @param opts      Optional per-packet option overrides (from cmsg), or NULL
 *
 * @return 0 on success, negative errno on failure
 */
int net_udp_opt_append(struct net_pkt *pkt, struct net_context *ctx,
		       const struct net_udp_opt_info *opts);

/**
 * @brief Compute and write the OCS field for the surplus area.
 *
 * Called after all options are written. Computes the one's complement
 * checksum over the surplus area and its length.
 *
 * @param pkt           Network packet
 * @param surplus_offset Byte offset of surplus area start in the packet
 * @param surplus_len   Total length of surplus area
 *
 * @return 0 on success
 */
int net_udp_opt_finalize_ocs(struct net_pkt *pkt,
			     uint16_t surplus_offset,
			     uint16_t surplus_len);

#ifdef __cplusplus
}
#endif

#endif /* __UDP_INTERNAL_H */
