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
			/* Even if neither WiFi or BLE is used we have to specify a Coex mode */
			.coex_mode = SL_SI91X_BLE_MODE,
			.ext_custom_feature_bit_map =
				MEMORY_CONFIG |
				SL_SI91X_EXT_FEAT_XTAL_CLK |
				SL_SI91X_EXT_FEAT_FRONT_END_SWITCH_PINS_ULP_GPIO_4_5_0,
		}
	};

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
