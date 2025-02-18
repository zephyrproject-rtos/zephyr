/**
 * Copyright 2023-2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/wifi_nm.h>
#include "includes.h"
#include "common.h"
#include "eloop.h"
#include "wpa_supplicant_i.h"
#include "hostapd.h"
#include "hostapd_cli_zephyr.h"
#include "eap_register.h"
#include "ap_drv_ops.h"
#include "l2_packet/l2_packet.h"
#include "supp_main.h"
#include "hapd_main.h"
#include "supp_api.h"
#include "hapd_api.h"
#include "hapd_events.h"

static const struct wifi_mgmt_ops mgmt_ap_ops = {
	.ap_enable = hostapd_ap_enable,
	.ap_disable = hostapd_ap_disable,
	.ap_sta_disconnect = hostapd_ap_sta_disconnect,
	.iface_status = hostapd_ap_status,
#ifdef CONFIG_WIFI_NM_HOSTAPD_WPS
	.wps_config = hostapd_ap_wps_config,
#endif
#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_DPP
	.dpp_dispatch = hostapd_dpp_dispatch,
#endif /* CONFIG_WIFI_NM_WPA_SUPPLICANT_DPP */
	.ap_config_params = hostapd_ap_config_params,
	.set_rts_threshold = supplicant_set_rts_threshold,
#ifdef CONFIG_WIFI_NM_HOSTAPD_CRYPTO_ENTERPRISE
	.enterprise_creds = hostapd_add_enterprise_creds,
#endif
	.set_btwt = supplicant_set_btwt,
};

DEFINE_WIFI_NM_INSTANCE(hostapd, &mgmt_ap_ops);

struct hapd_global {
	void **drv_priv;
	size_t drv_count;
};

static struct hapd_global hglobal;

#ifndef HOSTAPD_CLEANUP_INTERVAL
#define HOSTAPD_CLEANUP_INTERVAL 10
#endif /* HOSTAPD_CLEANUP_INTERVAL */

static int hostapd_periodic_call(struct hostapd_iface *iface, void *ctx)
{
	hostapd_periodic_iface(iface);
	return 0;
}

/* Periodic cleanup tasks */
static void hostapd_periodic(void *eloop_ctx, void *timeout_ctx)
{
	struct hapd_interfaces *interfaces = eloop_ctx;

	eloop_register_timeout(HOSTAPD_CLEANUP_INTERVAL, 0,
			       hostapd_periodic, interfaces, NULL);
	hostapd_for_each_interface(interfaces, hostapd_periodic_call, NULL);
}

struct hostapd_iface *zephyr_get_hapd_handle_by_ifname(const char *ifname)
{
	struct hapd_interfaces *interfaces = zephyr_get_default_hapd_context();
	struct hostapd_data *hapd = NULL;

	hapd = hostapd_get_iface(interfaces, ifname);
	if (!hapd) {
		wpa_printf(MSG_ERROR, "%s: Unable to get hapd handle for %s\n", __func__, ifname);
		return NULL;
	}

	return hapd->iface;
}

static void hostapd_event_eapol_rx_cb(void *ctx, const u8 *src_addr,
				      const u8 *buf, size_t len)
{
	hostapd_event_eapol_rx(ctx, src_addr, buf, len, FRAME_ENCRYPTION_UNKNOWN, -1);
}

struct hostapd_iface *hostapd_get_interface(const char *ifname)
{
	struct hapd_interfaces *interfaces = zephyr_get_default_hapd_context();

	return interfaces->iface[0];
}

static int hostapd_enable_iface_cb(struct hostapd_iface *hapd_iface)
{
	struct hostapd_data *bss;

	wpa_printf(MSG_DEBUG, "Enable interface %s", hapd_iface->conf->bss[0]->iface);

	bss = hapd_iface->bss[0];

	bss->conf->start_disabled = 0;

	if (hostapd_config_check(hapd_iface->conf, 1) < 0) {
		wpa_printf(MSG_INFO, "Invalid configuration - cannot enable");
		return -1;
	}

	l2_packet_deinit(bss->l2);
	bss->l2 = l2_packet_init(bss->conf->iface, bss->conf->bssid, ETH_P_EAPOL,
				 &hostapd_event_eapol_rx_cb, bss, 0);
	if (bss->l2 == NULL) {
		wpa_printf(MSG_ERROR, "Failed to initialize l2 for hostapd interface");
		return -1;
	}

	if (hostapd_setup_interface(hapd_iface)) {
		wpa_printf(MSG_ERROR, "Failed to initialize hostapd interface");
		return -1;
	}

	return 0;
}

static int hostapd_disable_iface_cb(struct hostapd_iface *hapd_iface)
{
	size_t j;
	struct hostapd_data *hapd = NULL;

	wpa_msg(hapd_iface->bss[0]->msg_ctx, MSG_INFO, AP_EVENT_DISABLED);

	hapd_iface->driver_ap_teardown = !!(hapd_iface->drv_flags
					    & WPA_DRIVER_FLAGS_AP_TEARDOWN_SUPPORT);

#ifdef NEED_AP_MLME
	for (j = 0; j < hapd_iface->num_bss; j++) {
		hostapd_cleanup_cs_params(hapd_iface->bss[j]);
	}
#endif /* NEED_AP_MLME */

	/* Same as hostapd_interface_deinit() without deinitializing control
	 * interface
	 */
	for (j = 0; j < hapd_iface->num_bss; j++) {
		hapd = hapd_iface->bss[j];
		hostapd_bss_deinit_no_free(hapd);
		hostapd_free_hapd_data(hapd);
	}

	hostapd_drv_stop_ap(hapd);

	hostapd_cleanup_iface_partial(hapd_iface);

	wpa_printf(MSG_DEBUG, "Interface %s disabled", hapd_iface->bss[0]->conf->iface);
	hostapd_set_state(hapd_iface, HAPD_IFACE_DISABLED);
	hostapd_send_wifi_mgmt_ap_status(hapd_iface,
					 NET_EVENT_WIFI_CMD_AP_DISABLE_RESULT,
					 WIFI_STATUS_AP_SUCCESS);
	hostapd_config_free(hapd_iface->conf);
	hapd_iface->conf = hapd_iface->interfaces->config_read_cb(hapd_iface->config_fname);
	for (j = 0; j < hapd_iface->num_bss; j++) {
		hapd = hapd_iface->bss[j];
		hapd->iconf = hapd_iface->conf;
		hapd->conf = hapd_iface->conf->bss[j];
		hapd->driver = hapd_iface->conf->driver;
	}

	return 0;
}

static int hostapd_global_init(struct hapd_interfaces *interfaces, const char *entropy_file)
{
	int i;

	os_memset(&hglobal, 0, sizeof(struct hapd_global));

	if (eap_server_register_methods()) {
		wpa_printf(MSG_ERROR, "Failed to register EAP methods");
		return -1;
	}

	interfaces->eloop_initialized = 1;

	for (i = 0; wpa_drivers[i]; i++) {
		hglobal.drv_count++;
	}

	if (hglobal.drv_count == 0) {
		wpa_printf(MSG_ERROR, "No drivers enabled");
		return -1;
	}
	hglobal.drv_priv = os_calloc(hglobal.drv_count, sizeof(void *));
	if (hglobal.drv_priv == NULL) {
		return -1;
	}

	return 0;
}

const char *zephyr_hostap_msg_ifname_cb(void *ctx)
{
	if (ctx == NULL) {
		return NULL;
	}

	if ((*((int *)ctx)) == 0) {
		struct wpa_supplicant *wpa_s = ctx;

		return wpa_s->ifname;
	}

	struct hostapd_data *hapd = ctx;

	if (hapd && hapd->conf) {
		return hapd->conf->iface;
	}

	return NULL;
}

void zephyr_hostap_ctrl_iface_msg_cb(void *ctx, int level, enum wpa_msg_type type,
				     const char *txt, size_t len)
{
	if (ctx == NULL) {
		return;
	}

	if ((*((int *)ctx)) == 0) {
		wpa_supplicant_msg_send(ctx, level, type, txt, len);
	} else {
		hostapd_msg_send(ctx, level, type, txt, len);
	}
}

static int hostapd_driver_init(struct hostapd_iface *iface)
{
	struct wpa_init_params params;
	size_t i;
	struct hostapd_data *hapd       = iface->bss[0];
	struct hostapd_bss_config *conf = hapd->conf;
	u8 *b                           = conf->bssid;
	struct wpa_driver_capa capa;

	if (hapd->driver == NULL || hapd->driver->hapd_init == NULL) {
		wpa_printf(MSG_ERROR, "No hostapd driver wrapper available");
		return -1;
	}

	/* Initialize the driver interface */
	if (!(b[0] | b[1] | b[2] | b[3] | b[4] | b[5])) {
		b = NULL;
	}

	os_memset(&params, 0, sizeof(params));
	for (i = 0; wpa_drivers[i]; i++) {
		if (wpa_drivers[i] != hapd->driver) {
			continue;
		}

		if (hglobal.drv_priv[i] == NULL && wpa_drivers[i]->global_init) {
			hglobal.drv_priv[i] = wpa_drivers[i]->global_init(iface->interfaces);
			if (hglobal.drv_priv[i] == NULL) {
				wpa_printf(MSG_ERROR, "Failed to initialize driver '%s'",
					   wpa_drivers[i]->name);
				return -1;
			}
			hglobal.drv_count++;
		}

		params.global_priv = hglobal.drv_priv[i];
		break;
	}
	params.bssid              = b;
	params.ifname             = hapd->conf->iface;
	params.driver_params      = hapd->iconf->driver_params;
	params.use_pae_group_addr = hapd->conf->use_pae_group_addr;
	params.num_bridge         = hapd->iface->num_bss;
	params.bridge             = os_calloc(hapd->iface->num_bss, sizeof(char *));
	if (params.bridge == NULL) {
		return -1;
	}
	for (i = 0; i < hapd->iface->num_bss; i++) {
		struct hostapd_data *bss = hapd->iface->bss[i];

		if (bss->conf->bridge[0]) {
			params.bridge[i] = bss->conf->bridge;
		}
	}

	params.own_addr = hapd->own_addr;

	hapd->drv_priv = hapd->driver->hapd_init(hapd, &params);
	os_free(params.bridge);
	if (hapd->drv_priv == NULL) {
		wpa_printf(MSG_ERROR, "%s driver initialization failed.",
			   hapd->driver->name);
		hapd->driver = NULL;
		return -1;
	}

	if (hapd->driver->get_capa && hapd->driver->get_capa(hapd->drv_priv, &capa) == 0) {
		struct wowlan_triggers *triggs;

		iface->drv_flags           = capa.flags;
		iface->drv_flags2          = capa.flags2;
		iface->probe_resp_offloads = capa.probe_resp_offloads;
		/*
		 * Use default extended capa values from per-radio information
		 */
		iface->extended_capa         = capa.extended_capa;
		iface->extended_capa_mask    = capa.extended_capa_mask;
		iface->extended_capa_len     = capa.extended_capa_len;
		iface->drv_max_acl_mac_addrs = capa.max_acl_mac_addrs;

		/*
		 * Override extended capa with per-interface type (AP), if
		 * available from the driver.
		 */
		hostapd_get_ext_capa(iface);

		triggs = wpa_get_wowlan_triggers(conf->wowlan_triggers, &capa);
		if (triggs && hapd->driver->set_wowlan) {
			if (hapd->driver->set_wowlan(hapd->drv_priv, triggs)) {
				wpa_printf(MSG_ERROR, "set_wowlan failed");
			}
		}
		os_free(triggs);
	}

	return 0;
}

struct hostapd_config *hostapd_config_read2(const char *fname)
{
	struct hostapd_config *conf;
	const struct device *dev;
	char ifname[IFNAMSIZ + 1] = {0};
	int errors = 0;
	size_t i;
	int aCWmin = 4, aCWmax = 10;
	/* background traffic */
	struct hostapd_wmm_ac_params ac_bk = {aCWmin, aCWmax, 9, 0, 0};
	/* best effort traffic */
	struct hostapd_wmm_ac_params ac_be = {aCWmin, aCWmax - 4, 5, 0, 0};
	/* video traffic */
	struct hostapd_wmm_ac_params ac_vi = {aCWmin - 1, aCWmin, 3,
					      3008 / 32, 0};
	/* voice traffic */
	struct hostapd_wmm_ac_params ac_vo = {aCWmin - 2, aCWmin - 1, 3,
					      1504 / 32, 0};

	dev = net_if_get_device(net_if_get_wifi_sap());
	strncpy(ifname, dev->name, IFNAMSIZ);
	ifname[IFNAMSIZ] = '\0';

	conf = hostapd_config_defaults();
	if (conf == NULL) {
		return NULL;
	}

	conf->wmm_ac_params[0] = ac_be;
	conf->wmm_ac_params[1] = ac_bk;
	conf->wmm_ac_params[2] = ac_vi;
	conf->wmm_ac_params[3] = ac_vo;

	/* set default driver based on configuration */
	conf->driver = wpa_drivers[0];
	if (conf->driver == NULL) {
		wpa_printf(MSG_ERROR, "No driver wrappers registered!");
		hostapd_config_free(conf);
		return NULL;
	}
	conf->last_bss = conf->bss[0];
	struct hostapd_bss_config *bss;

	bss                 = conf->last_bss;
	bss->start_disabled = 1;
	bss->max_num_sta    = CONFIG_WIFI_MGMT_AP_MAX_NUM_STA;
	bss->dtim_period    = 1;
	os_strlcpy(conf->bss[0]->iface, ifname, sizeof(conf->bss[0]->iface));
	bss->logger_stdout_level = HOSTAPD_LEVEL_INFO;
	bss->logger_stdout       = 0xffff;
	bss->nas_identifier      = os_strdup("ap.example.com");
	os_memcpy(conf->country, "US ", 3);
	conf->hw_mode        = HOSTAPD_MODE_IEEE80211G;
	bss->wps_state       = WPS_STATE_CONFIGURED;
	bss->eap_server      = 1;
#ifdef CONFIG_WPS
	bss->ap_setup_locked = 1;
#endif
	conf->channel        = 1;
	conf->acs            = conf->channel == 0;
#ifdef CONFIG_ACS
	conf->acs_num_scans  = 1;
#endif
	conf->ieee80211n     = 1;
	conf->ieee80211h     = 0;
	conf->ieee80211d     = 1;
	conf->acs_exclude_dfs = 1;
	conf->ht_capab |= HT_CAP_INFO_SHORT_GI20MHZ;
	bss->auth_algs = 1;
	bss->okc = 1;
	conf->no_pri_sec_switch = 1;
	conf->ht_op_mode_fixed  = 1;
	conf->ieee80211ac       = 1;
	conf->vht_oper_chwidth  = CHANWIDTH_USE_HT;
	conf->vht_capab |= VHT_CAP_MAX_A_MPDU_LENGTH_EXPONENT_MAX;
#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_11AX
	conf->ieee80211ax       = 1;
	conf->he_oper_chwidth   = CHANWIDTH_USE_HT;
	conf->he_phy_capab.he_su_beamformer = 0;
	conf->he_phy_capab.he_su_beamformee = 1;
	conf->he_phy_capab.he_mu_beamformer = 0;
	conf->he_op.he_bss_color = 1;
	conf->he_op.he_default_pe_duration  = 0;
	/* Set default basic MCS/NSS set to single stream MCS 0-7 */
	conf->he_op.he_basic_mcs_nss_set    = 0xfffc;
#endif

	for (i = 0; i < conf->num_bss; i++) {
		hostapd_set_security_params(conf->bss[i], 1);
	}

	if (hostapd_config_check(conf, 1)) {
		errors++;
	}

#ifndef WPA_IGNORE_CONFIG_ERRORS
	if (errors) {
		wpa_printf(MSG_ERROR, "%d errors found in configuration file '%s'",
			   errors, fname);
		hostapd_config_free(conf);
		conf = NULL;
	}
#endif /* WPA_IGNORE_CONFIG_ERRORS */
	return conf;
}

static struct hostapd_iface *hostapd_interface_init(struct hapd_interfaces *interfaces,
						    const char *if_name,
						    const char *config_fname,
						    int debug)
{
	struct hostapd_iface *iface;
	int k;

	wpa_printf(MSG_DEBUG, "Configuration file: %s", config_fname);
	iface = hostapd_init(interfaces, config_fname);
	if (!iface) {
		return NULL;
	}

	if (if_name) {
		os_strlcpy(iface->conf->bss[0]->iface, if_name,
			   sizeof(iface->conf->bss[0]->iface));
	}

	iface->interfaces = interfaces;

	for (k = 0; k < debug; k++) {
		if (iface->bss[0]->conf->logger_stdout_level > 0) {
			iface->bss[0]->conf->logger_stdout_level--;
		}
	}

	if (iface->conf->bss[0]->iface[0] == '\0' &&
	    !hostapd_drv_none(iface->bss[0])) {
		wpa_printf(MSG_ERROR,
			   "Interface name not specified in %s, nor by '-i' parameter",
			   config_fname);
		hostapd_interface_deinit_free(iface);
		return NULL;
	}

	iface->bss[0]->is_hostapd = 1;

	return iface;
}

void zephyr_hostapd_init(struct hapd_interfaces *interfaces)
{
	size_t i;
	int ret, debug = 0;
	struct net_if *iface;
	char ifname[IFNAMSIZ + 1] = { 0 };
	const char *entropy_file = NULL;
	size_t num_bss_configs = 0;
	int start_ifaces_in_sync = 0;
#ifdef CONFIG_DPP
	struct dpp_global_config dpp_conf;
#endif /* CONFIG_DPP */

	os_memset(interfaces, 0, sizeof(struct hapd_interfaces));
	interfaces->reload_config      = hostapd_reload_config;
	interfaces->config_read_cb     = hostapd_config_read2;
	interfaces->for_each_interface = hostapd_for_each_interface;
	interfaces->driver_init        = hostapd_driver_init;
	interfaces->global_ctrl_sock   = -1;
	dl_list_init(&interfaces->global_ctrl_dst);
#ifdef CONFIG_DPP
	os_memset(&dpp_conf, 0, sizeof(dpp_conf));
	dpp_conf.cb_ctx = interfaces;
	interfaces->dpp = dpp_global_init(&dpp_conf);
	if (!interfaces->dpp) {
		return;
	}
#endif /* CONFIG_DPP */

	interfaces->count = 1;
	if (interfaces->count || num_bss_configs) {
		interfaces->iface = os_calloc(interfaces->count + num_bss_configs,
					      sizeof(struct hostapd_iface *));
		if (interfaces->iface == NULL) {
			wpa_printf(MSG_ERROR, "malloc failed");
			return;
		}
	}

	if (hostapd_global_init(interfaces, entropy_file)) {
		wpa_printf(MSG_ERROR, "Failed to initialize global context");
		return;
	}

	eloop_register_timeout(HOSTAPD_CLEANUP_INTERVAL, 0,
			       hostapd_periodic, interfaces, NULL);

	iface = net_if_get_wifi_sap();
	ret = net_if_get_name(iface, ifname, sizeof(ifname) - 1);
	if (ret < 0) {
		wpa_printf(MSG_ERROR, "Cannot get interface %d (%p) name",
			   net_if_get_by_iface(iface), iface);
		goto out;
	}

	for (i = 0; i < interfaces->count; i++) {
		interfaces->iface[i] = hostapd_interface_init(interfaces, ifname,
							      "hostapd.conf", debug);
		if (!interfaces->iface[i]) {
			wpa_printf(MSG_ERROR, "Failed to initialize interface");
			goto out;
		}
		if (start_ifaces_in_sync) {
			interfaces->iface[i]->need_to_start_in_sync = 0;
		}
	}

	/*
	 * Enable configured interfaces. Depending on channel configuration,
	 * this may complete full initialization before returning or use a
	 * callback mechanism to complete setup in case of operations like HT
	 * co-ex scans, ACS, or DFS are needed to determine channel parameters.
	 * In such case, the interface will be enabled from eloop context within
	 * hostapd_global_run().
	 */
	interfaces->terminate_on_error = 0;
	for (i = 0; i < interfaces->count; i++) {
		if (hostapd_driver_init(interfaces->iface[i])) {
			goto out;
		}
		interfaces->iface[i]->enable_iface_cb  = hostapd_enable_iface_cb;
		interfaces->iface[i]->disable_iface_cb = hostapd_disable_iface_cb;
		zephyr_hostapd_ctrl_init((void *)interfaces->iface[i]->bss[0]);
	}

out:
	return;
}
