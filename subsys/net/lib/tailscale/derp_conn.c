/*
 * SPDX-FileCopyrightText: FoBE Studio
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdio.h>
#include <strings.h>
#include <mbedtls/platform_util.h>
#include <mbedtls/x509_crt.h>
#include <zephyr/data/json.h>
#include <zephyr/net/socket.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/byteorder.h>
#include "derp_conn.h"
#ifdef CONFIG_TAILSCALE_REQUIRED_PROXY
#include "proxy.h"
#endif
#include "disco_box.h"
#include "underlay_dns.h"

#define CONNECT_MS   5000
#define FRAME_MS     1500
#define BOX_OVERHEAD (DISCO_BOX_NONCE_SIZE + DISCO_BOX_TAG_SIZE)
#define DERP_DNS_STACK_ADDRESSES 4

static int verify_pin(void *ctx, mbedtls_x509_crt *crt, int depth, uint32_t *flags)
{
	uint8_t hash[32];
	size_t len;

	if (depth == 0 && (psa_hash_compute(PSA_ALG_SHA_256, crt->raw.p, crt->raw.len, hash,
					    sizeof(hash), &len) != PSA_SUCCESS ||
			   len != sizeof(hash) || memcmp(ctx, hash, sizeof(hash)) != 0)) {
		*flags |= MBEDTLS_X509_BADCERT_OTHER;
	}
	return 0;
}

static int connect_tls(struct derp_conn *conn, const struct tailscale_derp_node *node,
		       struct net_if *lan, sec_tag_t ca)
{
	const bool pin = strncmp(node->cert_name, "sha256-raw:", 11) == 0;
	const char *name = node->cert_name[0] && !pin ? node->cert_name : node->host;
	struct net_ifreq iface = {0};
	int verify = ZSOCK_TLS_PEER_VERIFY_REQUIRED;
	struct zsock_timeval timeout = {.tv_sec = CONNECT_MS / 1000};
	int rc = net_if_get_name(lan, iface.ifr_name, sizeof(iface.ifr_name));
	struct zsock_tls_cert_verify_cb cb = {.cb = verify_pin, .ctx = conn->pinned_hash};

	if (rc <= 0) {
		return rc < 0 ? rc : -ENODEV;
	}
	if (pin && hex2bin(node->cert_name + 11, 64, conn->pinned_hash, 32) != 32) {
		return -EINVAL;
	}
#ifdef CONFIG_TAILSCALE_REQUIRED_PROXY
	conn->io.deadline = k_uptime_get() + CONNECT_MS * 2;
	return tailscale_proxy_tls(&conn->io, node->host, node->port, name, ca, lan,
				 pin ? verify_pin : NULL, conn->pinned_hash);
#endif
	const char *const hosts[] = {node->ipv4, node->ipv6};
	const int families[] = {NET_AF_INET, NET_AF_INET6};

	for (size_t i = 0; i < ARRAY_SIZE(hosts); i++) {
#if CONFIG_DNS_RESOLVER_AI_MAX_ENTRIES > DERP_DNS_STACK_ADDRESSES
		struct net_sockaddr_storage *addresses;
#else
		struct net_sockaddr_storage addresses[CONFIG_DNS_RESOLVER_AI_MAX_ENTRIES];
#endif
		const net_socklen_t address_size = families[i] == NET_AF_INET
							? sizeof(struct net_sockaddr_in)
							: sizeof(struct net_sockaddr_in6);

		if (strcmp(hosts[i], "none") == 0) {
			continue;
		}
#if CONFIG_DNS_RESOLVER_AI_MAX_ENTRIES > DERP_DNS_STACK_ADDRESSES
		addresses = k_calloc(CONFIG_DNS_RESOLVER_AI_MAX_ENTRIES, sizeof(*addresses));
		if (addresses == NULL) {
			return -ENOMEM;
		}
#endif
		int count = tailscale_underlay_resolve(lan, hosts[i][0] ? hosts[i] : node->host,
						       families[i], node->port, addresses,
						       CONFIG_DNS_RESOLVER_AI_MAX_ENTRIES);

		for (int j = 0; j < count; j++) {
			int fd = zsock_socket(families[i], NET_SOCK_STREAM, NET_IPPROTO_TLS_1_2);

			if (fd < 0) {
				continue;
			}
			if (zsock_setsockopt(fd, ZSOCK_SOL_SOCKET, ZSOCK_SO_BINDTODEVICE, &iface,
					     sizeof(iface)) == 0 &&
			    zsock_setsockopt(fd, ZSOCK_SOL_SOCKET, ZSOCK_SO_SNDTIMEO, &timeout,
					     sizeof(timeout)) == 0 &&
			    zsock_setsockopt(fd, ZSOCK_SOL_SOCKET, ZSOCK_SO_RCVTIMEO, &timeout,
					     sizeof(timeout)) == 0 &&
			    zsock_setsockopt(fd, ZSOCK_SOL_TLS, ZSOCK_TLS_SEC_TAG_LIST, &ca,
					     sizeof(ca)) == 0 &&
			    zsock_setsockopt(fd, ZSOCK_SOL_TLS, ZSOCK_TLS_HOSTNAME, name,
					     strlen(name) + 1U) == 0 &&
			    zsock_setsockopt(fd, ZSOCK_SOL_TLS, ZSOCK_TLS_PEER_VERIFY, &verify,
					     sizeof(verify)) == 0 &&
			    (!pin ||
			     zsock_setsockopt(fd, ZSOCK_SOL_TLS, ZSOCK_TLS_CERT_VERIFY_CALLBACK,
					      &cb, sizeof(cb)) == 0) &&
			    zsock_connect(fd, net_sad(&addresses[j]), address_size) == 0) {
				conn->io = (struct control_io){.fd = fd};
#if CONFIG_DNS_RESOLVER_AI_MAX_ENTRIES > DERP_DNS_STACK_ADDRESSES
				k_free(addresses);
#endif
				return 0;
			}
			rc = -errno;
			zsock_close(fd);
		}
#if CONFIG_DNS_RESOLVER_AI_MAX_ENTRIES > DERP_DNS_STACK_ADDRESSES
		k_free(addresses);
#endif
	}
	return rc < 0 ? rc : -EHOSTUNREACH;
}

int derp_conn_read(struct derp_conn *conn, uint8_t *type, uint8_t data[DERP_FRAME_MAX], size_t *len)
{
	uint8_t header[5];
	int rc;

	conn->io.deadline = k_uptime_get() + FRAME_MS;
	rc = control_io_read(&conn->io, header, sizeof(header));
	if (rc == 0) {
		*type = header[0];
		*len = sys_get_be32(header + 1);
		rc = *len > DERP_FRAME_MAX ? -EMSGSIZE : control_io_read(&conn->io, data, *len);
	}
	if (rc == 0) {
		conn->last_rx = k_uptime_get();
	}
	return rc;
}

int derp_conn_write(struct derp_conn *conn, uint8_t type, const uint8_t *data, size_t len)
{
	uint8_t header[5] = {type};
	int64_t now = k_uptime_get();

	if (len > DERP_FRAME_MAX) {
		return -EMSGSIZE;
	}
	if (type == DERP_SEND && conn->rate != 0U) {
		int64_t elapsed = MIN(now - conn->credit_time, INT32_MAX);
		int64_t cost = (len + sizeof(header)) * 1000LL;

		conn->credit = MIN(conn->burst * 1000LL, conn->credit + elapsed * conn->rate);
		conn->credit_time = now;
		if (cost > conn->credit) {
			return -EAGAIN;
		}
		conn->credit -= cost;
	}
	sys_put_be32(len, header + 1);
	conn->io.deadline = now + FRAME_MS;
	int rc = control_io_write(&conn->io, header, sizeof(header));

	return rc == 0 ? control_io_write(&conn->io, data, len) : rc;
}

struct server_info {
	int32_t version;
	int32_t rate;
	int32_t burst;
};

static const struct json_obj_descr info_fields[] = {
	JSON_OBJ_DESCR_PRIM(struct server_info, version, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct server_info, "TokenBucketBytesPerSecond", rate,
				  JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct server_info, "TokenBucketBytesBurst", burst,
				  JSON_TOK_NUMBER),
};

static int authenticate(struct derp_conn *conn, mbedtls_svc_key_id_t private_key,
			uint8_t buf[DERP_FRAME_MAX])
{
	static const uint8_t magic[] = {'D', 'E', 'R', 'P', 0xf0, 0x9f, 0x94, 0x91};
	static const uint8_t info[] = "{\"version\":2,\"CanAckPings\":true}";
	uint8_t shared[32], key[32], decoded[512];
	static const uint8_t zero_key[sizeof(shared)];
	struct server_info limits = {0};
	uint8_t type;
	size_t len;
	int rc = derp_conn_read(conn, &type, buf, &len);

	if (rc != 0 || type != DERP_SERVER_KEY || len < 40 || memcmp(buf, magic, 8) != 0) {
		return rc != 0 ? rc : -EPROTO;
	}
	rc = control_x25519_agree(private_key, buf + 8, shared);
	if (rc == 0 && memcmp(shared, zero_key, sizeof(shared)) != 0) {
		disco_box_precompute(key, shared);
	} else {
		rc = -EACCES;
	}
	mbedtls_platform_zeroize(shared, sizeof(shared));
	if (rc == 0) {
		rc = control_x25519_public(private_key, buf);
	}
	if (rc == 0) {
		rc = sys_csrand_get(buf + 32, DISCO_BOX_NONCE_SIZE);
	}
	if (rc == 0) {
		rc = disco_box_seal(key, buf + 32, info, sizeof(info) - 1,
				    buf + 32 + DISCO_BOX_NONCE_SIZE);
	}
	if (rc == 0) {
		rc = derp_conn_write(conn, DERP_CLIENT_INFO, buf,
				     32 + BOX_OVERHEAD + sizeof(info) - 1);
	}
	if (rc == 0) {
		rc = derp_conn_read(conn, &type, buf, &len);
	}
	if (rc == 0) {
		rc = type != DERP_SERVER_INFO || len <= BOX_OVERHEAD ||
				     len - BOX_OVERHEAD >= sizeof(decoded)
			     ? -EPROTO
			     : disco_box_open(key, buf, buf + DISCO_BOX_NONCE_SIZE,
					      len - DISCO_BOX_NONCE_SIZE, decoded);
	}
	mbedtls_platform_zeroize(key, sizeof(key));
	if (rc != 0) {
		return rc;
	}
	decoded[len - BOX_OVERHEAD] = 0;
	int64_t fields = json_obj_parse((char *)decoded, len - BOX_OVERHEAD, info_fields,
					ARRAY_SIZE(info_fields), &limits);

	if (fields < 0 || limits.version != 2 || limits.rate < 0 || limits.burst < 0) {
		return -EPROTO;
	}
	conn->rate = limits.rate;
	conn->burst = limits.burst;
	conn->credit = limits.burst * 1000LL;
	conn->credit_time = k_uptime_get();
	return 0;
}

int derp_conn_open(struct derp_conn *conn, const struct tailscale_derp_node *node,
		   struct net_if *lan, sec_tag_t ca, mbedtls_svc_key_id_t private_key, bool home,
		   uint8_t scratch[DERP_FRAME_MAX])
{
	int rc = connect_tls(conn, node, lan, ca);

	if (rc != 0) {
		return rc;
	}
	int n = snprintf(
		(char *)scratch, DERP_FRAME_MAX,
		"GET /derp HTTP/1.1\r\nHost: %s:%u\r\nUpgrade: DERP\r\nConnection: Upgrade\r\n\r\n",
		node->host, node->port);

	conn->io.deadline = k_uptime_get() + CONNECT_MS;
	rc = control_io_write(&conn->io, scratch, n);
	if (rc == 0) {
		rc = control_http_reply(&conn->io, (char *)scratch, DERP_FRAME_MAX, 101);
	}
	if (rc == 0) {
		const char *upgrade = control_http_field((char *)scratch, "Upgrade");
		const char *connection = control_http_field((char *)scratch, "Connection");

		if (upgrade == NULL || strncasecmp(upgrade, "DERP\r\n", 6) != 0 ||
		    connection == NULL || strncasecmp(connection, "Upgrade\r\n", 9) != 0) {
			rc = -EPROTO;
		}
	}
	if (rc == 0) {
		rc = authenticate(conn, private_key, scratch);
	}
	if (rc == 0) {
		const uint8_t preferred = home;

		rc = derp_conn_write(conn, DERP_PREFERRED, &preferred, 1);
	}
	if (rc != 0) {
#ifdef CONFIG_TAILSCALE_REQUIRED_PROXY
		tailscale_proxy_close(&conn->io);
#else
		zsock_close(conn->io.fd);
#endif
		conn->io.fd = -1;
	}
	return rc;
}
