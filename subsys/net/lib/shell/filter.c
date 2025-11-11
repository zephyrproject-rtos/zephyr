/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_shell);

#include <zephyr/net/net_pkt_filter.h>

#include "net_shell_private.h"


#if defined(CONFIG_NET_PKT_FILTER)
static const char *rule_type2str(enum npf_rule_type type)
{
	switch (type) {
	case NPF_RULE_TYPE_SEND:
		return "send";
	case NPF_RULE_TYPE_RECV:
		return "recv";
	case NPF_RULE_TYPE_LOCAL_IN_RECV:
		return "local recv";
	case NPF_RULE_TYPE_IPV4_RECV:
		return "IPv4 recv";
	case NPF_RULE_TYPE_IPV6_RECV:
		return "IPv6 recv";
	case NPF_RULE_TYPE_UNKNOWN:
		__fallthrough;
	default:
		break;
	}

	return "<UNKNOWN>";
}

static const char *verdict2str(enum net_verdict verdict)
{
	switch (verdict) {
	case NET_OK:
		return "OK";
	case NET_DROP:
		return "DROP";
	case NET_CONTINUE:
		return "CONTINUE";
	default:
		break;
	}

	return "<UNK>";
}

static void rule_cb(struct npf_rule *rule, enum npf_rule_type type, void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	int *count = data->user_data;
	uint8_t tc;
	int thread_prio;

	PR("[%2d]  %-10s  %-8s  ",
	   (*count) + 1, rule_type2str(type), verdict2str(rule->result));

	if (rule->result == NET_CONTINUE && type == NPF_RULE_TYPE_SEND) {
		tc = net_tx_priority2tc(rule->priority);
		if (net_tc_tx_is_immediate(tc, rule->priority)) {
			PR("%8d  %5d         SKIP  ", rule->priority, tc);
		} else {
			thread_prio =  net_tc_tx_thread_priority(tc);
			PR("%8d  %5d  %11d  ", rule->priority, tc, thread_prio);
		}
	} else if (rule->result == NET_CONTINUE) {
		tc = net_rx_priority2tc(rule->priority);
		if (net_tc_rx_is_immediate(tc, rule->priority)) {
			PR("%8d  %5d         SKIP  ", rule->priority, tc);
		} else {
			thread_prio =  net_tc_rx_thread_priority(tc);
			PR("%8d  %5d  %11d  ", rule->priority, tc, thread_prio);
		}
	} else {
		PR("     N/A    N/A          N/A  ");
	}

	PR("%-5d", rule->nb_tests);

	for (int i = 0; i < rule->nb_tests; i++) {
		/* Allocate room for storing two full IPv4/6 addresses */
#define MAX_BUF_LEN ((IS_ENABLED(CONFIG_NET_IPV6) ?		\
		      INET6_ADDRSTRLEN : INET_ADDRSTRLEN) * 2)
		char buf[MAX_BUF_LEN] = { 0 };
		struct npf_test *test;
		const char *str;

		test = rule->tests[i];

		str = npf_test_get_str(test, buf, sizeof(buf) - 1);
		PR("%s%s%s", str, buf[0] != '\0' ? buf : "",
		   i == rule->nb_tests - 1 ? "" : ",");
	}

	PR("\n");
	(*count)++;
}
#endif /* CONFIG_NET_PKT_FILTER */

static int cmd_net_filter(const struct shell *sh, size_t argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

#if defined(CONFIG_NET_PKT_FILTER)
	struct net_shell_user_data user_data;
	int count = 0;

	PR("Rule  %-10s  Verdict   Pkt-Prio  Queue  Thread-Prio  Tests\n", "Type");

	user_data.sh = sh;
	user_data.user_data = &count;

	npf_rules_foreach(rule_cb, &user_data);

	if (count == 0) {
		PR("No network packet filter rules\n");
	}
#else
	PR_INFO("Set %s to enable %s support.\n",
		"CONFIG_NET_PKT_FILTER",
		"packet filter information");

#endif /* CONFIG_NET_PKT_FILTER */

	return 0;
}


SHELL_SUBCMD_ADD((net), filter, NULL, "Print information about network packet filters.",
		 cmd_net_filter, 1, 0);
