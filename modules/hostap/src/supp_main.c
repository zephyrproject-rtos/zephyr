/*
 * Copyright (c) 2023 Nordic Semiconductor ASA.
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wifi_supplicant, CONFIG_WIFI_NM_WPA_SUPPLICANT_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <poll.h>

#if !defined(CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_NONE) && !defined(CONFIG_MBEDTLS_ENABLE_HEAP)
#include <mbedtls/platform.h>
#endif /* !CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_NONE && !CONFIG_MBEDTLS_ENABLE_HEAP */
#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_MBEDTLS_PSA
#include "supp_psa_api.h"
#endif

#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/wifi_nm.h>
#include <zephyr/net/socket.h>

static K_THREAD_STACK_DEFINE(supplicant_thread_stack,
			     CONFIG_WIFI_NM_WPA_SUPPLICANT_THREAD_STACK_SIZE);
static struct k_thread tid;

static K_THREAD_STACK_DEFINE(iface_wq_stack, CONFIG_WIFI_NM_WPA_SUPPLICANT_WQ_STACK_SIZE);

#define IFACE_NOTIFY_TIMEOUT_MS 1000
#define IFACE_NOTIFY_RETRY_MS 10

#include "supp_main.h"
#include "supp_api.h"
#include "supp_events.h"

#include "includes.h"
#include "common.h"
#include "eloop.h"
#include "wpa_supplicant/config.h"
#include "wpa_supplicant_i.h"
#include "fst/fst.h"
#include "includes.h"
#include "wpa_cli_zephyr.h"
#ifdef CONFIG_WIFI_NM_HOSTAPD_AP
#include "hostapd.h"
#include "hostapd_cli_zephyr.h"
#include "eap_register.h"
#include "ap_drv_ops.h"
#include "l2_packet/l2_packet.h"
#endif

static const struct wifi_mgmt_ops mgmt_ops = {
	.get_version = supplicant_get_version,
	.scan = supplicant_scan,
	.connect = supplicant_connect,
	.disconnect = supplicant_disconnect,
	.iface_status = supplicant_status,
#ifdef CONFIG_NET_STATISTICS_WIFI
	.get_stats = supplicant_get_stats,
	.reset_stats = supplicant_reset_stats,
#endif
	.set_11k_enable = supplicant_11k_enable,
	.send_11k_neighbor_request = supplicant_11k_neighbor_request,
#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_ROAMING
	.spec_scan = supplicant_spec_scan,
	.start_11r_roaming = suppliant_11r_roaming,
#endif
	.set_power_save = supplicant_set_power_save,
	.set_twt = supplicant_set_twt,
	.set_btwt = supplicant_set_btwt,
	.set_rts_threshold = supplicant_set_rts_threshold,
	.get_power_save_config = supplicant_get_power_save_config,
	.reg_domain = supplicant_reg_domain,
	.mode = supplicant_mode,
	.filter = supplicant_filter,
	.channel = supplicant_channel,
	.set_rts_threshold = supplicant_set_rts_threshold,
	.get_rts_threshold = supplicant_get_rts_threshold,
	.bss_ext_capab = supplicant_bss_ext_capab,
	.legacy_roam = supplicant_legacy_roam,
#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_WNM
	.btm_query = supplicant_btm_query,
#endif
	.get_conn_params = supplicant_get_wifi_conn_params,
	.wps_config = supplicant_wps_config,
#ifdef CONFIG_AP
	.ap_enable = supplicant_ap_enable,
	.ap_disable = supplicant_ap_disable,
	.ap_sta_disconnect = supplicant_ap_sta_disconnect,
#endif /* CONFIG_AP */
#ifdef CONFIG_WIFI_NM_HOSTAPD_AP
	.ap_bandwidth = supplicant_ap_bandwidth,
#endif
#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_DPP
	.dpp_dispatch = supplicant_dpp_dispatch,
#endif /* CONFIG_WIFI_NM_WPA_SUPPLICANT_DPP */
	.pmksa_flush = supplicant_pmksa_flush,
#if defined CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_ENTERPRISE || \
	defined CONFIG_WIFI_NM_HOSTAPD_CRYPTO_ENTERPRISE
	.enterprise_creds = supplicant_add_enterprise_creds,
#endif
};

DEFINE_WIFI_NM_INSTANCE(wifi_supplicant, &mgmt_ops);

#ifdef CONFIG_WIFI_NM_HOSTAPD_AP
static const struct wifi_mgmt_ops mgmt_ap_ops = {
	.set_btwt = supplicant_set_btwt,
	.ap_enable = supplicant_ap_enable,
	.ap_disable = supplicant_ap_disable,
	.ap_sta_disconnect = supplicant_ap_sta_disconnect,
	.ap_bandwidth = supplicant_ap_bandwidth,
	.iface_status = supplicant_ap_status,
#ifdef CONFIG_WIFI_NM_HOSTAPD_WPS
	.ap_wps_config = supplicant_ap_wps_config,
#endif
#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_DPP
	.dpp_dispatch = hapd_dpp_dispatch,
#endif /* CONFIG_WIFI_NM_WPA_SUPPLICANT_DPP */
	.ap_config_params = supplicant_ap_config_params,
	.set_rts_threshold = supplicant_set_rts_threshold,
};

DEFINE_WIFI_NM_INSTANCE(hostapd, &mgmt_ap_ops);
#endif

#define WRITE_TIMEOUT 100 /* ms */
#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_INF_MON
#define INTERFACE_EVENT_MASK (NET_EVENT_IF_ADMIN_UP | NET_EVENT_IF_ADMIN_DOWN)
#endif
struct supplicant_context {
	struct wpa_global *supplicant;
#ifdef CONFIG_WIFI_NM_HOSTAPD_AP
	struct hapd_interfaces hostapd;
#endif
	struct net_mgmt_event_callback cb;
	struct net_if *iface;
	char if_name[CONFIG_NET_INTERFACE_NAME_LEN + 1];
	int event_socketpair[2];
	struct k_work iface_work;
	struct k_work_q iface_wq;
	int (*iface_handler)(struct supplicant_context *ctx, struct net_if *iface);
};

#ifdef CONFIG_WIFI_NM_HOSTAPD_AP
struct hapd_global {
	void **drv_priv;
	size_t drv_count;
};

static struct hapd_global hglobal;

#ifndef HOSTAPD_CLEANUP_INTERVAL
#define HOSTAPD_CLEANUP_INTERVAL 10
#endif /* HOSTAPD_CLEANUP_INTERVAL */

static void zephyr_hostap_ctrl_iface_msg_cb(void *ctx, int level, enum wpa_msg_type type,
					    const char *txt, size_t len);

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
#endif

static struct supplicant_context *get_default_context(void)
{
	static struct supplicant_context ctx;

	return &ctx;
}

struct wpa_global *zephyr_get_default_supplicant_context(void)
{
	return get_default_context()->supplicant;
}

struct k_work_q *get_workq(void)
{
	return &get_default_context()->iface_wq;
}

int zephyr_wifi_send_event(const struct wpa_supplicant_event_msg *msg)
{
	struct supplicant_context *ctx;
	struct pollfd fds[1];
	int ret;

	/* TODO: Fix this to get the correct container */
	ctx = get_default_context();

	if (ctx->event_socketpair[1] < 0) {
		ret = -ENOENT;
		goto out;
	}

	fds[0].fd = ctx->event_socketpair[0];
	fds[0].events = POLLOUT;
	fds[0].revents = 0;

	ret = zsock_poll(fds, 1, WRITE_TIMEOUT);
	if (ret < 0) {
		ret = -errno;
		LOG_ERR("Cannot write event (%d)", ret);
		goto out;
	}

	ret = zsock_send(ctx->event_socketpair[1], msg, sizeof(*msg), 0);
	if (ret < 0) {
		ret = -errno;
		LOG_WRN("Event send failed (%d)", ret);
		goto out;
	}

	if (ret != sizeof(*msg)) {
		ret = -EMSGSIZE;
		LOG_WRN("Event partial send (%d)", ret);
		goto out;
	}

	ret = 0;

out:
	return ret;
}

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_INF_MON
static int send_event(const struct wpa_supplicant_event_msg *msg)
{
	return zephyr_wifi_send_event(msg);
}

static bool is_wanted_interface(struct net_if *iface)
{
	if (!net_if_is_wifi(iface)) {
		return false;
	}

	/* TODO: check against a list of valid interfaces */

	return true;
}
#endif
struct wpa_supplicant *zephyr_get_handle_by_ifname(const char *ifname)
{
	struct wpa_supplicant *wpa_s = NULL;
	struct supplicant_context *ctx = get_default_context();

	wpa_s = wpa_supplicant_get_iface(ctx->supplicant, ifname);
	if (!wpa_s) {
		wpa_printf(MSG_ERROR, "%s: Unable to get wpa_s handle for %s\n", __func__, ifname);
		return NULL;
	}

	return wpa_s;
}

#ifdef CONFIG_WIFI_NM_HOSTAPD_AP
struct hostapd_iface *zephyr_get_hapd_handle_by_ifname(const char *ifname)
{
	struct hostapd_data *hapd = NULL;
	struct supplicant_context *ctx = get_default_context();

	hapd = hostapd_get_iface(&ctx->hostapd, ifname);
	if (!hapd) {
		wpa_printf(MSG_ERROR, "%s: Unable to get hapd handle for %s\n", __func__, ifname);
		return NULL;
	}

	return hapd->iface;
}
#endif

static int get_iface_count(struct supplicant_context *ctx)
{
	/* FIXME, should not access ifaces as it is supplicant internal data */
	struct wpa_supplicant *wpa_s;
	unsigned int count = 0;

	for (wpa_s = ctx->supplicant->ifaces; wpa_s; wpa_s = wpa_s->next) {
		count += 1;
	}

	return count;
}

static int add_interface(struct supplicant_context *ctx, struct net_if *iface)
{
	struct wpa_supplicant *wpa_s;
	char ifname[IFNAMSIZ + 1] = { 0 };
	int ret, retry = 0, count = IFACE_NOTIFY_TIMEOUT_MS / IFACE_NOTIFY_RETRY_MS;

	ret = net_if_get_name(iface, ifname, sizeof(ifname) - 1);
	if (ret < 0) {
		LOG_ERR("Cannot get interface %d (%p) name", net_if_get_by_iface(iface), iface);
		goto out;
	}

	LOG_DBG("Adding interface %s [%d] (%p)", ifname, net_if_get_by_iface(iface), iface);

	ret = zephyr_wpa_cli_global_cmd_v("interface_add %s %s %s %s",
					  ifname, "zephyr", "zephyr", "zephyr");
	if (ret) {
		LOG_ERR("Failed to add interface %s", ifname);
		goto out;
	}

	while (retry++ < count && !wpa_supplicant_get_iface(ctx->supplicant, ifname)) {
		k_sleep(K_MSEC(IFACE_NOTIFY_RETRY_MS));
	}

	wpa_s = wpa_supplicant_get_iface(ctx->supplicant, ifname);
	if (wpa_s == NULL) {
		LOG_ERR("Failed to add iface %s", ifname);
		goto out;
	}

	wpa_s->conf->filter_ssids = 1;
	wpa_s->conf->ap_scan = 1;

	/* Default interface, kick start supplicant */
	if (get_iface_count(ctx) > 0) {
		ctx->iface = iface;
		net_if_get_name(iface, ctx->if_name, CONFIG_NET_INTERFACE_NAME_LEN);
	}

	ret = zephyr_wpa_ctrl_init(wpa_s);
	if (ret) {
		LOG_ERR("Failed to initialize supplicant control interface");
		goto out;
	}

	ret = wifi_nm_register_mgd_type_iface(wifi_nm_get_instance("wifi_supplicant"),
					      WIFI_TYPE_STA,
					      iface);
	if (ret) {
		LOG_ERR("Failed to register mgd iface with native stack %s (%d)",
			ifname, ret);
		goto out;
	}

	supplicant_generate_state_event(ifname, NET_EVENT_SUPPLICANT_CMD_IFACE_ADDED, 0);

	if (get_iface_count(ctx) == 1) {
		supplicant_generate_state_event(ifname, NET_EVENT_SUPPLICANT_CMD_READY, 0);
	}

#ifdef CONFIG_WIFI_NM_HOSTAPD_AP
	wpa_msg_register_cb(zephyr_hostap_ctrl_iface_msg_cb);
#endif
	ret = 0;

out:
	return ret;
}
#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_INF_MON
static int del_interface(struct supplicant_context *ctx, struct net_if *iface)
{
	struct wpa_supplicant_event_msg msg;
	struct wpa_supplicant *wpa_s;
	union wpa_event_data *event = NULL;
	int ret, retry = 0, count = IFACE_NOTIFY_TIMEOUT_MS / IFACE_NOTIFY_RETRY_MS;
	char ifname[IFNAMSIZ + 1] = { 0 };

	ret = net_if_get_name(iface, ifname, sizeof(ifname) - 1);
	if (ret < 0) {
		LOG_ERR("Cannot get interface %d (%p) name", net_if_get_by_iface(iface), iface);
		goto out;
	}

	LOG_DBG("Removing interface %s %d (%p)", ifname, net_if_get_by_iface(iface), iface);

	event = os_zalloc(sizeof(*event));
	if (!event) {
		ret = -ENOMEM;
		LOG_ERR("Failed to allocate event data");
		goto out;
	}

	wpa_s = wpa_supplicant_get_iface(ctx->supplicant, ifname);
	if (!wpa_s) {
		ret = -ENOENT;
		LOG_ERR("Failed to get wpa_s handle for %s", ifname);
		goto free;
	}

	supplicant_generate_state_event(ifname, NET_EVENT_SUPPLICANT_CMD_IFACE_REMOVING, 0);

	if (sizeof(event->interface_status.ifname) < strlen(ifname)) {
		wpa_printf(MSG_ERROR, "Interface name too long: %s (max: %d)",
			ifname, sizeof(event->interface_status.ifname));
		goto free;
	}

	os_memcpy(event->interface_status.ifname, ifname, strlen(ifname));
	event->interface_status.ievent = EVENT_INTERFACE_REMOVED;

	msg.global = true;
	msg.ctx = ctx->supplicant;
	msg.event = EVENT_INTERFACE_STATUS;
	msg.data = event;

	ret = send_event(&msg);
	if (ret) {
		/* We failed notify WPA supplicant about interface removal.
		 * There is not much we can do, interface is still registered
		 * with WPA supplicant so we cannot unregister NM etc.
		 */
		wpa_printf(MSG_ERROR, "Failed to send event: %d", ret);
		goto free;
	}

	while (retry++ < count && wpa_s->wpa_state != WPA_INTERFACE_DISABLED) {
		k_sleep(K_MSEC(IFACE_NOTIFY_RETRY_MS));
	}

	if (wpa_s->wpa_state != WPA_INTERFACE_DISABLED) {
		LOG_ERR("Failed to notify remove interface %s", ifname);
		supplicant_generate_state_event(ifname, NET_EVENT_SUPPLICANT_CMD_IFACE_REMOVED, -1);
		goto out;
	}

	zephyr_wpa_ctrl_deinit(wpa_s);

	ret = zephyr_wpa_cli_global_cmd_v("interface_remove %s", ifname);
	if (ret) {
		LOG_ERR("Failed to remove interface %s", ifname);
		supplicant_generate_state_event(ifname, NET_EVENT_SUPPLICANT_CMD_IFACE_REMOVED,
					  -EINVAL);
		goto out;
	}

	ret = wifi_nm_unregister_mgd_iface(wifi_nm_get_instance("wifi_supplicant"), iface);
	if (ret) {
		LOG_ERR("Failed to unregister mgd iface %s with native stack (%d)",
			ifname, ret);
		goto out;
	}

	if (get_iface_count(ctx) == 0) {
		supplicant_generate_state_event(ifname, NET_EVENT_SUPPLICANT_CMD_NOT_READY, 0);
	}

	supplicant_generate_state_event(ifname, NET_EVENT_SUPPLICANT_CMD_IFACE_REMOVED, 0);

	return 0;

free:
	if (event) {
		os_free(event);
	}
out:
	return ret;
}
#endif
static void iface_work_handler(struct k_work *work)
{
	struct supplicant_context *ctx = CONTAINER_OF(work, struct supplicant_context,
						      iface_work);
	int ret;

	ret = (*ctx->iface_handler)(ctx, ctx->iface);
	if (ret < 0) {
		LOG_ERR("Interface %d (%p) handler failed (%d)",
			net_if_get_by_iface(ctx->iface), ctx->iface, ret);
	}
}

/* As the mgmt thread stack is limited, use a separate work queue for any network
 * interface add/delete.
 */
static void submit_iface_work(struct supplicant_context *ctx,
			      struct net_if *iface,
			      int (*handler)(struct supplicant_context *ctx,
					     struct net_if *iface))
{
	ctx->iface_handler = handler;

	k_work_submit_to_queue(&ctx->iface_wq, &ctx->iface_work);
}
#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_INF_MON
static void interface_handler(struct net_mgmt_event_callback *cb,
			      uint32_t mgmt_event, struct net_if *iface)
{
	if ((mgmt_event & INTERFACE_EVENT_MASK) != mgmt_event) {
		return;
	}

	if (!is_wanted_interface(iface)) {
		LOG_DBG("Ignoring event (0x%02x) from interface %d (%p)",
			mgmt_event, net_if_get_by_iface(iface), iface);
		return;
	}

	if (mgmt_event == NET_EVENT_IF_ADMIN_UP) {
		LOG_INF("Network interface %d (%p) up", net_if_get_by_iface(iface), iface);
		add_interface(get_default_context(), iface);
		return;
	}

	if (mgmt_event == NET_EVENT_IF_ADMIN_DOWN) {
		LOG_INF("Network interface %d (%p) down", net_if_get_by_iface(iface), iface);
		del_interface(get_default_context(), iface);
		return;
	}
}
#endif

static void iface_cb(struct net_if *iface, void *user_data)
{
	struct supplicant_context *ctx = user_data;
	int ret;

	if (!net_if_is_wifi(iface)) {
		return;
	}

#ifdef CONFIG_WIFI_NM_HOSTAPD_AP
	if (wifi_nm_iface_is_sap(iface)) {
		return;
	}
#endif

	if (!net_if_is_admin_up(iface)) {
		return;
	}

	ret = add_interface(ctx, iface);
	if (ret < 0) {
		return;
	}
}

static int setup_interface_monitoring(struct supplicant_context *ctx, struct net_if *iface)
{
	ARG_UNUSED(iface);
#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_INF_MON
	net_mgmt_init_event_callback(&ctx->cb, interface_handler,
				     INTERFACE_EVENT_MASK);
	net_mgmt_add_event_callback(&ctx->cb);
#endif
	net_if_foreach(iface_cb, ctx);

	return 0;
}

static void event_socket_handler(int sock, void *eloop_ctx, void *user_data)
{
	struct supplicant_context *ctx = user_data;
	struct wpa_supplicant_event_msg msg;
	int ret;

	ARG_UNUSED(eloop_ctx);
	ARG_UNUSED(ctx);

	ret = zsock_recv(sock, &msg, sizeof(msg), 0);
	if (ret < 0) {
		LOG_ERR("Failed to recv the message (%d)", -errno);
		return;
	}

	if (ret != sizeof(msg)) {
		LOG_ERR("Received incomplete message: got: %d, expected:%d",
			ret, sizeof(msg));
		return;
	}

	LOG_DBG("Passing message %d to wpa_supplicant", msg.event);

	if (msg.global) {
		wpa_supplicant_event_global(msg.ctx, msg.event, msg.data);
#ifdef CONFIG_WIFI_NM_HOSTAPD_AP
	} else if (msg.hostapd) {
		hostapd_event(msg.ctx, msg.event, msg.data);
#endif
	} else {
		wpa_supplicant_event(msg.ctx, msg.event, msg.data);
	}

	if (msg.data) {
		union wpa_event_data *data = msg.data;

		/* Free up deep copied data */
		if (msg.event == EVENT_AUTH) {
			os_free((char *)data->auth.ies);
		} else if (msg.event == EVENT_RX_MGMT) {
			os_free((char *)data->rx_mgmt.frame);
		} else if (msg.event == EVENT_TX_STATUS) {
			os_free((char *)data->tx_status.data);
		} else if (msg.event == EVENT_ASSOC) {
			os_free((char *)data->assoc_info.addr);
			os_free((char *)data->assoc_info.req_ies);
			os_free((char *)data->assoc_info.resp_ies);
			os_free((char *)data->assoc_info.resp_frame);
		} else if (msg.event == EVENT_ASSOC_REJECT) {
			os_free((char *)data->assoc_reject.bssid);
			os_free((char *)data->assoc_reject.resp_ies);
		} else if (msg.event == EVENT_DEAUTH) {
			os_free((char *)data->deauth_info.addr);
			os_free((char *)data->deauth_info.ie);
		} else if (msg.event == EVENT_DISASSOC) {
			os_free((char *)data->disassoc_info.addr);
			os_free((char *)data->disassoc_info.ie);
		} else if (msg.event == EVENT_UNPROT_DEAUTH) {
			os_free((char *)data->unprot_deauth.sa);
			os_free((char *)data->unprot_deauth.da);
		} else if (msg.event == EVENT_UNPROT_DISASSOC) {
			os_free((char *)data->unprot_disassoc.sa);
			os_free((char *)data->unprot_disassoc.da);
		}

		os_free(msg.data);
	}
}

static int register_supplicant_event_socket(struct supplicant_context *ctx)
{
	int ret;

	ret = socketpair(AF_UNIX, SOCK_STREAM, 0, ctx->event_socketpair);
	if (ret < 0) {
		ret = -errno;
		LOG_ERR("Failed to initialize socket (%d)", ret);
		return ret;
	}

	eloop_register_read_sock(ctx->event_socketpair[0], event_socket_handler, NULL, ctx);

	return 0;
}

#ifdef CONFIG_WIFI_NM_HOSTAPD_AP
struct hostapd_iface *hostapd_get_interface(const char *ifname)
{
	struct supplicant_context *ctx;

	ctx = get_default_context();
	(void)ifname;
	return ctx->hostapd.iface[0];
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
				 &hostapd_event_eapol_rx, bss, 0);
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
	supplicant_send_wifi_mgmt_ap_status(hapd_iface,
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
		if (wpa_drivers[i] != hapd->driver)
			continue;

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
	conf->ieee80211ax       = 1;
	conf->he_oper_chwidth   = CHANWIDTH_USE_HT;
	conf->he_phy_capab.he_su_beamformer = 0;
	conf->he_phy_capab.he_su_beamformee = 1;
	conf->he_phy_capab.he_mu_beamformer = 0;
	conf->he_op.he_bss_color = 1;
	conf->he_op.he_default_pe_duration  = 0;
	/* Set default basic MCS/NSS set to single stream MCS 0-7 */
	conf->he_op.he_basic_mcs_nss_set    = 0xfffc;

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

static void zephyr_hostapd_init(struct supplicant_context *ctx)
{
	struct hapd_interfaces *interfaces = &ctx->hostapd;
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
		LOG_ERR("Cannot get interface %d (%p) name", net_if_get_by_iface(iface), iface);
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

	ret = wifi_nm_register_mgd_iface(wifi_nm_get_instance("hostapd"), iface);
	if (ret) {
		LOG_ERR("Failed to register mgd iface with native stack %s (%d)",
			ifname, ret);
		goto out;
	}
out:
	return;
}

static const char *zephyr_hostap_msg_ifname_cb(void *ctx)
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

static void zephyr_hostap_ctrl_iface_msg_cb(void *ctx, int level, enum wpa_msg_type type,
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
#endif

static void handler(void)
{
	struct supplicant_context *ctx;
	struct wpa_params params;

#if !defined(CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_NONE) && !defined(CONFIG_MBEDTLS_ENABLE_HEAP)
	/* Needed for crypto operation as default is no-op and fails */
	mbedtls_platform_set_calloc_free(calloc, free);
#endif /* !CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_NONE && !CONFIG_MBEDTLS_ENABLE_HEAP */

#ifdef CONFIG_WIFI_NM_WPA_SUPPLICANT_CRYPTO_MBEDTLS_PSA
	supp_psa_crypto_init();
#endif

	ctx = get_default_context();

	k_work_queue_init(&ctx->iface_wq);
	k_work_queue_start(&ctx->iface_wq, iface_wq_stack,
			   K_THREAD_STACK_SIZEOF(iface_wq_stack),
			   CONFIG_WIFI_NM_WPA_SUPPLICANT_WQ_PRIO,
			   NULL);

	k_work_init(&ctx->iface_work, iface_work_handler);

	memset(&params, 0, sizeof(params));
	params.wpa_debug_level = CONFIG_WIFI_NM_WPA_SUPPLICANT_DEBUG_LEVEL;

	ctx->supplicant = wpa_supplicant_init(&params);
	if (ctx->supplicant == NULL) {
		LOG_ERR("Failed to initialize %s", "wpa_supplicant");
		goto err;
	}

	LOG_INF("%s initialized", "wpa_supplicant");

	if (fst_global_init()) {
		LOG_ERR("Failed to initialize %s", "FST");
		goto out;
	}

#if defined(CONFIG_FST) && defined(CONFIG_CTRL_IFACE)
	if (!fst_global_add_ctrl(fst_ctrl_cli)) {
		LOG_WRN("Failed to add CLI FST ctrl");
	}
#endif
	zephyr_global_wpa_ctrl_init();

	register_supplicant_event_socket(ctx);

	submit_iface_work(ctx, NULL, setup_interface_monitoring);

#ifdef CONFIG_WIFI_NM_HOSTAPD_AP
	zephyr_hostapd_init(ctx);
	wpa_msg_register_ifname_cb(zephyr_hostap_msg_ifname_cb);
#endif

	(void)wpa_supplicant_run(ctx->supplicant);

	supplicant_generate_state_event(ctx->if_name, NET_EVENT_SUPPLICANT_CMD_NOT_READY, 0);

	eloop_unregister_read_sock(ctx->event_socketpair[0]);

	zephyr_global_wpa_ctrl_deinit();

	fst_global_deinit();

out:
	wpa_supplicant_deinit(ctx->supplicant);

	zsock_close(ctx->event_socketpair[0]);
	zsock_close(ctx->event_socketpair[1]);

err:
	os_free(params.pid_file);
}

static int init(void)
{
	/* We create a thread that handles all supplicant connections */
	k_thread_create(&tid, supplicant_thread_stack,
			K_THREAD_STACK_SIZEOF(supplicant_thread_stack),
			(k_thread_entry_t)handler, NULL, NULL, NULL,
			CONFIG_WIFI_NM_WPA_SUPPLICANT_PRIO, 0, K_NO_WAIT);

	return 0;
}

SYS_INIT(init, APPLICATION, 0);
