/**
 * @file
 * @brief Network Processor Initialization for SiWx91x.
 *
 * This file contains the initialization routine for the (ThreadArch) network processor
 * on the SiWx91x platform. The component is responsible for setting up the necessary
 * hardware and software components to enable network communication.
 *
 * Copyright (c) 2024 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include "sl_wifi.h"
#include "sl_wifi_callback_framework.h"
#ifdef CONFIG_BT_SILABS_SIWX91X
#include "rsi_ble_common_config.h"
#endif

static int siwg917_nwp_init(void)
{
	sl_wifi_device_configuration_t network_config = {
		.boot_option = LOAD_NWP_FW,
		.band = SL_SI91X_WIFI_BAND_2_4GHZ,
		.region_code = DEFAULT_REGION,
		.boot_config = {
			.oper_mode = SL_SI91X_CLIENT_MODE,
			.tcp_ip_feature_bit_map = SL_SI91X_TCP_IP_FEAT_EXTENSION_VALID,
			.ext_tcp_ip_feature_bit_map = SL_SI91X_CONFIG_FEAT_EXTENSION_VALID,
			.config_feature_bit_map = SL_SI91X_ENABLE_ENHANCED_MAX_PSP,
			.custom_feature_bit_map = SL_SI91X_CUSTOM_FEAT_EXTENSION_VALID,
			.ext_custom_feature_bit_map =
				MEMORY_CONFIG |
				SL_SI91X_EXT_FEAT_XTAL_CLK |
				SL_SI91X_EXT_FEAT_FRONT_END_SWITCH_PINS_ULP_GPIO_4_5_0,
		}
	};
	sl_si91x_boot_configuration_t *cfg = &network_config.boot_config;

	if (IS_ENABLED(CONFIG_WIFI_SILABS_SIWX91X) && IS_ENABLED(CONFIG_BT_SILABS_SIWX91X)) {
		cfg->coex_mode = SL_SI91X_WLAN_BLE_MODE;
	} else if (IS_ENABLED(CONFIG_WIFI_SILABS_SIWX91X)) {
		cfg->coex_mode = SL_SI91X_WLAN_ONLY_MODE;
	} else if (IS_ENABLED(CONFIG_BT_SILABS_SIWX91X)) {
		cfg->coex_mode = SL_SI91X_BLE_MODE;
	} else {
		/*
		 * Even if neither WiFi or BLE is used we have to specify a Coex mode
		 */
		cfg->coex_mode = SL_SI91X_BLE_MODE;
	}

#ifdef CONFIG_WIFI_SILABS_SIWX91X
	cfg->feature_bit_map |= SL_SI91X_FEAT_SECURITY_OPEN | SL_SI91X_FEAT_WPS_DISABLE,
	cfg->ext_custom_feature_bit_map |= SL_SI91X_EXT_FEAT_IEEE_80211W;
	if (IS_ENABLED(CONFIG_WIFI_SILABS_SIWX91X_NET_STACK_OFFLOAD)) {
		cfg->ext_tcp_ip_feature_bit_map |= SL_SI91X_EXT_TCP_IP_WINDOW_SCALING;
		cfg->ext_tcp_ip_feature_bit_map |= SL_SI91X_EXT_TCP_IP_TOTAL_SELECTS(10);
		cfg->tcp_ip_feature_bit_map |= SL_SI91X_TCP_IP_FEAT_ICMP;
		if (IS_ENABLED(CONFIG_NET_IPV6)) {
			cfg->tcp_ip_feature_bit_map |= SL_SI91X_TCP_IP_FEAT_DHCPV6_CLIENT;
			cfg->tcp_ip_feature_bit_map |= SL_SI91X_TCP_IP_FEAT_IPV6;
		}
		if (IS_ENABLED(CONFIG_NET_IPV4)) {
			cfg->tcp_ip_feature_bit_map |= SL_SI91X_TCP_IP_FEAT_DHCPV4_CLIENT;
		}
	} else {
		cfg->tcp_ip_feature_bit_map |= SL_SI91X_TCP_IP_FEAT_BYPASS;
	}
#endif

#ifdef CONFIG_BT_SILABS_SIWX91X
	cfg->ext_custom_feature_bit_map |= SL_SI91X_EXT_FEAT_BT_CUSTOM_FEAT_ENABLE;
	cfg->bt_feature_bit_map |= SL_SI91X_BT_RF_TYPE | SL_SI91X_ENABLE_BLE_PROTOCOL;
	cfg->ble_feature_bit_map |= SL_SI91X_BLE_MAX_NBR_PERIPHERALS(RSI_BLE_MAX_NBR_PERIPHERALS) |
				    SL_SI91X_BLE_MAX_NBR_CENTRALS(RSI_BLE_MAX_NBR_CENTRALS) |
				    SL_SI91X_BLE_MAX_NBR_ATT_SERV(RSI_BLE_MAX_NBR_ATT_SERV) |
				    SL_SI91X_BLE_MAX_NBR_ATT_REC(RSI_BLE_MAX_NBR_ATT_REC) |
				    SL_SI91X_BLE_PWR_INX(RSI_BLE_PWR_INX) |
				    SL_SI91X_BLE_PWR_SAVE_OPTIONS(RSI_BLE_PWR_SAVE_OPTIONS) |
				    SL_SI91X_916_BLE_COMPATIBLE_FEAT_ENABLE |
				    SL_SI91X_FEAT_BLE_CUSTOM_FEAT_EXTENSION_VALID;
	cfg->ble_ext_feature_bit_map |= SL_SI91X_BLE_NUM_CONN_EVENTS(RSI_BLE_NUM_CONN_EVENTS) |
					SL_SI91X_BLE_NUM_REC_BYTES(RSI_BLE_NUM_REC_BYTES) |
					SL_SI91X_BLE_ENABLE_ADV_EXTN |
					SL_SI91X_BLE_AE_MAX_ADV_SETS(RSI_BLE_AE_MAX_ADV_SETS);
#endif

	/* TODO: If sl_net_*_profile() functions will be needed for WiFi then call
	 * sl_net_set_profile() here. Currently these are unused.
	 */
	return sl_wifi_init(&network_config, NULL, sl_wifi_default_event_handler);
}
SYS_INIT(siwg917_nwp_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

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
