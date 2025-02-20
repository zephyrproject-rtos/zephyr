/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_tc, CONFIG_NET_TC_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <string.h>

#include <zephyr/net/net_core.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_stats.h>

#include "net_private.h"
#include "net_stats.h"
#include "net_tc_mapping.h"

#define TC_RX_PSEUDO_QUEUE (COND_CODE_1(CONFIG_NET_TC_RX_SKIP_FOR_HIGH_PRIO, (1), (0)))
#define NET_TC_RX_EFFECTIVE_COUNT (NET_TC_RX_COUNT + TC_RX_PSEUDO_QUEUE)

#if NET_TC_RX_EFFECTIVE_COUNT > 1
#define NET_TC_RX_SLOTS (CONFIG_NET_PKT_RX_COUNT / NET_TC_RX_EFFECTIVE_COUNT)
BUILD_ASSERT(NET_TC_RX_SLOTS > 0,
		"Misconfiguration: There are more traffic classes then packets, "
		"either increase CONFIG_NET_PKT_RX_COUNT or decrease "
		"CONFIG_NET_TC_RX_COUNT or disable CONFIG_NET_TC_RX_SKIP_FOR_HIGH_PRIO");
#endif

#define TC_TX_PSEUDO_QUEUE (COND_CODE_1(CONFIG_NET_TC_TX_SKIP_FOR_HIGH_PRIO, (1), (0)))
#define NET_TC_TX_EFFECTIVE_COUNT (NET_TC_TX_COUNT + TC_TX_PSEUDO_QUEUE)

#if NET_TC_TX_EFFECTIVE_COUNT > 1
#define NET_TC_TX_SLOTS (CONFIG_NET_PKT_TX_COUNT / NET_TC_TX_EFFECTIVE_COUNT)
BUILD_ASSERT(NET_TC_TX_SLOTS > 0,
		"Misconfiguration: There are more traffic classes then packets, "
		"either increase CONFIG_NET_PKT_TX_COUNT or decrease "
		"CONFIG_NET_TC_TX_COUNT or disable CONFIG_NET_TC_TX_SKIP_FOR_HIGH_PRIO");
#endif

/* Template for thread name. The "xx" is either "TX" denoting transmit thread,
 * or "RX" denoting receive thread. The "q[y]" denotes the traffic class queue
 * where y indicates the traffic class id. The value of y can be from 0 to 7.
 */
#define MAX_NAME_LEN sizeof("xx_q[y]")

/* Stacks for TX work queue */
K_KERNEL_STACK_ARRAY_DEFINE(tx_stack, NET_TC_TX_COUNT,
			    CONFIG_NET_TX_STACK_SIZE);

/* Stacks for RX work queue */
K_KERNEL_STACK_ARRAY_DEFINE(rx_stack, NET_TC_RX_COUNT,
			    CONFIG_NET_RX_STACK_SIZE);

#if NET_TC_TX_COUNT > 0
static struct net_traffic_class tx_classes[NET_TC_TX_COUNT];
#endif

#if NET_TC_RX_COUNT > 0
static struct net_traffic_class rx_classes[NET_TC_RX_COUNT];
#endif

enum net_verdict net_tc_submit_to_tx_queue(uint8_t tc, struct net_pkt *pkt)
{
#if NET_TC_TX_COUNT > 0
	net_pkt_set_tx_stats_tick(pkt, k_cycle_get_32());

#if NET_TC_TX_EFFECTIVE_COUNT > 1
	if (k_sem_take(&tx_classes[tc].fifo_slot, K_NO_WAIT) != 0) {
		return NET_DROP;
	}
#endif

	k_fifo_put(&tx_classes[tc].fifo, pkt);
	return NET_OK;
#else
	ARG_UNUSED(tc);
	ARG_UNUSED(pkt);
	return NET_DROP;
#endif
}

enum net_verdict net_tc_submit_to_rx_queue(uint8_t tc, struct net_pkt *pkt)
{
#if NET_TC_RX_COUNT > 0
	net_pkt_set_rx_stats_tick(pkt, k_cycle_get_32());

#if NET_TC_RX_EFFECTIVE_COUNT > 1
	if (k_sem_take(&rx_classes[tc].fifo_slot, K_NO_WAIT) != 0) {
		return NET_DROP;
	}
#endif

	k_fifo_put(&rx_classes[tc].fifo, pkt);
	return NET_OK;
#else
	ARG_UNUSED(tc);
	ARG_UNUSED(pkt);
	return NET_DROP;
#endif
}

int net_tx_priority2tc(enum net_priority prio)
{
#if NET_TC_TX_COUNT > 0
	if (prio > NET_PRIORITY_NC) {
		/* Use default value suggested in 802.1Q */
		prio = NET_PRIORITY_BE;
	}

	return tx_prio2tc_map[prio];
#else
	ARG_UNUSED(prio);

	return 0;
#endif
}

int net_rx_priority2tc(enum net_priority prio)
{
#if NET_TC_RX_COUNT > 0
	if (prio > NET_PRIORITY_NC) {
		/* Use default value suggested in 802.1Q */
		prio = NET_PRIORITY_BE;
	}

	return rx_prio2tc_map[prio];
#else
	ARG_UNUSED(prio);

	return 0;
#endif
}

#if defined(CONFIG_NET_TC_THREAD_PRIO_CUSTOM)
#define BASE_PRIO_TX CONFIG_NET_TC_TX_THREAD_BASE_PRIO
#elif defined(CONFIG_NET_TC_THREAD_COOPERATIVE)
#define BASE_PRIO_TX (CONFIG_NET_TC_NUM_PRIORITIES - 1)
#else
#define BASE_PRIO_TX (CONFIG_NET_TC_TX_COUNT - 1)
#endif

#define PRIO_TX(i, _) (BASE_PRIO_TX - i)

#if defined(CONFIG_NET_TC_THREAD_PRIO_CUSTOM)
#define BASE_PRIO_RX CONFIG_NET_TC_RX_THREAD_BASE_PRIO
#elif defined(CONFIG_NET_TC_THREAD_COOPERATIVE)
#define BASE_PRIO_RX (CONFIG_NET_TC_NUM_PRIORITIES - 1)
#else
#define BASE_PRIO_RX (CONFIG_NET_TC_RX_COUNT - 1)
#endif

#define PRIO_RX(i, _) (BASE_PRIO_RX - i)

#if NET_TC_TX_COUNT > 0
/* Convert traffic class to thread priority */
static uint8_t tx_tc2thread(uint8_t tc)
{
	/* Initial implementation just maps the traffic class to certain queue.
	 * If there are less queues than classes, then map them into
	 * some specific queue.
	 *
	 * Lower value in this table means higher thread priority. The
	 * value is used as a parameter to K_PRIO_COOP() or K_PRIO_PREEMPT()
	 * which converts it to actual thread priority.
	 *
	 * Higher traffic class value means higher priority queue. This means
	 * that thread_priorities[7] value should contain the highest priority
	 * for the TX queue handling thread.
	 *
	 * For example, if NET_TC_TX_COUNT = 8, which is the maximum number of
	 * traffic classes, then this priority array will contain following
	 * values if preemptive priorities are used:
	 *      7, 6, 5, 4, 3, 2, 1, 0
	 * and
	 *      14, 13, 12, 11, 10, 9, 8, 7
	 * if cooperative priorities are used.
	 *
	 * Then these will be converted to following thread priorities if
	 * CONFIG_NET_TC_THREAD_COOPERATIVE is enabled:
	 *      -1, -2, -3, -4, -5, -6, -7, -8
	 *
	 * and if CONFIG_NET_TC_THREAD_PREEMPTIVE is enabled, following thread
	 * priorities are used:
	 *       7, 6, 5, 4, 3, 2, 1, 0
	 *
	 * This means that the lowest traffic class 1, will have the lowest
	 * cooperative priority -1 for coop priorities and 7 for preemptive
	 * priority.
	 */
	static const uint8_t thread_priorities[] = {
		LISTIFY(NET_TC_TX_COUNT, PRIO_TX, (,))
	};

	BUILD_ASSERT(NET_TC_TX_COUNT <= CONFIG_NUM_COOP_PRIORITIES,
		     "Too many traffic classes");

	NET_ASSERT(tc < ARRAY_SIZE(thread_priorities));

	return thread_priorities[tc];
}
#endif

#if NET_TC_RX_COUNT > 0
/* Convert traffic class to thread priority */
static uint8_t rx_tc2thread(uint8_t tc)
{
	static const uint8_t thread_priorities[] = {
		LISTIFY(NET_TC_RX_COUNT, PRIO_RX, (,))
	};

	BUILD_ASSERT(NET_TC_RX_COUNT <= CONFIG_NUM_COOP_PRIORITIES,
		     "Too many traffic classes");

	NET_ASSERT(tc < ARRAY_SIZE(thread_priorities));

	return thread_priorities[tc];
}
#endif

#if defined(CONFIG_NET_STATISTICS)
/* Fixup the traffic class statistics so that "net stats" shell command will
 * print output correctly.
 */
#if NET_TC_TX_COUNT > 0
static void tc_tx_stats_priority_setup(struct net_if *iface)
{
	int i;

	for (i = 0; i < 8; i++) {
		net_stats_update_tc_sent_priority(iface, net_tx_priority2tc(i),
						  i);
	}
}
#endif

#if NET_TC_RX_COUNT > 0
static void tc_rx_stats_priority_setup(struct net_if *iface)
{
	int i;

	for (i = 0; i < 8; i++) {
		net_stats_update_tc_recv_priority(iface, net_rx_priority2tc(i),
						  i);
	}
}
#endif

#if NET_TC_TX_COUNT > 0
static void net_tc_tx_stats_priority_setup(struct net_if *iface,
					   void *user_data)
{
	ARG_UNUSED(user_data);

	tc_tx_stats_priority_setup(iface);
}
#endif

#if NET_TC_RX_COUNT > 0
static void net_tc_rx_stats_priority_setup(struct net_if *iface,
					   void *user_data)
{
	ARG_UNUSED(user_data);

	tc_rx_stats_priority_setup(iface);
}
#endif
#endif

#if NET_TC_RX_COUNT > 0
static void tc_rx_handler(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p3);

	struct k_fifo *fifo = p1;
#if NET_TC_RX_EFFECTIVE_COUNT > 1
	struct k_sem *fifo_slot = p2;
#else
	ARG_UNUSED(p2);
#endif
	struct net_pkt *pkt;

	while (1) {
		pkt = k_fifo_get(fifo, K_FOREVER);
		if (pkt == NULL) {
			continue;
		}

#if NET_TC_RX_EFFECTIVE_COUNT > 1
		k_sem_give(fifo_slot);
#endif

		net_process_rx_packet(pkt);
	}
}
#endif

#if NET_TC_TX_COUNT > 0
static void tc_tx_handler(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p3);

	struct k_fifo *fifo = p1;
#if NET_TC_TX_EFFECTIVE_COUNT > 1
	struct k_sem *fifo_slot = p2;
#else
	ARG_UNUSED(p2);
#endif
	struct net_pkt *pkt;

	while (1) {
		pkt = k_fifo_get(fifo, K_FOREVER);
		if (pkt == NULL) {
			continue;
		}

#if NET_TC_TX_EFFECTIVE_COUNT > 1
		k_sem_give(fifo_slot);
#endif

		net_process_tx_packet(pkt);
	}
}
#endif

/* Create a fifo for each traffic class we are using. All the network
 * traffic goes through these classes.
 */
void net_tc_tx_init(void)
{
#if NET_TC_TX_COUNT == 0
	NET_DBG("No %s thread created", "TX");
	return;
#else
	int i;

	BUILD_ASSERT(NET_TC_TX_COUNT >= 0);

#if defined(CONFIG_NET_STATISTICS)
	net_if_foreach(net_tc_tx_stats_priority_setup, NULL);
#endif

	for (i = 0; i < NET_TC_TX_COUNT; i++) {
		uint8_t thread_priority;
		int priority;
		k_tid_t tid;

		thread_priority = tx_tc2thread(i);

		priority = IS_ENABLED(CONFIG_NET_TC_THREAD_COOPERATIVE) ?
			K_PRIO_COOP(thread_priority) :
			K_PRIO_PREEMPT(thread_priority);

		NET_DBG("[%d] Starting TX handler %p stack size %zd "
			"prio %d %s(%d)", i,
			&tx_classes[i].handler,
			K_KERNEL_STACK_SIZEOF(tx_stack[i]),
			thread_priority,
			IS_ENABLED(CONFIG_NET_TC_THREAD_COOPERATIVE) ?
							"coop" : "preempt",
			priority);

		k_fifo_init(&tx_classes[i].fifo);

#if NET_TC_TX_EFFECTIVE_COUNT > 1
		k_sem_init(&tx_classes[i].fifo_slot, NET_TC_TX_SLOTS, NET_TC_TX_SLOTS);
#endif

		tid = k_thread_create(&tx_classes[i].handler, tx_stack[i],
				      K_KERNEL_STACK_SIZEOF(tx_stack[i]),
				      tc_tx_handler,
				      &tx_classes[i].fifo,
#if NET_TC_TX_EFFECTIVE_COUNT > 1
				      &tx_classes[i].fifo_slot,
#else
				      NULL,
#endif
				      NULL,
				      priority, 0, K_FOREVER);
		if (!tid) {
			NET_ERR("Cannot create TC handler thread %d", i);
			continue;
		}

		if (IS_ENABLED(CONFIG_THREAD_NAME)) {
			char name[MAX_NAME_LEN];

			snprintk(name, sizeof(name), "tx_q[%d]", i);
			k_thread_name_set(tid, name);
		}

		k_thread_start(tid);
	}
#endif
}

void net_tc_rx_init(void)
{
#if NET_TC_RX_COUNT == 0
	NET_DBG("No %s thread created", "RX");
	return;
#else
	int i;

	BUILD_ASSERT(NET_TC_RX_COUNT >= 0);

#if defined(CONFIG_NET_STATISTICS)
	net_if_foreach(net_tc_rx_stats_priority_setup, NULL);
#endif

	for (i = 0; i < NET_TC_RX_COUNT; i++) {
		uint8_t thread_priority;
		int priority;
		k_tid_t tid;

		thread_priority = rx_tc2thread(i);

		priority = IS_ENABLED(CONFIG_NET_TC_THREAD_COOPERATIVE) ?
			K_PRIO_COOP(thread_priority) :
			K_PRIO_PREEMPT(thread_priority);

		NET_DBG("[%d] Starting RX handler %p stack size %zd "
			"prio %d %s(%d)", i,
			&rx_classes[i].handler,
			K_KERNEL_STACK_SIZEOF(rx_stack[i]),
			thread_priority,
			IS_ENABLED(CONFIG_NET_TC_THREAD_COOPERATIVE) ?
							"coop" : "preempt",
			priority);

		k_fifo_init(&rx_classes[i].fifo);

#if NET_TC_RX_EFFECTIVE_COUNT > 1
		k_sem_init(&rx_classes[i].fifo_slot, NET_TC_RX_SLOTS, NET_TC_RX_SLOTS);
#endif

		tid = k_thread_create(&rx_classes[i].handler, rx_stack[i],
				      K_KERNEL_STACK_SIZEOF(rx_stack[i]),
				      tc_rx_handler,
				      &rx_classes[i].fifo,
#if NET_TC_RX_EFFECTIVE_COUNT > 1
				      &rx_classes[i].fifo_slot,
#else
				      NULL,
#endif
				      NULL,
				      priority, 0, K_FOREVER);
		if (!tid) {
			NET_ERR("Cannot create TC handler thread %d", i);
			continue;
		}

		if (IS_ENABLED(CONFIG_THREAD_NAME)) {
			char name[MAX_NAME_LEN];

			snprintk(name, sizeof(name), "rx_q[%d]", i);
			k_thread_name_set(tid, name);
		}

		k_thread_start(tid);
	}
#endif
}
