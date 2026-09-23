/*
 * TLS credentials for the BMC HTTPS service.
 *
 * The BMC core never owns keys: it only knows the security tag configured with
 * CONFIG_BMC_HTTPS_SEC_TAG, and the application fills that tag before
 * bmc_init() starts the servers. A real product would load the certificate
 * from its own secure storage instead of building it in.
 *
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/mgmt/bmc.h>
#include <zephyr/net/tls_credentials.h>

LOG_MODULE_DECLARE(bmc_sample, LOG_LEVEL_INF);

static const unsigned char server_certificate[] = {
#include "server_cert.der.inc"
};

/* In pkcs#8 format. */
static const unsigned char private_key[] = {
#include "server_privkey.der.inc"
};

static int tls_credentials_init(void)
{
	int ret;

	ret = tls_credential_add(CONFIG_BMC_HTTPS_SEC_TAG, TLS_CREDENTIAL_PUBLIC_CERTIFICATE,
				 server_certificate, sizeof(server_certificate));
	if (ret < 0) {
		LOG_ERR("Could not register the server certificate (err=%d)", ret);
		return ret;
	}

	ret = tls_credential_add(CONFIG_BMC_HTTPS_SEC_TAG, TLS_CREDENTIAL_PRIVATE_KEY, private_key,
				 sizeof(private_key));
	if (ret < 0) {
		LOG_ERR("Could not register the server private key (err=%d)", ret);
		return ret;
	}

	return 0;
}

/* Before the HTTP services start in BMC_INIT_PHASE_SERVICE. */
BMC_COMPONENT_DEFINE(sample_tls, BMC_INIT_PHASE_STORAGE, tls_credentials_init, false);
