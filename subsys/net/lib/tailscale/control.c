/*
 * SPDX-FileCopyrightText: FoBE Studio
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <mbedtls/platform_util.h>
#include <zephyr/data/json.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>
#include "tailscale_internal.h"
#include <zephyr/sys/util.h>

#include "client_internal.h"
#include "control.h"
#include "control_internal.h"
#ifdef CONFIG_TAILSCALE_REQUIRED_PROXY
#include "proxy.h"
#endif
#ifdef CONFIG_TAILSCALE_TRANSPORT
#include "transport.h"
#endif
#include "policy.h"
#include "underlay_dns.h"

#define HTTP_HEADER_MAX      4096
#define CONTROL_REPLY_MAX    4096
#define CONTROL_MAP_MAX      (64 * 1024)
#define CONTROL_AUTH_MAX     256
#ifdef CONFIG_TAILSCALE_REQUIRED_PROXY
#define CONTROL_CONNECT_MAX (CONFIG_NET_SOCKETS_CONNECT_TIMEOUT + TAILSCALE_PROXY_SETUP_MS)
#define CONTROL_DNS_MAX     0U
#else
#define CONTROL_CONNECT_MAX \
	(CONFIG_NET_SOCKETS_CONNECT_TIMEOUT + CONFIG_NET_SOCKETS_TLS_CONNECT_TIMEOUT)
#define CONTROL_DNS_MAX CONFIG_NET_SOCKETS_DNS_TIMEOUT
#endif
#define CONTROL_MIN_TIMEOUT_MS (CONTROL_DNS_MAX + 2U * CONTROL_CONNECT_MAX + WATCH_POLL_MS)
#define ENDPOINT_TEXT_SIZE (NET_INET6_ADDRSTRLEN + sizeof("[]:65535"))
#define WIRE_KEY_LEN     (2 * CONTROL_KEY_SIZE + 1)
#define WATCH_IDLE_MS    120000U
#define WATCH_POLL_MS    250
#define WATCH_STACK_SIZE 12288

LOG_MODULE_REGISTER(tailscale_control, LOG_LEVEL_INF);

struct join_state {
	struct control_io io;
	struct control_noise noise;
	struct control_h2 h2;
	char body[CONTROL_MAP_MAX];
};

#if defined(CONFIG_TAILSCALE_POLICY) && defined(CONFIG_SOC_FAMILY_ESPRESSIF_SPIRAM)
static struct join_state join __attribute__((section(".ext_ram.bss")));
#else
static struct join_state join;
#endif
K_MUTEX_DEFINE(join_lock);
K_MUTEX_DEFINE(watch_status_lock);
K_MUTEX_DEFINE(watch_lifecycle_lock);
K_THREAD_STACK_DEFINE(watch_stack, WATCH_STACK_SIZE);

static struct {
	struct k_thread thread;
	struct tailscale_control_stream stream;
	struct tailscale_control_params params;
	struct net_if *lan;
	struct tailscale_watch_status status;
	struct control_h2_body body;
	struct net_in_addr underlay_ipv4;
#ifdef CONFIG_TAILSCALE_TRANSPORT
	char node_hex[WIRE_KEY_LEN];
	char disco_hex[WIRE_KEY_LEN];
	uint16_t published_home;
	struct tailscale_transport_endpoints published_endpoints;
	uint32_t next_status_stream;
#endif
	bool started;
} watch;

struct public_key_reply {
	const char *public_key;
};

static const struct json_obj_descr public_key_fields[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct public_key_reply, "publicKey", public_key,
				  JSON_TOK_STRING),
};

struct early_reply {
	const char *challenge;
};

static const struct json_obj_descr early_fields[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct early_reply, "nodeKeyChallenge", challenge,
				  JSON_TOK_STRING),
};

struct register_reply {
	bool authorized;
	bool expired;
	const char *auth_url;
	const char *error;
	const char *node_key_signature;
};

enum register_operation {
	REGISTER_NODE,
	EXPIRE_NODE,
};

static const struct json_obj_descr register_fields[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct register_reply, "MachineAuthorized", authorized,
				  JSON_TOK_TRUE),
	JSON_OBJ_DESCR_PRIM_NAMED(struct register_reply, "NodeKeyExpired", expired, JSON_TOK_TRUE),
	JSON_OBJ_DESCR_PRIM_NAMED(struct register_reply, "AuthURL", auth_url, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct register_reply, "Error", error, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct register_reply, "NodeKeySignature", node_key_signature,
				  JSON_TOK_STRING),
};

static bool safe_name(const char *name, size_t max_len)
{
	if (name == NULL) {
		return false;
	}
	size_t len = strnlen(name, max_len + 1);

	if (len == 0 || len > max_len) {
		return false;
	}
	for (size_t i = 0; i < len; i++) {
		uint8_t c = name[i];

		if (!(c <= 0x7f && (isalnum(c) || c == '-'))) {
			return false;
		}
	}
	return true;
}

static bool safe_auth_key(const char *key)
{
	if (key == NULL) {
		return true;
	}
	size_t len = strnlen(key, CONTROL_AUTH_MAX + 1);

	if (len > CONTROL_AUTH_MAX) {
		return false;
	}
	for (size_t i = 0; i < len; i++) {
		uint8_t c = key[i];

		if (!(c <= 0x7f && (isalnum(c) || c == '-' || c == '_'))) {
			return false;
		}
	}
	return true;
}

static bool safe_login_url(const char *url)
{
	if (strncmp(url, "https://", sizeof("https://") - 1) != 0) {
		return false;
	}
	for (const char *p = url; *p; p++) {
		if ((unsigned char)*p < 0x21 || (unsigned char)*p > 0x7e) {
			return false;
		}
	}
	return url[sizeof("https://") - 1] != '\0';
}

static int parse_public_key(const char *value, const char *prefix, uint8_t key[CONTROL_KEY_SIZE])
{
	size_t prefix_len = strlen(prefix);

	if (value == NULL || strncmp(value, prefix, prefix_len) != 0 ||
	    strlen(value + prefix_len) != 2 * CONTROL_KEY_SIZE ||
	    hex2bin(value + prefix_len, 2 * CONTROL_KEY_SIZE, key, CONTROL_KEY_SIZE) !=
		    CONTROL_KEY_SIZE) {
		return -EBADMSG;
	}
	return 0;
}

int control_io_socket_timeout(const struct control_io *io, int option)
{
	int64_t remaining = io->deadline - k_uptime_get();

	if (remaining <= 0) {
		return -ETIMEDOUT;
	}
	if (io->stream != NULL) {
		if (tailscale_control_stream_cancelled(io->stream)) {
			return -ECANCELED;
		}
		remaining = MIN(remaining, WATCH_POLL_MS);
	}
	struct zsock_timeval timeout = {
		.tv_sec = remaining / 1000,
		.tv_usec = (remaining % 1000) * 1000,
	};

	return zsock_setsockopt(io->fd, ZSOCK_SOL_SOCKET, option, &timeout, sizeof(timeout)) == 0
		       ? 0
		       : -errno;
}

static int io_error(void)
{
	return errno == EAGAIN || errno == EWOULDBLOCK ? -ETIMEDOUT : -errno;
}

static bool watch_retry_io(const struct control_io *io)
{
	return io->stream != NULL &&
	       (errno == EAGAIN || errno == EWOULDBLOCK || errno == ETIMEDOUT);
}

int control_io_write(struct control_io *io, const void *data, size_t len)
{
	const uint8_t *next = data;

	while (len > 0) {
		int rc = control_io_socket_timeout(io, ZSOCK_SO_SNDTIMEO);

		if (rc != 0) {
			return rc;
		}
#ifdef CONFIG_TAILSCALE_REQUIRED_PROXY
		ssize_t sent = tailscale_proxy_write(io, next, len);
#else
		ssize_t sent = zsock_send(io->fd, next, len, 0);
#endif

		if (sent < 0) {
			if (watch_retry_io(io)) {
				continue;
			}
			return io_error();
		}
		if (sent == 0) {
			return -ECONNRESET;
		}
		next += sent;
		len -= sent;
	}
	return 0;
}

static int io_refill(struct control_io *io)
{
	ssize_t received;

	for (;;) {
		int rc = control_io_socket_timeout(io, ZSOCK_SO_RCVTIMEO);

		if (rc != 0) {
			return rc;
		}
#ifdef CONFIG_TAILSCALE_REQUIRED_PROXY
		received = tailscale_proxy_read(io, io->rx, sizeof(io->rx));
#else
		received = zsock_recv(io->fd, io->rx, sizeof(io->rx), 0);
#endif
		if (received >= 0 || !watch_retry_io(io)) {
			break;
		}
#ifdef CONFIG_TAILSCALE_TRANSPORT
		if (io->reading_noise_header && io->on_idle != NULL) {
			rc = io->on_idle();
			if (rc != 0) {
				return rc;
			}
		}
#endif
	}
	if (received < 0) {
		return io_error();
	}
	io->rx_pos = 0;
	io->rx_len = received;
	return received;
}

int control_io_read(struct control_io *io, void *data, size_t len)
{
	uint8_t *next = data;

	while (len > 0) {
		if (io->rx_pos == io->rx_len) {
			int rc = io_refill(io);

			if (rc <= 0) {
				return rc == 0 ? -ECONNRESET : rc;
			}
		}
		size_t part = MIN(len, io->rx_len - io->rx_pos);

		memcpy(next, io->rx + io->rx_pos, part);
		io->rx_pos += part;
		next += part;
		len -= part;
	}
	return 0;
}

int control_http_reply(struct control_io *io, char *header, size_t cap, int status)
{
	for (size_t i = 0; i < cap - 1; i++) {
		int rc = control_io_read(io, header + i, 1);

		if (rc != 0) {
			return rc;
		}
		if (i >= 3 && memcmp(header + i - 3, "\r\n\r\n", 4) == 0) {
			header[i + 1] = '\0';
			char expected[16];

			(void)snprintf(expected, sizeof(expected), "HTTP/1.1 %d ", status);
			return strncmp(header, expected, strlen(expected)) == 0 ? 0 : -EPROTO;
		}
	}
	return -EMSGSIZE;
}

static int open_control_socket(struct control_io *io, const struct net_sockaddr_in *server,
			       const struct tailscale_control_params *params)
{
	if (io->stream != NULL && tailscale_control_stream_cancelled(io->stream)) {
		return -ECANCELED;
	}
	if (io->deadline - k_uptime_get() < CONTROL_CONNECT_MAX) {
		return -ETIMEDOUT;
	}
#ifdef CONFIG_TAILSCALE_REQUIRED_PROXY
	return tailscale_proxy_tls(io, CONTROL_HOST, CONFIG_TAILSCALE_CONTROL_PORT, CONTROL_HOST,
				 params->ca_tag, params->underlay, NULL, NULL);
#endif
	struct net_ifreq binding = {0};
	int name_len =
		net_if_get_name(params->underlay, binding.ifr_name, sizeof(binding.ifr_name));

	if (name_len <= 0) {
		return name_len < 0 ? name_len : -ENODEV;
	}
	int fd = zsock_socket(NET_AF_INET, NET_SOCK_STREAM, NET_IPPROTO_TLS_1_2);

	if (fd < 0) {
		return -errno;
	}
	int verify = ZSOCK_TLS_PEER_VERIFY_REQUIRED;
	int rc = 0;

	if (zsock_setsockopt(fd, ZSOCK_SOL_SOCKET, ZSOCK_SO_BINDTODEVICE, &binding,
			     sizeof(binding)) < 0 ||
	    zsock_setsockopt(fd, ZSOCK_SOL_TLS, ZSOCK_TLS_SEC_TAG_LIST, &params->ca_tag,
			     sizeof(params->ca_tag)) < 0 ||
	    zsock_setsockopt(fd, ZSOCK_SOL_TLS, ZSOCK_TLS_HOSTNAME, CONTROL_HOST,
			     sizeof(CONTROL_HOST)) < 0 ||
	    zsock_setsockopt(fd, ZSOCK_SOL_TLS, ZSOCK_TLS_PEER_VERIFY, &verify, sizeof(verify)) <
		    0 ||
	    zsock_connect(fd, (const struct net_sockaddr *)server, sizeof(*server)) < 0) {
		rc = -errno;
		(void)zsock_close(fd);
		return rc;
	}
	io->fd = fd;
	io->rx_pos = 0;
	io->rx_len = 0;
	return 0;
}

const char *control_http_field(const char *headers, const char *name)
{
	const char *line = strstr(headers, "\r\n");
	size_t name_len = strlen(name);

	while (line != NULL && line[2] != '\r') {
		line += 2;
		if (strncasecmp(line, name, name_len) == 0 && line[name_len] == ':') {
			line += name_len + 1;
			while (*line == ' ' || *line == '\t') {
				line++;
			}
			return line;
		}
		line = strstr(line, "\r\n");
	}
	return NULL;
}

static int read_chunks(struct control_io *io, char *out, size_t cap, size_t *out_len)
{
	size_t used = 0;

	for (;;) {
		char size_line[64];
		size_t len = 0;
		int rc;

		do {
			if (len >= sizeof(size_line) - 1) {
				return -EMSGSIZE;
			}
			rc = control_io_read(io, size_line + len++, 1);
			if (rc != 0) {
				return rc;
			}
		} while (len < 2 || memcmp(size_line + len - 2, "\r\n", 2) != 0);
		size_line[len] = '\0';
		char *end;
		unsigned long chunk = strtoul(size_line, &end, 16);

		if (!isxdigit((unsigned char)size_line[0]) || (*end != ';' && *end != '\r')) {
			return -EBADMSG;
		}
		if (chunk == 0) {
			*out_len = used;
			return 0;
		}
		if (chunk >= cap - used) {
			return -EMSGSIZE;
		}
		rc = control_io_read(io, out + used, chunk);
		if (rc == 0) {
			rc = control_io_read(io, size_line, 2);
		}
		if (rc != 0 || memcmp(size_line, "\r\n", 2) != 0) {
			return rc != 0 ? rc : -EBADMSG;
		}
		used += chunk;
	}
}

static int read_key_body(struct control_io *io, char *header, char *body, size_t cap, size_t *len)
{
	const char *transfer = control_http_field(header, "Transfer-Encoding");
	const char *length = control_http_field(header, "Content-Length");

	if (transfer != NULL) {
		if (strncasecmp(transfer, "chunked\r\n", sizeof("chunked\r\n") - 1) != 0) {
			return -ENOTSUP;
		}
		return read_chunks(io, body, cap, len);
	}
	if (length != NULL) {
		char *end;
		unsigned long value = strtoul(length, &end, 10);

		if (!isdigit((unsigned char)length[0]) || memcmp(end, "\r\n", 2) != 0 ||
		    value >= cap) {
			return -EBADMSG;
		}
		*len = value;
		return control_io_read(io, body, *len);
	}
	*len = 0;
	for (;;) {
		size_t buffered = io->rx_len - io->rx_pos;

		if (buffered >= cap - *len) {
			return -EMSGSIZE;
		}
		memcpy(body + *len, io->rx + io->rx_pos, buffered);
		*len += buffered;
		int rc = io_refill(io);

		if (rc <= 0) {
			return rc;
		}
	}
}

static int fetch_server_key(struct join_state *state, uint8_t server_key[CONTROL_KEY_SIZE])
{
	static const char request[] = "GET /key?v=" CONTROL_VERSION_STRING " HTTP/1.1\r\n"
				      "Host: " CONTROL_HOST "\r\nConnection: close\r\n\r\n";
	size_t len = 0;
	int rc = control_io_write(&state->io, request, sizeof(request) - 1);

	if (rc == 0) {
		rc = control_http_reply(&state->io, (char *)state->noise.record, HTTP_HEADER_MAX,
					200);
	}
	if (rc == 0) {
		rc = read_key_body(&state->io, (char *)state->noise.record, state->body,
				   CONTROL_REPLY_MAX, &len);
	}
	if (rc != 0) {
		return rc;
	}
	struct public_key_reply reply = {0};

	state->body[len] = '\0';
	if (json_obj_parse(state->body, len, public_key_fields, ARRAY_SIZE(public_key_fields),
			   &reply) != BIT(0)) {
		return -EBADMSG;
	}
	return parse_public_key(reply.public_key, "mkey:", server_key);
}

/* Zephyr's object parser rejects null fields before it can skip them. */
static void normalize_json_nulls(char *body, size_t len)
{
	bool quoted = false;

	for (size_t i = 0; i < len; ++i) {
		if (body[i] == '"') {
			quoted = !quoted;
		} else if (quoted && body[i] == '\\' && i + 1 < len) {
			++i;
		} else if (!quoted && body[i] == 'n' && len - i >= 4 &&
			   (i == 0 || strchr(":,[ \t\r\n", body[i - 1]) != NULL) &&
			   (i + 4 == len || strchr("}], \t\r\n", body[i + 4]) != NULL) &&
			   memcmp(body + i, "null", 4) == 0) {
			memcpy(body + i, "\"\"  ", 4);
			i += 3;
		}
	}
}

static int register_node(struct join_state *state, const struct tailscale_control_params *params,
			 mbedtls_svc_key_id_t node, const char node_hex[WIRE_KEY_LEN],
			 size_t early_len, enum register_operation operation)
{
	char challenge_field[sizeof(",\"NodeKeyChallengeResponse\":\"chalresp:\"") + WIRE_KEY_LEN];
	const char *key = params->auth_key;
	bool use_auth = operation == REGISTER_NODE && key != NULL && *key != '\0';
	const char *expiry = operation == EXPIRE_NODE ? ",\"Expiry\":\"1970-01-01T00:02:03Z\"" : "";
	const char *ephemeral =
		params->ephemeral && operation == REGISTER_NODE ? ",\"Ephemeral\":true" : "";
	int rc;

	challenge_field[0] = '\0';
	if (early_len != 0) {
		struct early_reply early = {0};
		uint8_t challenge[CONTROL_KEY_SIZE];
		uint8_t response[CONTROL_KEY_SIZE];
		char response_hex[WIRE_KEY_LEN];

		if (json_obj_parse(state->body, early_len, early_fields, ARRAY_SIZE(early_fields),
				   &early) < 0) {
			return -EBADMSG;
		}
		if (early.challenge != NULL) {
			rc = parse_public_key(early.challenge, "chalpub:", challenge);
			if (rc != 0) {
				return rc;
			}
			rc = control_x25519_agree(node, challenge, response);
			if (rc == 0) {
				(void)bin2hex(response, sizeof(response), response_hex,
					      sizeof(response_hex));
				(void)snprintf(challenge_field, sizeof(challenge_field),
					       ",\"NodeKeyChallengeResponse\":\"chalresp:%s\"",
					       response_hex);
			}
			mbedtls_platform_zeroize(response, sizeof(response));
			mbedtls_platform_zeroize(response_hex, sizeof(response_hex));
			if (rc != 0) {
				return rc;
			}
		}
	}
	int len = snprintf(
		state->body, CONTROL_REPLY_MAX,
		"{\"Version\":" CONTROL_VERSION_STRING ",\"NodeKey\":\"nodekey:%s\","
		"\"Hostinfo\":{\"Hostname\":\"%s\",\"OS\":\"linux\",\"OSVersion\":\"Zephyr\"}"
		"%s%s%s%s%s%s}",
		node_hex, params->hostname, challenge_field, expiry, ephemeral,
		use_auth ? ",\"Auth\":{\"AuthKey\":\"" : "", use_auth ? key : "",
		use_auth ? "\"}" : "");

	mbedtls_platform_zeroize(challenge_field, sizeof(challenge_field));
	if (len < 0 || len >= CONTROL_REPLY_MAX) {
		return -EMSGSIZE;
	}
	rc = control_h2_request(&state->h2, 1, "/machine/register", node_hex, state->body, len);
	mbedtls_platform_zeroize(state->body, len);
	if (rc != 0) {
		return rc;
	}
	size_t reply_len;
	struct register_reply reply = {0};

	rc = control_h2_response(&state->h2, 1, state->body, CONTROL_REPLY_MAX, &reply_len);
	if (rc != 0) {
		return rc;
	}
	normalize_json_nulls(state->body, reply_len);
	if (json_obj_parse(state->body, reply_len, register_fields, ARRAY_SIZE(register_fields),
			   &reply) < 0) {
		return -EBADMSG;
	}
	if (reply.node_key_signature != NULL && *reply.node_key_signature != '\0') {
		return -ENOTSUP;
	}
	if (reply.error != NULL && *reply.error != '\0') {
		return -EACCES;
	}
	if (operation == EXPIRE_NODE) {
		/* Expiry of this node's key is a successful logout response. */
		return reply.expired || (reply.authorized &&
					 (reply.auth_url == NULL || *reply.auth_url == '\0'))
			       ? 0
			       : -EACCES;
	}
	if (reply.expired) {
		return -EACCES;
	}
	if (reply.auth_url != NULL && *reply.auth_url != '\0') {
		if (!safe_login_url(reply.auth_url)) {
			return -EBADMSG;
		}
		if (params->login_required != NULL) {
			params->login_required(reply.auth_url, params->login_context);
		}
		return -EINPROGRESS;
	}
	return reply.authorized ? 0 : -EACCES;
}

static bool valid_endpoint(const struct net_sockaddr_storage *endpoint)
{
	if (endpoint->ss_family == NET_AF_INET) {
		const struct net_sockaddr_in *ipv4 = net_sin(net_sad(endpoint));
		uint32_t address = net_ntohl(ipv4->sin_addr.s_addr);

		return ipv4->sin_port != 0 && address != 0 && address < 0xe0000000 &&
		       (address & 0xff000000) != 0x7f000000;
	}
#if defined(CONFIG_NET_IPV6)
	if (endpoint->ss_family == NET_AF_INET6) {
		const struct net_sockaddr_in6 *ipv6 = net_sin6(net_sad(endpoint));
		const struct net_in6_addr *address = &ipv6->sin6_addr;

		return ipv6->sin6_port != 0 && !net_ipv6_is_addr_unspecified(address) &&
		       !net_ipv6_is_addr_loopback(address) && !net_ipv6_is_addr_mcast(address) &&
		       !net_ipv6_is_ll_addr(address);
	}
#endif
	return false;
}

static int format_endpoint(const struct net_sockaddr_storage *endpoint, char *text, size_t cap)
{
	char address[NET_INET6_ADDRSTRLEN];
	int len;

	if (endpoint->ss_family == NET_AF_INET) {
		const struct net_sockaddr_in *ipv4 = net_sin(net_sad(endpoint));

		if (zsock_inet_ntop(NET_AF_INET, &ipv4->sin_addr, address, sizeof(address)) ==
		    NULL) {
			return -EINVAL;
		}
		len = snprintf(text, cap, "%s:%u", address, net_ntohs(ipv4->sin_port));
	} else {
		const struct net_sockaddr_in6 *ipv6 = net_sin6(net_sad(endpoint));

		if (zsock_inet_ntop(NET_AF_INET6, &ipv6->sin6_addr, address, sizeof(address)) ==
		    NULL) {
			return -EINVAL;
		}
		len = snprintf(text, cap, "[%s]:%u", address, net_ntohs(ipv6->sin6_port));
	}
	return len >= 0 && len < cap ? 0 : -EMSGSIZE;
}

static int get_public_hex(mbedtls_svc_key_id_t key, char encoded[WIRE_KEY_LEN])
{
	uint8_t public_key[CONTROL_KEY_SIZE];
	int rc = control_x25519_public(key, public_key);

	if (rc == 0) {
		(void)bin2hex(public_key, sizeof(public_key), encoded, WIRE_KEY_LEN);
	}
	return rc;
}

static int resolve_control(struct control_io *io, struct net_if *iface,
			   struct net_sockaddr_in *server)
{
	struct net_sockaddr_storage address;

	if (io->stream != NULL && tailscale_control_stream_cancelled(io->stream)) {
		return -ECANCELED;
	}
	if (io->deadline - k_uptime_get() < CONFIG_NET_SOCKETS_DNS_TIMEOUT) {
		return -ETIMEDOUT;
	}
	int rc = tailscale_underlay_resolve(iface, CONTROL_HOST, NET_AF_INET,
					   CONFIG_TAILSCALE_CONTROL_PORT, &address, 1);

	if (rc < 0) {
		return rc == -ETIMEDOUT ? rc : -EHOSTUNREACH;
	}
	*server = *net_sin(net_sad(&address));
	return 0;
}

static void close_control_socket(struct control_io *io)
{
	if (io->fd >= 0) {
#ifdef CONFIG_TAILSCALE_REQUIRED_PROXY
		tailscale_proxy_close(io);
#else
		(void)zsock_close(io->fd);
#endif
		io->fd = -1;
	}
}

static int connect_control_session(struct join_state *state,
				   const struct tailscale_control_params *params,
				   char node_hex[WIRE_KEY_LEN], char disco_hex[WIRE_KEY_LEN],
				   const char **stage, bool *authority_lost,
				   enum register_operation operation)
{
	mbedtls_svc_key_id_t machine = MBEDTLS_SVC_KEY_ID_INIT;
	mbedtls_svc_key_id_t node = MBEDTLS_SVC_KEY_ID_INIT;
	mbedtls_svc_key_id_t disco = MBEDTLS_SVC_KEY_ID_INIT;
	struct net_sockaddr_in server;
	uint8_t server_key[CONTROL_KEY_SIZE];
	size_t early_len;
	*stage = "trusted realtime";
	int rc = tailscale_realtime_now(NULL);

	state->io.fd = -1;
	if (rc != 0) {
		if (authority_lost != NULL) {
			*authority_lost = true;
		}
		return rc;
	}
	*stage = "key setup";
	rc = psa_crypto_init() == PSA_SUCCESS ? 0 : -EIO;
	state->io.deadline = k_uptime_get() + params->timeout_ms;
	state->noise.io = &state->io;
	state->h2.noise = &state->noise;
	if (rc == 0) {
		rc = control_x25519_import(params->machine_private, &machine);
	}
	if (rc == 0) {
		rc = control_x25519_import(params->node_private, &node);
	}
	if (rc == 0) {
		rc = get_public_hex(node, node_hex);
	}
	if (rc == 0) {
		rc = control_x25519_import(params->disco_private, &disco);
	}
	if (rc == 0) {
		rc = get_public_hex(disco, disco_hex);
	}
	(void)psa_destroy_key(disco);
	if (rc != 0) {
		goto out;
	}

	*stage = "HTTPS control-key fetch";
	rc = IS_ENABLED(CONFIG_TAILSCALE_REQUIRED_PROXY) ?
		     0 : resolve_control(&state->io, params->underlay, &server);
	if (rc == 0) {
		rc = open_control_socket(&state->io, &server, params);
	}
	if (rc == 0) {
		rc = fetch_server_key(state, server_key);
	}
	close_control_socket(&state->io);
	if (rc != 0) {
		goto out;
	}

	*stage = "TS2021 handshake";
	rc = open_control_socket(&state->io, &server, params);
	if (rc == 0) {
		rc = control_noise_connect(&state->noise, machine, server_key);
	}
	if (rc != 0) {
		goto out;
	}

	*stage = "HTTP/2 preface";
	rc = control_h2_start(&state->h2, state->body, CONTROL_REPLY_MAX, &early_len);
	if (rc != 0) {
		goto out;
	}

	*stage = operation == EXPIRE_NODE ? "node expiry" : "node registration";
	rc = register_node(state, params, node, node_hex, early_len, operation);
	if (rc != 0) {
		if (authority_lost != NULL) {
			*authority_lost = rc == -EACCES || rc == -EINPROGRESS || rc == -ENOTSUP ||
					  rc == -EBADMSG;
		}
		goto out;
	}
	LOG_INF("Control plane accepted %s", *stage);
out:
	(void)psa_destroy_key(machine);
	(void)psa_destroy_key(node);
	return rc;
}

static void close_control_session(struct join_state *state)
{
	close_control_socket(&state->io);
	(void)psa_destroy_key(state->noise.tx_key);
	(void)psa_destroy_key(state->noise.rx_key);
	mbedtls_platform_zeroize(state, sizeof(*state));
}

bool tailscale_control_registration_valid(const char *hostname, const char *auth_key,
					uint32_t timeout_ms)
{
	return safe_name(hostname, TAILSCALE_HOSTNAME_MAX) && safe_auth_key(auth_key) &&
	       timeout_ms >= CONTROL_MIN_TIMEOUT_MS;
}

static int control_register(const struct tailscale_control_params *params,
			    enum register_operation operation)
{
	char node_hex[WIRE_KEY_LEN];
	char disco_hex[WIRE_KEY_LEN];
	const char *stage;
	int rc;

	if (params == NULL || params->underlay == NULL || params->machine_private == NULL ||
	    params->node_private == NULL || params->disco_private == NULL ||
	    !tailscale_control_registration_valid(params->hostname, params->auth_key,
						  params->timeout_ms)) {
		return -EINVAL;
	}
	if (k_mutex_lock(&join_lock, K_NO_WAIT) != 0) {
		return -EBUSY;
	}
	rc = connect_control_session(&join, params, node_hex, disco_hex, &stage, NULL, operation);
	if (rc != 0 && rc != -EINPROGRESS) {
		LOG_ERR("%s failed (%d)", stage, rc);
	}
	close_control_session(&join);
	k_mutex_unlock(&join_lock);
	return rc;
}

int tailscale_control_enroll(const struct tailscale_control_params *params)
{
	return control_register(params, REGISTER_NODE);
}

int tailscale_control_logout(const struct tailscale_control_params *params)
{
	return control_register(params, EXPIRE_NODE);
}

static int start_watch_policy(const struct tailscale_control_params *params)
{
	mbedtls_svc_key_id_t node = MBEDTLS_SVC_KEY_ID_INIT;
	uint8_t public_key[CONTROL_KEY_SIZE];
	size_t len;
	int index = net_if_get_by_name("wg0");
	int rc;

	if (index < 0) {
		return index;
	}
	rc = control_x25519_import(params->node_private, &node);
	if (rc == 0 &&
	    (psa_export_public_key(node, public_key, sizeof(public_key), &len) != PSA_SUCCESS ||
	     len != sizeof(public_key))) {
		rc = -EIO;
	}
	(void)psa_destroy_key(node);
	if (rc == 0) {
		rc = tailscale_client_policy_start_hosted(net_if_get_by_index(index),
							  params->node_private, public_key);
	}
	return rc;
}

static bool watch_ipv4_address(struct net_in_addr *address)
{
	bool available = false;

	net_if_lock(watch.lan);
	if (net_if_is_up(watch.lan)) {
		const struct net_in_addr *current =
			net_if_ipv4_get_global_addr(watch.lan, NET_ADDR_PREFERRED);

		if (current != NULL) {
			*address = *current;
			available = true;
		}
	}
	net_if_unlock(watch.lan);
	return available;
}

#ifdef CONFIG_TAILSCALE_TRANSPORT
static int format_watch_endpoints(struct control_h2_map_request *request,
				  const struct tailscale_transport_endpoints *endpoints,
				  char text[3][ENDPOINT_TEXT_SIZE])
{
	const struct net_sockaddr_storage *const sockets[] = {
		&endpoints->local_ipv4, &endpoints->local_ipv6, &endpoints->public_ipv4,
	};
	const char **const formatted[] = {
		&request->local_ipv4, &request->local_ipv6, &request->public_endpoint,
	};

	for (size_t i = 0; i < ARRAY_SIZE(sockets); i++) {
		if (sockets[i]->ss_family == NET_AF_UNSPEC) {
			continue;
		}
		int rc = format_endpoint(sockets[i], text[i], ENDPOINT_TEXT_SIZE);

		if (rc != 0) {
			return rc;
		}
		*formatted[i] = text[i];
	}
	return 0;
}
#endif

static int open_watch(void *user_data)
{
	const struct tailscale_control_params *params = user_data;
	char node_hex[WIRE_KEY_LEN];
	char disco_hex[WIRE_KEY_LEN];
	const char *stage;
	bool authority_lost = false;
	int rc;

	if (!watch_ipv4_address(&watch.underlay_ipv4)) {
		k_mutex_lock(&watch_status_lock, K_FOREVER);
		watch.status.stage = "Underlay address";
		k_mutex_unlock(&watch_status_lock);
		join.io.fd = -1;
		return -ENETDOWN;
	}

	join.io.stream = &watch.stream;
	rc = connect_control_session(&join, params, node_hex, disco_hex, &stage, &authority_lost,
				     REGISTER_NODE);
	if (authority_lost) {
		int revoke_rc = tailscale_client_policy_revoke();

		if (revoke_rc != 0) {
			LOG_ERR("Policy withdrawal failed (%d)", revoke_rc);
		}
	}
	if (rc == 0) {
		struct control_h2_map_request request = {
			.node_hex = node_hex,
			.disco_hex = disco_hex,
			.hostname = params->hostname,
		};

		stage = "endpoint publication and map stream";
#ifdef CONFIG_TAILSCALE_TRANSPORT
		char text[3][ENDPOINT_TEXT_SIZE];

		tailscale_transport_get_endpoints(&watch.published_endpoints);
		rc = format_watch_endpoints(&request, &watch.published_endpoints, text);
		strcpy(watch.node_hex, node_hex);
		strcpy(watch.disco_hex, disco_hex);
		watch.published_home = 0;
		watch.next_status_stream = 7;
		join.h2.status_stream = 0;
#else
		char text[ENDPOINT_TEXT_SIZE];
		struct net_sockaddr_storage endpoint = params->local_endpoint;

		net_sin(net_sad(&endpoint))->sin_addr = watch.underlay_ipv4;
		rc = format_endpoint(&endpoint, text, sizeof(text));
		request.local_ipv4 = text;
#endif
		if (rc == 0) {
			rc = control_h2_watch_open(&join.h2, &request, &watch.body, join.body,
						   CONTROL_REPLY_MAX);
		}
	}

	k_mutex_lock(&watch_status_lock, K_FOREVER);
	watch.status.stage = stage;
	k_mutex_unlock(&watch_status_lock);
	return rc;
}

#ifdef CONFIG_TAILSCALE_TRANSPORT
static int publish_watch_path(void)
{
	struct tailscale_transport_endpoints endpoints;
	struct net_in_addr address;
	uint16_t home = tailscale_transport_home();

	if (!watch_ipv4_address(&address) ||
	    !net_ipv4_addr_cmp(&watch.underlay_ipv4, &address)) {
		return -ENETDOWN;
	}
	if (join.h2.status_stream != 0U) {
		return 0;
	}
	tailscale_transport_get_endpoints(&endpoints);
	if (watch.published_home != home ||
	    memcmp(&endpoints, &watch.published_endpoints, sizeof(endpoints)) != 0) {
		struct control_h2_map_request update = {
			.node_hex = watch.node_hex,
			.disco_hex = watch.disco_hex,
			.hostname = watch.params.hostname,
		};
		char text[3][ENDPOINT_TEXT_SIZE];
		int rc = format_watch_endpoints(&update, &endpoints, text);

		if (rc != 0) {
			return rc;
		}
		if (watch.next_status_stream > INT32_MAX) {
			return -EOVERFLOW;
		}
		/* join.body may contain a partially received map. */
		char publication[640];

		rc = control_h2_publish_endpoint(&join.h2, watch.next_status_stream, &update,
						  home, publication, sizeof(publication));
		if (rc != 0) {
			return rc;
		}
		join.h2.status_stream = watch.next_status_stream;
		join.h2.status_headers = false;
		watch.next_status_stream += 2;
		watch.published_home = home;
		watch.published_endpoints = endpoints;
	}
	return 0;
}
#endif

static int read_watch(int64_t deadline, const uint8_t **data, size_t *len, void *user_data)
{
	ARG_UNUSED(user_data);
	join.io.deadline = deadline;
#ifdef CONFIG_TAILSCALE_TRANSPORT
	join.io.on_idle = publish_watch_path;
	int rc = publish_watch_path();

	if (rc != 0) {
		return rc;
	}
#endif
	return control_h2_read_body(&join.h2, &watch.body, data, len);
}

static void close_watch(void *user_data)
{
#ifdef CONFIG_TAILSCALE_TRANSPORT
	join.io.on_idle = NULL;
#endif
	ARG_UNUSED(user_data);
	close_control_session(&join);
}

static int apply_watch(uint32_t generation, uint8_t *json, size_t len, void *user_data)
{
	enum tailscale_peer_inventory_frame frame;
	int rc;

	ARG_UNUSED(user_data);
	if (tailscale_control_stream_cancelled(&watch.stream)) {
		return -ECANCELED;
	}

	if (tailscale_realtime_now(NULL) != 0) {
		rc = tailscale_client_policy_revoke();
		if (rc != 0) {
			LOG_ERR("Policy withdrawal failed (%d)", rc);
		}
		return -ENODATA;
	}
	k_mutex_lock(&watch_status_lock, K_FOREVER);
	watch.status.stage = "installing policy";
	k_mutex_unlock(&watch_status_lock);
	rc = tailscale_client_policy_apply(generation, json, len, &frame);

	k_mutex_lock(&watch_status_lock, K_FOREVER);
	if (tailscale_control_stream_cancelled(&watch.stream)) {
		rc = -ECANCELED;
		goto out;
	}
	if (rc != 0) {
		goto out;
	}

	watch.status.stage = "receiving";

	switch (frame) {
	case TAILSCALE_PEER_INVENTORY_KEEPALIVE:
		watch.status.keepalives++;
		break;
	case TAILSCALE_PEER_INVENTORY_FULL:
		watch.status.full_maps++;
		break;
	case TAILSCALE_PEER_INVENTORY_UPDATE:
		watch.status.updates++;
		break;
	case TAILSCALE_PEER_INVENTORY_OTHER:
		break;
	}
out:
	k_mutex_unlock(&watch_status_lock);
	return rc;
}

static void watch_event(uint32_t generation, enum tailscale_control_stream_event event, int reason,
			void *user_data)
{
	ARG_UNUSED(user_data);
	k_mutex_lock(&watch_status_lock, K_FOREVER);
	watch.status.generation = generation;
	if (event == TAILSCALE_CONTROL_STREAM_CLOSED) {
		if (reason != -ECANCELED) {
			if (reason != watch.status.last_error) {
				LOG_WRN("Control stream %s failed (%d)", watch.status.stage,
					reason);
			}
			watch.status.last_error = reason;
		}
	}
	if (watch.status.state != TAILSCALE_WATCH_STOPPING) {
		switch (event) {
		case TAILSCALE_CONTROL_STREAM_OPENING:
			watch.status.state = TAILSCALE_WATCH_CONNECTING;
			watch.status.stage = "opening";
			break;
		case TAILSCALE_CONTROL_STREAM_RECEIVING:
			watch.status.state = TAILSCALE_WATCH_RECEIVING;
			watch.status.stage = "receiving";
			break;
		case TAILSCALE_CONTROL_STREAM_CLOSED:
			watch.status.state = TAILSCALE_WATCH_RECONNECTING;
			break;
		}
	}
	k_mutex_unlock(&watch_status_lock);
}

static void run_watch(void *a, void *b, void *c)
{
	static const struct tailscale_control_stream_config config = {
		.frame = (uint8_t *)join.body,
		.capacity = sizeof(join.body),
		.idle_ms = WATCH_IDLE_MS,
		.retry_min_ms = 1000U,
		.retry_max_ms = 10000U,
	};
	static const struct tailscale_control_stream_ops ops = {
		.open = open_watch, .read = read_watch, .close = close_watch, .map = apply_watch,
		.event = watch_event,
	};
	int rc;

	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);
	if (k_mutex_lock(&join_lock, K_NO_WAIT) == 0) {
		rc = tailscale_control_stream_run(&watch.stream, &config, &ops, &watch.params);
		k_mutex_unlock(&join_lock);
	} else {
		rc = -EBUSY;
	}

	int stop_rc = tailscale_client_policy_stop();

	if (stop_rc != 0) {
		LOG_ERR("Policy shutdown failed (%d)", stop_rc);
		if (rc == 0) {
			rc = stop_rc;
		}
	}
	k_mutex_lock(&watch_status_lock, K_FOREVER);
	watch.status.state = rc == 0 ? TAILSCALE_WATCH_STOPPED : TAILSCALE_WATCH_FAILED;
	watch.status.valid = false;
	watch.status.peers = 0;
	if (rc != 0) {
		watch.status.last_error = rc;
	}
	watch.params = (struct tailscale_control_params){0};
	k_mutex_unlock(&watch_status_lock);
}

int tailscale_control_watch_start(const struct tailscale_control_params *params, struct net_if *lan)
{
	int rc = 0;

	if (params == NULL || lan == NULL || params->machine_private == NULL ||
	    params->node_private == NULL || params->local_endpoint.ss_family != NET_AF_INET ||
	    params->disco_private == NULL || params->auth_key != NULL ||
	    !tailscale_control_registration_valid(params->hostname, NULL, params->timeout_ms) ||
	    !valid_endpoint(&params->local_endpoint)) {
		return -EINVAL;
	}

	k_mutex_lock(&watch_lifecycle_lock, K_FOREVER);
	if (watch.started && k_thread_join(&watch.thread, K_NO_WAIT) != 0) {
		rc = -EBUSY;
		goto out;
	}
	tailscale_client_policy_set_underlay(lan);
	rc = start_watch_policy(params);
#ifdef CONFIG_TAILSCALE_TRANSPORT
	if (rc == 0) {
		rc = tailscale_transport_start(params, lan);
		if (rc != 0) {
			(void)tailscale_client_policy_stop();
		}
	}
#endif
	if (rc != 0) {
		goto out;
	}
	if (!watch.started) {
		tailscale_control_stream_init(&watch.stream);
		watch.started = true;
	}
	tailscale_control_stream_prepare(&watch.stream);
	watch.params = *params;
	watch.params.underlay = lan;
	watch.lan = lan;

	k_mutex_lock(&watch_status_lock, K_FOREVER);
	watch.status = (struct tailscale_watch_status){
		.state = TAILSCALE_WATCH_CONNECTING, .stage = "opening",
	};
	k_mutex_unlock(&watch_status_lock);

	k_thread_create(&watch.thread, watch_stack, K_THREAD_STACK_SIZEOF(watch_stack), run_watch,
			NULL, NULL, NULL, K_PRIO_PREEMPT(7), 0, K_NO_WAIT);
	(void)k_thread_name_set(&watch.thread, "tailnet-control");
out:
	k_mutex_unlock(&watch_lifecycle_lock);
	return rc;
}

int tailscale_control_watch_stop(void)
{
	int rc = 0;

	k_mutex_lock(&watch_lifecycle_lock, K_FOREVER);
	if (watch.started && k_thread_join(&watch.thread, K_NO_WAIT) != 0) {
		k_mutex_lock(&watch_status_lock, K_FOREVER);
		if (watch.status.state != TAILSCALE_WATCH_STOPPED &&
		    watch.status.state != TAILSCALE_WATCH_FAILED) {
			watch.status.state = TAILSCALE_WATCH_STOPPING;
		}
		tailscale_control_stream_cancel(&watch.stream);
		k_mutex_unlock(&watch_status_lock);
	}
	int stop_rc = tailscale_client_policy_stop();

	k_mutex_lock(&watch_status_lock, K_FOREVER);
	watch.status.valid = false;
	watch.status.peers = 0;
	k_mutex_unlock(&watch_status_lock);
	if (watch.started) {
		rc = k_thread_join(&watch.thread, K_MSEC(CONTROL_DNS_MAX + CONTROL_CONNECT_MAX +
							2 * WATCH_POLL_MS));
	}
	if (rc == 0) {
		rc = stop_rc;
	}
	if (rc == 0) {
		k_mutex_lock(&watch_status_lock, K_FOREVER);
		watch.status.state = TAILSCALE_WATCH_STOPPED;
		k_mutex_unlock(&watch_status_lock);
	}
	k_mutex_unlock(&watch_lifecycle_lock);
	return rc;
}

void tailscale_control_watch_status(struct tailscale_watch_status *status)
{
	k_mutex_lock(&watch_status_lock, K_FOREVER);
	*status = watch.status;
	k_mutex_unlock(&watch_status_lock);
	struct tailscale_peer_inventory_summary summary;

	(void)tailscale_client_policy_summary(&summary);
	status->valid = summary.valid;
	status->peers = summary.peer_count;
	status->installed_generation = summary.generation;
	status->revision = summary.revision;
}
