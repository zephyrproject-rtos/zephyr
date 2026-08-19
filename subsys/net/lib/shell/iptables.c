/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_shell);

#include "net_shell_private.h"

#if defined(CONFIG_NET_IPV4_NAT)
#include <zephyr/net/ipv4_nat.h>
#include "../ip/ipv4_nat_internal.h"
#endif

#define IPSTR "%hhu.%hhu.%hhu.%hhu"
#define IP2STR(a) (a)[0], (a)[1], (a)[2], (a)[3]

#if !defined(CONFIG_NET_IPV4_NAT)
static void print_iptable_error(const struct shell *sh)
{
	PR_INFO("Set %s to enable %s support.\n",
		"CONFIG_NET_IPV4_NAT", "iptable");
}
#endif

#if defined(CONFIG_NET_IPV4_NAT)
extern struct net_ip4_table iptables;
#endif

static int cmd_iptable(const struct shell *sh, size_t argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

#if defined(CONFIG_NET_IPV4_NAT)
	struct net_iptable_rule *rule, *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&iptables.rules, rule, next, node) {
		PR("%d %d " IPSTR ":" IPSTR " -> " IPSTR ":" IPSTR " prio %d timeout %d:%d\n",
		   rule->idx, rule->proto, IP2STR(rule->src), IP2STR(rule->src_mask),
		   IP2STR(rule->dst), IP2STR(rule->dst_mask), rule->priority,
		   rule->unreply_timeout, rule->reply_timeout);
	}
#else
	print_iptable_error(sh);
#endif
	return 0;
}

static int cmd_net_iptable_add(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_IPV4_NAT)
	int ret = 0;
	int opt;
	int opt_index = 0;
	struct sys_getopt_state *state;
	static const struct sys_getopt_option long_options[] = {
		{"in", sys_getopt_required_argument, 0, 'i'},
		{"out", sys_getopt_required_argument, 0, 'o'},
		{"src-ip", sys_getopt_required_argument, 0, 's'},
		{"dst-ip", sys_getopt_required_argument, 0, 'd'},
		{"src-mask", sys_getopt_required_argument, 0, 'S'},
		{"dst-mask", sys_getopt_required_argument, 0, 'D'},
		{"proto", sys_getopt_required_argument, 0, 'p'},
		{"prio", sys_getopt_required_argument, 0, 'P'},
		{"unreply-timeout", sys_getopt_required_argument, 0, 't'},
		{"reply-timeout", sys_getopt_required_argument, 0, 'T'},
		{"help", sys_getopt_no_argument, 0, 'h'},
		{0, 0, 0, 0}};
	struct net_iptable_rule_params params = {0};

	while ((opt = sys_getopt_long(argc, argv, "i:o:s:d:S:D:p:P:t:T:h",
				  long_options, &opt_index)) != -1) {
		state = sys_getopt_state_get();
		switch (opt) {
		case 'i':
			params.input_iface_idx = shell_strtol(state->optarg, 10, &ret);
			break;
		case 'o':
			params.output_iface_idx = shell_strtol(state->optarg, 10, &ret);
			break;
		case 's':
			if (net_addr_pton(NET_AF_INET, state->optarg, params.src)) {
				PR_ERROR("Invalid address: %s\n", state->optarg);
				return -EINVAL;
			}
			break;
		case 'S':
			if (net_addr_pton(NET_AF_INET, state->optarg, params.src_mask)) {
				PR_ERROR("Invalid address: %s\n", state->optarg);
				return -EINVAL;
			}
			break;
		case 'd':
			if (net_addr_pton(NET_AF_INET, state->optarg, params.dst)) {
				PR_ERROR("Invalid address: %s\n", state->optarg);
				return -EINVAL;
			}
			break;
		case 'D':
			if (net_addr_pton(NET_AF_INET, state->optarg, params.dst_mask)) {
				PR_ERROR("Invalid address: %s\n", state->optarg);
				return -EINVAL;
			}
			break;
		case 'p':
			params.proto = shell_strtol(state->optarg, 10, &ret);
			break;
		case 'P':
			params.priority = shell_strtol(state->optarg, 10, &ret);
			break;
		case 't':
			params.unreply_timeout = shell_strtol(state->optarg, 10, &ret);
			break;
		case 'T':
			params.reply_timeout = shell_strtol(state->optarg, 10, &ret);
			break;
		case 'h':
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		default:
			PR_ERROR("Invalid option %c\n", state->optopt);
			return -EINVAL;
		}

		if (ret) {
			PR_ERROR("Invalid argument %d ret %d\n", opt_index, ret);
			return -EINVAL;
		}
	}

	if (params.proto != NET_IPPROTO_ICMP &&
	    params.proto != NET_IPPROTO_TCP &&
	    params.proto != NET_IPPROTO_UDP) {
		PR_ERROR("iptable rule add only support ICMP, TCP, UDP\n");
		return -EINVAL;
	}

	ret = net_ipv4_table_rule_add(&params);
	if (ret) {
		PR_ERROR("iptable rule add fail ret %d\n", ret);
		return -EINVAL;
	}
#else
	print_iptable_error(sh);
#endif
	return 0;
}

static int cmd_net_iptable_del(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_IPV4_NAT)
	int ret = 0;
	int id = 0;

	if (argc >= 2) {
		id = shell_strtol(argv[1], 10, &ret);
	}

	if (ret) {
		PR_ERROR("parse iptable del index fail\n");
		return -EINVAL;
	}

	net_ipv4_table_rule_del(id);
#else
	print_iptable_error(sh);
#endif
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(net_cmd_iptable,
	SHELL_CMD(add, NULL,
		  SHELL_HELP("Add a iptable forward rule",
			    "-i, --input <interface index>\n"
			    "-o, --output <interface index>\n"
			    "-s, --src \"src IP address\"\n"
			    "-S, --src-mask \"src IP mask\"\n"
			    "-d, --dst \"dst IP address\"\n"
			    "-D, --dst-mask \"dst IP mask\"\n"
			    "-p, --proto <IP protocol 1/6/17>\n"
			    "[-P, --prio rule match priority in ascending order]\n"
			    "[-t, --unreply-timeout <timeout in seconds for new connection track>]\n"
			    "[-T, --reply-timeout <timeout in seconds for "
			    "established connection track>]\n"),
		  cmd_net_iptable_add),
	SHELL_CMD(del, NULL,
		  SHELL_HELP("Delete the iptable forward rule",
			     "<index>"),
		  cmd_net_iptable_del),
	SHELL_CMD(show, NULL,
		  SHELL_HELP("Show all the iptable forward rules", NULL),
		  cmd_iptable),
	SHELL_SUBCMD_SET_END
);

SHELL_SUBCMD_ADD((net), iptable, &net_cmd_iptable,
		 "Print information of IPv4 iptable rules",
		 cmd_iptable, 1, 20);
