/*
 * Copyright (c) 2023 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wifi_supplicant, CONFIG_WIFI_NM_WPA_SUPPLICANT_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <poll.h>

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

static const struct wifi_mgmt_ops mgmt_ops = {
	.scan = supplicant_scan,
	.connect = supplicant_connect,
	.disconnect = supplicant_disconnect,
	.iface_status = supplicant_status,
#ifdef CONFIG_NET_STATISTICS_WIFI
	.get_stats = supplicant_get_stats,
#endif
	.set_power_save = supplicant_set_power_save,
	.set_twt = supplicant_set_twt,
	.get_power_save_config = supplicant_get_power_save_config,
	.reg_domain = supplicant_reg_domain,
};

DEFINE_WIFI_NM_INSTANCE(wifi_supplicant, &mgmt_ops);

#define WRITE_TIMEOUT 100 /* ms */
#define INTERFACE_EVENT_MASK (NET_EVENT_IF_ADMIN_UP | NET_EVENT_IF_ADMIN_DOWN)

struct supplicant_context {
	struct wpa_global *supplicant;
	struct net_mgmt_event_callback cb;
	struct net_if *iface;
	char if_name[CONFIG_NET_INTERFACE_NAME_LEN + 1];
	int event_socketpair[2];
	struct k_work iface_work;
	struct k_work_q iface_wq;
	int (*iface_handler)(struct supplicant_context *ctx, struct net_if *iface);
};

static struct supplicant_context *get_default_context(void)
{
	static struct supplicant_context ctx;

	return &ctx;
}

struct wpa_global *zephyr_get_default_supplicant_context(void)
{
	return get_default_context()->supplicant;
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

struct wpa_supplicant *zephyr_get_handle_by_ifname(const char *ifname)
{
	struct wpa_supplicant *wpa_s = NULL;
	struct supplicant_context *ctx = get_default_context();
	int ret;

	wpa_s = wpa_supplicant_get_iface(ctx->supplicant, ifname);
	if (!wpa_s) {
		wpa_printf(MSG_ERROR, "%s: Unable to get wpa_s handle for %s\n", __func__, ifname);
		return NULL;
	}

	return wpa_s;
}

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

	ret = wifi_nm_register_mgd_iface(wifi_nm_get_instance("wifi_supplicant"), iface);
	if (ret) {
		LOG_ERR("Failed to register mgd iface with native stack %s (%d)",
			ifname, ret);
		goto out;
	}

	supplicant_generate_state_event(ifname, NET_EVENT_SUPPLICANT_CMD_IFACE_ADDED, 0);

	if (get_iface_count(ctx) == 1) {
		supplicant_generate_state_event(ifname, NET_EVENT_SUPPLICANT_CMD_READY, 0);
	}

	ret = 0;

out:
	return ret;
}

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
		goto out;
	}

	supplicant_generate_state_event(ifname, NET_EVENT_SUPPLICANT_CMD_IFACE_REMOVING, 0);

	os_memcpy(event->interface_status.ifname, ifname, IFNAMSIZ);
	event->interface_status.ievent = EVENT_INTERFACE_REMOVED;

	msg.global = true;
	msg.ctx = ctx->supplicant;
	msg.event = EVENT_INTERFACE_STATUS;
	msg.data = event;

	send_event(&msg);

	while (retry++ < count && wpa_s->wpa_state != WPA_INTERFACE_DISABLED) {
		k_sleep(K_MSEC(IFACE_NOTIFY_RETRY_MS));
	}

	if (wpa_s->wpa_state != WPA_INTERFACE_DISABLED) {
		LOG_ERR("Failed to notify remove interface %s", ifname);
		supplicant_generate_state_event(ifname, NET_EVENT_SUPPLICANT_CMD_IFACE_REMOVED, -1);
		goto out;
	}

	ret = zephyr_wpa_cli_global_cmd_v("interface_remove %s", ifname);
	if (ret) {
		LOG_ERR("Failed to remove interface %s", ifname);
		supplicant_generate_state_event(ifname, NET_EVENT_SUPPLICANT_CMD_IFACE_REMOVED,
					  -EINVAL);
		goto out;
	}

	ret = wifi_nm_unregister_mgd_iface(wifi_nm_get_instance("wpa_supplicant"), iface);
	if (ret) {
		LOG_ERR("Failed to unregister mgd iface %s with native stack (%d)",
			ifname, ret);
		goto out;
	}

	if (get_iface_count(ctx) == 0) {
		supplicant_generate_state_event(ifname, NET_EVENT_SUPPLICANT_CMD_NOT_READY, 0);
	}

	supplicant_generate_state_event(ifname, NET_EVENT_SUPPLICANT_CMD_IFACE_REMOVED, 0);

out:
	if (event) {
		os_free(event);
	}

	return ret;
}

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

static void interface_handler(struct net_mgmt_event_callback *cb,
			      uint32_t mgmt_event, struct net_if *iface)
{
	struct supplicant_context *ctx = CONTAINER_OF(cb, struct supplicant_context,
						      cb);

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
		submit_iface_work(ctx, iface, add_interface);
		return;
	}

	if (mgmt_event == NET_EVENT_IF_ADMIN_DOWN) {
		LOG_INF("Network interface %d (%p) down", net_if_get_by_iface(iface), iface);
		submit_iface_work(ctx, iface, del_interface);
		return;
	}
}

static void iface_cb(struct net_if *iface, void *user_data)
{
	struct supplicant_context *ctx = user_data;
	int ret;

	if (!net_if_is_wifi(iface)) {
		return;
	}

	if (!net_if_is_up(iface)) {
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

	net_mgmt_init_event_callback(&ctx->cb, interface_handler,
				     INTERFACE_EVENT_MASK);
	net_mgmt_add_event_callback(&ctx->cb);

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

static void handler(void)
{
	struct supplicant_context *ctx;
	struct wpa_params params;

#if defined(CONFIG_WPA_SUPP_CRYPTO) && !defined(CONFIG_MBEDTLS_ENABLE_HEAP)
	/* Needed for crypto operation as default is no-op and fails */
	mbedtls_platform_set_calloc_free(calloc, free);
#endif /* CONFIG_WPA_SUPP_CRYPTO */

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

	(void)wpa_supplicant_run(ctx->supplicant);

	supplicant_generate_state_event(ctx->if_name, NET_EVENT_SUPPLICANT_CMD_NOT_READY, 0);

	eloop_unregister_read_sock(ctx->event_socketpair[0]);

	zephyr_wpa_ctrl_deinit(ctx->supplicant);
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
			0, 0, K_NO_WAIT);

	return 0;
}

SYS_INIT(init, APPLICATION, 0);
