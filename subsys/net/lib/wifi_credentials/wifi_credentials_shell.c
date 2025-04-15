/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L /* For strnlen() */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <zephyr/shell/shell.h>
#include <zephyr/init.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/wifi_utils.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/net_l2.h>
#include <zephyr/net/ethernet.h>

#include <zephyr/net/wifi_credentials.h>

LOG_MODULE_REGISTER(wifi_credentials_shell, CONFIG_WIFI_CREDENTIALS_LOG_LEVEL);

#define MAX_BANDS_STR_LEN 64
#define MACSTR "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx"

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE
#ifdef CONFIG_WIFI_CREDENTIALS_RUNTIME_CERTIFICATES
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
#endif /* CONFIG_WIFI_CREDENTIALS_RUNTIME_CERTIFICATES */
#endif /* CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE */

#if defined CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE
#ifdef CONFIG_WIFI_CREDENTIALS_RUNTIME_CERTIFICATES

struct wifi_enterprise_creds_params enterprise_creds_params;

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

static void set_enterprise_creds_params(struct wifi_enterprise_creds_params *params,
					bool is_ap)
{
	struct wifi_cert_data certs_common[] = {
		{
			.type = TLS_CREDENTIAL_CA_CERTIFICATE,
			.sec_tag = WIFI_CERT_CA_SEC_TAG,
			.data = &params->ca_cert,
			.len = &params->ca_cert_len,
		},
	};

	struct wifi_cert_data certs_sta[] = {
		{
			.type = TLS_CREDENTIAL_PRIVATE_KEY,
			.sec_tag = WIFI_CERT_CLIENT_KEY_SEC_TAG,
			.data = &params->client_key,
			.len = &params->client_key_len,
		},
		{
			.type = TLS_CREDENTIAL_PUBLIC_CERTIFICATE,
			.sec_tag = WIFI_CERT_CLIENT_SEC_TAG,
			.data = &params->client_cert,
			.len = &params->client_cert_len,
		},
		{
			.type = TLS_CREDENTIAL_CA_CERTIFICATE,
			.sec_tag = WIFI_CERT_CA_P2_SEC_TAG,
			.data = &params->ca_cert2,
			.len = &params->ca_cert2_len,
		},
		{
			.type = TLS_CREDENTIAL_PRIVATE_KEY,
			.sec_tag = WIFI_CERT_CLIENT_KEY_P2_SEC_TAG,
			.data = &params->client_key2,
			.len = &params->client_key2_len,
		},
		{
			.type = TLS_CREDENTIAL_PUBLIC_CERTIFICATE,
			.sec_tag = WIFI_CERT_CLIENT_P2_SEC_TAG,
			.data = &params->client_cert2,
			.len = &params->client_cert2_len,
		},
	};

	memset(params, 0, sizeof(*params));

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

	memcpy(&enterprise_creds_params, params, sizeof(*params));
	return;

cleanup:
	for (size_t i = 0; i < ARRAY_SIZE(certs_common); i++) {
		if (certs_common[i].data) {
			k_free(*certs_common[i].data);
			*certs_common[i].data = NULL;
		}
	}

	if (!is_ap) {
		for (size_t i = 0; i < ARRAY_SIZE(certs_sta); i++) {
			if (certs_sta[i].data) {
				k_free(*certs_sta[i].data);
				*certs_sta[i].data = NULL;
			}
		}
	}

}

static void clear_enterprise_creds_params(struct wifi_enterprise_creds_params *params)
{
	if (params == NULL) {
		return;
	}

	const uint8_t *certs[] = {
		params->ca_cert,
		params->client_cert,
		params->client_key,
		params->ca_cert2,
		params->client_cert2,
		params->client_key2,
	};

	for (size_t i = 0; i < ARRAY_SIZE(certs); i++) {
		k_free((void *)certs[i]);
	}
	memset(params, 0, sizeof(*params));
}
#else
static void set_enterprise_creds_params(struct wifi_enterprise_creds_params *params,
					 bool is_ap)
{
	params->ca_cert = (uint8_t *)ca_cert_test;
	params->ca_cert_len = ARRAY_SIZE(ca_cert_test);

	if (!is_ap) {
		params->client_cert = (uint8_t *)client_cert_test;
		params->client_cert_len = ARRAY_SIZE(client_cert_test);
		params->client_key = (uint8_t *)client_key_test;
		params->client_key_len = ARRAY_SIZE(client_key_test);
		params->ca_cert2 = (uint8_t *)ca_cert2_test;
		params->ca_cert2_len = ARRAY_SIZE(ca_cert2_test);
		params->client_cert2 = (uint8_t *)client_cert2_test;
		params->client_cert2_len = ARRAY_SIZE(client_cert2_test);
		params->client_key2 = (uint8_t *)client_key2_test;
		params->client_key2_len = ARRAY_SIZE(client_key2_test);

		return;
	}
}
#endif /* CONFIG_WIFI_CREDENTIALS_RUNTIME_CERTIFICATES */

static int wifi_set_enterprise_creds(const struct shell *sh, struct net_if *iface,
				     bool is_ap)
{
	struct wifi_enterprise_creds_params params = {0};

#ifdef CONFIG_WIFI_SHELL_RUNTIME_CERTIFICATES
	clear_enterprise_creds_params(&enterprise_creds_params);
#endif /* CONFIG_WIFI_SHELL_RUNTIME_CERTIFICATES */

	set_enterprise_creds_params(&params, is_ap);

	if (net_mgmt(NET_REQUEST_WIFI_ENTERPRISE_CREDS, iface, &params, sizeof(params))) {
		shell_warn(sh, "Set enterprise credentials failed\n");
		return -1;
	}

	return 0;
}
#endif /* CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE */

static void print_network_info(void *cb_arg, const char *ssid, size_t ssid_len)
{
	int ret = 0;
	struct wifi_credentials_personal creds = {0};
	const struct shell *sh = (const struct shell *)cb_arg;

	ret = wifi_credentials_get_by_ssid_personal_struct(ssid, ssid_len, &creds);
	if (ret) {
		shell_error(sh,
			    "An error occurred when trying to load credentials for network \"%.*s\""
			    ". err: %d",
			    (int)ssid_len, ssid, ret);
		return;
	}

	shell_fprintf(sh, SHELL_VT100_COLOR_DEFAULT,
		      "  network ssid: \"%.*s\", ssid_len: %d, type: %s", (int)ssid_len, ssid,
		      ssid_len, wifi_security_txt(creds.header.type));

	if (creds.header.type == WIFI_SECURITY_TYPE_PSK ||
	    creds.header.type == WIFI_SECURITY_TYPE_PSK_SHA256 ||
	    creds.header.type == WIFI_SECURITY_TYPE_SAE ||
	    creds.header.type == WIFI_SECURITY_TYPE_WPA_PSK) {
		shell_fprintf(sh, SHELL_VT100_COLOR_DEFAULT,
			      ", password: \"%.*s\", password_len: %d", (int)creds.password_len,
			      creds.password, creds.password_len);
	}

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE
	if (creds.header.type == WIFI_SECURITY_TYPE_EAP_TLS) {
		if (creds.header.key_passwd_length > 0) {
			shell_fprintf(sh, SHELL_VT100_COLOR_DEFAULT,
				      ", key_passwd: \"%.*s\", key_passwd_len: %d",
				      creds.header.key_passwd_length, creds.header.key_passwd,
				      creds.header.key_passwd_length);
		}
		if (creds.header.aid_length > 0) {
			shell_fprintf(sh, SHELL_VT100_COLOR_DEFAULT,
				      ", anon_id: \"%.*s\", anon_id_len: %d",
				      creds.header.aid_length, creds.header.anon_id,
				      creds.header.aid_length);
		}
	}
#endif /* CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE */

	if (creds.header.flags & WIFI_CREDENTIALS_FLAG_BSSID) {
		shell_fprintf(sh, SHELL_VT100_COLOR_DEFAULT, ", bssid: " MACSTR,
			      creds.header.bssid[0], creds.header.bssid[1], creds.header.bssid[2],
			      creds.header.bssid[3], creds.header.bssid[4], creds.header.bssid[5]);
	}

	if (creds.header.flags & WIFI_CREDENTIALS_FLAG_2_4GHz) {
		shell_fprintf(sh, SHELL_VT100_COLOR_DEFAULT, ", band: 2.4GHz");
	}

	if (creds.header.flags & WIFI_CREDENTIALS_FLAG_5GHz) {
		shell_fprintf(sh, SHELL_VT100_COLOR_DEFAULT, ", band: 5GHz");
	}

	if (creds.header.channel) {
		shell_fprintf(sh, SHELL_VT100_COLOR_DEFAULT, ", channel: %d", creds.header.channel);
	}

	if (creds.header.flags & WIFI_CREDENTIALS_FLAG_FAVORITE) {
		shell_fprintf(sh, SHELL_VT100_COLOR_DEFAULT, ", favorite");
	}

	if (creds.header.flags & WIFI_CREDENTIALS_FLAG_MFP_REQUIRED) {
		shell_fprintf(sh, SHELL_VT100_COLOR_DEFAULT, ", MFP_REQUIRED");
	} else if (creds.header.flags & WIFI_CREDENTIALS_FLAG_MFP_DISABLED) {
		shell_fprintf(sh, SHELL_VT100_COLOR_DEFAULT, ", MFP_DISABLED");
	} else {
		shell_fprintf(sh, SHELL_VT100_COLOR_DEFAULT, ", MFP_OPTIONAL");
	}

	if (creds.header.timeout) {
		shell_fprintf(sh, SHELL_VT100_COLOR_DEFAULT, ", timeout: %d", creds.header.timeout);
	}

	shell_fprintf(sh, SHELL_VT100_COLOR_DEFAULT, "\n");
}

static int cmd_add_network(const struct shell *sh, size_t argc, char *argv[])
{
	int opt;
	int opt_index = 0;
	struct getopt_state *state;
	static const struct option long_options[] = {
		{"ssid", required_argument, 0, 's'},	 {"passphrase", required_argument, 0, 'p'},
		{"key-mgmt", required_argument, 0, 'k'}, {"ieee-80211w", required_argument, 0, 'w'},
		{"bssid", required_argument, 0, 'm'},	 {"band", required_argument, 0, 'b'},
		{"channel", required_argument, 0, 'c'},	 {"timeout", required_argument, 0, 't'},
		{"identity", required_argument, 0, 'a'}, {"key-passwd", required_argument, 0, 'K'},
		{"help", no_argument, 0, 'h'},		 {0, 0, 0, 0}};
	char *endptr;
	bool secure_connection = false;
	uint8_t band;
	struct wifi_credentials_personal creds = {0};

	const uint8_t all_bands[] = {WIFI_FREQ_BAND_2_4_GHZ, WIFI_FREQ_BAND_5_GHZ,
				     WIFI_FREQ_BAND_6_GHZ};
	bool found = false;
	char bands_str[MAX_BANDS_STR_LEN] = {0};
	size_t offset = 0;
	long channel;
	long mfp = WIFI_MFP_OPTIONAL;

	while ((opt = getopt_long(argc, argv, "s:p:k:w:b:c:m:t:a:K:h", long_options, &opt_index)) !=
	       -1) {
		state = getopt_state_get();
		switch (opt) {
		case 's':
			creds.header.ssid_len = strlen(state->optarg);
			if (creds.header.ssid_len > WIFI_SSID_MAX_LEN) {
				shell_warn(sh, "SSID too long (max %d characters)\n",
					   WIFI_SSID_MAX_LEN);
				return -EINVAL;
			}
			memcpy(creds.header.ssid, state->optarg, creds.header.ssid_len);
			break;
		case 'k':
			creds.header.type = atoi(state->optarg);
			if (creds.header.type) {
				secure_connection = true;
			}
			break;
		case 'p':
			creds.password_len = strlen(state->optarg);
			if (creds.password_len < WIFI_PSK_MIN_LEN) {
				shell_warn(sh, "Passphrase should be minimum %d characters\n",
					   WIFI_PSK_MIN_LEN);
				return -EINVAL;
			}
			if (creds.password_len > WIFI_PSK_MAX_LEN) {
				shell_warn(sh, "Passphrase too long (max %d characters)\n",
					   WIFI_PSK_MAX_LEN);
				return -EINVAL;
			}
			memcpy(creds.password, state->optarg, creds.password_len);
			break;
		case 'c':
			channel = strtol(state->optarg, &endptr, 10);
			if (*endptr != '\0') {
				shell_error(sh, "Invalid channel: %s\n", state->optarg);
				return -EINVAL;
			}

			for (band = 0; band < ARRAY_SIZE(all_bands); band++) {
				offset += snprintf(bands_str + offset, sizeof(bands_str) - offset,
						   "%s%s", band ? "," : "",
						   wifi_band_txt(all_bands[band]));
				if (offset >= sizeof(bands_str)) {
					shell_error(sh,
						    "Failed to parse channel: %ld: "
						    "band string too long\n",
						    channel);
					return -EINVAL;
				}

				if (wifi_utils_validate_chan(all_bands[band], channel)) {
					found = true;
					break;
				}
			}

			if (!found) {
				shell_error(sh, "Invalid channel: %ld, checked bands: %s\n",
					    channel, bands_str);
				return -EINVAL;
			}

			creds.header.channel = channel;
			break;
		case 'b':
			switch (atoi(state->optarg)) {
			case 2:
				creds.header.flags |= WIFI_CREDENTIALS_FLAG_2_4GHz;
				break;
			case 5:
				creds.header.flags |= WIFI_CREDENTIALS_FLAG_5GHz;
				break;
			case 6:
				creds.header.flags |= WIFI_CREDENTIALS_FLAG_6GHz;
				break;
			default:
				shell_error(sh, "Invalid band: %d\n", atoi(state->optarg));
				return -EINVAL;
			}
			break;
		case 'w':
			if (creds.header.type == WIFI_SECURITY_TYPE_NONE ||
			    creds.header.type == WIFI_SECURITY_TYPE_WPA_PSK) {
				shell_error(sh, "MFP not supported for security type %s",
					    wifi_security_txt(creds.header.type));
				return -ENOTSUP;
			}
			mfp = strtol(state->optarg, &endptr, 10);
			if (*endptr != '\0') {
				shell_error(sh, "Invalid IEEE 802.11w value: %s", state->optarg);
				return -EINVAL;
			}
			if (mfp == WIFI_MFP_DISABLE) {
				creds.header.flags |= WIFI_CREDENTIALS_FLAG_MFP_DISABLED;
			} else if (mfp == WIFI_MFP_REQUIRED) {
				creds.header.flags |= WIFI_CREDENTIALS_FLAG_MFP_REQUIRED;
			} else if (mfp > 2) {
				shell_error(sh, "Invalid IEEE 802.11w value: %s",
					    state->optarg);
				return -EINVAL;
			}
			break;
		case 'm':
			if (net_bytes_from_str(creds.header.bssid, sizeof(creds.header.bssid),
					       state->optarg) < 0) {
				shell_warn(sh, "Invalid MAC address\n");
				return -EINVAL;
			}
			creds.header.flags |= WIFI_CREDENTIALS_FLAG_BSSID;
			break;
		case 'a':
			creds.header.aid_length = strlen(state->optarg);
			if (creds.header.aid_length > WIFI_ENT_IDENTITY_MAX_LEN) {
				shell_warn(sh, "anon_id too long (max %d characters)\n",
					   WIFI_ENT_IDENTITY_MAX_LEN);
				return -EINVAL;
			}
			memcpy(creds.header.anon_id, state->optarg, creds.header.aid_length);
			creds.header.flags |= WIFI_CREDENTIALS_FLAG_ANONYMOUS_IDENTITY;
			break;
		case 'K':
			creds.header.key_passwd_length = strlen(state->optarg);
			if (creds.header.key_passwd_length > WIFI_ENT_PSWD_MAX_LEN) {
				shell_warn(sh, "key_passwd too long (max %d characters)\n",
					   WIFI_ENT_PSWD_MAX_LEN);
				return -EINVAL;
			}
			memcpy(creds.header.key_passwd, state->optarg,
			       creds.header.key_passwd_length);
			creds.header.flags |= WIFI_CREDENTIALS_FLAG_KEY_PASSWORD;
			break;
		case 'h':
			shell_help(sh);
			return -ENOEXEC;
		default:
			shell_error(sh, "Invalid option %c\n", state->optopt);
			return -EINVAL;
		}
	}
	if (creds.password_len > 0 && !secure_connection) {
		shell_warn(sh, "Passphrase provided without security configuration\n");
	}

	if (creds.header.ssid_len == 0) {
		shell_error(sh, "SSID not provided\n");
		shell_help(sh);
		return -EINVAL;
	}

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE
	struct net_if *iface = net_if_get_wifi_sta();

	/* Load the enterprise credentials if needed */
	if (creds.header.type == WIFI_SECURITY_TYPE_EAP_TLS ||
	    creds.header.type == WIFI_SECURITY_TYPE_EAP_PEAP_MSCHAPV2 ||
	    creds.header.type == WIFI_SECURITY_TYPE_EAP_PEAP_GTC ||
	    creds.header.type == WIFI_SECURITY_TYPE_EAP_TTLS_MSCHAPV2 ||
	    creds.header.type == WIFI_SECURITY_TYPE_EAP_PEAP_TLS) {
		wifi_set_enterprise_creds(sh, iface, 0);
	}
#endif /* CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE */

	return wifi_credentials_set_personal_struct(&creds);
}

static int cmd_delete_network(const struct shell *sh, size_t argc, char *argv[])
{
	if (argc != 2) {
		shell_print(sh, "Usage: wifi cred delete \"network name\"");
		return -EINVAL;
	}

	if (strnlen(argv[1], WIFI_SSID_MAX_LEN + 1) > WIFI_SSID_MAX_LEN) {
		shell_error(sh, "SSID too long");
		return -EINVAL;
	}

	shell_print(sh, "\tDeleting network ssid: \"%s\", ssid_len: %d", argv[1], strlen(argv[1]));

#ifdef CONFIG_WIFI_SHELL_RUNTIME_CERTIFICATES
	/* Clear the certificates */
	clear_enterprise_creds_params(&enterprise_creds_params);
#endif /* CONFIG_WIFI_SHELL_RUNTIME_CERTIFICATES */

	return wifi_credentials_delete_by_ssid(argv[1], strlen(argv[1]));
}

static int cmd_list_networks(const struct shell *sh, size_t argc, char *argv[])
{
	wifi_credentials_for_each_ssid(print_network_info, (void *)sh);
	return 0;
}

#if CONFIG_WIFI_CREDENTIALS_CONNECT_STORED

static int cmd_auto_connect(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_wifi_sta();

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE
	wifi_set_enterprise_creds(sh, iface, 0);
#endif /* CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE */
	int rc = net_mgmt(NET_REQUEST_WIFI_CONNECT_STORED, iface, NULL, 0);

	if (rc) {
		shell_error(sh,
			    "An error occurred when trying to auto-connect to a network. err: %d",
			    rc);
	}

	return 0;
}

#endif /* CONFIG_WIFI_CREDENTIALS_CONNECT_STORED */

SHELL_STATIC_SUBCMD_SET_CREATE(sub_wifi_cred,
	SHELL_CMD_ARG(add, NULL,
		      "Add network to storage.\n"
		      "<-s --ssid \"<SSID>\">: SSID.\n"
		      "[-c --channel]: Channel that needs to be scanned for connection. 0:any channel.\n"
		      "[-b, --band] 0: any band (2:2.4GHz, 5:5GHz, 6:6GHz]\n"
		      "[-p, --passphrase]: Passphrase (valid only for secure SSIDs)\n"
		      "[-k, --key-mgmt]: Key Management type (valid only for secure SSIDs)\n"
		      "0:None, 1:WPA2-PSK, 2:WPA2-PSK-256, 3:SAE-HNP, 4:SAE-H2E, 5:SAE-AUTO, 6:WAPI,"
		      " 7:EAP-TLS, 8:WEP, 9: WPA-PSK, 10: WPA-Auto-Personal, 11: DPP\n"
		      "12: EAP-PEAP-MSCHAPv2, 13: EAP-PEAP-GTC, 14: EAP-TTLS-MSCHAPv2,\n"
		      "15: EAP-PEAP-TLS, 20: SAE-EXT-KEY\n"
		      "[-w, --ieee-80211w]: MFP (optional: needs security type to be specified)\n"
		      ": 0:Disable, 1:Optional, 2:Required.\n"
		      "[-m, --bssid]: MAC address of the AP (BSSID).\n"
		      "[-t, --timeout]: Timeout for the connection attempt (in seconds).\n"
		      "[-a, --anon-id]: Anonymous identity for enterprise mode.\n"
		      "[-K, --key1-pwd for eap phase1 or --key2-pwd for eap phase2]:\n"
		      "Private key passwd for enterprise mode. Default no password for private key.\n"
		      "[-S, --wpa3-enterprise]: WPA3 enterprise mode:\n"
		      "Default 0: Not WPA3 enterprise mode.\n"
		      "1:Suite-b mode, 2:Suite-b-192-bit mode, 3:WPA3-enterprise-only mode.\n"
		      "[-T, --TLS-cipher]: 0:TLS-NONE, 1:TLS-ECC-P384, 2:TLS-RSA-3K.\n"
		      "[-V, --eap-version]: 0 or 1. Default 1: eap version 1.\n"
		      "[-I, --eap-id1]: Client Identity. Default no eap identity.\n"
		      "[-P, --eap-pwd1]: Client Password.\n"
		      "Default no password for eap user.\n"
		      "[-R, --ieee-80211r]: Use IEEE80211R fast BSS transition connect."
		      "[-h, --help]: Print out the help for the add network command.\n",
		      cmd_add_network,
		      2, 12),
	SHELL_CMD_ARG(delete, NULL,
		      "Delete network from storage.\n",
		      cmd_delete_network,
		      0, 0),
	SHELL_CMD_ARG(list, NULL,
		      "List stored networks.\n",
		      cmd_list_networks,
		      0, 0),

#if CONFIG_WIFI_CREDENTIALS_CONNECT_STORED
	SHELL_CMD_ARG(auto_connect, NULL,
		      "Connect to any stored network.\n",
		      cmd_auto_connect,
		      0, 0),
#endif /* CONFIG_WIFI_CREDENTIALS_CONNECT_STORED */

	SHELL_SUBCMD_SET_END
);

SHELL_SUBCMD_ADD((wifi), cred, &sub_wifi_cred,
		 "Wifi credentials management.\n",
		 NULL,
		 0, 0);
