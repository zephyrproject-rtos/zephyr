/**
 * Copyright 2023-2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/net/wifi_mgmt.h>
#include "includes.h"
#include "common.h"
#include "common/defs.h"
#include "wpa_supplicant_i.h"
#include "hostapd.h"
#include "hostapd_cli_zephyr.h"
#include "ap_drv_ops.h"
#include "hapd_main.h"
#include "hapd_api.h"
#include "supp_events.h"
#include "supp_api.h"

K_MUTEX_DEFINE(hostapd_mutex);

#ifdef CONFIG_WIFI_NM_HOSTAPD_CRYPTO_ENTERPRISE
static struct wifi_eap_config hapd_eap_config[] = {
	{WIFI_SECURITY_TYPE_EAP_TLS, WIFI_EAP_TYPE_TLS, WIFI_EAP_TYPE_NONE, "TLS", NULL},
	{WIFI_SECURITY_TYPE_EAP_PEAP_MSCHAPV2, WIFI_EAP_TYPE_PEAP, WIFI_EAP_TYPE_MSCHAPV2, "PEAP",
	 "auth=MSCHAPV2"},
	{WIFI_SECURITY_TYPE_EAP_PEAP_GTC, WIFI_EAP_TYPE_PEAP, WIFI_EAP_TYPE_GTC, "PEAP",
	 "auth=GTC"},
	{WIFI_SECURITY_TYPE_EAP_TTLS_MSCHAPV2, WIFI_EAP_TYPE_TTLS, WIFI_EAP_TYPE_NONE, "TTLS",
	 "auth=MSCHAPV2"},
	{WIFI_SECURITY_TYPE_EAP_PEAP_TLS, WIFI_EAP_TYPE_PEAP, WIFI_EAP_TYPE_TLS, "PEAP",
	 "auth=TLS"},
};

static struct wifi_enterprise_creds_params hapd_enterprise_creds;
#endif

#define hostapd_cli_cmd_v(cmd, ...) ({					\
	bool status;							\
									\
	if (zephyr_hostapd_cli_cmd_v(iface->ctrl_conn, cmd, ##__VA_ARGS__) < 0) {		\
		wpa_printf(MSG_ERROR,					\
			   "Failed to execute wpa_cli command: %s",	\
			   cmd);					\
		status = false;						\
	} else {							\
		status = true;						\
	}								\
									\
	status;								\
})

static inline struct hostapd_iface *get_hostapd_handle(const struct device *dev)
{
	struct net_if *iface = net_if_lookup_by_dev(dev);
	char if_name[CONFIG_NET_INTERFACE_NAME_LEN + 1];
	struct hostapd_iface *hapd;
	int ret;

	if (!iface) {
		wpa_printf(MSG_ERROR, "Interface for device %s not found", dev->name);
		return NULL;
	}

	ret = net_if_get_name(iface, if_name, sizeof(if_name));
	if (!ret) {
		wpa_printf(MSG_ERROR, "Cannot get interface name (%d)", ret);
		return NULL;
	}

	hapd = zephyr_get_hapd_handle_by_ifname(if_name);
	if (!hapd) {
		wpa_printf(MSG_ERROR, "Interface %s not found", if_name);
		return NULL;
	}

	return hapd;
}

#ifdef CONFIG_WIFI_NM_HOSTAPD_CRYPTO_ENTERPRISE
static int hapd_process_cert_data(struct hostapd_bss_config *conf,
				  char *type, uint8_t *data, uint32_t data_len)
{
	if (os_strcmp(type, "ca_cert_blob") == 0) {
		conf->ca_cert_blob = data;
		conf->ca_cert_blob_len = data_len;
	} else if (os_strcmp(type, "server_cert_blob") == 0) {
		conf->server_cert_blob = data;
		conf->server_cert_blob_len = data_len;
	} else if (os_strcmp(type, "private_key_blob") == 0) {
		conf->private_key_blob = data;
		conf->private_key_blob_len = data_len;
	} else if (os_strcmp(type, "dh_blob") == 0) {
		conf->dh_blob = data;
		conf->dh_blob_len = data_len;
	} else {
		wpa_printf(MSG_ERROR, "input type error");
		return -ENOTSUP;
	}

	return 0;
}

static int hapd_get_eap_config(struct wifi_connect_req_params *params,
			       struct wifi_eap_config *eap_cfg)
{
	unsigned int index = 0;

	for (index = 0; index < ARRAY_SIZE(hapd_eap_config); index++) {
		if (params->security == hapd_eap_config[index].type) {
			memcpy(eap_cfg, &hapd_eap_config[index], sizeof(struct wifi_eap_config));
			break;
		}
	}

	if (index == ARRAY_SIZE(hapd_eap_config)) {
		wpa_printf(MSG_ERROR, "Get eap method type with security type: %d",
		params->security);
		return -ENOTSUP;
	}

	return 0;
}

static struct hostapd_eap_user *hapd_process_eap_user_phase1(
	struct wifi_connect_req_params *params, struct hostapd_eap_user **pnew_user)
{
	struct hostapd_eap_user *user = NULL, *tail = NULL, *new_user = NULL;
	struct wifi_eap_config eap_cfg;

	user = os_zalloc(sizeof(*user));
	if (user == NULL) {
		wpa_printf(MSG_ERROR, "EAP user allocation failed");
		goto failed;
	}

	user->force_version = -1;
	if (params->eap_ver >= 0) {
		user->force_version = params->eap_ver;
	}

	if (hapd_get_eap_config(params, &eap_cfg)) {
		goto failed;
	}

	user->methods[0].method = eap_cfg.eap_type_phase1;
	user->methods[0].vendor = 0;

	if (tail == NULL) {
		tail = new_user = user;
	} else {
		tail->next = user;
		tail       = user;
	}

	*pnew_user = new_user;

	return tail;

failed:
	if (user) {
		hostapd_config_free_eap_user(user);
	}

	return NULL;
}

static int hapd_process_eap_user(struct wifi_connect_req_params *params,
				 struct hostapd_bss_config *conf)
{
	struct hostapd_eap_user *user = NULL, *tail = NULL, *user_list = NULL;
	int i, nusers = params->nusers;
	const char *identity, *password;
	struct wifi_eap_config eap_cfg;
	int ret = 0;

	if (hapd_get_eap_config(params, &eap_cfg)) {
		goto failed;
	}

	if (eap_cfg.phase2 != NULL) {
		tail = hapd_process_eap_user_phase1(params, &user_list);
	}

	if (eap_cfg.phase2 != NULL && !nusers) {
		wpa_printf(MSG_ERROR, "EAP users not found.");
		goto failed;
	}

	for (i = 0; i < nusers; i++) {
		user = os_zalloc(sizeof(*user));
		if (user == NULL) {
			wpa_printf(MSG_ERROR, "EAP user allocation failed");
			goto failed;
		}

		user->force_version = -1;
		if (params->eap_ver >= 0) {
			user->force_version = params->eap_ver;
		}

		identity = params->identities[i];
		password = params->passwords[i];

		user->identity = os_memdup(identity, os_strlen(identity));
		if (user->identity == NULL) {
			wpa_printf(MSG_ERROR,
				   "Failed to allocate "
				   "memory for EAP identity");
			goto failed;
		}
		user->identity_len = os_strlen(identity);

		user->methods[0].method = eap_cfg.eap_type_phase1;
		user->methods[0].vendor = 0;

		if (eap_cfg.phase2 != NULL) {
			user->methods[0].method = eap_cfg.eap_type_phase2;
			user->password = os_memdup(password, os_strlen(password));
			if (user->password == NULL) {
				wpa_printf(MSG_ERROR,
					   "Failed to allocate "
					   "memory for EAP password");
				goto failed;
			}
			user->password_len = os_strlen(password);

			user->phase2 = 1;
		}

		if (params->security == WIFI_SECURITY_TYPE_EAP_TTLS_MSCHAPV2) {
			user->ttls_auth |= 0x1E;
		}

		if (tail == NULL) {
			tail = user_list = user;
		} else {
			tail->next = user;
			tail       = user;
		}

		continue;

failed:
		if (user) {
			hostapd_config_free_eap_user(user);
		}

		ret = -1;
		break;
	}

	if (ret == 0) {
		hostapd_config_free_eap_users(conf->eap_user);
		conf->eap_user = user_list;
	} else {
		hostapd_config_free_eap_users(user_list);
	}

	return ret;
}

int hapd_process_enterprise_config(struct hostapd_iface *iface,
				   struct wifi_connect_req_params *params)
{
	struct wifi_eap_cipher_config cipher_config = {
		NULL, "DEFAULT:!EXP:!LOW", "CCMP", "CCMP", "AES-128-CMAC", NULL};
	int ret = 0;

	if (process_cipher_config(params, &cipher_config)) {
		goto out;
	}

	if (!hostapd_cli_cmd_v("set wpa %d", WPA_PROTO_RSN)) {
		goto out;
	}

	if (!hostapd_cli_cmd_v("set wpa_key_mgmt %s", cipher_config.key_mgmt)) {
		goto out;
	}

	if (!hostapd_cli_cmd_v("set rsn_pairwise %s", cipher_config.pairwise_cipher)) {
		goto out;
	}

	if (!hostapd_cli_cmd_v("set wpa_pairwise %s", cipher_config.pairwise_cipher)) {
		goto out;
	}

	if (!hostapd_cli_cmd_v("set group_cipher %s", cipher_config.group_cipher)) {
		goto out;
	}

	if (!hostapd_cli_cmd_v("set group_mgmt_cipher %s", cipher_config.group_mgmt_cipher)) {
		goto out;
	}

	if (cipher_config.tls_flags != NULL) {
		if (!hostapd_cli_cmd_v("set tls_flags %s", cipher_config.tls_flags)) {
			goto out;
		}
	}

	if (!hostapd_cli_cmd_v("set ieee8021x %d", 1)) {
		goto out;
	}

	if (!hostapd_cli_cmd_v("set eapol_version %d", 2)) {
		goto out;
	}

	if (!hostapd_cli_cmd_v("set eap_server %d", 1)) {
		goto out;
	}

	if (hapd_process_cert_data(iface->bss[0]->conf, "ca_cert_blob",
		hapd_enterprise_creds.ca_cert, hapd_enterprise_creds.ca_cert_len)) {
		goto out;
	}

	if (hapd_process_cert_data(iface->bss[0]->conf, "server_cert_blob",
		hapd_enterprise_creds.server_cert, hapd_enterprise_creds.server_cert_len)) {
		goto out;
	}

	if (hapd_process_cert_data(iface->bss[0]->conf, "private_key_blob",
		hapd_enterprise_creds.server_key, hapd_enterprise_creds.server_key_len)) {
		goto out;
	}

	if (hapd_process_cert_data(iface->bss[0]->conf, "dh_blob",
		hapd_enterprise_creds.dh_param, hapd_enterprise_creds.dh_param_len)) {
		goto out;
	}

	if (!hostapd_cli_cmd_v("set private_key_passwd %s", params->key_passwd)) {
		goto out;
	}

	if (hapd_process_eap_user(params, iface->bss[0]->conf)) {
		goto out;
	}

	return ret;
out:
	return -1;
}

int hostapd_add_enterprise_creds(const struct device *dev,
				 struct wifi_enterprise_creds_params *creds)
{
	int ret = 0;

	if (!creds) {
		ret = -1;
		wpa_printf(MSG_ERROR, "enterprise creds is NULL");
		goto out;
	}

	memcpy((void *)&hapd_enterprise_creds, (void *)creds,
	       sizeof(struct wifi_enterprise_creds_params));

out:
	return ret;
}
#endif

bool hostapd_ap_reg_domain(const struct device *dev,
	struct wifi_reg_domain *reg_domain)
{
	struct hostapd_iface *iface;

	iface = get_hostapd_handle(dev);
	if (iface == NULL) {
		wpa_printf(MSG_ERROR, "Interface %s not found", dev->name);
		return false;
	}

	return hostapd_cli_cmd_v("set country_code %s", reg_domain->country_code);
}

static int hapd_config_chan_center_seg0(struct hostapd_iface *iface,
	struct wifi_connect_req_params *params)
{
	int ret = 0;
	uint8_t center_freq_seg0_idx = 0;
	uint8_t oper_chwidth = CHANWIDTH_USE_HT;
	const uint8_t *center_freq = NULL;
	static const uint8_t center_freq_40MHz[] = {38,  46,  54,  62,  102, 110,
						    118, 126, 134, 142, 151, 159};
	static const uint8_t center_freq_80MHz[] = {42, 58, 106, 122, 138, 155};
	uint8_t index, index_max, chan_idx, ch_offset = 0;

	/* Unless ACS is being used, both "channel" and "vht_oper_centr_freq_seg0_idx"
	 * parameters must be set.
	 */
	switch (params->bandwidth) {
	case WIFI_FREQ_BANDWIDTH_20MHZ:
		oper_chwidth = CHANWIDTH_USE_HT;
		center_freq_seg0_idx = params->channel;
		break;
	case WIFI_FREQ_BANDWIDTH_40MHZ:
		oper_chwidth = CHANWIDTH_USE_HT;
		center_freq = center_freq_40MHz;
		index_max = ARRAY_SIZE(center_freq_40MHz);
		ch_offset = 2;
		break;
	case WIFI_FREQ_BANDWIDTH_80MHZ:
		oper_chwidth = CHANWIDTH_80MHZ;
		center_freq = center_freq_80MHz;
		index_max = ARRAY_SIZE(center_freq_80MHz);
		ch_offset = 6;
		break;
	default:
		return -EINVAL;
	}

	if (params->bandwidth != WIFI_FREQ_BANDWIDTH_20MHZ) {
		chan_idx = params->channel;
		for (index = 0; index < index_max; index++) {
			if ((chan_idx >= (center_freq[index] - ch_offset)) &&
			    (chan_idx <= (center_freq[index] + ch_offset))) {
				center_freq_seg0_idx = center_freq[index];
				break;
			}
		}
	}

	if (!hostapd_cli_cmd_v("set vht_oper_chwidth %d", oper_chwidth)) {
		goto out;
	}
	if (!hostapd_cli_cmd_v("set vht_oper_centr_freq_seg0_idx %d", center_freq_seg0_idx)) {
		goto out;
	}
#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_11AX
	if (!hostapd_cli_cmd_v("set he_oper_chwidth %d", oper_chwidth)) {
		goto out;
	}
	if (!hostapd_cli_cmd_v("set he_oper_centr_freq_seg0_idx %d", center_freq_seg0_idx)) {
		goto out;
	}
#endif

	return ret;
out:
	return -EINVAL;
}

int hapd_config_network(struct hostapd_iface *iface,
			struct wifi_connect_req_params *params)
{
	int ret = 0;

	if (!hostapd_cli_cmd_v("set ssid %s", params->ssid)) {
		goto out;
	}

	if (params->channel == 0) {
		if (params->band == 0) {
			if (!hostapd_cli_cmd_v("set hw_mode g")) {
				goto out;
			}
		} else if (params->band == 1) {
			if (!hostapd_cli_cmd_v("set hw_mode a")) {
				goto out;
			}
		} else {
			wpa_printf(MSG_ERROR, "Invalid band %d", params->band);
			goto out;
		}
	} else if (params->channel > 14) {
		if (!hostapd_cli_cmd_v("set hw_mode a")) {
			goto out;
		}
	} else {
		if (!hostapd_cli_cmd_v("set hw_mode g")) {
			goto out;
		}
	}

	if (!hostapd_cli_cmd_v("set channel %d", params->channel)) {
		goto out;
	}

	ret = hapd_config_chan_center_seg0(iface, params);
	if (ret) {
		goto out;
	}

	if (params->security != WIFI_SECURITY_TYPE_NONE) {
		if (params->security == WIFI_SECURITY_TYPE_WPA_PSK) {
			if (!hostapd_cli_cmd_v("set wpa 1")) {
				goto out;
			}
			if (!hostapd_cli_cmd_v("set wpa_key_mgmt WPA-PSK")) {
				goto out;
			}
			if (!hostapd_cli_cmd_v("set wpa_passphrase %s", params->psk)) {
				goto out;
			}
			if (!hostapd_cli_cmd_v("set wpa_pairwise CCMP")) {
				goto out;
			}
		} else if (params->security == WIFI_SECURITY_TYPE_PSK) {
			if (!hostapd_cli_cmd_v("set wpa 2")) {
				goto out;
			}
			if (!hostapd_cli_cmd_v("set wpa_key_mgmt WPA-PSK")) {
				goto out;
			}
			if (!hostapd_cli_cmd_v("set wpa_passphrase %s", params->psk)) {
				goto out;
			}
			if (!hostapd_cli_cmd_v("set rsn_pairwise CCMP")) {
				goto out;
			}
		} else if (params->security == WIFI_SECURITY_TYPE_PSK_SHA256) {
			if (!hostapd_cli_cmd_v("set wpa 2")) {
				goto out;
			}
			if (!hostapd_cli_cmd_v("set wpa_key_mgmt WPA-PSK-SHA256")) {
				goto out;
			}
			if (!hostapd_cli_cmd_v("set wpa_passphrase %s", params->psk)) {
				goto out;
			}
			if (!hostapd_cli_cmd_v("set rsn_pairwise CCMP")) {
				goto out;
			}
		} else if (params->security == WIFI_SECURITY_TYPE_SAE_HNP ||
			   params->security == WIFI_SECURITY_TYPE_SAE_H2E ||
			   params->security == WIFI_SECURITY_TYPE_SAE_AUTO) {
			if (!hostapd_cli_cmd_v("set wpa 2")) {
				goto out;
			}
			if (!hostapd_cli_cmd_v("set wpa_key_mgmt SAE")) {
				goto out;
			}
			if (!hostapd_cli_cmd_v("set sae_password %s",
					       params->sae_password ? params->sae_password :
					       params->psk)) {
				goto out;
			}
			if (!hostapd_cli_cmd_v("set rsn_pairwise CCMP")) {
				goto out;
			}
			if (params->security == WIFI_SECURITY_TYPE_SAE_H2E ||
			    params->security == WIFI_SECURITY_TYPE_SAE_AUTO) {
				if (!hostapd_cli_cmd_v("set sae_pwe %d",
				    (params->security == WIFI_SECURITY_TYPE_SAE_H2E)
						? 1
						: 2)) {
					goto out;
				}
			}
		} else if (params->security == WIFI_SECURITY_TYPE_WPA_AUTO_PERSONAL) {
			if (!hostapd_cli_cmd_v("set wpa 2")) {
				goto out;
			}

			if (!hostapd_cli_cmd_v("set wpa_key_mgmt WPA-PSK SAE")) {
				goto out;
			}

			if (!hostapd_cli_cmd_v("set wpa_passphrase %s", params->psk)) {
				goto out;
			}

			if (!hostapd_cli_cmd_v("set sae_password %s",
					       params->sae_password ? params->sae_password
								    : params->psk)) {
				goto out;
			}

			if (!hostapd_cli_cmd_v("set rsn_pairwise CCMP")) {
				goto out;
			}

			if (!hostapd_cli_cmd_v("set sae_pwe 2")) {
				goto out;
			}
		} else if (params->security == WIFI_SECURITY_TYPE_DPP) {
			if (!hostapd_cli_cmd_v("set wpa 2")) {
				goto out;
			}
			if (!hostapd_cli_cmd_v("set wpa_key_mgmt WPA-PSK DPP")) {
				goto out;
			}
			if (!hostapd_cli_cmd_v("set wpa_passphrase %s", params->psk)) {
				goto out;
			}
			if (!hostapd_cli_cmd_v("set wpa_pairwise CCMP")) {
				goto out;
			}
			if (!hostapd_cli_cmd_v("set rsn_pairwise CCMP")) {
				goto out;
			}
			if (!hostapd_cli_cmd_v("set dpp_configurator_connectivity 1")) {
				goto out;
			}
#ifdef CONFIG_WIFI_NM_HOSTAPD_CRYPTO_ENTERPRISE
		} else if (is_eap_valid_security(params->security)) {
			if (hapd_process_enterprise_config(iface, params)) {
				goto out;
			}
#endif
		} else {
			wpa_printf(MSG_ERROR, "Security type %d is not supported",
				   params->security);
			goto out;
		}
	} else {
		if (!hostapd_cli_cmd_v("set wpa 0")) {
			goto out;
		}
		iface->bss[0]->conf->wpa_key_mgmt = WPA_KEY_MGMT_NONE;
	}

	if (!hostapd_cli_cmd_v("set ieee80211w %d", params->mfp)) {
		goto out;
	}

	if (!hostapd_cli_cmd_v("set ignore_broadcast_ssid %d", params->ignore_broadcast_ssid)) {
		goto out;
	}

	return ret;
out:
	return -1;
}

static int set_ap_config_params(const struct device *dev, struct wifi_ap_config_params *params)
{
	const struct wifi_mgmt_ops *const wifi_mgmt_api = get_wifi_mgmt_api(dev);

	if (wifi_mgmt_api == NULL || wifi_mgmt_api->ap_config_params == NULL) {
		return -ENOTSUP;
	}

	return wifi_mgmt_api->ap_config_params(dev, params);
}

int hostapd_ap_config_params(const struct device *dev, struct wifi_ap_config_params *params)
{
	struct hostapd_iface *iface;
	int ret = 0;

	ret = set_ap_config_params(dev, params);
	if (ret && (ret != -ENOTSUP)) {
		wpa_printf(MSG_ERROR, "Failed to set ap config params");
		return -EINVAL;
	}

	k_mutex_lock(&hostapd_mutex, K_FOREVER);

	iface = get_hostapd_handle(dev);
	if (iface == NULL) {
		ret = -ENOENT;
		wpa_printf(MSG_ERROR, "Interface %s not found", dev->name);
		goto out;
	}

	if (iface->state > HAPD_IFACE_DISABLED) {
		ret = -EBUSY;
		wpa_printf(MSG_ERROR, "Interface %s is not in disable state", dev->name);
		goto out;
	}

	if (params->type & WIFI_AP_CONFIG_PARAM_MAX_NUM_STA) {
		if (!hostapd_cli_cmd_v("set max_num_sta %d", params->max_num_sta)) {
			ret = -EINVAL;
			wpa_printf(MSG_ERROR, "Failed to set maximum number of stations");
			goto out;
		}
		wpa_printf(MSG_INFO, "Set maximum number of stations: %d", params->max_num_sta);
	}

	if (params->type & WIFI_AP_CONFIG_PARAM_HT_CAPAB) {
		if (!hostapd_cli_cmd_v("set ht_capab %s", params->ht_capab)) {
			ret = -EINVAL;
			wpa_printf(MSG_ERROR, "Failed to set HT capabilities");
			goto out;
		}
		wpa_printf(MSG_INFO, "Set HT capabilities: %s", params->ht_capab);
	}

	if (params->type & WIFI_AP_CONFIG_PARAM_VHT_CAPAB) {
		if (!hostapd_cli_cmd_v("set vht_capab %s", params->vht_capab)) {
			ret = -EINVAL;
			wpa_printf(MSG_ERROR, "Failed to set VHT capabilities");
			goto out;
		}
		wpa_printf(MSG_INFO, "Set VHT capabilities: %s", params->vht_capab);
	}

out:
	k_mutex_unlock(&hostapd_mutex);
	return ret;
}

int hostapd_ap_status(const struct device *dev, struct wifi_iface_status *status)
{
	int ret = 0;
	struct hostapd_iface *iface;
	struct hostapd_config *conf;
	struct hostapd_data *hapd;
	struct hostapd_bss_config *bss;
	struct hostapd_ssid *ssid;
	struct hostapd_hw_modes *hw_mode;
	int proto;    /* Wi-Fi secure protocol */
	int key_mgmt; /*  Wi-Fi key management */
	int sae_pwe;

	k_mutex_lock(&hostapd_mutex, K_FOREVER);

	iface = get_hostapd_handle(dev);
	if (!iface) {
		ret = -1;
		wpa_printf(MSG_ERROR, "Interface %s not found", dev->name);
		goto out;
	}

	conf = iface->conf;
	if (!conf) {
		ret = -1;
		wpa_printf(MSG_ERROR, "Conf %s not found", dev->name);
		goto out;
	}

	bss = conf->bss[0];
	if (!bss) {
		ret = -1;
		wpa_printf(MSG_ERROR, "Bss_conf %s not found", dev->name);
		goto out;
	}

	hapd = iface->bss[0];
	if (!hapd) {
		ret = -1;
		wpa_printf(MSG_ERROR, "Bss %s not found", dev->name);
		goto out;
	}

	status->state = iface->state;
	ssid = &bss->ssid;

	os_memcpy(status->bssid, hapd->own_addr, WIFI_MAC_ADDR_LEN);
	status->iface_mode = WIFI_MODE_AP;
	status->band = wpas_band_to_zephyr(wpas_freq_to_band(iface->freq));
	key_mgmt = bss->wpa_key_mgmt;
	proto = bss->wpa;
	sae_pwe = bss->sae_pwe;
	status->wpa3_ent_type = wpas_key_mgmt_to_zephyr_wpa3_ent(key_mgmt);
	status->security = wpas_key_mgmt_to_zephyr(1, hapd->conf, key_mgmt, proto, sae_pwe);
	status->mfp = get_mfp(bss->ieee80211w);
	status->channel = conf->channel;
	os_memcpy(status->ssid, ssid->ssid, ssid->ssid_len);

	status->dtim_period = bss->dtim_period;
	status->beacon_interval = conf->beacon_int;

	hw_mode = iface->current_mode;

	status->link_mode = conf->ieee80211ax                          ? WIFI_6
			    : conf->ieee80211ac                        ? WIFI_5
			    : conf->ieee80211n                         ? WIFI_4
			    : hw_mode->mode == HOSTAPD_MODE_IEEE80211G ? WIFI_3
			    : hw_mode->mode == HOSTAPD_MODE_IEEE80211A ? WIFI_2
			    : hw_mode->mode == HOSTAPD_MODE_IEEE80211B ? WIFI_1
								       : WIFI_0;
	status->twt_capable = (hw_mode->he_capab[IEEE80211_MODE_AP].mac_cap[0] & 0x04);

out:
	k_mutex_unlock(&hostapd_mutex);
	return ret;
}

#ifdef CONFIG_WIFI_NM_HOSTAPD_WPS
static int hapd_ap_wps_pbc(const struct device *dev)
{
	struct hostapd_iface *iface;
	int ret = -1;

	k_mutex_lock(&hostapd_mutex, K_FOREVER);

	iface = get_hostapd_handle(dev);
	if (!iface) {
		ret = -1;
		wpa_printf(MSG_ERROR, "Interface %s not found", dev->name);
		goto out;
	}

	if (iface->state != HAPD_IFACE_ENABLED) {
		ret = -EBUSY;
		wpa_printf(MSG_ERROR, "Interface %s is not in enable state", dev->name);
		goto out;
	}

	if (!hostapd_cli_cmd_v("wps_pbc")) {
		goto out;
	}

	ret = 0;

out:
	k_mutex_unlock(&hostapd_mutex);
	return ret;
}

static int hapd_ap_wps_pin(const struct device *dev, struct wifi_wps_config_params *params)
{
#define WPS_PIN_EXPIRE_TIME 120
	struct hostapd_iface *iface;
	char *get_pin_cmd = "WPS_AP_PIN random";
	int ret  = 0;

	k_mutex_lock(&hostapd_mutex, K_FOREVER);

	iface = get_hostapd_handle(dev);
	if (!iface) {
		ret = -1;
		wpa_printf(MSG_ERROR, "Interface %s not found", dev->name);
		goto out;
	}

	if (iface->state != HAPD_IFACE_ENABLED) {
		ret = -EBUSY;
		wpa_printf(MSG_ERROR, "Interface %s is not in enable state", dev->name);
		goto out;
	}

	if (params->oper == WIFI_WPS_PIN_GET) {
		if (zephyr_hostapd_cli_cmd_resp(get_pin_cmd, params->pin)) {
			goto out;
		}
	} else if (params->oper == WIFI_WPS_PIN_SET) {
		if (!hostapd_cli_cmd_v("wps_check_pin %s", params->pin)) {
			goto out;
		}

		if (!hostapd_cli_cmd_v("wps_pin any %s %d", params->pin, WPS_PIN_EXPIRE_TIME)) {
			goto out;
		}
	} else {
		wpa_printf(MSG_ERROR, "Error wps pin operation : %d", params->oper);
		goto out;
	}

	ret = 0;

out:
	k_mutex_unlock(&hostapd_mutex);
	return ret;
}

int hostapd_ap_wps_config(const struct device *dev, struct wifi_wps_config_params *params)
{
	int ret = 0;

	if (params->oper == WIFI_WPS_PBC) {
		ret = hapd_ap_wps_pbc(dev);
	} else if (params->oper == WIFI_WPS_PIN_GET || params->oper == WIFI_WPS_PIN_SET) {
		ret = hapd_ap_wps_pin(dev, params);
	}

	return ret;
}
#endif

int hostapd_ap_enable(const struct device *dev,
		      struct wifi_connect_req_params *params)
{
	struct hostapd_iface *iface;
	struct hostapd_data *hapd;
	struct wpa_driver_capa capa;
	int ret;

	if (!net_if_is_admin_up(net_if_lookup_by_dev(dev))) {
		wpa_printf(MSG_ERROR,
			   "Interface %s is down, dropping connect",
			   dev->name);
		return -1;
	}

	ret = set_ap_bandwidth(dev, params->bandwidth);
	if (ret && (ret != -ENOTSUP)) {
		wpa_printf(MSG_ERROR, "Failed to set ap bandwidth");
		return -EINVAL;
	}

	k_mutex_lock(&hostapd_mutex, K_FOREVER);

	iface = get_hostapd_handle(dev);
	if (!iface) {
		ret = -1;
		wpa_printf(MSG_ERROR, "Interface %s not found", dev->name);
		goto out;
	}

	iface->owner = iface;

	if (iface->state == HAPD_IFACE_ENABLED) {
		ret = -EBUSY;
		wpa_printf(MSG_ERROR, "Interface %s is not in disable state", dev->name);
		goto out;
	}

	ret = hapd_config_network(iface, params);
	if (ret) {
		wpa_printf(MSG_ERROR, "Failed to configure network for AP: %d", ret);
		goto out;
	}

	hapd = iface->bss[0];
	if (!iface->extended_capa || !iface->extended_capa_mask) {
		if (hapd->driver->get_capa && hapd->driver->get_capa(hapd->drv_priv, &capa) == 0) {
			iface->extended_capa         = capa.extended_capa;
			iface->extended_capa_mask    = capa.extended_capa_mask;
			iface->extended_capa_len     = capa.extended_capa_len;
			iface->drv_max_acl_mac_addrs = capa.max_acl_mac_addrs;

			/*
			 * Override extended capa with per-interface type (AP), if
			 * available from the driver.
			 */
			hostapd_get_ext_capa(iface);
		} else {
			ret = -1;
			wpa_printf(MSG_ERROR, "Failed to get capability for AP: %d", ret);
			goto out;
		}
	}

	if (!hostapd_cli_cmd_v("enable")) {
		goto out;
	}
out:
	k_mutex_unlock(&hostapd_mutex);
	return ret;
}

int hostapd_ap_disable(const struct device *dev)
{
	struct hostapd_iface *iface;
	int ret = 0;

	k_mutex_lock(&hostapd_mutex, K_FOREVER);

	iface = get_hostapd_handle(dev);
	if (!iface) {
		ret = -ENOENT;
		wpa_printf(MSG_ERROR, "Interface %s not found", dev->name);
		goto out;
	}

	if (iface->state != HAPD_IFACE_ENABLED) {
		ret = -EBUSY;
		wpa_printf(MSG_ERROR, "Interface %s is not in enable state", dev->name);
		goto out;
	}

	if (!hostapd_cli_cmd_v("disable")) {
		goto out;
	}

	iface->freq = 0;

out:
	k_mutex_unlock(&hostapd_mutex);
	return ret;
}

int hostapd_ap_sta_disconnect(const struct device *dev,
			      const uint8_t *mac_addr)
{
	struct hostapd_iface *iface;
	int ret  = 0;

	k_mutex_lock(&hostapd_mutex, K_FOREVER);

	iface = get_hostapd_handle(dev);
	if (!iface) {
		ret = -1;
		wpa_printf(MSG_ERROR, "Interface %s not found", dev->name);
		goto out;
	}

	if (iface->state != HAPD_IFACE_ENABLED) {
		ret = -EBUSY;
		wpa_printf(MSG_ERROR, "Interface %s is not in enable state", dev->name);
		goto out;
	}

	if (!mac_addr) {
		ret = -EINVAL;
		wpa_printf(MSG_ERROR, "Invalid MAC address");
		goto out;
	}

	if (!hostapd_cli_cmd_v("deauthenticate %02x:%02x:%02x:%02x:%02x:%02x",
				mac_addr[0], mac_addr[1], mac_addr[2],
				mac_addr[3], mac_addr[4], mac_addr[5])) {
		goto out;
	}

out:
	k_mutex_unlock(&hostapd_mutex);
	return ret;
}

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_DPP
int hostapd_dpp_dispatch(const struct device *dev, struct wifi_dpp_params *params)
{
	int ret;
	char *cmd = NULL;

	if (params == NULL) {
		return -EINVAL;
	}

	cmd = os_zalloc(SUPPLICANT_DPP_CMD_BUF_SIZE);
	if (cmd == NULL) {
		return -ENOMEM;
	}

	/* leave one byte always be 0 */
	ret = dpp_params_to_cmd(params, cmd, SUPPLICANT_DPP_CMD_BUF_SIZE - 2);
	if (ret) {
		os_free(cmd);
		return ret;
	}

	wpa_printf(MSG_DEBUG, "hostapd_cli %s", cmd);
	if (zephyr_hostapd_cli_cmd_resp(cmd, params->resp)) {
		os_free(cmd);
		return -ENOEXEC;
	}

	os_free(cmd);
	return 0;
}
#endif /* CONFIG_WIFI_NM_WPA_SUPPLICANT_DPP */
