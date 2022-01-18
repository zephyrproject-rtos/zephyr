Copyright 2022 TU Wien

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

#include <logging/log.h>
LOG_MODULE_REGISTER(net_mcast_mpl, CONFIG_NET_ROUTE_LOG_LEVEL);

#include <errno.h>
#include <stdlib.h>
#include <net/net_core.h>
#include <net/net_pkt.h>
#include <net/net_stats.h>
#include <net/net_context.h>
#include <net/net_mgmt.h>
#include <net/virtual.h>
#include "net_private.h"
#include "connection.h"
#include "icmpv6.h"
#include "udp_internal.h"
#include "tcp_internal.h"
#include "ipv6.h"
#include "nbr.h"
#include "6lo.h"
#include "route.h"
#include "net_stats.h"

#include <kernel.h>
#include <limits.h>
#include <zephyr/types.h>
#include <sys/slist.h>
#include <net/trickle.h>
#include <sys/sflist.h>

#include "mpl.h"

#define MPL_SEED_ID_UNKNOWN 0xFF
#define PKT_WAIT_TIME	    K_SECONDS(1)

#if MPL_SEED_ID_TYPE == 0
#define HBHO_TOTAL_LEN HBHO_BASE_LEN + HBHO_S0_LEN
#elif MPL_SEED_ID_TYPE == 1
#define HBHO_TOTAL_LEN HBHO_BASE_LEN + HBHO_S1_LEN
#elif MPL_SEED_ID_TYPE == 2
#define HBHO_TOTAL_LEN HBHO_BASE_LEN + HBHO_S2_LEN
#elif MPL_SEED_ID_TYPE == 3
#define HBHO_TOTAL_LEN HBHO_BASE_LEN + HBHO_S3_LEN
#endif

typedef struct seed_id_s {
	uint8_t s;
	uint8_t id[16];
} seed_id_t;

struct mpl_seed {
	seed_id_t seed_id;
	uint8_t min_seqno;
	uint8_t lifetime;
	uint8_t count;
	sys_slist_t msg_list;
	struct mpl_domain *domain;
};

struct mpl_msg {
	struct mpl_msg *next;
	struct mpl_seed *seed;
	struct net_trickle trickle;
	struct net_if iface;
	struct in6_addr src;
	uint16_t size;
	uint8_t seq;
	uint8_t exp;
	uint8_t data[CONFIG_NET_BUF_DATA_SIZE];
};

struct mpl_domain {
	struct in6_addr data_addr;
	struct in6_addr ctrl_addr;
	struct net_trickle trickle;
	uint8_t exp;
};

struct mpl_hbho {
	uint8_t type;
	uint8_t len;
	uint8_t flags;
	uint8_t seq;
};
struct mpl_hbho_s1 {
	uint8_t type;
	uint8_t len;
	uint8_t flags;
	uint8_t seq;
	uint16_t seed_id;
};
struct mpl_hbho_s2 {
	uint8_t type;
	uint8_t len;
	uint8_t flags;
	uint8_t seq;
	uint64_t seed_id;
};
struct mpl_hbho_s3 {
	uint8_t type;
	uint8_t len;
	uint8_t flags;
	uint8_t seq;
	uint8_t seed_id[16];
};

static struct mpl_msg
	buffered_message_set[CONFIG_NET_MCAST_MPL_BUFFERED_MESSAGE_SET_SIZE];
static struct mpl_seed seed_set[CONFIG_NET_MCAST_MPL_SEED_SET_SIZE];
static struct mpl_domain domain_set[CONFIG_NET_MCAST_MPL_DOMAIN_SET_SIZE];

static uint16_t last_seq;
static seed_id_t local_seed_id;

#if defined(CONFIG_NET_MCAST_MPL_ALL_FORWARDERS_SUB)
// static struct in6_addr all_forwarders;
#endif

// static struct ctimer lifetime_timer;

static struct mpl_domain *domain_ptr;
static struct mpl_msg *msg_ptr;
static struct mpl_seed *seed_ptr;
static struct mpl_hbho *hbh_header_ptr;

#define MSG_SET_IS_USED(h)    ((h)->seed != NULL)
#define MSG_SET_CLEAR_USED(h) ((h)->seed = NULL)

#define SEED_SET_IS_USED(h)    (((h)->domain != NULL))
#define SEED_SET_CLEAR_USED(h) ((h)->domain = NULL)

#define DOMAIN_SET_IS_USED(h) (net_ipv6_is_addr_mcast(&(h)->data_addr))
#define DOMAIN_SET_CLEAR_USED(h)                                               \
	(memset(&(h)->data_addr, 0, sizeof(struct in6_addr)))

/* MPL Seed IDs can be either 16 bits, 64 bits, or 128 bits. If they are 128
 *  bits then the IPV6 Source address may also be used as the seed ID.
 *  These are always represented in contiki as a 128 bit number in the type
 *  seed_id_t. The functions below convert a seed id of various lengths to
 *  this 128 bit representation.
 */
/**
 * \brief Set the seed id to a 16 bit constant
 * dst: seed_id_t to set to the constant
 * src: 16 bit integer to set
 */
#define SEED_ID_S1(dst, src)                                                   \
	{                                                                      \
		(*(uint16_t *)&(dst)->id) = (src);                             \
		(dst)->s = 1;                                                  \
	}
/**
 * \brief Set the seed id to a 64 bit constant
 * dst: seed_id_t to set to the constant
 * src: 64 bit integer to set
 */
#define SEED_ID_S2(dst, src)                                                   \
	{                                                                      \
		(*(uint64_t *)&(dst)->id) = (src);                             \
		(dst)->s = 2;                                                  \
	}
/**
 * \brief Set the seed id to a 128 bit constant
 * dst: seed_id_t to set to the constant
 * l: Lower 64 bits of the seed id to set
 * h: Upper 64 bits of the seed id to set
 */
#define SEED_ID_S3(dst, l, h)                                                  \
	{                                                                      \
		(*(uint64_t *)&(dst)->id) = (l);                               \
		(*(uint64_t *)&(dst)->id[8]) = (h);                            \
		(dst)->s = 3;                                                  \
	}

/**
 * \brief Get the MPL Parametrization for a multicast HBHO header
 * m: pointer to the HBHO header
 */
#define HBH_GET_S(h) (((h)->flags & 0xC0) >> 6)

/**
 * \brief Set the MPL Parametrization bit for a multicast HBHO header
 * m: pointer to the HBHO header
 */
#define HBH_SET_S(h, s) ((h)->flags |= ((s & 0x03) << 6))

/**
 * \brief Clear the MPL Parametrization bit for a multicast HBHO header
 * m: pointer to the HBHO header
 */
#define HBH_CLR_S(h) ((h)->flags &= ~0xC0)

/**
 * \brief Get the MPL Parametrization for a multicast HBHO header
 * m: pointer to the HBHO header
 */
#define HBH_GET_M(h) (((h)->flags & 0x20) == 0x20)

/**
 * \brief Set the MPL Parametrization bit for a multicast HBHO header
 * m: pointer to the HBHO header
 */
#define HBH_SET_M(h) ((h)->flags |= 0x20)

/**
 * \brief Get the MPL Parametrization for a multicast HBHO header
 * m: pointer to the HBHO header
 */
#define HBH_GET_V(h) (((h)->flags & 0x10) == 0x10)

/**
 * \brief Set the MPL Parametrization bit for a multicast HBHO header
 * m: pointer to the HBHO header
 */
#define HBH_CLR_V(h) ((h)->flags &= ~0x10)

#define SEQ_VAL_IS_EQ(i1, i2) ((i1) == (i2))

/**
 * \brief s1 is said to be less than s2 if SEQ_VAL_IS_LT(s1, s2) == 1
 */
#define SEQ_VAL_IS_LT(i1, i2)                                                  \
	(((i1) != (i2)) &&                                                     \
	 ((((i1) < (i2)) && ((int16_t)((i2) - (i1)) < 0x100)) ||               \
	  (((i1) > (i2)) && ((int16_t)((i1) - (i2)) > 0x100))))

/**
 * \brief s1 is said to be greater than s2 iif SEQ_VAL_IS_LT(s1, s2) == 1
 */
#define SEQ_VAL_IS_GT(i1, i2)                                                  \
	(((i1) != (i2)) &&                                                     \
	 ((((i1) < (i2)) && ((int16_t)((i2) - (i1)) > 0x100)) ||               \
	  (((i1) > (i2)) && ((int16_t)((i1) - (i2)) < 0x100))))

#define SEQ_VAL_ADD(s, n) (((s) + (n)) % 0x100)

/**
 * \brief Compare two contiki seed ids represented as seed_id_t types
 * a: First value to compare
 * b: Second value to compare
 */
#define seed_id_cmp(a, b) (memcmp((a)->id, (b)->id, sizeof(uint8_t) * 16) == 0)
/**
 * \brief Copy one seed_id_t into another.
 * a: Destination
 * b: Source
 */
#define seed_id_cpy(a, b) (memcpy((a), (b), sizeof(seed_id_t)))
/**
 * \brief Clear a seed id value to zero
 * a: Value to clear
 */
#define seed_id_clr(a)	  (memset((a), 0, sizeof(seed_id_t)))

static struct mpl_msg *buffer_allocate(void)
{
	for (msg_ptr = &buffered_message_set
			       [CONFIG_NET_MCAST_MPL_BUFFERED_MESSAGE_SET_SIZE -
				1];
	     msg_ptr >= buffered_message_set; msg_ptr--) {
		if (!MSG_SET_IS_USED(msg_ptr)) {
			memset(msg_ptr, 0, sizeof(struct mpl_msg));
			return msg_ptr;
		}
	}
	return NULL;
}

static void buffer_free(struct mpl_msg *msg)
{
	if (net_trickle_is_running(&msg->trickle)) {
		net_trickle_stop(&msg->trickle);
	}
	MSG_SET_CLEAR_USED(msg);
}

/* Reclaim the message with min_seq in the largest seed set */
static struct mpl_msg *buffer_reclaim(void)
{
	static struct mpl_msg *reclaim_msg_ptr;
	static struct mpl_seed *seed_ptr;
	static struct mpl_seed *largest_seed_ptr;

	largest_seed_ptr = NULL;
	reclaim_msg_ptr = NULL;
	for (seed_ptr = &seed_set[CONFIG_NET_MCAST_MPL_SEED_SET_SIZE];
	     seed_ptr >= seed_set; seed_ptr--) {
		if (SEED_SET_IS_USED(seed_ptr) &&
		    (largest_seed_ptr == NULL ||
		     seed_ptr->count > largest_seed_ptr->count)) {
			largest_seed_ptr = seed_ptr;
		}
	}

	if (largest_seed_ptr != NULL) {
		reclaim_msg_ptr = (struct mpl_msg *)sys_slist_get(
			&largest_seed_ptr->msg_list);
		if (sys_slist_peek_next((sys_snode_t *)reclaim_msg_ptr) ==
		    NULL) {
			largest_seed_ptr->min_seqno = reclaim_msg_ptr->seq;
		} else {
			largest_seed_ptr->min_seqno =
				((struct mpl_msg *)sys_slist_peek_next(
					 (sys_snode_t *)reclaim_msg_ptr))
					->seq;
		}
		largest_seed_ptr->count--;
		net_trickle_stop(&reclaim_msg_ptr->trickle);
		reclaim_msg_ptr->seed->domain->exp = 0;
		memset(reclaim_msg_ptr, 0, sizeof(struct mpl_msg));
	}

	return reclaim_msg_ptr;
}

static struct mpl_domain *domain_set_allocate(struct in6_addr *address,
					      struct net_if *iface)
{
	struct in6_addr data_addr;
	struct in6_addr ctrl_addr;

	struct net_if_mcast_addr *maddr;

	net_ipaddr_copy(&data_addr, address);
	net_ipaddr_copy(&ctrl_addr, address);

	/* Determine the two addresses for this domain */
	if (net_ipv6_is_addr_mcast_scope(address, 2)) {
		do {
			data_addr.s6_addr[1]++;
			if (net_if_ipv6_maddr_lookup(&data_addr,
						     (struct net_if **)iface)) {
				break;
			}
		} while (data_addr.s6_addr[1] <= 5);

		if (data_addr.s6_addr[1] > 5) {
			NET_ERR("Failed to find MPL domain data address in "
				"table\n");
			return NULL;
		}
	} else {
		ctrl_addr.s6_addr[1] = 0x02;
	}

	for (domain_ptr = &domain_set[CONFIG_NET_MCAST_MPL_DOMAIN_SET_SIZE - 1];
	     domain_ptr >= domain_set; domain_ptr--) {
		if (!DOMAIN_SET_IS_USED(domain_ptr)) {

			maddr = net_if_ipv6_maddr_add(iface, &ctrl_addr);
			if (!maddr) {
				NET_ERR("failed to add ctrl address");
				return NULL;
			}

			net_if_ipv6_maddr_join(maddr);

			net_if_mcast_monitor(iface, &ctrl_addr, true);

			memset(domain_ptr, 0, sizeof(struct mpl_domain));
			memcpy(&domain_ptr->data_addr, &data_addr,
			       sizeof(struct in6_addr));
			memcpy(&domain_ptr->ctrl_addr, &ctrl_addr,
			       sizeof(struct in6_addr));
			if (!net_trickle_create(
				    &domain_ptr->trickle,
				    CONFIG_NET_MCAST_MPL_CONTROL_MESSAGE_IMIN,
				    CONFIG_NET_MCAST_MPL_CONTROL_MESSAGE_IMAX,
				    CONFIG_NET_MCAST_MPL_CONTROL_MESSAGE_K)) {
				LOG_ERR("Unable to configure trickle timer for "
					"domain. Dropping,...\n");
				DOMAIN_SET_CLEAR_USED(domain_ptr);
				return NULL;
			}
			return domain_ptr;
		}
	}
	return NULL;
}

static struct mpl_seed *seed_set_lookup(seed_id_t *seed_id,
					struct mpl_domain *domain)
{
	for (seed_ptr = &seed_set[CONFIG_NET_MCAST_MPL_SEED_SET_SIZE - 1];
	     seed_ptr >= seed_set; seed_ptr--) {
		if (SEED_SET_IS_USED(seed_ptr) &&
		    seed_id_cmp(seed_id, &seed_ptr->seed_id) &&
		    seed_ptr->domain == domain) {
			return seed_ptr;
		}
	}
	return NULL;
}

static struct mpl_seed *seed_set_allocate(void)
{
	for (seed_ptr = &seed_set[CONFIG_NET_MCAST_MPL_SEED_SET_SIZE - 1];
	     seed_ptr >= seed_set; seed_ptr--) {
		if (!SEED_SET_IS_USED(seed_ptr)) {
			seed_ptr->count = 0;

			sys_slist_init(&seed_ptr->msg_list);

			return seed_ptr;
		}
	}
	return NULL;
}

/*
static void seed_set_free(struct mpl_seed *s)
{
	while((msg_ptr = (struct mpl_msg*)sys_slist_get(&s->msg_list)) != NULL) {
		buffer_free(msg_ptr);
	}
	SEED_SET_CLEAR_USED(s);
}*/

static struct mpl_domain *domain_set_lookup(struct in6_addr *domain)
{
	for (domain_ptr = &domain_set[CONFIG_NET_MCAST_MPL_DOMAIN_SET_SIZE - 1];
	     domain_ptr >= domain_set; domain_ptr--) {
		if (DOMAIN_SET_IS_USED(domain_ptr)) {
			if (net_ipv6_addr_cmp(domain, &domain_ptr->data_addr) ||
			    net_ipv6_addr_cmp(domain, &domain_ptr->ctrl_addr)) {
				return domain_ptr;
			}
		}
	}
	return NULL;
}

/*
static void domain_set_free(struct mpl_domain *domain, struct net_if *iface)
{
	struct net_if_mcast_addr *maddr;

	for(seed_ptr = &seed_set[CONFIG_NET_MCAST_MPL_SEED_SET_SIZE]; seed_ptr >= seed_set; seed_ptr--) {
		if(SEED_SET_IS_USED(seed_ptr) && seed_ptr->domain == domain) {
			seed_set_free(seed_ptr);
		}
	}

	maddr = net_if_ipv6_maddr_lookup(&domain->data_addr, &iface);
	if (maddr != NULL) {
		net_if_ipv6_maddr_rm(iface, &domain->data_addr);
		net_if_mcast_monitor(iface, &domain->data_addr, false);
	}

	maddr = net_if_ipv6_maddr_lookup(&domain->ctrl_addr, &iface);
	if (maddr != NULL) {
		net_if_ipv6_maddr_rm(iface, &domain->ctrl_addr);
		net_if_mcast_monitor(iface, &domain->ctrl_addr, false);
	}

	if(net_trickle_is_running(&domain->trickle)) {
		net_trickle_stop(&domain->trickle);
	}

	DOMAIN_SET_CLEAR_USED(domain);
}*/

static void seed_id_net_to_host(seed_id_t *dst, void *src, uint8_t s)
{
	/**
	 * Convert a seed id in network order header format and length S to
	 * internal representation.
	 */
	static uint8_t i;
	static uint8_t *ptr;
	ptr = src;

	switch (s) {
	case 0:
		/* 128 bit seed ID from IPV6 Address */
		dst->s = 0;
		for (i = 0; i < 16; i++) {
			dst->id[i] = ptr[15 - i];
		}
		return;
	case 1:
		/* 16 bit seed ID */
		dst->s = 1;
		for (i = 2; i < 15; i++) {
			/* Clear the first 13 bytes in the id */
			dst->id[i] = 0;
		}
		dst->id[0] = ptr[1];
		dst->id[1] = ptr[0];
		return;
	case 2:
		/* 64 bit Seed ID */
		dst->s = 2;
		for (i = 0; i < 8; i++) {
			/* Reverse the byte order */
			dst->id[i] = ptr[7 - i];
		}
		for (i = 8; i < 16; i++) {
			/* Set the remainder to zero */
			dst->id[i] = 0;
		}
		return;
	case 3:
		/* 128 bit seed ID */
		dst->s = 3;
		for (i = 0; i < 16; i++) {
			dst->id[i] = ptr[15 - i];
		}
		return;
	default:
		/* Invalid seed size */
		return;
	}
}

static void update_seed_id()
{
/* Load my seed ID into memory */
#if MPL_SEED_ID_TYPE == 0
	return;
#elif MPL_SEED_ID_TYPE == 1
	/* 16 bit seed id */
	SEED_ID_S1(&local_seed_id, CONFIG_NET_MCAST_MPL_SEED_ID_L);
#elif MPL_SEED_ID_TYPE == 2
	/* 64 bit seed id */
	SEED_ID_S2(&local_seed_id, CONFIG_NET_MCAST_MPL_SEED_ID_L);
#elif MPL_SEED_ID_TYPE == 3
	/* 128 bit seed id */
	SEED_ID_S3(&local_seed_id, CONFIG_NET_MCAST_MPL_SEED_ID_L,
		   CONFIG_NET_MCAST_MPL_SEED_ID_H);
#endif
}

static inline bool ipv6_drop_on_unknown_option(struct net_pkt *pkt,
					       struct net_ipv6_hdr *hdr,
					       uint8_t opt_type,
					       uint16_t length)
{
	/* RFC 2460 chapter 4.2 tells how to handle the unknown
	 * options by the two highest order bits of the option:
	 *
	 * 00: Skip over this option and continue processing the header.
	 * 01: Discard the packet.
	 * 10: Discard the packet and, regardless of whether or not the
	 *     packet's Destination Address was a multicast address,
	 *     send an ICMP Parameter Problem, Code 2, message to the packet's
	 *     Source Address, pointing to the unrecognized Option Type.
	 * 11: Discard the packet and, only if the packet's Destination
	 *     Address was not a multicast address, send an ICMP Parameter
	 *     Problem, Code 2, message to the packet's Source Address,
	 *     pointing to the unrecognized Option Type.
	 */
	// NET_DBG("Unknown option %d (0x%02x) MSB %d - 0x%02x",
		opt_type, opt_type, opt_type >> 6, opt_type & 0xc0);

	switch (opt_type & 0xc0) {
	case 0x00:
		return false;
	case 0x40:
		break;
	case 0xc0:
		if (net_ipv6_is_addr_mcast(&hdr->dst)) {
			break;
		}

		__fallthrough;
	case 0x80:
		net_icmpv6_send_error(pkt, NET_ICMPV6_PARAM_PROBLEM,
				      NET_ICMPV6_PARAM_PROB_OPTION,
				      (uint32_t)length);
		break;
	}

	return true;
}

static inline int ipv6_handle_ext_hdr_options(struct net_pkt *pkt,
					      struct net_ipv6_hdr *hdr,
					      uint16_t pkt_len)
{
	uint16_t exthdr_len = 0U;
	uint16_t length = 0U;

	{
		uint8_t val = 0U;

		if (net_pkt_read_u8(pkt, &val)) {
			return -ENOBUFS;
		}
		exthdr_len = val * 8U + 8;
	}

	if (exthdr_len > pkt_len) {
		NET_ERR("Corrupted packet, extension header %d too long "
			"(max %d bytes)", exthdr_len, pkt_len);
		return -EINVAL;
	}

	length += 2U;

	while (length < exthdr_len) {
		uint8_t opt_type, opt_len;

		/* Each extension option has type and length */
		if (net_pkt_read_u8(pkt, &opt_type)) {
			return -ENOBUFS;
		}

		if (opt_type != NET_IPV6_EXT_HDR_OPT_PAD1) {
			if (net_pkt_read_u8(pkt, &opt_len)) {
				return -ENOBUFS;
			}
		}

		switch (opt_type) {
		case NET_IPV6_EXT_HDR_OPT_PAD1:
			length++;
			break;
		case NET_IPV6_EXT_HDR_OPT_PADN:
			length += opt_len + 2;

			break;
		default:
			/* Make sure that the option length is not too large.
			 * The former 1 + 1 is the length of extension type +
			 * length fields.
			 * The latter 1 + 1 is the length of the sub-option
			 * type and length fields.
			 */
			if (opt_len > (exthdr_len - (1 + 1 + 1 + 1))) {
				return -EINVAL;
			}

			if (ipv6_drop_on_unknown_option(pkt, hdr, opt_type,
							length)) {
				return -ENOTSUP;
			}

			if (net_pkt_skip(pkt, opt_len)) {
				return -ENOBUFS;
			}

			length += opt_len + 2;

			break;
		}
	}

	return exthdr_len;
}

static void ctrl_message_expiration(struct net_trickle *trickle,
				    bool do_suppress, void *ptr)
{
	domain_ptr = ((struct mpl_domain *)ptr);

	if (domain_ptr->exp >
	    CONFIG_NET_MCAST_MPL_CONTROL_MESSAGE_TIMER_EXPIRATION) {
		net_trickle_stop(&domain_ptr->trickle);
		return;
	}
	if (do_suppress) {
		/* Send an MPL Control Message */
		// icmp_out(locdsptr);
	}

	domain_ptr->exp++;
}

static void data_message_expiration(struct net_trickle *trickle,
				    bool do_suppress, void *ptr)
{
	struct net_pkt *pkt = NULL;
	uint8_t mpl_flags = 0;
	msg_ptr = ((struct mpl_msg *)ptr);

	if (msg_ptr->exp > CONFIG_NET_MCAST_MPL_DATA_MESSAGE_TIMER_EXPIRATION) {
		net_trickle_stop(&msg_ptr->trickle);
		return;
	}

	if (do_suppress) {
		pkt = net_pkt_alloc_with_buffer(
			&msg_ptr->iface, HBHO_BASE_LEN + msg_ptr->size,
			AF_INET6, IPPROTO_IP, PKT_WAIT_TIME);

		if (net_ipv6_create(pkt, &msg_ptr->src,
				    &msg_ptr->seed->domain->data_addr)) {
			return;
		}

		net_pkt_set_ipv6_next_hdr(pkt, NET_IPV6_NEXTHDR_HBHO);
		net_pkt_set_ipv6_ext_len(pkt, MPL_OPT_LEN_S0);

		net_pkt_write_u8(pkt, HBHO_OPT_TYPE_MPL);

		if (sys_slist_peek_next((sys_snode_t *)msg_ptr) == NULL) {
			mpl_flags |= 1 << 2;
		}

		switch (msg_ptr->seed->seed_id.s) {
		case 0:
			net_pkt_write_u8(pkt, MPL_OPT_LEN_S0);
			mpl_flags |= 0x00;
			net_pkt_write_u8(pkt, mpl_flags);
			net_pkt_write_u8(pkt, 0x01);
			net_pkt_write_u8(pkt, 0x00);
			break;
		case 1:
			// placeholder for S=1 option
			break;
		case 2:
			// placeholder for S=2 option
			break;
		case 3:
			// placeholder for S=3 option
			break;
		default:
			NET_ERR("unknown S option");
			return;
		}

		net_pkt_write_u8(pkt, mpl_flags);
		net_pkt_write_u8(pkt, msg_ptr->seq);

		net_pkt_write_u8(pkt, IPPROTO_UDP);
		net_pkt_write(pkt, msg_ptr->data, msg_ptr->size);

		net_pkt_cursor_init(pkt);
		net_udp_finalize(pkt);

		if (net_send_data(pkt) < 0) {
			return;
		}
	}

	msg_ptr->exp++;
}

int net_route_mpl_accept(struct net_pkt *pkt, uint8_t is_input)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(ipv6_access, struct net_ipv6_hdr);
	NET_PKT_DATA_ACCESS_DEFINE(udp_access, struct net_udp_hdr);
	uint8_t nexthdr = 0;
	uint8_t seed_len = 0;
	uint8_t seq_val = 0;
	static seed_id_t seed_id;
	struct net_udp_hdr *udp_hdr;
	int pkt_len = 0;
	int exthdr_len = 0;
	int ext_len = 0;

	static struct mpl_msg *msg_local_ptr;
	struct net_ipv6_hdr *hdr;

	hdr = (struct net_ipv6_hdr *)net_pkt_get_data(pkt, &ipv6_access);
	if (!hdr) {
		return -1;
	}

	if (net_ipv6_is_my_maddr(&hdr->src) && is_input == 0x01) {
		NET_WARN("Received message from ourselves.\n");
		return -1;
	}

	pkt_len = ntohs(hdr->len) + sizeof(struct net_ipv6_hdr);
	nexthdr = hdr->nexthdr;

	/* Check the Next Header field: Must be HBHO */
	if (nexthdr != NET_IPV6_NEXTHDR_HBHO) {
		NET_ERR("Mcast I/O, bad proto\n");
		return -1;
	} else {
		hbh_header_ptr =
			(struct mpl_hbho *)net_pkt_get_data(pkt, &ipv6_access);
		/* Check the Option Type */
		if (hbh_header_ptr->type != HBHO_OPT_TYPE_MPL) {
			LOG_ERR("Mcast I/O, bad HBHO type: %d\n", nexthdr);
			return -1;
		}
	}

	if (HBH_GET_V(hbh_header_ptr)) {
		NET_ERR("invalid V bit");
		return -1;
	}

	seed_len = HBH_GET_S(hbh_header_ptr);

	if (seed_len == 0) {
		seed_id_net_to_host(&seed_id, &hdr->src, seed_len);
	} else {
		seed_id_net_to_host(
			&seed_id,
			&((struct mpl_hbho_s1 *)hbh_header_ptr)->seed_id,
			seed_len);
	}

	domain_ptr = domain_set_lookup(&hdr->dst);

	if (!domain_ptr) {
		domain_ptr = domain_set_allocate(&hdr->dst, net_pkt_iface(pkt));

		if (!domain_ptr) {
			NET_ERR("Could not add Domain to MPL Domain Set");
			return -1;
		}

		if (!net_trickle_create(
			    &domain_ptr->trickle,
			    CONFIG_NET_MCAST_MPL_CONTROL_MESSAGE_IMIN,
			    CONFIG_NET_MCAST_MPL_CONTROL_MESSAGE_IMAX,
			    CONFIG_NET_MCAST_MPL_CONTROL_MESSAGE_K)) {
			NET_ERR("failure creating trickle timer for domain");
			return -1;
		}
	}

	seed_ptr = seed_set_lookup(&seed_id, domain_ptr);

	seq_val = hbh_header_ptr->seq;

	if (seed_ptr) {
		if (SEQ_VAL_IS_LT(hbh_header_ptr->seq, seed_ptr->min_seqno)) {
			return -1;
		}

		if (sys_slist_peek_head(&seed_ptr->msg_list) != NULL) {
			for (msg_ptr = (struct mpl_msg *)sys_slist_peek_head(
				     &seed_ptr->msg_list);
			     msg_ptr != NULL;
			     msg_ptr = (struct mpl_msg *)sys_slist_peek_next(
				     (sys_snode_t *)msg_ptr)) {
				if (SEQ_VAL_IS_EQ(seq_val, msg_ptr->seq)) {
					if (HBH_GET_M(hbh_header_ptr) &&
					    sys_slist_peek_next(
						    (sys_snode_t *)msg_ptr) !=
						    NULL) {
						msg_ptr->exp = 0;
						net_trickle_inconsistency(
							&msg_ptr->trickle);
					} else {
						net_trickle_consistency(
							&msg_ptr->trickle);
					}

					return -1;
				}
			}
		}
	}

	if (!seed_ptr) {
		seed_ptr = seed_set_allocate();
		if (!seed_ptr) {
			NET_ERR("Failed to allocate seed set");
			return -1;
		}
		memset(seed_ptr, 0, sizeof(struct mpl_seed));
		sys_slist_init(&seed_ptr->msg_list);
		seed_id_cpy(&seed_ptr->seed_id, &seed_id);
		seed_ptr->domain = domain_ptr;
	}

	msg_ptr = buffer_allocate();

	if (!msg_ptr) {
		msg_ptr = buffer_reclaim();

		if (!msg_ptr) {
			NET_ERR("buffer reclaim failed");
			return -1;
		}
	}

	memcpy(&msg_ptr->iface, net_pkt_iface(pkt), sizeof(struct net_if));

	net_ipaddr_copy(&msg_ptr->src, &hdr->src);

	while (!net_ipv6_is_nexthdr_upper_layer(nexthdr)) {
		if (net_pkt_read_u8(pkt, &nexthdr)) {
			return -1;
		}
		exthdr_len = ipv6_handle_ext_hdr_options(pkt, hdr, pkt_len);
		if (exthdr_len < 0) {
			return -1;
		}

		ext_len += exthdr_len;
	}

	net_pkt_set_ipv6_ext_len(pkt, ext_len);

	udp_hdr = (struct net_udp_hdr *)net_pkt_get_data(pkt, &udp_access);
	if (!udp_hdr || net_pkt_set_data(pkt, &udp_access)) {
		NET_ERR("DROP: corrupted header");
		return -1;
	}

	if (ntohs(udp_hdr->len) !=
	    (net_pkt_get_len(pkt) - net_pkt_ip_hdr_len(pkt) -
	     net_pkt_ip_opts_len(pkt))) {
		NET_ERR("DROP: Invalid hdr length");
		return -1;
	}

	msg_ptr->size = udp_hdr->len;
	memcpy(&msg_ptr->data, udp_hdr, udp_hdr->len);
	msg_ptr->seq = hbh_header_ptr->seq;
	msg_ptr->seed = seed_ptr;

	if (!net_trickle_create(&msg_ptr->trickle,
				CONFIG_NET_MCAST_MPL_DATA_MESSAGE_IMIN,
				CONFIG_NET_MCAST_MPL_DATA_MESSAGE_IMAX,
				CONFIG_NET_MCAST_MPL_DATA_MESSAGE_K)) {
		NET_ERR("Failed to create trickle timer for message");
		buffer_free(msg_ptr);
		return -1;
	}

	if (sys_slist_peek_head(&seed_ptr->msg_list) == NULL) {
		sys_slist_prepend(&seed_ptr->msg_list, (sys_snode_t *)msg_ptr);
		seed_ptr->min_seqno = msg_ptr->seq;
	} else {
		for (msg_local_ptr = (struct mpl_msg *)sys_slist_peek_head(
			     &seed_ptr->msg_list);
		     msg_local_ptr != NULL;
		     msg_local_ptr = (struct mpl_msg *)sys_slist_peek_next(
			     (sys_snode_t *)msg_local_ptr)) {
			if (sys_slist_peek_next((sys_snode_t *)msg_local_ptr) ==
				    NULL ||
			    (SEQ_VAL_IS_GT(msg_ptr->seq, msg_local_ptr->seq) &&
			     SEQ_VAL_IS_LT(
				     msg_ptr->seq,
				     ((struct mpl_msg *)sys_slist_peek_next(
					      (sys_snode_t *)msg_local_ptr))
					     ->seq))) {
				sys_slist_insert(&seed_ptr->msg_list,
						 (sys_snode_t *)msg_local_ptr,
						 (sys_snode_t *)msg_ptr);
				break;
			}
		}
	}

	seed_ptr->count++;

#if defined(CONFIG_NET_MCAST_MPL_PROACTIVE)
	msg_ptr->exp = 0;
	net_trickle_start(&msg_ptr->trickle, data_message_expiration, msg_ptr);
#endif
	seed_ptr->lifetime = CONFIG_NET_MCAST_MPL_SEED_SET_ENTRY_LIFETIME;

#if CONFIG_NET_MCAST_MPL_DATA_MESSAGE_TIMER_EXPIRATION > 0
	domain_ptr->exp = 0;
	net_trickle_start(&domain_ptr->trickle, ctrl_message_expiration,
			  domain_ptr);
#endif

#if defined(CONFIG_NET_MCAST_MPL_PROACTIVE)
	if (HBH_GET_M(hbh_header_ptr) == 1 &&
	    sys_slist_peek_next((sys_snode_t *)msg_ptr) != NULL) {
		msg_ptr->exp = 0;
		net_trickle_inconsistency(&msg_ptr->trickle);
	} else {
		net_trickle_consistency(&msg_ptr->trickle);
	}
#endif

	return 1;
}

void net_route_mpl_send_data(struct net_pkt *pkt)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(ipv6_access, struct net_ipv6_hdr);
	struct net_pkt *mpl_pkt;
	struct net_ipv6_hdr *hdr;
	uint8_t mpl_flags = 0;

	if (local_seed_id.s == MPL_SEED_ID_UNKNOWN) {
		update_seed_id();
		if (local_seed_id.s == MPL_SEED_ID_UNKNOWN) {
			LOG_ERR("Our seed ID is not yet known.\n");
			return;
		}
	}

	if (net_pkt_available_buffer(pkt) < HBHO_TOTAL_LEN) {
		LOG_ERR("Multicast Out can not add HBHO. Packet too long\n");
		return;
	}

	hdr = (struct net_ipv6_hdr *)net_pkt_get_data(pkt, &ipv6_access);
	if (!hdr) {
		NET_ERR("DROP: no buffer");
		return;
	}

	mpl_pkt = net_pkt_alloc_with_buffer(
		net_pkt_iface(pkt), HBHO_BASE_LEN + net_pkt_get_len(pkt),
		AF_INET6, IPPROTO_IP, PKT_WAIT_TIME);

	if (net_ipv6_create(mpl_pkt, &hdr->src, &hdr->dst)) {
		NET_ERR("DROP: wrong buffer");
		return;
	}

	net_pkt_set_ipv6_next_hdr(mpl_pkt, NET_IPV6_NEXTHDR_HBHO);
	net_pkt_set_ipv6_ext_len(mpl_pkt, MPL_OPT_LEN_S0);

	net_pkt_write_u8(mpl_pkt, HBHO_OPT_TYPE_MPL);

	net_pkt_write_u8(mpl_pkt, MPL_OPT_LEN_S0);
	mpl_flags |= 0x00;   // S=0
	mpl_flags |= 1 << 2; // M=1

	net_pkt_write_u8(mpl_pkt, mpl_flags);

	last_seq = SEQ_VAL_ADD(last_seq, 1);
	net_pkt_write_u8(mpl_pkt, last_seq);

	net_pkt_write_u8(mpl_pkt, 0x01);
	net_pkt_write_u8(mpl_pkt, 0x00);

	net_pkt_copy(mpl_pkt + net_pkt_get_len(mpl_pkt), pkt,
		     net_pkt_get_len(pkt));

	if (net_route_mpl_accept(mpl_pkt, 0)) {
		net_send_data(pkt);
	}
}
