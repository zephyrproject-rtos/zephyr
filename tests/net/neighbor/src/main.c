/* main.c - Application main entry point */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_IPV6_NBR_CACHE_LOG_LEVEL);

#include <zephyr/ztest.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/sys/printk.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/linker/sections.h>

#include "nbr.h"

#define NET_LOG_ENABLED 1
#include "net_private.h"

static int remove_count, add_count, clear_called;

static void net_neighbor_data_remove(struct net_nbr *nbr)
{
	printk("Neighbor %p removed\n", nbr);

	remove_count++;
}

static void net_neighbor_table_clear(struct net_nbr_table *table)
{
	printk("Neighbor table %p cleared\n", table);

	clear_called = true;
}

NET_NBR_POOL_INIT(net_test_neighbor_pool, CONFIG_NET_IPV6_MAX_NEIGHBORS,
		  0, net_neighbor_data_remove);

NET_NBR_TABLE_INIT(NET_NBR_LOCAL, test_neighbor, net_test_neighbor_pool,
		   net_neighbor_table_clear);


static struct net_eth_addr hwaddr1 = { { 0x42, 0x11, 0x69, 0xde, 0xfa, 0x01 } };
static struct net_eth_addr hwaddr2 = { { 0x5f, 0x1c, 0x04, 0xae, 0x99, 0x02 } };
static struct net_eth_addr hwaddr3 = { { 0xee, 0xe1, 0x55, 0xfe, 0x44, 0x03 } };
static struct net_eth_addr hwaddr4 = { { 0x61, 0xf2, 0xfe, 0x4e, 0x8e, 0x04 } };
static struct net_eth_addr hwaddr5 = { { 0x8a, 0x52, 0x01, 0x21, 0x11, 0x05 } };

ZTEST(neighbor_test_suite, test_neighbor)
{
	struct net_eth_addr *addrs[] = {
		&hwaddr1,
		&hwaddr2,
		&hwaddr3,
		&hwaddr4,
		&hwaddr5,
		NULL
	};
	struct net_nbr *nbrs[sizeof(addrs) - 1] = { 0 };
	struct net_eth_addr *eth_addr;

	struct net_nbr *nbr;
	struct net_linkaddr lladdr;
	struct net_linkaddr *lladdr_ptr;
	struct net_if *iface1 = INT_TO_POINTER(1);
	struct net_if *iface2 = INT_TO_POINTER(2);
	int ret, i;

	zassert_true(CONFIG_NET_IPV6_MAX_NEIGHBORS == (ARRAY_SIZE(addrs) - 2),
		     "There should be exactly %d valid entries in addrs array\n",
		     CONFIG_NET_IPV6_MAX_NEIGHBORS + 1);

	/* First adding a neighbor and trying to add multiple hw addresses
	 * to it. Only the first one should succeed.
	 */
	nbr = net_nbr_get(&net_test_neighbor.table);
	zassert_not_null(nbr, "Cannot get neighbor from table %p\n",
			 &net_test_neighbor.table);

	zassert_false(nbr->ref != 1, "Invalid ref count %d\n", nbr->ref);

	lladdr.len = sizeof(struct net_eth_addr);

	for (i = 0; i < 2; i++) {
		eth_addr = addrs[i];

		memcpy(lladdr.addr, eth_addr->addr, sizeof(struct net_eth_addr));

		ret = net_nbr_link(nbr, iface1, &lladdr);

		/* One neighbor can have only one lladdr */
		zassert_false(i == 0 && ret < 0, "Cannot add %s to nbr cache (%d)\n",
			      net_sprint_ll_addr(lladdr.addr, lladdr.len),
			      ret);

		if (!ret) {
			printk("Adding %s\n",
			       net_sprint_ll_addr(eth_addr->addr,
						  sizeof(struct net_eth_addr)));
		}
	}

	memcpy(lladdr.addr, addrs[0]->addr, sizeof(struct net_eth_addr));

	nbr = net_nbr_lookup(&net_test_neighbor.table, iface1, &lladdr);
	zassert_true(nbr->idx == 0, "Wrong index %d should be %d\n", nbr->idx, 0);

	for (i = 0; i < 2; i++) {
		eth_addr = addrs[i];

		memcpy(lladdr.addr, eth_addr->addr, sizeof(struct net_eth_addr));

		ret = net_nbr_unlink(nbr, &lladdr);
		zassert_false(i == 0 && ret < 0, "Cannot add %s to nbr cache (%d)\n",
			      net_sprint_ll_addr(lladdr.addr, lladdr.len),
			      ret);
		if (!ret) {
			printk("Deleting %s\n",
			       net_sprint_ll_addr(eth_addr->addr,
						  sizeof(struct net_eth_addr)));
		}
	}

	net_nbr_unref(nbr);
	zassert_false(nbr->ref != 0, "nbr still referenced, ref %d\n",
		      nbr->ref);

	/* Then adding multiple neighbors.
	 */
	lladdr.len = sizeof(struct net_eth_addr);

	for (i = 0; i < ARRAY_SIZE(addrs); i++) {
		nbr = net_nbr_get(&net_test_neighbor.table);
		if (!nbr) {
			if (i >= CONFIG_NET_IPV6_MAX_NEIGHBORS) {
				/* Ok, last entry will not fit cache */
				break;
			}
			zassert_true(1, "[%d] Cannot get neighbor from table %p\n",
				     i, &net_test_neighbor.table);
		}

		zassert_true(nbr->ref == 1, "[%d] Invalid ref count %d\n", i, nbr->ref);
		nbrs[i] = nbr;

		eth_addr = addrs[i];

		memcpy(lladdr.addr, eth_addr->addr, sizeof(struct net_eth_addr));

		ret = net_nbr_link(nbr, iface1, &lladdr);
		zassert_false(ret < 0, "Cannot add %s to nbr cache (%d)\n",
			      net_sprint_ll_addr(lladdr.addr, lladdr.len),
			      ret);
		printk("Adding %s\n", net_sprint_ll_addr(eth_addr->addr,
							 sizeof(struct net_eth_addr)));
	}

	for (i = 0; i < ARRAY_SIZE(addrs) - 2; i++) {
		memcpy(lladdr.addr, addrs[i]->addr, sizeof(struct net_eth_addr));
		nbr = net_nbr_lookup(&net_test_neighbor.table, iface1, &lladdr);
		zassert_false(nbr->idx != i, "Wrong index %d should be %d\n", nbr->idx, i);
	}

	for (i = 0; i < ARRAY_SIZE(addrs); i++) {
		eth_addr = addrs[i];
		nbr = nbrs[i];
		memcpy(lladdr.addr, eth_addr->addr, sizeof(struct net_eth_addr));

		if (!nbr || i >= CONFIG_NET_IPV6_MAX_NEIGHBORS) {
			break;
		}

		ret = net_nbr_unlink(nbr, &lladdr);
		zassert_false(ret < 0, "Cannot del %s from nbr cache (%d)\n",
			      net_sprint_ll_addr(lladdr.addr, lladdr.len),
			      ret);
		printk("Deleting %s\n", net_sprint_ll_addr(eth_addr->addr,
							   sizeof(struct net_eth_addr)));

		net_nbr_unref(nbr);
		zassert_false(nbr->ref != 0, "nbr still referenced, ref %d\n",
			      nbr->ref);
	}

	/* Then adding multiple neighbors but some of them in different
	 * interface.
	 */
	lladdr.len = sizeof(struct net_eth_addr);
	remove_count = add_count = 0;

	for (i = 0; i < ARRAY_SIZE(addrs); i++) {
		nbr = net_nbr_get(&net_test_neighbor.table);
		if (!nbr) {
			if (i >= CONFIG_NET_IPV6_MAX_NEIGHBORS) {
				/* Ok, last entry will not fit cache */
				break;
			}
			zassert_true(1, "[%d] Cannot get neighbor from table %p\n",
				     i, &net_test_neighbor.table);
		}

		zassert_false(nbr->ref != 1, "[%d] Invalid ref count %d\n", i, nbr->ref);
		nbrs[i] = nbr;

		eth_addr = addrs[i];

		memcpy(lladdr.addr, eth_addr->addr, sizeof(struct net_eth_addr));

		if (i % 2) {
			ret = net_nbr_link(nbr, iface1, &lladdr);
		} else {
			ret = net_nbr_link(nbr, iface2, &lladdr);
		}
		zassert_false(ret < 0, "Cannot add %s to nbr cache (%d)\n",
			      net_sprint_ll_addr(lladdr.addr, lladdr.len),
			      ret);
		printk("Adding %s iface %p\n",
		       net_sprint_ll_addr(eth_addr->addr,
					  sizeof(struct net_eth_addr)),
		       nbr->iface);
		add_count++;
	}

	for (i = 0; i < ARRAY_SIZE(addrs) - 2; i++) {
		memcpy(lladdr.addr, addrs[i]->addr, sizeof(struct net_eth_addr));

		if (i % 2) {
			nbr = net_nbr_lookup(&net_test_neighbor.table, iface1,
					     &lladdr);
		} else {
			nbr = net_nbr_lookup(&net_test_neighbor.table, iface2,
					     &lladdr);
		}
		zassert_false(nbr->idx != i, "Wrong index %d should be %d\n", nbr->idx, i);

		lladdr_ptr = net_nbr_get_lladdr(i);
		if (memcmp(lladdr_ptr->addr, addrs[i]->addr,
			   sizeof(struct net_eth_addr))) {
			zassert_true(1, "Wrong lladdr %s in index %d\n",
				     net_sprint_ll_addr(lladdr_ptr->addr,
							lladdr_ptr->len),
				     i);
		}
	}

	for (i = 0; i < ARRAY_SIZE(addrs); i++) {
		struct net_if *iface;

		eth_addr = addrs[i];
		nbr = nbrs[i];
		memcpy(lladdr.addr, eth_addr->addr, sizeof(struct net_eth_addr));

		if (!nbr || i >= CONFIG_NET_IPV6_MAX_NEIGHBORS) {
			break;
		}

		iface = nbr->iface;

		ret = net_nbr_unlink(nbr, &lladdr);
		zassert_false(ret < 0, "Cannot del %s from nbr cache (%d)\n",
			      net_sprint_ll_addr(lladdr.addr, lladdr.len),
			      ret);

		printk("Deleting %s iface %p\n", net_sprint_ll_addr(eth_addr->addr,
								    sizeof(struct net_eth_addr)), iface);

		net_nbr_unref(nbr);
		zassert_false(nbr->ref != 0, "nbr still referenced, ref %d\n", nbr->ref);
	}

	zassert_false(add_count != remove_count, "Remove count %d does not match add count %d\n",
		      remove_count, add_count);

	net_nbr_clear_table(&net_test_neighbor.table);

	zassert_true(clear_called, "Table clear check failed");

	/* The table should be empty now */
	memcpy(lladdr.addr, addrs[0]->addr, sizeof(struct net_eth_addr));

	nbr = net_nbr_lookup(&net_test_neighbor.table, iface1, &lladdr);

	zassert_is_null(nbr, "Some entries still found in nbr cache");

	return;
}

void *setup(void)
{
	if (IS_ENABLED(CONFIG_NET_TC_THREAD_COOPERATIVE)) {
		k_thread_priority_set(k_current_get(),
				K_PRIO_COOP(CONFIG_NUM_COOP_PRIORITIES - 1));
	} else {
		k_thread_priority_set(k_current_get(), K_PRIO_PREEMPT(9));
	}
	return NULL;
}

/*test case main entry*/
ZTEST_SUITE(neighbor_test_suite, NULL, setup, NULL, NULL, NULL);
