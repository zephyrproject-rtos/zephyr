/* main.c - Application main entry point */

/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define NET_LOG_LEVEL CONFIG_NET_L2_ETHERNET_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, NET_LOG_LEVEL);

#include <zephyr/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/linker/sections.h>
#include <zephyr/random/random.h>

#include <zephyr/ztest.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/wifi_nm.h>

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
#define DBG(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define DBG(fmt, ...)
#endif

struct wifi_drv_context {
	struct net_if *iface;
	uint8_t mac_addr[6];
	enum ethernet_if_types eth_if_type;
};

static struct wifi_drv_context wifi_context;

bool wifi_nm_op_called;
bool wifi_offload_op_called;

static void wifi_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct wifi_drv_context *context = dev->data;
	struct ethernet_context *eth_ctx = net_if_l2_data(iface);

	net_if_set_link_addr(iface, context->mac_addr,
			     sizeof(context->mac_addr),
			     NET_LINK_ETHERNET);

	eth_ctx->eth_if_type = L2_ETH_IF_TYPE_WIFI;

	ethernet_init(iface);
}

static int wifi_scan(const struct device *dev, struct wifi_scan_params *params,
		     scan_result_cb_t cb)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(params);
	ARG_UNUSED(cb);

	wifi_offload_op_called = true;

	return 0;
}

static struct wifi_mgmt_ops wifi_mgmt_api = {
	.scan		= wifi_scan,
};

static struct net_wifi_mgmt_offload api_funcs = {
	.wifi_iface.iface_api.init = wifi_iface_init,
	.wifi_mgmt_api = &wifi_mgmt_api,
};

static void generate_mac(uint8_t *mac_addr)
{
	/* 00-00-5E-00-53-xx Documentation RFC 7042 */
	mac_addr[0] = 0x00;
	mac_addr[1] = 0x00;
	mac_addr[2] = 0x5E;
	mac_addr[3] = 0x00;
	mac_addr[4] = 0x53;
	mac_addr[5] = sys_rand8_get();
}

static int wifi_init(const struct device *dev)
{
	struct wifi_drv_context *context = dev->data;

	context->eth_if_type = L2_ETH_IF_TYPE_WIFI;

	generate_mac(context->mac_addr);

	return 0;
}

ETH_NET_DEVICE_INIT(wlan0, "wifi_test",
		    wifi_init, NULL,
		    &wifi_context, NULL, CONFIG_ETH_INIT_PRIORITY,
		    &api_funcs, NET_ETH_MTU);

static int wifi_nm_scan(const struct device *dev, struct wifi_scan_params *params,
			scan_result_cb_t cb)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(params);
	ARG_UNUSED(cb);

	wifi_nm_op_called = true;

	return 0;
}

static struct wifi_mgmt_ops wifi_nm_test_ops = {
	.scan		= wifi_nm_scan,
};

DEFINE_WIFI_NM_INSTANCE(test, &wifi_nm_test_ops);

static int request_scan(void)
{
	struct net_if *iface = net_if_get_first_wifi();

	if (net_mgmt(NET_REQUEST_WIFI_SCAN, iface, NULL, 0)) {
		printk("Scan request failed\n");

		return -ENOEXEC;
	}

	return 0;
}

ZTEST(net_wifi, test_wifi_offload)
{

	int ret;
#ifdef CONFIG_WIFI_NM
	struct wifi_nm_instance *nm = wifi_nm_get_instance("test");

	if (wifi_nm_get_instance_iface(net_if_get_first_wifi())) {
		ret = wifi_nm_unregister_mgd_iface(nm, net_if_get_first_wifi());
		zassert_equal(ret, 0, "Failed to unregister managed interface");
	}
#endif /* CONFIG_WIFI_NM */

	ret = request_scan();
	zassert_equal(ret, 0, "Scan request failed");
	zassert_true(wifi_offload_op_called, "Scan callback not called");
}

ZTEST(net_wifi, test_wifi_nm_managed)
{

	int ret;
	struct wifi_nm_instance *nm = wifi_nm_get_instance("test");

	zassert_equal(nm->ops, &wifi_nm_test_ops,
		      "Invalid wifi nm ops");

	/* Offload: in presence of registered NM but with no managed
	 *          interfaces.
	 */
	ret = request_scan();
	zassert_equal(ret, 0, "Scan request failed");
	zassert_true(wifi_offload_op_called, "Scan callback not called");

	ret = wifi_nm_register_mgd_iface(nm, net_if_get_first_wifi());
	zassert_equal(ret, 0, "Failed to register managed interface");

	zassert_equal(nm->ops, &wifi_nm_test_ops,
		      "Invalid wifi nm ops");

	ret = request_scan();
	zassert_equal(ret, 0, "Scan request failed");
	zassert_true(wifi_nm_op_called, "Scan callback not called");
}


ZTEST_SUITE(net_wifi, NULL, NULL, NULL, NULL, NULL);
