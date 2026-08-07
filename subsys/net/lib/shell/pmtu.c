/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_shell);

#include "net_shell_private.h"

#include "pmtu.h"
#include "dplpmtud_internal.h"

#if !defined(CONFIG_NET_PMTU)
static void print_pmtu_error(const struct shell *sh)
{
	PR_INFO("Set %s to enable %s support.\n",
		"CONFIG_NET_IPV6_PMTU or CONFIG_NET_IPV4_PMTU", "PMTU");
	PR_INFO("Set %s to enable %s support.\n",
		"CONFIG_NET_IPV6_PMTU_DPLPMTUD or CONFIG_NET_IPV4_PMTU_DPLPMTUD",
		"DPLPMTUD");
}
#endif

#if defined(CONFIG_NET_IPV4) && defined(CONFIG_NET_IPV6)
/* Use the value of NET_IPV6_ADDR_LEN */
#define ADDR_STR_LEN 40
#elif defined(CONFIG_NET_IPV4)
#define ADDR_STR_LEN NET_INET_ADDRSTRLEN
#elif defined(CONFIG_NET_IPV6)
#define ADDR_STR_LEN 40
#else
#define ADDR_STR_LEN NET_INET_ADDRSTRLEN
#endif

#if defined(CONFIG_NET_PMTU)
static void pmtu_cb(struct net_pmtu_entry *entry, void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	int *count = data->user_data;

	if (!entry->in_use) {
		return;
	}

	if (*count == 0) {
		PR("     %" STRINGIFY(ADDR_STR_LEN) "s   MTU  Age (sec)\n",
		   "Destination Address");
	}

	PR("[%2d] %" STRINGIFY(ADDR_STR_LEN) "s %5d  %d\n", *count + 1,
	   net_sprint_addr(entry->dst.family, (void *)&entry->dst.in_addr),
	   entry->mtu,
	   (k_uptime_get_32() - entry->last_update) / 1000U);

	(*count)++;
}
#endif /* CONFIG_NET_PMTU */

#if defined(CONFIG_NET_PMTU_DPLPMTUD)
static void dplpmtud_cb(struct net_dplpmtud_entry *entry, void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	int *count = data->user_data;

	if (!entry->in_use) {
		return;
	}

	if (*count == 0) {
		PR("     %" STRINGIFY(ADDR_STR_LEN) "s   PLPMTU  Age (sec)\n",
		   "Destination Address");
	}

	PR("[%2d] %" STRINGIFY(ADDR_STR_LEN) "s %8u  %u\n", *count + 1,
	   net_sprint_addr(entry->dst.family, (void *)&entry->dst.in_addr),
	   (unsigned int)entry->validated_plpmtu,
	   (unsigned int)((k_uptime_get_32() - entry->last_update) / 1000U));

	(*count)++;
}
#endif /* CONFIG_NET_PMTU_DPLPMTUD */

static int cmd_net_pmtu(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_PMTU)
	struct net_shell_user_data user_data;
#endif

	ARG_UNUSED(argc);

#if defined(CONFIG_NET_PMTU)
	if (argv[1] == NULL) {
		/* PMTU destination cache content */
		int count = 0;

		user_data.sh = sh;
		user_data.user_data = &count;

		PR("%s destination cache content:\n", "PMTU");
		(void)net_pmtu_foreach(pmtu_cb, &user_data);
		if (count == 0) {
			PR("%s destination cache is empty.\n", "PMTU");
		}

#if defined(CONFIG_NET_PMTU_DPLPMTUD)
		PR("%s destination cache content:\n", "DPLPMTUD");
		count = 0;
		(void)net_dplpmtud_foreach(dplpmtud_cb, &user_data);
		if (count == 0) {
			PR("%s destination cache is empty.\n", "DPLPMTUD");
		}
#endif
	}
#else
	print_pmtu_error(sh);
#endif

	return 0;
}

static int cmd_net_pmtu_flush(const struct shell *sh, size_t argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

#if defined(CONFIG_NET_PMTU)
	PR("Flushing %s destination cache.\n", "PMTU");
	net_pmtu_init();

#if defined(CONFIG_NET_PMTU_DPLPMTUD)
	PR("Flushing %s destination cache.\n", "DPLPMTUD");
	net_dplpmtud_init();
#endif
#else
	print_pmtu_error(sh);
#endif

	return 0;
}

#if defined(CONFIG_NET_PMTU) && defined(CONFIG_NET_PMTU_DPLPMTUD)
#define PMTU_STR "PMTU/DPLPMTUD"
#elif defined(CONFIG_NET_PMTU)
#define PMTU_STR "PMTU"
#else
#define PMTU_STR "PMTU"
#endif /* CONFIG_NET_PMTU && CONFIG_NET_PMTU_DPLPMTUD */

SHELL_STATIC_SUBCMD_SET_CREATE(net_cmd_pmtu,
	SHELL_CMD(flush, NULL,
		  SHELL_HELP("Remove all entries from " PMTU_STR " destination cache", ""),
		  cmd_net_pmtu_flush),
	SHELL_SUBCMD_SET_END
);

SHELL_SUBCMD_ADD((net), pmtu, &net_cmd_pmtu,
		 "Show " PMTU_STR " information.",
		 cmd_net_pmtu, 1, 0);
