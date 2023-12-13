/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_shell);

#include "net_shell_private.h"

static bool is_pkt_part_of_slab(const struct k_mem_slab *slab, const char *ptr)
{
	size_t last_offset = (slab->info.num_blocks - 1) * slab->info.block_size;
	size_t ptr_offset;

	/* Check if pointer fits into slab buffer area. */
	if ((ptr < slab->buffer) || (ptr > slab->buffer + last_offset)) {
		return false;
	}

	/* Check if pointer offset is correct. */
	ptr_offset = ptr - slab->buffer;
	if (ptr_offset % slab->info.block_size != 0) {
		return false;
	}

	return true;
}

struct ctx_pkt_slab_info {
	const void *ptr;
	bool pkt_source_found;
};

static void check_context_pool(struct net_context *context, void *user_data)
{
#if defined(CONFIG_NET_CONTEXT_NET_PKT_POOL)
	if (!net_context_is_used(context)) {
		return;
	}

	if (context->tx_slab) {
		struct ctx_pkt_slab_info *info = user_data;
		struct k_mem_slab *slab = context->tx_slab();

		if (is_pkt_part_of_slab(slab, info->ptr)) {
			info->pkt_source_found = true;
		}
	}
#endif /* CONFIG_NET_CONTEXT_NET_PKT_POOL */
}

static bool is_pkt_ptr_valid(const void *ptr)
{
	struct k_mem_slab *rx, *tx;

	net_pkt_get_info(&rx, &tx, NULL, NULL);

	if (is_pkt_part_of_slab(rx, ptr) || is_pkt_part_of_slab(tx, ptr)) {
		return true;
	}

	if (IS_ENABLED(CONFIG_NET_CONTEXT_NET_PKT_POOL)) {
		struct ctx_pkt_slab_info info;

		info.ptr = ptr;
		info.pkt_source_found = false;

		net_context_foreach(check_context_pool, &info);

		if (info.pkt_source_found) {
			return true;
		}
	}

	return false;
}

static struct net_pkt *get_net_pkt(const char *ptr_str)
{
	uint8_t buf[sizeof(intptr_t)];
	intptr_t ptr = 0;
	size_t len;
	int i;

	if (ptr_str[0] == '0' && ptr_str[1] == 'x') {
		ptr_str += 2;
	}

	len = hex2bin(ptr_str, strlen(ptr_str), buf, sizeof(buf));
	if (!len) {
		return NULL;
	}

	for (i = len - 1; i >= 0; i--) {
		ptr |= buf[i] << 8 * (len - 1 - i);
	}

	return (struct net_pkt *)ptr;
}

static void net_pkt_buffer_info(const struct shell *sh, struct net_pkt *pkt)
{
	struct net_buf *buf = pkt->buffer;

	PR("net_pkt %p buffer chain:\n", pkt);
	PR("%p[%ld]", pkt, atomic_get(&pkt->atomic_ref));

	if (buf) {
		PR("->");
	}

	while (buf) {
		PR("%p[%ld/%u (%u/%u)]", buf, atomic_get(&pkt->atomic_ref),
		   buf->len, net_buf_max_len(buf), buf->size);

		buf = buf->frags;
		if (buf) {
			PR("->");
		}
	}

	PR("\n");
}

static void net_pkt_buffer_hexdump(const struct shell *sh,
				   struct net_pkt *pkt)
{
	struct net_buf *buf = pkt->buffer;
	int i = 0;

	if (!buf || buf->ref == 0) {
		return;
	}

	PR("net_pkt %p buffer chain hexdump:\n", pkt);

	while (buf) {
		PR("net_buf[%d] %p\n", i++, buf);
		shell_hexdump(sh, buf->data, buf->len);
		buf = buf->frags;
	}
}

static int cmd_net_pkt(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_pkt *pkt;

	pkt = get_net_pkt(argv[1]);
	if (!pkt) {
		PR_ERROR("Invalid ptr value (%s). "
			 "Example: 0x01020304\n", argv[1]);

		return -ENOEXEC;
	}

	if (!is_pkt_ptr_valid(pkt)) {
		PR_ERROR("Pointer is not recognized as net_pkt (%s).\n",
			 argv[1]);

		return -ENOEXEC;
	}

	net_pkt_buffer_info(sh, pkt);
	PR("\n");
	net_pkt_buffer_hexdump(sh, pkt);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(net_cmd_pkt,
	SHELL_CMD(--help, NULL,
		  "'net pkt <ptr in hex>' "
		  "Print information about given net_pkt",
		  cmd_net_pkt),
	SHELL_SUBCMD_SET_END
);

SHELL_SUBCMD_ADD((net), pkt, &net_cmd_pkt,
		 "net_pkt information.",
		 cmd_net_pkt, 2, 0);
