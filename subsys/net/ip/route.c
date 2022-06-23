/** @file
 * @brief Route handling.
 *
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_route, CONFIG_NET_ROUTE_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <limits.h>
#include <zephyr/types.h>
#include <zephyr/sys/slist.h>

#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_stats.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_ip.h>

#include "net_private.h"
#include "ipv6.h"
#include "icmpv6.h"
#include "nbr.h"
#include "route.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//MPL Routing
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(CONFIG_NET_MCAST_MPL)

#include <kernel.h>
#include <limits.h>
#include <zephyr/types.h>
#include <sys/slist.h>
#include <net/trickle.h>
#include <sys/sflist.h>
#include "udp_internal.h"

#define MPL_SEED_ID_UNKNOWN 0xFF
#define PKT_WAIT_TIME K_SECONDS(1)

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
  struct net_if iface;
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

struct seed_info {
  uint8_t min_seqno;
  uint8_t bm_len_S; 
  uint8_t seed_id[16];
};
struct seed_info_s0 {
  uint8_t min_seqno;
  uint8_t bm_len_S; 
};
struct seed_info_s1 {
  uint8_t min_seqno;
  uint8_t bm_len_S; 
  uint16_t seed_id;
};
struct seed_info_s2 {
  uint8_t min_seqno;
  uint8_t bm_len_S; 
  uint64_t seed_id;
};
struct seed_info_s3 {
  uint8_t min_seqno;
  uint8_t bm_len_S; 
  uint8_t seed_id[16];
};

static struct mpl_msg buffered_message_set[CONFIG_NET_MCAST_MPL_BUFFERED_MESSAGE_SET_SIZE];
static struct mpl_seed seed_set[CONFIG_NET_MCAST_MPL_SEED_SET_SIZE];
static struct mpl_domain domain_set[CONFIG_NET_MCAST_MPL_DOMAIN_SET_SIZE];

static bool init_done = false;
static uint16_t last_seq;

//static struct ctimer lifetime_timer;

#define MPL_SEED_ID_UNKNOWN 0xFF
/* Define a way of logging the seed id */

#define MSG_SET_IS_USED(h) ((h)->seed != NULL)
#define MSG_SET_CLEAR_USED(h) ((h)->seed = NULL)
#define SEED_SET_IS_USED(h) (((h)->domain != NULL))
#define SEED_SET_CLEAR_USED(h) ((h)->domain = NULL)
#define DOMAIN_SET_IS_USED(h) (net_ipv6_is_addr_mcast(&(h)->data_addr))
#define DOMAIN_SET_CLEAR_USED(h) (memset(&(h)->data_addr, 0, sizeof(struct in6_addr)))
#define SEED_ID_S1(dst, src) { (*(uint16_t *)&(dst)->id) = (src); (dst)->s = 1; }
#define SEED_ID_S2(dst, src) { (*(uint64_t *)&(dst)->id) = (src); (dst)->s = 2; }
#define SEED_ID_S3(dst, l, h) { (*(uint64_t *)&(dst)->id) = (l); (*(uint64_t *)&(dst)->id[8]) = (h); (dst)->s = 3; }
#define HBH_GET_S(h) (((h)->flags & 0xC0) >> 6)
#define HBH_SET_S(h, s) ((h)->flags |= ((s & 0x03) << 6))
#define HBH_CLR_S(h) ((h)->flags &= ~0xC0)
#define HBH_GET_M(h) (((h)->flags & 0x20) == 0x20)
#define HBH_SET_M(h) ((h)->flags |= 0x20)
#define HBH_GET_V(h) (((h)->flags & 0x10) == 0x10)
#define HBH_CLR_V(h) ((h)->flags &= ~0x10)
#define SEQ_VAL_IS_EQ(i1, i2) ((i1) == (i2))
#define SEQ_VAL_IS_LT(i1, i2) \
  ( \
    ((i1) != (i2)) && \
    ((((i1) < (i2)) && ((int16_t)((i2) - (i1)) < 0x100)) || \
     (((i1) > (i2)) && ((int16_t)((i1) - (i2)) > 0x100))) \
  )
#define SEQ_VAL_IS_GT(i1, i2) \
  ( \
    ((i1) != (i2)) && \
    ((((i1) < (i2)) && ((int16_t)((i2) - (i1)) > 0x100)) || \
     (((i1) > (i2)) && ((int16_t)((i1) - (i2)) < 0x100))) \
  )
#define SEQ_VAL_ADD(s, n) (((s) + (n)) % 0x100)
#define seed_id_cmp(a, b) (memcmp((a)->id, (b)->id, sizeof(uint8_t) * 16) == 0)
#define seed_id_cpy(a, b) (memcpy((a), (b), sizeof(seed_id_t)))
#define seed_id_clr(a) (memset((a), 0, sizeof(seed_id_t)))
#define SEED_INFO_GET_S(h) ((h)->bm_len_S & 0x03)
#define SEED_INFO_CLR_S(h) ((h)->bm_len_S &= ~0x03)
#define SEED_INFO_SET_S(h, s) ((h)->bm_len_S |= (s & 0x03))
#define SEED_INFO_GET_LEN(h) (((h)->bm_len_S >> 2) & 0x3F)
#define SEED_INFO_CLR_LEN(h) ((h)->bm_len_S &= 0x03)
#define SEED_INFO_SET_LEN(h, l) ((h)->bm_len_S |= (l << 2))

static struct mpl_msg* buffer_allocate(void)
{
	struct mpl_msg *msg_ptr; 

	for(msg_ptr = &buffered_message_set[CONFIG_NET_MCAST_MPL_BUFFERED_MESSAGE_SET_SIZE-1]; 
		msg_ptr >= buffered_message_set; 
		msg_ptr--) {
		if(!MSG_SET_IS_USED(msg_ptr)) {
			memset(msg_ptr, 0, sizeof(struct mpl_msg));
			return msg_ptr;
		}
	}
	return NULL;
}

static void buffer_free(struct mpl_msg *msg)
{
	if(net_trickle_is_running(&msg->trickle)) {
		net_trickle_stop(&msg->trickle);
	}
	MSG_SET_CLEAR_USED(msg);
}

static struct mpl_msg* buffer_reclaim(void)
{
	struct mpl_msg *reclaim_msg_ptr;
	struct mpl_seed *seed_ptr;
	struct mpl_seed *largest_seed_ptr;
  
	largest_seed_ptr = NULL;
	reclaim_msg_ptr = NULL;
	for(seed_ptr = &seed_set[CONFIG_NET_MCAST_MPL_SEED_SET_SIZE]; seed_ptr >= seed_set; seed_ptr--) {
		if(SEED_SET_IS_USED(seed_ptr) && (largest_seed_ptr == NULL || seed_ptr->count > largest_seed_ptr->count)) {
			largest_seed_ptr = seed_ptr;
		}
	}

	if(largest_seed_ptr != NULL) {
		reclaim_msg_ptr = (struct mpl_msg*)sys_slist_get(&largest_seed_ptr->msg_list);
		if(sys_slist_peek_next((sys_snode_t *)reclaim_msg_ptr) == NULL){
			largest_seed_ptr->min_seqno = reclaim_msg_ptr->seq;
		}else{
		 	largest_seed_ptr->min_seqno = ((struct mpl_msg *)sys_slist_peek_next((sys_snode_t *)reclaim_msg_ptr))->seq;
		}
		largest_seed_ptr->count--;
		net_trickle_stop(&reclaim_msg_ptr->trickle);
		reclaim_msg_ptr->seed->domain->exp = 0;
		memset(reclaim_msg_ptr, 0, sizeof(struct mpl_msg));
	}

	return reclaim_msg_ptr;
}

static void mpl_init()
{
	memset(domain_set, 0, sizeof(struct mpl_domain) * CONFIG_NET_MCAST_MPL_DOMAIN_SET_SIZE);
	memset(seed_set, 0, sizeof(struct mpl_seed) * CONFIG_NET_MCAST_MPL_SEED_SET_SIZE);
	memset(buffered_message_set, 0, sizeof(struct mpl_msg) * CONFIG_NET_MCAST_MPL_BUFFERED_MESSAGE_SET_SIZE);

	init_done = true;
}

static struct mpl_domain* domain_set_allocate(struct in6_addr *address, struct net_if *iface)
{
	struct mpl_domain *domain_ptr;
	struct in6_addr data_addr;
	struct in6_addr ctrl_addr;
	
	struct net_if_mcast_addr *maddr;

	net_ipaddr_copy(&data_addr, address);
	net_ipaddr_copy(&ctrl_addr, address);

	if(net_ipv6_is_addr_mcast_scope(address, 2)) {
		do {
			data_addr.s6_addr[1]++;

			if(net_if_ipv6_maddr_lookup(&data_addr, &iface)) {
				break;
			}
		} while(data_addr.s6_addr[1] <= 5);

		if(data_addr.s6_addr[1] > 5) {
			NET_ERR("Failed to find MPL domain data address in table\n");
			return NULL;
		}
	} else {
		ctrl_addr.s6_addr[1] = 0x02;
	}

	for(domain_ptr = &domain_set[CONFIG_NET_MCAST_MPL_DOMAIN_SET_SIZE - 1]; domain_ptr >= domain_set; domain_ptr--) {
		if(!DOMAIN_SET_IS_USED(domain_ptr)) {

			maddr = net_if_ipv6_maddr_add(iface, &ctrl_addr);
			if (!maddr) {
				return NULL;
			}
			
			net_if_ipv6_maddr_join(maddr);

			net_if_mcast_monitor(iface, (struct net_addr *) &ctrl_addr, true);

			memset(domain_ptr, 0, sizeof(struct mpl_domain));
			memcpy(&domain_ptr->data_addr, &data_addr, sizeof(struct in6_addr));
			memcpy(&domain_ptr->ctrl_addr, &ctrl_addr, sizeof(struct in6_addr));
			memcpy(&domain_ptr->iface, iface, sizeof(struct net_if));

			if(net_trickle_create(&domain_ptr->trickle,
			               CONFIG_NET_MCAST_MPL_CONTROL_MESSAGE_IMIN,
			               CONFIG_NET_MCAST_MPL_CONTROL_MESSAGE_IMAX,
			               CONFIG_NET_MCAST_MPL_CONTROL_MESSAGE_K)) {
				LOG_ERR("Unable to configure trickle timer for domain. Dropping,...\n");
				DOMAIN_SET_CLEAR_USED(domain_ptr);
				return NULL;
			}

			return domain_ptr;
		}
	}
	return NULL;
}

static struct mpl_seed* seed_set_lookup(struct in6_addr *seed_id, struct mpl_domain *domain)
{
	struct mpl_seed *seed_ptr;

	for(seed_ptr = &seed_set[CONFIG_NET_MCAST_MPL_SEED_SET_SIZE - 1]; seed_ptr >= seed_set; seed_ptr--) {
		if(SEED_SET_IS_USED(seed_ptr) && net_ipv6_addr_cmp((struct in6_addr *)&seed_ptr->seed_id.id, seed_id) && seed_ptr->domain == domain) {
			return seed_ptr;
		}
	}
	return NULL;
}

static struct mpl_seed* seed_set_allocate(void)
{
	struct mpl_seed *seed_ptr;

	for(seed_ptr = &seed_set[CONFIG_NET_MCAST_MPL_SEED_SET_SIZE - 1]; seed_ptr >= seed_set; seed_ptr--) {
		if(!SEED_SET_IS_USED(seed_ptr)) {
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

static struct mpl_domain* domain_set_lookup(struct in6_addr *domain)
{
	struct mpl_domain *domain_ptr;

	for(domain_ptr = &domain_set[CONFIG_NET_MCAST_MPL_DOMAIN_SET_SIZE - 1]; domain_ptr >= domain_set; domain_ptr--) {
		if(DOMAIN_SET_IS_USED(domain_ptr)) {
			if(net_ipv6_addr_cmp(domain, &domain_ptr->data_addr)
				|| net_ipv6_addr_cmp(domain, &domain_ptr->ctrl_addr)) {
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

static inline bool ipv6_drop_on_unknown_option(struct net_pkt *pkt,
					       struct net_ipv6_hdr *hdr,
					       uint8_t opt_type,
					       uint16_t length)
{
	switch (opt_type & 0xc0) {
	case 0x00:
		return false;
	case 0x40:
		break;
	case 0xc0:
		if (net_ipv6_is_addr_mcast((struct in6_addr *) &hdr->dst)) {
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
			if (opt_len > (exthdr_len - (1 + 1 + 1 + 1))) {
				return -EINVAL;
			}

			if (ipv6_drop_on_unknown_option(pkt, hdr,
							opt_type, length)) {
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

#ifndef NET_MCAST_MPL_FLOODING
static void ctrl_message_out(struct mpl_domain *domain)
{
	struct mpl_msg *msg_ptr;
	struct mpl_seed *seed_ptr;

	uint8_t vector[32];
	uint8_t vec_len;
	uint8_t vec_size;
	uint8_t cur_seq;

	uint8_t payload_buffer[CONFIG_NET_BUF_DATA_SIZE];
	struct net_pkt *pkt = NULL;
	struct seed_info *info;
	uint16_t info_cursor = 0;

	struct in6_addr *addr;
	struct in6_addr src;
	struct in6_addr dst;

	info = (struct seed_info*)payload_buffer;

	net_ipaddr_copy(&dst, &domain->ctrl_addr);
	net_ipaddr_copy(&src, net_if_ipv6_select_src_addr(&domain->iface, &domain->data_addr));

	for(seed_ptr = &seed_set[CONFIG_NET_MCAST_MPL_SEED_SET_SIZE-1];
			seed_ptr >= seed_set; seed_ptr--) {
		if(SEED_SET_IS_USED(seed_ptr) && seed_ptr->domain == domain) {
			info->min_seqno = seed_ptr->min_seqno;
			SEED_INFO_CLR_LEN(info);
			SEED_INFO_CLR_S(info);

			addr = net_if_ipv6_get_global_addr(NET_ADDR_PREFERRED, (struct net_if **)&domain->iface);

			if(addr) {
				net_ipaddr_copy(&src, addr);
			} else {
				net_ipaddr_copy(&src, net_if_ipv6_select_src_addr(&domain->iface, &domain->data_addr));

				if(net_ipv6_is_addr_unspecified(&src)) {
					NET_ERR("icmp out: Cannot set src ip\n");
					return;
				}
			}

			switch(seed_ptr->seed_id.s) {
				case 0:
					if(net_ipv6_addr_cmp((struct in6_addr *)&seed_ptr->seed_id.id, &src)) {
						SEED_INFO_SET_LEN(info, 0);
						break;
					} 
				case 3:
					memcpy(info->seed_id, seed_ptr->seed_id.id, 16);
        	SEED_INFO_SET_S(info, 3);
					break;
				case 1:
					
					break;
				case 2:
					
					break;
				default:

					break;
			}

			memset(vector, 0, sizeof(vector));
			vec_len = 0;
			cur_seq = 0;

			for(msg_ptr = (struct mpl_msg *)sys_slist_peek_head(&seed_ptr->msg_list); msg_ptr != NULL; msg_ptr = (struct mpl_msg *)sys_slist_peek_next((sys_snode_t *)msg_ptr)) {
				cur_seq = SEQ_VAL_ADD(seed_ptr->min_seqno, vec_len);

				if(msg_ptr->seq == SEQ_VAL_ADD(seed_ptr->min_seqno, vec_len)) {
					vector[vec_len/8] |= 0x01<<(vec_len%8);
					vec_len++;
				} else {
					vec_len += msg_ptr->seq - cur_seq;
					vector[vec_len/8] |= 0x01<<(vec_len%8);
					vec_len++;
				}
			}

			vec_size = (vec_len - 1) / 8 + 1;

			SEED_INFO_SET_LEN(info, vec_size);

			switch(SEED_INFO_GET_S(info)) {
			case 0:
				info_cursor += sizeof(struct seed_info_s0);
				break;
			case 1:
				info_cursor += sizeof(struct seed_info_s1);
				break;
			case 2:
				info_cursor += sizeof(struct seed_info_s2);
				break;
			case 3:
				info_cursor += sizeof(struct seed_info_s3);
				break;
			}

			memcpy(&payload_buffer[info_cursor], vector, vec_size);
			info_cursor += vec_size;

			info = (struct seed_info*)&payload_buffer[info_cursor];
		}
	}

	pkt = net_pkt_alloc_with_buffer(&domain->iface, info_cursor,
				  AF_INET6, IPPROTO_ICMPV6,
				  PKT_WAIT_TIME);

	if (net_ipv6_create(pkt, &src, &dst)) {
		NET_ERR("DROP: wrong buffer");
		return;
	}

	if(net_icmpv6_create(pkt, ICMPV6_MPL, 0)) {
		NET_ERR("DROP: cannot setup icmp packet");
		return;
	}

	net_pkt_write(pkt, payload_buffer, info_cursor);

	net_pkt_cursor_init(pkt);
	net_ipv6_finalize(pkt, IPPROTO_ICMPV6);

	net_pkt_cursor_init(pkt);

	if (net_send_data(pkt) < 0) {
		NET_ERR("net_send_data failed");
		return;
	}

#if defined(CONFIG_NET_STATISTICS_MPL)
	net_stats_update_mpl_ctrl_sent(net_pkt_iface(pkt));
#endif /* CONFIG_NET_STATISTICS_MPL */
}

static void ctrl_message_expiration(struct net_trickle *trickle, bool tx_allowed, void *ptr)
{
	struct mpl_domain *domain_ptr;

	domain_ptr = ((struct mpl_domain *)ptr);

	if(domain_ptr->exp >= CONFIG_NET_MCAST_MPL_CONTROL_MESSAGE_TIMER_EXPIRATION) {
		net_trickle_stop(&domain_ptr->trickle);
		return;
	}
	if(tx_allowed) {
		ctrl_message_out(domain_ptr);
	}

	domain_ptr->exp++;
}

static void data_message_expiration(struct net_trickle *trickle, bool tx_allowed, void *ptr)
{
	struct mpl_msg *msg_ptr;
	struct net_pkt *pkt = NULL;
	uint8_t mpl_flags = 0;
	msg_ptr = ((struct mpl_msg *)ptr);

	if(msg_ptr->exp >= CONFIG_NET_MCAST_MPL_DATA_MESSAGE_TIMER_EXPIRATION) {
		net_trickle_stop(&msg_ptr->trickle);
		return;
	}

	if(tx_allowed) {
		pkt = net_pkt_alloc_with_buffer(&msg_ptr->iface, HBHO_BASE_LEN + msg_ptr->size,
					  AF_INET6, IPPROTO_IP,
					  PKT_WAIT_TIME);

		if(pkt == NULL) {
		}
		int ret = 0;
		ret = net_ipv6_create(pkt, &msg_ptr->src, &msg_ptr->seed->domain->data_addr);
		if (ret) {
			NET_ERR("DROP: wrong buffer %d", ret);
			return;
		}

		net_pkt_set_iface(pkt, &msg_ptr->iface);

		net_pkt_write_u8(pkt, IPPROTO_UDP);
		net_pkt_write_u8(pkt, 0);

		net_pkt_set_ipv6_next_hdr(pkt, NET_IPV6_NEXTHDR_HBHO);
		net_pkt_set_ipv6_ext_len(pkt, HBHO_TOTAL_LEN);

		net_pkt_write_u8(pkt, HBHO_OPT_TYPE_MPL);
		net_pkt_write_u8(pkt, 4);

		if(sys_slist_peek_next((sys_snode_t *)msg_ptr) == NULL) {
			mpl_flags |= 1<<2;
		}

		switch(msg_ptr->seed->seed_id.s) {
			case 0:
				mpl_flags |= 0x00;
				break;
			case 1:
				//placeholder for S=1 option
				break;
			case 2:
				//placeholder for S=2 option
				break;
			case 3:
				//placeholder for S=3 option
				break;
			default:
				NET_ERR("unknown S option");
				return;
		}

		net_pkt_write_u8(pkt, mpl_flags);
		net_pkt_write_u8(pkt, msg_ptr->seq);
		net_pkt_write_u8(pkt, 0x01);
		net_pkt_write_u8(pkt, 0x00);

		net_pkt_write(pkt, msg_ptr->data, 6);
		net_pkt_write_le16(pkt, 0x00);
		net_pkt_write(pkt, (msg_ptr->data)+8, (msg_ptr->size)-8);

		net_pkt_cursor_init(pkt);
		net_ipv6_finalize(pkt, IPPROTO_UDP);
		
		if (net_send_data(pkt) < 0) {
			NET_ERR("net_send_data failed");
			return;
		}

#if defined(CONFIG_NET_STATISTICS_MPL)
		net_stats_update_mpl_data_sent(net_pkt_iface(pkt));
#endif /* CONFIG_NET_STATISTICS_MPL */
	}

	msg_ptr->exp++;
}
#endif

int net_route_mpl_accept(struct net_pkt *pkt, uint8_t is_input)
{
	struct mpl_domain *domain_ptr;
	struct mpl_msg *msg_ptr;
	struct mpl_seed *seed_ptr;
	struct mpl_hbho *hbh_header_ptr;  
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(ipv6_access, struct net_ipv6_hdr);
	NET_PKT_DATA_ACCESS_DEFINE(udp_access, struct net_udp_hdr);
	NET_PKT_DATA_ACCESS_DEFINE(hbho_access, struct mpl_hbho);
	uint8_t nexthdr = 0;
	uint8_t seed_len = 0;
	uint8_t seq_val = 0;
	static seed_id_t seed_id;
	struct net_udp_hdr *udp_hdr;
	int pkt_len = 0;
	uint8_t test = 0;

#ifdef NET_MCAST_MPL_FLOODING
	struct net_pkt *pkt = NULL;
	uint8_t mpl_flags = 0;
#endif

	static struct mpl_msg *msg_local_ptr;
	struct net_ipv6_hdr *hdr;

	if(!init_done) {
		mpl_init();
	}
	
	hdr = (struct net_ipv6_hdr *)net_pkt_get_data(pkt, &ipv6_access);
	if (!hdr) {
		NET_ERR("DROP: no buffer");
		return -1;
	}
	net_pkt_acknowledge_data(pkt, &ipv6_access);

	if(net_ipv6_is_my_maddr(&hdr->src) && is_input == 0x01) {
		NET_WARN("Received message from ourselves.\n");
#if defined(CONFIG_NET_STATISTICS_MPL)
		net_stats_update_mpl_old_msg_recv(net_pkt_iface(pkt));
#endif /* CONFIG_NET_STATISTICS_MPL */
		return -1;
	}

	pkt_len = ntohs(hdr->len) + sizeof(struct net_ipv6_hdr);

	nexthdr = hdr->nexthdr;

	if(nexthdr != NET_IPV6_NEXTHDR_HBHO) {
		NET_ERR("Mcast I/O, bad proto\n");

		if(nexthdr == IPPROTO_ICMPV6) {
			return 1;
		}

		return -1;
	} else {
		net_pkt_read_u8(pkt, &nexthdr);
		net_pkt_read_u8(pkt, &test);

		if(nexthdr != IPPROTO_UDP) {
			NET_ERR("missing UDP header");
		}

		hbh_header_ptr = (struct mpl_hbho*)net_pkt_get_data(pkt, &hbho_access);
		if(hbh_header_ptr->type != HBHO_OPT_TYPE_MPL) {
			LOG_ERR("Mcast I/O, bad HBHO type: %d\n",nexthdr);
			return -1;
		}
	}
	
	if(HBH_GET_V(hbh_header_ptr)) {
		NET_ERR("invalid V bit");
		return -1;
	}
	
	if(is_input) {
#if defined(CONFIG_NET_STATISTICS_MPL)
		net_stats_update_mpl_data_recv(net_pkt_iface(pkt));
#endif /* CONFIG_NET_STATISTICS_MPL */
	}
	
	seed_len = HBH_GET_S(hbh_header_ptr);

	memcpy(&seed_id.id, &hdr->src, 16);

	domain_ptr = domain_set_lookup(&hdr->dst);

	if(!domain_ptr) {
		domain_ptr = domain_set_allocate(&hdr->dst, net_pkt_iface(pkt));

		if(!domain_ptr) {
			NET_ERR("Could not add Domain to MPL Domain Set");
			return -1;
		}
#ifndef NET_MCAST_MPL_FLOODING
		if(net_trickle_create(&domain_ptr->trickle,
								CONFIG_NET_MCAST_MPL_CONTROL_MESSAGE_IMIN,
								CONFIG_NET_MCAST_MPL_CONTROL_MESSAGE_IMAX,
								CONFIG_NET_MCAST_MPL_CONTROL_MESSAGE_K)) {
			NET_ERR("failure creating trickle timer for domain");
			return -1;
		}
#endif
	}

	seed_ptr = seed_set_lookup(&hdr->src, domain_ptr);
	seq_val = hbh_header_ptr->seq;

	if(seed_ptr) {
		if(SEQ_VAL_IS_LT(hbh_header_ptr->seq, seed_ptr->min_seqno)) {
			if(is_input) {
#if defined(CONFIG_NET_STATISTICS_MPL)
					net_stats_update_mpl_old_msg_recv(net_pkt_iface(pkt));
#endif /* CONFIG_NET_STATISTICS_MPL */
			}
			return -1;
		}

		if(sys_slist_peek_head(&seed_ptr->msg_list) != NULL) {
			for(msg_ptr = (struct mpl_msg *)sys_slist_peek_head(&seed_ptr->msg_list); msg_ptr != NULL; msg_ptr = (struct mpl_msg *)sys_slist_peek_next((sys_snode_t *)msg_ptr)) {
				if(SEQ_VAL_IS_EQ(seq_val, msg_ptr->seq)) {
#ifndef NET_MCAST_MPL_FLOODING
					if(HBH_GET_M(hbh_header_ptr) && sys_slist_peek_next((sys_snode_t *)msg_ptr) != NULL) {
						msg_ptr->exp = 0;
						net_trickle_inconsistency(&msg_ptr->trickle);
					} else {
						net_trickle_consistency(&msg_ptr->trickle);
					}
#endif
					if(is_input) {
#if defined(CONFIG_NET_STATISTICS_MPL)
						net_stats_update_mpl_old_msg_recv(net_pkt_iface(pkt));
#endif /* CONFIG_NET_STATISTICS_MPL */
					}
					return -1;
				}
			}
		}
	}

	if(is_input) {
#if defined(CONFIG_NET_STATISTICS_MPL)
		net_stats_update_mpl_new_msg_recv(net_pkt_iface(pkt));
#endif /* CONFIG_NET_STATISTICS_MPL */
	}

	if(!seed_ptr) {
		seed_ptr = seed_set_allocate();
		if(!seed_ptr) {
			NET_ERR("Failed to allocate seed set");
			return -1;
		}
		memset(seed_ptr, 0, sizeof(struct mpl_seed));
		sys_slist_init(&seed_ptr->msg_list);
		seed_id_cpy(&seed_ptr->seed_id, &seed_id);

		seed_ptr->domain = domain_ptr;
	}

	msg_ptr = buffer_allocate();

	if(!msg_ptr) {
		msg_ptr = buffer_reclaim();

		if(!msg_ptr) {
			NET_ERR("buffer reclaim failed");
			return -1;
		}
	}

	memcpy(&msg_ptr->iface, net_pkt_iface(pkt), sizeof(struct net_if));

	net_ipaddr_copy((struct in6_addr *) &msg_ptr->src,(struct in6_addr *) &hdr->src);

	net_pkt_read_u8(pkt, &test);
	net_pkt_read_u8(pkt, &test);

	net_pkt_acknowledge_data(pkt, &hbho_access);

	udp_hdr = (struct net_udp_hdr *)net_pkt_get_data(pkt, &udp_access);
	if (!udp_hdr) {
		NET_ERR("DROP: corrupted header");
		return -1;
	}
	
	msg_ptr->size = ntohs(udp_hdr->len);
	memcpy(&msg_ptr->data, udp_hdr, ntohs(udp_hdr->len));
	msg_ptr->seq = hbh_header_ptr->seq;
	msg_ptr->seed = seed_ptr;

#ifndef NET_MCAST_MPL_FLOODING
	if(net_trickle_create(&msg_ptr->trickle,
							CONFIG_NET_MCAST_MPL_DATA_MESSAGE_IMIN,
							CONFIG_NET_MCAST_MPL_DATA_MESSAGE_IMAX,
							CONFIG_NET_MCAST_MPL_DATA_MESSAGE_K)) {
		NET_ERR("Failed to create trickle timer for message");
		buffer_free(msg_ptr);
		return -1;
	}
#endif

	if(sys_slist_peek_head(&seed_ptr->msg_list) == NULL) {
		sys_slist_prepend(&seed_ptr->msg_list, (sys_snode_t*)msg_ptr);
		seed_ptr->min_seqno = msg_ptr->seq;
	} else {
		for(msg_local_ptr = (struct mpl_msg *)sys_slist_peek_head(&seed_ptr->msg_list); msg_local_ptr != NULL; msg_local_ptr = (struct mpl_msg *)sys_slist_peek_next((sys_snode_t*)msg_local_ptr)) {
			if(sys_slist_peek_next((sys_snode_t*)msg_local_ptr) == NULL ||
				(SEQ_VAL_IS_GT(msg_ptr->seq, msg_local_ptr->seq) && SEQ_VAL_IS_LT(msg_ptr->seq, ((struct mpl_msg *)sys_slist_peek_next((sys_snode_t*)msg_local_ptr))->seq))) {
				sys_slist_insert(&seed_ptr->msg_list, (sys_snode_t*)msg_local_ptr, (sys_snode_t*)msg_ptr);
				break;
			}
		}
	}

	seed_ptr->count++;

#ifndef NET_MCAST_MPL_FLOODING
#if defined(CONFIG_NET_MCAST_MPL_PROACTIVE)
	msg_ptr->exp = 0;
	net_trickle_start(&msg_ptr->trickle, data_message_expiration, msg_ptr);
#endif

	seed_ptr->lifetime = CONFIG_NET_MCAST_MPL_SEED_SET_ENTRY_LIFETIME;

#if CONFIG_NET_MCAST_MPL_CONTROL_MESSAGE_TIMER_EXPIRATION > 0
	domain_ptr->exp = 0;
	net_trickle_start(&domain_ptr->trickle, ctrl_message_expiration, domain_ptr);
#endif

#else
	pkt = net_pkt_alloc_with_buffer(&msg_ptr->iface, HBHO_BASE_LEN + msg_ptr->size,
				  AF_INET6, IPPROTO_IP,
				  PKT_WAIT_TIME);

	if(pkt == NULL) {
	}
	int ret = 0;
	ret = net_ipv6_create(pkt, &msg_ptr->src, &msg_ptr->seed->domain->data_addr);
	if (ret) {
		NET_ERR("DROP: wrong buffer %d", ret);
		return;
	}

	net_pkt_set_iface(pkt, &msg_ptr->iface);

	net_pkt_write_u8(pkt, IPPROTO_UDP);
	net_pkt_write_u8(pkt, 0);

	net_pkt_set_ipv6_next_hdr(pkt, NET_IPV6_NEXTHDR_HBHO);
	net_pkt_set_ipv6_ext_len(pkt, HBHO_TOTAL_LEN);

	net_pkt_write_u8(pkt, HBHO_OPT_TYPE_MPL);
	net_pkt_write_u8(pkt, 4);

	if(sys_slist_peek_next((sys_snode_t *)msg_ptr) == NULL) {
		mpl_flags |= 1<<2;
	}

	switch(msg_ptr->seed->seed_id.s) {
		case 0:
			mpl_flags |= 0x00;
			break;
		case 1:
			//placeholder for S=1 option
			break;
		case 2:
			//placeholder for S=2 option
			break;
		case 3:
			//placeholder for S=3 option
			break;
		default:
			NET_ERR("unknown S option");
			return;
	}

	net_pkt_write_u8(pkt, mpl_flags);
	net_pkt_write_u8(pkt, msg_ptr->seq);
	net_pkt_write_u8(pkt, 0x01);
	net_pkt_write_u8(pkt, 0x00);

	net_pkt_write(pkt, msg_ptr->data, 6);
	net_pkt_write_le16(pkt, 0x00);
	net_pkt_write(pkt, (msg_ptr->data)+8, (msg_ptr->size)-8);

	net_pkt_cursor_init(pkt);
	net_ipv6_finalize(pkt, IPPROTO_UDP);
	
	if (net_send_data(pkt) < 0) {
		NET_ERR("net_send_data failed");
		return;
	}

#if defined(CONFIG_NET_STATISTICS_MPL)
	net_stats_update_mpl_data_sent(net_pkt_iface(pkt));
#endif /* CONFIG_NET_STATISTICS_MPL */
#endif

	return 1;
}

void net_route_mpl_add_hdr(struct net_pkt *pkt, size_t *len)
{
	uint8_t mpl_flags = 0;

	net_pkt_write_u8(pkt, IPPROTO_UDP);
	net_pkt_write_u8(pkt, 0);

	net_pkt_set_ipv6_next_hdr(pkt, NET_IPV6_NEXTHDR_HBHO);
	net_pkt_set_ipv6_ext_len(pkt, HBHO_TOTAL_LEN);

	net_pkt_write_u8(pkt, HBHO_OPT_TYPE_MPL);
	net_pkt_write_u8(pkt, 4);
	mpl_flags |= 0x00; //S=0
	mpl_flags |= 1<<2; //M=1

	net_pkt_write_u8(pkt, mpl_flags);

	last_seq = SEQ_VAL_ADD(last_seq, 1);
	net_pkt_write_u8(pkt, last_seq);

	net_pkt_write_u8(pkt, 0x01);
	net_pkt_write_u8(pkt, 0x00);
}


void net_route_mpl_send_data(struct net_pkt *pkt)
{
	net_pkt_cursor_init(pkt);

	if(net_route_mpl_accept(pkt, 0)) {
#if defined(CONFIG_NET_STATISTICS_MPL)
		net_stats_update_mpl_data_sent(net_pkt_iface(pkt));
#endif /* CONFIG_NET_STATISTICS_MPL */
		net_send_data(pkt);
	}
}

#ifndef NET_MCAST_MPL_FLOODING
enum net_verdict icmpv6_handle_mpl_ctrl(struct net_pkt *pkt,
					    struct net_ipv6_hdr *ip_hdr,
					    struct net_icmp_hdr *icmp_hdr)
{
	struct mpl_domain *domain_ptr;
	struct mpl_msg *msg_ptr;
	struct mpl_seed *seed_ptr;
	static seed_id_t seed_id;

	static uint8_t r;
	static uint8_t *vector;
	static uint8_t vector_len;
	static uint8_t r_missing;
	static uint8_t l_missing;
	bool new_domain = false;

	struct seed_info info;

#if defined(CONFIG_NET_STATISTICS_MPL)
	net_stats_update_mpl_ctrl_recv(net_pkt_iface(pkt));
#endif /* CONFIG_NET_STATISTICS_MPL */

	domain_ptr = domain_set_lookup(&ip_hdr->dst);

	if(!domain_ptr) {
		domain_ptr = domain_set_allocate(&ip_hdr->dst, net_pkt_iface(pkt));
		if(!domain_ptr) {
			NET_ERR("Couldn't allocate new domain. Dropping.");
			return NET_DROP;
		}
		domain_ptr->exp = 0;
		net_trickle_start(&domain_ptr->trickle, ctrl_message_expiration, domain_ptr);
		new_domain = true;
	}
	l_missing = 0;
	r_missing = 0;

	net_pkt_cursor_init(pkt);
	net_pkt_skip(pkt, sizeof(struct net_ipv6_hdr));
	net_pkt_skip(pkt, sizeof(struct net_icmp_hdr));

	for(seed_ptr = &seed_set[CONFIG_NET_MCAST_MPL_SEED_SET_SIZE-1];
			seed_ptr >= seed_set; seed_ptr--) {

		if(SEED_SET_IS_USED(seed_ptr) && seed_ptr->domain == domain_ptr) {
			while(net_pkt_read(pkt, &info, sizeof(struct seed_info_s0)) == 0) {
				switch(SEED_INFO_GET_S(&info)) {
					case 0:
						net_pkt_skip(pkt, SEED_INFO_GET_LEN(&info)-sizeof(struct seed_info_s0));
						memcpy(seed_id.id, &ip_hdr->src, 16);
						seed_id.s = 0;

						if(net_ipv6_addr_cmp((struct in6_addr *)&seed_ptr->seed_id.id, &ip_hdr->src)) {
							goto seed_present;
						}
						break;
					case 1:

						break;
					case 2:

						break;
					case 3:
						net_pkt_read(pkt, seed_id.id, 16);
						seed_id.s = 3;
	          if(seed_id_cmp(&seed_id, &seed_ptr->seed_id)) {
	            goto seed_present;
	          }
						break;
				}
			}

			r_missing = 1;

			if(sys_slist_peek_head(&seed_ptr->msg_list) != NULL) {
				for(msg_ptr = (struct mpl_msg *)sys_slist_peek_head(&seed_ptr->msg_list); msg_ptr != NULL; msg_ptr = (struct mpl_msg *)sys_slist_peek_next((sys_snode_t*)msg_ptr)) {
					if(!net_trickle_is_running(&msg_ptr->trickle)) {
						net_trickle_start(&msg_ptr->trickle, data_message_expiration, msg_ptr);
					}

					msg_ptr->exp = 0;
					net_trickle_inconsistency(&msg_ptr->trickle);
				}
			}
			
		seed_present:
				continue;
		}
	}

	net_pkt_cursor_init(pkt);
	net_pkt_skip(pkt, sizeof(struct net_ipv6_hdr));
	net_pkt_skip(pkt, sizeof(struct net_icmp_hdr));

	while(net_pkt_read(pkt, &info, sizeof(struct seed_info_s0)) == 0) {
		seed_id.s = SEED_INFO_GET_S(&info);

		switch(SEED_INFO_GET_S(&info)) {
			case 0:
				memcpy(seed_id.id, &ip_hdr->src, 16);
				break;

			case 1:
				net_pkt_read(pkt, &seed_id.id, 2);
				break;

			case 2:
				net_pkt_read(pkt, &seed_id.id, 8);
				break;

			case 3:
				net_pkt_read(pkt, &seed_id.id, 16);
				break;
		}

		seed_ptr = seed_set_lookup((struct in6_addr *)seed_id.id, domain_ptr);
		if(!seed_ptr) {
			l_missing = 1;

			goto next;
		}

		vector_len = SEED_INFO_GET_LEN(&info) * 8;
		vector = net_pkt_cursor_get_pos(pkt);

		msg_ptr = (struct mpl_msg *)sys_slist_peek_head(&seed_ptr->msg_list);

		if(msg_ptr == NULL) {
			if(vector[0] > 0) {
				l_missing = 1;
			}
			goto next;
		}

		r = 0;

		if(msg_ptr->seq != info.min_seqno) {
			if(SEQ_VAL_IS_GT(&msg_ptr->seq, &info.min_seqno)) {
				while(msg_ptr->seq != SEQ_VAL_ADD(info.min_seqno, r) && r <= vector_len) {
					r++;
				}
			} else {
				while(msg_ptr != NULL && msg_ptr->seq != info.min_seqno) {
					msg_ptr = (struct mpl_msg *)sys_slist_peek_next((sys_snode_t*)msg_ptr);
				}
			}

			if(r > vector_len || msg_ptr == NULL) {
				msg_ptr = (struct mpl_msg *)sys_slist_peek_head(&seed_ptr->msg_list);

				while(sys_slist_peek_next((sys_snode_t*)msg_ptr) != NULL) {
					msg_ptr = (struct mpl_msg *)sys_slist_peek_next((sys_snode_t*)msg_ptr);
				}
				r = vector_len;

				while(!((vector[r/8]) & (0x01<<(r%8)))) {
					r--;
				}

				if(SEQ_VAL_IS_GT(msg_ptr->seq, SEQ_VAL_ADD(info.min_seqno, r))) {
					r_missing = 1;

					if(sys_slist_peek_head(&seed_ptr->msg_list) != NULL) {
						for(msg_ptr = (struct mpl_msg *)sys_slist_peek_head(&seed_ptr->msg_list); msg_ptr != NULL; msg_ptr = (struct mpl_msg *)sys_slist_peek_next((sys_snode_t*)msg_ptr)) {
							if(!net_trickle_is_running(&msg_ptr->trickle)) {
								net_trickle_start(&msg_ptr->trickle, data_message_expiration, msg_ptr);
							}
							net_trickle_inconsistency(&msg_ptr->trickle);
						}
					}
				} else {
					l_missing = 1;
				}
				goto next;
			}
		}

		do {
			while(msg_ptr->seq != SEQ_VAL_ADD(info.min_seqno, r)) {
				if((vector[r/8]) & (0x01<<(r%8))) {
					l_missing = 1;
				}
				r++;
			}

			if(!((vector[r/8]) & (0x01<<(r%8)))) {
				r_missing = 1;
				if(!net_trickle_is_running(&msg_ptr->trickle)) {
					net_trickle_start(&msg_ptr->trickle, data_message_expiration, msg_ptr);
				}
				net_trickle_inconsistency(&msg_ptr->trickle);
			}

			r++;
			msg_ptr = (struct mpl_msg *)sys_slist_peek_next((sys_snode_t*)msg_ptr);
		} while(msg_ptr != NULL && r <= vector_len);

		if(msg_ptr != NULL || r < vector_len) {
			while(r < vector_len) {
				if((vector[r/8]) & (0x01<<(r%8))) {
					l_missing = 1;
				}
				r++;
			}
		} else if(r >= vector_len && msg_ptr != NULL) {
			while(msg_ptr != NULL) {
				if(!net_trickle_is_running(&msg_ptr->trickle)) {
					net_trickle_start(&msg_ptr->trickle, data_message_expiration, msg_ptr);
				}
				msg_ptr->exp = 0;
				net_trickle_inconsistency(&msg_ptr->trickle);
				r_missing = 1;
				msg_ptr = (struct mpl_msg *)sys_slist_peek_next((sys_snode_t*)msg_ptr);
			}
		}

		net_pkt_skip(pkt, vector_len);
	}

next:
	
	if(l_missing && !net_trickle_is_running(&domain_ptr->trickle)) {
		domain_ptr->exp = 0;
		net_trickle_start(&domain_ptr->trickle, ctrl_message_expiration, domain_ptr);
	}
	
	if(l_missing || r_missing) {
#if defined(CONFIG_NET_STATISTICS_MPL)
		net_stats_update_mpl_ctrl_inconsistent(net_pkt_iface(pkt));
#endif /* CONFIG_NET_STATISTICS_MPL */
		if(net_trickle_is_running(&domain_ptr->trickle)) {
			net_trickle_inconsistency(&domain_ptr->trickle);
		}
	} else {
#if defined(CONFIG_NET_STATISTICS_MPL)
		net_stats_update_mpl_ctrl_consistent(net_pkt_iface(pkt));
#endif /* CONFIG_NET_STATISTICS_MPL */
		net_trickle_consistency(&domain_ptr->trickle);
	}

	return NET_DROP;
}
#endif
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//End of MPL Routing
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if !defined(NET_ROUTE_EXTRA_DATA_SIZE)
#define NET_ROUTE_EXTRA_DATA_SIZE 0
#endif

/* We keep track of the routes in a separate list so that we can remove
 * the oldest routes (at tail) if needed.
 */
static sys_slist_t routes;

/* Track currently active route lifetime timers */
static sys_slist_t active_route_lifetime_timers;

/* Timer that manages expired route entries. */
static struct k_work_delayable route_lifetime_timer;

static K_MUTEX_DEFINE(lock);

static void net_route_nexthop_remove(struct net_nbr *nbr)
{
	NET_DBG("Nexthop %p removed", nbr);
}

/*
 * This pool contains information next hop neighbors.
 */
NET_NBR_POOL_INIT(net_route_nexthop_pool,
		  CONFIG_NET_MAX_NEXTHOPS,
		  sizeof(struct net_route_nexthop),
		  net_route_nexthop_remove,
		  0);

static inline struct net_route_nexthop *net_nexthop_data(struct net_nbr *nbr)
{
	return (struct net_route_nexthop *)nbr->data;
}

static inline struct net_nbr *get_nexthop_nbr(struct net_nbr *start, int idx)
{
	NET_ASSERT(idx < CONFIG_NET_MAX_NEXTHOPS, "idx %d >= max %d",
		   idx, CONFIG_NET_MAX_NEXTHOPS);

	return (struct net_nbr *)((uint8_t *)start +
			((sizeof(struct net_nbr) + start->size) * idx));
}

static void release_nexthop_route(struct net_route_nexthop *route_nexthop)
{
	struct net_nbr *nbr = CONTAINER_OF(route_nexthop, struct net_nbr, __nbr);

	net_nbr_unref(nbr);
}

static struct net_nbr *get_nexthop_route(void)
{
	int i;

	for (i = 0; i < CONFIG_NET_MAX_NEXTHOPS; i++) {
		struct net_nbr *nbr = get_nexthop_nbr(
			(struct net_nbr *)net_route_nexthop_pool, i);

		if (!nbr->ref) {
			nbr->data = nbr->__nbr;

			nbr->idx = NET_NBR_LLADDR_UNKNOWN;

			return net_nbr_ref(nbr);
		}
	}

	return NULL;
}

static void net_route_entry_remove(struct net_nbr *nbr)
{
	NET_DBG("Route %p removed", nbr);
}

static void net_route_entries_table_clear(struct net_nbr_table *table)
{
	NET_DBG("Route table %p cleared", table);
}

/*
 * This pool contains routing table entries.
 */
NET_NBR_POOL_INIT(net_route_entries_pool,
		  CONFIG_NET_MAX_ROUTES,
		  sizeof(struct net_route_entry),
		  net_route_entry_remove,
		  NET_ROUTE_EXTRA_DATA_SIZE);

NET_NBR_TABLE_INIT(NET_NBR_LOCAL, nbr_routes, net_route_entries_pool,
		   net_route_entries_table_clear);

static inline struct net_nbr *get_nbr(int idx)
{
	return &net_route_entries_pool[idx].nbr;
}

static inline struct net_route_entry *net_route_data(struct net_nbr *nbr)
{
	return (struct net_route_entry *)nbr->data;
}

struct net_nbr *net_route_get_nbr(struct net_route_entry *route)
{
	int i;

	NET_ASSERT(route);

	k_mutex_lock(&lock, K_FOREVER);

	for (i = 0; i < CONFIG_NET_MAX_ROUTES; i++) {
		struct net_nbr *nbr = get_nbr(i);

		if (!nbr->ref) {
			continue;
		}

		if (nbr->data == (uint8_t *)route) {
			if (!nbr->ref) {
				k_mutex_unlock(&lock);
				return NULL;
			}

			k_mutex_unlock(&lock);
			return nbr;
		}
	}

	k_mutex_unlock(&lock);
	return NULL;
}

void net_routes_print(void)
{
	int i;

	k_mutex_lock(&lock, K_FOREVER);

	for (i = 0; i < CONFIG_NET_MAX_ROUTES; i++) {
		struct net_nbr *nbr = get_nbr(i);

		if (!nbr->ref) {
			continue;
		}

		NET_DBG("[%d] %p %d addr %s/%d",
			i, nbr, nbr->ref,
			log_strdup(net_sprint_ipv6_addr(
					   &net_route_data(nbr)->addr)),
			net_route_data(nbr)->prefix_len);
		NET_DBG("    iface %p idx %d ll %s",
			nbr->iface, nbr->idx,
			nbr->idx == NET_NBR_LLADDR_UNKNOWN ? "?" :
			log_strdup(net_sprint_ll_addr(
				net_nbr_get_lladdr(nbr->idx)->addr,
				net_nbr_get_lladdr(nbr->idx)->len)));
	}

	k_mutex_unlock(&lock);
}

static inline void nbr_free(struct net_nbr *nbr)
{
	NET_DBG("nbr %p", nbr);

	net_nbr_unref(nbr);
}

static struct net_nbr *nbr_new(struct net_if *iface,
			       struct in6_addr *addr,
			       uint8_t prefix_len)
{
	struct net_nbr *nbr = net_nbr_get(&net_nbr_routes.table);

	if (!nbr) {
		return NULL;
	}

	nbr->iface = iface;

	net_ipaddr_copy(&net_route_data(nbr)->addr, addr);
	net_route_data(nbr)->prefix_len = prefix_len;

	NET_DBG("[%d] nbr %p iface %p IPv6 %s/%d",
		nbr->idx, nbr, iface,
		log_strdup(net_sprint_ipv6_addr(&net_route_data(nbr)->addr)),
		prefix_len);

	return nbr;
}

static struct net_nbr *nbr_nexthop_get(struct net_if *iface,
				       struct in6_addr *addr)
{
	/* Note that the nexthop host must be already in the neighbor
	 * cache. We just increase the ref count of an existing entry.
	 */
	struct net_nbr *nbr;

	nbr = net_ipv6_nbr_lookup(iface, addr);
	if (nbr == NULL) {
		NET_DBG("Next hop neighbor not found!");
		return NULL;
	}

	NET_ASSERT(nbr->idx != NET_NBR_LLADDR_UNKNOWN,
		   "Nexthop %s not in neighbor cache!",
		   net_sprint_ipv6_addr(addr));

	net_nbr_ref(nbr);

	NET_DBG("[%d] nbr %p iface %p IPv6 %s",
		nbr->idx, nbr, iface,
		log_strdup(net_sprint_ipv6_addr(addr)));

	return nbr;
}

static int nbr_nexthop_put(struct net_nbr *nbr)
{
	NET_ASSERT(nbr);

	NET_DBG("[%d] nbr %p iface %p", nbr->idx, nbr, nbr->iface);

	net_nbr_unref(nbr);

	return 0;
}


#define net_route_info(str, route, dst)					\
	do {								\
	if (CONFIG_NET_ROUTE_LOG_LEVEL >= LOG_LEVEL_DBG) {		\
		struct in6_addr *naddr = net_route_get_nexthop(route);	\
									\
		NET_ASSERT(naddr, "Unknown nexthop address");	\
									\
		NET_DBG("%s route to %s via %s (iface %p)", str,	\
			log_strdup(net_sprint_ipv6_addr(dst)),		\
			log_strdup(net_sprint_ipv6_addr(naddr)),	\
			route->iface);					\
	} } while (0)

/* Route was accessed, so place it in front of the routes list */
static inline void update_route_access(struct net_route_entry *route)
{
	sys_slist_find_and_remove(&routes, &route->node);
	sys_slist_prepend(&routes, &route->node);
}

struct net_route_entry *net_route_lookup(struct net_if *iface,
					 struct in6_addr *dst)
{
	struct net_route_entry *route, *found = NULL;
	uint8_t longest_match = 0U;
	int i;

	k_mutex_lock(&lock, K_FOREVER);

	for (i = 0; i < CONFIG_NET_MAX_ROUTES && longest_match < 128; i++) {
		struct net_nbr *nbr = get_nbr(i);

		if (!nbr->ref) {
			continue;
		}

		if (iface && nbr->iface != iface) {
			continue;
		}

		route = net_route_data(nbr);

		if (route->prefix_len >= longest_match &&
		    net_ipv6_is_prefix(dst->s6_addr,
				       route->addr.s6_addr,
				       route->prefix_len)) {
			found = route;
			longest_match = route->prefix_len;
		}
	}

	if (found) {
		net_route_info("Found", found, dst);

		update_route_access(found);
	}

	k_mutex_unlock(&lock);
	return found;
}

static inline bool route_preference_is_lower(uint8_t old, uint8_t new)
{
	if (new == NET_ROUTE_PREFERENCE_RESERVED || (new & 0xfc) != 0) {
		return true;
	}

	/* Transform valid preference values into comparable integers */
	old = (old + 1) & 0x3;
	new = (new + 1) & 0x3;

	return new < old;
}

struct net_route_entry *net_route_add(struct net_if *iface,
				      struct in6_addr *addr,
				      uint8_t prefix_len,
				      struct in6_addr *nexthop,
				      uint32_t lifetime,
				      uint8_t preference)
{
	struct net_linkaddr_storage *nexthop_lladdr;
	struct net_nbr *nbr, *nbr_nexthop, *tmp;
	struct net_route_nexthop *nexthop_route;
	struct net_route_entry *route = NULL;
#if defined(CONFIG_NET_MGMT_EVENT_INFO)
       struct net_event_ipv6_route info;
#endif

	NET_ASSERT(addr);
	NET_ASSERT(iface);
	NET_ASSERT(nexthop);

	if (net_ipv6_addr_cmp(addr, net_ipv6_unspecified_address())) {
		NET_DBG("Route cannot be towards unspecified address");
		return NULL;
	}

	k_mutex_lock(&lock, K_FOREVER);

	nbr_nexthop = net_ipv6_nbr_lookup(iface, nexthop);
	if (!nbr_nexthop) {
		NET_DBG("No such neighbor %s found",
			log_strdup(net_sprint_ipv6_addr(nexthop)));
		goto exit;
	}

	nexthop_lladdr = net_nbr_get_lladdr(nbr_nexthop->idx);

	NET_ASSERT(nexthop_lladdr);

	NET_DBG("Nexthop %s lladdr is %s",
		log_strdup(net_sprint_ipv6_addr(nexthop)),
		log_strdup(net_sprint_ll_addr(nexthop_lladdr->addr,
					      nexthop_lladdr->len)));

	route = net_route_lookup(iface, addr);
	if (route) {
		/* Update nexthop if not the same */
		struct in6_addr *nexthop_addr;

		nexthop_addr = net_route_get_nexthop(route);
		if (nexthop_addr && net_ipv6_addr_cmp(nexthop, nexthop_addr)) {
			NET_DBG("No changes, return old route %p", route);

			/* Reset lifetime timer. */
			net_route_update_lifetime(route, lifetime);

			route->preference = preference;

			goto exit;
		}

		if (route_preference_is_lower(route->preference, preference)) {
			NET_DBG("No changes, ignoring route with lower preference");
			route = NULL;
			goto exit;
		}

		NET_DBG("Old route to %s found",
			log_strdup(net_sprint_ipv6_addr(nexthop_addr)));

		net_route_del(route);
	}

	nbr = nbr_new(iface, addr, prefix_len);
	if (!nbr) {
		/* Remove the oldest route and try again */
		sys_snode_t *last = sys_slist_peek_tail(&routes);

		sys_slist_find_and_remove(&routes, last);

		route = CONTAINER_OF(last,
				     struct net_route_entry,
				     node);

		if (CONFIG_NET_ROUTE_LOG_LEVEL >= LOG_LEVEL_DBG) {
			struct in6_addr *tmp;
			struct net_linkaddr_storage *llstorage;

			tmp = net_route_get_nexthop(route);
			nbr = net_ipv6_nbr_lookup(iface, tmp);
			if (nbr) {
				llstorage = net_nbr_get_lladdr(nbr->idx);

				NET_DBG("Removing the oldest route %s "
					"via %s [%s]",
					log_strdup(net_sprint_ipv6_addr(
							   &route->addr)),
					log_strdup(net_sprint_ipv6_addr(tmp)),
					log_strdup(net_sprint_ll_addr(
							   llstorage->addr,
							   llstorage->len)));
			}
		}

		net_route_del(route);

		nbr = nbr_new(iface, addr, prefix_len);
		if (!nbr) {
			NET_ERR("Neighbor route alloc failed!");
			route = NULL;
			goto exit;
		}
	}

	tmp = get_nexthop_route();
	if (!tmp) {
		NET_ERR("No nexthop route available!");
		route = NULL;
		goto exit;
	}

	nexthop_route = net_nexthop_data(tmp);

	route = net_route_data(nbr);
	route->iface = iface;
	route->preference = preference;

	net_route_update_lifetime(route, lifetime);

	sys_slist_prepend(&routes, &route->node);

	tmp = nbr_nexthop_get(iface, nexthop);

	NET_ASSERT(tmp == nbr_nexthop);

	nexthop_route->nbr = tmp;

	sys_slist_init(&route->nexthop);
	sys_slist_prepend(&route->nexthop, &nexthop_route->node);

	net_route_info("Added", route, addr);

#if defined(CONFIG_NET_MGMT_EVENT_INFO)
	net_ipaddr_copy(&info.addr, addr);
	net_ipaddr_copy(&info.nexthop, nexthop);
	info.prefix_len = prefix_len;

	net_mgmt_event_notify_with_info(NET_EVENT_IPV6_ROUTE_ADD,
					iface, (void *) &info,
					sizeof(struct net_event_ipv6_route));
#else
	net_mgmt_event_notify(NET_EVENT_IPV6_ROUTE_ADD, iface);
#endif

exit:
	k_mutex_unlock(&lock);
	return route;
}

static void route_expired(struct net_route_entry *route)
{
	NET_DBG("Route to %s expired",
		log_strdup(net_sprint_ipv6_addr(&route->addr)));

	sys_slist_find_and_remove(&active_route_lifetime_timers,
				  &route->lifetime.node);

	net_route_del(route);
}

static void route_lifetime_timeout(struct k_work *work)
{
	uint32_t next_update = UINT32_MAX;
	uint32_t current_time = k_uptime_get_32();
	struct net_route_entry *current, *next;

	ARG_UNUSED(work);

	k_mutex_lock(&lock, K_FOREVER);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&active_route_lifetime_timers,
					  current, next, lifetime.node) {
		struct net_timeout *timeout = &current->lifetime;
		uint32_t this_update = net_timeout_evaluate(timeout,
							    current_time);

		if (this_update == 0U) {
			route_expired(current);
			continue;
		}

		if (this_update < next_update) {
			next_update = this_update;
		}
	}

	if (next_update != UINT32_MAX) {
		k_work_reschedule(&route_lifetime_timer, K_MSEC(next_update));
	}

	k_mutex_unlock(&lock);
}

void net_route_update_lifetime(struct net_route_entry *route, uint32_t lifetime)
{
	NET_DBG("Updating route lifetime of %s to %u secs",
		log_strdup(net_sprint_ipv6_addr(&route->addr)),
		lifetime);

	if (!route) {
		return;
	}

	k_mutex_lock(&lock, K_FOREVER);

	if (lifetime == NET_IPV6_ND_INFINITE_LIFETIME) {
		route->is_infinite = true;

		(void)sys_slist_find_and_remove(&active_route_lifetime_timers,
						&route->lifetime.node);
	} else {
		route->is_infinite = false;

		net_timeout_set(&route->lifetime, lifetime, k_uptime_get_32());

		(void)sys_slist_find_and_remove(&active_route_lifetime_timers,
						&route->lifetime.node);
		sys_slist_append(&active_route_lifetime_timers,
				 &route->lifetime.node);
		k_work_reschedule(&route_lifetime_timer, K_NO_WAIT);
	}

	k_mutex_unlock(&lock);
}

int net_route_del(struct net_route_entry *route)
{
	struct net_nbr *nbr;
	struct net_route_nexthop *nexthop_route;
#if defined(CONFIG_NET_MGMT_EVENT_INFO)
       struct net_event_ipv6_route info;
#endif

	if (!route) {
		return -EINVAL;
	}

	k_mutex_lock(&lock, K_FOREVER);

#if defined(CONFIG_NET_MGMT_EVENT_INFO)
	net_ipaddr_copy(&info.addr, &route->addr);
	info.prefix_len = route->prefix_len;
	net_ipaddr_copy(&info.nexthop,
			net_route_get_nexthop(route));

	net_mgmt_event_notify_with_info(NET_EVENT_IPV6_ROUTE_DEL,
					route->iface, (void *) &info,
					sizeof(struct net_event_ipv6_route));
#else
	net_mgmt_event_notify(NET_EVENT_IPV6_ROUTE_DEL, route->iface);
#endif

	if (!route->is_infinite) {
		sys_slist_find_and_remove(&active_route_lifetime_timers,
					  &route->lifetime.node);

		if (sys_slist_is_empty(&active_route_lifetime_timers)) {
			k_work_cancel_delayable(&route_lifetime_timer);
		}
	}

	sys_slist_find_and_remove(&routes, &route->node);

	nbr = net_route_get_nbr(route);
	if (!nbr) {
		k_mutex_unlock(&lock);
		return -ENOENT;
	}

	net_route_info("Deleted", route, &route->addr);

	SYS_SLIST_FOR_EACH_CONTAINER(&route->nexthop, nexthop_route, node) {
		if (!nexthop_route->nbr) {
			continue;
		}

		nbr_nexthop_put(nexthop_route->nbr);
		release_nexthop_route(nexthop_route);
	}

	nbr_free(nbr);

	k_mutex_unlock(&lock);
	return 0;
}

int net_route_del_by_nexthop(struct net_if *iface, struct in6_addr *nexthop)
{
	int count = 0, status = 0;
	struct net_nbr *nbr_nexthop;
	struct net_route_nexthop *nexthop_route;
	int i, ret;

	NET_ASSERT(iface);
	NET_ASSERT(nexthop);

	k_mutex_lock(&lock, K_FOREVER);

	nbr_nexthop = net_ipv6_nbr_lookup(iface, nexthop);

	for (i = 0; i < CONFIG_NET_MAX_ROUTES; i++) {
		struct net_nbr *nbr = get_nbr(i);
		struct net_route_entry *route = net_route_data(nbr);

		if (!route) {
			continue;
		}

		SYS_SLIST_FOR_EACH_CONTAINER(&route->nexthop, nexthop_route,
					     node) {
			if (nexthop_route->nbr == nbr_nexthop) {
				/* This route contains this nexthop */
				ret = net_route_del(route);
				if (!ret) {
					count++;
				} else {
					status = ret;
				}
				break;
			}
		}
	}

	k_mutex_unlock(&lock);

	if (count) {
		return count;
	} else if (status < 0) {
		return status;
	}

	return 0;
}

int net_route_del_by_nexthop_data(struct net_if *iface,
				  struct in6_addr *nexthop,
				  void *data)
{
	int count = 0, status = 0;
	struct net_nbr *nbr_nexthop;
	struct net_route_nexthop *nexthop_route;
	int i, ret;

	NET_ASSERT(iface);
	NET_ASSERT(nexthop);

	k_mutex_lock(&lock, K_FOREVER);

	nbr_nexthop = net_ipv6_nbr_lookup(iface, nexthop);
	if (!nbr_nexthop) {
		k_mutex_unlock(&lock);
		return -EINVAL;
	}

	for (i = 0; i < CONFIG_NET_MAX_ROUTES; i++) {
		struct net_nbr *nbr = get_nbr(i);
		struct net_route_entry *route = net_route_data(nbr);

		SYS_SLIST_FOR_EACH_CONTAINER(&route->nexthop, nexthop_route,
					     node) {
			void *extra_data;

			if (nexthop_route->nbr != nbr_nexthop) {
				continue;
			}

			if (nbr->extra_data_size == 0U) {
				continue;
			}

			/* Routing engine specific extra data needs
			 * to match too.
			 */
			extra_data = net_nbr_extra_data(nbr_nexthop);
			if (extra_data != data) {
				continue;
			}

			ret = net_route_del(route);
			if (!ret) {
				count++;
			} else {
				status = ret;
			}

			break;
		}
	}

	k_mutex_unlock(&lock);

	if (count) {
		return count;
	}

	return status;
}

struct in6_addr *net_route_get_nexthop(struct net_route_entry *route)
{
	struct net_route_nexthop *nexthop_route;
	struct net_ipv6_nbr_data *ipv6_nbr_data;

	if (!route) {
		return NULL;
	}

	k_mutex_lock(&lock, K_FOREVER);

	SYS_SLIST_FOR_EACH_CONTAINER(&route->nexthop, nexthop_route, node) {
		struct in6_addr *addr;

		NET_ASSERT(nexthop_route->nbr->idx != NET_NBR_LLADDR_UNKNOWN);

		if (nexthop_route->nbr->idx == NET_NBR_LLADDR_UNKNOWN) {
			continue;
		}

		ipv6_nbr_data = net_ipv6_nbr_data(nexthop_route->nbr);
		if (ipv6_nbr_data) {
			addr = &ipv6_nbr_data->addr;
			NET_ASSERT(addr);

			k_mutex_unlock(&lock);
			return addr;
		} else {
			NET_ERR("could not get neighbor data from next hop");
		}
	}

	k_mutex_unlock(&lock);
	return NULL;
}

int net_route_foreach(net_route_cb_t cb, void *user_data)
{
	int i, ret = 0;

	k_mutex_lock(&lock, K_FOREVER);

	for (i = 0; i < CONFIG_NET_MAX_ROUTES; i++) {
		struct net_route_entry *route;
		struct net_nbr *nbr;

		nbr = get_nbr(i);
		if (!nbr) {
			continue;
		}

		if (!nbr->ref) {
			continue;
		}

		route = net_route_data(nbr);
		if (!route) {
			continue;
		}

		cb(route, user_data);

		ret++;
	}

	k_mutex_unlock(&lock);
	return ret;
}

#if defined(CONFIG_NET_ROUTE_MCAST)
/*
 * This array contains multicast routing entries.
 */
static
struct net_route_entry_mcast route_mcast_entries[CONFIG_NET_MAX_MCAST_ROUTES];

#if defined(CONFIG_NET_MCAST_MPL)

int net_route_mcast_forward_packet(struct net_pkt *pkt, const struct net_ipv6_hdr *hdr)
{
	if(hdr != NULL) {
		if(net_route_mpl_accept(pkt, 1) < 0) {
			NET_DBG("Packet dropped\n");
			return -1;
		} else {
			NET_DBG("Ours. Deliver to upper layers\n");
			return 1;
		}
	}
	else
	{
		net_route_mpl_send_data(pkt);

		return 1;
	}
}
#else
int net_route_mcast_forward_packet(struct net_pkt *pkt,
				   const struct net_ipv6_hdr *hdr)
{
	int i, ret = 0, err = 0;

	for (i = 0; i < CONFIG_NET_MAX_MCAST_ROUTES; ++i) {
		struct net_route_entry_mcast *route = &route_mcast_entries[i];
		struct net_pkt *pkt_cpy = NULL;

		if (!route->is_used) {
			continue;
		}

		if (!net_if_flag_is_set(route->iface,
					NET_IF_FORWARD_MULTICASTS) ||
		    !net_ipv6_is_prefix(hdr->dst,
					route->group.s6_addr,
					route->prefix_len)         ||
		    (pkt->iface == route->iface)) {
			continue;
		}

		pkt_cpy = net_pkt_shallow_clone(pkt, K_NO_WAIT);

		if (pkt_cpy == NULL) {
			err--;
			continue;
		}

		net_pkt_set_forwarding(pkt_cpy, true);
		net_pkt_set_iface(pkt_cpy, route->iface);

		if (net_send_data(pkt_cpy) >= 0) {
			++ret;
		} else {
			--err;
		}
	}

	return (err == 0) ? ret : err;
}

#endif

int net_route_mcast_foreach(net_route_mcast_cb_t cb,
			    struct in6_addr *skip,
			    void *user_data)
{
	int i, ret = 0;

	for (i = 0; i < CONFIG_NET_MAX_MCAST_ROUTES; i++) {
		struct net_route_entry_mcast *route = &route_mcast_entries[i];

		if (route->is_used) {
			if (skip && net_ipv6_is_prefix(skip->s6_addr,
						       route->group.s6_addr,
						       route->prefix_len)) {
				continue;
			}

			cb(route, user_data);

			ret++;
		}
	}

	return ret;
}

struct net_route_entry_mcast *net_route_mcast_add(struct net_if *iface,
						  struct in6_addr *group,
						  uint8_t prefix_len)
{
	int i;

	if ((!net_if_flag_is_set(iface, NET_IF_FORWARD_MULTICASTS)) ||
			(!net_ipv6_is_addr_mcast(group)) ||
			(net_ipv6_is_addr_mcast_iface(group)) ||
			(net_ipv6_is_addr_mcast_link(group))) {
		return NULL;
	}

	for (i = 0; i < CONFIG_NET_MAX_MCAST_ROUTES; i++) {
		struct net_route_entry_mcast *route = &route_mcast_entries[i];

		if (!route->is_used) {
			net_ipaddr_copy(&route->group, group);

			route->prefix_len = prefix_len;
			route->iface = iface;
			route->is_used = true;

			k_mutex_unlock(&lock);
			return route;
		}
	}

	return NULL;
}

bool net_route_mcast_del(struct net_route_entry_mcast *route)
{
	if (route > &route_mcast_entries[CONFIG_NET_MAX_MCAST_ROUTES - 1] ||
	    route < &route_mcast_entries[0]) {
		return false;
	}

	NET_ASSERT(route->is_used,
		   "Multicast route %p to %s was already removed", route,
		   log_strdup(net_sprint_ipv6_addr(&route->group)));

	route->is_used = false;

	return true;
}

struct net_route_entry_mcast *
net_route_mcast_lookup(struct in6_addr *group)
{
	int i;

	for (i = 0; i < CONFIG_NET_MAX_MCAST_ROUTES; i++) {
		struct net_route_entry_mcast *route = &route_mcast_entries[i];

		if (!route->is_used) {
			continue;
		}

		if (net_ipv6_is_prefix(group->s6_addr,
					route->group.s6_addr,
					route->prefix_len)) {
			return route;
		}
	}

	return NULL;
}
#endif /* CONFIG_NET_ROUTE_MCAST */

bool net_route_get_info(struct net_if *iface,
			struct in6_addr *dst,
			struct net_route_entry **route,
			struct in6_addr **nexthop)
{
	struct net_if_router *router;
	bool ret = false;

	k_mutex_lock(&lock, K_FOREVER);

	/* Search in neighbor table first, if not search in routing table. */
	if (net_ipv6_nbr_lookup(iface, dst)) {
		/* Found nexthop, no need to look into routing table. */
		*route = NULL;
		*nexthop = dst;

		ret = true;
		goto exit;
	}

	*route = net_route_lookup(iface, dst);
	if (*route) {
		*nexthop = net_route_get_nexthop(*route);
		if (!*nexthop) {
			goto exit;
		}

		ret = true;
		goto exit;
	} else {
		/* No specific route to this host, use the default
		 * route instead.
		 */
		router = net_if_ipv6_router_find_default(NULL, dst);
		if (!router) {
			goto exit;
		}

		*nexthop = &router->address.in6_addr;

		ret = true;
		goto exit;
	}

exit:
	k_mutex_unlock(&lock);
	return ret;
}

int net_route_packet(struct net_pkt *pkt, struct in6_addr *nexthop)
{
	struct net_linkaddr_storage *lladdr;
	struct net_nbr *nbr;
	int err;

	k_mutex_lock(&lock, K_FOREVER);

	nbr = net_ipv6_nbr_lookup(NULL, nexthop);
	if (!nbr) {
		NET_DBG("Cannot find %s neighbor",
			log_strdup(net_sprint_ipv6_addr(nexthop)));
		err = -ENOENT;
		goto error;
	}

	lladdr = net_nbr_get_lladdr(nbr->idx);
	if (!lladdr) {
		NET_DBG("Cannot find %s neighbor link layer address.",
			log_strdup(net_sprint_ipv6_addr(nexthop)));
		err = -ESRCH;
		goto error;
	}

#if defined(CONFIG_NET_L2_DUMMY)
	/* No need to do this check for dummy L2 as it does not have any
	 * link layer. This is done at runtime because we can have multiple
	 * network technologies enabled.
	 */
	if (net_if_l2(net_pkt_iface(pkt)) != &NET_L2_GET_NAME(DUMMY)) {
#endif
#if defined(CONFIG_NET_L2_PPP)
		/* PPP does not populate the lladdr fields */
		if (net_if_l2(net_pkt_iface(pkt)) != &NET_L2_GET_NAME(PPP)) {
#endif
			if (!net_pkt_lladdr_src(pkt)->addr) {
				NET_DBG("Link layer source address not set");
				err = -EINVAL;
				goto error;
			}

			/* Sanitycheck: If src and dst ll addresses are going
			 * to be same, then something went wrong in route
			 * lookup.
			 */
			if (!memcmp(net_pkt_lladdr_src(pkt)->addr, lladdr->addr,
				    lladdr->len)) {
				NET_ERR("Src ll and Dst ll are same");
				err = -EINVAL;
				goto error;
			}
#if defined(CONFIG_NET_L2_PPP)
		}
#endif
#if defined(CONFIG_NET_L2_DUMMY)
	}
#endif

	net_pkt_set_forwarding(pkt, true);

	/* Set the destination and source ll address in the packet.
	 * We set the destination address to be the nexthop recipient.
	 */
	net_pkt_lladdr_src(pkt)->addr = net_pkt_lladdr_if(pkt)->addr;
	net_pkt_lladdr_src(pkt)->type = net_pkt_lladdr_if(pkt)->type;
	net_pkt_lladdr_src(pkt)->len = net_pkt_lladdr_if(pkt)->len;

	net_pkt_lladdr_dst(pkt)->addr = lladdr->addr;
	net_pkt_lladdr_dst(pkt)->type = lladdr->type;
	net_pkt_lladdr_dst(pkt)->len = lladdr->len;

	net_pkt_set_iface(pkt, nbr->iface);

	k_mutex_unlock(&lock);
	return net_send_data(pkt);

error:
	k_mutex_unlock(&lock);
	return err;
}

int net_route_packet_if(struct net_pkt *pkt, struct net_if *iface)
{
	/* The destination is reachable via iface. But since no valid nexthop
	 * is known, net_pkt_lladdr_dst(pkt) cannot be set here.
	 */

	net_pkt_set_orig_iface(pkt, net_pkt_iface(pkt));
	net_pkt_set_iface(pkt, iface);

	net_pkt_set_forwarding(pkt, true);

	net_pkt_lladdr_src(pkt)->addr = net_pkt_lladdr_if(pkt)->addr;
	net_pkt_lladdr_src(pkt)->type = net_pkt_lladdr_if(pkt)->type;
	net_pkt_lladdr_src(pkt)->len = net_pkt_lladdr_if(pkt)->len;

	return net_send_data(pkt);
}

void net_route_init(void)
{
	NET_DBG("Allocated %d routing entries (%zu bytes)",
		CONFIG_NET_MAX_ROUTES, sizeof(net_route_entries_pool));

	NET_DBG("Allocated %d nexthop entries (%zu bytes)",
		CONFIG_NET_MAX_NEXTHOPS, sizeof(net_route_nexthop_pool));

	k_work_init_delayable(&route_lifetime_timer, route_lifetime_timeout);
}
