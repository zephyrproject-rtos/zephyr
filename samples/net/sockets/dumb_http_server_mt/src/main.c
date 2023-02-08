/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(net_dumb_http_srv_mt_sample);

#include <zephyr/kernel.h>
#include <errno.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/tls_credentials.h>

#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/net_conn_mgr.h>

#define MY_PORT 8080

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
#define STACK_SIZE 4096

#define SERVER_CERTIFICATE_TAG 1

static const unsigned char server_certificate[] = {
#include "mt-http-server-cert.der.inc"
};

/* This is the private key in pkcs#8 format. */
static const unsigned char private_key[] = {
#include "mt-http-server-key.der.inc"
};
#else
#define STACK_SIZE 1024
#endif
#if defined(CONFIG_NET_TC_THREAD_COOPERATIVE)
#define THREAD_PRIORITY K_PRIO_COOP(CONFIG_NUM_COOP_PRIORITIES - 1)
#else
#define THREAD_PRIORITY K_PRIO_PREEMPT(8)
#endif

static const char content[] = {
#if defined(CONFIG_NET_SAMPLE_SERVE_LARGE_FILE)
    #include "response_100k.html.bin.inc"
#else
    #include "response_big.html.bin.inc"
#endif
};

#define MAX_CLIENT_QUEUE CONFIG_NET_SAMPLE_NUM_HANDLERS

#if defined(CONFIG_NET_IPV4)
K_THREAD_STACK_ARRAY_DEFINE(tcp4_handler_stack, CONFIG_NET_SAMPLE_NUM_HANDLERS,
			    STACK_SIZE);
static struct k_thread tcp4_handler_thread[CONFIG_NET_SAMPLE_NUM_HANDLERS];
static k_tid_t tcp4_handler_tid[CONFIG_NET_SAMPLE_NUM_HANDLERS];
#endif

#if defined(CONFIG_NET_IPV6)
K_THREAD_STACK_ARRAY_DEFINE(tcp6_handler_stack, CONFIG_NET_SAMPLE_NUM_HANDLERS,
			    STACK_SIZE);
static struct k_thread tcp6_handler_thread[CONFIG_NET_SAMPLE_NUM_HANDLERS];
static k_tid_t tcp6_handler_tid[CONFIG_NET_SAMPLE_NUM_HANDLERS];
#endif

static struct net_mgmt_event_callback mgmt_cb;
static bool connected;
K_SEM_DEFINE(run_app, 0, 1);
K_SEM_DEFINE(quit_lock, 0, 1);
static bool running_status;
static bool want_to_quit;
static int tcp4_listen_sock;
static int tcp4_accepted[CONFIG_NET_SAMPLE_NUM_HANDLERS];
static int tcp6_listen_sock;
static int tcp6_accepted[CONFIG_NET_SAMPLE_NUM_HANDLERS];

static void process_tcp4(void);
static void process_tcp6(void);

K_THREAD_DEFINE(tcp4_thread_id, STACK_SIZE,
		process_tcp4, NULL, NULL, NULL,
		THREAD_PRIORITY, 0, -1);

K_THREAD_DEFINE(tcp6_thread_id, STACK_SIZE,
		process_tcp6, NULL, NULL, NULL,
		THREAD_PRIORITY, 0, -1);

#define EVENT_MASK (NET_EVENT_L4_CONNECTED | \
		    NET_EVENT_L4_DISCONNECTED)

static void event_handler(struct net_mgmt_event_callback *cb,
			  uint32_t mgmt_event, struct net_if *iface)
{
	if ((mgmt_event & EVENT_MASK) != mgmt_event) {
		return;
	}

	if (want_to_quit) {
		k_sem_give(&run_app);
		want_to_quit = false;
	}

	if (mgmt_event == NET_EVENT_L4_CONNECTED) {
		LOG_INF("Network connected");

		connected = true;
		k_sem_give(&run_app);

		return;
	}

	if (mgmt_event == NET_EVENT_L4_DISCONNECTED) {
		if (connected == false) {
			LOG_INF("Waiting network to be connected");
		} else {
			LOG_INF("Network disconnected");
			connected = false;
		}

		k_sem_reset(&run_app);

		return;
	}
}

static ssize_t sendall(int sock, const void *buf, size_t len)
{
	while (len) {
		ssize_t out_len = send(sock, buf, len, 0);

		if (out_len < 0) {
			return out_len;
		}

		buf = (const char *)buf + out_len;
		len -= out_len;
	}

	return 0;
}

static int setup(int *sock, struct sockaddr *bind_addr,
		 socklen_t bind_addrlen)
{
	int ret;

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
	*sock = socket(bind_addr->sa_family, SOCK_STREAM, IPPROTO_TLS_1_2);
#else
	*sock = socket(bind_addr->sa_family, SOCK_STREAM, IPPROTO_TCP);
#endif
	if (*sock < 0) {
		LOG_ERR("Failed to create TCP socket: %d", errno);
		return -errno;
	}

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
	sec_tag_t sec_tag_list[] = {
		SERVER_CERTIFICATE_TAG,
	};

	ret = setsockopt(*sock, SOL_TLS, TLS_SEC_TAG_LIST,
			 sec_tag_list, sizeof(sec_tag_list));
	if (ret < 0) {
		LOG_ERR("Failed to set TCP secure option %d", errno);
		ret = -errno;
	}
#endif

	ret = bind(*sock, bind_addr, bind_addrlen);
	if (ret < 0) {
		LOG_ERR("Failed to bind TCP socket %d", errno);
		return -errno;
	}

	ret = listen(*sock, MAX_CLIENT_QUEUE);
	if (ret < 0) {
		LOG_ERR("Failed to listen on TCP socket %d", errno);
		ret = -errno;
	}

	return ret;
}

static void client_conn_handler(void *ptr1, void *ptr2, void *ptr3)
{
	ARG_UNUSED(ptr1);
	int *sock = ptr2;
	k_tid_t *in_use = ptr3;
	int client;
	int received;
	int ret;
	char buf[100];

	client = *sock;

	/* Discard HTTP request (or otherwise client will get
	 * connection reset error).
	 */
	do {
		received = recv(client, buf, sizeof(buf), 0);
		if (received == 0) {
			/* Connection closed */
			LOG_DBG("[%d] Connection closed by peer", client);
			break;
		} else if (received < 0) {
			/* Socket error */
			ret = -errno;
			LOG_ERR("[%d] Connection error %d", client, ret);
			break;
		}

		/* Note that something like this strstr() check should *NOT*
		 * be used in production code. This is done like this just
		 * for this sample application to keep things simple.
		 *
		 * We are assuming here that the full HTTP request is received
		 * in one TCP segment which in real life might not.
		 */
		if (strstr(buf, "\r\n\r\n")) {
			break;
		}
	} while (true);

	/* We received status from the client */
	if (strstr(buf, "\r\n\r\nOK")) {
		running_status = true;
		want_to_quit = true;
		k_sem_give(&quit_lock);
	} else if (strstr(buf, "\r\n\r\nFAIL")) {
		running_status = false;
		want_to_quit = true;
		k_sem_give(&quit_lock);
	} else {
		(void)sendall(client, content, sizeof(content));
	}

	(void)close(client);

	*sock = -1;
	*in_use = NULL;
}

static int get_free_slot(int *accepted)
{
	int i;

	for (i = 0; i < CONFIG_NET_SAMPLE_NUM_HANDLERS; i++) {
		if (accepted[i] < 0) {
			return i;
		}
	}

	return -1;
}

static int process_tcp(int *sock, int *accepted)
{
	static int counter;
	int client;
	int slot;
	struct sockaddr_in6 client_addr;
	socklen_t client_addr_len = sizeof(client_addr);

	client = accept(*sock, (struct sockaddr *)&client_addr,
			&client_addr_len);
	if (client < 0) {
		LOG_ERR("Error in accept %d, stopping server", -errno);
		return -errno;
	}

	slot = get_free_slot(accepted);
	if (slot < 0 || slot >= CONFIG_NET_SAMPLE_NUM_HANDLERS) {
		LOG_ERR("Cannot accept more connections");
		close(client);
		return 0;
	}

	accepted[slot] = client;

#if defined(CONFIG_NET_IPV6)
	if (client_addr.sin6_family == AF_INET6) {
		tcp6_handler_tid[slot] = k_thread_create(
			&tcp6_handler_thread[slot],
			tcp6_handler_stack[slot],
			K_THREAD_STACK_SIZEOF(tcp6_handler_stack[slot]),
			(k_thread_entry_t)client_conn_handler,
			INT_TO_POINTER(slot),
			&accepted[slot],
			&tcp6_handler_tid[slot],
			THREAD_PRIORITY,
			0, K_NO_WAIT);
	}
#endif

#if defined(CONFIG_NET_IPV4)
	if (client_addr.sin6_family == AF_INET) {
		tcp4_handler_tid[slot] = k_thread_create(
			&tcp4_handler_thread[slot],
			tcp4_handler_stack[slot],
			K_THREAD_STACK_SIZEOF(tcp4_handler_stack[slot]),
			(k_thread_entry_t)client_conn_handler,
			INT_TO_POINTER(slot),
			&accepted[slot],
			&tcp4_handler_tid[slot],
			THREAD_PRIORITY,
			0, K_NO_WAIT);
	}
#endif

	if (LOG_LEVEL >= LOG_LEVEL_DBG) {
		char addr_str[INET6_ADDRSTRLEN];

		net_addr_ntop(client_addr.sin6_family,
			      &client_addr.sin6_addr,
			      addr_str, sizeof(addr_str));

		LOG_DBG("[%d] Connection #%d from %s",
			client, ++counter,
			addr_str);
	}

	return 0;
}

static void process_tcp4(void)
{
	struct sockaddr_in addr4;
	int ret;

	(void)memset(&addr4, 0, sizeof(addr4));
	addr4.sin_family = AF_INET;
	addr4.sin_port = htons(MY_PORT);

	ret = setup(&tcp4_listen_sock, (struct sockaddr *)&addr4,
		    sizeof(addr4));
	if (ret < 0) {
		return;
	}

	LOG_DBG("Waiting for IPv4 HTTP connections on port %d, sock %d",
		MY_PORT, tcp4_listen_sock);

	while (ret == 0 || !want_to_quit) {
		ret = process_tcp(&tcp4_listen_sock, tcp4_accepted);
		if (ret < 0) {
			return;
		}
	}
}

static void process_tcp6(void)
{
	struct sockaddr_in6 addr6;
	int ret;

	(void)memset(&addr6, 0, sizeof(addr6));
	addr6.sin6_family = AF_INET6;
	addr6.sin6_port = htons(MY_PORT);

	ret = setup(&tcp6_listen_sock, (struct sockaddr *)&addr6,
		    sizeof(addr6));
	if (ret < 0) {
		return;
	}

	LOG_DBG("Waiting for IPv6 HTTP connections on port %d, sock %d",
		MY_PORT, tcp6_listen_sock);

	while (ret == 0 || !want_to_quit) {
		ret = process_tcp(&tcp6_listen_sock, tcp6_accepted);
		if (ret != 0) {
			return;
		}
	}
}

void start_listener(void)
{
	int i;

	for (i = 0; i < CONFIG_NET_SAMPLE_NUM_HANDLERS; i++) {
#if defined(CONFIG_NET_IPV4)
		tcp4_accepted[i] = -1;
		tcp4_listen_sock = -1;
#endif
#if defined(CONFIG_NET_IPV6)
		tcp6_accepted[i] = -1;
		tcp6_listen_sock = -1;
#endif
	}

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		k_thread_start(tcp6_thread_id);
	}

	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		k_thread_start(tcp4_thread_id);
	}
}

int main(void)
{
#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
	int err = tls_credential_add(SERVER_CERTIFICATE_TAG,
				     TLS_CREDENTIAL_SERVER_CERTIFICATE,
				     server_certificate,
				     sizeof(server_certificate));
	if (err < 0) {
		LOG_ERR("Failed to register public certificate: %d", err);
	}

	err = tls_credential_add(SERVER_CERTIFICATE_TAG,
				 TLS_CREDENTIAL_PRIVATE_KEY,
				 private_key, sizeof(private_key));
	if (err < 0) {
		LOG_ERR("Failed to register private key: %d", err);
	}
#endif

	if (IS_ENABLED(CONFIG_NET_CONNECTION_MANAGER)) {
		net_mgmt_init_event_callback(&mgmt_cb,
					     event_handler, EVENT_MASK);
		net_mgmt_add_event_callback(&mgmt_cb);

		net_conn_mgr_resend_status();
	}

	if (!IS_ENABLED(CONFIG_NET_CONNECTION_MANAGER)) {
		/* If the config library has not been configured to start the
		 * app only after we have a connection, then we can start
		 * it right away.
		 */
		k_sem_give(&run_app);
	}

	/* Wait for the connection. */
	k_sem_take(&run_app, K_FOREVER);

	start_listener();

	k_sem_take(&quit_lock, K_FOREVER);

	if (running_status) {
		/* No issues, let the testing system know about this */
		exit(0);
	} else {
		exit(1);
	}
	return 0;
}
