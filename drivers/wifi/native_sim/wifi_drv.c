/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Simple wifi driver for native-sim board. It gets data from
 *        wpa_supplicant running in Zephyr and tries to do something
 *        with the data.
 */

#include <zephyr/kernel.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/logging/log.h>
#include <ethernet/eth_stats.h>

LOG_MODULE_REGISTER(wifi_native_sim, CONFIG_WIFI_LOG_LEVEL);

#include "internal.h"
#include "iface_api.h"
#include "mgmt_api.h"
#include "wifi_drv_api.h"

/* Supplicant callbacks we implement in the driver. */
static struct zep_wpa_supp_dev_ops wifi_drv_ops = {
	.init = wifi_drv_init,
	.deinit = wifi_drv_deinit,
	.scan2 = wifi_drv_scan2,
	.scan_abort = wifi_drv_scan_abort,
	.get_scan_results2 = wifi_drv_get_scan_results2,
	.deauthenticate = wifi_drv_deauthenticate,
	.authenticate = wifi_drv_authenticate,
	.associate = wifi_drv_associate,
	.set_key = wifi_drv_set_key,
	.set_supp_port = wifi_drv_set_supp_port,
	.signal_poll = wifi_drv_signal_poll,
	.send_mlme = wifi_drv_send_mlme,
	.get_wiphy = wifi_drv_get_wiphy,
	.register_frame = wifi_drv_register_frame,
	.get_capa = wifi_drv_get_capa,
	.get_conn_info = wifi_drv_get_conn_info,
};

/* Management operations called by wifi shell. These are typically
 * routed via module/hostap glue code which then calls these driver
 * installed management callbacks.
 */
static struct wifi_mgmt_ops wifi_mgmt_ops = {
	.scan = wifi_scan,
	.connect = wifi_connect,
	.disconnect = wifi_disconnect,
#ifdef CONFIG_NET_STATISTICS_WIFI
	.get_stats = wifi_get_stats,
#endif /* CONFIG_NET_STATISTICS_WIFI */
	.set_power_save = wifi_set_power_save,
	.set_twt = wifi_set_twt,
	.reg_domain = wifi_reg_domain,
	.get_power_save_config = wifi_get_power_save_config,
	.iface_status = wifi_iface_status,
	.filter = wifi_filter,
	.mode = wifi_mode,
	.channel = wifi_channel,
};

/* Normal network interface API */
static const struct net_wifi_mgmt_offload wifi_if_api = {
	.wifi_iface.iface_api.init = wifi_if_init,
	.wifi_iface.start = wifi_if_start,
	.wifi_iface.stop = wifi_if_stop,
	.wifi_iface.set_config = wifi_if_set_config,
	.wifi_iface.get_capabilities = wifi_if_caps_get,
	.wifi_iface.send = wifi_if_send,
#ifdef CONFIG_NET_STATISTICS_ETHERNET
	.wifi_iface.get_stats = wifi_if_stats_get,
#endif /* CONFIG_NET_STATISTICS_ETHERNET */
	.wifi_mgmt_api = &wifi_mgmt_ops,
	.wifi_drv_ops = &wifi_drv_ops,
};

#define DEFINE_RX_THREAD(x, _)						\
	K_KERNEL_STACK_DEFINE(rx_thread_stack_##x,			\
			      CONFIG_ARCH_POSIX_RECOMMENDED_STACK_SIZE);\
	static struct k_thread rx_thread_data_##x

LISTIFY(CONFIG_WIFI_NATIVE_SIM_INTERFACE_COUNT, DEFINE_RX_THREAD, (;), _);

#define DEFINE_WIFI_DEV_DATA(x, _)					     \
	static struct wifi_context wifi_context_data_##x = {		     \
		.if_name_host = CONFIG_WIFI_NATIVE_SIM_DRV_NAME #x,	     \
		.rx_thread = &rx_thread_data_##x,			     \
		.rx_stack = rx_thread_stack_##x,			     \
		.rx_stack_size = K_KERNEL_STACK_SIZEOF(rx_thread_stack_##x), \
	}

LISTIFY(CONFIG_WIFI_NATIVE_SIM_INTERFACE_COUNT, DEFINE_WIFI_DEV_DATA, (;), _);

#define DEFINE_WIFI_DEVICE(x, _)					\
	ETH_NET_DEVICE_INIT(wifi_native_sim_##x,			\
			    CONFIG_WIFI_NATIVE_SIM_DRV_NAME #x,		\
			    NULL, NULL, &wifi_context_data_##x, NULL,	\
			    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,	\
			    &wifi_if_api,				\
			    NET_ETH_MTU)

LISTIFY(CONFIG_WIFI_NATIVE_SIM_INTERFACE_COUNT, DEFINE_WIFI_DEVICE, (;), _);
