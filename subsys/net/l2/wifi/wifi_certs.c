/*
 * Copyright (c) 2025 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_wifi_certs, CONFIG_NET_L2_WIFI_MGMT_LOG_LEVEL);

#include <zephyr/net/wifi_certs.h>
#include <utils/common.h>
#include <eap_peer/eap_config.h>
#include <ctrl_iface_zephyr.h>
#include <wpa_supplicant/config.h>
#include <supp_main.h>

static struct wifi_enterprise_creds_params enterprise_creds_params = { 0 };

#ifdef CONFIG_WIFI_SHELL_RUNTIME_CERTIFICATES
#include <zephyr/net/tls_credentials.h>
enum wifi_enterprise_cert_sec_tags {
	WIFI_CERT_CA_SEC_TAG = 0x1020001,
	WIFI_CERT_CLIENT_KEY_SEC_TAG,
	WIFI_CERT_SERVER_KEY_SEC_TAG,
	WIFI_CERT_CLIENT_SEC_TAG,
	WIFI_CERT_SERVER_SEC_TAG,
	/* Phase 2 */
	WIFI_CERT_CA_P2_SEC_TAG,
	WIFI_CERT_CLIENT_KEY_P2_SEC_TAG,
	WIFI_CERT_CLIENT_P2_SEC_TAG,
};

struct wifi_cert_data {
	enum tls_credential_type type;
	uint32_t sec_tag;
	uint8_t **data;
	size_t *len;
};
#else
static const char ca_cert_test[] = {
	#include <wifi_enterprise_test_certs/ca.pem.inc>
	'\0'
};

static const char client_cert_test[] = {
	#include <wifi_enterprise_test_certs/client.pem.inc>
	'\0'
};

static const char client_key_test[] = {
	#include <wifi_enterprise_test_certs/client-key.pem.inc>
	'\0'
};

static const char ca_cert2_test[] = {
	#include <wifi_enterprise_test_certs/ca2.pem.inc>
	'\0'};

static const char client_cert2_test[] = {
	#include <wifi_enterprise_test_certs/client2.pem.inc>
	'\0'};

static const char client_key2_test[] = {
	#include <wifi_enterprise_test_certs/client-key2.pem.inc>
	'\0'};

static const char server_cert_test[] = {
	#include <wifi_enterprise_test_certs/server.pem.inc>
	'\0'
};

static const char server_key_test[] = {
	#include <wifi_enterprise_test_certs/server-key.pem.inc>
	'\0'
};
#endif /* CONFIG_WIFI_SHELL_RUNTIME_CERTIFICATES */

#ifdef CONFIG_WIFI_SHELL_RUNTIME_CERTIFICATES
static int process_certificates(struct wifi_cert_data *certs, size_t cert_count)
{
	for (size_t i = 0; i < cert_count; i++) {
		int err;
		size_t len = 0;
		uint8_t *cert_tmp;

		err = tls_credential_get(certs[i].sec_tag, certs[i].type, NULL, &len);
		if (err != -EFBIG) {
			LOG_ERR("Failed to get credential tag: %d length, err: %d",
				certs[i].sec_tag, err);
			return err;
		}

		cert_tmp = k_malloc(len);
		if (!cert_tmp) {
			LOG_ERR("Failed to allocate memory for credential tag: %d",
				certs[i].sec_tag);
			return -ENOMEM;
		}

		err = tls_credential_get(certs[i].sec_tag, certs[i].type, cert_tmp, &len);
		if (err) {
			LOG_ERR("Failed to get credential tag: %d", certs[i].sec_tag);
			k_free(cert_tmp);
			return err;
		}

		*certs[i].data = cert_tmp;
		*certs[i].len = len;
	}

	return 0;
}

static void set_enterprise_creds_params(bool is_ap)
{
	struct wifi_cert_data certs_common[] = {
		{
			.type = TLS_CREDENTIAL_CA_CERTIFICATE,
			.sec_tag = WIFI_CERT_CA_SEC_TAG,
			.data = &enterprise_creds_params.ca_cert,
			.len = &enterprise_creds_params.ca_cert_len,
		},
	};
	struct wifi_cert_data certs_sta[] = {
		{
			.type = TLS_CREDENTIAL_PRIVATE_KEY,
			.sec_tag = WIFI_CERT_CLIENT_KEY_SEC_TAG,
			.data = &enterprise_creds_params.client_key,
			.len = &enterprise_creds_params.client_key_len,
		},
		{
			.type = TLS_CREDENTIAL_PUBLIC_CERTIFICATE,
			.sec_tag = WIFI_CERT_CLIENT_SEC_TAG,
			.data = &enterprise_creds_params.client_cert,
			.len = &enterprise_creds_params.client_cert_len,
		},
		{
			.type = TLS_CREDENTIAL_CA_CERTIFICATE,
			.sec_tag = WIFI_CERT_CA_P2_SEC_TAG,
			.data = &enterprise_creds_params.ca_cert2,
			.len = &enterprise_creds_params.ca_cert2_len,
		},
		{
			.type = TLS_CREDENTIAL_PRIVATE_KEY,
			.sec_tag = WIFI_CERT_CLIENT_KEY_P2_SEC_TAG,
			.data = &enterprise_creds_params.client_key2,
			.len = &enterprise_creds_params.client_key2_len,
		},
		{
			.type = TLS_CREDENTIAL_PUBLIC_CERTIFICATE,
			.sec_tag = WIFI_CERT_CLIENT_P2_SEC_TAG,
			.data = &enterprise_creds_params.client_cert2,
			.len = &enterprise_creds_params.client_cert2_len,
		},
	};

	struct wifi_cert_data certs_ap[] = {
		{
			.type = TLS_CREDENTIAL_PUBLIC_CERTIFICATE,
			.sec_tag = WIFI_CERT_SERVER_SEC_TAG,
			.data = &enterprise_creds_params.server_cert,
			.len = &enterprise_creds_params.server_cert_len,
		},
		{
			.type = TLS_CREDENTIAL_PRIVATE_KEY,
			.sec_tag = WIFI_CERT_SERVER_KEY_SEC_TAG,
			.data = &enterprise_creds_params.server_key,
			.len = &enterprise_creds_params.server_key_len,
		},
	};

	memset(&enterprise_creds_params, 0, sizeof(struct wifi_enterprise_creds_params));

	/* Process common certificates */
	if (process_certificates(certs_common, ARRAY_SIZE(certs_common)) != 0) {
		goto cleanup;
	}

	/* Process STA-specific certificates */
	if (!is_ap) {
		if (process_certificates(certs_sta, ARRAY_SIZE(certs_sta)) != 0) {
			goto cleanup;
		}
	}

	/* Process AP-specific certificates if is_ap is true */
	if (is_ap) {
		if (process_certificates(certs_ap, ARRAY_SIZE(certs_ap)) != 0) {
			goto cleanup;
		}
	}

	return;

cleanup:
	for (size_t i = 0; i < ARRAY_SIZE(certs_common); i++) {
		if (certs_common[i].data) {
			k_free(*certs_common[i].data);
		}
	}

	if (!is_ap) {
		for (size_t i = 0; i < ARRAY_SIZE(certs_sta); i++) {
			if (certs_sta[i].data) {
				k_free(*certs_sta[i].data);
			}
		}
	}

	if (is_ap) {
		for (size_t i = 0; i < ARRAY_SIZE(certs_ap); i++) {
			if (certs_ap[i].data) {
				k_free(*certs_ap[i].data);
			}
		}
	}
}

void wifi_clear_enterprise_credentials(void)
{
	size_t i;

	const uint8_t *certs[] = {
		enterprise_creds_params.ca_cert,
		enterprise_creds_params.client_cert,
		enterprise_creds_params.client_key,
		enterprise_creds_params.server_cert,
		enterprise_creds_params.server_key,
		enterprise_creds_params.ca_cert2,
		enterprise_creds_params.client_cert2,
		enterprise_creds_params.client_key2,
	};

	for (i = 0; i < ARRAY_SIZE(certs); i++) {
		k_free((void *)certs[i]);
	}
	memset(&enterprise_creds_params, 0, sizeof(struct wifi_enterprise_creds_params));
}
#else
int config_process_blob(struct wpa_config *config, char *name, uint8_t *data,
			uint32_t data_len)
{
	struct wpa_config_blob *blob;

	if (!data || !data_len) {
		return -1;
	}

	blob = os_zalloc(sizeof(*blob));
	if (blob == NULL) {
		return -1;
	}

	blob->data = os_zalloc(data_len);
	if (blob->data == NULL) {
		os_free(blob);
		return -1;
	}

	blob->name = os_strdup(name);

	if (blob->name == NULL) {
		wpa_config_free_blob(blob);
		return -1;
	}

	os_memcpy(blob->data, data, data_len);
	blob->len = data_len;

	wpa_config_set_blob(config, blob);

	return 0;
}

int process_certificates(void)
{
	struct wpa_supplicant *wpa_s;
	int ret;
	char if_name[CONFIG_NET_INTERFACE_NAME_LEN + 1];
	struct net_if *iface = net_if_get_wifi_sta();

	ret = net_if_get_name(iface, if_name, sizeof(if_name));
	if (!ret) {
		LOG_ERR("Cannot get interface name (%d)", ret);
		return -1;
	}

	wpa_s = zephyr_get_handle_by_ifname(if_name);
	if (!wpa_s) {
		LOG_ERR("Unable to find the interface: %s, quitting", if_name);
		return -1;
	}

	wifi_set_enterprise_credentials(iface, 0);

	if (config_process_blob(wpa_s->conf, "ca_cert",
				enterprise_creds_params.ca_cert,
				enterprise_creds_params.ca_cert_len)) {
		return -1;
	}

	if (config_process_blob(wpa_s->conf, "client_cert",
				enterprise_creds_params.client_cert,
				enterprise_creds_params.client_cert_len)) {
		return -1;
	}

	if (config_process_blob(wpa_s->conf, "private_key",
				enterprise_creds_params.client_key,
				enterprise_creds_params.client_key_len)) {
		return -1;
	}

	return 0;
}

static void set_enterprise_creds_params(bool is_ap)
{
	enterprise_creds_params.ca_cert = (uint8_t *)ca_cert_test;
	enterprise_creds_params.ca_cert_len = ARRAY_SIZE(ca_cert_test);

	if (!is_ap) {
		enterprise_creds_params.client_cert = (uint8_t *)client_cert_test;
		enterprise_creds_params.client_cert_len = ARRAY_SIZE(client_cert_test);
		enterprise_creds_params.client_key = (uint8_t *)client_key_test;
		enterprise_creds_params.client_key_len = ARRAY_SIZE(client_key_test);
		enterprise_creds_params.ca_cert2 = (uint8_t *)ca_cert2_test;
		enterprise_creds_params.ca_cert2_len = ARRAY_SIZE(ca_cert2_test);
		enterprise_creds_params.client_cert2 = (uint8_t *)client_cert2_test;
		enterprise_creds_params.client_cert2_len = ARRAY_SIZE(client_cert2_test);
		enterprise_creds_params.client_key2 = (uint8_t *)client_key2_test;
		enterprise_creds_params.client_key2_len = ARRAY_SIZE(client_key2_test);

		return;
	}

	enterprise_creds_params.server_cert = (uint8_t *)server_cert_test;
	enterprise_creds_params.server_cert_len = ARRAY_SIZE(server_cert_test);
	enterprise_creds_params.server_key = (uint8_t *)server_key_test;
	enterprise_creds_params.server_key_len = ARRAY_SIZE(server_key_test);
}

void wifi_clear_enterprise_credentials(void)
{
	/**
	 * No operation needed because Wi-Fi credentials
	 * are statically configured at build time and
	 * no dynamic memory needs to be freed.
	 */
}
#endif /* CONFIG_WIFI_SHELL_RUNTIME_CERTIFICATES */

int wifi_set_enterprise_credentials(struct net_if *iface, bool is_ap)
{
#ifdef CONFIG_WIFI_SHELL_RUNTIME_CERTIFICATES
	wifi_clear_enterprise_credentials();
#endif /* CONFIG_WIFI_SHELL_RUNTIME_CERTIFICATES */
	set_enterprise_creds_params(is_ap);
	if (net_mgmt(NET_REQUEST_WIFI_ENTERPRISE_CREDS, iface,
			&enterprise_creds_params, sizeof(enterprise_creds_params))) {
		LOG_WRN("Set enterprise credentials failed\n");
		return -1;
	}

	return 0;
}
