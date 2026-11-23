/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2024 Nordic Semiconductor ASA
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tls_configuration_sample, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <errno.h>
#include <stdio.h>

#include <zephyr/net/socket.h>
#include <zephyr/net/socket_poll.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/net/net_if.h>
#include <zephyr/sys/util.h>

#if defined(CONFIG_TLS_CONFIGURATIONS_PSK_FORMAT_OPAQUE)
/* Needed to create the PSA key storing PSK. */
#include <psa/crypto.h>
#endif

#if defined(CONFIG_NET_SOCKETS_TEST_SESSION_CACHE)
#include <mbedtls/ssl.h>

int ztls_get_cached_session(int fd, mbedtls_ssl_session *session);
#endif

#if defined(CONFIG_MBEDTLS_CIPHERSUITE_TLS_PSK_WITH_AES_256_CBC_SHA384) || \
	defined(CONFIG_MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_PSK_ENABLED)
#define USE_PSK_KEY_EXCHANGE
#endif

/* All credential related variables are declared globally because
 * tls_credentials_volatile only takes a reference to them, it doesn't make
 * a copy.
 */
#if defined(USE_PSK_KEY_EXCHANGE)
static const unsigned char psk[] = { 0x01, 0x02, 0x03, 0x04, 0x05 };
static const char psk_id[] = "PSK_identity";
#if defined(CONFIG_TLS_CONFIGURATIONS_PSK_FORMAT_OPAQUE)
static psa_key_id_t psk_key_id = PSA_KEY_ID_NULL;
#endif /* CONFIG_TLS_CONFIGURATIONS_PSK_FORMAT_OPAQUE */
#else /* USE_PSK_KEY_EXCHANGE */
/* Server certificate is only used when not using PSK key exchanges for simplicity. */
#define USE_CERTIFICATE
static const unsigned char certificate[] = {
#include "ec.crt.inc"
};
#endif /* USE_PSK_KEY_EXCHANGE */

#define APP_BANNER "TLS socket configuration sample"

#define INVALID_SOCKET (-1)

/* Credentials have different types, so they can all share the same tag. */
#define CREDENTIALS_TAG 1

static int socket_fd = INVALID_SOCKET;
static struct zsock_pollfd fds[1];

/* Keep the new line because openssl uses that to start processing the incoming data */
#define TEST_STRING "hello world\n"
static uint8_t test_buf[sizeof(TEST_STRING)];

#if defined(CONFIG_NET_SOCKETS_TEST_SESSION_CACHE)
static int check_cached_ticket(void)
{
	mbedtls_ssl_session session;
	int ret;

	mbedtls_ssl_session_init(&session);
	ret = ztls_get_cached_session(socket_fd, &session);
	if (ret == 0 && session.MBEDTLS_PRIVATE(ticket_len) == 0) {
		ret = -ENOENT;
	}

	mbedtls_ssl_session_free(&session);
	return ret;
}
#endif /* CONFIG_NET_SOCKETS_TEST_SESSION_CACHE */

static int wait_for_event(void)
{
	int ret;

	/* Wait for event on any socket used. Once event occurs,
	 * we'll check them all.
	 */
	ret = zsock_poll(fds, ARRAY_SIZE(fds), -1);
	if (ret < 0) {
		LOG_ERR("Error in poll (%d)", errno);
		return ret;
	}

	return 0;
}

static int create_socket(void)
{
	int ret = 0;
	struct net_sockaddr_in addr;

	addr.sin_family = NET_AF_INET;
	addr.sin_port = net_htons(CONFIG_SERVER_PORT);
	zsock_inet_pton(NET_AF_INET, CONFIG_SERVER_ADDR, &addr.sin_addr);

#if defined(CONFIG_MBEDTLS_SSL_PROTO_TLS1_3)
	socket_fd = zsock_socket(addr.sin_family, NET_SOCK_STREAM, NET_IPPROTO_TLS_1_3);
#else
	socket_fd = zsock_socket(addr.sin_family, NET_SOCK_STREAM, NET_IPPROTO_TLS_1_2);
#endif
	if (socket_fd < 0) {
		LOG_ERR("Failed to create TLS socket (%d)", errno);
		return -errno;
	}

	sec_tag_t sec_tag_list[] = {
		CREDENTIALS_TAG,
	};

	ret = zsock_setsockopt(socket_fd, ZSOCK_SOL_TLS, ZSOCK_TLS_SEC_TAG_LIST, sec_tag_list,
			       sizeof(sec_tag_list));
	if (ret < 0) {
		LOG_ERR("Failed to set TLS_SEC_TAG_LIST option (%d)", errno);
		return -errno;
	}

	/* HOSTNAME is only required for key exchanges that use a certificate. */
#if defined(USE_CERTIFICATE)
	ret = zsock_setsockopt(socket_fd, ZSOCK_SOL_TLS, ZSOCK_TLS_HOSTNAME, "localhost",
			       sizeof("localhost"));
	if (ret < 0) {
		LOG_ERR("Failed to set TLS_HOSTNAME option (%d)", errno);
		return -errno;
	}
#endif

#if defined(CONFIG_NET_SOCKETS_TEST_SESSION_CACHE)
	int session_cache = ZSOCK_TLS_SESSION_CACHE_ENABLED;

	ret = zsock_setsockopt(socket_fd, ZSOCK_SOL_TLS, ZSOCK_TLS_SESSION_CACHE,
			       &session_cache, sizeof(session_cache));
	if (ret < 0) {
		LOG_ERR("Failed to enable TLS session cache (%d)", errno);
		return -errno;
	}
#endif

	ret = zsock_connect(socket_fd, (struct net_sockaddr *)&addr, sizeof(addr));
	if (ret < 0) {
		LOG_ERR("Cannot connect to TCP remote (%d)", errno);
		return -errno;
	}

	/* Prepare file descriptor for polling */
	fds[0].fd = socket_fd;
	fds[0].events = ZSOCK_POLLIN;

	return ret;
}

void close_socket(void)
{
	if (socket_fd != INVALID_SOCKET) {
		zsock_close(socket_fd);
	}
}

#if defined(CONFIG_TLS_CONFIGURATIONS_PSK_FORMAT_OPAQUE)
/* Import the PSK into PSA with the same policy that Mbed TLS uses for raw PSKs. */
static int import_opaque_psk(void)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t status;

	/* Only TLS 1.2 is supported: hash matches TLS_PSK_WITH_AES_256_CBC_SHA384.
	 * PSK in TLS 1.3 is under investigation.
	 */
	psa_set_key_type(&attr, PSA_KEY_TYPE_DERIVE);
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_DERIVE);
	psa_set_key_algorithm(&attr, PSA_ALG_TLS12_PSK_TO_MS(PSA_ALG_SHA_384));

	status = psa_import_key(&attr, psk, sizeof(psk), &psk_key_id);
	psa_reset_key_attributes(&attr);
	if (status != PSA_SUCCESS) {
		LOG_ERR("Failed to import PSK into PSA: %d", status);
		return -EIO;
	}

	return 0;
}
#endif /* CONFIG_TLS_CONFIGURATIONS_PSK_FORMAT_OPAQUE */

/* Wait until the interface has been assigned an IPv4 address. */
static int wait_for_ipv4_addr(void)
{
	struct net_if *iface = net_if_get_default();
	k_timeout_t timeout = K_SECONDS(CONFIG_TEST_IPV4_ADDR_TIMEOUT);
	k_timepoint_t end = sys_timepoint_calc(timeout);

	while (net_if_ipv4_get_global_addr(iface, NET_ADDR_PREFERRED) == NULL) {
		if (sys_timepoint_expired(end)) {
			LOG_ERR("Timeout waiting for an IPv4 address");
			return -ETIMEDOUT;
		}
		k_sleep(K_MSEC(100));
	}

	return 0;
}

static int setup_credentials(void)
{
	__maybe_unused int err;

#if defined(USE_CERTIFICATE)
	err = tls_credential_add(CREDENTIALS_TAG,
				TLS_CREDENTIAL_CA_CERTIFICATE,
				certificate,
				sizeof(certificate));
	if (err < 0) {
		LOG_ERR("Failed to register certificate: %d", err);
		return err;
	}
#endif

#if defined(USE_PSK_KEY_EXCHANGE)
#if defined(CONFIG_TLS_CONFIGURATIONS_PSK_FORMAT_OPAQUE)
	err = import_opaque_psk();
	if (err < 0) {
		return err;
	}

	err = tls_credential_add(CREDENTIALS_TAG,
				TLS_CREDENTIAL_PSK_PSA,
				&psk_key_id,
				sizeof(psk_key_id));
	if (err < 0) {
		LOG_ERR("Failed to register opaque PSK: %d", err);
	}
#else
	err = tls_credential_add(CREDENTIALS_TAG,
				TLS_CREDENTIAL_PSK,
				psk,
				sizeof(psk));
	if (err < 0) {
		LOG_ERR("Failed to register PSK: %d", err);
	}
#endif /* CONFIG_TLS_CONFIGURATIONS_PSK_FORMAT_OPAQUE */
	err = tls_credential_add(CREDENTIALS_TAG,
				TLS_CREDENTIAL_PSK_ID,
				psk_id,
				sizeof(psk_id) - 1);
	if (err < 0) {
		LOG_ERR("Failed to register PSK ID: %d", err);
	}
#endif

	return 0;
}

int main(void)
{
	int ret;
	size_t data_len;

	LOG_INF(APP_BANNER);

	setup_credentials();

	/* On native_sim target sockets are offloaded to the host PC, so the
	 * Zephyr TCP/IP stack is not used (i.e. no IPv4 to wait for).
	 */
	if (!IS_ENABLED(CONFIG_NET_NATIVE_OFFLOADED_SOCKETS)) {
		ret = wait_for_ipv4_addr();
		if (ret < 0) {
			goto exit;
		}
	}

	ret = create_socket();
	if (ret < 0) {
		LOG_ERR("Socket creation failed (%d)", ret);
		goto exit;
	}

	memcpy(test_buf, TEST_STRING, sizeof(TEST_STRING));
	/* The -1 here is because sizeof() accounts for "\0" but that's not
	 * needed for socket functions send/recv.
	 */
	data_len = sizeof(TEST_STRING) - 1;

	/* OpenSSL s_server has only the "-rev" option as echo-like behavior
	 * which echoes back the data that we send it in reversed order. So
	 * in the following we send the test buffer twice (in the 1st iteration
	 * it will contain the original TEST_STRING, whereas in the 2nd one
	 * it will contain TEST_STRING reversed) so that in the end we can
	 * just memcmp() it against the original TEST_STRING.
	 */
	for (int i = 0; i < 2; i++) {
		LOG_DBG("Send: %s", test_buf);
		ret = zsock_send(socket_fd, test_buf, data_len, 0);
		if (ret < 0) {
			LOG_ERR("Error sending test string (%d)", errno);
			goto exit;
		}

		memset(test_buf, 0, sizeof(test_buf));

		wait_for_event();

		ret = zsock_recv(socket_fd, test_buf, data_len, ZSOCK_MSG_WAITALL);
		if (ret == 0) {
			LOG_ERR("Server terminated unexpectedly");
			ret = -EIO;
			goto exit;
		} else if (ret < 0) {
			LOG_ERR("Error receiving data (%d)", errno);
			goto exit;
		}
		if (ret != data_len) {
			LOG_ERR("Sent %zu bytes, but received %d", data_len, ret);
			ret = -EINVAL;
			goto exit;
		}
		LOG_DBG("Received: %s", test_buf);
	}

#if defined(CONFIG_NET_SOCKETS_TEST_SESSION_CACHE)
	ret = check_cached_ticket();
	if (ret < 0) {
		LOG_ERR("TLS session ticket was not cached (%d)", ret);
		goto exit;
	}
#endif /* CONFIG_NET_SOCKETS_TEST_SESSION_CACHE */

	ret = memcmp(TEST_STRING, test_buf, data_len);
	if (ret != 0) {
		LOG_ERR("Received data does not match with TEST_STRING");
		LOG_HEXDUMP_ERR(test_buf, data_len, "Received:");
		LOG_HEXDUMP_ERR(TEST_STRING, data_len, "Expected:");
		ret = -EINVAL;
	}

exit:
	LOG_INF("Test %s", (ret < 0) ? "FAILED" : "PASSED");

	close_socket();

	return 0;
}
