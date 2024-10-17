/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_connectivity_check_sample, LOG_LEVEL_DBG);

#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/net/http/client.h>
#include <zephyr/net/conn_mgr_monitor.h>

#include "ca_certificate.h"

static void trigger_check(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(dwork, trigger_check);
static K_SEM_DEFINE(quit_lock, 0, K_SEM_MAX_LIMIT);
static struct net_if *my_iface;
static bool connected;

#define TRIGGER_TIMEOUT 5
static k_timeout_t trigger_timeout = K_SECONDS(TRIGGER_TIMEOUT);

void quit(void)
{
	k_sem_give(&quit_lock);
}

static void trigger_check(struct k_work *work)
{
	static int count;

	ARG_UNUSED(work);

	if (count >= CONFIG_NET_SAMPLE_ONLINE_COUNT) {
		quit();
	}

	count++;
}

#define EVENTS (NET_EVENT_CONNECTIVITY_ONLINE | NET_EVENT_CONNECTIVITY_OFFLINE)

static void event_handler(uint32_t mgmt_event, struct net_if *iface,
			  void *info, size_t info_length, void *user_data)
{
	if ((mgmt_event & EVENTS) != mgmt_event) {
		return;
	}

	if (mgmt_event == NET_EVENT_CONNECTIVITY_ONLINE) {
		char ifname[CONFIG_NET_INTERFACE_NAME_LEN];

		(void)net_if_get_name(iface, ifname, sizeof(ifname));

		LOG_INF("Connected online on interface %s (%p)",
			ifname, iface);
		connected = true;
		my_iface = iface;

		/* Wait a bit, and then trigger online check again */
		k_work_reschedule(&dwork, trigger_timeout);
		return;
	}

	if (mgmt_event == NET_EVENT_CONNECTIVITY_OFFLINE) {
		if (connected == false) {
			LOG_INF("Waiting network to get online.");
		} else {
			LOG_INF("Network no longer online");
			connected = false;
		}

		return;
	}
}

static NET_MGMT_REGISTER_EVENT_HANDLER(connectivity_events, EVENTS,
				       event_handler, NULL);

static int tls_setup_cb(struct net_if *iface,
			const sec_tag_t **sec_tag_list,
			size_t *sec_tag_size,
			const char **tls_hostname,
			const char *url,
			const char *host,
			const char *port,
			struct sockaddr *dst,
			void *user_data)
{
	if (!IS_ENABLED(CONFIG_NET_SOCKETS_SOCKOPT_TLS)) {
		return -ENOTSUP;
	}

	/* In this test app, we are only interested in the security tags */
	ARG_UNUSED(iface);
	ARG_UNUSED(url);
	ARG_UNUSED(host);
	ARG_UNUSED(port);
	ARG_UNUSED(dst);
	ARG_UNUSED(user_data);

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
	static sec_tag_t my_sec_tag_list[] = {
		CA_CERTIFICATE_TAG,
	};

	int ret = tls_credential_add(CA_CERTIFICATE_TAG,
				     TLS_CREDENTIAL_CA_CERTIFICATE,
				     ca_certificate,
				     sizeof(ca_certificate));
	if (ret < 0 && ret != -EEXIST) {
		LOG_ERR("Failed to register public certificate: %d", ret);
	}

	*sec_tag_list = my_sec_tag_list;
	*sec_tag_size = sizeof(my_sec_tag_list);

	/* This works only for this sample and not to be used in real life
	 * environment.
	 */
	*tls_hostname = "localhost";
#else
	ARG_UNUSED(sec_tag_list);
	ARG_UNUSED(sec_tag_size);
	ARG_UNUSED(tls_hostname);
#endif

	return 0;
}

int main(void)
{
	if (IS_ENABLED(CONFIG_NET_DHCPV4)) {
		net_dhcpv4_start(net_if_get_default());
	}

	/* Default strategy is PING so only set the HTTP one if enabled. */
	if (IS_ENABLED(CONFIG_NET_CONNECTION_MANAGER_ONLINE_CHECK_HTTP)) {
		conn_mgr_set_online_check_strategy(NET_CONN_MGR_ONLINE_CHECK_HTTP);
	}

	if (IS_ENABLED(CONFIG_NET_SOCKETS_SOCKOPT_TLS)) {
		int ret;

		ret = conn_mgr_register_online_checker_cb(tls_setup_cb, NULL);
		if (ret < 0) {
			LOG_WRN("Cannot enable TLS (%d)", ret);
		}
	}

	k_sem_take(&quit_lock, K_FOREVER);

	/* This print is for Docker based tester which monitors the output
	 * of this script and stops when it sees the "DONE!" string on a
	 * single line.
	 */
	printk("DONE!\n");

	return 0;
}
