/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/net/tls_credentials_keygen.h>
#include <zephyr/sys/base64.h>

LOG_MODULE_REGISTER(tls_keygen_sample, LOG_LEVEL_INF);

#define SEC_TAG CONFIG_NET_SAMPLE_SEC_TAG
#define MESSAGE "Hello from Zephyr\n"

/* PEM has 64 characters per line, that is 48 bytes of DER. */
#define PEM_LINE_BYTES 48

static uint8_t csr[TLS_CREDENTIAL_KEYGEN_CSR_SIZE(sizeof(CONFIG_NET_SAMPLE_CSR_SUBJECT) - 1)];

static void pem_print(const char *label, const uint8_t *der, size_t len)
{
	char line[64 + 1];
	size_t olen;

	printk("-----BEGIN %s-----\n", label);

	for (size_t i = 0; i < len; i += PEM_LINE_BYTES) {
		(void)base64_encode(line, sizeof(line), &olen, der + i,
				    MIN(PEM_LINE_BYTES, len - i));
		printk("%s\n", line);
	}

	printk("-----END %s-----\n", label);
}

/* The key is persistent, so it is generated only on the first boot. */
static int key_ensure(void)
{
	psa_key_id_t key_id;
	int err;

	err = tls_credential_keygen_key_id_get(SEC_TAG, &key_id);
	if (err == 0) {
		LOG_INF("Using the key of sec tag %d", SEC_TAG);
		return 0;
	}

	if (err != -ENOENT) {
		return err;
	}

	LOG_INF("Generating a key for sec tag %d", SEC_TAG);

	return tls_credential_keygen_generate(SEC_TAG, TLS_CREDENTIAL_KEYGEN_ECDSA_SECP256R1);
}

static int csr_print(void)
{
	uint8_t pubkey[TLS_CREDENTIAL_KEYGEN_PUBKEY_MAX_SIZE];
	size_t len;
	int err;

	err = tls_credential_keygen_pubkey_get(SEC_TAG, pubkey, sizeof(pubkey), &len);
	if (err) {
		return err;
	}

	pem_print("PUBLIC KEY", pubkey, len);

	err = tls_credential_keygen_csr(SEC_TAG, CONFIG_NET_SAMPLE_CSR_SUBJECT, csr, sizeof(csr),
					&len);
	if (err) {
		return err;
	}

	pem_print("CERTIFICATE REQUEST", csr, len);

	return 0;
}

static bool credential_exists(enum tls_credential_type type)
{
	uint8_t buf;
	size_t len = sizeof(buf);
	int err;

	err = tls_credential_get(SEC_TAG, type, &buf, &len);

	return err == 0 || err == -EFBIG;
}

static void certificates_wait(void)
{
	if (credential_exists(TLS_CREDENTIAL_PUBLIC_CERTIFICATE) &&
	    credential_exists(TLS_CREDENTIAL_CA_CERTIFICATE)) {
		return;
	}

	LOG_INF("Waiting for the certificates. Add them in the shell:");
	LOG_INF("  cred add %d CLIENT_CERT DEFAULT BIN <base64 DER>", SEC_TAG);
	LOG_INF("  cred add %d CA_CERT DEFAULT BIN <base64 DER>", SEC_TAG);

	while (!credential_exists(TLS_CREDENTIAL_PUBLIC_CERTIFICATE) ||
	       !credential_exists(TLS_CREDENTIAL_CA_CERTIFICATE)) {
		k_sleep(K_SECONDS(1));
	}
}

static int server_talk(void)
{
	static const sec_tag_t tags[] = {SEC_TAG};
	struct net_sockaddr_in addr = {
		.sin_family = NET_AF_INET,
		.sin_port = net_htons(CONFIG_NET_SAMPLE_SERVER_PORT),
	};
	char buf[64];
	int ret;
	int fd;

	if (zsock_inet_pton(NET_AF_INET, CONFIG_NET_SAMPLE_SERVER_ADDR, &addr.sin_addr) != 1) {
		return -EINVAL;
	}

	fd = zsock_socket(NET_AF_INET, NET_SOCK_STREAM, NET_IPPROTO_TLS_1_2);
	if (fd < 0) {
		return -errno;
	}

	/* The sec tag gives the device certificate, the generated key and the CA certificate. */
	ret = zsock_setsockopt(fd, ZSOCK_SOL_TLS, ZSOCK_TLS_SEC_TAG_LIST, tags, sizeof(tags));
	if (ret < 0) {
		goto out;
	}

	ret = zsock_setsockopt(fd, ZSOCK_SOL_TLS, ZSOCK_TLS_HOSTNAME,
			       CONFIG_NET_SAMPLE_SERVER_HOSTNAME,
			       sizeof(CONFIG_NET_SAMPLE_SERVER_HOSTNAME));
	if (ret < 0) {
		goto out;
	}

	LOG_INF("Connecting to %s:%d", CONFIG_NET_SAMPLE_SERVER_ADDR,
		CONFIG_NET_SAMPLE_SERVER_PORT);

	ret = zsock_connect(fd, (struct net_sockaddr *)&addr, sizeof(addr));
	if (ret < 0) {
		goto out;
	}

	LOG_INF("TLS handshake done");

	ret = zsock_send(fd, MESSAGE, sizeof(MESSAGE) - 1, 0);
	if (ret < 0) {
		goto out;
	}

	ret = zsock_recv(fd, buf, sizeof(buf) - 1, 0);
	if (ret < 0) {
		goto out;
	}

	buf[ret] = '\0';
	LOG_INF("Received: %s", buf);
	ret = 0;

out:
	if (ret < 0) {
		ret = -errno;
	}

	(void)zsock_close(fd);

	return ret;
}

int main(void)
{
	int err;

	err = key_ensure();
	if (err) {
		LOG_ERR("Key generation failed: %d", err);
		return 0;
	}

	err = csr_print();
	if (err) {
		LOG_ERR("CSR generation failed: %d", err);
		return 0;
	}

	certificates_wait();

	err = server_talk();
	if (err) {
		LOG_ERR("Connection failed: %d", err);
		return 0;
	}

	LOG_INF("Done");

	return 0;
}
