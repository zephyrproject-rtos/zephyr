/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2024 Nordic Semiconductor ASA
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tls_configuration_sample, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <errno.h>
#include <stdio.h>

#include <zephyr/posix/sys/eventfd.h>

#include <zephyr/net/socket.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/net/net_if.h>
#include <zephyr/sys/util.h>

/* This include is required for the definition of the Mbed TLS internal symbol
 * MBEDTLS_SSL_HANDSHAKE_WITH_PSK_ENABLED.
 */
#include <mbedtls/ssl_ciphersuites.h>

#if defined(MBEDTLS_SSL_HANDSHAKE_WITH_PSK_ENABLED)
static const unsigned char psk[] = { 0x01, 0x02, 0x03, 0x04, 0x05 };
static const char psk_id[] = "PSK_identity";
#endif /* MBEDTLS_SSL_HANDSHAKE_WITH_PSK_ENABLED */

/* Following certificates (*.inc files) are:
 * - generated from "create-certs.sh" script
 * - converted in C array shape in the CMakeList file
 */
#if defined(CONFIG_PSA_WANT_ALG_RSA_PKCS1V15_SIGN) || defined(CONFIG_PSA_WANT_ALG_RSA_PSS)
#define USE_CERTIFICATE
static const unsigned char certificate[] = {
#include "rsa.crt.inc"
};
#elif defined(CONFIG_PSA_WANT_ALG_ECDSA)
#define USE_CERTIFICATE
static const unsigned char certificate[] = {
#include "ec.crt.inc"
};
#endif

#define APP_BANNER "TLS socket configuration sample"

#define INVALID_SOCKET (-1)

enum {
	_PLACEHOLDER_TAG_ = 0,
#if defined(USE_CERTIFICATE)
	CA_CERTIFICATE_TAG,
#endif
#if defined(MBEDTLS_SSL_HANDSHAKE_WITH_PSK_ENABLED)
	PSK_TAG,
#endif
};

static int socket_fd = INVALID_SOCKET;
static struct pollfd fds[1];

/* Keep the new line because openssl uses that to start processing the incoming data */
#define TEST_STRING "hello world\n"
static uint8_t test_buf[sizeof(TEST_STRING)];

static int wait_for_event(void)
{
	int ret;

	/* Wait for event on any socket used. Once event occurs,
	 * we'll check them all.
	 */
	ret = poll(fds, ARRAY_SIZE(fds), -1);
	if (ret < 0) {
		LOG_ERR("Error in poll (%d)", errno);
		return ret;
	}

	return 0;
}

static int create_socket(void)
{
	int ret = 0;
	struct sockaddr_in addr;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(CONFIG_SERVER_PORT);
	inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

#if defined(CONFIG_MBEDTLS_TLS_VERSION_1_3)
	socket_fd = socket(addr.sin_family, SOCK_STREAM, IPPROTO_TLS_1_3);
#else
	socket_fd = socket(addr.sin_family, SOCK_STREAM, IPPROTO_TLS_1_2);
#endif
	if (socket_fd < 0) {
		LOG_ERR("Failed to create TLS socket (%d)", errno);
		return -errno;
	}

	sec_tag_t sec_tag_list[] = {
#if defined(USE_CERTIFICATE)
		CA_CERTIFICATE_TAG,
#endif
#if defined(MBEDTLS_SSL_HANDSHAKE_WITH_PSK_ENABLED)
		PSK_TAG,
#endif
	};

	ret = setsockopt(socket_fd, SOL_TLS, TLS_SEC_TAG_LIST,
			sec_tag_list, sizeof(sec_tag_list));
	if (ret < 0) {
		LOG_ERR("Failed to set TLS_SEC_TAG_LIST option (%d)", errno);
		return -errno;
	}

	/* HOSTNAME is only required for key exchanges that use a certificate. */
#if defined(USE_CERTIFICATE)
	ret = setsockopt(socket_fd, SOL_TLS, TLS_HOSTNAME,
			 "localhost", sizeof("localhost"));
	if (ret < 0) {
		LOG_ERR("Failed to set TLS_HOSTNAME option (%d)", errno);
		return -errno;
	}
#endif

	ret = connect(socket_fd, (struct sockaddr *) &addr, sizeof(addr));
	if (ret < 0) {
		LOG_ERR("Cannot connect to TCP remote (%d)", errno);
		return -errno;
	}

	/* Prepare file descriptor for polling */
	fds[0].fd = socket_fd;
	fds[0].events = POLLIN;

	return ret;
}

void close_socket(void)
{
	if (socket_fd != INVALID_SOCKET) {
		close(socket_fd);
	}
}

static int setup_credentials(void)
{
	int err;

#if defined(USE_CERTIFICATE)
	err = tls_credential_add(CA_CERTIFICATE_TAG,
				TLS_CREDENTIAL_CA_CERTIFICATE,
				certificate,
				sizeof(certificate));
	if (err < 0) {
		LOG_ERR("Failed to register certificate: %d", err);
		return err;
	}
#endif

#if defined(MBEDTLS_SSL_HANDSHAKE_WITH_PSK_ENABLED)
	err = tls_credential_add(PSK_TAG,
				TLS_CREDENTIAL_PSK,
				psk,
				sizeof(psk));
	if (err < 0) {
		LOG_ERR("Failed to register PSK: %d", err);
	}
	err = tls_credential_add(PSK_TAG,
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
		ret = send(socket_fd, test_buf, data_len, 0);
		if (ret < 0) {
			LOG_ERR("Error sending test string (%d)", errno);
			goto exit;
		}

		memset(test_buf, 0, sizeof(test_buf));

		wait_for_event();

		ret = recv(socket_fd, test_buf, data_len, MSG_WAITALL);
		if (ret == 0) {
			LOG_ERR("Server terminated unexpectedly");
			ret = -EIO;
			goto exit;
		} else if (ret < 0) {
			LOG_ERR("Error receiving data (%d)", errno);
			goto exit;
		}
		if (ret != data_len) {
			LOG_ERR("Sent %d bytes, but received %d", data_len, ret);
			ret = -EINVAL;
			goto exit;
		}
		LOG_DBG("Received: %s", test_buf);
	}

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
