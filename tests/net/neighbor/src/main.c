/* main.c - Application main entry point */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <device.h>
#include <init.h>
#include <misc/printk.h>
#include <net/net_core.h>
#include <net/nbuf.h>
#include <net/net_ip.h>
#include <net/ethernet.h>
#include <sections.h>

#include <tc_util.h>

#include "nbr.h"

#define NET_DEBUG 1
#include "net_private.h"

static int remove_count, add_count, clear_called;

static void net_neighbor_data_remove(struct net_nbr *nbr)
{
	printk("Neighbor %p removed\n", nbr);

	remove_count++;

	return;
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

static bool run_tests(void)
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
	struct net_linkaddr_storage *lladdr_ptr;
	struct net_if *iface1 = INT_TO_POINTER(1);
	struct net_if *iface2 = INT_TO_POINTER(2);
	int ret, i;

	if (CONFIG_NET_IPV6_MAX_NEIGHBORS != (ARRAY_SIZE(addrs) - 2)) {
		printk("There should be exactly %d valid entries "
		       "in addrs array\n", CONFIG_NET_IPV6_MAX_NEIGHBORS + 1);
		return false;
	}

	/* First adding a neighbor and trying to add multiple hw addresses
	 * to it. Only the first one should succeed.
	 */
	nbr = net_nbr_get(&net_test_neighbor.table);
	if (!nbr) {
		printk("Cannot get neighbor from table %p\n",
		       &net_test_neighbor.table);
		return false;
	}

	if (nbr->ref != 1) {
		printk("Invalid ref count %d\n", nbr->ref);
		return false;
	}

	lladdr.len = sizeof(struct net_eth_addr);

	for (i = 0; i < 2; i++) {
		eth_addr = addrs[i];

		lladdr.addr = eth_addr->addr;

		ret = net_nbr_link(nbr, iface1, &lladdr);

		/* One neighbor can have only one lladdr */
		if (i == 0 && ret < 0) {
			printk("Cannot add %s to nbr cache (%d)\n",
			       net_sprint_ll_addr(lladdr.addr, lladdr.len),
			       ret);
			return false;
		} else if (!ret) {
			printk("Adding %s\n",
			       net_sprint_ll_addr(eth_addr->addr,
						sizeof(struct net_eth_addr)));
		}
	}

	lladdr.addr = addrs[0]->addr;
	nbr = net_nbr_lookup(&net_test_neighbor.table, iface1, &lladdr);
	if (nbr->idx != 0) {
		printk("Wrong index %d should be %d\n", nbr->idx, 0);
		return false;
	}

	for (i = 0; i < 2; i++) {
		eth_addr = addrs[i];

		lladdr.addr = eth_addr->addr;

		ret = net_nbr_unlink(nbr, &lladdr);
		if (i == 0 && ret < 0) {
			printk("Cannot del %s from nbr cache (%d)\n",
			       net_sprint_ll_addr(lladdr.addr, lladdr.len),
			       ret);
			return false;
		} else if (!ret) {
			printk("Deleting %s\n",
			       net_sprint_ll_addr(eth_addr->addr,
						sizeof(struct net_eth_addr)));
		}
	}

	net_nbr_unref(nbr);
	if (nbr->ref != 0) {
		printk("nbr still referenced, ref %d\n", nbr->ref);
		return false;
	}

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
			printk("[%d] Cannot get neighbor from table %p\n",
			       i, &net_test_neighbor.table);
			return false;
		}

		if (nbr->ref != 1) {
			printk("[%d] Invalid ref count %d\n", i, nbr->ref);
			return false;
		}
		nbrs[i] = nbr;

		eth_addr = addrs[i];

		lladdr.addr = eth_addr->addr;

		ret = net_nbr_link(nbr, iface1, &lladdr);
		if (ret < 0) {
			printk("Cannot add %s to nbr cache (%d)\n",
			       net_sprint_ll_addr(lladdr.addr, lladdr.len),
			       ret);
			return false;
		} else {
			printk("Adding %s\n",
			       net_sprint_ll_addr(eth_addr->addr,
						sizeof(struct net_eth_addr)));
		}
	}

	for (i = 0; i < ARRAY_SIZE(addrs) - 2; i++) {
		lladdr.addr = addrs[i]->addr;
		nbr = net_nbr_lookup(&net_test_neighbor.table, iface1, &lladdr);
		if (nbr->idx != i) {
			printk("Wrong index %d should be %d\n", nbr->idx, i);
			return false;
		}
	}

	for (i = 0; i < ARRAY_SIZE(addrs); i++) {
		eth_addr = addrs[i];
		nbr = nbrs[i];
		lladdr.addr = eth_addr->addr;

		if (!nbr || i >= CONFIG_NET_IPV6_MAX_NEIGHBORS) {
			break;
		}

		ret = net_nbr_unlink(nbr, &lladdr);
		if (ret < 0) {
			printk("Cannot del %s from nbr cache (%d)\n",
			       net_sprint_ll_addr(lladdr.addr, lladdr.len),
			       ret);
			return false;
		} else {
			printk("Deleting %s\n",
			       net_sprint_ll_addr(eth_addr->addr,
						sizeof(struct net_eth_addr)));
		}

		net_nbr_unref(nbr);
		if (nbr->ref != 0) {
			printk("nbr still referenced, ref %d\n", nbr->ref);
			return false;
		}
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
			printk("[%d] Cannot get neighbor from table %p\n",
			       i, &net_test_neighbor.table);
			return false;
		}

		if (nbr->ref != 1) {
			printk("[%d] Invalid ref count %d\n", i, nbr->ref);
			return false;
		}
		nbrs[i] = nbr;

		eth_addr = addrs[i];

		lladdr.addr = eth_addr->addr;

		if (i % 2) {
			ret = net_nbr_link(nbr, iface1, &lladdr);
		} else {
			ret = net_nbr_link(nbr, iface2, &lladdr);
		}
		if (ret < 0) {
			printk("Cannot add %s to nbr cache (%d)\n",
			       net_sprint_ll_addr(lladdr.addr, lladdr.len),
			       ret);
			return false;
		} else {
			printk("Adding %s iface %p\n",
			       net_sprint_ll_addr(eth_addr->addr,
						  sizeof(struct net_eth_addr)),
			       nbr->iface);
			add_count++;
		}
	}

	for (i = 0; i < ARRAY_SIZE(addrs) - 2; i++) {
		lladdr.addr = addrs[i]->addr;
		if (i % 2) {
			nbr = net_nbr_lookup(&net_test_neighbor.table, iface1,
					     &lladdr);
		} else {
			nbr = net_nbr_lookup(&net_test_neighbor.table, iface2,
					     &lladdr);
		}
		if (nbr->idx != i) {
			printk("Wrong index %d should be %d\n", nbr->idx, i);
			return false;
		}

		lladdr_ptr = net_nbr_get_lladdr(i);
		if (memcmp(lladdr_ptr->addr, addrs[i]->addr,
			   sizeof(struct net_eth_addr))) {
			printk("Wrong lladdr %s in index %d\n",
			       net_sprint_ll_addr(lladdr_ptr->addr,
						  lladdr_ptr->len),
			       i);
			return false;
		}
	}

	for (i = 0; i < ARRAY_SIZE(addrs); i++) {
		struct net_if *iface;

		eth_addr = addrs[i];
		nbr = nbrs[i];
		lladdr.addr = eth_addr->addr;

		if (!nbr || i >= CONFIG_NET_IPV6_MAX_NEIGHBORS) {
			break;
		}

		iface = nbr->iface;

		ret = net_nbr_unlink(nbr, &lladdr);
		if (ret < 0) {
			printk("Cannot del %s from nbr cache (%d)\n",
			       net_sprint_ll_addr(lladdr.addr, lladdr.len),
			       ret);
			return false;
		} else {
			printk("Deleting %s iface %p\n",
			       net_sprint_ll_addr(eth_addr->addr,
						  sizeof(struct net_eth_addr)),
			       iface);
		}

		net_nbr_unref(nbr);
		if (nbr->ref != 0) {
			printk("nbr still referenced, ref %d\n", nbr->ref);
			return false;
		}
	}

	if (add_count != remove_count) {
		printk("Remove count %d does not match add count %d\n",
		       remove_count, add_count);
		return false;
	}

	net_nbr_clear_table(&net_test_neighbor.table);

	if (!clear_called) {
		printk("Table clear check failed\n");
		return false;
	}

	/* The table should be empty now */
	lladdr.addr = addrs[0]->addr;
	nbr = net_nbr_lookup(&net_test_neighbor.table, iface1,
					     &lladdr);
	if (nbr) {
		printk("Some entries still found in nbr cache\n");
		return false;
	}

	printk("Neighbor cache checks passed\n");
	return true;
}


void main_fiber(void)
{
	if (run_tests()) {
		TC_END_REPORT(TC_PASS);
	} else {
		TC_END_REPORT(TC_FAIL);
	}
}

#if defined(CONFIG_NANOKERNEL)
#define STACKSIZE 2000
char __noinit __stack fiberStack[STACKSIZE];
#endif

void main(void)
{
#if defined(CONFIG_MICROKERNEL)
	main_fiber();
#else
	task_fiber_start(&fiberStack[0], STACKSIZE,
			(nano_fiber_entry_t)main_fiber, 0, 0, 7, 0);
#endif
}
