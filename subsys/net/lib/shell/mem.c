/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_shell);

#include "net_shell_private.h"

struct ctx_info {
	int pos;
	bool are_external_pools;
	struct k_mem_slab *tx_slabs[CONFIG_NET_MAX_CONTEXTS];
	struct net_buf_pool *data_pools[CONFIG_NET_MAX_CONTEXTS];
};

#if defined(CONFIG_NET_OFFLOAD) || defined(CONFIG_NET_NATIVE)
#if defined(CONFIG_NET_CONTEXT_NET_PKT_POOL)
static bool slab_pool_found_already(struct ctx_info *info,
				    struct k_mem_slab *slab,
				    struct net_buf_pool *pool)
{
	int i;

	for (i = 0; i < CONFIG_NET_MAX_CONTEXTS; i++) {
		if (slab) {
			if (info->tx_slabs[i] == slab) {
				return true;
			}
		} else {
			if (info->data_pools[i] == pool) {
				return true;
			}
		}
	}

	return false;
}
#endif /* CONFIG_NET_CONTEXT_NET_PKT_POOL */

static void context_info(struct net_context *context, void *user_data)
{
#if defined(CONFIG_NET_CONTEXT_NET_PKT_POOL)
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	struct ctx_info *info = data->user_data;
	struct k_mem_slab *slab;
	struct net_buf_pool *pool;

	if (!net_context_is_used(context)) {
		return;
	}

	if (context->tx_slab) {
		slab = context->tx_slab();

		if (slab_pool_found_already(info, slab, NULL)) {
			return;
		}

#if defined(CONFIG_NET_BUF_POOL_USAGE)
		PR("%p\t%u\t%u\tETX\n",
		   slab, slab->info.num_blocks, k_mem_slab_num_free_get(slab));
#else
		PR("%p\t%d\tETX\n", slab, slab->info.num_blocks);
#endif
		info->are_external_pools = true;
		info->tx_slabs[info->pos] = slab;
	}

	if (context->data_pool) {
		pool = context->data_pool();

		if (slab_pool_found_already(info, NULL, pool)) {
			return;
		}

#if defined(CONFIG_NET_BUF_POOL_USAGE)
		PR("%p\t%d\t%ld\tEDATA (%s)\n", pool, pool->buf_count,
		   atomic_get(&pool->avail_count), pool->name);
#else
		PR("%p\t%d\tEDATA\n", pool, pool->buf_count);
#endif
		info->are_external_pools = true;
		info->data_pools[info->pos] = pool;
	}

	info->pos++;
#endif /* CONFIG_NET_CONTEXT_NET_PKT_POOL */
}
#endif /* CONFIG_NET_OFFLOAD || CONFIG_NET_NATIVE */

static int cmd_net_mem(const struct shell *sh, size_t argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

#if defined(CONFIG_NET_OFFLOAD) || defined(CONFIG_NET_NATIVE)
	struct k_mem_slab *rx, *tx;
	struct net_buf_pool *rx_data, *tx_data;

	net_pkt_get_info(&rx, &tx, &rx_data, &tx_data);

#if defined(CONFIG_NET_BUF_FIXED_DATA_SIZE)
	PR("Fragment length %d bytes\n", CONFIG_NET_BUF_DATA_SIZE);
#else
	PR("Fragment RX data pool size %d bytes\n", CONFIG_NET_PKT_BUF_RX_DATA_POOL_SIZE);
	PR("Fragment TX data pool size %d bytes\n", CONFIG_NET_PKT_BUF_TX_DATA_POOL_SIZE);
#endif /* CONFIG_NET_BUF_FIXED_DATA_SIZE */

	PR("Network buffer pools:\n");

#if defined(CONFIG_NET_BUF_POOL_USAGE)
	PR("Address\t\tTotal\tAvail\tName\n");

	PR("%p\t%d\t%u\tRX\n",
	       rx, rx->info.num_blocks, k_mem_slab_num_free_get(rx));

	PR("%p\t%d\t%u\tTX\n",
	       tx, tx->info.num_blocks, k_mem_slab_num_free_get(tx));

	PR("%p\t%d\t%ld\tRX DATA (%s)\n", rx_data, rx_data->buf_count,
	   atomic_get(&rx_data->avail_count), rx_data->name);

	PR("%p\t%d\t%ld\tTX DATA (%s)\n", tx_data, tx_data->buf_count,
	   atomic_get(&tx_data->avail_count), tx_data->name);
#else
	PR("Address\t\tTotal\tName\n");

	PR("%p\t%d\tRX\n", rx, rx->info.num_blocks);
	PR("%p\t%d\tTX\n", tx, tx->info.num_blocks);
	PR("%p\t%d\tRX DATA\n", rx_data, rx_data->buf_count);
	PR("%p\t%d\tTX DATA\n", tx_data, tx_data->buf_count);
	PR_INFO("Set %s to enable %s support.\n",
		"CONFIG_NET_BUF_POOL_USAGE", "net_buf allocation");
#endif /* CONFIG_NET_BUF_POOL_USAGE */

	if (IS_ENABLED(CONFIG_NET_CONTEXT_NET_PKT_POOL)) {
		struct net_shell_user_data user_data;
		struct ctx_info info;

		(void)memset(&info, 0, sizeof(info));

		user_data.sh = sh;
		user_data.user_data = &info;

		net_context_foreach(context_info, &user_data);

		if (!info.are_external_pools) {
			PR("No external memory pools found.\n");
		}
	}

#if defined(CONFIG_NET_PKT_ALLOC_STATS)
	PR("\n");
	PR("Slab\t\tStatus\tAllocs\tAvg size\tAvg time (usec)\n");

	STRUCT_SECTION_FOREACH(net_pkt_alloc_stats_slab, stats) {
		if (stats->ok.count) {
			PR("%p\tOK  \t%u\t%llu\t\t%llu\n", stats->slab, stats->ok.count,
			   stats->ok.alloc_sum / (uint64_t)stats->ok.count,
			   k_cyc_to_us_ceil64(stats->ok.time_sum /
					      (uint64_t)stats->ok.count));
		}

		if (stats->fail.count) {
			PR("%p\tFAIL\t%u\t%llu\t\t%llu\n", stats->slab, stats->fail.count,
			   stats->fail.alloc_sum / (uint64_t)stats->fail.count,
			   k_cyc_to_us_ceil64(stats->fail.time_sum /
					      (uint64_t)stats->fail.count));
		}
	}
#endif /* CONFIG_NET_PKT_ALLOC_STATS */

#else
	PR_INFO("Set %s to enable %s support.\n",
		"CONFIG_NET_OFFLOAD or CONFIG_NET_NATIVE", "memory usage");
#endif /* CONFIG_NET_OFFLOAD || CONFIG_NET_NATIVE */

	return 0;
}

SHELL_SUBCMD_ADD((net), mem, NULL,
		 "Print information about network memory usage.",
		 cmd_net_mem, 1, 0);
