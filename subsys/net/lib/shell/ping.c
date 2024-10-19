/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_shell);

#include <stdlib.h>
#include <stdio.h>
#include <zephyr/random/random.h>
#include <zephyr/net/icmp.h>

#include "net_shell_private.h"

#include "../ip/icmpv6.h"
#include "../ip/icmpv4.h"
#include "../ip/route.h"

#if defined(CONFIG_NET_IP)

static struct ping_context {
	struct k_work_delayable work;
	struct net_icmp_ctx icmp;
	union {
		struct sockaddr_in addr4;
		struct sockaddr_in6 addr6;
		struct sockaddr addr;
	};
	struct net_if *iface;
	const struct shell *sh;

	/* Ping parameters */
	uint32_t count;
	uint32_t interval;
	uint32_t sequence;
	uint16_t payload_size;
	uint8_t tos;
	int priority;
} ping_ctx;

static void ping_done(struct ping_context *ctx);

#if defined(CONFIG_NET_NATIVE_IPV6)

static int handle_ipv6_echo_reply(struct net_icmp_ctx *ctx,
				  struct net_pkt *pkt,
				  struct net_icmp_ip_hdr *hdr,
				  struct net_icmp_hdr *icmp_hdr,
				  void *user_data)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(icmp_access,
					      struct net_icmpv6_echo_req);
	struct net_ipv6_hdr *ip_hdr = hdr->ipv6;
	struct net_icmpv6_echo_req *icmp_echo;
	uint32_t cycles;
	char time_buf[16] = { 0 };

	icmp_echo = (struct net_icmpv6_echo_req *)net_pkt_get_data(pkt,
								&icmp_access);
	if (icmp_echo == NULL) {
		return -EIO;
	}

	net_pkt_skip(pkt, sizeof(*icmp_echo));

	if (net_pkt_remaining_data(pkt) >= sizeof(uint32_t)) {
		if (net_pkt_read_be32(pkt, &cycles)) {
			return -EIO;
		}

		cycles = k_cycle_get_32() - cycles;

		snprintf(time_buf, sizeof(time_buf),
#ifdef CONFIG_FPU
			 "time=%.2f ms",
			 (double)((uint32_t)k_cyc_to_ns_floor64(cycles) / 1000000.f)
#else
			 "time=%d ms",
			 ((uint32_t)k_cyc_to_ns_floor64(cycles) / 1000000)
#endif
			);
	}

	PR_SHELL(ping_ctx.sh, "%d bytes from %s to %s: icmp_seq=%d ttl=%d "
#ifdef CONFIG_IEEE802154
		 "rssi=%d "
#endif
		 "%s\n",
		 ntohs(ip_hdr->len) - net_pkt_ipv6_ext_len(pkt) -
								NET_ICMPH_LEN,
		 net_sprint_ipv6_addr(&ip_hdr->src),
		 net_sprint_ipv6_addr(&ip_hdr->dst),
		 ntohs(icmp_echo->sequence),
		 ip_hdr->hop_limit,
#ifdef CONFIG_IEEE802154
		 net_pkt_ieee802154_rssi_dbm(pkt),
#endif
		 time_buf);

	if (ntohs(icmp_echo->sequence) == ping_ctx.count) {
		ping_done(&ping_ctx);
	}

	return 0;
}
#else
static int handle_ipv6_echo_reply(struct net_icmp_ctx *ctx,
				  struct net_pkt *pkt,
				  struct net_icmp_ip_hdr *hdr,
				  struct net_icmp_hdr *icmp_hdr,
				  void *user_data)
{
	ARG_UNUSED(ctx);
	ARG_UNUSED(pkt);
	ARG_UNUSED(hdr);
	ARG_UNUSED(icmp_hdr);
	ARG_UNUSED(user_data);

	return -ENOTSUP;
}
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_NATIVE_IPV4)

static int handle_ipv4_echo_reply(struct net_icmp_ctx *ctx,
				  struct net_pkt *pkt,
				  struct net_icmp_ip_hdr *hdr,
				  struct net_icmp_hdr *icmp_hdr,
				  void *user_data)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(icmp_access,
					      struct net_icmpv4_echo_req);
	struct net_ipv4_hdr *ip_hdr = hdr->ipv4;
	uint32_t cycles;
	struct net_icmpv4_echo_req *icmp_echo;
	char time_buf[16] = { 0 };

	icmp_echo = (struct net_icmpv4_echo_req *)net_pkt_get_data(pkt,
								&icmp_access);
	if (icmp_echo == NULL) {
		return -EIO;
	}

	net_pkt_skip(pkt, sizeof(*icmp_echo));

	if (net_pkt_remaining_data(pkt) >= sizeof(uint32_t)) {
		if (net_pkt_read_be32(pkt, &cycles)) {
			return -EIO;
		}

		cycles = k_cycle_get_32() - cycles;

		snprintf(time_buf, sizeof(time_buf),
#ifdef CONFIG_FPU
			 "time=%.2f ms",
			 (double)((uint32_t)k_cyc_to_ns_floor64(cycles) / 1000000.f)
#else
			 "time=%d ms",
			 ((uint32_t)k_cyc_to_ns_floor64(cycles) / 1000000)
#endif
			);
	}

	PR_SHELL(ping_ctx.sh, "%d bytes from %s to %s: icmp_seq=%d ttl=%d "
		 "%s\n",
		 ntohs(ip_hdr->len) - net_pkt_ipv6_ext_len(pkt) -
								NET_ICMPH_LEN,
		 net_sprint_ipv4_addr(&ip_hdr->src),
		 net_sprint_ipv4_addr(&ip_hdr->dst),
		 ntohs(icmp_echo->sequence),
		 ip_hdr->ttl,
		 time_buf);

	if (ntohs(icmp_echo->sequence) == ping_ctx.count) {
		ping_done(&ping_ctx);
	}

	return 0;
}
#else
static int handle_ipv4_echo_reply(struct net_icmp_ctx *ctx,
				  struct net_pkt *pkt,
				  struct net_icmp_ip_hdr *hdr,
				  struct net_icmp_hdr *icmp_hdr,
				  void *user_data)
{
	ARG_UNUSED(ctx);
	ARG_UNUSED(pkt);
	ARG_UNUSED(hdr);
	ARG_UNUSED(icmp_hdr);
	ARG_UNUSED(user_data);

	return -ENOTSUP;
}
#endif /* CONFIG_NET_IPV4 */

static int parse_arg(size_t *i, size_t argc, char *argv[])
{
	int res;
	int err;
	int base;
	const char *str = argv[*i] + 2;

	if (*str == 0) {
		if (*i + 1 >= argc) {
			return -1;
		}

		*i += 1;
		str = argv[*i];
	}

	if (strncmp(str, "0x", 2) == 0) {
		base = 16;
	} else {
		base = 10;
	}

	err = 0;
	res = shell_strtol(str, base, &err);
	if (err != 0) {
		return -1;
	}

	return res;
}

static void ping_cleanup(struct ping_context *ctx)
{
	(void)net_icmp_cleanup_ctx(&ctx->icmp);
	shell_set_bypass(ctx->sh, NULL);
}

static void ping_done(struct ping_context *ctx)
{
	k_work_cancel_delayable(&ctx->work);
	ping_cleanup(ctx);
	/* Dummy write to refresh the prompt. */
	shell_fprintf(ctx->sh, SHELL_NORMAL, "");
}

static void ping_work(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct ping_context *ctx =
			CONTAINER_OF(dwork, struct ping_context, work);
	const struct shell *sh = ctx->sh;
	struct net_icmp_ping_params params;
	int ret;

	ctx->sequence++;

	if (ctx->sequence > ctx->count) {
		PR_INFO("Ping timeout\n");
		ping_done(ctx);
		return;
	}

	if (ctx->sequence < ctx->count) {
		k_work_reschedule(&ctx->work, K_MSEC(ctx->interval));
	} else {
		k_work_reschedule(&ctx->work, K_SECONDS(2));
	}

	params.identifier = sys_rand32_get();
	params.sequence = ctx->sequence;
	params.tc_tos = ctx->tos;
	params.priority = ctx->priority;
	params.data = NULL;
	params.data_size = ctx->payload_size;

	ret = net_icmp_send_echo_request(&ctx->icmp,
					 ctx->iface,
					 &ctx->addr,
					 &params,
					 ctx);
	if (ret != 0) {
		PR_WARNING("Failed to send ping, err: %d", ret);
		ping_done(ctx);
		return;
	}
}

#define ASCII_CTRL_C 0x03

static void ping_bypass(const struct shell *sh, uint8_t *data, size_t len)
{
	ARG_UNUSED(sh);

	for (size_t i = 0; i < len; i++) {
		if (data[i] == ASCII_CTRL_C) {
			k_work_cancel_delayable(&ping_ctx.work);
			ping_cleanup(&ping_ctx);
			break;
		}
	}
}

static struct net_if *ping_select_iface(int id, struct sockaddr *target)
{
	struct net_if *iface = net_if_get_by_index(id);

	if (iface != NULL) {
		goto out;
	}

	if (IS_ENABLED(CONFIG_NET_IPV4) && target->sa_family == AF_INET) {
		iface = net_if_ipv4_select_src_iface(&net_sin(target)->sin_addr);
		if (iface != NULL) {
			goto out;
		}

		iface = net_if_get_default();
		goto out;
	}

	if (IS_ENABLED(CONFIG_NET_IPV6) && target->sa_family == AF_INET6) {
		struct net_nbr *nbr;
#if defined(CONFIG_NET_ROUTE)
		struct net_route_entry *route;
#endif

		iface = net_if_ipv6_select_src_iface(&net_sin6(target)->sin6_addr);
		if (iface != NULL) {
			goto out;
		}

		nbr = net_ipv6_nbr_lookup(NULL, &net_sin6(target)->sin6_addr);
		if (nbr) {
			iface = nbr->iface;
			goto out;
		}

#if defined(CONFIG_NET_ROUTE)
		route = net_route_lookup(NULL, &net_sin6(target)->sin6_addr);
		if (route) {
			iface = route->iface;
			goto out;
		}
#endif

		iface = net_if_get_default();
	}

out:
	return iface;
}

#endif /* CONFIG_NET_IP */

static int cmd_net_ping(const struct shell *sh, size_t argc, char *argv[])
{
#if !defined(CONFIG_NET_IPV4) && !defined(CONFIG_NET_IPV6)
	ARG_UNUSED(sh);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	return -EOPNOTSUPP;
#else
	char *host = NULL;

	int count = 3;
	int interval = 1000;
	int iface_idx = -1;
	int tos = 0;
	int payload_size = 4;
	int priority = -1;
	int ret;

	for (size_t i = 1; i < argc; ++i) {

		if (*argv[i] != '-') {
			host = argv[i];
			continue;
		}

		switch (argv[i][1]) {
		case 'c':
			count = parse_arg(&i, argc, argv);
			if (count < 0) {
				PR_WARNING("Parse error: %s\n", argv[i]);
				return -ENOEXEC;
			}


			break;
		case 'i':
			interval = parse_arg(&i, argc, argv);
			if (interval < 0) {
				PR_WARNING("Parse error: %s\n", argv[i]);
				return -ENOEXEC;
			}

			break;

		case 'I':
			iface_idx = parse_arg(&i, argc, argv);
			if (iface_idx < 0 || !net_if_get_by_index(iface_idx)) {
				PR_WARNING("Parse error: %s\n", argv[i]);
				return -ENOEXEC;
			}
			break;

		case 'p':
			priority = parse_arg(&i, argc, argv);
			if (priority < 0 || priority > UINT8_MAX) {
				PR_WARNING("Parse error: %s\n", argv[i]);
				return -ENOEXEC;
			}
			break;

		case 'Q':
			tos = parse_arg(&i, argc, argv);
			if (tos < 0 || tos > UINT8_MAX) {
				PR_WARNING("Parse error: %s\n", argv[i]);
				return -ENOEXEC;
			}

			break;

		case 's':
			payload_size = parse_arg(&i, argc, argv);
			if (payload_size < 0 || payload_size > UINT16_MAX) {
				PR_WARNING("Parse error: %s\n", argv[i]);
				return -ENOEXEC;
			}

			break;

		default:
			PR_WARNING("Unrecognized argument: %s\n", argv[i]);
			return -ENOEXEC;
		}
	}

	if (!host) {
		PR_WARNING("Target host missing\n");
		return -ENOEXEC;
	}

	memset(&ping_ctx, 0, sizeof(ping_ctx));

	k_work_init_delayable(&ping_ctx.work, ping_work);

	ping_ctx.sh = sh;
	ping_ctx.count = count;
	ping_ctx.interval = interval;
	ping_ctx.priority = priority;
	ping_ctx.tos = tos;
	ping_ctx.payload_size = payload_size;

	if (IS_ENABLED(CONFIG_NET_IPV6) &&
	    net_addr_pton(AF_INET6, host, &ping_ctx.addr6.sin6_addr) == 0) {
		ping_ctx.addr6.sin6_family = AF_INET6;

		ret = net_icmp_init_ctx(&ping_ctx.icmp, NET_ICMPV6_ECHO_REPLY, 0,
					handle_ipv6_echo_reply);
		if (ret < 0) {
			PR_WARNING("Cannot initialize ICMP context for %s\n", "IPv6");
			return 0;
		}
	} else if (IS_ENABLED(CONFIG_NET_IPV4) &&
		   net_addr_pton(AF_INET, host, &ping_ctx.addr4.sin_addr) == 0) {
		ping_ctx.addr4.sin_family = AF_INET;

		ret = net_icmp_init_ctx(&ping_ctx.icmp, NET_ICMPV4_ECHO_REPLY, 0,
					handle_ipv4_echo_reply);
		if (ret < 0) {
			PR_WARNING("Cannot initialize ICMP context for %s\n", "IPv4");
			return 0;
		}
	} else {
		PR_WARNING("Invalid IP address\n");
		return 0;
	}

	ping_ctx.iface = ping_select_iface(iface_idx, &ping_ctx.addr);

	PR("PING %s\n", host);

	shell_set_bypass(sh, ping_bypass);
	k_work_reschedule(&ping_ctx.work, K_NO_WAIT);

	return 0;
#endif
}

SHELL_STATIC_SUBCMD_SET_CREATE(net_cmd_ping,
	SHELL_CMD(--help, NULL,
		  "'net ping [-c count] [-i interval ms] [-I <iface index>] "
		  "[-Q tos] [-s payload size] [-p priority] <host>' "
		  "Send ICMPv4 or ICMPv6 Echo-Request to a network host.",
		  cmd_net_ping),
	SHELL_SUBCMD_SET_END
);

SHELL_SUBCMD_ADD((net), ping, &net_cmd_ping,
		 "Ping a network host.",
		 cmd_net_ping, 2, 12);
