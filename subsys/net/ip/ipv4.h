/** @file
 @brief IPv4 related functions

 This is not to be included by the application.
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __IPV4_H
#define __IPV4_H

#include <zephyr/types.h>

#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_context.h>

#define NET_IPV4_IHL_MASK 0x0F
#define NET_IPV4_DSCP_MASK 0xFC
#define NET_IPV4_DSCP_OFFSET 2
#define NET_IPV4_ECN_MASK 0x03

/* IPv4 Options */
#define NET_IPV4_OPTS_EO   0   /* End of Options */
#define NET_IPV4_OPTS_NOP  1   /* No operation */
#define NET_IPV4_OPTS_RR   7   /* Record Route */
#define NET_IPV4_OPTS_TS   68  /* Timestamp */
#define NET_IPV4_OPTS_RA   148 /* Router Alert */

/* IPv4 Options Timestamp flags */
#define NET_IPV4_TS_OPT_TS_ONLY	0 /* Timestamp only */
#define NET_IPV4_TS_OPT_TS_ADDR	1 /* Timestamp and address */
#define NET_IPV4_TS_OPT_TS_PRES	3 /* Timestamp prespecified hops*/

#define NET_IPV4_HDR_OPTNS_MAX_LEN 40

/* Fragment bits */
#define NET_IPV4_MF BIT(0) /* More fragments  */
#define NET_IPV4_DF BIT(1) /* Do not fragment */

#define NET_IPV4_IGMP_QUERY     0x11 /* Membership query     */
#define NET_IPV4_IGMP_REPORT_V1 0x12 /* v1 Membership report */
#define NET_IPV4_IGMP_REPORT_V2 0x16 /* v2 Membership report */
#define NET_IPV4_IGMP_LEAVE     0x17 /* v2 Leave group       */
#define NET_IPV4_IGMP_REPORT_V3 0x22 /* v3 Membership report */

struct net_ipv4_igmp_v2_query {
	/* IGMP message type */
	uint8_t type;
	/* Max response code */
	uint8_t max_rsp;
	/* 16-bit ones' complement of the entire message */
	uint16_t chksum;
	/* The multicast address being queried */
	struct in_addr address;
} __packed;

struct net_ipv4_igmp_v2_report {
	/* IGMP message type */
	uint8_t type;
	/* Max response code */
	uint8_t max_rsp;
	/* 16-bit ones' complement of the entire message */
	uint16_t chksum;
	/* The multicast address being queried */
	struct in_addr address;
} __packed;

struct net_ipv4_igmp_v3_query {
	/* IGMP message type */
	uint8_t type;
	/* Max response code */
	uint8_t max_rsp;
	/* 16-bit ones' complement of the entire message */
	uint16_t chksum;
	/* The multicast address being queried */
	struct in_addr address;
	/* Reserved field, ignore */
	uint8_t reserved: 4;
	/* Suppress Router-side Processing Flag */
	uint8_t suppress: 1;
	/* Querier's Robustness Variable */
	uint8_t qrv: 3;
	/* Querier's Query Interval Code */
	uint8_t qqic;
	/* Number of Source Addresses */
	uint16_t sources_len;
} __packed;

struct net_ipv4_igmp_v3_group_record {
	/* Record type */
	uint8_t type;
	/* Aux Data Len */
	uint8_t aux_len;
	/* Number of Source Addresses */
	uint16_t sources_len;
	/* The multicast address to report to*/
	struct in_addr address;
} __packed;

struct net_ipv4_igmp_v3_report {
	/* IGMP message type */
	uint8_t type;
	/* Reserved field, ignore */
	uint8_t reserved_1;
	/* 16-bit ones' complement of the entire message */
	uint16_t chksum;
	/* Reserved field, ignore */
	uint16_t reserved_2;
	/* Number of Group Records */
	uint16_t groups_len;
} __packed;

/**
 * @brief Create IPv4 packet in provided net_pkt with option to set all the
 *        caller settable values.
 *
 * @param pkt Network packet
 * @param src Source IPv4 address
 * @param dst Destination IPv4 address
 * @param tos Type of service
 * @param id Fragment id
 * @param flags Fragmentation flags
 * @param offset Fragment offset
 *
 * @return 0 on success, negative errno otherwise.
 */
#if defined(CONFIG_NET_NATIVE_IPV4)
int net_ipv4_create_full(struct net_pkt *pkt,
			 const struct in_addr *src,
			 const struct in_addr *dst,
			 uint8_t tos,
			 uint16_t id,
			 uint8_t flags,
			 uint16_t offset);
#else
static inline int net_ipv4_create_full(struct net_pkt *pkt,
				       const struct in_addr *src,
				       const struct in_addr *dst,
				       uint8_t tos,
				       uint16_t id,
				       uint8_t flags,
				       uint16_t offset)
{
	ARG_UNUSED(pkt);
	ARG_UNUSED(src);
	ARG_UNUSED(dst);
	ARG_UNUSED(tos);
	ARG_UNUSED(id);
	ARG_UNUSED(flags);
	ARG_UNUSED(offset);

	return -ENOTSUP;
}
#endif

/**
 * @brief Create IPv4 packet in provided net_pkt.
 *
 * @param pkt Network packet
 * @param src Source IPv4 address
 * @param dst Destination IPv4 address
 *
 * @return 0 on success, negative errno otherwise.
 */
#if defined(CONFIG_NET_NATIVE_IPV4)
int net_ipv4_create(struct net_pkt *pkt,
		    const struct in_addr *src,
		    const struct in_addr *dst);
#else
static inline int net_ipv4_create(struct net_pkt *pkt,
				  const struct in_addr *src,
				  const struct in_addr *dst)
{
	ARG_UNUSED(pkt);
	ARG_UNUSED(src);
	ARG_UNUSED(dst);

	return -ENOTSUP;
}
#endif

/**
 * @brief Finalize IPv4 packet. It should be called right before
 * sending the packet and after all the data has been added into
 * the packet. This function will set the length of the
 * packet and calculate the higher protocol checksum if needed.
 *
 * @param pkt Network packet
 * @param next_header_proto Protocol type of the next header after IPv4 header.
 *
 * @return 0 on success, negative errno otherwise.
 */
#if defined(CONFIG_NET_NATIVE_IPV4)
int net_ipv4_finalize(struct net_pkt *pkt, uint8_t next_header_proto);
#else
static inline int net_ipv4_finalize(struct net_pkt *pkt,
				    uint8_t next_header_proto)
{
	ARG_UNUSED(pkt);
	ARG_UNUSED(next_header_proto);

	return -ENOTSUP;
}
#endif

/**
 * @typedef net_ipv4_parse_hdr_options_cb_t
 * @brief IPv4 header options handle callback
 *
 * @details The callback is called when parser encounter
 * supported options.
 *
 * @param opt_type Option type
 * @param opt_data Option data
 * @param opt_len Option length
 * @param user_data Userdata given in net_ipv4_parse_hdr_options()
 *
 * @return 0 on success, negative otherwise.
 */
typedef int (*net_ipv4_parse_hdr_options_cb_t)(uint8_t opt_type,
					       uint8_t *opt_data,
					       uint8_t opt_len,
					       void *user_data);

/**
 * @brief Parse IPv4 header options.
 * Parse the IPv4 header options and call the callback with
 * options type, data and length along with user_data.
 *
 * @param pkt Network packet
 * @param cb callback to handle IPv4 header options
 * @param user_data User data
 *
 * @return 0 on success, negative otherwise.
 */
#if defined(CONFIG_NET_IPV4_HDR_OPTIONS)
int net_ipv4_parse_hdr_options(struct net_pkt *pkt,
			       net_ipv4_parse_hdr_options_cb_t cb,
			       void *user_data);
#else
static inline int net_ipv4_parse_hdr_options(struct net_pkt *pkt,
					     net_ipv4_parse_hdr_options_cb_t cb,
					     void *user_data)
{
	ARG_UNUSED(pkt);
	ARG_UNUSED(cb);
	ARG_UNUSED(user_data);

	return -ENOTSUP;
}
#endif

/**
 * @brief Decode DSCP value from ToS field.
 *
 * @param tos ToS field value from the IPv4 header.
 *
 * @return Decoded DSCP value.
 */
static inline uint8_t net_ipv4_get_dscp(uint8_t tos)
{
	return (tos & NET_IPV4_DSCP_MASK) >> NET_IPV4_DSCP_OFFSET;
}

/**
 * @brief Encode DSCP value into ToS field.
 *
 * @param tos A pointer to the ToS field.
 * @param dscp DSCP value to set.
 */
static inline void net_ipv4_set_dscp(uint8_t *tos, uint8_t dscp)
{
	*tos &= ~NET_IPV4_DSCP_MASK;
	*tos |= (dscp << NET_IPV4_DSCP_OFFSET) & NET_IPV4_DSCP_MASK;
}

/**
 * @brief Convert DSCP value to priority.
 *
 * @param dscp DSCP value.
 */
static inline uint8_t net_ipv4_dscp_to_priority(uint8_t dscp)
{
	return dscp >> 3;
}

/**
 * @brief Decode ECN value from ToS field.
 *
 * @param tos ToS field value from the IPv4 header.
 *
 * @return Decoded ECN value.
 */
static inline uint8_t net_ipv4_get_ecn(uint8_t tos)
{
	return tos & NET_IPV4_ECN_MASK;
}

/**
 * @brief Encode ECN value into ToS field.
 *
 * @param tos A pointer to the ToS field.
 * @param ecn ECN value to set.
 */
static inline void net_ipv4_set_ecn(uint8_t *tos, uint8_t ecn)
{
	*tos &= ~NET_IPV4_ECN_MASK;
	*tos |= ecn & NET_IPV4_ECN_MASK;
}

#if defined(CONFIG_NET_IPV4_FRAGMENT)
/** Store pending IPv4 fragment information that is needed for reassembly. */
struct net_ipv4_reassembly {
	/** IPv4 source address of the fragment */
	struct in_addr src;

	/** IPv4 destination address of the fragment */
	struct in_addr dst;

	/**
	 * Timeout for cancelling the reassembly. The timer is used
	 * also to detect if this reassembly slot is used or not.
	 */
	struct k_work_delayable timer;

	/** Pointers to pending fragments */
	struct net_pkt *pkt[CONFIG_NET_IPV4_FRAGMENT_MAX_PKT];

	/** IPv4 fragment identification */
	uint16_t id;
	uint8_t protocol;
};
#else
struct net_ipv4_reassembly;
#endif

/**
 * @typedef net_ipv4_frag_cb_t
 * @brief Callback used while iterating over pending IPv4 fragments.
 *
 * @param reass IPv4 fragment reassembly struct
 * @param user_data A valid pointer on some user data or NULL
 */
typedef void (*net_ipv4_frag_cb_t)(struct net_ipv4_reassembly *reass, void *user_data);

/**
 * @brief Go through all the currently pending IPv4 fragments.
 *
 * @param cb Callback to call for each pending IPv4 fragment.
 * @param user_data User specified data or NULL.
 */
void net_ipv4_frag_foreach(net_ipv4_frag_cb_t cb, void *user_data);

#if defined(CONFIG_NET_NATIVE_IPV4)
/**
 * @brief Initialises IPv4
 */
void net_ipv4_init(void);

/**
 * @brief Handles IPv4 fragmented packets.
 *
 * @param pkt     Network head packet.
 * @param hdr     The IPv4 header of the current packet
 *
 * @return Return verdict about the packet.
 */
#if defined(CONFIG_NET_IPV4_FRAGMENT)
enum net_verdict net_ipv4_handle_fragment_hdr(struct net_pkt *pkt, struct net_ipv4_hdr *hdr);
#else
static inline enum net_verdict net_ipv4_handle_fragment_hdr(struct net_pkt *pkt,
							    struct net_ipv4_hdr *hdr)
{
	ARG_UNUSED(pkt);
	ARG_UNUSED(hdr);

	return NET_DROP;
}
#endif /* CONFIG_NET_IPV4_FRAGMENT */

/**
 * @brief Prepare packet for sending, this will split up a packet that is too large to send into
 * multiple fragments so that it can be sent.
 *
 * @param pkt Network packet
 *
 * @return Return verdict about the packet.
 */
#if defined(CONFIG_NET_IPV4_FRAGMENT)
enum net_verdict net_ipv4_prepare_for_send(struct net_pkt *pkt);
#else
static inline enum net_verdict net_ipv4_prepare_for_send(struct net_pkt *pkt)
{
	return NET_OK;
}
#endif /* CONFIG_NET_IPV4_FRAGMENT */

/**
 * @brief Sets up fragment buffers for usage, should only be called by the SYS_INIT() handler in
 * net_core.c
 */
#if defined(CONFIG_NET_IPV4_FRAGMENT)
void net_ipv4_setup_fragment_buffers(void);
#else
static inline void net_ipv4_setup_fragment_buffers(void)
{
}
#endif /* CONFIG_NET_IPV4_FRAGMENT */
#else
#define net_ipv4_init(...)
#endif /* CONFIG_NET_NATIVE_IPV4 */

#endif /* __IPV4_H */
