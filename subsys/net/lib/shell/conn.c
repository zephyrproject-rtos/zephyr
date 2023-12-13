/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_shell);

#include "net_shell_private.h"

#if defined(CONFIG_NET_TCP)
#include "tcp_internal.h"
#include <zephyr/sys/slist.h>
#endif

#if defined(CONFIG_NET_OFFLOAD) || defined(CONFIG_NET_NATIVE)
static void context_cb(struct net_context *context, void *user_data)
{
#if defined(CONFIG_NET_IPV6) && !defined(CONFIG_NET_IPV4)
#define ADDR_LEN NET_IPV6_ADDR_LEN
#elif defined(CONFIG_NET_IPV4) && !defined(CONFIG_NET_IPV6)
#define ADDR_LEN NET_IPV4_ADDR_LEN
#else
#define ADDR_LEN NET_IPV6_ADDR_LEN
#endif
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	int *count = data->user_data;
	/* +7 for []:port */
	char addr_local[ADDR_LEN + 7];
	char addr_remote[ADDR_LEN + 7] = "";

	get_addresses(context, addr_local, sizeof(addr_local),
		      addr_remote, sizeof(addr_remote));

	PR("[%2d] %p\t%d      %c%c%c   %16s\t%16s\n",
	   (*count) + 1, context,
	   net_if_get_by_iface(net_context_get_iface(context)),
	   net_context_get_family(context) == AF_INET6 ? '6' :
	   (net_context_get_family(context) == AF_INET ? '4' : ' '),
	   net_context_get_type(context) == SOCK_DGRAM ? 'D' :
	   (net_context_get_type(context) == SOCK_STREAM ? 'S' :
	    (net_context_get_type(context) == SOCK_RAW ? 'R' : ' ')),
	   net_context_get_proto(context) == IPPROTO_UDP ? 'U' :
	   (net_context_get_proto(context) == IPPROTO_TCP ? 'T' : ' '),
	   addr_local, addr_remote);

	(*count)++;
}
#endif /* CONFIG_NET_OFFLOAD || CONFIG_NET_NATIVE */

#if CONFIG_NET_CONN_LOG_LEVEL >= LOG_LEVEL_DBG
static void conn_handler_cb(struct net_conn *conn, void *user_data)
{
#if defined(CONFIG_NET_IPV6) && !defined(CONFIG_NET_IPV4)
#define ADDR_LEN NET_IPV6_ADDR_LEN
#elif defined(CONFIG_NET_IPV4) && !defined(CONFIG_NET_IPV6)
#define ADDR_LEN NET_IPV4_ADDR_LEN
#else
#define ADDR_LEN NET_IPV6_ADDR_LEN
#endif
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	int *count = data->user_data;
	/* +7 for []:port */
	char addr_local[ADDR_LEN + 7];
	char addr_remote[ADDR_LEN + 7] = "";

	if (IS_ENABLED(CONFIG_NET_IPV6) && conn->local_addr.sa_family == AF_INET6) {
		snprintk(addr_local, sizeof(addr_local), "[%s]:%u",
			 net_sprint_ipv6_addr(
				 &net_sin6(&conn->local_addr)->sin6_addr),
			 ntohs(net_sin6(&conn->local_addr)->sin6_port));
		snprintk(addr_remote, sizeof(addr_remote), "[%s]:%u",
			 net_sprint_ipv6_addr(
				 &net_sin6(&conn->remote_addr)->sin6_addr),
			 ntohs(net_sin6(&conn->remote_addr)->sin6_port));

	} else if (IS_ENABLED(CONFIG_NET_IPV4) && conn->local_addr.sa_family == AF_INET) {
		snprintk(addr_local, sizeof(addr_local), "%s:%d",
			 net_sprint_ipv4_addr(
				 &net_sin(&conn->local_addr)->sin_addr),
			 ntohs(net_sin(&conn->local_addr)->sin_port));
		snprintk(addr_remote, sizeof(addr_remote), "%s:%d",
			 net_sprint_ipv4_addr(
				 &net_sin(&conn->remote_addr)->sin_addr),
			 ntohs(net_sin(&conn->remote_addr)->sin_port));

	} else if (conn->local_addr.sa_family == AF_UNSPEC) {
		snprintk(addr_local, sizeof(addr_local), "AF_UNSPEC");
	} else {
		snprintk(addr_local, sizeof(addr_local), "AF_UNK(%d)",
			 conn->local_addr.sa_family);
	}

	PR("[%2d] %p %p\t%s\t%16s\t%16s\n",
	   (*count) + 1, conn, conn->cb,
	   net_proto2str(conn->local_addr.sa_family, conn->proto),
	   addr_local, addr_remote);

	(*count)++;
}
#endif /* CONFIG_NET_CONN_LOG_LEVEL >= LOG_LEVEL_DBG */

#if CONFIG_NET_TCP_LOG_LEVEL >= LOG_LEVEL_DBG
struct tcp_detail_info {
	int printed_send_queue_header;
	int printed_details;
	int count;
};
#endif

#if defined(CONFIG_NET_TCP) && \
	(defined(CONFIG_NET_OFFLOAD) || defined(CONFIG_NET_NATIVE))
static void tcp_cb(struct tcp *conn, void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	int *count = data->user_data;
	uint16_t recv_mss = net_tcp_get_supported_mss(conn);

	PR("%p %p   %5u    %5u %10u %10u %5u   %s\n",
	   conn, conn->context,
	   ntohs(net_sin6_ptr(&conn->context->local)->sin6_port),
	   ntohs(net_sin6(&conn->context->remote)->sin6_port),
	   conn->seq, conn->ack, recv_mss,
	   net_tcp_state_str(net_tcp_get_state(conn)));

	(*count)++;
}

#if CONFIG_NET_TCP_LOG_LEVEL >= LOG_LEVEL_DBG
static void tcp_sent_list_cb(struct tcp *conn, void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	struct tcp_detail_info *details = data->user_data;
	struct net_pkt *pkt;
	sys_snode_t *node;

	if (conn->state != TCP_LISTEN) {
		if (!details->printed_details) {
			PR("\nTCP        Ref  Recv_win Send_win Pending "
			   "Unacked Flags Queue\n");
			details->printed_details = true;
		}

		PR("%p   %ld    %u\t %u\t  %zd\t  %d\t  %d/%d/%d %s\n",
		   conn, atomic_get(&conn->ref_count), conn->recv_win,
		   conn->send_win, conn->send_data_total, conn->unacked_len,
		   conn->in_retransmission, conn->in_connect, conn->in_close,
		   sys_slist_is_empty(&conn->send_queue) ? "empty" : "data");

		details->count++;
	}

	if (sys_slist_is_empty(&conn->send_queue)) {
		return;
	}

	if (!details->printed_send_queue_header) {
		PR("\nTCP packets waiting ACK:\n");
		PR("TCP             net_pkt[ref/totlen]->net_buf[ref/len]..."
		   "\n");
	}

	PR("%p      ", conn);

	node = sys_slist_peek_head(&conn->send_queue);
	if (node) {
		pkt = CONTAINER_OF(node, struct net_pkt, next);
		if (pkt) {
			struct net_buf *frag = pkt->frags;

			if (!details->printed_send_queue_header) {
				PR("%p[%ld/%zd]", pkt,
				   atomic_get(&pkt->atomic_ref),
				   net_pkt_get_len(pkt));
				details->printed_send_queue_header = true;
			} else {
				PR("                %p[%ld/%zd]",
				   pkt, atomic_get(&pkt->atomic_ref),
				   net_pkt_get_len(pkt));
			}

			if (frag) {
				PR("->");
			}

			while (frag) {
				PR("%p[%d/%d]", frag, frag->ref, frag->len);

				frag = frag->frags;
				if (frag) {
					PR("->");
				}
			}

			PR("\n");
		}
	}

	details->printed_send_queue_header = true;
}
#endif /* CONFIG_NET_TCP_LOG_LEVEL >= LOG_LEVEL_DBG */
#endif /* TCP */

static int cmd_net_conn(const struct shell *sh, size_t argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

#if defined(CONFIG_NET_OFFLOAD) || defined(CONFIG_NET_NATIVE)
	struct net_shell_user_data user_data;
	int count = 0;

	PR("     Context   \tIface  Flags            Local             Remote\n");

	user_data.sh = sh;
	user_data.user_data = &count;

	net_context_foreach(context_cb, &user_data);

	if (count == 0) {
		PR("No connections\n");
	}

#if CONFIG_NET_CONN_LOG_LEVEL >= LOG_LEVEL_DBG
	PR("\n     Handler    Callback  \tProto\tLocal           \tRemote\n");

	count = 0;

	net_conn_foreach(conn_handler_cb, &user_data);

	if (count == 0) {
		PR("No connection handlers found.\n");
	}
#endif

#if defined(CONFIG_NET_TCP)
	PR("\nTCP        Context   Src port Dst port   "
	   "Send-Seq   Send-Ack  MSS    State\n");

	count = 0;

	net_tcp_foreach(tcp_cb, &user_data);

	if (count == 0) {
		PR("No TCP connections\n");
	} else {
#if CONFIG_NET_TCP_LOG_LEVEL >= LOG_LEVEL_DBG
		/* Print information about pending packets */
		struct tcp_detail_info details;

		count = 0;

		if (IS_ENABLED(CONFIG_NET_TCP)) {
			memset(&details, 0, sizeof(details));
			user_data.user_data = &details;
		}

		net_tcp_foreach(tcp_sent_list_cb, &user_data);

		if (IS_ENABLED(CONFIG_NET_TCP)) {
			if (details.count == 0) {
				PR("No active connections.\n");
			}
		}
#endif /* CONFIG_NET_TCP_LOG_LEVEL >= LOG_LEVEL_DBG */
	}

#if CONFIG_NET_TCP_LOG_LEVEL < LOG_LEVEL_DBG
	PR_INFO("Set %s to enable %s support.\n",
		"CONFIG_NET_TCP_LOG_LEVEL_DBG", "TCP debugging");
#endif /* CONFIG_NET_TCP_LOG_LEVEL < LOG_LEVEL_DBG */

#endif

#if defined(CONFIG_NET_IPV6_FRAGMENT)
	count = 0;

	net_ipv6_frag_foreach(ipv6_frag_cb, &user_data);

	/* Do not print anything if no fragments are pending atm */
#endif

#else
	PR_INFO("Set %s to enable %s support.\n",
		"CONFIG_NET_OFFLOAD or CONFIG_NET_NATIVE",
		"connection information");

#endif /* CONFIG_NET_OFFLOAD || CONFIG_NET_NATIVE */

	return 0;
}


SHELL_SUBCMD_ADD((net), conn, NULL, "Print information about network connections.",
		 cmd_net_conn, 1, 0);
