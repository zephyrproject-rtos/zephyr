/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_OPENTHREAD_H_
#define ZEPHYR_INCLUDE_NET_OPENTHREAD_H_

#include <kernel.h>

#include <net/net_if.h>

#include <openthread/instance.h>

#ifdef __cplusplus
extern "C" {
#endif

struct pkt_list_elem {
	struct net_pkt *pkt;
};

struct openthread_context {
	otInstance *instance;
	struct net_if *iface;
	u16_t pkt_list_in_idx;
	u16_t pkt_list_out_idx;
	u8_t pkt_list_full;
	struct pkt_list_elem pkt_list[CONFIG_OPENTHREAD_PKT_LIST_SIZE];
};

k_tid_t openthread_thread_id_get(void);

#define OPENTHREAD_L2_CTX_TYPE struct openthread_context

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_OPENTHREAD_H_ */
