/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * WPA Supplicant / main() function for Zephyr OS
 * Copyright (c) 2003-2013, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include <zephyr/logging/log.h>
#include <sys/fcntl.h>
LOG_MODULE_REGISTER(wpa_supplicant, LOG_LEVEL_DBG);

#include <zephyr/net/wifi_nm.h>

#include "includes.h"
#include "common.h"
#include "eloop.h"
#include "wpa_supplicant/config.h"
#include "wpa_supplicant_i.h"

#include "fst/fst.h"
#include "includes.h"
#include "p2p_supplicant.h"
#include "wpa_supplicant_i.h"
#include "driver_i.h"

#include "supp_main.h"
#include "supp_api.h"


K_SEM_DEFINE(z_wpas_ready_sem, 0, 1);
#include <l2_packet/l2_packet.h>

#define DUMMY_SOCKET_PORT 9999

struct wpa_global *global;

static int wpa_event_sock = -1;

static void start_wpa_supplicant(void);

K_THREAD_DEFINE(wpa_s_tid,
			CONFIG_WIFI_NM_WPA_SUPP_THREAD_STACK_SIZE,
			start_wpa_supplicant,
			NULL,
			NULL,
			NULL,
			0,
			0,
			0);

static const struct wifi_mgmt_ops wpa_supp_ops = {
	.scan = z_wpa_supplicant_scan,
	.connect = z_wpa_supplicant_connect,
	.disconnect = z_wpa_supplicant_disconnect,
	.iface_status = z_wpa_supplicant_status,
#ifdef CONFIG_NET_STATISTICS_WIFI
	.get_stats = z_wpa_supplicant_get_stats,
#endif
	.set_power_save = z_wpa_supplicant_set_power_save,
	.set_twt = z_wpa_supplicant_set_twt,
	.get_power_save_config = z_wpa_supplicant_get_power_save_config,
	.reg_domain = z_wpa_supplicant_reg_domain,
	.mode = z_wpa_supplicant_mode,
	.filter = z_wpa_supplicant_filter,
	.channel = z_wpa_supplicant_channel,
};

DEFINE_WIFI_NM_INSTANCE(wpa_supplicant, &wpa_supp_ops);

#ifdef CONFIG_MATCH_IFACE
static int wpa_supplicant_init_match(struct wpa_global *global)
{
	/*
	 * The assumption is that the first driver is the primary driver and
	 * will handle the arrival / departure of interfaces.
	 */
	if (wpa_drivers[0]->global_init && !global->drv_priv[0]) {
		global->drv_priv[0] = wpa_drivers[0]->global_init(global);
		if (!global->drv_priv[0]) {
			wpa_printf(MSG_ERROR,
				   "Failed to initialize driver '%s'",
				   wpa_drivers[0]->name);
			return -1;
		}
	}

	return 0;
}
#endif /* CONFIG_MATCH_IFACE */

struct wpa_supplicant *z_wpas_get_handle_by_ifname(const char *ifname)
{
	struct wpa_supplicant *wpa_s = NULL;
	int ret = k_sem_take(&z_wpas_ready_sem, K_SECONDS(2));

	if (ret) {
		wpa_printf(MSG_ERROR, "%s: WPA supplicant not ready: %d", __func__, ret);
		return NULL;
	}

	k_sem_give(&z_wpas_ready_sem);

	wpa_s = wpa_supplicant_get_iface(global, ifname);
	if (!wpa_s) {
		wpa_printf(MSG_ERROR,
		"%s: Unable to get wpa_s handle for %s\n", __func__, ifname);
		return NULL;
	}

	return wpa_s;
}

#include "config.h"
static void iface_cb(struct net_if *iface, void *user_data)
{
	struct wpa_interface *ifaces = user_data;
	struct net_linkaddr *link_addr = NULL;
	int ifindex;
	char own_addr[6];
	char *ifname;

	ifindex = net_if_get_by_iface(iface);
	link_addr = &iface->if_dev->link_addr;
	os_memcpy(own_addr, link_addr->addr, link_addr->len);
	ifname = os_strdup(iface->if_dev->dev->name);

	wpa_printf(
		MSG_INFO,
		"%s: iface %s ifindex %d %02x:%02x:%02x:%02x:%02x:%02x",
		__func__, ifname, ifindex, own_addr[0], own_addr[1], own_addr[2],
		own_addr[3], own_addr[4], own_addr[5]);

	/* TODO : make this user configurable*/
	ifaces[0].ifname = "wlan0";
}

static void wpa_event_sock_handler(int sock, void *eloop_ctx, void *sock_ctx)
{
	int ret;
	struct wpa_supplicant_event_msg msg;

	ARG_UNUSED(eloop_ctx);
	ARG_UNUSED(sock_ctx);

	ret = recv(sock, &msg, sizeof(msg), 0);

	if (ret < 0) {
		wpa_printf(MSG_ERROR, "Failed to recv the message: %s", strerror(errno));
		return;
	}

	if (ret != sizeof(msg)) {
		wpa_printf(MSG_ERROR, "Received incomplete message: got: %d, expected:%d",
			ret, sizeof(msg));
		return;
	}

	if (msg.ignore_msg) {
		return;
	}

	wpa_printf(MSG_DEBUG, "Passing message %d to wpa_supplicant", msg.event);

	wpa_supplicant_event(msg.ctx, msg.event, msg.data);

	if (msg.data) {
		os_free(msg.data);
	}
}

static int register_wpa_event_sock(void)
{
	struct sockaddr_in addr;
	int ret;

	wpa_event_sock = socket(PF_INET, SOCK_DGRAM, 0);

	if (wpa_event_sock < 0) {
		wpa_printf(MSG_ERROR, "Failed to initialize socket: %s", strerror(errno));
		return -1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_family = AF_INET;
	addr.sin_port = DUMMY_SOCKET_PORT;

	ret = bind(wpa_event_sock, (struct sockaddr *) &addr, sizeof(addr));
	if (ret < 0) {
		wpa_printf(MSG_ERROR, "Failed to bind socket: %s", strerror(errno));
		return -1;
	}

	eloop_register_read_sock(wpa_event_sock, wpa_event_sock_handler, NULL, NULL);

	return 0;
}

int send_wpa_supplicant_event(const struct wpa_supplicant_event_msg *msg)
{
	int ret;
	unsigned int retry = 0;
	struct sockaddr_in dst = {
		.sin_addr = {
			.s4_addr[0] = 127,
			.s4_addr[1] = 0,
			.s4_addr[2] = 0,
			.s4_addr[3] = 2
		 },
		.sin_family = AF_INET,
		.sin_port = DUMMY_SOCKET_PORT
	};

	if (wpa_event_sock < 0) {
		return -1;
	}

retry_send:
	ret = sendto(wpa_event_sock, msg, sizeof(*msg), 0, (struct sockaddr *)&dst, sizeof(dst));
	if (ret < 0) {
		if (errno == EINTR || errno == EAGAIN || errno == EBUSY || errno == EWOULDBLOCK) {
			k_msleep(2);
			if (retry++ < 3) {
				goto retry_send;
			} else {
				wpa_printf(MSG_WARNING, "Dummy socket send fail (max retries): %s",
					strerror(errno));
				return -1;
			}
		} else {
			wpa_printf(MSG_WARNING, "Dummy socket send fail: %s",
				strerror(errno));
			return -1;
		}
	}

	return 0;
}

static void start_wpa_supplicant(void)
{
	int i;
	struct wpa_interface *ifaces, *iface;
	int iface_count, exitcode = -1;
	struct wpa_params params;

	os_memset(&params, 0, sizeof(params));
	params.wpa_debug_level = CONFIG_WIFI_NM_WPA_SUPP_DEBUG_LEVEL;

	wpa_printf(MSG_INFO, "%s: %d Starting wpa_supplicant thread with debug level: %d\n",
		  __func__, __LINE__, params.wpa_debug_level);

	iface = ifaces = os_zalloc(sizeof(struct wpa_interface));
	if (ifaces == NULL) {
		return;
	}
	iface_count = 1;

	exitcode = 0;
	global = wpa_supplicant_init(&params);
	if (global == NULL) {
		wpa_printf(MSG_ERROR, "Failed to initialize wpa_supplicant");
		exitcode = -1;
		goto out;
	} else {
		wpa_printf(MSG_INFO, "Successfully initialized "
				     "wpa_supplicant");
	}

	if (fst_global_init()) {
		wpa_printf(MSG_ERROR, "Failed to initialize FST");
		exitcode = -1;
		goto out;
	}

#if defined(CONFIG_FST) && defined(CONFIG_CTRL_IFACE)
	if (!fst_global_add_ctrl(fst_ctrl_cli)) {
		wpa_printf(MSG_WARNING, "Failed to add CLI FST ctrl");
	}
#endif

	net_if_foreach(iface_cb, ifaces);

	ifaces[0].ctrl_interface = "test_ctrl";
	params.ctrl_interface = "test_ctrl";
	wpa_printf(MSG_INFO, "Using interface %s\n", ifaces[0].ifname);

	for (i = 0; exitcode == 0 && i < iface_count; i++) {
		struct wpa_supplicant *wpa_s;

		if ((ifaces[i].confname == NULL &&
		     ifaces[i].ctrl_interface == NULL) ||
		    ifaces[i].ifname == NULL) {
			if (iface_count == 1 && (params.ctrl_interface ||
#ifdef CONFIG_MATCH_IFACE
						 params.match_iface_count ||
#endif /* CONFIG_MATCH_IFACE */
						 params.dbus_ctrl_interface))
				break;
			wpa_printf(MSG_INFO,
				   "Failed to initialize interface %d\n", i);
			exitcode = -1;
			break;
		}
		wpa_printf(MSG_INFO, "Initializing interface %d: %s\n", i,
			   ifaces[i].ifname);
		wpa_s = wpa_supplicant_add_iface(global, &ifaces[i], NULL);
		if (wpa_s == NULL) {
			exitcode = -1;
			break;
		}
		wpa_s->conf->filter_ssids = 1;
		wpa_s->conf->ap_scan = 1;
	}

	register_wpa_event_sock();

#ifdef CONFIG_MATCH_IFACE
	if (exitcode == 0) {
		exitcode = wpa_supplicant_init_match(global);
	}
#endif /* CONFIG_MATCH_IFACE */

	if (exitcode == 0) {
		exitcode = wpa_supplicant_run(global);
	}

	eloop_unregister_read_sock(wpa_event_sock);

	wpa_supplicant_deinit(global);

	fst_global_deinit();

	if (wpa_event_sock) {
		close(wpa_event_sock);
	}

out:
	os_free(ifaces);
#ifdef CONFIG_MATCH_IFACE
	os_free(params.match_ifaces);
#endif /* CONFIG_MATCH_IFACE */
	os_free(params.pid_file);

	wpa_printf(MSG_INFO, "%s: exitcode %d", __func__, exitcode);
}
