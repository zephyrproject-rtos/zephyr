/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_shell);

#include "net_shell_private.h"

#if defined(CONFIG_QUIC_SHELL)

#include <zephyr/net/quic.h>
#include "quic/quic_internal.h"

static void quic_endpoint_cb(struct quic_endpoint *ep, void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	int *count = data->user_data;
	/* +7 for []:port */
	char addr_local[ADDR_LEN + 7];
	char addr_remote[ADDR_LEN + 7] = "";

	if ((*count) == 0) {
		PR("Endpoints  Ref  Sock  %-6s  %-16s\t Remote\n", "Role", "Local");
	}

	if (IS_ENABLED(CONFIG_NET_IPV6) && ep->local_addr.ss_family == NET_AF_INET6) {
		snprintk(addr_local, sizeof(addr_local), "[%s]:%u",
			 net_sprint_ipv6_addr(
				 &net_sin6(net_sad(&ep->local_addr))->sin6_addr),
			 net_ntohs(net_sin6(net_sad(&ep->local_addr))->sin6_port));
		snprintk(addr_remote, sizeof(addr_remote), "[%s]:%u",
			 net_sprint_ipv6_addr(
				 &net_sin6(net_sad(&ep->remote_addr))->sin6_addr),
			 net_ntohs(net_sin6(net_sad(&ep->remote_addr))->sin6_port));

	} else if (IS_ENABLED(CONFIG_NET_IPV4) && ep->local_addr.ss_family == NET_AF_INET) {
		snprintk(addr_local, sizeof(addr_local), "%s:%d",
			 net_sprint_ipv4_addr(
				 &net_sin(net_sad(&ep->local_addr))->sin_addr),
			 net_ntohs(net_sin(net_sad(&ep->local_addr))->sin_port));

		/* Check if we need to print the v4-mapping-to-v6 address */
		if (IS_ENABLED(CONFIG_NET_IPV4_MAPPING_TO_IPV6) &&
		    net_sin(net_sad(&ep->remote_addr))->sin_family == NET_AF_INET6 &&
		    net_ipv6_addr_is_v4_mapped(&net_sin6(net_sad(&ep->remote_addr))->sin6_addr)) {
			snprintk(addr_remote, sizeof(addr_remote), "[%s]:%d",
				 net_sprint_ipv6_addr(
					 &net_sin6(net_sad(&ep->remote_addr))->sin6_addr),
				 net_ntohs(net_sin6(net_sad(&ep->remote_addr))->sin6_port));
		} else {
			snprintk(addr_remote, sizeof(addr_remote), "%s:%d",
				 net_sprint_ipv4_addr(
					 &net_sin(net_sad(&ep->remote_addr))->sin_addr),
				 net_ntohs(net_sin(net_sad(&ep->remote_addr))->sin_port));
		}
	}

	PR("[%2d]       %d    %-2d    %-6s  %-16s\t %-16s\n",
	   (*count) + 1, (int)ep->refcount, ep->sock, ep->is_server ? "server" : "client",
	   addr_local, addr_remote);

	(*count)++;
}

static void quic_stream_cb(struct quic_stream *stream, void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	int *count = data->user_data;
	char id_str[sizeof("18446744073709551615")];

	if ((*count) == 0) {
		PR("Streams  Ref  Ctx  Ep   Sock  %-4s  Role\t %-9s\n",
		   "Dir", "Id");
	}

	if (stream->id == QUIC_STREAM_ID_UNASSIGNED) {
		snprintk(id_str, sizeof(id_str), "<waiting connection>");
	} else {
		snprintk(id_str, sizeof(id_str), "%llu", stream->id);
	}

	PR("[%2d]     %d    %-2d   %-2d   %-4d  %-4s  %s\t %s\n",
	   (*count) + 1, (int)stream->refcount, stream->conn->id,
	   stream->ep == NULL ? -1 : stream->ep->sock, stream->sock,
	   ((stream->type & 0x02) == 0) ? "bidi" : "uni",
	   /* Note that the initiator bit tells the initiator, which
	    * is the opposite of the role.
	    */
	   ((stream->type & 0x01) == 0) ? "server" : "client",
	   id_str);

	(*count)++;
}

static void quic_context_endpoint_cb(struct quic_endpoint *ep, void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	int *count = data->user_data;
	/* +7 for []:port */
	char addr_local[ADDR_LEN + 7];
	char addr_remote[ADDR_LEN + 7] = "";

	if ((*count) == 0) {
		PR("     Endpoints  Ref  %-16s\t Remote\n", "Local");
	}

	if (IS_ENABLED(CONFIG_NET_IPV6) && ep->local_addr.ss_family == NET_AF_INET6) {
		snprintk(addr_local, sizeof(addr_local), "[%s]:%u",
			 net_sprint_ipv6_addr(
				 &net_sin6(net_sad(&ep->local_addr))->sin6_addr),
			 net_ntohs(net_sin6(net_sad(&ep->local_addr))->sin6_port));
		snprintk(addr_remote, sizeof(addr_remote), "[%s]:%u",
			 net_sprint_ipv6_addr(
				 &net_sin6(net_sad(&ep->remote_addr))->sin6_addr),
			 net_ntohs(net_sin6(net_sad(&ep->remote_addr))->sin6_port));

	} else if (IS_ENABLED(CONFIG_NET_IPV4) && ep->local_addr.ss_family == NET_AF_INET) {
		snprintk(addr_local, sizeof(addr_local), "%s:%d",
			 net_sprint_ipv4_addr(
				 &net_sin(net_sad(&ep->local_addr))->sin_addr),
			 net_ntohs(net_sin(net_sad(&ep->local_addr))->sin_port));

		/* Check if we need to print the v4-mapping-to-v6 address */
		if (IS_ENABLED(CONFIG_NET_IPV4_MAPPING_TO_IPV6) &&
		    net_sin(net_sad(&ep->remote_addr))->sin_family == NET_AF_INET6 &&
		    net_ipv6_addr_is_v4_mapped(&net_sin6(net_sad(&ep->remote_addr))->sin6_addr)) {
			snprintk(addr_remote, sizeof(addr_remote), "[%s]:%d",
				 net_sprint_ipv6_addr(
					 &net_sin6(net_sad(&ep->remote_addr))->sin6_addr),
				 net_ntohs(net_sin6(net_sad(&ep->remote_addr))->sin6_port));
		} else {
			snprintk(addr_remote, sizeof(addr_remote), "%s:%d",
				 net_sprint_ipv4_addr(
					 &net_sin(net_sad(&ep->remote_addr))->sin_addr),
				 net_ntohs(net_sin(net_sad(&ep->remote_addr))->sin_port));
		}
	}

	PR("          [%2d]  %d    %-16s\t %-16s\n",
	   (*count) + 1, (int)ep->refcount, addr_local, addr_remote);

	(*count)++;
}

static void quic_context_stream_cb(struct quic_stream *stream, void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	int *count = data->user_data;
	char id_str[sizeof("18446744073709551615")]; /* Max uint64_t in decimal */

	if ((*count) == 0) {
		PR("     Streams    Ref  Sock  %-4s  Role\t %-9s\n",
		   "Dir", "Id");
	}

	if (stream->id == QUIC_STREAM_ID_UNASSIGNED) {
		snprintk(id_str, sizeof(id_str), "<waiting connection>");
	} else {
		snprintk(id_str, sizeof(id_str), "%llu", stream->id);
	}

	PR("          [%2d]  %d    %-4d  %-4s  %s\t %s\n",
	   (*count) + 1, (int)stream->refcount, stream->sock,
	   ((stream->type & 0x02) == 0) ? "bidi" : "uni",
	   /* Note that the initiator bit tells the initiator, which
	    * is the opposite of the role.
	    */
	   ((stream->type & 0x01) == 0) ? "server" : "client",
	   id_str);

	(*count)++;
}

static void quic_context_cb(struct quic_context *context,
			    void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	int *count = data->user_data;
	int extra_count = 0;
	struct net_shell_user_data extra_data = {
		.sh = sh,
		.user_data = &extra_count,
	};

	if ((*count) == 0) {
		PR("Context  Id  Sock\n");
	}

	if ((*count) > 0) {
		PR("\n");
	}

	PR("[%2d]     %-2d  %-4d\n",
	   (*count) + 1, context->id, context->sock);

	quic_context_endpoint_foreach(context, quic_context_endpoint_cb, &extra_data);

	extra_count = 0;

	quic_context_stream_foreach(context, quic_context_stream_cb, &extra_data);

	(*count)++;
}
#endif /* CONFIG_QUIC_SHELL */

static int cmd_net_quic_streams(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_QUIC_SHELL)
	struct net_shell_user_data user_data;
	int count = 0;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	user_data.sh = sh;
	user_data.user_data = &count;

	quic_stream_foreach(quic_stream_cb, &user_data);

	if (count == 0) {
		PR("No streams\n");
	}
#else
	PR_INFO("Set %s to enable %s support.\n", "CONFIG_QUIC_SHELL",
		"Quic");
#endif /* CONFIG_QUIC_SHELL */

	return 0;
}

static int cmd_net_quic_endpoints(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_QUIC_SHELL)
	struct net_shell_user_data user_data;
	int count = 0;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	user_data.sh = sh;
	user_data.user_data = &count;

	quic_endpoint_foreach(quic_endpoint_cb, &user_data);

	if (count == 0) {
		PR("No endpoints\n");
	}
#else
	PR_INFO("Set %s to enable %s support.\n", "CONFIG_QUIC_SHELL",
		"Quic");
#endif /* CONFIG_QUIC_SHELL */

	return 0;
}

static int cmd_net_quic(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_QUIC_SHELL)
	struct net_shell_user_data user_data;
	int count = 0;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	user_data.sh = sh;
	user_data.user_data = &count;

	quic_context_foreach(quic_context_cb, &user_data);

	if (count == 0) {
		PR("No connections\n");
	}
#else
	PR_INFO("Set %s to enable %s support.\n", "CONFIG_QUIC_SHELL",
		"Quic");
#endif /* CONFIG_QUIC_SHELL */

	return 0;
}

#if defined(CONFIG_NET_STATISTICS_QUIC) && defined(CONFIG_NET_STATISTICS_USER_API)
#include "quic_stats.h"

static void print_quic_stats(struct quic_context *ctx, const struct shell *sh)
{
	PR("Statistics for Quic context %d (sock %d)\n", ctx->id, ctx->sock);

	PR("Handshake init RX   : %u\n", ctx->stats.handshake_init_rx);
	PR("Handshake init TX   : %u\n", ctx->stats.handshake_init_tx);
	PR("Handshake resp RX   : %u\n", ctx->stats.handshake_resp_rx);
	PR("Handshake resp TX   : %u\n", ctx->stats.handshake_resp_tx);
	PR("Peer not found      : %u\n", ctx->stats.peer_not_found);
	PR("Invalid packet      : %u\n", ctx->stats.invalid_packet);
	PR("Invalid key         : %u\n", ctx->stats.invalid_key);
	PR("Invalid packet len  : %u\n", ctx->stats.invalid_packet_len);
	PR("Invalid handshake   : %u\n", ctx->stats.invalid_handshake);
	PR("Decrypt failed      : %u\n", ctx->stats.decrypt_failed);
	PR("Dropped RX          : %u\n", ctx->stats.drop_rx);
	PR("Dropped TX          : %u\n", ctx->stats.drop_tx);
	PR("Allocation failed   : %u\n", ctx->stats.alloc_failed);
	PR("RX data packets     : %u\n", ctx->stats.valid_rx);
	PR("TX data packets     : %u\n", ctx->stats.valid_tx);
	PR("\n");
}

static void context_stats_cb(struct quic_context *context, void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	int *count = data->user_data;

	print_quic_stats(context, sh);
	(*count)++;
}
#endif /* CONFIG_NET_STATISTICS_QUIC && CONFIG_NET_STATISTICS_USER_API */

static int cmd_net_quic_stats(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_STATISTICS_QUIC) && defined(CONFIG_NET_STATISTICS_USER_API)
	struct net_shell_user_data user_data;
	int count = 0;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	user_data.sh = sh;
	user_data.user_data = &count;

	PR("Global Quic statistics\n");

	PR("Packets RX          : %u\n", quic_stats->packets_rx);
	PR("Connections opened  : %u\n", quic_stats->connections_opened);
	PR("Connections closed  : %u\n", quic_stats->connections_closed);
	PR("Streams opened      : %u\n", quic_stats->streams_opened);
	PR("Streams closed      : %u\n", quic_stats->streams_closed);
	PR("Conn open failed    : %u\n", quic_stats->connection_open_failed);
	PR("Stream open failed  : %u\n", quic_stats->stream_open_failed);
	PR("Conn close failed   : %u\n", quic_stats->connection_close_failed);
	PR("Stream close failed : %u\n", quic_stats->stream_close_failed);
	PR("\n");

	quic_context_foreach(context_stats_cb, &user_data);

	if (count == 0) {
		PR("No connections\n");
	}
#else
	PR_INFO("Set %s to enable %s support.\n",
		"CONFIG_NET_STATISTICS_QUIC, CONFIG_NET_STATISTICS_USER_API and CONFIG_QUIC",
		"Quic statistics");
#endif /* CONFIG_NET_STATISTICS_QUIC */

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(net_cmd_quic,
	SHELL_CMD_ARG(endpoints, NULL,
		      SHELL_HELP("Show information about Quic endpoints", ""),
		      cmd_net_quic_endpoints, 1, 0),
	SHELL_CMD_ARG(streams, NULL,
		      SHELL_HELP("Show information about Quic streams", ""),
		      cmd_net_quic_streams, 1, 0),
	SHELL_CMD_ARG(stats, NULL,
		      SHELL_HELP("Show statistics information for Quic connections", ""),
		      cmd_net_quic_stats, 1, 1),
	SHELL_SUBCMD_SET_END
);

SHELL_SUBCMD_ADD((net), quic, &net_cmd_quic,
		 "Print information about Quic connections.",
		 cmd_net_quic, 1, 0);
