/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_shell);

#include "net_shell_private.h"

#if defined(CONFIG_NET_DEBUG_NET_PKT_ALLOC)
static void allocs_cb(struct net_pkt *pkt,
		      struct net_buf *buf,
		      const char *func_alloc,
		      int line_alloc,
		      const char *func_free,
		      int line_free,
		      bool in_use,
		      void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	const char *str;

	if (in_use) {
		str = "used";
	} else {
		if (func_alloc) {
			str = "free";
		} else {
			str = "avail";
		}
	}

	if (buf) {
		goto buf;
	}

	if (func_alloc) {
		if (in_use) {
			PR("%p/%ld\t%5s\t%5s\t%s():%d\n",
			   pkt, atomic_get(&pkt->atomic_ref), str,
			   net_pkt_slab2str(pkt->slab),
			   func_alloc, line_alloc);
		} else {
			PR("%p\t%5s\t%5s\t%s():%d -> %s():%d\n",
			   pkt, str, net_pkt_slab2str(pkt->slab),
			   func_alloc, line_alloc, func_free,
			   line_free);
		}
	}

	return;
buf:
	if (func_alloc) {
		struct net_buf_pool *pool = net_buf_pool_get(buf->pool_id);

		if (in_use) {
			PR("%p/%d\t%5s\t%5s\t%s():%d\n",
			   buf, buf->ref,
			   str, net_pkt_pool2str(pool), func_alloc,
			   line_alloc);
		} else {
			PR("%p\t%5s\t%5s\t%s():%d -> %s():%d\n",
			   buf, str, net_pkt_pool2str(pool),
			   func_alloc, line_alloc, func_free,
			   line_free);
		}
	}
}
#endif /* CONFIG_NET_DEBUG_NET_PKT_ALLOC */

static int cmd_net_allocs(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_DEBUG_NET_PKT_ALLOC)
	struct net_shell_user_data user_data;
#endif

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

#if defined(CONFIG_NET_DEBUG_NET_PKT_ALLOC)
	user_data.sh = sh;

	PR("Network memory allocations\n\n");
	PR("memory\t\tStatus\tPool\tFunction alloc -> freed\n");
	net_pkt_allocs_foreach(allocs_cb, &user_data);
#else
	PR_INFO("Set %s to enable %s support.\n",
		"CONFIG_NET_DEBUG_NET_PKT_ALLOC", "net_pkt allocation");
#endif /* CONFIG_NET_DEBUG_NET_PKT_ALLOC */

	return 0;
}

SHELL_SUBCMD_ADD((net), allocs, NULL,
		 "Print network memory allocations.",
		 cmd_net_allocs, 1, 0);
