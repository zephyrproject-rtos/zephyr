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

static struct wifi_mgmt_ops wifi_mgmt_ops = {
	.scan = wifi_scan,
#ifdef CONFIG_NET_STATISTICS_WIFI
	.get_stats = wifi_get_stats,
#endif /* CONFIG_NET_STATISTICS_WIFI */
	.set_power_save = wifi_set_power_save,
	.set_twt = wifi_set_twt,
	.reg_domain = wifi_reg_domain,
	.get_power_save_config = wifi_get_power_save_config,
};

/* TODO: replace the net_wifi_mgmt_offload with something more appropriate
 *       as this is not an offloaded driver.
 */
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
			    NULL, NULL,	&wifi_context_data_##x, NULL,	\
			    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,	\
			    &wifi_if_api,				\
			    NET_ETH_MTU)

LISTIFY(CONFIG_WIFI_NATIVE_SIM_INTERFACE_COUNT, DEFINE_WIFI_DEVICE, (;), _);
