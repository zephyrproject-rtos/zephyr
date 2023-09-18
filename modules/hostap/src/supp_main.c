/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <sys/fcntl.h>
LOG_MODULE_REGISTER(wpa_supplicant, LOG_LEVEL_DBG);

#if defined(CONFIG_WPA_SUPP_CRYPTO) && !defined(CONFIG_MBEDTLS_ENABLE_HEAP)
#include <mbedtls/platform.h>
#endif /* CONFIG_WPA_SUPP_CRYPTO */

#include <zephyr/net/socket.h>
#include <zephyr/net/socket_select.h>
#include <zephyr/net/wifi_nm.h>

#include "includes.h"
#include "common.h"
#include "eloop.h"
#include "wpa_supplicant/config.h"
#include "wpa_supplicant_i.h"

#include "fst/fst.h"
#include "includes.h"
#include "p2p_supplicant.h"
#include "driver_i.h"

#include "supp_main.h"
#include "supp_events.h"
#include "wpa_cli_zephyr.h"

#include "supp_api.h"

K_SEM_DEFINE(z_wpas_ready_sem, 0, 1);
#include <l2_packet/l2_packet.h>

/* Should match with the driver name */
#define DEFAULT_IFACE_NAME "wlan0"
#define IFACE_MATCHING_PREFIX "wlan"

static struct net_mgmt_event_callback cb;
struct k_mutex iface_up_mutex;

struct wpa_global *global;

static int z_wpas_event_sockpair[2];

static void z_wpas_start(void);
static void z_wpas_iface_work_handler(struct k_work *item);

static K_THREAD_STACK_DEFINE(z_wpa_s_thread_stack,
			     CONFIG_WIFI_NM_WPA_SUPPLICANT_THREAD_STACK_SIZE);
static struct k_thread z_wpa_s_tid;

static K_THREAD_STACK_DEFINE(z_wpas_iface_wq_stack,
	CONFIG_WIFI_NM_WPA_SUPPLICANT_WQ_STACK_SIZE);

/* TODO: Debug why wsing system workqueue blocks the driver dedicated
 * workqueue?
 */
static struct k_work_q z_wpas_iface_wq;
static K_WORK_DEFINE(z_wpas_iface_work,
	z_wpas_iface_work_handler);

K_MUTEX_DEFINE(z_wpas_event_mutex);

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
};

DEFINE_WIFI_NM_INSTANCE(wpa_supplicant, &wpa_supp_ops);

#ifdef CONFIG_MATCH_IFACE
static int z_wpas_init_match(struct wpa_global *global)
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

static int z_wpas_get_iface_count(void)
{
	struct wpa_supplicant *wpa_s;
	unsigned int count = 0;

	for (wpa_s = global->ifaces; wpa_s; wpa_s = wpa_s->next) {
		count += 1;
	}
	return count;
}

#define Z_WPA_S_IFACE_NOTIFY_TIMEOUT_MS 1000
#define Z_WPA_S_IFACE_NOTIFY_RETRY_MS 10

static int z_wpas_add_interface(const char *ifname)
{
	struct wpa_supplicant *wpa_s;
	struct net_if *iface;
	int ret = -1;
	int retry = 0, count = Z_WPA_S_IFACE_NOTIFY_TIMEOUT_MS / Z_WPA_S_IFACE_NOTIFY_RETRY_MS;

	wpa_printf(MSG_DEBUG, "Adding interface %s\n", ifname);

	iface = net_if_lookup_by_dev(device_get_binding(ifname));
	if (!iface) {
		wpa_printf(MSG_ERROR, "Failed to get net_if handle for %s", ifname);
		goto err;
	}

	ret = z_wpa_cli_global_cmd_v("interface_add %s %s %s %s",
				ifname, "zephyr", "zephyr", "zephyr");
	if (ret) {
		wpa_printf(MSG_ERROR, "Failed to add interface: %s", ifname);
		goto err;
	}

	/* This cannot be through control interface as need the handle */
	while (retry++ < count && !wpa_supplicant_get_iface(global, ifname)) {
		k_sleep(K_MSEC(Z_WPA_S_IFACE_NOTIFY_RETRY_MS));
	}

	wpa_s = wpa_supplicant_get_iface(global, ifname);
	if (wpa_s == NULL) {
		wpa_printf(MSG_ERROR, "Failed to add iface: %s", ifname);
		goto err;
	}

	wpa_s->conf->filter_ssids = 1;
	wpa_s->conf->ap_scan = 1;

	/* Default interface, kick start wpa_supplicant */
	if (z_wpas_get_iface_count() == 1) {
		k_mutex_unlock(&iface_up_mutex);
	}

	ret = z_wpa_ctrl_init(wpa_s);
	if (ret) {
		wpa_printf(MSG_ERROR, "Failed to initialize control interface");
		goto err;
	}

	ret = wifi_nm_register_mgd_iface(wifi_nm_get_instance("wifi_supplicant"), iface);

	if (ret) {
		wpa_printf(MSG_ERROR, "Failed to register mgd iface with native stack: %s (%d)",
			ifname, ret);
		goto err;
	}

	generate_supp_state_event(ifname, NET_EVENT_WPA_SUPP_CMD_IFACE_ADDED, 0);

	if (z_wpas_get_iface_count() == 1) {
		generate_supp_state_event(ifname, NET_EVENT_WPA_SUPP_CMD_READY, 0);
	}

	return 0;
err:
	return ret;
}

static int z_wpas_remove_interface(const char *ifname)
{
	int ret = -1;
	union wpa_event_data *event = os_zalloc(sizeof(*event));
	struct wpa_supplicant *wpa_s = wpa_supplicant_get_iface(global, ifname);
	struct net_if *iface = net_if_lookup_by_dev(device_get_binding(ifname));

	int retry = 0, count = Z_WPA_S_IFACE_NOTIFY_TIMEOUT_MS / Z_WPA_S_IFACE_NOTIFY_RETRY_MS;

	if (!event) {
		wpa_printf(MSG_ERROR, "Failed to allocate event data");
		goto err;
	}

	if (!wpa_s) {
		wpa_printf(MSG_ERROR, "Failed to get wpa_s handle for %s", ifname);
		goto err;
	}

	if (!iface) {
		wpa_printf(MSG_ERROR, "Failed to get net_if handle for %s", ifname);
		goto err;
	}

	generate_supp_state_event(ifname, NET_EVENT_WPA_SUPP_CMD_IFACE_REMOVING, 0);
	wpa_printf(MSG_DEBUG, "Remove interface %s\n", ifname);

	os_memcpy(event->interface_status.ifname, ifname, IFNAMSIZ);
	event->interface_status.ievent = EVENT_INTERFACE_REMOVED;

	struct wpa_supplicant_event_msg msg = {
		.global = true,
		.ctx = global,
		.event = EVENT_INTERFACE_STATUS,
		.data = event,
	};

	z_wpas_send_event(&msg);

	while (retry++ < count &&
		   wpa_s->wpa_state != WPA_INTERFACE_DISABLED) {
		k_sleep(K_MSEC(Z_WPA_S_IFACE_NOTIFY_RETRY_MS));
	}

	if (wpa_s->wpa_state != WPA_INTERFACE_DISABLED) {
		wpa_printf(MSG_ERROR, "Failed to notify remove interface: %s", ifname);
		generate_supp_state_event(ifname, NET_EVENT_WPA_SUPP_CMD_IFACE_REMOVED,
		-1);
		goto err;
	}

	z_wpa_ctrl_deinit(wpa_s);

	ret = z_wpa_cli_global_cmd_v("interface_remove %s", ifname);
	if (ret) {
		wpa_printf(MSG_ERROR, "Failed to remove interface: %s", ifname);
		generate_supp_state_event(ifname, NET_EVENT_WPA_SUPP_CMD_IFACE_REMOVED,
		-EINVAL);
		goto err;
	}

	ret = wifi_nm_unregister_mgd_iface(wifi_nm_get_instance("wpa_supplicant"), iface);
	if (ret) {
		wpa_printf(MSG_ERROR, "Failed to unregister mgd iface with native stack: %s (%d)",
			ifname, ret);
		goto err;
	}


	if (z_wpas_get_iface_count() == 0) {
		generate_supp_state_event(ifname, NET_EVENT_WPA_SUPP_CMD_NOT_READY, 0);
	}

	generate_supp_state_event(ifname, NET_EVENT_WPA_SUPP_CMD_IFACE_REMOVED, 0);

	return 0;
err:
	if (event) {
		os_free(event);
	}
	return ret;
}

static void iface_event_handler(struct net_mgmt_event_callback *cb,
							uint32_t mgmt_event, struct net_if *iface)
{
	const char *ifname = iface->if_dev->dev->name;

	if (strncmp(ifname, IFACE_MATCHING_PREFIX, sizeof(IFACE_MATCHING_PREFIX) - 1) != 0) {
		return;
	}

	wpa_printf(MSG_DEBUG, "Event: %d", mgmt_event);
	if (mgmt_event == NET_EVENT_IF_ADMIN_UP) {
		z_wpas_add_interface(ifname);
	} else if (mgmt_event == NET_EVENT_IF_ADMIN_DOWN) {
		z_wpas_remove_interface(ifname);
	}
}

static void register_iface_events(void)
{
	k_mutex_init(&iface_up_mutex);

	k_mutex_lock(&iface_up_mutex, K_FOREVER);
	net_mgmt_init_event_callback(&cb, iface_event_handler,
		NET_EVENT_IF_ADMIN_UP | NET_EVENT_IF_ADMIN_DOWN);
	net_mgmt_add_event_callback(&cb);
}

static void wait_for_interface_up(const char *iface_name)
{
	if (z_wpas_get_iface_count() == 0) {
		k_mutex_lock(&iface_up_mutex, K_FOREVER);
	}
}

#include "config.h"
static void iface_cb(struct net_if *iface, void *user_data)
{
	const char *ifname = iface->if_dev->dev->name;

	if (ifname == NULL) {
		return;
	}

	if (strncmp(ifname, DEFAULT_IFACE_NAME, strlen(ifname)) != 0) {
		return;
	}

	/* Check default interface */
	if (net_if_is_admin_up(iface)) {
		z_wpas_add_interface(ifname);
	}

	register_iface_events();
}


static void z_wpas_iface_work_handler(struct k_work *item)
{
	ARG_UNUSED(item);

	int ret = k_sem_take(&z_wpas_ready_sem, K_SECONDS(5));

	if (ret) {
		wpa_printf(MSG_ERROR, "Timed out waiting for wpa_supplicant");
		return;
	}

	net_if_foreach(iface_cb, NULL);
	wait_for_interface_up(DEFAULT_IFACE_NAME);

	k_sem_give(&z_wpas_ready_sem);
}

static void z_wpas_event_sock_handler(int sock, void *eloop_ctx, void *sock_ctx)
{
	int ret;

	ARG_UNUSED(eloop_ctx);
	ARG_UNUSED(sock_ctx);
	struct wpa_supplicant_event_msg msg;

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

	wpa_printf(MSG_DEBUG, "Passing message %d to wpa_supplicant", msg.event);

	if (msg.global) {
		wpa_supplicant_event_global(msg.ctx, msg.event, msg.data);
	} else {
		wpa_supplicant_event(msg.ctx, msg.event, msg.data);
	}

	if (msg.data) {
		if (msg.event == EVENT_AUTH) {
			union wpa_event_data *data = msg.data;

			os_free((char *)data->auth.ies);
		}
		os_free(msg.data);
	}
}

static int register_wpa_event_sock(void)
{
	int ret;

	ret = socketpair(AF_UNIX, SOCK_STREAM, 0, z_wpas_event_sockpair);

	if (ret != 0) {
		wpa_printf(MSG_ERROR, "Failed to initialize socket: %s", strerror(errno));
		return -1;
	}

	eloop_register_read_sock(z_wpas_event_sockpair[0], z_wpas_event_sock_handler, NULL, NULL);

	return 0;
}

int z_wpas_send_event(const struct wpa_supplicant_event_msg *msg)
{
	int ret;
	unsigned int retry = 0;

	k_mutex_lock(&z_wpas_event_mutex, K_FOREVER);

	if (z_wpas_event_sockpair[1] < 0) {
		goto err;
	}

retry_send:
	ret = send(z_wpas_event_sockpair[1], msg, sizeof(*msg), 0);
	if (ret < 0) {
		if (errno == EINTR || errno == EAGAIN || errno == EBUSY || errno == EWOULDBLOCK) {
			k_msleep(2);
			if (retry++ < 3) {
				goto retry_send;
			} else {
				wpa_printf(MSG_WARNING, "Event send fail (max retries): %s",
					strerror(errno));
				goto err;
			}
		} else {
			wpa_printf(MSG_WARNING, "Event send fail: %s",
				strerror(errno));
			goto err;
		}
	}

	ret = 0;
err:
	k_mutex_unlock(&z_wpas_event_mutex);
	return -1;
}

static void z_wpas_start(void)
{
	struct wpa_params params;
	int exitcode = -1;

#if defined(CONFIG_WPA_SUPP_CRYPTO) && !defined(CONFIG_MBEDTLS_ENABLE_HEAP)
	/* Needed for crypto operation as default is no-op and fails */
	mbedtls_platform_set_calloc_free(calloc, free);
#endif /* CONFIG_WPA_CRYPTO */

	k_work_queue_init(&z_wpas_iface_wq);

	k_work_queue_start(&z_wpas_iface_wq,
					   z_wpas_iface_wq_stack,
					   K_THREAD_STACK_SIZEOF(z_wpas_iface_wq_stack),
					   7,
					   NULL);

	os_memset(&params, 0, sizeof(params));
	params.wpa_debug_level = CONFIG_WIFI_NM_WPA_SUPPLICANT_DEBUG_LEVEL;

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
	z_global_wpa_ctrl_init();

	register_wpa_event_sock();

	k_work_submit_to_queue(&z_wpas_iface_wq, &z_wpas_iface_work);

#ifdef CONFIG_MATCH_IFACE
	if (exitcode == 0) {
		exitcode = z_wpas_init_match(global);
	}
#endif /* CONFIG_MATCH_IFACE */

	if (exitcode == 0) {
		k_sem_give(&z_wpas_ready_sem);
		exitcode = wpa_supplicant_run(global);
	}

	generate_supp_state_event(DEFAULT_IFACE_NAME, NET_EVENT_WPA_SUPP_CMD_NOT_READY, 0);
	eloop_unregister_read_sock(z_wpas_event_sockpair[0]);

	z_global_wpa_ctrl_deinit();
	wpa_supplicant_deinit(global);

	fst_global_deinit();

	close(z_wpas_event_sockpair[0]);
	close(z_wpas_event_sockpair[1]);

out:
#ifdef CONFIG_MATCH_IFACE
	os_free(params.match_ifaces);
#endif /* CONFIG_MATCH_IFACE */
	os_free(params.pid_file);

	wpa_printf(MSG_INFO, "z_wpas_start: exitcode %d", exitcode);
}

static int z_wpas_init(void)
{
	k_thread_create(&z_wpa_s_tid, z_wpa_s_thread_stack,
			CONFIG_WIFI_NM_WPA_SUPPLICANT_THREAD_STACK_SIZE,
			(k_thread_entry_t)z_wpas_start,
			NULL, NULL, NULL,
			0, 0, K_NO_WAIT);

	return 0;
}

SYS_INIT(z_wpas_init, APPLICATION, 0);
