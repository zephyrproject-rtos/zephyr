/*
 * Copyright (c) 2025 Netfeasa Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/modem/chat.h>
#include <zephyr/modem/backend/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_offload.h>
#include <zephyr/net/offloaded_netdev.h>
#include <zephyr/net/socket_offload.h>
#include <zephyr/posix/fcntl.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS) && defined(CONFIG_MODEM_HL78XX_SOCKETS_SOCKOPT_TLS)
#include "tls_internal.h"
#include <zephyr/net/tls_credentials.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include "hl78xx.h"
#include "hl78xx_chat.h"
#include "hl78xx_cfg.h"

LOG_MODULE_REGISTER(hl78xx_socket, CONFIG_MODEM_LOG_LEVEL);

/*
 * hl78xx_sockets.c
 *
 * Responsibilities:
 *  - Provide the socket offload integration for the HL78xx modem.
 *  - Parse modem URC/chat replies used to transfer payloads over the UART pipe.
 *  - Format and send AT commands for socket lifecycle (create, connect, send, recv,
 *    close, delete) and handle their confirmation/URC callbacks.
 *  - Provide TLS credential handling when enabled.
 */

/* Helper macros and constants */
#define MODEM_STREAM_STARTER_WORD "\r\n" CONNECT_STRING "\r\n"
#define MODEM_STREAM_END_WORD     "\r\n" OK_STRING "\r\n"

#define MODEM_SOCKET_DATA_LEFTOVER_STATE_BIT     (0)
#define HL78XX_UART_PIPE_WORK_SOCKET_BUFFER_SIZE 32
/* modem socket id is 1-based */
#define HL78XX_TCP_STATUS_ID(x)                  ((x > 1 ? (x) - 1 : 0))
/* modem socket id is 1-based */
#define HL78XX_UDP_STATUS_ID(x)                  ((x > 1 ? (x) - 1 : 0))

#define DNS_SERVERS_COUNT                                                                          \
	(0 + (IS_ENABLED(CONFIG_NET_IPV6) ? 1 : 0) + (IS_ENABLED(CONFIG_NET_IPV4) ? 1 : 0) +       \
	 1 /* for NULL terminator */                                                               \
	)
RING_BUF_DECLARE(mdm_recv_pool, CONFIG_MODEM_HL78XX_UART_BUFFER_SIZES);

struct hl78xx_dns_info {
#ifdef CONFIG_NET_IPV4
	char v4_string[NET_IPV4_ADDR_LEN];
	struct in_addr v4;
#endif
#ifdef CONFIG_NET_IPV6
	char v6_string[NET_IPV6_ADDR_LEN];
	struct in6_addr v6;
#endif
	bool ready;
};

/* IPv4 information is optional and only present when IPv4 is enabled */
#ifdef CONFIG_NET_IPV4
struct hl78xx_ipv4_info {
	struct in_addr addr;
	struct in_addr subnet;
	struct in_addr gateway;
	struct in_addr new_addr;
};
#endif
/* IPv6 information is optional and only present when IPv6 is enabled */
#ifdef CONFIG_NET_IPV6
struct hl78xx_ipv6_info {
	struct in6_addr addr;
	struct in6_addr subnet;
	struct in6_addr gateway;
	struct in6_addr new_addr;
};
#endif
/* TLS information is optional and only present when TLS is enabled */
struct hl78xx_tls_info {
	char hostname[MDM_MAX_HOSTNAME_LEN];
	bool hostname_set;
};

enum hl78xx_tcp_socket_status_code {
	/** Error occurred, socket is not usable */
	TCP_SOCKET_ERROR = 0,
	/** Connection is up, socket can be used to send/receive data */
	TCP_SOCKET_CONNECTED,
};

enum hl78xx_udp_socket_status_code {
	UDP_SOCKET_ERROR = 0, /* Error occurred, socket is not usable */
	/** Connection is up, socket can be used to send/receive data */
	UDP_SOCKET_CREATED,
};
struct hl78xx_tcp_status {
	enum hl78xx_tcp_socket_status_code err_code;
	bool is_connected;
	bool is_created;
};
struct hl78xx_udp_status {
	enum hl78xx_udp_socket_status_code err_code;
	bool is_created;
};

struct receive_socket_data {
	char buf[MDM_MAX_DATA_LENGTH + ARRAY_SIZE(MODEM_STREAM_STARTER_WORD) +
		 ARRAY_SIZE(MODEM_STREAM_END_WORD)];
	uint16_t len;
};
struct hl78xx_socket_data {
	struct net_if *net_iface;
	uint8_t mac_addr[6];
	/* socket data */
	struct modem_socket_config socket_config;
	struct modem_socket sockets[MDM_MAX_SOCKETS];
	int current_sock_fd;
	int sizeof_socket_data;
	int requested_socket_id;
	bool socket_data_error;
#if defined(CONFIG_NET_IPV4) || defined(CONFIG_NET_IPV6)
	struct hl78xx_dns_info dns;
#endif
#ifdef CONFIG_NET_IPV4
	struct hl78xx_ipv4_info ipv4;
#endif
#ifdef CONFIG_NET_IPV6
	struct hl78xx_ipv6_info ipv6;
#endif
	/* rx net buffer */
	struct ring_buf *buf_pool;
	uint32_t expected_buf_len;
	uint32_t collected_buf_len;
	struct receive_socket_data receive_buf;
	/* device information */
	const struct device *modem_dev;
	const struct device *offload_dev;
	struct hl78xx_data *mdata_global;
	/* socket state */
	struct hl78xx_tls_info tls;
	struct hl78xx_tcp_status tcp_conn_status[MDM_MAX_SOCKETS];
	struct hl78xx_udp_status udp_conn_status[MDM_MAX_SOCKETS];
	/* per-socket parser state (migrated from globals) - use a small enum to
	 * make the parser's intent explicit and easier to read.
	 */
	enum {
		HL78XX_PARSER_IDLE = 0,
		HL78XX_PARSER_CONNECT_MATCHED,
		HL78XX_PARSER_EOF_OK_MATCHED,
		HL78XX_PARSER_ERROR_MATCHED,
	} parser_state;
	/* transient: prevents further parsing until parser_reset clears it */
	bool parser_match_found;
	uint16_t parser_start_index_eof;
	uint16_t parser_size_of_socketdata;
	/* true once payload has been pushed into ring_buf */
	bool parser_socket_data_received;
	/* set when EOF pattern was found and payload pushed */
	bool parser_eof_detected;
	/* set when OK token was matched after payload */
	bool parser_ok_detected;
};

static struct hl78xx_socket_data *socket_data_global;

/* ===== Utils ==========================================================
 * Small, stateless utility helpers used across this file.
 * Grouping here reduces cognitive load when navigating the file.
 */
static inline void hl78xx_set_socket_global(struct hl78xx_socket_data *d)
{
	socket_data_global = d;
}

static inline struct hl78xx_socket_data *hl78xx_get_socket_global(void)
{
	return socket_data_global;
}

/* Helper: map an internal return code into POSIX errno and set errno.
 * - negative values are assumed to be negative errno semantics -> map to positive
 * - positive values are assumed already POSIX errno -> pass through
 * - zero or unknown -> fallback to EIO
 */
static inline void hl78xx_set_errno_from_code(int code)
{
	if (code < 0) {
		errno = -code;
	} else if (code > 0) {
		errno = code;
	} else {
		errno = EIO;
	}
}
/* ===== Forward declarations ==========================================
 * Group commonly used static helper prototypes here so callers can be
 * reordered without implicit-declaration warnings. Keep this section
 * compact. When moving functions into groups, add any new prototypes
 * here first.
 */
static void check_tcp_state_if_needed(struct hl78xx_socket_data *socket_data,
				      struct modem_socket *sock);
/* Parser helpers */
static bool split_ipv4_and_subnet(const char *combined, char *ip_out, size_t ip_out_len,
				  char *subnet_out, size_t subnet_out_len);
static bool parse_ip(bool is_ipv4, const char *ip_str, void *out_addr);
static bool update_dns(struct hl78xx_socket_data *socket_data, bool is_ipv4, const char *dns_str);
static void set_iface(struct hl78xx_socket_data *socket_data, bool is_ipv4);
static void parser_reset(struct hl78xx_socket_data *socket_data);
static void found_reset(struct hl78xx_socket_data *socket_data);
static bool modem_chat_parse_end_del_start(struct hl78xx_socket_data *socket_data,
					   struct modem_chat *chat);
static bool modem_chat_parse_end_del_complete(struct hl78xx_socket_data *socket_data,
					      struct modem_chat *chat);
static bool modem_chat_match_matches_received(struct hl78xx_socket_data *socket_data,
					      const char *match, uint16_t match_size);

/* Receive / parser entrypoints */
static void socket_process_bytes(struct hl78xx_socket_data *socket_data, char byte);
static int modem_process_handler(struct hl78xx_data *data);
static void modem_pipe_callback(struct modem_pipe *pipe, enum modem_pipe_event event,
				void *user_data);

/* Socket I/O helpers */
static int on_cmd_sockread_common(int socket_id, uint16_t socket_data_length, uint16_t len,
				  void *user_data);
static ssize_t offload_recvfrom(void *obj, void *buf, size_t len, int flags, struct sockaddr *from,
				socklen_t *fromlen);
static int prepare_send_cmd(const struct modem_socket *sock, const struct sockaddr *dst_addr,
			    size_t buf_len, char *cmd_buf, size_t cmd_buf_size);
static int send_data_buffer(struct hl78xx_socket_data *socket_data, const char *buf,
			    const size_t buf_len, int *sock_written);

/* Socket lifecycle */
static int create_socket(struct modem_socket *sock, const struct sockaddr *addr,
			 struct hl78xx_socket_data *data);
static int socket_close(struct hl78xx_socket_data *socket_data, struct modem_socket *sock);
static int socket_delete(struct hl78xx_socket_data *socket_data, struct modem_socket *sock);
static void socket_notify_data(int socket_id, int new_total, void *user_data);
/* ===== TLS prototypes (conditional) ==================================
 * Forward declarations for TLS-related helpers. Grouped separately so
 * TLS-specific code paths are easy to find.
 */
#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS) && defined(CONFIG_MODEM_HL78XX_SOCKETS_SOCKOPT_TLS)
static int map_credentials(struct hl78xx_socket_data *socket_data, const void *optval,
			   socklen_t optlen);
static int hl78xx_configure_chipper_suit(struct hl78xx_socket_data *socket_data);
#endif /* CONFIG_NET_SOCKETS_SOCKOPT_TLS */

/* ===== Container helpers =============================================
 * Small helpers used to map between container structures and their
 * member pointers (eg. `modem_socket` -> `hl78xx_socket_data`).
 */
static inline struct hl78xx_socket_data *hl78xx_socket_data_from_sock(struct modem_socket *sock)
{
	/* Robustly recover the parent `hl78xx_socket_data` for any element
	 * address within the `sockets[]` array. Using CONTAINER_OF with
	 * `sockets[0]` is not safe when `sock` points to `sockets[i]` (i>0),
	 * because CONTAINER_OF assumes the pointer is to the member named
	 * in the macro (sockets[0]). That yields a pointer offset by
	 * i * sizeof(sockets[0]).
	 *
	 * Strategy: for each possible index i, compute the candidate parent
	 * base address so that &candidate->sockets[i] == sock. If the math
	 * yields a candidate that looks like a valid container, return it.
	 */
	if (!sock) {
		return NULL;
	}

	const size_t elem_size = sizeof(((struct hl78xx_socket_data *)0)->sockets[0]);
	const size_t sockets_off = offsetof(struct hl78xx_socket_data, sockets);
	struct hl78xx_socket_data *result = NULL;

	for (int i = 0; i < MDM_MAX_SOCKETS; i++) {
		struct hl78xx_socket_data *candidate =
			(struct hl78xx_socket_data *)((char *)sock -
						      (ptrdiff_t)(sockets_off +
								  (size_t)i * elem_size));
		/* Quick sanity: does candidate->sockets[i] point back to sock? */
		if ((struct modem_socket *)&candidate->sockets[i] != sock) {
			continue;
		}
		if (candidate->offload_dev && candidate->mdata_global) {
			return candidate;
		}

		/* Remember the first match as a fallback */
		if (!result) {
			result = candidate;
		}
	}
	return result;
}

/* ===== Chat callbacks (grouped) =====================================
 * Group all chat/URC handlers together to make the socket TU easier to
 * scan. These handlers are registered via hl78xx_chat getters in
 * `hl78xx_chat.c` and forward URC context into the socket layer.
 */
void hl78xx_on_socknotifydata(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	int socket_id = -1;
	int new_total = -1;

	if (argc < 2) {
		return;
	}

	socket_id = ATOI(argv[1], -1, "socket_id");
	new_total = ATOI(argv[2], -1, "length");
	if (socket_id < 0 || new_total < 0) {
		return;
	}
	HL78XX_LOG_DBG("%d %d %d", __LINE__, socket_id, new_total);
	/* Notify the socket layer that data is available */
	socket_notify_data(socket_id, new_total, user_data);
}

/** +KTCP_NOTIF: <session_id>, <tcp_notif> */
void hl78xx_on_ktcpnotif(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;
	struct hl78xx_socket_data *socket_data =
		(struct hl78xx_socket_data *)data->offload_dev->data;
	enum hl78xx_tcp_notif tcp_notif_received;
	int socket_id = -1;
	int tcp_notif = -1;

	if (!data || !socket_data) {
		LOG_ERR("%s: invalid user_data", __func__);
		return;
	}
	if (argc < 2) {
		return;
	}
	socket_id = ATOI(argv[1], -1, "socket_id");
	tcp_notif = ATOI(argv[2], -1, "tcp_notif");
	if (tcp_notif == -1) {
		return;
	}
	tcp_notif_received = (enum hl78xx_tcp_notif)tcp_notif;
	/* Store the socket id for the notification */
	socket_data->requested_socket_id = socket_id;
	switch (tcp_notif_received) {
	case TCP_NOTIF_REMOTE_DISCONNECTION:
		/**
		 * To Handle remote disconnection
		 * give a dummy packet size of 1
		 *
		 */
		socket_notify_data(socket_id, 1, user_data);
		break;
	case TCP_NOTIF_NETWORK_ERROR:
		/* Handle network error */
		break;
	default:
		break;
	}
}

void hl78xx_on_ktcpind(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;
	struct hl78xx_socket_data *socket_data =
		(struct hl78xx_socket_data *)data->offload_dev->data;
	struct modem_socket *sock = NULL;
	int socket_id = -1;
	int tcp_conn_stat = -1;

	if (!data || !socket_data) {
		LOG_ERR("%s: invalid user_data", __func__);
		return;
	}
	if (argc < 3 || !argv[1] || !argv[2]) {
		LOG_ERR("TCP_IND: Incomplete response");
		goto exit;
	}
	socket_id = ATOI(argv[1], -1, "socket_id");
	if (socket_id == -1) {
		goto exit;
	}
	sock = modem_socket_from_id(&socket_data->socket_config, socket_id);
	tcp_conn_stat = ATOI(argv[2], -1, "tcp_status");
	if (tcp_conn_stat == TCP_SOCKET_CONNECTED) {
		socket_data->tcp_conn_status[HL78XX_TCP_STATUS_ID(socket_id)].err_code =
			tcp_conn_stat;
		socket_data->tcp_conn_status[HL78XX_TCP_STATUS_ID(socket_id)].is_connected = true;
		return;
	}
exit:
	socket_data->tcp_conn_status[HL78XX_TCP_STATUS_ID(socket_id)].err_code = tcp_conn_stat;
	socket_data->tcp_conn_status[HL78XX_TCP_STATUS_ID(socket_id)].is_connected = false;
	if (socket_id != -1) {
		modem_socket_put(&socket_data->socket_config, sock->sock_fd);
	}
}

/* Chat/URC handler for socket-create/indication responses
 * Matches +KTCPCFG: <id>
 */
void hl78xx_on_ktcpsocket_create(struct modem_chat *chat, char **argv, uint16_t argc,
				 void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;
	struct hl78xx_socket_data *socket_data =
		(struct hl78xx_socket_data *)data->offload_dev->data;
	struct modem_socket *sock = NULL;
	int socket_id = -1;

	if (!data || !socket_data) {
		LOG_ERR("%s: invalid user_data", __func__);
		return;
	}
	if (argc < 2 || !argv[1]) {
		LOG_ERR("%s: Incomplete response", __func__);
		goto exit;
	}
	/* argv[0] may contain extra CSV fields; parse leading integer */
	socket_id = ATOI(argv[1], -1, "socket_id");
	if (socket_id <= 0) {
		LOG_DBG("unable to parse socket id from '%s'", argv[1]);
		goto exit;
	}
	/* Try to find a reserved/new socket slot and assign the modem-provided id. */
	sock = modem_socket_from_newid(&socket_data->socket_config);
	if (!sock) {
		goto exit;
	}

	if (modem_socket_id_assign(&socket_data->socket_config, sock, socket_id) < 0) {
		LOG_ERR("Failed to assign modem socket id %d to fd %d", socket_id, sock->sock_fd);
		goto exit;
	} else {
		LOG_DBG("Assigned modem socket id %d to fd %d", socket_id, sock->sock_fd);
	}

	socket_data->tcp_conn_status[HL78XX_TCP_STATUS_ID(socket_id)].is_created = true;
	return;

exit:
	socket_data->tcp_conn_status[HL78XX_TCP_STATUS_ID(socket_id)].err_code = TCP_SOCKET_ERROR;
	socket_data->tcp_conn_status[HL78XX_TCP_STATUS_ID(socket_id)].is_created = false;
	if (socket_id != -1 && sock) {
		modem_socket_put(&socket_data->socket_config, sock->sock_fd);
	}
}
/* Chat/URC handler for socket-create/indication responses
 * Matches +KUDPCFG: <id>
 *         +KUDP_IND: <id>,... (or +KTCP_IND)
 */
void hl78xx_on_kudpsocket_create(struct modem_chat *chat, char **argv, uint16_t argc,
				 void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;
	struct hl78xx_socket_data *socket_data =
		(struct hl78xx_socket_data *)data->offload_dev->data;
	struct modem_socket *sock = NULL;
	int socket_id = -1;
	int udp_create_stat = -1;

	if (!data || !socket_data) {
		LOG_ERR("%s: invalid user_data", __func__);
		return;
	}
	if (argc < 2 || !argv[1]) {
		LOG_ERR("%s: Incomplete response", __func__);
		goto exit;
	}
	/* argv[0] may contain extra CSV fields; parse leading integer */
	socket_id = ATOI(argv[1], -1, "socket_id");
	if (socket_id <= 0) {
		LOG_DBG("unable to parse socket id from '%s'", argv[1]);
		goto exit;
	}
	/* Try to find a reserved/new socket slot and assign the modem-provided id. */
	sock = modem_socket_from_newid(&socket_data->socket_config);
	if (!sock) {
		goto exit;
	}

	if (modem_socket_id_assign(&socket_data->socket_config, sock, socket_id) < 0) {
		LOG_ERR("Failed to assign modem socket id %d to fd %d", socket_id, sock->sock_fd);
		goto exit;
	} else {
		LOG_DBG("Assigned modem socket id %d to fd %d", socket_id, sock->sock_fd);
	}
	/* Parse connection status: 1=created, otherwise=error */
	udp_create_stat = ATOI(argv[2], 0, "udp_status");
	if (udp_create_stat == UDP_SOCKET_CREATED) {
		socket_data->udp_conn_status[HL78XX_UDP_STATUS_ID(socket_id)].err_code =
			udp_create_stat;
		socket_data->udp_conn_status[HL78XX_UDP_STATUS_ID(socket_id)].is_created = true;
		return;
	}
exit:
	socket_data->udp_conn_status[HL78XX_UDP_STATUS_ID(socket_id)].err_code = UDP_SOCKET_ERROR;
	socket_data->udp_conn_status[HL78XX_UDP_STATUS_ID(socket_id)].is_created = false;
	if (socket_id != -1 && sock) {
		modem_socket_put(&socket_data->socket_config, sock->sock_fd);
	}
}

#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
#ifdef CONFIG_MODEM_HL78XX_12
/**
 * @brief Handle modem state update from +KSTATE URC of RAT Scan Finish.
 * This command is intended to report events for different important state transitions and system
 * occurrences.
 * Actually this eventc'state is really important functionality to understand networks
 * searching phase of the modem.
 * Verbose debug logging for KSTATEV events
 */
void hl78xx_on_kstatev_parser(struct hl78xx_data *data, int state, int rat_mode)
{
	switch (state) {
	case EVENT_START_SCAN:
		break;
	case EVENT_FAIL_SCAN:
		LOG_DBG("Modem failed to find a suitable network");
		break;
	case EVENT_ENTER_CAMPED:
		LOG_DBG("Modem entered camped state on a suitable or acceptable cell");
		break;
	case EVENT_CONNECTION_ESTABLISHMENT:
		LOG_DBG("Modem successfully established a connection to the network");
		break;
	case EVENT_START_RESCAN:
		LOG_DBG("Modem is starting a rescan for available networks");
		break;
	case EVENT_RRC_CONNECTED:
		LOG_DBG("Modem has established an RRC connection with the network");
		break;
	case EVENT_NO_SUITABLE_CELLS:
		LOG_DBG("Modem did not find any suitable cells during the scan");
		break;
	case EVENT_ALL_REGISTRATION_FAILED:
		LOG_DBG("Modem failed to register to any network");
		break;
	default:
		LOG_DBG("Unhandled KSTATEV for state %d", state);
		break;
	}
}
#endif
/**
 * @brief This function doesn't handle incoming UDP data.
 * It is just a placeholder for verbose debug logging of incoming UDP data.
 * +KUDP_RCV: <remote_addr>,<remote_port>,
 */
void hl78xx_on_udprcv(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	if (argc < 2) {
		return;
	}
	HL78XX_LOG_DBG("%d %d [%s] [%s] [%s]", __LINE__, argc, argv[0], argv[1], argv[2]);
}
#endif
/* Handler for +CGCONTRDP: <cid>,<bearer>,<apn>,<addr>,<dcomp>,<hcomp>,<dns1>[,<dns2>]
 * This function is invoked by the chat layer when a CGCONTRDP URC is matched.
 * It extracts the PDP context address, gateway and DNS servers and updates the
 * per-instance socket_data DNS fields so dns_work_cb() can apply them.
 */
void hl78xx_on_cgdcontrdp(struct modem_chat *chat, char **argv, uint16_t argc, void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;
	struct hl78xx_socket_data *socket_data =
		(struct hl78xx_socket_data *)data->offload_dev->data;
	const char *addr_field = NULL;
	const char *gw_field = NULL;
	const char *dns_field = NULL;
	const char *apn_field = NULL;

	/* Accept both comma-split argv[] or a single raw token that needs tokenizing */
	if (argc >= 7) {
		apn_field = argv[3];
		addr_field = argv[4];
		gw_field = argv[5];
		dns_field = argv[6];
	} else {
		LOG_ERR("Incomplete CGCONTRDP response: argc=%d", argc);
		return;
	}

	LOG_INF("Apn=%s", apn_field);
	LOG_INF("Addr=%s", addr_field);
	LOG_INF("Gw=%s", gw_field);
	LOG_INF("DNS=%s", dns_field);
#ifdef CONFIG_MODEM_HL78XX_APN_SOURCE_NETWORK
	if (apn_field) {
		hl78xx_extract_essential_part_apn(apn_field, data->identity.apn,
						  sizeof(data->identity.apn));
	}
#endif
	/* Handle address parsing: IPv4 replies sometimes embed subnet as extra
	 * octets concatenated after the IP (e.g. "10.149.122.90.255.255.255.252").
	 * Split and parse into the instance IPv4 fields so the interface can be
	 * configured before the DNS resolver is invoked.
	 */
#ifdef CONFIG_NET_IPV4
	if (addr_field && strchr(addr_field, '.') && !strchr(addr_field, ':')) {
		char ip_addr[NET_IPV6_ADDR_LEN] = {0};
		char subnet_mask[NET_IPV6_ADDR_LEN] = {0};

		if (!split_ipv4_and_subnet(addr_field, ip_addr, sizeof(ip_addr), subnet_mask,
					   sizeof(subnet_mask))) {
			LOG_ERR("CGCONTRDP: failed to split IPv4+subnet: %s", addr_field);
			return;
		}
		if (!parse_ip(true, ip_addr, &socket_data->ipv4.new_addr)) {
			return;
		}
		if (!parse_ip(true, subnet_mask, &socket_data->ipv4.subnet)) {
			return;
		}
		if (gw_field && !parse_ip(true, gw_field, &socket_data->ipv4.gateway)) {
			return;
		}
	}
#else
	ARG_UNUSED(gw_field);
#endif

#ifdef CONFIG_NET_IPV6
	if (addr_field && strchr(addr_field, ':') &&
	    !parse_ip(false, addr_field, &socket_data->ipv6.new_addr)) {
		return;
	}
#endif
	/* Update DNS and configure interface */
	if (!update_dns(socket_data,
#ifdef CONFIG_NET_IPV4
			(addr_field && strchr(addr_field, '.') && !strchr(addr_field, ':')),
#else
			false,
#endif
			dns_field ? dns_field : "")) {
		return;
	}
	/* Configure the interface addresses so net_if_is_up()/address selection
	 * will succeed before attempting to reconfigure the resolver.
	 */
#ifdef CONFIG_NET_IPV4
	set_iface(socket_data, (addr_field && strchr(addr_field, '.') && !strchr(addr_field, ':')));
#elif defined(CONFIG_NET_IPV6)
	set_iface(socket_data, false);
#endif

	socket_data->dns.ready = false;
	LOG_DBG("CGCONTRDP processed, dns strings: v4=%s v6=%s",
#ifdef CONFIG_NET_IPV4
		socket_data->dns.v4_string,
#else
		"<no-v4>",
#endif
#ifdef CONFIG_NET_IPV6
		socket_data->dns.v6_string
#else
		"<no-v6>"
#endif
	);
}
/* ===== Network / Parsing Utilities ===================================
 * Helpers that operate on IP address parsing and DNS/address helpers.
 */
static bool parse_ip(bool is_ipv4, const char *ip_str, void *out_addr)
{
	int ret = net_addr_pton(is_ipv4 ? AF_INET : AF_INET6, ip_str, out_addr);

	LOG_DBG("Parsing %s address: %s -> %s", is_ipv4 ? "IPv4" : "IPv6", ip_str,
		(ret < 0) ? "FAIL" : "OK");
	if (ret < 0) {
		LOG_ERR("Invalid IP address: %s", ip_str);
		return false;
	}
	return true;
}

static bool update_dns(struct hl78xx_socket_data *socket_data, bool is_ipv4, const char *dns_str)
{
	int ret;

	/* ===== Interface helpers ==============================================
	 * Helpers that configure the network interface for IPv4/IPv6.
	 */
	LOG_DBG("Updating DNS (%s): %s", is_ipv4 ? "IPv4" : "IPv6", dns_str);
#ifdef CONFIG_NET_IPV4
	if (is_ipv4) {
		ret = strncmp(dns_str, socket_data->dns.v4_string, strlen(dns_str));
		if (ret != 0) {
			LOG_DBG("New IPv4 DNS differs from current, marking dns_ready = false");
			socket_data->dns.ready = false;
		}
		strncpy(socket_data->dns.v4_string, dns_str, sizeof(socket_data->dns.v4_string));
		socket_data->dns.v4_string[sizeof(socket_data->dns.v4_string) - 1] = '\0';
		return parse_ip(true, socket_data->dns.v4_string, &socket_data->dns.v4);
	}
#else
	if (is_ipv4) {
		LOG_DBG("IPv4 DNS reported but IPv4 disabled in build; ignoring");
		return false;
	}
#endif /* CONFIG_NET_IPV4 */
#ifdef CONFIG_NET_IPV6
	else {
		ret = strncmp(dns_str, socket_data->dns.v6_string, strlen(dns_str));
		if (ret != 0) {
			LOG_DBG("New IPv6 DNS differs from current, marking dns_ready = false");
			socket_data->dns.ready = false;
		}
		strncpy(socket_data->dns.v6_string, dns_str, sizeof(socket_data->dns.v6_string));
		socket_data->dns.v6_string[sizeof(socket_data->dns.v6_string) - 1] = '\0';

		if (!parse_ip(false, socket_data->dns.v6_string, &socket_data->dns.v6)) {
			return false;
		}

		net_addr_ntop(AF_INET6, &socket_data->dns.v6, socket_data->dns.v6_string,
			      sizeof(socket_data->dns.v6_string));
		LOG_DBG("Parsed IPv6 DNS: %s", socket_data->dns.v6_string);
	}
#endif /* CONFIG_NET_IPV6 */
	return true;
}

static void set_iface(struct hl78xx_socket_data *socket_data, bool is_ipv4)
{
	if (!socket_data->net_iface) {
		LOG_DBG("No network interface set. Skipping iface config.");
		return;
	}
	LOG_DBG("Setting %s interface address...", is_ipv4 ? "IPv4" : "IPv6");
	if (is_ipv4) {
#ifdef CONFIG_NET_IPV4
		if (socket_data->ipv4.addr.s_addr != 0) {
			net_if_ipv4_addr_rm(socket_data->net_iface, &socket_data->ipv4.addr);
		}
		/* Use MANUAL so the stack treats this as a configured address and it is
		 * available for source address selection immediately.
		 */
		if (!net_if_ipv4_addr_add(socket_data->net_iface, &socket_data->ipv4.new_addr,
					  NET_ADDR_MANUAL, 0)) {
			LOG_ERR("Failed to set IPv4 interface address.");
		}

		net_if_ipv4_set_netmask_by_addr(socket_data->net_iface, &socket_data->ipv4.new_addr,
						&socket_data->ipv4.subnet);
		net_if_ipv4_set_gw(socket_data->net_iface, &socket_data->ipv4.gateway);

		net_ipaddr_copy(&socket_data->ipv4.addr, &socket_data->ipv4.new_addr);
		LOG_DBG("IPv4 interface configuration complete.");

		(void)net_if_up(socket_data->net_iface);
#else
		LOG_DBG("IPv4 disabled: skipping IPv4 interface configuration");
#endif /* CONFIG_NET_IPV4 */
	}
#ifdef CONFIG_NET_IPV6
	else {
		net_if_ipv6_addr_rm(socket_data->net_iface, &socket_data->ipv6.addr);

		if (!net_if_ipv6_addr_add(socket_data->net_iface, &socket_data->ipv6.new_addr,
					  NET_ADDR_MANUAL, 0)) {
			LOG_ERR("Failed to set IPv6 interface address.");
		} else {
			LOG_DBG("IPv6 interface configuration complete.");
		}
		/* Ensure iface up after adding address */
		(void)net_if_up(socket_data->net_iface);
	}
#endif /* CONFIG_NET_IPV6 */
}

static bool split_ipv4_and_subnet(const char *combined, char *ip_out, size_t ip_out_len,
				  char *subnet_out, size_t subnet_out_len)
{
	int dot_count = 0;
	const char *ptr = combined;
	const char *split = NULL;
	size_t ip_len = 0;

	while (*ptr && dot_count < 4) {
		if (*ptr == '.') {
			dot_count++;
			if (dot_count == 4) {
				split = ptr;
				break;
			}
		}
		ptr++;
	}
	if (!split) {
		LOG_ERR("Invalid IPv4 + subnet format: %s", combined);
		return false;
	}

	ip_len = split - combined;
	if (ip_len >= ip_out_len) {
		ip_len = ip_out_len - 1;
	}
	strncpy(ip_out, combined, ip_len);
	ip_out[ip_len] = '\0';
	strncpy(subnet_out, split + 1, subnet_out_len);
	subnet_out[subnet_out_len - 1] = '\0';
	LOG_DBG("Extracted IP: %s, Subnet: %s", ip_out, subnet_out);
	return true;
}

/* ===== Validation ====================================================
 * Small validation helpers used by send/recv paths.
 */
static int validate_socket(const struct modem_socket *sock, struct hl78xx_socket_data *socket_data)
{
	if (!sock) {
		errno = EINVAL;
		return -1;
	}

	bool not_connected = (!sock->is_connected && sock->type != SOCK_DGRAM);
	bool tcp_disconnected =
		(sock->type == SOCK_STREAM &&
		 !socket_data->tcp_conn_status[HL78XX_TCP_STATUS_ID(sock->id)].is_connected);
	bool udp_not_created =
		(sock->type == SOCK_DGRAM &&
		 !socket_data->udp_conn_status[HL78XX_UDP_STATUS_ID(sock->id)].is_created);

	if (not_connected || tcp_disconnected || udp_not_created) {
		errno = ENOTCONN;
		return -1;
	}

	return 0;
}

/* ===== Parser helpers ================================================
 * Helpers that implement the streaming parser for incoming socket payloads
 * and chat end-delimiter/EOF matching logic.
 */
static void parser_reset(struct hl78xx_socket_data *socket_data)
{
	memset(&socket_data->receive_buf, 0, sizeof(socket_data->receive_buf));
	socket_data->parser_match_found = false;
}

static void found_reset(struct hl78xx_socket_data *socket_data)
{
	if (!socket_data) {
		return;
	}
	/* Clear all parser progress state so a new transfer can start cleanly. */
	socket_data->parser_state = HL78XX_PARSER_IDLE;
	socket_data->parser_match_found = false;
	socket_data->parser_socket_data_received = false;
	socket_data->parser_eof_detected = false;
	socket_data->parser_ok_detected = false;
}

static bool modem_chat_parse_end_del_start(struct hl78xx_socket_data *socket_data,
					   struct modem_chat *chat)
{
	if (socket_data->receive_buf.len == 0) {
		return false;
	}
	/* If the last received byte matches any of the delimiter bytes, we are
	 * starting the end-delimiter sequence. Use memchr to avoid an explicit
	 * loop and to be clearer about intent.
	 */
	return memchr(chat->delimiter,
		      socket_data->receive_buf.buf[socket_data->receive_buf.len - 1],
		      chat->delimiter_size) != NULL;
}

static bool modem_chat_parse_end_del_complete(struct hl78xx_socket_data *socket_data,
					      struct modem_chat *chat)
{
	if (socket_data->receive_buf.len < chat->delimiter_size) {
		return false;
	}

	return memcmp(&socket_data->receive_buf
			       .buf[socket_data->receive_buf.len - chat->delimiter_size],
		      chat->delimiter, chat->delimiter_size) == 0;
}

static bool modem_chat_match_matches_received(struct hl78xx_socket_data *socket_data,
					      const char *match, uint16_t match_size)
{
	if (socket_data->receive_buf.len < match_size) {
		return false;
	}
	return memcmp(socket_data->receive_buf.buf, match, match_size) == 0;
}

static bool is_receive_buffer_full(struct hl78xx_socket_data *socket_data)
{
	return socket_data->receive_buf.len >= ARRAY_SIZE(socket_data->receive_buf.buf);
}

static void handle_expected_length_decrement(struct hl78xx_socket_data *socket_data)
{
	/* Decrement expected length if CONNECT matched and expected length > 0 */
	if (socket_data->parser_state == HL78XX_PARSER_CONNECT_MATCHED &&
	    socket_data->expected_buf_len > 0) {
		socket_data->expected_buf_len--;
	}
}

static bool is_end_delimiter_only(struct hl78xx_socket_data *socket_data)
{
	return socket_data->receive_buf.len == socket_data->mdata_global->chat.delimiter_size;
}

static bool is_valid_eof_index(struct hl78xx_socket_data *socket_data, uint8_t size_match)
{
	socket_data->parser_start_index_eof = socket_data->receive_buf.len - size_match - 2;
	return socket_data->parser_start_index_eof < ARRAY_SIZE(socket_data->receive_buf.buf);
}

/* Handle EOF pattern: if EOF_PATTERN is found at the expected location,
 * push socket payload (excluding EOF marker) into the ring buffer.
 * Returns number of bytes pushed on success, 0 otherwise.
 */
static int handle_eof_pattern(struct hl78xx_socket_data *socket_data)
{
	uint8_t size_match = strlen(EOF_PATTERN);

	if (socket_data->receive_buf.len < size_match + 2) {
		return 0;
	}
	if (!is_valid_eof_index(socket_data, size_match)) {
		return 0;
	}
	if (strncmp(&socket_data->receive_buf.buf[socket_data->parser_start_index_eof], EOF_PATTERN,
		    size_match) == 0) {
		int ret = ring_buf_put(socket_data->buf_pool, socket_data->receive_buf.buf,
				       socket_data->parser_start_index_eof);

		if (ret <= 0) {
			LOG_ERR("ring_buf_put failed: %d", ret);
			return 0;
		}

		/* Mark that payload was successfully pushed and EOF was detected */
		socket_data->parser_socket_data_received = true;
		socket_data->parser_eof_detected = true;
		LOG_DBG("pushed %d bytes to ring_buf; "
			"collected_buf_len(before)=%u",
			ret, socket_data->collected_buf_len);
		socket_data->collected_buf_len += ret;
		LOG_DBG("parser_socket_data_received=1 "
			"collected_buf_len(after)=%u",
			socket_data->collected_buf_len);
		return ret;
	}
	return 0;
}

/* Helper: centralize handling when the chat end-delimiter has been fully
 * received. Returns true if caller should return immediately after handling.
 */
static bool handle_delimiter_complete(struct hl78xx_socket_data *socket_data,
				      struct modem_chat *chat)
{
	if (!modem_chat_parse_end_del_complete(socket_data, chat)) {
		return false;
	}

	if (is_end_delimiter_only(socket_data)) {
		parser_reset(socket_data);
		return true;
	}

	socket_data->parser_size_of_socketdata = socket_data->receive_buf.len;
	if (socket_data->parser_state == HL78XX_PARSER_CONNECT_MATCHED &&
	    socket_data->parser_state != HL78XX_PARSER_EOF_OK_MATCHED) {
		size_t connect_len = strlen(CONNECT_STRING);
		size_t connect_plus_delim = connect_len + chat->delimiter_size;

		/* Case 1: Drop the initial "CONNECT" line including its CRLF */
		if (socket_data->receive_buf.len == connect_plus_delim &&
		    modem_chat_match_matches_received(socket_data, CONNECT_STRING,
						      (uint16_t)connect_len)) {
			parser_reset(socket_data);
			return true;
		}

		/* Case 2: Try to handle EOF; only reset if EOF was actually found/pushed */
		if (handle_eof_pattern(socket_data) > 0) {
			parser_reset(socket_data);
			return true;
		}

		/* Not the initial CONNECT+CRLF and no EOF yet -> keep accumulating */
		return false;
	}

	/* For other states, treat CRLF as end-of-line and reset as before */
	parser_reset(socket_data);
	return true;
}

/* Convenience helper for matching an exact string against the receive buffer.
 * This consolidates the repeated pattern of checking length and content.
 */
static inline bool modem_chat_match_exact(struct hl78xx_socket_data *socket_data, const char *match)
{
	size_t size_match = strlen(match);

	if (socket_data->receive_buf.len != size_match) {
		return false;
	}
	return modem_chat_match_matches_received(socket_data, match, (uint16_t)size_match);
}

static void socket_process_bytes(struct hl78xx_socket_data *socket_data, char byte)
{
	const size_t cme_size = strlen(CME_ERROR_STRING);

	if (is_receive_buffer_full(socket_data)) {
		LOG_WRN("Receive buffer overrun");
		parser_reset(socket_data);
		return;
	}
	socket_data->receive_buf.buf[socket_data->receive_buf.len++] = byte;
	handle_expected_length_decrement(socket_data);
	if (handle_delimiter_complete(socket_data, &socket_data->mdata_global->chat)) {
		return;
	}
	if (modem_chat_parse_end_del_start(socket_data, &socket_data->mdata_global->chat)) {
		return;
	}
	if (socket_data->parser_state != HL78XX_PARSER_ERROR_MATCHED &&
	    socket_data->parser_state != HL78XX_PARSER_CONNECT_MATCHED) {
		/* Exact CONNECT match: length must equal CONNECT string length */
		if (modem_chat_match_exact(socket_data, CONNECT_STRING)) {
			socket_data->parser_state = HL78XX_PARSER_CONNECT_MATCHED;
			LOG_DBG("CONNECT matched. Expecting %d more bytes.",
				socket_data->expected_buf_len);
			return;
		}
		/* Partial CME ERROR match: length must be at least CME string length */
		if (socket_data->receive_buf.len >= cme_size &&
		    modem_chat_match_matches_received(socket_data, CME_ERROR_STRING,
						      (uint16_t)cme_size)) {
			socket_data->parser_state =
				HL78XX_PARSER_ERROR_MATCHED; /* prevent further parsing */
			LOG_ERR("CME ERROR received. Connection failed.");
			socket_data->expected_buf_len = 0;
			socket_data->collected_buf_len = 0;
			parser_reset(socket_data);
			socket_data->socket_data_error = true;
			k_sem_give(&socket_data->mdata_global->script_stopped_sem_rx_int);
			return;
		}
	}
	if (socket_data->parser_state == HL78XX_PARSER_CONNECT_MATCHED &&
	    socket_data->parser_state != HL78XX_PARSER_EOF_OK_MATCHED &&
	    modem_chat_match_exact(socket_data, OK_STRING)) {
		socket_data->parser_state = HL78XX_PARSER_EOF_OK_MATCHED;
		/* Mark that OK was observed. Payload may have already been pushed by EOF handler.
		 */
		socket_data->parser_ok_detected = true;
		LOG_DBG("OK matched. parser_ok_detected=%d parser_socket_data_received=%d "
			"collected=%u",
			socket_data->parser_ok_detected, socket_data->parser_socket_data_received,
			socket_data->collected_buf_len);
	}
}

/* ===== Modem pipe handlers ===========================================
 * Handlers and callbacks for modem pipe events (receive/transmit).
 */
static int modem_process_handler(struct hl78xx_data *data)
{
	struct hl78xx_socket_data *socket_data =
		(struct hl78xx_socket_data *)data->offload_dev->data;
	char work_buf_local[HL78XX_UART_PIPE_WORK_SOCKET_BUFFER_SIZE] = {0};
	int recv_len = 0;
	int work_len = 0;
	/* If no more data is expected, set leftover state and return */
	if (socket_data->expected_buf_len == 0) {
		LOG_DBG("No more data expected");
		atomic_set_bit(&socket_data->mdata_global->state_leftover,
			       MODEM_SOCKET_DATA_LEFTOVER_STATE_BIT);
		return 0;
	}

	/* Use a small stack buffer for the pipe read to avoid TU-global BSS */
	work_len = MIN(sizeof(work_buf_local), socket_data->expected_buf_len);
	recv_len =
		modem_pipe_receive(socket_data->mdata_global->uart_pipe, work_buf_local, work_len);
	if (recv_len <= 0) {
		return recv_len;
	}

#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_HEXDUMP_DBG(work_buf_local, recv_len, "Received bytes:");
#endif /* CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG */
	for (int i = 0; i < recv_len; i++) {
		socket_process_bytes(socket_data, work_buf_local[i]);
	}

	LOG_DBG("post-process state=%d recv_len=%d recv_buf.len=%u "
		"expected=%u collected=%u socket_data_received=%d",
		socket_data->parser_state, recv_len, socket_data->receive_buf.len,
		socket_data->expected_buf_len, socket_data->collected_buf_len,
		socket_data->parser_socket_data_received);

	/* Check if we've completed reception */
	if (socket_data->parser_eof_detected && socket_data->parser_ok_detected &&
	    socket_data->parser_socket_data_received) {
		LOG_DBG("All data received: %d bytes", socket_data->parser_size_of_socketdata);
		socket_data->expected_buf_len = 0;
		LOG_DBG("About to give RX semaphore (eof=%d ok=%d socket_data_received=%d "
			"collected=%u)",
			socket_data->parser_eof_detected, socket_data->parser_ok_detected,
			socket_data->parser_socket_data_received, socket_data->collected_buf_len);
		k_sem_give(&socket_data->mdata_global->script_stopped_sem_rx_int);
		/* Clear parser progress after the receiver has been notified */
		found_reset(socket_data);
	}
	return 0;
}

static void modem_pipe_callback(struct modem_pipe *pipe, enum modem_pipe_event event,
				void *user_data)
{
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;

	switch (event) {
	case MODEM_PIPE_EVENT_RECEIVE_READY:
		(void)modem_process_handler(data);
		break;

	case MODEM_PIPE_EVENT_TRANSMIT_IDLE:
		k_sem_give(&data->script_stopped_sem_tx_int);
		break;

	default:
		LOG_DBG("Unhandled event: %d", event);
		break;
	}
}

void notif_carrier_off(const struct device *dev)
{
	struct hl78xx_data *data = dev->data;
	struct hl78xx_socket_data *socket_data =
		(struct hl78xx_socket_data *)data->offload_dev->data;

	net_if_carrier_off(socket_data->net_iface);
}

void notif_carrier_on(const struct device *dev)
{
	struct hl78xx_data *data = dev->data;
	struct hl78xx_socket_data *socket_data =
		(struct hl78xx_socket_data *)data->offload_dev->data;

	net_if_carrier_on(socket_data->net_iface);
}

void iface_status_work_cb(struct hl78xx_data *data, modem_chat_script_callback script_user_callback)
{

	const char *cmd = "AT+CGCONTRDP=1";
	int ret = 0;

	ret = modem_dynamic_cmd_send(data, script_user_callback, cmd, strlen(cmd),
				     hl78xx_get_cgdcontrdp_match(), 1, false);
	if (ret < 0) {
		LOG_ERR("Failed to send AT+CGCONTRDP command: %d", ret);
		return;
	}
}

void dns_work_cb(const struct device *dev, bool hard_reset)
{
#if defined(CONFIG_DNS_RESOLVER) && !defined(CONFIG_DNS_SERVER_IP_ADDRESSES)
	int ret;
	struct hl78xx_data *data = dev->data;
	struct hl78xx_socket_data *socket_data =
		(struct hl78xx_socket_data *)data->offload_dev->data;
	struct dns_resolve_context *dnsCtx;
	struct sockaddr temp_addr;
	bool valid_address = false;
	bool retry = false;
	const char *const dns_servers_str[DNS_SERVERS_COUNT] = {
#ifdef CONFIG_NET_IPV6
		socket_data->dns.v6_string,
#endif
#ifdef CONFIG_NET_IPV4
		socket_data->dns.v4_string,
#endif
		NULL};
	const char *dns_servers_wrapped[ARRAY_SIZE(dns_servers_str)];

	if (hard_reset) {
		LOG_DBG("Resetting DNS resolver");
		dnsCtx = dns_resolve_get_default();
		if (!dnsCtx) {
			LOG_WRN("No default DNS resolver context available; skipping "
				"reconfigure");
			socket_data->dns.ready = true;
			return;
		}
		if (dnsCtx->state != DNS_RESOLVE_CONTEXT_INACTIVE) {
			dns_resolve_close(dnsCtx);
		}
		socket_data->dns.ready = false;
	}

#ifdef CONFIG_NET_IPV6
	valid_address = net_ipaddr_parse(socket_data->dns.v6_string,
					 strlen(socket_data->dns.v6_string), &temp_addr);
	if (!valid_address && IS_ENABLED(CONFIG_NET_IPV4)) {
		/* IPv6 DNS string is not valid, replace it with IPv4 address and recheck */
#ifdef CONFIG_NET_IPV4
		strncpy(socket_data->dns.v6_string, socket_data->dns.v4_string,
			sizeof(socket_data->dns.v4_string) - 1);
		valid_address = net_ipaddr_parse(socket_data->dns.v6_string,
						 strlen(socket_data->dns.v6_string), &temp_addr);
#endif
	}
#elif defined(CONFIG_NET_IPV4)
	valid_address = net_ipaddr_parse(socket_data->dns.v4_string,
					 strlen(socket_data->dns.v4_string), &temp_addr);
#else
	/* No IP stack configured */
	valid_address = false;
#endif
	if (!valid_address) {
		LOG_WRN("No valid DNS address!");
		return;
	}
	if (!socket_data->net_iface || !net_if_is_up(socket_data->net_iface) ||
	    socket_data->dns.ready) {
		LOG_DBG("DNS already ready or net_iface problem %d %d %d", !socket_data->net_iface,
			!net_if_is_up(socket_data->net_iface), socket_data->dns.ready);
		return;
	}
	memcpy(dns_servers_wrapped, dns_servers_str, sizeof(dns_servers_wrapped));
	/* set new DNS addr in DNS resolver */
	LOG_DBG("Refresh DNS resolver");
	dnsCtx = dns_resolve_get_default();
	ret = dns_resolve_reconfigure(dnsCtx, dns_servers_wrapped, NULL, DNS_SOURCE_MANUAL);
	if (ret < 0) {
		LOG_ERR("dns_resolve_reconfigure fail (%d)", ret);
		retry = true;
	} else {
		LOG_DBG("DNS ready");
		socket_data->dns.ready = true;
	}
	if (retry) {
		LOG_WRN("DNS not ready, scheduling a retry");
	}
#endif
}

static int on_cmd_sockread_common(int socket_id, uint16_t socket_data_length, uint16_t len,
				  void *user_data)
{
	struct modem_socket *sock;
	struct socket_read_data *sock_data;
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;
	struct hl78xx_socket_data *socket_data =
		(struct hl78xx_socket_data *)data->offload_dev->data;
	int ret = 0;

	sock = modem_socket_from_fd(&socket_data->socket_config, socket_id);
	if (!sock) {
		LOG_ERR("Socket not found! (%d)", socket_id);
		return -EINVAL;
	}
	sock_data = sock->data;
	if (!sock_data) {
		LOG_ERR("Socket data missing! Ignoring (%d)", socket_id);
		return -EINVAL;
	}
	if (socket_data->socket_data_error && socket_data->collected_buf_len == 0) {
		errno = ECONNABORTED;
		return -ECONNABORTED;
	}
	if ((len <= 0) || socket_data_length <= 0 || socket_data->collected_buf_len < (size_t)len) {
		LOG_ERR("%d Invalid data length: %d %d %d Aborting!", __LINE__, socket_data_length,
			(int)len, socket_data->collected_buf_len);
		return -EAGAIN;
	}
	if (len < socket_data_length) {
		LOG_DBG("Incomplete data received! Expected: %d, Received: %d", socket_data_length,
			len);
		return -EAGAIN;
	}
	ret = ring_buf_get(socket_data->buf_pool, sock_data->recv_buf, len);
	if (ret != len) {
		LOG_ERR("%d Data retrieval mismatch: expected %u, got %d", __LINE__, len, ret);
		return -EAGAIN;
	}
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
	LOG_HEXDUMP_DBG(sock_data->recv_buf, ret, "Received Data:");
#endif /* CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG */
	if (sock_data->recv_buf_len < (size_t)len) {
		LOG_ERR("Buffer overflow! Received: %zu vs. Available: %zu", len,
			sock_data->recv_buf_len);
		return -EINVAL;
	}
	if ((size_t)len != (size_t)socket_data_length) {
		LOG_ERR("Data mismatch! Copied: %zu vs. Received: %d", len, socket_data_length);
		return -EINVAL;
	}
	sock_data->recv_read_len = len;
	/* Remove packet from list */
	modem_socket_next_packet_size(&socket_data->socket_config, sock);
	modem_socket_packet_size_update(&socket_data->socket_config, sock, -socket_data_length);
	socket_data->collected_buf_len = 0;
	return len;
}

int modem_handle_data_capture(size_t target_len, struct hl78xx_data *data)
{
	struct hl78xx_socket_data *socket_data =
		(struct hl78xx_socket_data *)data->offload_dev->data;

	return on_cmd_sockread_common(socket_data->current_sock_fd, socket_data->sizeof_socket_data,
				      target_len, data);
}

static int extract_ip_family_and_port(const struct sockaddr *addr, int *af, uint16_t *port)
{
#if defined(CONFIG_NET_IPV6)
	if (addr->sa_family == AF_INET6) {
		*port = ntohs(net_sin6(addr)->sin6_port);
		*af = MDM_HL78XX_SOCKET_AF_IPV6;
	} else {
#endif /* CONFIG_NET_IPV6 */
#if defined(CONFIG_NET_IPV4)
		if (addr->sa_family == AF_INET) {
			*port = ntohs(net_sin(addr)->sin_port);
			*af = MDM_HL78XX_SOCKET_AF_IPV4;
		} else {
#endif /* CONFIG_NET_IPV4 */
			errno = EAFNOSUPPORT;
			return -1;
#if defined(CONFIG_NET_IPV4)
		}
#endif /* CONFIG_NET_IPV4 */
#if defined(CONFIG_NET_IPV6)
	}
#endif /* CONFIG_NET_IPV6 */
	return 0;
}

static int format_ip_and_setup_tls(struct hl78xx_socket_data *socket_data,
				   const struct sockaddr *addr, char *ip_str, size_t ip_str_len,
				   struct modem_socket *sock)
{
	int ret = modem_context_sprint_ip_addr(addr, ip_str, ip_str_len);

	if (ret != 0) {
		LOG_ERR("Failed to format IP!");
		errno = ENOMEM;
		return -1;
	}
	if (sock->ip_proto == IPPROTO_TCP) {
		/* Determine actual length of the formatted IP string (it may be
		 * shorter than the provided buffer size). Copy at most
		 * MDM_MAX_HOSTNAME_LEN - 1 bytes and ensure NUL-termination to
		 * avoid writing past the hostname buffer.
		 */
		size_t actual_len = strnlen(ip_str, ip_str_len);
		size_t copy_len = MIN(actual_len, (size_t)MDM_MAX_HOSTNAME_LEN - 1);

		if (copy_len > 0) {
			memcpy(socket_data->tls.hostname, ip_str, copy_len);
		}
		socket_data->tls.hostname[copy_len] = '\0';
		socket_data->tls.hostname_set = false;
	}
	return 0;
}

static int send_tcp_or_tls_config(struct modem_socket *sock, uint16_t dst_port, int af, int mode,
				  struct hl78xx_socket_data *socket_data)
{
	int ret = 0;
	char cmd_buf[sizeof("AT+KTCPCFG=#,#,\"" MODEM_HL78XX_ADDRESS_FAMILY_FORMAT
			    "\",#####,,,,#,,#") +
		     MDM_MAX_HOSTNAME_LEN + NET_IPV6_ADDR_LEN];

	snprintk(cmd_buf, sizeof(cmd_buf), "AT+KTCPCFG=1,%d,\"%s\",%u,,,,%d,%s,0", mode,
		 socket_data->tls.hostname, dst_port, af, mode == 3 ? "0" : "");
	ret = modem_dynamic_cmd_send(socket_data->mdata_global, NULL, cmd_buf, strlen(cmd_buf),
				     hl78xx_get_ktcpcfg_match(), 1, false);
	if (ret < 0 ||
	    socket_data->tcp_conn_status[HL78XX_TCP_STATUS_ID(sock->id)].is_created == false) {
		LOG_ERR("%s ret:%d", cmd_buf, ret);
		modem_socket_put(&socket_data->socket_config, sock->sock_fd);
		/* Map negative internal return codes to positive errno; fall back to EIO
		 * when the code is non-negative but the operation failed.
		 */
		hl78xx_set_errno_from_code(ret);
		return -1;
	}
	return 0;
}

static int send_udp_config(const struct sockaddr *addr, struct hl78xx_socket_data *socket_data,
			   struct modem_socket *sock)
{
	int ret = 0;
	char cmd_buf[64];
	uint8_t display_data_urc = 0;

#if defined(CONFIG_MODEM_HL78XX_SOCKET_UDP_DISPLAY_DATA_URC)
	display_data_urc = CONFIG_MODEM_HL78XX_SOCKET_UDP_DISPLAY_DATA_URC;
#endif
	snprintk(cmd_buf, sizeof(cmd_buf), "AT+KUDPCFG=1,%u,,%d,,,%d,%d", 0, display_data_urc,
		 (addr->sa_family - 1), 0);

	ret = modem_dynamic_cmd_send(socket_data->mdata_global, NULL, cmd_buf, strlen(cmd_buf),
				     hl78xx_get_kudpind_match(), 1, false);
	if (ret < 0) {
		goto error;
	}
	return 0;
error:
	LOG_ERR("%s ret:%d", cmd_buf, ret);
	modem_socket_put(&socket_data->socket_config, sock->sock_fd);
	hl78xx_set_errno_from_code(ret);
	return -1;
}

static int create_socket(struct modem_socket *sock, const struct sockaddr *addr,
			 struct hl78xx_socket_data *data)
{
	LOG_DBG("entry fd=%d id=%d", sock->sock_fd, sock->id);
	int af;
	uint16_t dst_port;
	char ip_str[NET_IPV6_ADDR_LEN];
	bool is_udp;
	int mode;
	int ret;
	/* save destination address */
	memcpy(&sock->dst, addr, sizeof(*addr));
	if (extract_ip_family_and_port(addr, &af, &dst_port) < 0) {
		return -1;
	}
	if (format_ip_and_setup_tls(data, addr, ip_str, sizeof(ip_str), sock) < 0) {
		return -1;
	}
	is_udp = (sock->ip_proto == IPPROTO_UDP);
	if (is_udp) {
		ret = send_udp_config(addr, data, sock);
		LOG_DBG("send_udp_config returned %d", ret);
		return ret;
	}
	mode = (sock->ip_proto == IPPROTO_TLS_1_2) ? 3 : 0;
	/* only TCP and TLS are supported */
	if (sock->ip_proto != IPPROTO_TCP && sock->ip_proto != IPPROTO_TLS_1_2) {
		LOG_ERR("Unsupported protocol: %d", sock->ip_proto);
		errno = EPROTONOSUPPORT;
		return -1;
	}
	LOG_DBG("TCP/TLS socket, calling send_tcp_or_tls_config af=%d port=%u "
		"mode=%d",
		af, dst_port, mode);
	ret = send_tcp_or_tls_config(sock, dst_port, af, mode, data);
	LOG_DBG("send_tcp_or_tls_config returned %d", ret);
	return ret;
}

static int socket_close(struct hl78xx_socket_data *socket_data, struct modem_socket *sock)
{
	char buf[sizeof("AT+KTCPCLOSE=##\r")];
	int ret = 0;

	if (sock->ip_proto == IPPROTO_UDP) {
		snprintk(buf, sizeof(buf), "AT+KUDPCLOSE=%d", sock->id);
	} else {
		snprintk(buf, sizeof(buf), "AT+KTCPCLOSE=%d", sock->id);
	}
	ret = modem_dynamic_cmd_send(socket_data->mdata_global, NULL, buf, strlen(buf),
				     hl78xx_get_sockets_allow_matches(),
				     hl78xx_get_sockets_allow_matches_size(), false);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", buf, ret);
	}
	return ret;
}

static int socket_delete(struct hl78xx_socket_data *socket_data, struct modem_socket *sock)
{
	char buf[sizeof("AT+KTCPDEL=##\r")];
	int ret = 0;

	if (sock->ip_proto == IPPROTO_UDP) {
		/**
		 * snprintk(buf, sizeof(buf), "AT+KUDPDEL=%d", sock->id);
		 * No need to delete udp config here according to ref guide. The at UDPCLOSE
		 * automatically deletes the session
		 */
		return 0;
	}
	snprintk(buf, sizeof(buf), "AT+KTCPDEL=%d", sock->id);
	ret = modem_dynamic_cmd_send(socket_data->mdata_global, NULL, buf, strlen(buf),
				     hl78xx_get_sockets_allow_matches(),
				     hl78xx_get_sockets_allow_matches_size(), false);
	if (ret < 0) {
		LOG_ERR("%s ret:%d", buf, ret);
	}
	return ret;
}

/* ===== Socket Offload OPS ======================================== */

static int offload_socket(int family, int type, int proto)
{
	int ret;
	/* defer modem's socket create call to bind(); use accessor and check */
	struct hl78xx_socket_data *g = hl78xx_get_socket_global();

	HL78XX_LOG_DBG("%d %d %d %d", __LINE__, family, type, proto);

	if (!g) {
		LOG_ERR("Socket global not initialized");
		errno = ENODEV;
		return -1;
	}
	ret = modem_socket_get(&g->socket_config, family, type, proto);
	if (ret < 0) {
		hl78xx_set_errno_from_code(ret);
		return -1;
	}
	errno = 0;
	return ret;
}

static int offload_close(void *obj)
{
	struct modem_socket *sock = (struct modem_socket *)obj;
	struct hl78xx_socket_data *socket_data = NULL;

	if (!sock) {
		return -EINVAL;
	}
	/* Recover the containing instance; guard in case sock isn't from this driver */
	socket_data = hl78xx_get_socket_global();
	if (!socket_data || !socket_data->offload_dev ||
	    socket_data->offload_dev->data != socket_data) {
		LOG_WRN("parent mismatch: parent != offload_dev->data (%p != %p)", socket_data,
			socket_data ? socket_data->offload_dev->data : NULL);
		errno = EINVAL;
		return -1;
	}
	/* make sure socket is allocated and assigned an id */
	if (modem_socket_id_is_assigned(&socket_data->socket_config, sock) == false) {
		return 0;
	}
	if (validate_socket(sock, socket_data) == 0) {
		socket_close(socket_data, sock);
		socket_delete(socket_data, sock);
		modem_socket_put(&socket_data->socket_config, sock->sock_fd);
		sock->is_connected = false;
	}
	/* Consider here successfully socket is closed */
	return 0;
}

static int offload_bind(void *obj, const struct sockaddr *addr, socklen_t addrlen)
{
	struct modem_socket *sock = (struct modem_socket *)obj;
	struct hl78xx_socket_data *socket_data = hl78xx_socket_data_from_sock(sock);
	int ret = 0;

	if (!sock || !socket_data || !socket_data->offload_dev) {
		errno = EINVAL;
		return -1;
	}
	LOG_DBG("entry for socket fd=%d id=%d", ((struct modem_socket *)obj)->sock_fd,
		((struct modem_socket *)obj)->id);
	/*  Save bind address information */
	memcpy(&sock->src, addr, sizeof(*addr));
	/* Check if socket is allocated */
	if (modem_socket_is_allocated(&socket_data->socket_config, sock)) {
		/* Trigger socket creation */
		ret = create_socket(sock, addr, socket_data);
		LOG_DBG("create_socket returned %d", ret);
		if (ret < 0) {
			LOG_ERR("%d %s SOCKET CREATION", __LINE__, __func__);
			return -1;
		}
	}
	return 0;
}

static int offload_connect(void *obj, const struct sockaddr *addr, socklen_t addrlen)
{
	struct modem_socket *sock = (struct modem_socket *)obj;
	struct hl78xx_socket_data *socket_data = hl78xx_socket_data_from_sock(sock);
	int ret = 0;
	char cmd_buf[sizeof("AT+KTCPCFG=#\r")];
	char ip_str[NET_IPV6_ADDR_LEN];

	if (!addr || !socket_data || !socket_data->offload_dev) {
		errno = EINVAL;
		return -1;
	}
	if (!hl78xx_is_registered(socket_data->mdata_global)) {
		errno = ENETUNREACH;
		return -1;
	}
	/* make sure socket has been allocated */
	if (modem_socket_is_allocated(&socket_data->socket_config, sock) == false) {
		LOG_ERR("Invalid socket_id(%d) from fd:%d", sock->id, sock->sock_fd);
		errno = EINVAL;
		return -1;
	}
	/* make sure we've created the socket */
	if (modem_socket_id_is_assigned(&socket_data->socket_config, sock) == false) {
		LOG_DBG("%d no socket assigned", __LINE__);
		if (create_socket(sock, addr, socket_data) < 0) {
			return -1;
		}
	}
	memcpy(&sock->dst, addr, sizeof(*addr));
	/* skip socket connect if UDP */
	if (sock->ip_proto == IPPROTO_UDP) {
		errno = 0;
		return 0;
	}
	ret = modem_context_sprint_ip_addr(addr, ip_str, sizeof(ip_str));
	if (ret != 0) {
		hl78xx_set_errno_from_code(ret);
		LOG_ERR("Error formatting IP string %d", ret);
		return -1;
	}
	/* send connect command */
	snprintk(cmd_buf, sizeof(cmd_buf), "AT+KTCPCNX=%d", sock->id);
	ret = modem_dynamic_cmd_send(socket_data->mdata_global, NULL, cmd_buf, strlen(cmd_buf),
				     hl78xx_get_ktcpind_match(), 1, false);
	if (ret < 0 ||
	    socket_data->tcp_conn_status[HL78XX_TCP_STATUS_ID(sock->id)].is_connected == false) {
		sock->is_connected = false;
		LOG_ERR("%s ret:%d", cmd_buf, ret);
		/* Map tcp_conn_status.err_code:
		 * - positive values are assumed to be direct POSIX errno values -> pass
		 * through
		 * - zero or unknown -> use conservative EIO
		 */
		errno = (socket_data->tcp_conn_status[HL78XX_TCP_STATUS_ID(sock->id)].err_code > 0)
				? socket_data->tcp_conn_status[HL78XX_TCP_STATUS_ID(sock->id)]
					  .err_code
				: EIO;
		return -1;
	}
	sock->is_connected = true;
	errno = 0;
	return 0;
}

static bool validate_recv_args(void *buf, size_t len, int flags)
{
	if (!buf || len == 0) {
		errno = EINVAL;
		return false;
	}
	if (flags & ZSOCK_MSG_PEEK) {
		errno = ENOTSUP;
		return false;
	}
	return true;
}

static int wait_for_data_if_needed(struct hl78xx_socket_data *socket_data,
				   struct modem_socket *sock, int flags)
{
	int size = modem_socket_next_packet_size(&socket_data->socket_config, sock);

	if (size > 0) {
		return size;
	}
	if (flags & ZSOCK_MSG_DONTWAIT) {
		errno = EAGAIN;
		return -1;
	}
	if (validate_socket(sock, socket_data) == -1) {
		errno = 0;
		return 0;
	}

	modem_socket_wait_data(&socket_data->socket_config, sock);
	return modem_socket_next_packet_size(&socket_data->socket_config, sock);
}

static void prepare_read_command(struct hl78xx_socket_data *socket_data, char *sendbuf,
				 size_t bufsize, struct modem_socket *sock, size_t read_size)
{
	snprintk(sendbuf, bufsize, "AT+K%sRCV=%d,%zd%s",
		 sock->ip_proto == IPPROTO_UDP ? "UDP" : "TCP", sock->id, read_size,
		 socket_data->mdata_global->chat.delimiter);
}

/* Perform the receive transaction: release chat, attach pipe, wait for tx sem,
 * transmit read command, wait for rx sem and capture data. Returns 0 on
 * success or a negative code which will be mapped by caller.
 */
static int hl78xx_perform_receive_transaction(struct hl78xx_socket_data *socket_data,
					      const char *sendbuf)
{
	int rv;
	int ret;

	modem_chat_release(&socket_data->mdata_global->chat);
	modem_pipe_attach(socket_data->mdata_global->uart_pipe, modem_pipe_callback,
			  socket_data->mdata_global);

	rv = k_sem_take(&socket_data->mdata_global->script_stopped_sem_tx_int, K_FOREVER);
	if (rv < 0) {
		LOG_ERR("%s: k_sem_take(tx) returned %d", __func__, rv);
		return rv;
	}

	ret = modem_pipe_transmit(socket_data->mdata_global->uart_pipe, (const uint8_t *)sendbuf,
				  strlen(sendbuf));
	if (ret < 0) {
		LOG_ERR("Error sending read command: %d", ret);
		return ret;
	}
	rv = k_sem_take(&socket_data->mdata_global->script_stopped_sem_rx_int, K_FOREVER);
	if (rv < 0) {
		return rv;
	}

	rv = modem_handle_data_capture(socket_data->sizeof_socket_data, socket_data->mdata_global);
	if (rv < 0) {
		return rv;
	}

	return 0;
}

static void setup_socket_data(struct hl78xx_socket_data *socket_data, struct modem_socket *sock,
			      struct socket_read_data *sock_data, void *buf, size_t len,
			      struct sockaddr *from, uint16_t read_size)
{
	memset(sock_data, 0, sizeof(*sock_data));
	sock_data->recv_buf = buf;
	sock_data->recv_buf_len = len;
	sock_data->recv_addr = from;
	sock->data = sock_data;

	socket_data->sizeof_socket_data = read_size;
	socket_data->requested_socket_id = sock->id;
	socket_data->current_sock_fd = sock->sock_fd;
	socket_data->expected_buf_len = read_size + sizeof("\r\n") - 1 +
					socket_data->mdata_global->buffers.eof_pattern_size +
					sizeof(MODEM_STREAM_END_WORD) - 1;
	socket_data->collected_buf_len = 0;
	socket_data->socket_data_error = false;
}

static void check_tcp_state_if_needed(struct hl78xx_socket_data *socket_data,
				      struct modem_socket *sock)
{
	const char *check_ktcp_stat = "AT+KTCPSTAT";
	/* Only check for TCP sockets */
	if (sock->type != SOCK_STREAM) {
		return;
	}
	if (atomic_test_and_clear_bit(&socket_data->mdata_global->state_leftover,
				      MODEM_SOCKET_DATA_LEFTOVER_STATE_BIT) &&
	    sock && sock->ip_proto == IPPROTO_TCP) {
		modem_dynamic_cmd_send(socket_data->mdata_global, NULL, check_ktcp_stat,
				       strlen(check_ktcp_stat), hl78xx_get_ktcp_state_match(), 1,
				       true);
	}
}

static ssize_t offload_recvfrom(void *obj, void *buf, size_t len, int flags, struct sockaddr *from,
				socklen_t *fromlen)
{
	struct modem_socket *sock = (struct modem_socket *)obj;
	struct hl78xx_socket_data *socket_data = hl78xx_socket_data_from_sock(sock);
	char sendbuf[sizeof("AT+KUDPRCV=#,##########\r\n")];
	struct socket_read_data sock_data;
	int next_packet_size = 0;
	uint32_t max_data_length = 0;
	uint16_t read_size = 0;
	int trv = 0;
	int ret;

	if (!sock || !socket_data || !socket_data->offload_dev) {
		errno = EINVAL;
		return -1;
	}
	/* If modem is not registered yet, propagate EAGAIN to indicate try again
	 * later. However, if the socket simply isn't connected (validate_socket
	 * returns -1) we return 0 with errno cleared so upper layers (eg. DNS
	 * dispatcher) treat this as no data available rather than an error and
	 * avoid noisy repeated error logs.
	 */
	if (!hl78xx_is_registered(socket_data->mdata_global)) {
		errno = EAGAIN;
		return -1;
	}
	if (validate_socket(sock, socket_data) == -1) {
		errno = 0;
		return 0;
	}

	if (!validate_recv_args(buf, len, flags)) {
		return -1;
	}
	ret = k_mutex_lock(&socket_data->mdata_global->tx_lock, K_SECONDS(1));
	if (ret < 0) {
		LOG_ERR("Failed to acquire TX lock: %d", ret);
		hl78xx_set_errno_from_code(ret);
		return -1;
	}
	next_packet_size = wait_for_data_if_needed(socket_data, sock, flags);
	if (next_packet_size <= 0) {
		ret = next_packet_size;
		goto exit;
	}
	max_data_length =
		MDM_MAX_DATA_LENGTH - (socket_data->mdata_global->buffers.eof_pattern_size +
				       sizeof(MODEM_STREAM_STARTER_WORD) - 1);
	/* limit read size to modem max data length */
	next_packet_size = MIN(next_packet_size, max_data_length);
	/* limit read size to user buffer length */
	read_size = MIN(next_packet_size, len);
	/* prepare socket data for the read operation */
	setup_socket_data(socket_data, sock, &sock_data, buf, len, from, read_size);
	prepare_read_command(socket_data, sendbuf, sizeof(sendbuf), sock, read_size);
	HL78XX_LOG_DBG("%d socket_fd: %d, socket_id: %d, expected_data_len: %d", __LINE__,
		       socket_data->current_sock_fd, socket_data->requested_socket_id,
		       socket_data->expected_buf_len);
	LOG_HEXDUMP_DBG(sendbuf, strlen(sendbuf), "sending");
	trv = hl78xx_perform_receive_transaction(socket_data, sendbuf);
	if (trv < 0) {
		hl78xx_set_errno_from_code(trv);
		ret = -1;
		goto exit;
	}
	if (from && fromlen) {
		*fromlen = sizeof(sock->dst);
		memcpy(from, &sock->dst, *fromlen);
	}
	errno = 0;
	ret = sock_data.recv_read_len;
exit:
	k_mutex_unlock(&socket_data->mdata_global->tx_lock);
	modem_chat_attach(&socket_data->mdata_global->chat, socket_data->mdata_global->uart_pipe);
	socket_data->expected_buf_len = 0;
	check_tcp_state_if_needed(socket_data, sock);
	return ret;
}

int check_if_any_socket_connected(const struct device *dev)
{
	struct hl78xx_data *data = dev->data;
	struct hl78xx_socket_data *socket_data =
		(struct hl78xx_socket_data *)data->offload_dev->data;
	struct modem_socket_config *cfg = &socket_data->socket_config;

	k_sem_take(&cfg->sem_lock, K_FOREVER);
	for (int i = 0; i < cfg->sockets_len; i++) {
		if (cfg->sockets[i].is_connected) {
			/* if there is any socket  connected */
			k_sem_give(&cfg->sem_lock);
			return true;
		}
	}
	k_sem_give(&cfg->sem_lock);
	return false;
}

/* ===== Send / Receive helpers ========================================
 * Helpers used by sendto/recv paths, preparing commands and transmitting
 * data over the modem pipe.
 */
static int prepare_send_cmd(const struct modem_socket *sock, const struct sockaddr *dst_addr,
			    size_t buf_len, char *cmd_buf, size_t cmd_buf_size)
{
	int ret = 0;

	if (sock->ip_proto == IPPROTO_UDP) {
		char ip_str[NET_IPV6_ADDR_LEN];
		uint16_t dst_port = 0;

		ret = modem_context_sprint_ip_addr(dst_addr, ip_str, sizeof(ip_str));
		if (ret < 0) {
			LOG_ERR("Error formatting IP string %d", ret);
			return ret;
		}
		ret = modem_context_get_addr_port(dst_addr, &dst_port);
		if (ret < 0) {
			LOG_ERR("Error getting port from IP address %d", ret);
			return ret;
		}
		snprintk(cmd_buf, cmd_buf_size, "AT+KUDPSND=%d,\"%s\",%u,%zu", sock->id, ip_str,
			 dst_port, buf_len);
		return 0;
	}

	/* Default to TCP-style send command */
	snprintk(cmd_buf, cmd_buf_size, "AT+KTCPSND=%d,%zu", sock->id, buf_len);
	return 0;
}

static int send_data_buffer(struct hl78xx_socket_data *socket_data, const char *buf,
			    const size_t buf_len, int *sock_written)
{
	uint32_t offset = 0;
	int len = buf_len;
	int ret = 0;

	if (len == 0) {
		LOG_DBG("%d No data to send", __LINE__);
		return 0;
	}
	while (len > 0) {
		LOG_DBG("waiting for TX semaphore (offset=%u len=%d)", offset, len);
		if (k_sem_take(&socket_data->mdata_global->script_stopped_sem_tx_int, K_FOREVER) <
		    0) {
			LOG_ERR("%s: k_sem_take(tx) failed", __func__);
			return -1;
		}
		ret = modem_pipe_transmit(socket_data->mdata_global->uart_pipe,
					  ((const uint8_t *)buf) + offset, len);
		if (ret <= 0) {
			LOG_ERR("Transmit error %d", ret);
			return -1;
		}
		offset += ret;
		len -= ret;
		*sock_written += ret;
	}
	return 0;
}

static int validate_and_prepare(struct modem_socket *sock, const struct sockaddr **dst_addr,
				size_t *buf_len, char *cmd_buf, size_t cmd_buf_len)
{
	/* Validate args and prepare send command */
	if (!sock) {
		errno = EINVAL;
		return -1;
	}
	if (sock->type != SOCK_DGRAM && !sock->is_connected) {
		errno = ENOTCONN;
		return -1;
	}
	if (!*dst_addr && sock->ip_proto == IPPROTO_UDP) {
		*dst_addr = &sock->dst;
	}
	if (*buf_len > MDM_MAX_DATA_LENGTH) {
		if (sock->type == SOCK_DGRAM) {
			errno = EMSGSIZE;
			return -1;
		}
		*buf_len = MDM_MAX_DATA_LENGTH;
	}
	/* Consolidated send command helper handles UDP vs TCP formatting */
	return prepare_send_cmd(sock, *dst_addr, *buf_len, cmd_buf, cmd_buf_len);
}

static int transmit_regular_data(struct hl78xx_socket_data *socket_data, const char *buf,
				 size_t buf_len, int *sock_written)
{
	int ret;

	ret = send_data_buffer(socket_data, buf, buf_len, sock_written);
	if (ret < 0) {
		return ret;
	}
	ret = k_sem_take(&socket_data->mdata_global->script_stopped_sem_tx_int, K_FOREVER);
	if (ret < 0) {
		LOG_ERR("%s: k_sem_take(tx) returned %d", __func__, ret);
		return ret;
	}
	return modem_pipe_transmit(socket_data->mdata_global->uart_pipe,
				   (uint8_t *)socket_data->mdata_global->buffers.eof_pattern,
				   socket_data->mdata_global->buffers.eof_pattern_size);
}

/* send binary data via the +KUDPSND/+KTCPSND commands */
static ssize_t send_socket_data(void *obj, struct hl78xx_socket_data *socket_data,
				const struct sockaddr *dst_addr, const char *buf, size_t buf_len,
				k_timeout_t timeout)
{
	struct modem_socket *sock = (struct modem_socket *)obj;
	char cmd_buf[82] = {0}; /* AT+KUDPSND/KTCP=,IP,PORT,LENGTH */
	int ret;
	int sock_written = 0;

	ret = validate_and_prepare(sock, &dst_addr, &buf_len, cmd_buf, sizeof(cmd_buf));
	if (ret < 0) {
		return ret;
	}
	socket_data->socket_data_error = false;
	if (k_mutex_lock(&socket_data->mdata_global->tx_lock, K_SECONDS(1)) < 0) {
		return -1;
	}
	ret = modem_dynamic_cmd_send(socket_data->mdata_global, NULL, cmd_buf, strlen(cmd_buf),
				     (const struct modem_chat_match *)hl78xx_get_connect_matches(),
				     hl78xx_get_connect_matches_size(), false);
	if (ret < 0 || socket_data->socket_data_error) {
		hl78xx_set_errno_from_code(ret);
		ret = -1;
		goto cleanup;
	}
	modem_pipe_attach(socket_data->mdata_global->chat.pipe, modem_pipe_callback,
			  socket_data->mdata_global);
	ret = transmit_regular_data(socket_data, buf, buf_len, &sock_written);
	if (ret < 0) {
		goto cleanup;
	}
	modem_chat_attach(&socket_data->mdata_global->chat, socket_data->mdata_global->uart_pipe);
	ret = modem_dynamic_cmd_send(socket_data->mdata_global, NULL, "", 0,
				     hl78xx_get_sockets_ok_match(), 1, false);
	if (ret < 0) {
		LOG_ERR("Final confirmation failed: %d", ret);
		goto cleanup;
	}
cleanup:
	k_mutex_unlock(&socket_data->mdata_global->tx_lock);
	return (ret < 0) ? -1 : sock_written;
}

#ifdef CONFIG_MODEM_HL78XX_SOCKETS_SOCKOPT_TLS
/* ===== TLS implementation (conditional) ================================
 * TLS credential upload and chipper settings helper implementations.
 */
static int handle_tls_sockopts(void *obj, int optname, const void *optval, socklen_t optlen)
{
	int ret;
	struct modem_socket *sock = (struct modem_socket *)obj;
	struct hl78xx_socket_data *socket_data = hl78xx_socket_data_from_sock(sock);

	if (!sock || !socket_data || !socket_data->offload_dev) {
		return -EINVAL;
	}

	switch (optname) {
	case TLS_SEC_TAG_LIST:
		ret = map_credentials(socket_data, optval, optlen);
		return ret;

	case TLS_HOSTNAME:
		if (optlen >= MDM_MAX_HOSTNAME_LEN) {
			return -EINVAL;
		}
		memset(socket_data->tls.hostname, 0, MDM_MAX_HOSTNAME_LEN);
		memcpy(socket_data->tls.hostname, optval, optlen);
		socket_data->tls.hostname[optlen] = '\0';
		socket_data->tls.hostname_set = true;
		ret = hl78xx_configure_chipper_suit(socket_data);
		if (ret < 0) {
			LOG_ERR("Failed to configure chipper suit: %d", ret);
			return ret;
		}
		LOG_DBG("TLS hostname set to: %s", socket_data->tls.hostname);
		return 0;

	case TLS_PEER_VERIFY:
		if (*(const uint32_t *)optval != TLS_PEER_VERIFY_REQUIRED) {
			LOG_WRN("Disabling peer verification is not supported");
		}
		return 0;

	case TLS_CERT_NOCOPY:
		return 0; /* No-op, success */

	default:
		LOG_DBG("Unsupported TLS option: %d", optname);
		return -EINVAL;
	}
}

static int offload_setsockopt(void *obj, int level, int optname, const void *optval,
			      socklen_t optlen)
{
	int ret = 0;

	if (!IS_ENABLED(CONFIG_NET_SOCKETS_SOCKOPT_TLS)) {
		return -EINVAL;
	}
	if (level == SOL_TLS) {
		ret = handle_tls_sockopts(obj, optname, optval, optlen);
		if (ret < 0) {
			hl78xx_set_errno_from_code(ret);
			return -1;
		}
		return 0;
	}
	LOG_DBG("Unsupported socket option: %d", optname);
	return -EINVAL;
}
#endif /* CONFIG_MODEM_HL78XX_SOCKETS_SOCKOPT_TLS */

static ssize_t offload_sendto(void *obj, const void *buf, size_t len, int flags,
			      const struct sockaddr *to, socklen_t tolen)
{
	int ret = 0;
	struct modem_socket *sock = (struct modem_socket *)obj;
	struct hl78xx_socket_data *socket_data = hl78xx_socket_data_from_sock(sock);

	if (!sock || !socket_data || !socket_data->offload_dev) {
		errno = EINVAL;
		return -1;
	}
	if (!hl78xx_is_registered(socket_data->mdata_global)) {
		LOG_ERR("Modem currently not attached to the network!");
		return -EAGAIN;
	}
	/* Do some sanity checks. */
	if (!buf || len == 0) {
		errno = EINVAL;
		return -1;
	}
	/* For stream sockets (TCP) the socket must be connected. For datagram
	 * sockets (UDP) sendto can be used without a prior connect as long as a
	 * destination address is provided or the socket has a stored dst. The
	 * helper validate_and_prepare will supply sock->dst for UDP when needed.
	 */
	if (sock->type != SOCK_DGRAM && !sock->is_connected) {
		errno = ENOTCONN;
		return -1;
	}
	/* Only send up to MTU bytes. */
	if (len > MDM_MAX_DATA_LENGTH) {
		len = MDM_MAX_DATA_LENGTH;
	}
	ret = send_socket_data(obj, socket_data, to, buf, len, K_SECONDS(MDM_CMD_TIMEOUT));
	if (ret < 0) {
		/* Map internal negative return codes to positive errno values. Use EIO as
		 * a conservative fallback when ret is non-negative (unexpected)
		 */
		hl78xx_set_errno_from_code(ret);
		return -1;
	}
	errno = 0;
	return ret;
}

static int offload_ioctl(void *obj, unsigned int request, va_list args)
{
	int ret = 0;
	struct modem_socket *sock = (struct modem_socket *)obj;
	struct hl78xx_socket_data *socket_data = hl78xx_socket_data_from_sock(sock);
	struct zsock_pollfd *pfd;
	struct k_poll_event **pev;
	struct k_poll_event *pev_end;
	/* sanity check: does parent == parent->offload_dev->data ? */
	if (socket_data && socket_data->offload_dev &&
	    socket_data->offload_dev->data != socket_data) {
		LOG_WRN("parent mismatch: parent != offload_dev->data (%p != %p)", socket_data,
			socket_data->offload_dev->data);
	}
	switch (request) {
	case ZFD_IOCTL_POLL_PREPARE:
		pfd = va_arg(args, struct zsock_pollfd *);
		pev = va_arg(args, struct k_poll_event **);
		pev_end = va_arg(args, struct k_poll_event *);
		ret = modem_socket_poll_prepare(&socket_data->socket_config, obj, pfd, pev,
						pev_end);

		if (ret == -1 && errno == ENOTSUP && (pfd->events & ZSOCK_POLLOUT) &&
		    sock->ip_proto == IPPROTO_UDP) {
			/* Not Implemented */
			/*
			 *	You can implement this later when needed
			 *	For now, just ignore it
			 */
			errno = ENOTSUP;
			ret = 0;
		}
		return ret;

	case ZFD_IOCTL_POLL_UPDATE:
		pfd = va_arg(args, struct zsock_pollfd *);
		pev = va_arg(args, struct k_poll_event **);
		return modem_socket_poll_update(obj, pfd, pev);

	case F_GETFL:
		return 0;

	case F_SETFL: {
#ifdef CONFIG_MODEM_HL78XX_LOG_CONTEXT_VERBOSE_DEBUG
		int flags = va_arg(args, int);

		LOG_DBG("F_SETFL called with flags=0x%x", flags);
		ARG_UNUSED(flags);
#endif
		/* You can store flags if you want, but it's safe to just ignore them. */
		return 0;
	}

	default:
		errno = EINVAL;
		return -1;
	}
}

static ssize_t offload_read(void *obj, void *buffer, size_t count)
{
	return offload_recvfrom(obj, buffer, count, 0, NULL, 0);
}

static ssize_t offload_write(void *obj, const void *buffer, size_t count)
{
	return offload_sendto(obj, buffer, count, 0, NULL, 0);
}

static ssize_t offload_sendmsg(void *obj, const struct msghdr *msg, int flags)
{
	ssize_t sent = 0;
	struct iovec bkp_iovec = {0};
	struct msghdr crafted_msg = {.msg_name = msg->msg_name, .msg_namelen = msg->msg_namelen};
	struct modem_socket *sock = (struct modem_socket *)obj;
	struct hl78xx_socket_data *socket_data = hl78xx_socket_data_from_sock(sock);
	size_t full_len = 0;
	int ret;

	if (!sock || !socket_data || !socket_data->offload_dev) {
		errno = EINVAL;
		return -1;
	}
	/* Compute the full length to send and validate input */
	for (int i = 0; i < msg->msg_iovlen; i++) {
		if (!msg->msg_iov[i].iov_base || msg->msg_iov[i].iov_len == 0) {
			errno = EINVAL;
			return -1;
		}
		full_len += msg->msg_iov[i].iov_len;
	}
	while (full_len > sent) {
		int removed = 0;
		int i = 0;
		int bkp_iovec_idx = -1;

		crafted_msg.msg_iovlen = msg->msg_iovlen;
		crafted_msg.msg_iov = &msg->msg_iov[0];

		/* Adjust iovec to remove already sent bytes */
		while (removed < sent) {
			int to_remove = sent - removed;

			if (to_remove >= msg->msg_iov[i].iov_len) {
				crafted_msg.msg_iovlen -= 1;
				crafted_msg.msg_iov = &msg->msg_iov[i + 1];
				removed += msg->msg_iov[i].iov_len;
			} else {
				bkp_iovec_idx = i;
				bkp_iovec = msg->msg_iov[i];

				msg->msg_iov[i].iov_len -= to_remove;
				msg->msg_iov[i].iov_base =
					((uint8_t *)msg->msg_iov[i].iov_base) + to_remove;

				removed += to_remove;
			}
			i++;
		}
		/* send_socket_data expects a buffer pointer and its byte length.
		 * crafted_msg.msg_iovlen is the number of iovec entries and is
		 * incorrect here (was causing sends of '2' bytes when two iovecs
		 * were present). Use the first iovec's iov_len for the byte length.
		 */
		ret = send_socket_data(obj, socket_data, crafted_msg.msg_name,
				       crafted_msg.msg_iov->iov_base, crafted_msg.msg_iov->iov_len,
				       K_SECONDS(MDM_CMD_TIMEOUT));
		if (bkp_iovec_idx != -1) {
			msg->msg_iov[bkp_iovec_idx] = bkp_iovec;
		}
		if (ret < 0) {
			/* Map negative internal return code to positive errno; fall back to
			 * EIO
			 */
			hl78xx_set_errno_from_code(ret);
			return -1;
		}
		sent += ret;
	}
	return sent;
}
/* clang-format off */
static const struct socket_op_vtable offload_socket_fd_op_vtable = {
	.fd_vtable = {
			.read = offload_read,
			.write = offload_write,
			.close = offload_close,
			.ioctl = offload_ioctl,
		},
	.bind = offload_bind,
	.connect = offload_connect,
	.sendto = offload_sendto,
	.recvfrom = offload_recvfrom,
	.listen = NULL,
	.accept = NULL,
	.sendmsg = offload_sendmsg,
	.getsockopt = NULL,
#if defined(CONFIG_MODEM_HL78XX_SOCKETS_SOCKOPT_TLS)
	.setsockopt = offload_setsockopt,
#else
	.setsockopt = NULL,
#endif
};
/* clang-format on */
static int hl78xx_init_sockets(const struct device *dev)
{
	int ret;
	struct hl78xx_socket_data *socket_data = (struct hl78xx_socket_data *)dev->data;

	socket_data->buf_pool = &mdm_recv_pool;
	/* socket config */
	ret = modem_socket_init(&socket_data->socket_config, &socket_data->sockets[0],
				ARRAY_SIZE(socket_data->sockets), MDM_BASE_SOCKET_NUM, false,
				&offload_socket_fd_op_vtable);
	if (ret) {
		goto error;
	}
	return 0;
error:
	return ret;
}
static void socket_notify_data(int socket_id, int new_total, void *user_data)
{
	int ret = 0;
	struct modem_socket *sock;
	struct hl78xx_data *data = (struct hl78xx_data *)user_data;
	struct hl78xx_socket_data *socket_data =
		(struct hl78xx_socket_data *)data->offload_dev->data;

	if (!data || !socket_data) {
		LOG_ERR("%s: invalid user_data", __func__);
		return;
	}
	sock = modem_socket_from_id(&socket_data->socket_config, socket_id);
	if (!sock) {
		return;
	}
	/* Update the packet size */
	ret = modem_socket_packet_size_update(&socket_data->socket_config, sock, new_total);
	if (ret < 0) {
		LOG_ERR("socket_id:%d left_bytes:%d err: %d", socket_id, new_total, ret);
	}
	if (new_total > 0) {
		modem_socket_data_ready(&socket_data->socket_config, sock);
	}
	/* Duplicate/chat callback block removed; grouped versions live earlier */
}

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS) && defined(CONFIG_MODEM_HL78XX_SOCKETS_SOCKOPT_TLS)
static int hl78xx_configure_chipper_suit(struct hl78xx_socket_data *socket_data)
{
	const char *cmd_chipper_suit = "AT+KSSLCRYPTO=0,8,1,8192,4,4,3,0";

	return modem_dynamic_cmd_send(
		socket_data->mdata_global, NULL, cmd_chipper_suit, strlen(cmd_chipper_suit),
		(const struct modem_chat_match *)hl78xx_get_ok_match(), 1, false);
}
/* send binary data via the K....STORE commands */
static ssize_t hl78xx_send_cert(struct hl78xx_socket_data *socket_data, const char *cert_data,
				size_t cert_len, enum tls_credential_type cert_type)
{
	int ret;
	char send_buf[sizeof("AT+KPRIVKSTORE=#,####\r\n")];
	int sock_written = 0;

	if (!socket_data || !socket_data->mdata_global) {
		return -EINVAL;
	}

	if (cert_len == 0 || !cert_data) {
		LOG_ERR("Invalid certificate data or length");
		return -EINVAL;
	}
	/** Certificate length exceeds maximum allowed size */
	if (cert_len > MDM_MAX_CERT_LENGTH) {
		return -EINVAL;
	}

	if (cert_type == TLS_CREDENTIAL_CA_CERTIFICATE ||
	    cert_type == TLS_CREDENTIAL_SERVER_CERTIFICATE) {
		snprintk(send_buf, sizeof(send_buf), "AT+KCERTSTORE=%d,%d", (cert_type - 1),
			 cert_len);

	} else if (cert_type == TLS_CREDENTIAL_PRIVATE_KEY) {
		snprintk(send_buf, sizeof(send_buf), "AT+KPRIVKSTORE=0,%d", cert_len);

	} else {
		LOG_ERR("Unsupported certificate type: %d", cert_type);
		return -EINVAL;
	}
	socket_data->socket_data_error = false;
	if (k_mutex_lock(&socket_data->mdata_global->tx_lock, K_SECONDS(1)) < 0) {
		errno = EBUSY;
		return -1;
	}
	ret = modem_dynamic_cmd_send(socket_data->mdata_global, NULL, send_buf, strlen(send_buf),
				     (const struct modem_chat_match *)hl78xx_get_connect_matches(),
				     hl78xx_get_connect_matches_size(), false);
	if (ret < 0) {
		LOG_ERR("Error sending AT command %d", ret);
	}
	if (socket_data->socket_data_error) {
		ret = -ENODEV;
		errno = ENODEV;
		goto cleanup;
	}
	modem_pipe_attach(socket_data->mdata_global->chat.pipe, modem_pipe_callback,
			  socket_data->mdata_global);
	ret = send_data_buffer(socket_data, cert_data, cert_len, &sock_written);
	if (ret < 0) {
		goto cleanup;
	}
	ret = k_sem_take(&socket_data->mdata_global->script_stopped_sem_tx_int, K_FOREVER);
	if (ret < 0) {
		goto cleanup;
	}
	ret = modem_pipe_transmit(socket_data->mdata_global->uart_pipe,
				  (uint8_t *)socket_data->mdata_global->buffers.eof_pattern,
				  socket_data->mdata_global->buffers.eof_pattern_size);
	if (ret < 0) {
		LOG_ERR("Error sending EOF pattern: %d", ret);
	}
	modem_chat_attach(&socket_data->mdata_global->chat, socket_data->mdata_global->uart_pipe);
	ret = modem_dynamic_cmd_send(socket_data->mdata_global, NULL, "", 0,
				     (const struct modem_chat_match *)hl78xx_get_ok_match(), 1,
				     false);
	if (ret < 0) {
		LOG_ERR("Final confirmation failed: %d", ret);
		goto cleanup;
	}
cleanup:
	k_mutex_unlock(&socket_data->mdata_global->tx_lock);
	return (ret < 0) ? -1 : sock_written;
}

static int map_credentials(struct hl78xx_socket_data *socket_data, const void *optval,
			   socklen_t optlen)
{
	const sec_tag_t *sec_tags = (const sec_tag_t *)optval;
	int ret = 0;
	int tags_len;
	sec_tag_t tag;
	int i;
	struct tls_credential *cert;

	if ((optlen % sizeof(sec_tag_t)) != 0 || (optlen == 0)) {
		return -EINVAL;
	}
	tags_len = optlen / sizeof(sec_tag_t);
	/* For each tag, retrieve the credentials value and type: */
	for (i = 0; i < tags_len; i++) {
		tag = sec_tags[i];
		cert = credential_next_get(tag, NULL);
		while (cert != NULL) {
			switch (cert->type) {
			case TLS_CREDENTIAL_CA_CERTIFICATE:
				LOG_DBG("TLS_CREDENTIAL_CA_CERTIFICATE tag: %d", tag);
				break;

			case TLS_CREDENTIAL_SERVER_CERTIFICATE:
				LOG_DBG("TLS_CREDENTIAL_SERVER_CERTIFICATE tag: %d", tag);
				break;

			case TLS_CREDENTIAL_PRIVATE_KEY:
				LOG_DBG("TLS_CREDENTIAL_PRIVATE_KEY tag: %d", tag);
				break;

			case TLS_CREDENTIAL_NONE:
			case TLS_CREDENTIAL_PSK:
			case TLS_CREDENTIAL_PSK_ID:
			default:
				/* Not handled */
				return -EINVAL;
			}
			ret = hl78xx_send_cert(socket_data, cert->buf, cert->len, cert->type);
			if (ret < 0) {
				return ret;
			}
			cert = credential_next_get(tag, cert);
		}
	}

	return 0;
}
#endif

static int hl78xx_socket_init(const struct device *dev)
{

	struct hl78xx_socket_data *data = (struct hl78xx_socket_data *)dev->data;

	data->offload_dev = dev;
	/* Ensure the parent modem device pointer was set at static init time */
	if (data->modem_dev == NULL) {
		LOG_ERR("modem_dev not initialized for %s", dev->name);
		return -EINVAL;
	}
	/* Ensure the modem device is ready before accessing its driver data */
	if (!device_is_ready(data->modem_dev)) {
		LOG_ERR("modem device %s not ready", data->modem_dev->name);
		return -ENODEV;
	}
	if (data->modem_dev->data == NULL) {
		LOG_ERR("modem device %s has no driver data yet", data->modem_dev->name);
		return -EAGAIN;
	}
	data->mdata_global = (struct hl78xx_data *)data->modem_dev->data;
	data->mdata_global->offload_dev = dev;
	/* Keep original single global pointer usage but set via accessor. */
	hl78xx_set_socket_global(data);
	atomic_set(&data->mdata_global->state_leftover, 0);

	return 0;
}

static void modem_net_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct hl78xx_socket_data *data = dev->data;

	/* startup trace */
	if (!data->mdata_global) {
		LOG_WRN("mdata_global not set for net iface init on %s", dev->name);
	}
	net_if_set_link_addr(
		iface,
		modem_get_mac(data->mac_addr,
			      data->mdata_global ? data->mdata_global->identity.imei : NULL),
		sizeof(data->mac_addr), NET_LINK_ETHERNET);
	data->net_iface = iface;
	hl78xx_init_sockets(dev);
	net_if_socket_offload_set(iface, offload_socket);
}

static struct offloaded_if_api api_funcs = {
	.iface_api.init = modem_net_iface_init,
};

static bool offload_is_supported(int family, int type, int proto)
{
	bool fam_ok = false;

#ifdef CONFIG_NET_IPV4
	if (family == AF_INET) {
		fam_ok = true;
	}
#endif
#ifdef CONFIG_NET_IPV6
	if (family == AF_INET6) {
		fam_ok = true;
	}
#endif
	if (!fam_ok) {
		return false;
	}
	if (!(type == SOCK_DGRAM || type == SOCK_STREAM)) {
		return false;
	}
	if (proto == IPPROTO_TCP || proto == IPPROTO_UDP) {
		return true;
	}
#if defined(CONFIG_MODEM_HL78XX_SOCKETS_SOCKOPT_TLS)
	if (proto == IPPROTO_TLS_1_2) {
		return true;
	}
#endif
	return false;
}

#define MODEM_HL78XX_DEFINE_OFFLOAD_INSTANCE(inst)                                                 \
	static struct hl78xx_socket_data hl78xx_socket_data_##inst = {                             \
		.modem_dev = DEVICE_DT_GET(DT_PARENT(DT_DRV_INST(inst))),                          \
	};                                                                                         \
	NET_DEVICE_OFFLOAD_INIT(                                                                   \
		inst, "hl78xx_dev", hl78xx_socket_init, NULL, &hl78xx_socket_data_##inst, NULL,    \
		CONFIG_MODEM_HL78XX_OFFLOAD_INIT_PRIORITY, &api_funcs, MDM_MAX_DATA_LENGTH);       \
                                                                                                   \
	NET_SOCKET_OFFLOAD_REGISTER(inst, CONFIG_NET_SOCKETS_OFFLOAD_PRIORITY, AF_UNSPEC,          \
				    offload_is_supported, offload_socket);

#define MODEM_OFFLOAD_DEVICE_SWIR_HL78XX(inst) MODEM_HL78XX_DEFINE_OFFLOAD_INSTANCE(inst)

#define DT_DRV_COMPAT swir_hl7812_offload
DT_INST_FOREACH_STATUS_OKAY(MODEM_OFFLOAD_DEVICE_SWIR_HL78XX)
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT swir_hl7800_offload
DT_INST_FOREACH_STATUS_OKAY(MODEM_OFFLOAD_DEVICE_SWIR_HL78XX)
#undef DT_DRV_COMPAT
