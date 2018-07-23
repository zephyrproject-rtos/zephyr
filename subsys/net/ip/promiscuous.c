/** @file
 * @brief Promiscuous mode support
 *
 * Allow user to receive all network packets that are seen by a
 * network interface. This requires that network device driver
 * supports promiscuous mode.
 */

/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_PROMISC)
#define SYS_LOG_DOMAIN "net/promisc"
#define NET_LOG_ENABLED 1
#endif

#include <kernel.h>
#include <errno.h>

#include <net/net_if.h>
#include <net/net_core.h>
#include <net/net_pkt.h>

static K_FIFO_DEFINE(promiscuous_queue);
static atomic_t enabled = ATOMIC_INIT(0);

struct net_pkt *net_promisc_mode_wait_data(s32_t timeout)
{
	return k_fifo_get(&promiscuous_queue, timeout);
}

int net_promisc_mode_on(struct net_if *iface)
{
	int ret;

	if (!iface) {
		return -EINVAL;
	}

	ret = net_if_set_promisc(iface);
	if (!ret) {
		atomic_inc(&enabled);
	}

	return ret;
}

static void flush_queue(void)
{
	struct net_pkt *pkt;

	while (1) {
		pkt = k_fifo_get(&promiscuous_queue, K_NO_WAIT);
		if (!pkt) {
			return;
		}

		net_pkt_unref(pkt);
	}
}

int net_promisc_mode_off(struct net_if *iface)
{
	atomic_val_t prev;
	bool ret;

	ret = net_if_is_promisc(iface);
	if (ret) {
		net_if_unset_promisc(iface);

		prev = atomic_dec(&enabled);
		if (!prev) {
			atomic_clear(&enabled);

			flush_queue();
		}

		return 0;
	}

	return -EALREADY;
}

enum net_verdict net_promisc_mode_input(struct net_pkt *pkt)
{
	if (!pkt) {
		return NET_CONTINUE;
	}

	if (!atomic_get(&enabled)) {
		return NET_DROP;
	}

	k_fifo_put(&promiscuous_queue, pkt);

	return NET_OK;
}

