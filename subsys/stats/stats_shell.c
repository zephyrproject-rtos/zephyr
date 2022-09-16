/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/shell/shell.h>
#include <zephyr/stats/stats.h>

static int stats_cb(struct stats_hdr *hdr, void *arg, const char *name, uint16_t off)
{
	struct shell *sh = arg;
	void *addr = (uint8_t *)hdr + off;
	uint64_t val = 0;

	switch (hdr->s_size) {
	case sizeof(uint16_t):
		val = *(uint16_t *)(addr);
		break;

	case sizeof(uint32_t):
		val = *(uint32_t *)(addr);
		break;
	case sizeof(uint64_t):
		val = *(uint64_t *)(addr);
		break;
	}
	shell_print(sh, "\t%s (offset: %u, addr: %p): %" PRIu64, name, off, addr, val);
	return 0;
}

static int stats_group_cb(struct stats_hdr *hdr, void *arg)
{
	struct shell *sh = arg;

	shell_print(sh, "Stats Group %s (hdr addr: %p)", hdr->s_name, (void *)hdr);
	return stats_walk(hdr, stats_cb, arg);
}

static int cmd_stats_list(const struct shell *sh, size_t argc,
			  char **argv)
{
	return stats_group_walk(stats_group_cb, (struct shell *)sh);
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_stats,
			       SHELL_CMD(list, NULL, "List stats", cmd_stats_list),
			       SHELL_SUBCMD_SET_END /* Array terminated. */
			       );

SHELL_CMD_REGISTER(stats, &sub_stats, "Stats commands", NULL);
