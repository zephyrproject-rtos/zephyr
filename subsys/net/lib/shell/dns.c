/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <strings.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_shell);

#include <zephyr/net/socket.h>
#include <zephyr/net/dns_resolve.h>
#include <zephyr/net/dns_sd.h>
#include "dns/dns_sd.h"

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
#define MAX_STR_LEN CONFIG_DNS_RESOLVER_MAX_NAME_LEN
		char str[MAX_STR_LEN + 1];

		switch (info->ai_family) {
		case AF_INET:
			net_addr_ntop(AF_INET,
				      &net_sin(&info->ai_addr)->sin_addr,
				      str, NET_IPV4_ADDR_LEN);
			break;

		case AF_INET6:
			net_addr_ntop(AF_INET6,
				      &net_sin6(&info->ai_addr)->sin6_addr,
				      str, NET_IPV6_ADDR_LEN);
			break;

		case AF_LOCAL:
			/* service discovery */
			memset(str, 0, MAX_STR_LEN);
			memcpy(str, info->ai_canonname,
			       MIN(info->ai_addrlen, MAX_STR_LEN));
			break;

		case AF_UNSPEC:
			if (info->ai_extension == DNS_RESOLVE_TXT) {
				memset(str, 0, MAX_STR_LEN);
				memcpy(str, info->ai_txt.text,
				       MIN(info->ai_txt.textlen, MAX_STR_LEN));
				break;
			} else if (info->ai_extension == DNS_RESOLVE_SRV) {
				snprintf(str, sizeof(str), "%d %d %d %.*s",
					 info->ai_srv.priority,
					 info->ai_srv.weight,
					 info->ai_srv.port,
					 (int)info->ai_srv.targetlen,
					 info->ai_srv.target);
				break;
			}

			/* fallthru */
		default:
			strncpy(str, "Invalid proto family", MAX_STR_LEN + 1);
			break;
		}

		str[MAX_STR_LEN] = '\0';

		PR("dns: %s\n", str);
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

	PR_WARNING("dns: Unhandled status %d received (errno %d)\n", status, errno);
}

static const char *printable_iface(const char *iface_name,
				   const char *found,
				   const char *not_found)
{
	if (iface_name[0] != '\0') {
		return found;
	}

	return not_found;
}

static void print_dns_info(const struct shell *sh,
			   struct dns_resolve_context *ctx)
{
	int i, ret;

	PR("DNS servers:\n");

	for (i = 0; i < CONFIG_DNS_RESOLVER_MAX_SERVERS +
		     DNS_MAX_MCAST_SERVERS; i++) {
		char iface_name[IFNAMSIZ] = { 0 };

		if (ctx->servers[i].if_index > 0) {
			ret = net_if_get_name(
				net_if_get_by_index(ctx->servers[i].if_index),
				iface_name, sizeof(iface_name));
			if (ret < 0) {
				snprintk(iface_name, sizeof(iface_name), "%d",
					 ctx->servers[i].if_index);
			}
		}

		if (ctx->servers[i].dns_server.sa_family == AF_INET) {
			PR("\t%s:%u%s%s%s%s%s\n",
			   net_sprint_ipv4_addr(
				   &net_sin(&ctx->servers[i].dns_server)->
				   sin_addr),
			   ntohs(net_sin(&ctx->servers[i].dns_server)->sin_port),
			   printable_iface(iface_name, " via ", ""),
			   printable_iface(iface_name, iface_name, ""),
			   ctx->servers[i].source != DNS_SOURCE_UNKNOWN ? " (" : "",
			   ctx->servers[i].source != DNS_SOURCE_UNKNOWN ?
					dns_get_source_str(ctx->servers[i].source) : "",
			   ctx->servers[i].source != DNS_SOURCE_UNKNOWN ? ")" : "");

		} else if (ctx->servers[i].dns_server.sa_family == AF_INET6) {
			PR("\t[%s]:%u%s%s%s%s%s\n",
			   net_sprint_ipv6_addr(
				   &net_sin6(&ctx->servers[i].dns_server)->
				   sin6_addr),
			   ntohs(net_sin6(&ctx->servers[i].dns_server)->sin6_port),
			   printable_iface(iface_name, " via ", ""),
			   printable_iface(iface_name, iface_name, ""),
			   ctx->servers[i].source != DNS_SOURCE_UNKNOWN ? " (" : "",
			   ctx->servers[i].source != DNS_SOURCE_UNKNOWN ?
					dns_get_source_str(ctx->servers[i].source) : "",
			   ctx->servers[i].source != DNS_SOURCE_UNKNOWN ? ")" : "");
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
			PR("\t%s[%u]: %s remaining %d\n", "IPv4",
			   ctx->queries[i].id,
			   ctx->queries[i].query,
			   remaining);
		} else if (ctx->queries[i].query_type == DNS_QUERY_TYPE_AAAA) {
			PR("\t%s[%u]: %s remaining %d\n", "IPv6",
			   ctx->queries[i].id,
			   ctx->queries[i].query,
			   remaining);
		} else if (ctx->queries[i].query_type == DNS_QUERY_TYPE_PTR) {
			PR("\t%s[%u]: %s remaining %d\n", "PTR",
			   ctx->queries[i].id,
			   ctx->queries[i].query,
			   remaining);
		} else {
			PR_WARNING("\tUnknown query type %d for query %s[%u] "
				   "remaining %d\n",
				   ctx->queries[i].query_type,
				   ctx->queries[i].query, ctx->queries[i].id,
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
	struct dns_resolve_context *ctx;
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
		if (strcasecmp(type, "A") == 0) {
			qtype = DNS_QUERY_TYPE_A;
			PR("IPv4 address query type\n");
		} else if (strcasecmp(type, "CNAME") == 0) {
			qtype = DNS_QUERY_TYPE_CNAME;
			PR("CNAME query type\n");
		} else if (strcasecmp(type, "PTR") == 0) {
			qtype = DNS_QUERY_TYPE_PTR;
			PR("Pointer query type\n");
		} else if (strcasecmp(type, "TXT") == 0) {
			qtype = DNS_QUERY_TYPE_TXT;
			PR("Text query type\n");
		} else if (strcasecmp(type, "AAAA") == 0) {
			qtype = DNS_QUERY_TYPE_AAAA;
			PR("IPv6 address query type\n");
		} else if (strcasecmp(type, "SRV") == 0) {
			qtype = DNS_QUERY_TYPE_SRV;
			PR("Service query type\n");
		} else {
			PR_WARNING("Unknown query type, specify either "
				   "A, CNAME, PTR, TXT, AAAA, or SRV\n");
			return -ENOEXEC;
		}
	}

	ctx = dns_resolve_get_default();
	if (!ctx) {
		PR_WARNING("No default DNS context found.\n");
		return -ENOEXEC;
	}

	ret = dns_resolve_name(ctx, host, qtype, NULL, dns_result_cb,
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

static int cmd_net_dns_list(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_DNS_SD)
#define MAX_PORT_LEN 6
	char buf[MAX_PORT_LEN];
	int n_records = 0;

	DNS_SD_FOREACH(record) {
		if (!dns_sd_rec_is_valid(record)) {
			continue;
		}

		if (n_records == 0) {
			PR("     DNS service records\n");
		}

		++n_records;

		if (record->port != NULL) {
			snprintk(buf, sizeof(buf), "%u", ntohs(*record->port));
		}

		PR("[%2d] %s.%s%s%s%s%s%s%s\n",
		   n_records,
		   record->instance != NULL ? record->instance : "",
		   record->service != NULL ? record->service : "",
		   record->proto != NULL ? "." : "",
		   record->proto != NULL ? record->proto : "",
		   record->domain != NULL ? "." : "",
		   record->domain != NULL ? record->domain : "",
		   record->port != NULL ? ":" : "",
		   record->port != NULL ? buf : "");
	}

	if (n_records == 0) {
		PR("No DNS service records found.\n");
		return 0;
	}
#else
	PR_INFO("DNS service discovery not supported. Set CONFIG_DNS_SD to "
		"enable it.\n");
#endif

	return 0;
}

static int cmd_net_dns_service(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_DNS_RESOLVER)
#define DNS_TIMEOUT (MSEC_PER_SEC * 2) /* ms */
	struct dns_resolve_context *ctx;
	char *service;
	uint16_t dns_id;
	int ret, arg = 1;

	service = argv[arg++];
	if (service == NULL) {
		PR_WARNING("Service not specified.\n");
		return -ENOEXEC;
	}

	ctx = dns_resolve_get_default();
	if (ctx == NULL) {
		PR_WARNING("No default DNS context found.\n");
		return -ENOEXEC;
	}

	ret = dns_resolve_service(ctx, service, &dns_id, dns_result_cb,
				(void *)sh, DNS_TIMEOUT);
	if (ret < 0) {
		PR_WARNING("Cannot resolve '%s' (%d)\n", service, ret);
	} else {
		PR("Query for '%s' sent.\n", service);
	}
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
	SHELL_CMD(list, NULL,
		  "List local DNS service records.",
		  cmd_net_dns_list),
	SHELL_CMD(service, NULL,
		  "'net dns service <service-description>\n"
		  "Execute DNS service discovery query.",
		  cmd_net_dns_service),
	SHELL_SUBCMD_SET_END
);

SHELL_SUBCMD_ADD((net), dns, &net_cmd_dns,
		 "Show how DNS is configured. Optionally do a query using a given name.",
		 cmd_net_dns, 1, 2);
