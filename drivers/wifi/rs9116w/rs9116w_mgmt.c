/*
 * Copyright (c) 2022 T-Mobile USA, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#define LOG_MODULE_NAME wifi_rs9116w_mgmt
#define LOG_LEVEL	CONFIG_WIFI_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#define DT_DRV_COMPAT silabs_rs9116w
#include "rs9116w.h"
#include "rsi_bootup_config.h"
#include "rsi_common_apis.h"
#include "rsi_wlan.h"
#include "rsi_wlan_apis.h"

#include <errno.h>

#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>

#include <zephyr/debug/stack.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/net/net_context.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_l2.h>
#include <zephyr/net/net_offload.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/kernel.h>

#undef s6_addr
#undef s6_addr32

#define RSI_OPERMODE_WLAN_BLE 13

#define RS9116W_MAX_IFACES 1

struct rs9116w_device s_rs9116w_dev[RS9116W_MAX_IFACES] = {0};

/* rsi_wlan_get_state() is not defined in WiseConnect, so define it here */
uint8_t rsi_wlan_get_state(void);

struct rs9116w_config {
	struct spi_dt_spec spi;
};

static struct rs9116w_config rs9116w_conf = {
	.spi = SPI_DT_SPEC_INST_GET(0, SPI_OP_MODE_MASTER | SPI_WORD_SET(8),
				    2)};

#define GLOBAL_BUFF_LEN 10000
uint8_t global_buf[GLOBAL_BUFF_LEN];

struct rs9116w_device *rs9116w_by_iface_idx(uint8_t iface_idx)
{
	return &s_rs9116w_dev[0];
}

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? a : b)
#endif

/*  mgmt functions */

/****************************************************************************/
static int rs9116w_mgmt_scan(const struct device *dev,
				struct wifi_scan_params *params,
				scan_result_cb_t cb)
{
	int ret;
	ARG_UNUSED(params);
	struct rs9116w_device *rs9116w_dev = dev->data;
	bool connected = (rsi_wlan_get_state() >= RSI_WLAN_STATE_CONNECTED);

	rsi_wlan_req_radio(1);

	if (!connected) {
		ret = rsi_wlan_scan(NULL, 0, &(rs9116w_dev->scan_results),
			    sizeof(rsi_rsp_scan_t));
	} else {
		ret = rsi_wlan_bgscan_profile(1, &(rs9116w_dev->scan_results),
			    sizeof(rsi_rsp_scan_t));
	}

	if (ret) {
		LOG_ERR("rsi_wlan_scan error: %d", ret);
		return -EIO;
	}

	printk("rsi_wlan_scan returned %d APs\n",
	       rs9116w_dev->scan_results.scan_count[0]);

	for (int i = 0; i < rs9116w_dev->scan_results.scan_count[0]; i++) {

		rsi_scan_info_t *r_result =
			&rs9116w_dev->scan_results.scan_info[i];
		struct wifi_scan_result z_result;

		/* convert index i to result
		 *
		 * SSID
		 * RSI:    #define RSI_SSID_LEN 34
		 * ZEPHYR: #define WIFI_SSID_MAX_LEN 32
		 */
		memcpy(z_result.ssid, r_result->ssid,
		       MIN(RSI_SSID_LEN, WIFI_SSID_MAX_LEN));
		z_result.ssid[MIN(RSI_SSID_LEN, WIFI_SSID_MAX_LEN) - 1] = '\0';

		/*  SSID length */
		z_result.ssid_length = strlen(z_result.ssid);

		/*  channel */
		z_result.channel = r_result->rf_channel;

		/* Security modes supported by RS9116W
		 * (from WiseConnect sapi/include/rsi_wlan_apis.h)
		 * typedef enum rsi_security_mode_e {
		 * RSI_OPEN = 0,             // open mode
		 * RSI_WPA,                  // WPA security with PSK
		 * RSI_WPA2,                 // WPA2 security with PSK
		 * RSI_WEP,                  // WEP security
		 * RSI_WPA_EAP,              // Enterprise WPA security
		 * RSI_WPA2_EAP,             // Enterprise WPA2 security
		 * RSI_WPA_WPA2_MIXED,       // Enterprise WPA2/WPA security
		 * RSI_WPA_PMK,              // WPA security with PMK
		 * RSI_WPA2_PMK,             // WPA2 security with PMK
		 * RSI_WPS_PIN,              // WPS pin method
		 * RSI_USE_GENERATED_WPSPIN, // WPS generated pin method
		 * RSI_WPS_PUSH_BUTTON,      // WPS push button method
		 * } rsi_security_mode_t;
		 */

		if (r_result->security_mode == RSI_OPEN) {
			z_result.security = WIFI_SECURITY_TYPE_NONE;
		} else if (r_result->security_mode == RSI_WPA2) {
			z_result.security = WIFI_SECURITY_TYPE_PSK;
		} else if (r_result->security_mode == RSI_WPA3
			|| r_result->security_mode == RSI_WPA3_TRANSITION) {
			z_result.security = WIFI_SECURITY_TYPE_SAE;
		} else {
			LOG_WRN("SSID: %s with security %u not supported",
				z_result.ssid, r_result->security_mode);
		}

		/* rssi */
		z_result.rssi = -r_result->rssi_val;

		/* mac/bssid */
		memcpy(z_result.mac, r_result->bssid, 6);
		z_result.mac_length = 6;

		/* Inform Zephyr about the AP */
		cb(rs9116w_dev->net_iface, 0, &z_result);
	}
	/* Inform Zephyr there are no more APs,
	 * should generate a SCAN COMPLETE message
	 */
	cb(rs9116w_dev->net_iface, 0, NULL);

	if (!connected) {
		LOG_DBG("Reinitializing WLAN radio");
		rsi_wlan_req_radio(0);
	} else {
		ret = rsi_wlan_bgscan_profile(0, &(rs9116w_dev->scan_results),
			    sizeof(rsi_rsp_scan_t));
	}

	return ret;
}
#define Z_PF_INET  1	      /**< IP protocol family version 4. */
#define Z_PF_INET6 2	      /**< IP protocol family version 6. */
#define Z_AF_INET  Z_PF_INET  /**< IP protocol family version 4. */
#define Z_AF_INET6 Z_PF_INET6 /**< IP protocol family version 6. */

/****************************************************************************/
static int rs9116w_mgmt_connect(const struct device *dev,
				struct wifi_connect_req_params *params)
{
	/* RSI security modes:
	 * 0: RSI_OPEN,
	 * 1: RSI_WPA,
	 * 2: RSI_WPA2,
	 * 3: RSI_WEP,
	 * 4: RSI_WPA_EAP,
	 * 5: RSI_WPA2_EAP,
	 * 6: RSI_WPA_WPA2_MIXED,
	 * 7: RSI_WPA_PMK,
	 * 8: RSI_WPA2_PMK,
	 * 9: RSI_WPS_PIN,
	 * 10: RSI_USE_GENERATED_WPSPIN,
	 * 11: RSI_WPS_PUSH_BUTTON
	 *
	 * Zephyr security modes
	 * 0: WIFI_SECURITY_TYPE_NONE,
	 * 1: WIFI_SECURITY_TYPE_PSK,
	 */

	struct rs9116w_device *rs9116w_dev = dev->data;
	rsi_security_mode_t rsi_security;
	void *rsi_psk;
	int ret;

	rsi_wlan_req_radio(1);

	/* Connect to an access point */
	if (params->security == WIFI_SECURITY_TYPE_NONE) {
		rsi_security = RSI_OPEN;
		rsi_psk = NULL;
	} else if (params->security == WIFI_SECURITY_TYPE_PSK) {
		rsi_security = RSI_WPA2;
		rsi_psk = params->psk;
	} else {
		return -EINVAL;
	}

	if (rsi_wlan_get_state() >= RSI_WLAN_STATE_CONNECTED) {
		return -EALREADY;
	}

	ret = rsi_wlan_connect((int8_t *)params->ssid, rsi_security, rsi_psk);

	wifi_mgmt_raise_connect_result_event(rs9116w_dev->net_iface, ret);

	if (ret) {
		LOG_ERR("rsi_wlan_connect error: %d", ret);
		return -EIO;
	}
#if IS_ENABLED(CONFIG_NET_IPV4)
	struct in_addr addr;
	uint8_t ipv4_mode = RSI_DHCP;
	uint8_t *ipv4_addr = NULL, *ipv4_mask = NULL, *ipv4_gw = NULL;
#if defined(CONFIG_NET_CONFIG_MY_IPV4_ADDR)
	uint32_t ipv4_addr_d = 0, ipv4_mask_d = 0, ipv4_gw_d = 0;

	if (strcmp(CONFIG_NET_CONFIG_MY_IPV4_ADDR, "") != 0) {
		ipv4_mode = RSI_STATIC;
		if (net_addr_pton(Z_AF_INET, CONFIG_NET_CONFIG_MY_IPV4_ADDR,
				  &addr) < 0) {
			LOG_ERR("Invalid CONFIG_NET_CONFIG_MY_IPV4_ADDR");
			return -1;
		}
		ipv4_addr_d = addr.s_addr;
		ipv4_addr = (uint8_t *)&ipv4_addr_d;
	}

#if defined(CONFIG_NET_CONFIG_MY_IPV4_GW)
	if (strcmp(CONFIG_NET_CONFIG_MY_IPV4_GW, "") != 0) {
		if (net_addr_pton(Z_AF_INET, CONFIG_NET_CONFIG_MY_IPV4_GW,
				  &addr) < 0) {
			LOG_ERR("Invalid CONFIG_NET_CONFIG_MY_IPV4_GW");
			return -1;
		}
		ipv4_gw_d = addr.s_addr;
		ipv4_gw = (uint8_t *)&ipv4_gw_d;
	}
#endif

#if defined(CONFIG_NET_CONFIG_MY_IPV4_NETMASK)
	if (strcmp(CONFIG_NET_CONFIG_MY_IPV4_NETMASK, "") != 0) {
		if (net_addr_pton(Z_AF_INET, CONFIG_NET_CONFIG_MY_IPV4_NETMASK,
				  &addr) < 0) {
			LOG_ERR("Invalid CONFIG_NET_CONFIG_MY_IPV4_NETMASK");
			return -1;
		}
		ipv4_mask_d = addr.s_addr;
		ipv4_mask = (uint8_t *)&ipv4_mask_d;
	}
#endif
#endif
	/* Configure IPv4 (DHCP) */
	rsi_rsp_ipv4_parmas_t rsi_rsp_ipv4_parmas;

	ret = rsi_config_ipaddress(RSI_IP_VERSION_4, ipv4_mode, ipv4_addr,
				   ipv4_mask, ipv4_gw,
				   (uint8_t *)&rsi_rsp_ipv4_parmas,
				   sizeof(rsi_rsp_ipv4_parmas), 0);
	if (ret != 0) {
		rs9116w_dev->has_ipv4 = false;
		LOG_WRN("ipv4: rsi_config_ipaddress error: %d", ret);
		ret = 0;
	} else {
		rs9116w_dev->has_ipv4 = true;
	}

	memcpy(addr.s4_addr, rsi_rsp_ipv4_parmas.gateway, 4);
#if IS_ENABLED(CONFIG_NET_NATIVE_IPV4)
	net_if_ipv4_set_gw(rs9116w_dev->net_iface, &addr);
#endif

	memcpy(addr.s4_addr, rsi_rsp_ipv4_parmas.netmask, 4);
#if IS_ENABLED(CONFIG_NET_NATIVE_IPV4)
	net_if_ipv4_set_netmask(rs9116w_dev->net_iface, &addr);
#endif
	memcpy(addr.s4_addr, rsi_rsp_ipv4_parmas.ipaddr, 4);

	LOG_DBG("ip = %d.%d.%d.%d", addr.s4_addr[0], addr.s4_addr[1],
		addr.s4_addr[2], addr.s4_addr[3]);

#if IS_ENABLED(CONFIG_NET_NATIVE_IPV4)
	net_if_ipv4_addr_add(rs9116w_dev->net_iface, &addr, NET_ADDR_DHCP, 0);
#endif
#endif
#if IS_ENABLED(CONFIG_NET_IPV6)
	struct in6_addr addr6;
	uint8_t ipv6_mode = RSI_DHCP;
	uint8_t *ipv6_addr = NULL;
#if defined(CONFIG_NET_CONFIG_MY_IPV6_ADDR)
	if (strcmp(CONFIG_NET_CONFIG_MY_IPV6_ADDR, "") != 0) {
		ipv6_mode = RSI_STATIC;
		if (net_addr_pton(Z_AF_INET6, CONFIG_NET_CONFIG_MY_IPV6_ADDR,
				  &addr6) < 0) {
			LOG_ERR("Invalid CONFIG_NET_CONFIG_MY_IPV6_ADDR");
			return -1;
		}
		ipv6_addr = addr6.s6_addr;
	}
#endif
	rsi_rsp_ipv6_parmas_t rsi_rsp_ipv6_parmas;

	ret = rsi_config_ipaddress(RSI_IP_VERSION_6, ipv6_mode, ipv6_addr, NULL,
				   NULL, (uint8_t *)&rsi_rsp_ipv6_parmas,
				   sizeof(rsi_rsp_ipv6_parmas), 0);
	if (ret != 0) {
		LOG_WRN("ipv6: rsi_config_ipaddress error: %x", ret);
		ret = 0;
		rs9116w_dev->has_ipv6 = false;
	} else {
		rs9116w_dev->has_ipv6 = true;
	}

	memcpy(addr6.s6_addr, rsi_rsp_ipv6_parmas.ipaddr6, 16);
	addr6.s6_addr32[0] = sys_cpu_to_be32(addr6.s6_addr32[0]);
	addr6.s6_addr32[1] = sys_cpu_to_be32(addr6.s6_addr32[1]);
	addr6.s6_addr32[2] = sys_cpu_to_be32(addr6.s6_addr32[2]);
	addr6.s6_addr32[3] = sys_cpu_to_be32(addr6.s6_addr32[3]);

	LOG_DBG("ip6 = %04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x",
		addr6.s6_addr16[1], addr6.s6_addr16[0], addr6.s6_addr16[3],
		addr6.s6_addr16[2], addr6.s6_addr16[5], addr6.s6_addr16[4],
		addr6.s6_addr16[7], addr6.s6_addr16[6]);
#if IS_ENABLED(CONFIG_NET_NATIVE_IPV6)
	net_if_ipv6_addr_add(rs9116w_dev->net_iface, &addr6, NET_ADDR_DHCP, 0);
#endif
#endif

	LOG_DBG("Connected!");

	net_if_up(rs9116w_dev->net_iface);

	return ret;
}

/****************************************************************************/
static int rs9116w_mgmt_disconnect(const struct device *dev)
{
	int ret;
	struct rs9116w_device *rs9116w_dev = dev->data;

	if (rsi_wlan_get_state() < RSI_WLAN_STATE_CONNECTED) {
		return -EALREADY;
	}

	rsi_wlan_req_radio(1);
	ret = rsi_wlan_disconnect();
	wifi_mgmt_raise_disconnect_result_event(rs9116w_dev->net_iface, ret);

	net_if_down(rs9116w_dev->net_iface);
	if (ret) {
		LOG_ERR("rsi_wlan_disconnect error: %d", ret);
		return -EIO;
	}

	LOG_DBG("Deinitializing WLAN radio");
	rsi_wlan_req_radio(0);

	return 0;
}

/* called after device init fcn */
static void rs9116w_iface_init(struct net_if *iface)
{
	s_rs9116w_dev[0].net_iface = iface;

	net_if_set_link_addr(iface, s_rs9116w_dev[0].mac,
			     sizeof(s_rs9116w_dev[0].mac), NET_LINK_ETHERNET);
	net_if_flag_set(iface, NET_IF_NO_AUTO_START);

	/* Initialize the offload engine */
	rs9116w_offload_init(&s_rs9116w_dev[0]);
}

#if IS_ENABLED(CONFIG_WISECONNECT_USE_OS_BINDINGS)
K_THREAD_STACK_DEFINE(driver_task_stack, 2048);
struct k_thread driver_task;

void driver_task_entry(void *p1, void *p2, void *p3)
{
	rsi_wireless_driver_task();
}
#endif

/* offload device init fcn (called before rs9116w_iface_init) */
static int rs9116w_init(const struct device *dev)
{
	int32_t status;
	struct rs9116w_device *rs9116w_dev = dev->data;
	uint8_t mac[6];

	/* Initialize SPI bus
	 *
	 * config
	 * TBD should these constant values be configuration settings?
	 * rs9116w_dev->spi = SPI_DT_SPEC_INST_GET(0, SPI_OP_MODE_MASTER |
	 * SPI_WORD_SET(8), 2);
	 */
	rs9116w_dev->spi = rs9116w_conf.spi;

	if (!spi_is_ready_dt(&rs9116w_dev->spi)) {
		LOG_ERR("spi bus %s not ready", rs9116w_dev->spi.bus->name);
		return -ENODEV;
	}

	/* Initialize the RSI driver */
	status = rsi_driver_init(global_buf, GLOBAL_BUFF_LEN);
	LOG_DBG("%s: rsi_driver_init returned %d", __func__, status);

	if (status >= 0 && status <= GLOBAL_BUFF_LEN) {
		LOG_DBG("%s: rsi_driver_init using %d of %d bytes", __func__,
			status, GLOBAL_BUFF_LEN);
	} else if (status > GLOBAL_BUFF_LEN) {
		LOG_ERR("%s: rsi_driver_init error: not enough "
			"memory, driver needs %d",
			__func__, status);
		return -ENOMEM;
	} else if (status < 0) {
		LOG_ERR("%s: rsi_driver_init error: %d", __func__, status);
		return status;
	}

	/* Initialize the device */
	status = rsi_device_init(LOAD_NWP_FW);

	if (status) {
		LOG_ERR("%s: rsi_device_init returned %d", __func__, status);
		return status;
	}

#if IS_ENABLED(CONFIG_WISECONNECT_USE_OS_BINDINGS)
	k_thread_create(&driver_task, driver_task_stack,
			K_THREAD_STACK_SIZEOF(driver_task_stack),
			driver_task_entry, NULL, NULL, NULL, K_PRIO_COOP(8),
			K_INHERIT_PERMS, K_NO_WAIT);
#endif

	/* Initialize WiseConnect features (Configure simultaneous WiFi and BLE)
	 */
	if (!status) {
		status = rsi_wireless_init(RSI_WLAN_CLIENT_MODE,
					   RSI_OPERMODE_WLAN_BLE);
		LOG_DBG("%s: rsi_wireless_init returned %d", __func__, status);
	}

	rsi_wlan_radio_init();

	/* Get the MAC (must be after rsi_wlan_radio_init()) */
	status = rsi_wlan_get(RSI_MAC_ADDRESS, mac, sizeof(mac));
	LOG_INF("rsi_wlan_get after rsi_wireless_init returned %d, mac: "
		"%02x:%02x:%02x:%02x:%02x:%02x",
		status, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	memcpy(rs9116w_dev->mac, mac, sizeof(mac));

	if (status)
		LOG_ERR("RS9116W WiFi driver failed to initialize, status = %d",
			status);
	else
		LOG_INF("RS9116W WiFi driver initialized");

	/* Get the FW version */
	status = rsi_get_fw_version(rs9116w_dev->fw_version,
				    sizeof(rs9116w_dev->fw_version));
	if (status != 0)
		LOG_DBG("rsi_get_fw_version returned %d", status);
	else {
		LOG_DBG("FW version: %s", rs9116w_dev->fw_version);
	}

	int state = rsi_wlan_get_state();

	LOG_DBG("rsi_wlan state: %d", state);
	/*Don't know that this is necessary, but it doesn't hurt*/
	status = rsi_send_feature_frame();

	LOG_DBG("Feature frame status: %d", status);
	return 0;
}

static enum offloaded_net_if_types rs9116w_get_type(void)
{
	return L2_OFFLOADED_NET_IF_TYPE_WIFI;
}

static const struct wifi_mgmt_ops rs9116w_mgmt = {
	.scan		= rs9116w_mgmt_scan,
	.connect	= rs9116w_mgmt_connect,
	.disconnect	= rs9116w_mgmt_disconnect,
};

static const struct net_wifi_mgmt_offload rs9116w_api = {
	.wifi_iface.iface_api.init = rs9116w_iface_init,
	.wifi_iface.get_type = rs9116w_get_type,
	.wifi_mgmt_api = &rs9116w_mgmt,
};

#ifdef CONFIG_PM_DEVICE
#if CONFIG_WISECONNECT_BT && CONFIG_BT_RS9116W
extern int bt_le_adv_stop(void);
extern void bt_le_adv_resume(void);
#include <rsi_bt_common_apis.h>
#endif
static int rs9116w_pm_action(const struct device *dev,
			     enum pm_device_action action)
{
	int ret = 0;

	if (!k_can_yield()) {
		LOG_ERR("Blocking actions cannot run in this context");
		return -ENOTSUP;
	}

	ret = rsi_wlan_power_save_profile(RSI_ACTIVE, RSI_MAX_PSP);
	if (ret) {
		return ret;
	}
	k_msleep(100);
	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = rsi_wlan_power_save_profile(RSI_ACTIVE, RSI_MAX_PSP);
		k_msleep(50);
#if CONFIG_WISECONNECT_BT && CONFIG_BT_RS9116W
		bt_le_adv_resume();
		if (!ret) {
			ret = rsi_bt_power_save_profile(RSI_ACTIVE,
							RSI_MAX_PSP);
		}
#endif
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		/* Todo: Add reinitialization logic */
		ret = rsi_wlan_power_save_profile(RSI_SLEEP_MODE_8,
						  RSI_MAX_PSP);
		k_msleep(50);
#if CONFIG_WISECONNECT_BT && CONFIG_BT_RS9116W
		bt_le_adv_stop();
		if (!ret) {
			ret = rsi_bt_power_save_profile(RSI_SLEEP_MODE_8,
							RSI_MAX_PSP);
		}
#endif
		break;
	case PM_DEVICE_ACTION_TURN_OFF:
		ret = rsi_wlan_power_save_profile(RSI_SLEEP_MODE_10,
						  RSI_MAX_PSP);
		k_msleep(50);
#if CONFIG_WISECONNECT_BT && CONFIG_BT_RS9116W
		bt_le_adv_stop();
		if (!ret) {
			ret = rsi_bt_power_save_profile(RSI_SLEEP_MODE_10,
							RSI_MAX_PSP);
		}
#endif
		pm_device_state_lock(dev);
		break;
	default:
		return -ENOTSUP;
	}

	return ret;
}
#endif /* CONFIG_PM_DEVICE */

PM_DEVICE_DT_INST_DEFINE(0, rs9116w_pm_action);

NET_DEVICE_DT_INST_OFFLOAD_DEFINE(
	0,			   /* instance number */
	rs9116w_init,		   /* offload device init fcn */
	PM_DEVICE_DT_INST_GET(0),  /* pm_control function? */
	&s_rs9116w_dev,		   /* private data */
	&rs9116w_conf,		   /* config info */
	CONFIG_WIFI_INIT_PRIORITY, /* priority */
	&rs9116w_api,		   /* api fcn table */
	MAX_PER_PACKET_SIZE);	   /* MTU */
