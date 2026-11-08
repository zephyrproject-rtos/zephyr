/*
 * Copyright (c) 2026 Elyorbek Kholikov
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/ztest.h>

#include <mbedtls/ssl_ciphersuites.h>

#define PSK_TAG 1

static const unsigned char psk[] = {0x01, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
				    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
static const char psk_id[] = "test_identity";

static void config_psk(int sock)
{
	sec_tag_t sec_tag_list[] = {PSK_TAG};
	const int cipher_list[] = {
		MBEDTLS_TLS_PSK_WITH_AES_256_CBC_SHA384,
	};

	zassert_ok(zsock_setsockopt(sock, ZSOCK_SOL_TLS, ZSOCK_TLS_SEC_TAG_LIST, sec_tag_list,
				    sizeof(sec_tag_list)));
	zassert_ok(zsock_setsockopt(sock, ZSOCK_SOL_TLS, ZSOCK_TLS_CIPHERSUITE_LIST, cipher_list,
				    sizeof(cipher_list)));
}

/* A listening TLS socket on a free port of the host's loopback interface that
 * leaves the handshake of the sockets it accepts to the socket calls.
 */
static int deferred_handshake_server(struct net_sockaddr_in *addr)
{
	net_socklen_t addrlen = sizeof(*addr);
	int optval = 0;
	int sock;

	*addr = (struct net_sockaddr_in){
		.sin_family = NET_AF_INET,
		.sin_addr = NET_INADDR_LOOPBACK_INIT,
	};

	sock = zsock_socket(NET_AF_INET, NET_SOCK_STREAM, NET_IPPROTO_TLS_1_2);
	zassert_true(sock >= 0, "socket() failed (%d)", errno);
	config_psk(sock);
	zassert_ok(zsock_setsockopt(sock, ZSOCK_SOL_TLS, ZSOCK_TLS_HANDSHAKE_ON_ACCEPT, &optval,
				    sizeof(optval)));
	zassert_ok(zsock_bind(sock, (struct net_sockaddr *)addr, sizeof(*addr)));
	zassert_ok(zsock_listen(sock, 1));
	zassert_ok(zsock_getsockname(sock, (struct net_sockaddr *)addr, &addrlen));

	return sock;
}

/*
 * poll() on a socket whose handshake is pending must not leave the host
 * descriptor registered when it returns: the next poll() registers it again,
 * and the host refuses a second registration.
 */
ZTEST(nsos_tls, test_deferred_handshake_poll_silent_client)
{
	struct net_sockaddr_in addr;
	struct zsock_pollfd pfd;
	int server, client, accepted;

	server = deferred_handshake_server(&addr);

	/* A client that connects at TCP level and sends nothing */
	client = zsock_socket(NET_AF_INET, NET_SOCK_STREAM, NET_IPPROTO_TCP);
	zassert_true(client >= 0, "socket() failed (%d)", errno);
	zassert_ok(zsock_connect(client, (struct net_sockaddr *)&addr, sizeof(addr)));

	accepted = zsock_accept(server, NULL, NULL);
	zassert_true(accepted >= 0, "accept() failed (%d)", errno);

	pfd.fd = accepted;
	pfd.events = ZSOCK_POLLIN;

	for (int i = 0; i < 3; i++) {
		zassert_equal(zsock_poll(&pfd, 1, 10), 0, "Unexpected poll() event");
	}

	zassert_ok(zsock_close(accepted));
	zassert_ok(zsock_close(client));
	zassert_ok(zsock_close(server));
}

static void *nsos_tls_setup(void)
{
	zassert_ok(tls_credential_add(PSK_TAG, TLS_CREDENTIAL_PSK, psk, sizeof(psk)));
	zassert_ok(tls_credential_add(PSK_TAG, TLS_CREDENTIAL_PSK_ID, psk_id, strlen(psk_id)));

	return NULL;
}

ZTEST_SUITE(nsos_tls, NULL, nsos_tls_setup, NULL, NULL, NULL);
