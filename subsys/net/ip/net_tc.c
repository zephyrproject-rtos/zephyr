/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_tc, CONFIG_NET_TC_LOG_LEVEL);

#include <zephyr.h>
#include <string.h>

#include <net/net_core.h>
#include <net/net_pkt.h>
#include <net/net_stats.h>

#include "net_private.h"
#include "net_stats.h"
#include "net_tc_mapping.h"

/* Stacks for TX work queue */
K_KERNEL_STACK_ARRAY_DEFINE(tx_stack, NET_TC_TX_COUNT,
			    CONFIG_NET_TX_STACK_SIZE);

/* Stacks for RX work queue */
K_KERNEL_STACK_ARRAY_DEFINE(rx_stack, NET_TC_RX_COUNT,
			    CONFIG_NET_RX_STACK_SIZE);

static struct net_traffic_class tx_classes[NET_TC_TX_COUNT];
static struct net_traffic_class rx_classes[NET_TC_RX_COUNT];

bool net_tc_submit_to_tx_queue(uint8_t tc, struct net_pkt *pkt)
{
	if (k_work_pending(net_pkt_work(pkt))) {
		return false;
	}

	net_pkt_set_tx_stats_tick(pkt, k_cycle_get_32());

	k_work_submit_to_queue(&tx_classes[tc].work_q, net_pkt_work(pkt));

	return true;
}

void net_tc_submit_to_rx_queue(uint8_t tc, struct net_pkt *pkt)
{
	net_pkt_set_rx_stats_tick(pkt, k_cycle_get_32());

	k_work_submit_to_queue(&rx_classes[tc].work_q, net_pkt_work(pkt));
}

int net_tx_priority2tc(enum net_priority prio)
{
	if (prio > NET_PRIORITY_NC) {
		/* Use default value suggested in 802.1Q */
		prio = NET_PRIORITY_BE;
	}

	return tx_prio2tc_map[prio];
}

int net_rx_priority2tc(enum net_priority prio)
{
	if (prio > NET_PRIORITY_NC) {
		/* Use default value suggested in 802.1Q */
		prio = NET_PRIORITY_BE;
	}

	return rx_prio2tc_map[prio];
}

/* Convert traffic class to thread priority */
static uint8_t tx_tc2thread(uint8_t tc)
{
	/* Initial implementation just maps the traffic class to certain queue.
	 * If there are less queues than classes, then map them into
	 * some specific queue. In order to make this work same way as before,
	 * the thread priority 7 is used to map the default traffic class so
	 * this system works same way as before when TX thread default priority
	 * was 7.
	 *
	 * Lower value in this table means higher thread priority. The
	 * value is used as a parameter to K_PRIO_COOP() which converts it
	 * to actual thread priority.
	 *
	 * Higher traffic class value means higher priority queue. This means
	 * that thread_priorities[7] value should contain the highest priority
	 * for the TX queue handling thread.
	 */
	static const uint8_t thread_priorities[] = {
#if NET_TC_TX_COUNT == 1
		7
#endif
#if NET_TC_TX_COUNT == 2
		8, 7
#endif
#if NET_TC_TX_COUNT == 3
		8, 7, 6
#endif
#if NET_TC_TX_COUNT == 4
		8, 7, 6, 5
#endif
#if NET_TC_TX_COUNT == 5
		8, 7, 6, 5, 4
#endif
#if NET_TC_TX_COUNT == 6
		8, 7, 6, 5, 4, 3
#endif
#if NET_TC_TX_COUNT == 7
		8, 7, 6, 5, 4, 3, 2
#endif
#if NET_TC_TX_COUNT == 8
		8, 7, 6, 5, 4, 3, 2, 1
#endif
	};

	BUILD_ASSERT(NET_TC_TX_COUNT <= CONFIG_NUM_COOP_PRIORITIES,
		     "Too many traffic classes");

	NET_ASSERT(tc < ARRAY_SIZE(thread_priorities));

	return thread_priorities[tc];
}

/* Convert traffic class to thread priority */
static uint8_t rx_tc2thread(uint8_t tc)
{
	/* Initial implementation just maps the traffic class to certain queue.
	 * If there are less queues than classes, then map them into
	 * some specific queue. In order to make this work same way as before,
	 * the thread priority 7 is used to map the default traffic class so
	 * this system works same way as before when RX thread default priority
	 * was 7.
	 *
	 * Lower value in this table means higher thread priority. The
	 * value is used as a parameter to K_PRIO_COOP() which converts it
	 * to actual thread priority.
	 *
	 * Higher traffic class value means higher priority queue. This means
	 * that thread_priorities[7] value should contain the highest priority
	 * for the RX queue handling thread.
	 */
	static const uint8_t thread_priorities[] = {
#if NET_TC_RX_COUNT == 1
		7
#endif
#if NET_TC_RX_COUNT == 2
		8, 7
#endif
#if NET_TC_RX_COUNT == 3
		8, 7, 6
#endif
#if NET_TC_RX_COUNT == 4
		8, 7, 6, 5
#endif
#if NET_TC_RX_COUNT == 5
		8, 7, 6, 5, 4
#endif
#if NET_TC_RX_COUNT == 6
		8, 7, 6, 5, 4, 3
#endif
#if NET_TC_RX_COUNT == 7
		8, 7, 6, 5, 4, 3, 2
#endif
#if NET_TC_RX_COUNT == 8
		8, 7, 6, 5, 4, 3, 2, 1
#endif
	};

	BUILD_ASSERT(NET_TC_RX_COUNT <= CONFIG_NUM_COOP_PRIORITIES,
		     "Too many traffic classes");

	NET_ASSERT(tc < ARRAY_SIZE(thread_priorities));

	return thread_priorities[tc];
}

#if defined(CONFIG_NET_STATISTICS)
/* Fixup the traffic class statistics so that "net stats" shell command will
 * print output correctly.
 */
static void tc_tx_stats_priority_setup(struct net_if *iface)
{
	int i;

	for (i = 0; i < 8; i++) {
		net_stats_update_tc_sent_priority(iface, net_tx_priority2tc(i),
						  i);
	}
}

static void tc_rx_stats_priority_setup(struct net_if *iface)
{
	int i;

	for (i = 0; i < 8; i++) {
		net_stats_update_tc_recv_priority(iface, net_rx_priority2tc(i),
						  i);
	}
}

static void net_tc_tx_stats_priority_setup(struct net_if *iface,
					   void *user_data)
{
	ARG_UNUSED(user_data);

	tc_tx_stats_priority_setup(iface);
}

static void net_tc_rx_stats_priority_setup(struct net_if *iface,
					   void *user_data)
{
	ARG_UNUSED(user_data);

	tc_rx_stats_priority_setup(iface);
}
#endif

/* Create workqueue for each traffic class we are using. All the network
 * traffic goes through these classes. There needs to be at least one traffic
 * class in the system.
 */
void net_tc_tx_init(void)
{
	int i;

	BUILD_ASSERT(NET_TC_TX_COUNT > 0);

#if defined(CONFIG_NET_STATISTICS)
	net_if_foreach(net_tc_tx_stats_priority_setup, NULL);
#endif

	for (i = 0; i < NET_TC_TX_COUNT; i++) {
		uint8_t thread_priority;

		thread_priority = tx_tc2thread(i);
		tx_classes[i].tc = thread_priority;

		NET_DBG("[%d] Starting TX queue %p stack size %zd "
			"prio %d (%d)", i,
			&tx_classes[i].work_q.queue,
			K_KERNEL_STACK_SIZEOF(tx_stack[i]),
			thread_priority, K_PRIO_COOP(thread_priority));

		k_work_q_start(&tx_classes[i].work_q,
			       tx_stack[i],
			       K_KERNEL_STACK_SIZEOF(tx_stack[i]),
			       K_PRIO_COOP(thread_priority));
		k_thread_name_set(&tx_classes[i].work_q.thread, "tx_workq");
	}
}

void net_tc_rx_init(void)
{
	int i;

	BUILD_ASSERT(NET_TC_RX_COUNT > 0);

#if defined(CONFIG_NET_STATISTICS)
	net_if_foreach(net_tc_rx_stats_priority_setup, NULL);
#endif

	for (i = 0; i < NET_TC_RX_COUNT; i++) {
		uint8_t thread_priority;

		thread_priority = rx_tc2thread(i);
		rx_classes[i].tc = thread_priority;

		NET_DBG("[%d] Starting RX queue %p stack size %zd "
			"prio %d (%d)", i,
			&rx_classes[i].work_q.queue,
			K_KERNEL_STACK_SIZEOF(rx_stack[i]),
			thread_priority, K_PRIO_COOP(thread_priority));

		k_work_q_start(&rx_classes[i].work_q,
			       rx_stack[i],
			       K_KERNEL_STACK_SIZEOF(rx_stack[i]),
			       K_PRIO_COOP(thread_priority));
		k_thread_name_set(&rx_classes[i].work_q.thread, "rx_workq");
	}
}
