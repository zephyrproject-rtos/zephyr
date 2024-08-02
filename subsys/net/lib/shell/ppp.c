/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_shell);

#include <zephyr/net/ppp.h>

#include "net_shell_private.h"

#if defined(CONFIG_NET_L2_PPP)
#include <zephyr/net/ppp.h>
#include "ppp/ppp_internal.h"
#endif

static int cmd_net_ppp_ping(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_PPP)
	if (argv[1]) {
		int ret, idx = get_iface_idx(sh, argv[1]);

		if (idx < 0) {
			return -ENOEXEC;
		}

		ret = net_ppp_ping(idx, MSEC_PER_SEC * 1);
		if (ret < 0) {
			if (ret == -EAGAIN) {
				PR_INFO("PPP Echo-Req timeout.\n");
			} else if (ret == -ENODEV || ret == -ENOENT) {
				PR_INFO("Not a PPP interface (%d)\n", idx);
			} else {
				PR_INFO("PPP Echo-Req failed (%d)\n", ret);
			}
		} else {
			if (ret > 1000) {
				PR_INFO("%s%d msec\n",
					"Received PPP Echo-Reply in ",
					ret / 1000);
			} else {
				PR_INFO("%s%d usec\n",
					"Received PPP Echo-Reply in ", ret);
			}
		}
	} else {
		PR_INFO("PPP network interface must be given.\n");
		return -ENOEXEC;
	}
#else
	PR_INFO("Set %s to enable %s support.\n", "CONFIG_NET_L2_PPP", "PPP");
#endif
	return 0;
}

static int cmd_net_ppp_status(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_PPP)
	int idx = 0;
	struct ppp_context *ctx;

	if (argv[1]) {
		idx = get_iface_idx(sh, argv[1]);
		if (idx < 0) {
			return -ENOEXEC;
		}
	}

	ctx = net_ppp_context_get(idx);
	if (!ctx) {
		PR_INFO("PPP context not found.\n");
		return -ENOEXEC;
	}

	PR("PPP phase           : %s (%d)\n", ppp_phase_str(ctx->phase),
								ctx->phase);
	PR("LCP state           : %s (%d)\n",
	   ppp_state_str(ctx->lcp.fsm.state), ctx->lcp.fsm.state);
	PR("LCP retransmits     : %u\n", ctx->lcp.fsm.retransmits);
	PR("LCP NACK loops      : %u\n", ctx->lcp.fsm.nack_loops);
	PR("LCP NACKs recv      : %u\n", ctx->lcp.fsm.recv_nack_loops);
	PR("LCP current id      : %d\n", ctx->lcp.fsm.id);
	PR("LCP ACK received    : %s\n", ctx->lcp.fsm.ack_received ?
								"yes" : "no");

#if defined(CONFIG_NET_IPV4)
	PR("IPCP state          : %s (%d)\n",
	   ppp_state_str(ctx->ipcp.fsm.state), ctx->ipcp.fsm.state);
	PR("IPCP retransmits    : %u\n", ctx->ipcp.fsm.retransmits);
	PR("IPCP NACK loops     : %u\n", ctx->ipcp.fsm.nack_loops);
	PR("IPCP NACKs recv     : %u\n", ctx->ipcp.fsm.recv_nack_loops);
	PR("IPCP current id     : %d\n", ctx->ipcp.fsm.id);
	PR("IPCP ACK received   : %s\n", ctx->ipcp.fsm.ack_received ?
								"yes" : "no");
#endif /* CONFIG_NET_IPV4 */

#if defined(CONFIG_NET_IPV6)
	PR("IPv6CP state        : %s (%d)\n",
	   ppp_state_str(ctx->ipv6cp.fsm.state), ctx->ipv6cp.fsm.state);
	PR("IPv6CP retransmits  : %u\n", ctx->ipv6cp.fsm.retransmits);
	PR("IPv6CP NACK loops   : %u\n", ctx->ipv6cp.fsm.nack_loops);
	PR("IPv6CP NACKs recv   : %u\n", ctx->ipv6cp.fsm.recv_nack_loops);
	PR("IPv6CP current id   : %d\n", ctx->ipv6cp.fsm.id);
	PR("IPv6CP ACK received : %s\n", ctx->ipv6cp.fsm.ack_received ?
								"yes" : "no");
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_L2_PPP_PAP)
	PR("PAP state           : %s (%d)\n",
	   ppp_state_str(ctx->pap.fsm.state), ctx->pap.fsm.state);
	PR("PAP retransmits     : %u\n", ctx->pap.fsm.retransmits);
	PR("PAP NACK loops      : %u\n", ctx->pap.fsm.nack_loops);
	PR("PAP NACKs recv      : %u\n", ctx->pap.fsm.recv_nack_loops);
	PR("PAP current id      : %d\n", ctx->pap.fsm.id);
	PR("PAP ACK received    : %s\n", ctx->pap.fsm.ack_received ?
								"yes" : "no");
#endif /* CONFIG_NET_L2_PPP_PAP */

#else
	PR_INFO("Set %s to enable %s support.\n",
		"CONFIG_NET_L2_PPP and CONFIG_NET_PPP", "PPP");
#endif
	return 0;
}

#if defined(CONFIG_NET_SHELL_DYN_CMD_COMPLETION)

#define MAX_IFACE_HELP_STR_LEN sizeof("longbearername (0xabcd0123)")
#define MAX_IFACE_STR_LEN sizeof("xxx")

#if defined(CONFIG_NET_PPP)
static char iface_ppp_help_buffer[MAX_IFACE_COUNT][MAX_IFACE_HELP_STR_LEN];
static char iface_ppp_index_buffer[MAX_IFACE_COUNT][MAX_IFACE_STR_LEN];

static char *set_iface_ppp_index_buffer(size_t idx)
{
	struct net_if *iface = net_if_get_by_index(idx);

	/* Network interfaces start at 1 */
	if (idx == 0) {
		return "";
	}

	if (!iface) {
		return NULL;
	}

	if (net_if_l2(iface) != &NET_L2_GET_NAME(PPP)) {
		return NULL;
	}

	snprintk(iface_ppp_index_buffer[idx], MAX_IFACE_STR_LEN, "%d", (uint8_t)idx);

	return iface_ppp_index_buffer[idx];
}

static char *set_iface_ppp_index_help(size_t idx)
{
	struct net_if *iface = net_if_get_by_index(idx);

	/* Network interfaces start at 1 */
	if (idx == 0) {
		return "";
	}

	if (!iface) {
		return NULL;
	}

	if (net_if_l2(iface) != &NET_L2_GET_NAME(PPP)) {
		return NULL;
	}

#if defined(CONFIG_NET_INTERFACE_NAME)
	char name[CONFIG_NET_INTERFACE_NAME_LEN + 1];

	net_if_get_name(iface, name, CONFIG_NET_INTERFACE_NAME_LEN);
	name[CONFIG_NET_INTERFACE_NAME_LEN] = '\0';

	snprintk(iface_ppp_help_buffer[idx], MAX_IFACE_HELP_STR_LEN,
		 "%s [%s] (%p)", name, iface2str(iface, NULL), iface);
#else
	snprintk(iface_ppp_help_buffer[idx], MAX_IFACE_HELP_STR_LEN,
		 "%s (%p)", iface2str(iface, NULL), iface);
#endif

	return iface_ppp_help_buffer[idx];
}

static void iface_ppp_index_get(size_t idx, struct shell_static_entry *entry);

SHELL_DYNAMIC_CMD_CREATE(iface_ppp_index, iface_ppp_index_get);

static void iface_ppp_index_get(size_t idx, struct shell_static_entry *entry)
{
	entry->handler = NULL;
	entry->help  = set_iface_ppp_index_help(idx);
	entry->subcmd = &iface_ppp_index;
	entry->syntax = set_iface_ppp_index_buffer(idx);
}

#define IFACE_PPP_DYN_CMD &iface_ppp_index
#else
#define IFACE_PPP_DYN_CMD NULL
#endif /* CONFIG_NET_PPP */

#else
#define IFACE_PPP_DYN_CMD NULL
#endif /* CONFIG_NET_SHELL_DYN_CMD_COMPLETION */

SHELL_STATIC_SUBCMD_SET_CREATE(net_cmd_ppp,
	SHELL_CMD(ping, IFACE_PPP_DYN_CMD,
		  "'net ppp ping <index>' sends Echo-request to PPP interface.",
		  cmd_net_ppp_ping),
	SHELL_CMD(status, NULL,
		  "'net ppp status' prints information about PPP.",
		  cmd_net_ppp_status),
	SHELL_SUBCMD_SET_END
);

SHELL_SUBCMD_ADD((net), ppp, &net_cmd_ppp,
		 "PPP information.",
		 cmd_net_ppp_status, 1, 0);
