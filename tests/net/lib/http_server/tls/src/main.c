/*
 * Copyright (c) 2023, Emna Rekik
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "http_service.h"
#include "server_functions.h"
#include <zephyr/logging/log.h>
#include <zephyr/net/http/service.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

LOG_MODULE_REGISTER(tls_test, CONFIG_NET_SOCKETS_LOG_LEVEL);

#define SECRET "framework_tests_tls"

/**
 * @brief Size of the encrypted message passed between server and client.
 */
#define SECRET_SIZE (sizeof(SECRET) - 1)

/** @brief Stack size for the server thread */
#define STACK_SIZE 8192

#define MY_IPV4_ADDR "127.0.0.1"

/** @brief arbitrary timeout value in ms */
#define TIMEOUT 1000

#define BUFFER_SIZE 256
#define SERVER_PORT 8000

static struct k_sem server_sem;

static K_THREAD_STACK_DEFINE(server_stack, STACK_SIZE);
static struct k_thread server_thread;

static uint16_t test_http_service_port = htons(SERVER_PORT);
HTTP_SERVICE_DEFINE(test_http_service, CONFIG_NET_CONFIG_MY_IPV4_ADDR, &test_http_service_port, 1,
			10, NULL);

#ifdef CONFIG_TLS_CREDENTIALS

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
#define ca             NULL
#define server         NULL
#define server_privkey NULL
#define client         NULL
#define client_privkey NULL
#endif /* CONFIG_TLS_CREDENTIALS */

static void server_thread_fn(void *arg0, void *arg1, void *arg2)
{
	struct http_server_ctx *ctx = (struct http_server_ctx *)arg0;

	int r;
	int client_fd;
	socklen_t addrlen;
	char addrstr[BUFFER_SIZE];
	struct sockaddr_in sa;

	k_thread_name_set(k_current_get(), "server");

	memset(&sa, 0, sizeof(sa));
	addrlen = sizeof(sa);

	k_sem_give(&server_sem);

	client_fd = accept_new_client(ctx->server_fd);

	NET_DBG("calling recv()");
	r = recv(client_fd, addrstr, sizeof(addrstr), 0);
	zassert_not_equal(r, -1, "recv() failed (%d)", errno);
	zassert_equal(r, SECRET_SIZE, "expected: %zu actual: %d", SECRET_SIZE, r);

	NET_DBG("calling send()");
	r = send(client_fd, SECRET, SECRET_SIZE, 0);
	zassert_not_equal(r, -1, "send() failed (%d)", errno);
	zassert_equal(r, SECRET_SIZE, "expected: %zu actual: %d", SECRET_SIZE, r);

	r = close(client_fd);
	zassert_not_equal(r, -1, "close() failed on the server fd (%d)", errno);
}

static void test_tls(void)
{
	int r;
	int client_fd;
	int proto = IPPROTO_TCP;
	char *addrstrp;
	k_tid_t server_thread_id;
	struct sockaddr_in sa;
	char addrstr[256];

	k_sem_init(&server_sem, 0, 1);

	/* set the common protocol for both client and server */
	if (IS_ENABLED(CONFIG_NET_SOCKETS_SOCKOPT_TLS)) {
		proto = IPPROTO_TLS_1_2;
	}

	struct http_server_ctx ctx;

	int server_fd = http_server_init(&ctx);

	zassert_true(server_fd >= 0, "Failed to create server socket");

	server_thread_id =
		k_thread_create(&server_thread, server_stack, STACK_SIZE, server_thread_fn, &ctx,
				NULL, NULL, K_PRIO_PREEMPT(8), 0, K_NO_WAIT);

	r = k_sem_take(&server_sem, K_MSEC(TIMEOUT));
	zassert_equal(0, r, "failed to synchronize with server thread (%d)", r);

	k_thread_name_set(k_current_get(), "client");

	r = socket(AF_INET, SOCK_STREAM, proto);
	zassert_not_equal(r, -1, "failed to create client socket (%d)", errno);
	client_fd = r;

	if (IS_ENABLED(CONFIG_TLS_CREDENTIALS) && IS_ENABLED(CONFIG_NET_SOCKETS_SOCKOPT_TLS)) {

		static const sec_tag_t client_tag_list_verify_none[] = {
			HTTP_SERVER_CA_CERTIFICATE_TAG,
		};

		const sec_tag_t *sec_tag_list;
		size_t sec_tag_list_size;

		sec_tag_list = client_tag_list_verify_none;
		sec_tag_list_size = sizeof(client_tag_list_verify_none);

		r = setsockopt(client_fd, SOL_TLS, TLS_SEC_TAG_LIST, sec_tag_list,
			       sec_tag_list_size);
		zassert_not_equal(r, -1, "failed to set TLS_SEC_TAG_LIST (%d)", errno);

		r = setsockopt(client_fd, SOL_TLS, TLS_HOSTNAME, "localhost", sizeof("localhost"));
		zassert_not_equal(r, -1, "failed to set TLS_HOSTNAME (%d)", errno);
	}

	sa.sin_family = AF_INET;
	sa.sin_port = htons(SERVER_PORT);

	r = inet_pton(AF_INET, MY_IPV4_ADDR, &sa.sin_addr.s_addr);
	zassert_not_equal(-1, r, "inet_pton() failed (%d)", errno);
	zassert_not_equal(0, r, "%s is not a valid IPv4 address", MY_IPV4_ADDR);
	zassert_equal(1, r, "inet_pton() failed to convert %s", MY_IPV4_ADDR);

	memset(addrstr, '\0', sizeof(addrstr));
	addrstrp = (char *)inet_ntop(AF_INET, &sa.sin_addr, addrstr, sizeof(addrstr));
	zassert_not_equal(addrstrp, NULL, "inet_ntop() failed (%d)", errno);

	r = connect(client_fd, (struct sockaddr *)&sa, sizeof(sa));
	zassert_not_equal(r, -1, "failed to connect (%d)", errno);

	NET_DBG("Calling send()");
	r = send(client_fd, SECRET, SECRET_SIZE, 0);
	zassert_not_equal(r, -1, "send() failed (%d)", errno);
	zassert_equal(SECRET_SIZE, r, "expected: %zu actual: %d", SECRET_SIZE, r);

	NET_DBG("Calling recv()");
	memset(addrstr, 0, sizeof(addrstr));
	r = recv(client_fd, addrstr, sizeof(addrstr), 0);
	zassert_not_equal(r, -1, "recv() failed (%d)", errno);
	zassert_equal(SECRET_SIZE, r, "expected: %zu actual: %d", SECRET_SIZE, r);

	zassert_mem_equal(SECRET, addrstr, SECRET_SIZE, "expected: %s actual: %s", SECRET, addrstr);

	r = close(client_fd);
	zassert_not_equal(-1, r, "close() failed on the client fd (%d)", errno);

	r = close(server_fd);
	zassert_not_equal(-1, r, "close() failed on the server fd (%d)", errno);

	r = k_thread_join(&server_thread, K_FOREVER);
	zassert_equal(0, r, "k_thread_join() failed (%d)", r);
}

ZTEST(framework_tests_tls, test_tls)
{
	test_tls();
}

static void *setup(void)
{
	int r;

	if (IS_ENABLED(CONFIG_TLS_CREDENTIALS)) {
		NET_DBG("Loading credentials");
		r = tls_credential_add(HTTP_SERVER_CA_CERTIFICATE_TAG,
				       TLS_CREDENTIAL_CA_CERTIFICATE, ca, sizeof(ca));
		zassert_equal(r, 0, "failed to add CA Certificate (%d)", r);

		r = tls_credential_add(HTTP_SERVER_SERVER_CERTIFICATE_TAG,
				       TLS_CREDENTIAL_SERVER_CERTIFICATE, server, sizeof(server));
		zassert_equal(r, 0, "failed to add Server Certificate (%d)", r);

		r = tls_credential_add(HTTP_SERVER_SERVER_CERTIFICATE_TAG,
				       TLS_CREDENTIAL_PRIVATE_KEY, server_privkey,
				       sizeof(server_privkey));
		zassert_equal(r, 0, "failed to add Server Private Key (%d)", r);

		r = tls_credential_add(HTTP_SERVER_CLIENT_CERTIFICATE_TAG,
				       TLS_CREDENTIAL_SERVER_CERTIFICATE, client, sizeof(client));
		zassert_equal(r, 0, "failed to add Client Certificate (%d)", r);

		r = tls_credential_add(HTTP_SERVER_CLIENT_CERTIFICATE_TAG,
				       TLS_CREDENTIAL_PRIVATE_KEY, client_privkey,
				       sizeof(client_privkey));
		zassert_equal(r, 0, "failed to add Client Private Key (%d)", r);
	}
	return NULL;
}

ZTEST_SUITE(framework_tests_tls, NULL, setup, NULL, NULL, NULL);
