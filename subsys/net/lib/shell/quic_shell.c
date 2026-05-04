/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_shell);

#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>

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

static uint64_t quic_shell_uptime_ms(void)
{
	int64_t uptime_ms = k_uptime_get();

	return uptime_ms > 0 ? (uint64_t)uptime_ms : 0U;
}

static bool quic_addr_is_unspecified(const struct net_sockaddr_storage *addr)
{
	if (addr == NULL) {
		return true;
	}

	if (addr->ss_family == NET_AF_INET) {
		return net_ipv4_is_addr_unspecified(&net_sin(net_sad(addr))->sin_addr);
	}

	if (addr->ss_family == NET_AF_INET6) {
		return net_ipv6_is_addr_unspecified(&net_sin6(net_sad(addr))->sin6_addr);
	}

	return true;
}

static void quic_resolve_local_addr(const struct net_sockaddr_storage *local,
				    const struct net_sockaddr_storage *remote,
				    struct net_sockaddr_storage *resolved)
{
	if (resolved == NULL) {
		return;
	}

	memset(resolved, 0, sizeof(*resolved));

	if (local == NULL) {
		return;
	}

	memcpy(resolved, local, sizeof(*resolved));

	if (!quic_addr_is_unspecified(local) || remote == NULL) {
		return;
	}

	if (local->ss_family == NET_AF_INET && remote->ss_family == NET_AF_INET) {
		const struct net_in_addr *src = NULL;

		(void)net_if_ipv4_select_src_iface_addr(&net_sin(net_sad(remote))->sin_addr, &src);
		if (src != NULL) {
			net_ipaddr_copy(&net_sin(net_sad(resolved))->sin_addr, src);
		}

		return;
	}

	if (local->ss_family == NET_AF_INET6 && remote->ss_family == NET_AF_INET6) {
		const struct net_in6_addr *src6 = NULL;

		if (net_ipv6_addr_is_v4_mapped(&net_sin6(net_sad(remote))->sin6_addr)) {
			struct net_in_addr dst4;
			const struct net_in_addr *src4 = NULL;
			struct net_in6_addr mapped;

			net_ipv6_addr_get_v4_mapped(&net_sin6(net_sad(remote))->sin6_addr, &dst4);
			(void)net_if_ipv4_select_src_iface_addr(&dst4, &src4);
			if (src4 != NULL) {
				net_ipv6_addr_create_v4_mapped(src4, &mapped);
				net_ipaddr_copy(&net_sin6(net_sad(resolved))->sin6_addr, &mapped);
			}

			return;
		}

		(void)net_if_ipv6_select_src_iface_addr(&net_sin6(net_sad(remote))->sin6_addr,
							&src6);
		if (src6 != NULL) {
			net_ipaddr_copy(&net_sin6(net_sad(resolved))->sin6_addr, src6);
		}
	}
}

static void quic_format_addr(const struct net_sockaddr_storage *addr, char *buf, size_t len)
{
	if (addr == NULL || buf == NULL || len == 0U || addr->ss_family == 0) {
		if (buf != NULL && len > 0U) {
			snprintk(buf, len, "-");
		}

		return;
	}

	if (IS_ENABLED(CONFIG_NET_IPV6) && addr->ss_family == NET_AF_INET6) {
		snprintk(buf, len, "[%s]:%u",
			 net_sprint_ipv6_addr(&net_sin6(net_sad(addr))->sin6_addr),
			 net_ntohs(net_sin6(net_sad(addr))->sin6_port));
		return;
	}

	if (IS_ENABLED(CONFIG_NET_IPV4) && addr->ss_family == NET_AF_INET) {
		snprintk(buf, len, "%s:%u",
			 net_sprint_ipv4_addr(&net_sin(net_sad(addr))->sin_addr),
			 net_ntohs(net_sin(net_sad(addr))->sin_port));
		return;
	}

	snprintk(buf, len, "-");
}

static void print_quic_stats_values(const struct net_stats_quic *stats, const struct shell *sh)
{
	PR("Handshake init RX   : %u\n", stats->handshake_init_rx);
	PR("Handshake init TX   : %u\n", stats->handshake_init_tx);
	PR("Handshake resp RX   : %u\n", stats->handshake_resp_rx);
	PR("Handshake resp TX   : %u\n", stats->handshake_resp_tx);
	PR("Peer not found      : %u\n", stats->peer_not_found);
	PR("Invalid packet      : %u\n", stats->invalid_packet);
	PR("Invalid key         : %u\n", stats->invalid_key);
	PR("Invalid packet len  : %u\n", stats->invalid_packet_len);
	PR("Invalid handshake   : %u\n", stats->invalid_handshake);
	PR("Decrypt failed      : %u\n", stats->decrypt_failed);
	PR("Dropped RX          : %u\n", stats->drop_rx);
	PR("Dropped TX          : %u\n", stats->drop_tx);
	PR("Allocation failed   : %u\n", stats->alloc_failed);
	PR("RX data packets     : %u\n", stats->valid_rx);
	PR("TX data packets     : %u\n", stats->valid_tx);
	PR("\n");
}

static void print_active_quic_stats(struct quic_context *ctx, const struct shell *sh)
{
	char addr_local[ADDR_LEN + 7];
	char addr_remote[ADDR_LEN + 7];
	struct net_sockaddr_storage local_addr;
	uint64_t age_ms = quic_shell_uptime_ms();

	quic_resolve_local_addr(ctx->stats_metadata_valid ? &ctx->stats_local_addr : NULL,
				ctx->stats_metadata_valid ? &ctx->stats_remote_addr : NULL,
				&local_addr);
	quic_format_addr(ctx->stats_metadata_valid ? &local_addr : NULL,
			 addr_local, sizeof(addr_local));
	quic_format_addr(ctx->stats_metadata_valid ? &ctx->stats_remote_addr : NULL,
			 addr_remote, sizeof(addr_remote));

	if (age_ms > ctx->stats_started_at_ms) {
		age_ms -= ctx->stats_started_at_ms;
	} else {
		age_ms = 0U;
	}

	PR("Statistics for Quic context %d (sock %d)\n", ctx->id, ctx->sock);
	PR("Role                : %s\n", ctx->stats_is_server ? "server" : "client");
	PR("Age                 : %llu ms\n", age_ms);
	PR("Local               : %s\n", addr_local);
	PR("Remote              : %s\n", addr_remote);
	PR("Error code          : %d\n", ctx->error_code);
	print_quic_stats_values(&ctx->stats, sh);
}

static void context_stats_cb(struct quic_context *context, void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	int *count = data->user_data;

	if (context->is_listening) {
		return;
	}

	print_active_quic_stats(context, sh);
	(*count)++;
}

#if defined(CONFIG_QUIC_STATS_HISTORY)
static void print_closed_quic_stats(const struct quic_closed_context_stats *stats,
				    const struct shell *sh)
{
	char addr_local[ADDR_LEN + 7];
	char addr_remote[ADDR_LEN + 7];
	struct net_sockaddr_storage local_addr;

	quic_resolve_local_addr(&stats->local_addr, &stats->remote_addr, &local_addr);
	quic_format_addr(&local_addr, addr_local, sizeof(addr_local));
	quic_format_addr(&stats->remote_addr, addr_remote, sizeof(addr_remote));

	PR("Statistics for closed Quic context %d\n", stats->id);
	PR("Role                : %s\n", stats->is_server ? "server" : "client");
	PR("Lifetime            : %llu ms\n", stats->duration_ms);
	PR("Local               : %s\n", addr_local);
	PR("Remote              : %s\n", addr_remote);
	PR("Error code          : %d\n", stats->error_code);
	print_quic_stats_values(&stats->stats, sh);
}

static void closed_context_stats_cb(const struct quic_closed_context_stats *stats, void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	int *count = data->user_data;

	print_closed_quic_stats(stats, sh);
	(*count)++;
}
#endif /* CONFIG_QUIC_STATS_HISTORY */
#endif /* CONFIG_NET_STATISTICS_QUIC && CONFIG_NET_STATISTICS_USER_API */

static int cmd_net_quic_stats(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_STATISTICS_QUIC) && defined(CONFIG_NET_STATISTICS_USER_API)
	struct net_shell_user_data user_data;
	int active_count = 0;
#if defined(CONFIG_QUIC_STATS_HISTORY)
	int closed_count = 0;
#endif

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	user_data.sh = sh;

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

	PR("Active Quic connection statistics\n");

	user_data.user_data = &active_count;
	quic_context_foreach(context_stats_cb, &user_data);

	if (active_count == 0) {
		PR("No active connections\n");
	}

#if defined(CONFIG_QUIC_STATS_HISTORY)
	PR("\nClosed Quic connection statistics\n");

	user_data.user_data = &closed_count;
	quic_closed_context_stats_foreach(closed_context_stats_cb, &user_data);

	if (closed_count == 0) {
		PR("No closed connections\n");
	}
#endif
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
