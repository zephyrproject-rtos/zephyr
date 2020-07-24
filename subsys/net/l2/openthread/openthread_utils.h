/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __OPENTHREAD_UTILS_H__
#define __OPENTHREAD_UTILS_H__

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_OPENTHREAD_L2_DEBUG_DUMP_15_4) || \
	defined(CONFIG_OPENTHREAD_L2_DEBUG_DUMP_IPV6)

void dump_pkt(const char *str, struct net_pkt *pkt);
#else
#define dump_pkt(...)
#endif

void add_ipv6_addr_to_zephyr(struct openthread_context *context);
void add_ipv6_addr_to_ot(struct openthread_context *context);
void add_ipv6_maddr_to_ot(struct openthread_context *context);
void add_ipv6_maddr_to_zephyr(struct openthread_context *context);
void rm_ipv6_addr_from_zephyr(struct openthread_context *context);
void rm_ipv6_maddr_from_zephyr(struct openthread_context *context);

int pkt_list_add(struct openthread_context *context, struct net_pkt *pkt);
struct net_pkt *pkt_list_peek(struct openthread_context *context);
void pkt_list_remove_last(struct openthread_context *context);
void pkt_list_remove_first(struct openthread_context *context);

static inline int pkt_list_is_full(struct openthread_context *context)
{
	return context->pkt_list_full;
}

#ifdef __cplusplus
}
#endif

#endif /* __OPENTHREAD_UTILS_H__ */
