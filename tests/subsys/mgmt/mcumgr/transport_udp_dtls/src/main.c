/*
 * SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/mgmt/mcumgr/transport/smp_udp.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/ztest.h>

/* The server certificate is self-signed: it is also the CA, and a client that
 * holds the same certificate and key is a client the server trusts.
 */
static const unsigned char certificate[] = {
#include "echo-apps-cert.der.inc"
};

static const unsigned char private_key[] = {
#include "echo-apps-key.der.inc"
};

/* Tags of the client: what it trusts, and what it presents. */
#define CLIENT_CA_TAG   10
#define CLIENT_CERT_TAG 11

/* SMP echo request: header, then {"d": "hi"}. */
static const uint8_t echo_request[] = {0x02, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00,
				       0x00, 0xa1, 0x61, 0x64, 0x62, 0x68, 0x69};
/* The response: a write response header, then {"r": "hi"} as an indefinite-length map. */
static const uint8_t echo_reply[] = {0x03, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00,
				     0xbf, 0x61, 0x72, 0x62, 0x68, 0x69, 0xff};

/* Open a DTLS client socket to the transport. */
static int client_connect(bool with_certificate)
{
	struct net_sockaddr_in server = {
		.sin_family = NET_AF_INET,
		.sin_port = net_htons(CONFIG_MCUMGR_TRANSPORT_UDP_PORT),
	};
	sec_tag_t tags[] = {CLIENT_CA_TAG, CLIENT_CERT_TAG};
	struct zsock_timeval timeout = {.tv_sec = 2};
	int verify = ZSOCK_TLS_PEER_VERIFY_NONE;
	int sock;
	int ret;

	zassert_ok(net_addr_pton(NET_AF_INET, "127.0.0.1", &server.sin_addr));

	sock = zsock_socket(NET_AF_INET, NET_SOCK_DGRAM, NET_IPPROTO_DTLS_1_2);
	zassert_true(sock >= 0, "socket: %d", errno);

	zassert_ok(zsock_setsockopt(sock, ZSOCK_SOL_TLS, ZSOCK_TLS_SEC_TAG_LIST, tags,
				    with_certificate ? sizeof(tags) : sizeof(tags[0])));
	zassert_ok(zsock_setsockopt(sock, ZSOCK_SOL_TLS, ZSOCK_TLS_PEER_VERIFY, &verify,
				    sizeof(verify)));
	zassert_ok(zsock_setsockopt(sock, ZSOCK_SOL_SOCKET, ZSOCK_SO_RCVTIMEO, &timeout,
				    sizeof(timeout)));

	ret = zsock_connect(sock, (struct net_sockaddr *)&server, sizeof(server));
	if (ret < 0) {
		ret = -errno;
		(void)zsock_close(sock);
		return ret;
	}

	return sock;
}

static void expect_echo(int sock)
{
	uint8_t reply[64];
	ssize_t len;

	zassert_equal(zsock_send(sock, echo_request, sizeof(echo_request), 0),
		      (ssize_t)sizeof(echo_request), "send: %d", errno);

	len = zsock_recv(sock, reply, sizeof(reply), 0);
	zassert_equal(len, (ssize_t)sizeof(echo_reply), "recv: %d (%d)", (int)len, errno);
	zassert_mem_equal(reply, echo_reply, sizeof(echo_reply));
}

ZTEST(transport_udp_dtls, test_client_with_certificate)
{
	int sock = client_connect(true);

	zassert_true(sock >= 0, "connect: %d", sock);
	expect_echo(sock);
	zassert_ok(zsock_close(sock));
}

ZTEST(transport_udp_dtls, test_client_without_certificate)
{
	int sock = client_connect(false);

	if (IS_ENABLED(CONFIG_MCUMGR_TRANSPORT_UDP_DTLS_CLIENT_AUTH)) {
		zassert_true(sock < 0, "a client without a certificate was accepted");
		return;
	}

	zassert_true(sock >= 0, "connect: %d", sock);
	expect_echo(sock);
	zassert_ok(zsock_close(sock));
}

static void *suite_setup(void)
{
	zassert_ok(tls_credential_add(CONFIG_MCUMGR_TRANSPORT_UDP_DTLS_TLS_TAG,
				      TLS_CREDENTIAL_PUBLIC_CERTIFICATE, certificate,
				      sizeof(certificate)));
	zassert_ok(tls_credential_add(CONFIG_MCUMGR_TRANSPORT_UDP_DTLS_TLS_TAG,
				      TLS_CREDENTIAL_PRIVATE_KEY, private_key,
				      sizeof(private_key)));

	/* With client authentication the transport refuses to start without the CA. */
	if (IS_ENABLED(CONFIG_MCUMGR_TRANSPORT_UDP_DTLS_CLIENT_AUTH)) {
		zassert_equal(smp_udp_open(), -ENOENT, "started without a CA certificate");
	}

	zassert_ok(tls_credential_add(CONFIG_MCUMGR_TRANSPORT_UDP_DTLS_TLS_TAG,
				      TLS_CREDENTIAL_CA_CERTIFICATE, certificate,
				      sizeof(certificate)));

	zassert_ok(tls_credential_add(CLIENT_CA_TAG, TLS_CREDENTIAL_CA_CERTIFICATE, certificate,
				      sizeof(certificate)));
	zassert_ok(tls_credential_add(CLIENT_CERT_TAG, TLS_CREDENTIAL_PUBLIC_CERTIFICATE,
				      certificate, sizeof(certificate)));
	zassert_ok(tls_credential_add(CLIENT_CERT_TAG, TLS_CREDENTIAL_PRIVATE_KEY, private_key,
				      sizeof(private_key)));

	zassert_ok(smp_udp_open());
	/* Let the transport thread bind its socket. */
	k_sleep(K_MSEC(100));

	return NULL;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);
	/* Let the server finish with the session of the previous test. */
	k_sleep(K_MSEC(100));
}

ZTEST_SUITE(transport_udp_dtls, NULL, suite_setup, before, NULL, NULL);
