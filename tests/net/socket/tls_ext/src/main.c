/*
 * Copyright (c) 2020 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_log.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include <mbedtls/x509.h>
#include <mbedtls/x509_crt.h>

LOG_MODULE_REGISTER(tls_test, CONFIG_NET_SOCKETS_LOG_LEVEL);

/**
 * @brief An encrypted message to pass between server and client.
 *
 * The answer to life, the universe, and everything.
 *
 * See also <a href="https://en.wikipedia.org/wiki/42_(number)#The_Hitchhiker's_Guide_to_the_Galaxy">42</a>.
 */
#define SECRET "forty-two"

/**
 * @brief Size of the encrypted message passed between server and client.
 */
#define SECRET_SIZE (sizeof(SECRET) - 1)

/** @brief Stack size for the server thread */
#define STACK_SIZE 8192

#define MY_IPV4_ADDR "127.0.0.1"

/** @brief TCP port for the server thread */
#define PORT 4242

/** @brief arbitrary timeout value in ms */
#define TIMEOUT 1000

/**
 * @brief Application-dependent TLS credential identifiers
 *
 * Since both the server and client exist in the same test
 * application in this case, both the server and client credentials
 * are loaded together.
 *
 * The server would normally need
 * - SERVER_CERTIFICATE_TAG (for both public and private keys)
 * - CA_CERTIFICATE_TAG (only when client authentication is required)
 *
 * The client would normally load
 * - CA_CERTIFICATE_TAG (always required, to verify the server)
 * - CLIENT_CERTIFICATE_TAG (for both public and private keys, only when
 *   client authentication is required)
 */
enum tls_tag {
	/** The Certificate Authority public key */
	CA_CERTIFICATE_TAG,
	/** Used for both the public and private server keys */
	SERVER_CERTIFICATE_TAG,
	/** Used for both the public and private client keys */
	CLIENT_CERTIFICATE_TAG,
};

/** @brief synchronization object for server & client threads */
static struct k_sem server_sem;

/** @brief The server thread stack */
static K_THREAD_STACK_DEFINE(server_stack, STACK_SIZE);
/** @brief the server thread object */
static struct k_thread server_thread;

#ifdef CONFIG_TLS_CREDENTIALS
/**
 * @brief The Certificate Authority (CA) Certificate
 *
 * The client needs the CA cert to verify the server public key. TLS client
 * sockets are always required to verify the server public key.
 *
 * Additionally, when the peer verification mode is
 * @ref TLS_PEER_VERIFY_OPTIONAL or @ref TLS_PEER_VERIFY_REQUIRED, then
 * the server also needs the CA cert in order to verify the client. This
 * type of configuration is often referred to as *mutual authentication*.
 */
static const unsigned char ca[] = {
#include "ca.inc"
};

/**
 * @brief The Server Certificate
 *
 * This is the public key of the server.
 */
static const unsigned char server[] = {
#include "server.inc"
};

/**
 * @brief The Server Private Key
 *
 * This is the private key of the server.
 */
static const unsigned char server_privkey[] = {
#include "server_privkey.inc"
};

/**
 * @brief The Client Certificate
 *
 * This is the public key of the client.
 */
static const unsigned char client[] = {
#include "client.inc"
};

/**
 * @brief The Client Private Key
 *
 * This is the private key of the client.
 */
static const unsigned char client_privkey[] = {
#include "client_privkey.inc"
};
#else /* CONFIG_TLS_CREDENTIALS */
#define ca NULL
#define server NULL
#define server_privkey NULL
#define client NULL
#define client_privkey NULL
#endif /* CONFIG_TLS_CREDENTIALS */

/**
 * @brief The server thread function
 *
 * This function simply accepts a client connection and
 * echoes the first @ref SECRET_SIZE bytes of the first
 * packet. After that, the server is closed and connections
 * are no longer accepted.
 *
 * @param arg0 a pointer to the int representing the server file descriptor
 * @param arg1 ignored
 * @param arg2 ignored
 */
static void server_thread_fn(void *arg0, void *arg1, void *arg2)
{
	const int server_fd = POINTER_TO_INT(arg0);
	const int echo = POINTER_TO_INT(arg1);
	const int expect_failure = POINTER_TO_INT(arg2);

	int r;
	int client_fd;
	net_socklen_t addrlen;
	char addrstr[NET_INET_ADDRSTRLEN];
	struct net_sockaddr_in sa;
	char *addrstrp;

	k_thread_name_set(k_current_get(), "server");

	NET_DBG("Server thread running");

	memset(&sa, 0, sizeof(sa));
	addrlen = sizeof(sa);

	NET_DBG("Accepting client connection..");
	k_sem_give(&server_sem);
	r = zsock_accept(server_fd, (struct net_sockaddr *)&sa, &addrlen);
	if (expect_failure) {
		zassert_equal(r, -1, "accept() should've failed");
		return;
	}
	zassert_not_equal(r, -1, "accept() failed (%d)", r);
	client_fd = r;

	memset(addrstr, '\0', sizeof(addrstr));
	addrstrp = (char *)zsock_inet_ntop(NET_PF_INET, &sa.sin_addr,
					   addrstr, sizeof(addrstr));
	zassert_not_equal(addrstrp, NULL, "inet_ntop() failed (%d)", errno);

	NET_DBG("accepted connection from [%s]:%d as fd %d",
		addrstr, net_ntohs(sa.sin_port), client_fd);

	if (echo) {
		NET_DBG("calling recv()");
		r = zsock_recv(client_fd, addrstr, sizeof(addrstr), 0);
		zassert_not_equal(r, -1, "recv() failed (%d)", errno);
		zassert_equal(r, SECRET_SIZE, "expected: %zu actual: %d",
			      SECRET_SIZE, r);

		NET_DBG("calling send()");
		r = zsock_send(client_fd, SECRET, SECRET_SIZE, 0);
		zassert_not_equal(r, -1, "send() failed (%d)", errno);
		zassert_equal(r, SECRET_SIZE, "expected: %zu actual: %d",
			      SECRET_SIZE, r);
	}

	NET_DBG("closing client fd");
	r = zsock_close(client_fd);
	zassert_not_equal(r, -1, "close() failed on the server fd (%d)", errno);
}

static int test_configure_server(k_tid_t *server_thread_id, int peer_verify,
				 int echo, int expect_failure)
{
	static const sec_tag_t server_tag_list_verify_none[] = {
		SERVER_CERTIFICATE_TAG,
	};

	static const sec_tag_t server_tag_list_verify[] = {
		CA_CERTIFICATE_TAG,
		SERVER_CERTIFICATE_TAG,
	};

	char addrstr[NET_INET_ADDRSTRLEN];
	const sec_tag_t *sec_tag_list;
	size_t sec_tag_list_size;
	struct net_sockaddr_in sa;
	const int yes = true;
	char *addrstrp;
	int server_fd;
	int r;

	k_sem_init(&server_sem, 0, 1);

	NET_DBG("Creating server socket");
	r = zsock_socket(NET_PF_INET, NET_SOCK_STREAM, NET_IPPROTO_TLS_1_2);
	zassert_not_equal(r, -1, "failed to create server socket (%d)", errno);
	server_fd = r;

	r = zsock_setsockopt(server_fd, ZSOCK_SOL_SOCKET, ZSOCK_SO_REUSEADDR, &yes, sizeof(yes));
	zassert_not_equal(r, -1, "failed to set SO_REUSEADDR (%d)", errno);

	switch (peer_verify) {
	case ZSOCK_TLS_PEER_VERIFY_NONE:
		sec_tag_list = server_tag_list_verify_none;
		sec_tag_list_size = sizeof(server_tag_list_verify_none);
		break;
	case ZSOCK_TLS_PEER_VERIFY_OPTIONAL:
	case ZSOCK_TLS_PEER_VERIFY_REQUIRED:
		sec_tag_list = server_tag_list_verify;
		sec_tag_list_size = sizeof(server_tag_list_verify);

		r = zsock_setsockopt(server_fd, ZSOCK_SOL_TLS, ZSOCK_TLS_PEER_VERIFY,
				     &peer_verify, sizeof(peer_verify));
		zassert_not_equal(r, -1, "failed to set TLS_PEER_VERIFY (%d)",
				  errno);
		break;
	default:
		zassert_true(false, "unrecognized TLS peer verify type %d",
			     peer_verify);
		return -1;
	}

	r = zsock_setsockopt(server_fd, ZSOCK_SOL_TLS, ZSOCK_TLS_SEC_TAG_LIST,
			     sec_tag_list, sec_tag_list_size);
	zassert_not_equal(r, -1, "failed to set TLS_SEC_TAG_LIST (%d)", errno);

	r = zsock_setsockopt(server_fd, ZSOCK_SOL_TLS, ZSOCK_TLS_HOSTNAME, "localhost",
			     sizeof("localhost"));
	zassert_not_equal(r, -1, "failed to set TLS_HOSTNAME (%d)", errno);

	memset(&sa, 0, sizeof(sa));
	/* The server listens on all network interfaces */
	sa.sin_addr.s_addr = NET_INADDR_ANY;
	sa.sin_family = NET_PF_INET;
	sa.sin_port = net_htons(PORT);

	r = zsock_bind(server_fd, (struct net_sockaddr *)&sa, sizeof(sa));
	zassert_not_equal(r, -1, "failed to bind (%d)", errno);

	r = zsock_listen(server_fd, 1);
	zassert_not_equal(r, -1, "failed to listen (%d)", errno);

	memset(addrstr, '\0', sizeof(addrstr));
	addrstrp = (char *)zsock_inet_ntop(NET_PF_INET, &sa.sin_addr,
				     addrstr, sizeof(addrstr));
	zassert_not_equal(addrstrp, NULL, "inet_ntop() failed (%d)", errno);

	NET_DBG("listening on [%s]:%d as fd %d",
		addrstr, net_ntohs(sa.sin_port), server_fd);

	NET_DBG("Creating server thread");
	*server_thread_id = k_thread_create(&server_thread, server_stack,
					    STACK_SIZE, server_thread_fn,
					    INT_TO_POINTER(server_fd),
					    INT_TO_POINTER(echo),
					    INT_TO_POINTER(expect_failure),
					    K_PRIO_PREEMPT(8), 0, K_NO_WAIT);

	r = k_sem_take(&server_sem, K_MSEC(TIMEOUT));
	zassert_equal(0, r, "failed to synchronize with server thread (%d)", r);

	return server_fd;
}

static int test_configure_client(struct net_sockaddr_in *sa, bool own_cert,
				 const char *hostname)
{
	static const sec_tag_t client_tag_list_verify_none[] = {
		CA_CERTIFICATE_TAG,
	};

	static const sec_tag_t client_tag_list_verify[] = {
		CA_CERTIFICATE_TAG,
		CLIENT_CERTIFICATE_TAG,
	};

	char addrstr[NET_INET_ADDRSTRLEN];
	const sec_tag_t *sec_tag_list;
	size_t sec_tag_list_size;
	char *addrstrp;
	int client_fd;
	int r;

	k_thread_name_set(k_current_get(), "client");

	NET_DBG("Creating client socket");
	r = zsock_socket(NET_PF_INET, NET_SOCK_STREAM, NET_IPPROTO_TLS_1_2);
	zassert_not_equal(r, -1, "failed to create client socket (%d)", errno);
	client_fd = r;

	if (own_cert) {
		sec_tag_list = client_tag_list_verify;
		sec_tag_list_size = sizeof(client_tag_list_verify);
	} else {
		sec_tag_list = client_tag_list_verify_none;
		sec_tag_list_size = sizeof(client_tag_list_verify_none);
	}

	r = zsock_setsockopt(client_fd, ZSOCK_SOL_TLS, ZSOCK_TLS_SEC_TAG_LIST,
			     sec_tag_list, sec_tag_list_size);
	zassert_not_equal(r, -1, "failed to set TLS_SEC_TAG_LIST (%d)", errno);

	r = zsock_setsockopt(client_fd, ZSOCK_SOL_TLS, ZSOCK_TLS_HOSTNAME, hostname,
			     strlen(hostname) + 1);
	zassert_not_equal(r, -1, "failed to set TLS_HOSTNAME (%d)", errno);

	sa->sin_family = NET_PF_INET;
	sa->sin_port = net_htons(PORT);
	r = zsock_inet_pton(NET_PF_INET, MY_IPV4_ADDR, &sa->sin_addr.s_addr);
	zassert_not_equal(-1, r, "inet_pton() failed (%d)", errno);
	zassert_not_equal(0, r, "%s is not a valid IPv4 address", MY_IPV4_ADDR);
	zassert_equal(1, r, "inet_pton() failed to convert %s", MY_IPV4_ADDR);

	memset(addrstr, '\0', sizeof(addrstr));
	addrstrp = (char *)zsock_inet_ntop(NET_PF_INET, &sa->sin_addr,
					   addrstr, sizeof(addrstr));
	zassert_not_equal(addrstrp, NULL, "inet_ntop() failed (%d)", errno);

	NET_DBG("connecting to [%s]:%d with fd %d",
		addrstr, net_ntohs(sa->sin_port), client_fd);

	return client_fd;
}
static void test_shutdown(int client_fd, int server_fd, k_tid_t server_thread_id)
{
	int r;

	NET_DBG("closing client fd");
	r = zsock_close(client_fd);
	zassert_not_equal(-1, r, "close() failed on the client fd (%d)", errno);

	NET_DBG("closing server fd");
	r = zsock_close(server_fd);
	zassert_not_equal(-1, r, "close() failed on the server fd (%d)", errno);

	r = k_thread_join(&server_thread, K_FOREVER);
	zassert_equal(0, r, "k_thread_join() failed (%d)", r);

	k_yield();
}

static void test_common(int peer_verify)
{
	k_tid_t server_thread_id;
	struct net_sockaddr_in sa;
	uint8_t rx_buf[16];
	int server_fd;
	int client_fd;
	int r;

	/*
	 * Server socket setup
	 */
	server_fd = test_configure_server(&server_thread_id, peer_verify, true,
					  false);

	/*
	 * Client socket setup
	 */
	client_fd = test_configure_client(&sa, peer_verify != ZSOCK_TLS_PEER_VERIFY_NONE,
					  "localhost");

	/*
	 * The main part of the test
	 */

	r = zsock_connect(client_fd, (struct net_sockaddr *)&sa, sizeof(sa));
	zassert_not_equal(r, -1, "failed to connect (%d)", errno);

	NET_DBG("Calling send()");
	r = zsock_send(client_fd, SECRET, SECRET_SIZE, 0);
	zassert_not_equal(r, -1, "send() failed (%d)", errno);
	zassert_equal(SECRET_SIZE, r, "expected: %zu actual: %d", SECRET_SIZE, r);

	NET_DBG("Calling recv()");
	memset(rx_buf, 0, sizeof(rx_buf));
	r = zsock_recv(client_fd, rx_buf, sizeof(rx_buf), 0);
	zassert_not_equal(r, -1, "recv() failed (%d)", errno);
	zassert_equal(SECRET_SIZE, r, "expected: %zu actual: %d", SECRET_SIZE, r);
	zassert_mem_equal(SECRET, rx_buf, SECRET_SIZE,
			  "expected: %s actual: %s", SECRET, rx_buf);

	/*
	 * Cleanup resources
	 */
	 test_shutdown(client_fd, server_fd, server_thread_id);
}

ZTEST(net_socket_tls_api_extension, test_tls_peer_verify_none)
{
	test_common(ZSOCK_TLS_PEER_VERIFY_NONE);
}

ZTEST(net_socket_tls_api_extension, test_tls_peer_verify_optional)
{
	test_common(ZSOCK_TLS_PEER_VERIFY_OPTIONAL);
}

ZTEST(net_socket_tls_api_extension, test_tls_peer_verify_required)
{
	test_common(ZSOCK_TLS_PEER_VERIFY_REQUIRED);
}

static void test_tls_cert_verify_result_opt_common(uint32_t expect)
{
	int server_fd, client_fd, ret;
	k_tid_t server_thread_id;
	struct net_sockaddr_in sa;
	uint32_t optval;
	net_socklen_t optlen = sizeof(optval);
	const char *hostname = "localhost";
	int peer_verify = ZSOCK_TLS_PEER_VERIFY_OPTIONAL;

	if (expect == MBEDTLS_X509_BADCERT_CN_MISMATCH) {
		hostname = "dummy";
	}

	server_fd = test_configure_server(&server_thread_id, ZSOCK_TLS_PEER_VERIFY_NONE,
					  false, false);
	client_fd = test_configure_client(&sa, false, hostname);

	ret = zsock_setsockopt(client_fd, ZSOCK_SOL_TLS, ZSOCK_TLS_PEER_VERIFY,
			       &peer_verify, sizeof(peer_verify));
	zassert_ok(ret, "failed to set TLS_PEER_VERIFY (%d)", errno);

	ret = zsock_connect(client_fd, (struct net_sockaddr *)&sa, sizeof(sa));
	zassert_not_equal(ret, -1, "failed to connect (%d)", errno);

	ret = zsock_getsockopt(client_fd, ZSOCK_SOL_TLS, ZSOCK_TLS_CERT_VERIFY_RESULT,
			       &optval, &optlen);
	zassert_equal(ret, 0, "getsockopt failed (%d)", errno);
	zassert_equal(optval, expect, "getsockopt got invalid verify result %d",
		      optval);

	test_shutdown(client_fd, server_fd, server_thread_id);
}

ZTEST(net_socket_tls_api_extension, test_tls_cert_verify_result_opt_ok)
{
	test_tls_cert_verify_result_opt_common(0);
}

ZTEST(net_socket_tls_api_extension, test_tls_cert_verify_result_opt_bad_cn)
{
	test_tls_cert_verify_result_opt_common(MBEDTLS_X509_BADCERT_CN_MISMATCH);
}

struct test_cert_verify_ctx {
	bool cb_called;
	int result;
};

static int cert_verify_cb(void *ctx, mbedtls_x509_crt *crt, int depth,
			  uint32_t *flags)
{
	struct test_cert_verify_ctx *test_ctx = (struct test_cert_verify_ctx *)ctx;

	test_ctx->cb_called = true;

	if (test_ctx->result == 0) {
		*flags = 0;
	} else {
		*flags |= MBEDTLS_X509_BADCERT_NOT_TRUSTED;
	}

	return test_ctx->result;
}

static void test_tls_cert_verify_cb_opt_common(int result)
{
	int server_fd, client_fd, ret;
	k_tid_t server_thread_id;
	struct net_sockaddr_in sa;
	struct test_cert_verify_ctx ctx = {
		.cb_called = false,
		.result = result,
	};
	struct zsock_tls_cert_verify_cb cb = {
		.cb = cert_verify_cb,
		.ctx = &ctx,
	};

	server_fd = test_configure_server(&server_thread_id, ZSOCK_TLS_PEER_VERIFY_NONE,
					  false, result == 0 ? false : true);
	client_fd = test_configure_client(&sa, false, "localhost");

	ret = zsock_setsockopt(client_fd, ZSOCK_SOL_TLS, ZSOCK_TLS_CERT_VERIFY_CALLBACK,
			       &cb, sizeof(cb));
	zassert_ok(ret, "failed to set TLS_CERT_VERIFY_CALLBACK (%d)", errno);

	ret = zsock_connect(client_fd, (struct net_sockaddr *)&sa, sizeof(sa));
	zassert_true(ctx.cb_called, "callback not called");
	if (result == 0) {
		zassert_equal(ret, 0, "failed to connect (%d)", errno);
	} else {
		zassert_equal(ret, -1, "connect() should fail");
		zassert_equal(errno, ECONNABORTED, "invalid errno");
	}

	test_shutdown(client_fd, server_fd, server_thread_id);
}

ZTEST(net_socket_tls_api_extension, test_tls_cert_verify_cb_opt_ok)
{
	test_tls_cert_verify_cb_opt_common(0);
}

ZTEST(net_socket_tls_api_extension, test_tls_cert_verify_cb_opt_bad_cert)
{
	test_tls_cert_verify_cb_opt_common(MBEDTLS_ERR_X509_CERT_VERIFY_FAILED);
}

/*
 * @brief Wire-level check of the ClientHello sent by a TLS client socket.
 *
 * A plain TCP server socket accepts the connection started by a TLS
 * client socket and reads the first record, which is the ClientHello.
 * The handshake is never completed; only the server_name extension
 * (RFC 6066) is inspected.
 */

#define SNI_TEST_PORT 4243
#define SNI_TEST_HOSTNAME "zephyr.example.org"
#define CLIENT_HELLO_BUF_SIZE 1024

/* Extension type for server_name, RFC 6066 section 3. */
#define TLS_EXT_SERVER_NAME 0

static uint8_t client_hello_buf[CLIENT_HELLO_BUF_SIZE];
static size_t client_hello_len;
static struct k_sem sni_test_sem;

static K_THREAD_STACK_DEFINE(sni_test_stack, STACK_SIZE);
static struct k_thread sni_test_thread;

static void sni_server_thread_fn(void *arg0, void *arg1, void *arg2)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);

	const int listen_fd = POINTER_TO_INT(arg0);
	int conn_fd;
	int r;

	conn_fd = zsock_accept(listen_fd, NULL, NULL);
	if (conn_fd < 0) {
		client_hello_len = 0;
		k_sem_give(&sni_test_sem);
		return;
	}

	r = zsock_recv(conn_fd, client_hello_buf, sizeof(client_hello_buf), 0);
	client_hello_len = (r > 0) ? (size_t)r : 0;

	zsock_close(conn_fd);
	k_sem_give(&sni_test_sem);
}

/*
 * Locate the server_name extension in a ClientHello. Returns true when the
 * extension is present; *host_offset/*host_len then point to the HostName
 * field of the single ServerName entry.
 */
static bool client_hello_get_server_name(const uint8_t *buf, size_t len,
					 size_t *host_offset, size_t *host_len)
{
	size_t hs_len, ext_total, pos, end, name_len;

	if (len < 9U || buf[0] != 0x16U || buf[5] != 0x01U) {
		return false;
	}

	/* Handshake header: length in bytes 6..8. Everything below is
	 * bounded by the handshake message, itself bounded by the record.
	 */
	hs_len = ((size_t)buf[6] << 16) | ((size_t)buf[7] << 8) | buf[8];
	if (9U + hs_len > len) {
		return false;
	}
	end = 9U + hs_len;

	/* ClientHello: legacy_version (2), random (32), session_id,
	 * cipher_suites, compression_methods, extensions.
	 */
	pos = 9U + 2U + 32U;
	if (pos + 1U > end) {
		return false;
	}
	pos += 1U + buf[pos];			/* session_id */
	if (pos + 2U > end) {
		return false;
	}
	pos += 2U + (((size_t)buf[pos] << 8) | buf[pos + 1]); /* cipher_suites */
	if (pos + 1U > end) {
		return false;
	}
	pos += 1U + buf[pos];			/* compression methods */
	if (pos + 2U > end) {
		return false;
	}
	ext_total = ((size_t)buf[pos] << 8) | buf[pos + 1];
	pos += 2U;
	if (pos + ext_total > end) {
		return false;
	}
	end = pos + ext_total;

	while (pos + 4U <= end) {
		size_t ext_type = ((size_t)buf[pos] << 8) | buf[pos + 1];
		size_t ext_len = ((size_t)buf[pos + 2] << 8) | buf[pos + 3];

		pos += 4U;
		if (pos + ext_len > end) {
			return false;
		}
		if (ext_type != TLS_EXT_SERVER_NAME) {
			pos += ext_len;
			continue;
		}

		/* ServerNameList: list_length (2), then one or more entries
		 * of name_type (1) and HostName length (2) + data.
		 */
		if (ext_len < 5U || buf[pos + 2] != 0U) {
			return false;
		}
		name_len = ((size_t)buf[pos + 3] << 8) | buf[pos + 4];
		if (5U + name_len > ext_len) {
			return false;
		}
		*host_offset = pos + 5U;
		*host_len = name_len;
		return true;
	}

	return false;
}

static void test_client_hello_common(bool set_hostname)
{
	struct net_sockaddr_in listen_sa = {
		.sin_family = NET_AF_INET,
		.sin_port = net_htons(SNI_TEST_PORT),
	};
	struct net_sockaddr_in connect_sa;
	size_t host_offset, host_len;
	int listen_fd;
	int client_fd;
	int r;

	client_hello_len = 0;
	k_sem_init(&sni_test_sem, 0, 1);

	listen_fd = zsock_socket(NET_PF_INET, NET_SOCK_STREAM, NET_IPPROTO_TCP);
	zassert_not_equal(listen_fd, -1, "failed to create listener (%d)", errno);

	r = zsock_bind(listen_fd, (struct net_sockaddr *)&listen_sa,
		       sizeof(listen_sa));
	zassert_not_equal(r, -1, "failed to bind listener (%d)", errno);
	r = zsock_listen(listen_fd, 1);
	zassert_not_equal(r, -1, "failed to listen (%d)", errno);

	k_thread_create(&sni_test_thread, sni_test_stack,
			STACK_SIZE, sni_server_thread_fn,
			INT_TO_POINTER(listen_fd), NULL, NULL,
			K_PRIO_PREEMPT(8), 0, K_NO_WAIT);

	client_fd = zsock_socket(NET_PF_INET, NET_SOCK_STREAM,
				 NET_IPPROTO_TLS_1_2);
	zassert_not_equal(client_fd, -1, "failed to create TLS socket (%d)", errno);

	r = ZSOCK_TLS_PEER_VERIFY_NONE;
	r = zsock_setsockopt(client_fd, ZSOCK_SOL_TLS, ZSOCK_TLS_PEER_VERIFY,
			     &r, sizeof(r));
	zassert_not_equal(r, -1, "failed to set TLS_PEER_VERIFY (%d)", errno);

	if (set_hostname) {
		r = zsock_setsockopt(client_fd, ZSOCK_SOL_TLS, ZSOCK_TLS_HOSTNAME,
				     SNI_TEST_HOSTNAME,
				     sizeof(SNI_TEST_HOSTNAME));
		zassert_not_equal(r, -1, "failed to set TLS_HOSTNAME (%d)", errno);
	}

	/* The peer closes the connection instead of completing the
	 * handshake, so connect() is expected to fail; only the
	 * ClientHello matters here.
	 */
	connect_sa.sin_family = NET_AF_INET;
	connect_sa.sin_port = net_htons(SNI_TEST_PORT);
	r = zsock_inet_pton(NET_AF_INET, MY_IPV4_ADDR, &connect_sa.sin_addr.s_addr);
	zassert_equal(r, 1, "inet_pton() failed (%d)", errno);

	(void)zsock_connect(client_fd, (struct net_sockaddr *)&connect_sa,
			    sizeof(connect_sa));

	r = k_sem_take(&sni_test_sem, K_MSEC(TIMEOUT));
	zassert_equal(r, 0, "server thread did not capture the ClientHello");
	k_thread_join(&sni_test_thread, K_FOREVER);

	zsock_close(client_fd);
	zsock_close(listen_fd);

	zassert_not_equal(client_hello_len, 0, "no ClientHello captured");

	if (set_hostname) {
		r = client_hello_get_server_name(client_hello_buf,
						 client_hello_len,
						 &host_offset, &host_len);
		zassert_true(r, "server_name extension not found");
		zassert_equal(host_len, sizeof(SNI_TEST_HOSTNAME) - 1,
			      "unexpected hostname length");
		zassert_mem_equal(SNI_TEST_HOSTNAME, &client_hello_buf[host_offset],
				  host_len, "unexpected hostname");
	} else {
		r = client_hello_get_server_name(client_hello_buf,
						 client_hello_len,
						 &host_offset, &host_len);
		zassert_false(r, "server_name extension sent without hostname");
	}
}

ZTEST(net_socket_tls_api_extension, test_client_hello_no_sni)
{
	test_client_hello_common(false);
}

ZTEST(net_socket_tls_api_extension, test_client_hello_with_sni)
{
	test_client_hello_common(true);
}

static void *setup(void)
{
	int r;

	/*
	 * Load both client & server credentials
	 *
	 * Normally, this would be split into separate applications but
	 * for testing purposes, we just use separate threads.
	 *
	 * Also, it has to be done before tests are run, otherwise
	 * there are errors due to attempts to load too many certificates.
	 *
	 * The server would normally load
	 * - server public key
	 * - server private key
	 * - ca cert (only when client authentication is required)
	 *
	 * The client would normally load
	 * - ca cert (to verify the server)
	 * - client public key (only when client authentication is required)
	 * - client private key (only when client authentication is required)
	 */
	if (IS_ENABLED(CONFIG_TLS_CREDENTIALS)) {
		NET_DBG("Loading credentials");
		r = tls_credential_add(CA_CERTIFICATE_TAG,
				       TLS_CREDENTIAL_CA_CERTIFICATE,
				       ca, sizeof(ca));
		zassert_equal(r, 0, "failed to add CA Certificate (%d)", r);

		r = tls_credential_add(SERVER_CERTIFICATE_TAG,
				       TLS_CREDENTIAL_PUBLIC_CERTIFICATE,
				       server, sizeof(server));
		zassert_equal(r, 0, "failed to add Server Certificate (%d)", r);

		r = tls_credential_add(SERVER_CERTIFICATE_TAG,
				       TLS_CREDENTIAL_PRIVATE_KEY,
				       server_privkey, sizeof(server_privkey));
		zassert_equal(r, 0, "failed to add Server Private Key (%d)", r);

		r = tls_credential_add(CLIENT_CERTIFICATE_TAG,
				       TLS_CREDENTIAL_PUBLIC_CERTIFICATE,
				       client, sizeof(client));
		zassert_equal(r, 0, "failed to add Client Certificate (%d)", r);

		r = tls_credential_add(CLIENT_CERTIFICATE_TAG,
				       TLS_CREDENTIAL_PRIVATE_KEY,
				       client_privkey, sizeof(client_privkey));
		zassert_equal(r, 0, "failed to add Client Private Key (%d)", r);
	}
	return NULL;
}

ZTEST_SUITE(net_socket_tls_api_extension, NULL, setup, NULL, NULL, NULL);
