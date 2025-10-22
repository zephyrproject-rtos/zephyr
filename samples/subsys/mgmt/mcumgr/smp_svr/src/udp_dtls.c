/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(smp_sample);

static const unsigned char server_certificate[] = {
#include "echo-apps-cert.der.inc"
};

/* This is the private key in pkcs#8 format. */
static const unsigned char private_key[] = {
#include "echo-apps-key.der.inc"
};

int setup_udp_dtls(void)
{
	int rc;

	rc = tls_credential_add(CONFIG_MCUMGR_TRANSPORT_UDP_DTLS_TLS_TAG,
				TLS_CREDENTIAL_PUBLIC_CERTIFICATE, server_certificate,
				sizeof(server_certificate));

	if (rc < 0) {
		LOG_ERR("Failed to register public certificate: %d", rc);
		return rc;
	}

	rc = tls_credential_add(CONFIG_MCUMGR_TRANSPORT_UDP_DTLS_TLS_TAG,
				TLS_CREDENTIAL_PRIVATE_KEY, private_key, sizeof(private_key));

	if (rc < 0) {
		LOG_ERR("Failed to register private key: %d", rc);
	}

	return rc;
}
