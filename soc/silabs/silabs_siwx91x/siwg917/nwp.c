/**
 * @file
 * @brief Network Processor Initialization for SiWx91x.
 *
 * This file contains the initialization routine for the (ThreadArch) network processor
 * on the SiWx91x platform. The component is responsible for setting up the necessary
 * hardware and software components to enable network communication.
 *
 * Copyright (c) 2025 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/net/wifi.h>
#include <zephyr/logging/log.h>

#include "nwp.h"
#include "sl_wifi_callback_framework.h"
#ifdef CONFIG_BT_SILABS_SIWX91X
#include "rsi_ble_common_config.h"
#endif

#define AP_MAX_NUM_STA 4

LOG_MODULE_REGISTER(siwx91x_nwp);

BUILD_ASSERT(DT_REG_SIZE(DT_CHOSEN(zephyr_sram)) == KB(195) ||
	     DT_REG_SIZE(DT_CHOSEN(zephyr_sram)) == KB(255) ||
	     DT_REG_SIZE(DT_CHOSEN(zephyr_sram)) == KB(319));

static void siwx91x_apply_sram_config(sl_si91x_boot_configuration_t *boot_config)
{
	/* The size does not match exactly because 1 KB is reserved at the start of the RAM */
	size_t sram_size = DT_REG_SIZE(DT_CHOSEN(zephyr_sram));

	if (sram_size == KB(195)) {
		boot_config->ext_custom_feature_bit_map |= SL_SI91X_EXT_FEAT_480K_M4SS_192K;
	} else if (sram_size == KB(255)) {
		boot_config->ext_custom_feature_bit_map |= SL_SI91X_EXT_FEAT_416K_M4SS_256K;
	} else if (sram_size == KB(319)) {
		boot_config->ext_custom_feature_bit_map |= SL_SI91X_EXT_FEAT_352K_M4SS_320K;
	} else {
		k_panic();
	}
}

static void siwx91x_configure_sta_mode(sl_si91x_boot_configuration_t *boot_config)
{
	const bool wifi_enabled = IS_ENABLED(CONFIG_WIFI_SILABS_SIWX91X);
	const bool bt_enabled = IS_ENABLED(CONFIG_BT_SILABS_SIWX91X);

	boot_config->oper_mode = SL_SI91X_CLIENT_MODE;

	if (IS_ENABLED(CONFIG_WIFI_SILABS_SIWX91X_ROAMING_USE_DEAUTH)) {
		boot_config->custom_feature_bit_map |=
			SL_SI91X_CUSTOM_FEAT_ROAM_WITH_DEAUTH_OR_NULL_DATA;
	}

	if (wifi_enabled && bt_enabled) {
		boot_config->coex_mode = SL_SI91X_WLAN_BLE_MODE;
	} else if (wifi_enabled) {
		boot_config->coex_mode = SL_SI91X_WLAN_ONLY_MODE;
	} else {
		/*
		 * Even if neither WiFi or BLE is used we have to specify a Coex mode
		 */
		boot_config->coex_mode = SL_SI91X_BLE_MODE;
	}

#ifdef CONFIG_WIFI_SILABS_SIWX91X
	boot_config->ext_tcp_ip_feature_bit_map = SL_SI91X_CONFIG_FEAT_EXTENSION_VALID;
	boot_config->ext_custom_feature_bit_map |= SL_SI91X_EXT_FEAT_IEEE_80211W |
			SL_SI91X_EXT_FEAT_FRONT_END_SWITCH_PINS_ULP_GPIO_4_5_0;
	if (IS_ENABLED(CONFIG_WIFI_SILABS_SIWX91X_ENHANCED_MAX_PSP)) {
		boot_config->config_feature_bit_map = SL_SI91X_ENABLE_ENHANCED_MAX_PSP;
	}
#endif

#ifdef CONFIG_BT_SILABS_SIWX91X
	boot_config->ext_custom_feature_bit_map |= SL_SI91X_EXT_FEAT_BT_CUSTOM_FEAT_ENABLE;
	boot_config->bt_feature_bit_map |= SL_SI91X_BT_RF_TYPE | SL_SI91X_ENABLE_BLE_PROTOCOL;
	boot_config->ble_feature_bit_map |=
			SL_SI91X_BLE_MAX_NBR_PERIPHERALS(RSI_BLE_MAX_NBR_PERIPHERALS) |
			SL_SI91X_BLE_MAX_NBR_CENTRALS(RSI_BLE_MAX_NBR_CENTRALS) |
			SL_SI91X_BLE_MAX_NBR_ATT_SERV(RSI_BLE_MAX_NBR_ATT_SERV) |
			SL_SI91X_BLE_MAX_NBR_ATT_REC(RSI_BLE_MAX_NBR_ATT_REC) |
			SL_SI91X_BLE_PWR_INX(RSI_BLE_PWR_INX) |
			SL_SI91X_BLE_PWR_SAVE_OPTIONS(RSI_BLE_PWR_SAVE_OPTIONS) |
			SL_SI91X_916_BLE_COMPATIBLE_FEAT_ENABLE |
			SL_SI91X_FEAT_BLE_CUSTOM_FEAT_EXTENSION_VALID;

	boot_config->ble_ext_feature_bit_map |=
			SL_SI91X_BLE_NUM_CONN_EVENTS(RSI_BLE_NUM_CONN_EVENTS) |
			SL_SI91X_BLE_NUM_REC_BYTES(RSI_BLE_NUM_REC_BYTES) |
			SL_SI91X_BLE_ENABLE_ADV_EXTN |
			SL_SI91X_BLE_AE_MAX_ADV_SETS(RSI_BLE_AE_MAX_ADV_SETS);
#endif
}

static void siwx91x_configure_ap_mode(sl_si91x_boot_configuration_t *boot_config,
				      bool hidden_ssid, uint8_t max_num_sta)
{
	boot_config->oper_mode = SL_SI91X_ACCESS_POINT_MODE;
	boot_config->coex_mode = SL_SI91X_WLAN_ONLY_MODE;

	if (IS_ENABLED(CONFIG_WIFI_SILABS_SIWX91X_LIMIT_PACKET_BUF_PER_STA)) {
		boot_config->custom_feature_bit_map |= SL_SI91X_CUSTOM_FEAT_LIMIT_PACKETS_PER_STA;
	}

	if (hidden_ssid) {
		boot_config->custom_feature_bit_map |= SL_SI91X_CUSTOM_FEAT_AP_IN_HIDDEN_MODE;
	}

	boot_config->custom_feature_bit_map |= SL_WIFI_CUSTOM_FEAT_MAX_NUM_OF_CLIENTS(max_num_sta);

	if (IS_ENABLED(CONFIG_BT_SILABS_SIWX91X)) {
		LOG_WRN("Bluetooth is not supported in AP mode");
	}
}

static void siwx91x_configure_network_stack(sl_si91x_boot_configuration_t *boot_config,
					    uint8_t wifi_oper_mode)
{
	if (!IS_ENABLED(CONFIG_WIFI_SILABS_SIWX91X_NET_STACK_OFFLOAD)) {
		boot_config->tcp_ip_feature_bit_map |= SL_SI91X_TCP_IP_FEAT_BYPASS;
		return;
	}

	boot_config->tcp_ip_feature_bit_map |= SL_SI91X_TCP_IP_FEAT_ICMP;
	boot_config->ext_tcp_ip_feature_bit_map |= SL_SI91X_EXT_TCP_IP_WINDOW_SCALING;
	boot_config->ext_tcp_ip_feature_bit_map |= SL_SI91X_EXT_TCP_IP_TOTAL_SELECTS(10);

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		boot_config->tcp_ip_feature_bit_map |= SL_SI91X_TCP_IP_FEAT_IPV6;

		if (wifi_oper_mode == WIFI_STA_MODE) {
			boot_config->tcp_ip_feature_bit_map |= SL_SI91X_TCP_IP_FEAT_DHCPV6_CLIENT;
		} else if (wifi_oper_mode == WIFI_SOFTAP_MODE) {
			boot_config->tcp_ip_feature_bit_map |= SL_SI91X_TCP_IP_FEAT_DHCPV6_SERVER;
		} else {
			/* No DHCPv6 configuration needed for other modes */
		}
	}

	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		if (wifi_oper_mode == WIFI_STA_MODE) {
			boot_config->tcp_ip_feature_bit_map |= SL_SI91X_TCP_IP_FEAT_DHCPV4_CLIENT;
		} else if (wifi_oper_mode == WIFI_SOFTAP_MODE) {
			boot_config->tcp_ip_feature_bit_map |= SL_SI91X_TCP_IP_FEAT_DHCPV4_SERVER;
		} else {
			/* No DHCPv4 configuration needed for other modes */
		}
	}
}

int siwx91x_get_nwp_config(sl_wifi_device_configuration_t *get_config, uint8_t wifi_oper_mode,
			   bool hidden_ssid, uint8_t max_num_sta)
{
	sl_wifi_device_configuration_t default_config = {
		.band = SL_SI91X_WIFI_BAND_2_4GHZ,
		.region_code = DEFAULT_REGION,
		.boot_option = LOAD_NWP_FW,
		.boot_config = {
			.feature_bit_map = SL_SI91X_FEAT_SECURITY_OPEN | SL_SI91X_FEAT_WPS_DISABLE |
					   SL_SI91X_FEAT_SECURITY_PSK | SL_SI91X_FEAT_AGGREGATION |
					   SL_SI91X_FEAT_HIDE_PSK_CREDENTIALS,
			.tcp_ip_feature_bit_map = SL_SI91X_TCP_IP_FEAT_EXTENSION_VALID,
			.custom_feature_bit_map = SL_SI91X_CUSTOM_FEAT_EXTENSION_VALID |
						  SL_SI91X_CUSTOM_FEAT_ASYNC_CONNECTION_STATUS |
						  SL_SI91X_CUSTOM_FEAT_RTC_FROM_HOST,
			.ext_custom_feature_bit_map =
				SL_SI91X_EXT_FEAT_XTAL_CLK | SL_SI91X_EXT_FEAT_1P8V_SUPPORT |
				SL_SI91X_EXT_FEAT_DISABLE_XTAL_CORRECTION |
				SL_SI91X_EXT_FEAT_NWP_QSPI_80MHZ_CLK_ENABLE |
				SL_SI91X_EXT_FEAT_FRONT_END_SWITCH_PINS_ULP_GPIO_4_5_0 |
				SL_SI91X_EXT_FEAT_FRONT_END_INTERNAL_SWITCH |
				SL_SI91X_EXT_FEAT_XTAL_CLK,
		}
	};

	sl_si91x_boot_configuration_t *boot_config = &default_config.boot_config;

	__ASSERT(get_config, "get_config cannot be NULL");
	__ASSERT((hidden_ssid == false && max_num_sta == 0) || wifi_oper_mode == WIFI_SOFTAP_MODE,
		 "hidden_ssid or max_num_sta requires SOFT AP mode");

	if (wifi_oper_mode == WIFI_SOFTAP_MODE && max_num_sta > AP_MAX_NUM_STA) {
		LOG_ERR("Exceeded maximum supported stations (%d)", AP_MAX_NUM_STA);
		return -EINVAL;
	}

	siwx91x_apply_sram_config(boot_config);

	switch (wifi_oper_mode) {
	case WIFI_STA_MODE:
		siwx91x_configure_sta_mode(boot_config);
		break;
	case WIFI_SOFTAP_MODE:
		siwx91x_configure_ap_mode(boot_config, hidden_ssid, max_num_sta);
		break;
	default:
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_WIFI_SILABS_SIWX91X)) {
		if (!IS_ENABLED(CONFIG_PM)) {
			boot_config->custom_feature_bit_map |=
				SL_SI91X_CUSTOM_FEAT_SOC_CLK_CONFIG_160MHZ;
		}

		siwx91x_configure_network_stack(boot_config, wifi_oper_mode);
	}

	memcpy(get_config, &default_config, sizeof(default_config));
	return 0;
}

int siwx91x_nwp_mode_switch(uint8_t oper_mode, bool hidden_ssid, uint8_t max_num_sta)
{
	sl_wifi_device_configuration_t nwp_config;
	int status;

	status = siwx91x_get_nwp_config(&nwp_config, oper_mode, hidden_ssid, max_num_sta);
	if (status < 0) {
		return status;
	}

	/* FIXME: Calling sl_wifi_deinit() impacts Bluetooth if coexistence is enabled */
	status = sl_wifi_deinit();
	if (status != SL_STATUS_OK) {
		return -ETIMEDOUT;
	}

	status = sl_wifi_init(&nwp_config, NULL, sl_wifi_default_event_handler);
	if (status != SL_STATUS_OK) {
		return -ETIMEDOUT;
	}

	return 0;
}

static int siwg917_nwp_init(void)
{
	sl_wifi_device_configuration_t network_config;
	sl_status_t status;

	siwx91x_get_nwp_config(&network_config, WIFI_STA_MODE, false, 0);
	/* TODO: If sl_net_*_profile() functions will be needed for WiFi then call
	 * sl_net_set_profile() here. Currently these are unused.
	 */
	status = sl_wifi_init(&network_config, NULL, sl_wifi_default_event_handler);
	if (status != SL_STATUS_OK) {
		return -EINVAL;
	}

	return 0;
}
#if defined(CONFIG_MBEDTLS_INIT)
BUILD_ASSERT(CONFIG_SIWX91X_NWP_INIT_PRIORITY < CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
	     "mbed TLS must be initialized after the NWP.");
#endif
SYS_INIT(siwg917_nwp_init, POST_KERNEL, CONFIG_SIWX91X_NWP_INIT_PRIORITY);

/* IRQn 74 is used for communication with co-processor */
Z_ISR_DECLARE(74, 0, IRQ074_Handler, 0);

/* Co-processor will use value stored in IVT to store its stack.
 *
 * FIXME: We can't use Z_ISR_DECLARE() to declare this entry
 * FIXME: Allow to configure size of buffer
 */
static uint8_t __aligned(8) siwg917_nwp_stack[10 * 1024];
static Z_DECL_ALIGN(struct _isr_list) Z_GENERIC_SECTION(.intList)
	__used __isr_siwg917_coprocessor_stack_irq = {
		.irq = 30,
		.flags = ISR_FLAG_DIRECT,
		.func = &siwg917_nwp_stack[sizeof(siwg917_nwp_stack) - 1],
	};
