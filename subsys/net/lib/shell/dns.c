/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_shell);

#include <zephyr/net/dns_resolve.h>

#include "net_shell_private.h"

#if defined(CONFIG_DNS_RESOLVER)
static void dns_result_cb(enum dns_resolve_status status,
			  struct dns_addrinfo *info,
			  void *user_data)
{
	const struct shell *sh = user_data;

	if (status == DNS_EAI_CANCELED) {
		PR_WARNING("dns: Timeout while resolving name.\n");
		return;
	}

	if (status == DNS_EAI_INPROGRESS && info) {
		char addr[NET_IPV6_ADDR_LEN];

		if (info->ai_family == AF_INET) {
			net_addr_ntop(AF_INET,
				      &net_sin(&info->ai_addr)->sin_addr,
				      addr, NET_IPV4_ADDR_LEN);
		} else if (info->ai_family == AF_INET6) {
			net_addr_ntop(AF_INET6,
				      &net_sin6(&info->ai_addr)->sin6_addr,
				      addr, NET_IPV6_ADDR_LEN);
		} else {
			strncpy(addr, "Invalid protocol family",
				sizeof(addr));
			/* strncpy() doesn't guarantee NUL byte at the end. */
			addr[sizeof(addr) - 1] = 0;
		}

		PR("dns: %s\n", addr);
		return;
	}

	if (status == DNS_EAI_ALLDONE) {
		PR("dns: All results received\n");
		return;
	}

	if (status == DNS_EAI_FAIL) {
		PR_WARNING("dns: No such name found.\n");
		return;
	}

	PR_WARNING("dns: Unhandled status %d received\n", status);
}

static void print_dns_info(const struct shell *sh,
			   struct dns_resolve_context *ctx)
{
	int i;

	PR("DNS servers:\n");

	for (i = 0; i < CONFIG_DNS_RESOLVER_MAX_SERVERS +
		     DNS_MAX_MCAST_SERVERS; i++) {
		if (ctx->servers[i].dns_server.sa_family == AF_INET) {
			PR("\t%s:%u\n",
			   net_sprint_ipv4_addr(
				   &net_sin(&ctx->servers[i].dns_server)->
				   sin_addr),
			   ntohs(net_sin(
				 &ctx->servers[i].dns_server)->sin_port));
		} else if (ctx->servers[i].dns_server.sa_family == AF_INET6) {
			PR("\t[%s]:%u\n",
			   net_sprint_ipv6_addr(
				   &net_sin6(&ctx->servers[i].dns_server)->
				   sin6_addr),
			   ntohs(net_sin6(
				 &ctx->servers[i].dns_server)->sin6_port));
		}
	}

	PR("Pending queries:\n");

	for (i = 0; i < CONFIG_DNS_NUM_CONCUR_QUERIES; i++) {
		int32_t remaining;

		if (!ctx->queries[i].cb || !ctx->queries[i].query) {
			continue;
		}

		remaining = k_ticks_to_ms_ceil32(
			k_work_delayable_remaining_get(&ctx->queries[i].timer));

		if (ctx->queries[i].query_type == DNS_QUERY_TYPE_A) {
			PR("\tIPv4[%u]: %s remaining %d\n",
			   ctx->queries[i].id,
			   ctx->queries[i].query,
			   remaining);
		} else if (ctx->queries[i].query_type == DNS_QUERY_TYPE_AAAA) {
			PR("\tIPv6[%u]: %s remaining %d\n",
			   ctx->queries[i].id,
			   ctx->queries[i].query,
			   remaining);
		}
	}
}
#endif

static int cmd_net_dns_cancel(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_DNS_RESOLVER)
	struct dns_resolve_context *ctx;
	int ret, i;
#endif

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

#if defined(CONFIG_DNS_RESOLVER)
	ctx = dns_resolve_get_default();
	if (!ctx) {
		PR_WARNING("No default DNS context found.\n");
		return -ENOEXEC;
	}

	for (ret = 0, i = 0; i < CONFIG_DNS_NUM_CONCUR_QUERIES; i++) {
		if (!ctx->queries[i].cb) {
			continue;
		}

		if (!dns_resolve_cancel(ctx, ctx->queries[i].id)) {
			ret++;
		}
	}

	if (ret) {
		PR("Cancelled %d pending requests.\n", ret);
	} else {
		PR("No pending DNS requests.\n");
	}
#else
	PR_INFO("Set %s to enable %s support.\n", "CONFIG_DNS_RESOLVER",
		"DNS resolver");
#endif

	return 0;
}

static int cmd_net_dns_query(const struct shell *sh, size_t argc, char *argv[])
{

#if defined(CONFIG_DNS_RESOLVER)
#define DNS_TIMEOUT (MSEC_PER_SEC * 2) /* ms */
	enum dns_query_type qtype = DNS_QUERY_TYPE_A;
	char *host, *type = NULL;
	int ret, arg = 1;

	host = argv[arg++];
	if (!host) {
		PR_WARNING("Hostname not specified.\n");
		return -ENOEXEC;
	}

	if (argv[arg]) {
		type = argv[arg];
	}

	if (type) {
		if (strcmp(type, "A") == 0) {
			qtype = DNS_QUERY_TYPE_A;
			PR("IPv4 address type\n");
		} else if (strcmp(type, "AAAA") == 0) {
			qtype = DNS_QUERY_TYPE_AAAA;
			PR("IPv6 address type\n");
		} else {
			PR_WARNING("Unknown query type, specify either "
				   "A or AAAA\n");
			return -ENOEXEC;
		}
	}

	ret = dns_get_addr_info(host, qtype, NULL, dns_result_cb,
				(void *)sh, DNS_TIMEOUT);
	if (ret < 0) {
		PR_WARNING("Cannot resolve '%s' (%d)\n", host, ret);
	} else {
		PR("Query for '%s' sent.\n", host);
	}
#else
	PR_INFO("DNS resolver not supported. Set CONFIG_DNS_RESOLVER to "
		"enable it.\n");
#endif

	return 0;
}

static int cmd_net_dns(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_DNS_RESOLVER)
	struct dns_resolve_context *ctx;
#endif

#if defined(CONFIG_DNS_RESOLVER)
	if (argv[1]) {
		/* So this is a query then */
		cmd_net_dns_query(sh, argc, argv);
		return 0;
	}

	/* DNS status */
	ctx = dns_resolve_get_default();
	if (!ctx) {
		PR_WARNING("No default DNS context found.\n");
		return -ENOEXEC;
	}

	print_dns_info(sh, ctx);
#else
	PR_INFO("DNS resolver not supported. Set CONFIG_DNS_RESOLVER to "
		"enable it.\n");
#endif

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(net_cmd_dns,
	SHELL_CMD(cancel, NULL, "Cancel all pending requests.",
		  cmd_net_dns_cancel),
	SHELL_CMD(query, NULL,
		  "'net dns <hostname> [A or AAAA]' queries IPv4 address "
		  "(default) or IPv6 address for a host name.",
		  cmd_net_dns_query),
	SHELL_SUBCMD_SET_END
);

SHELL_SUBCMD_ADD((net), dns, &net_cmd_dns,
		 "Show how DNS is configured. Optionally do a query using a given name.",
		 cmd_net_dns, 1, 2);
