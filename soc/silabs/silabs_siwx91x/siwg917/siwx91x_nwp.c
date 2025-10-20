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

#define DT_DRV_COMPAT silabs_siwx91x_nwp

#include <zephyr/kernel.h>
#include <zephyr/net/wifi.h>
#include <zephyr/logging/log.h>
#include <zephyr/devicetree.h>

#include "siwx91x_nwp.h"
#include "nwp_fw_version.h"
#include "sl_wifi_callback_framework.h"

#include "sl_si91x_ble.h"
#ifdef CONFIG_BT_SILABS_SIWX91X
#include "rsi_ble_common_config.h"
#endif
#include "sl_si91x_power_manager.h"

#define AP_MAX_NUM_STA 4

LOG_MODULE_REGISTER(siwx91x_nwp);

BUILD_ASSERT(DT_REG_SIZE(DT_CHOSEN(zephyr_sram)) == KB(195) ||
	     DT_REG_SIZE(DT_CHOSEN(zephyr_sram)) == KB(255) ||
	     DT_REG_SIZE(DT_CHOSEN(zephyr_sram)) == KB(319));

struct siwx91x_nwp_data {
	char current_country_code[WIFI_COUNTRY_CODE_LEN];
};

struct siwx91x_nwp_config {
	void (*config_irq)(const struct device *dev);
	uint32_t stack_size;
	uint8_t power_profile;
};

typedef struct {
	const char *const *codes;
	size_t num_codes;
	sl_wifi_region_code_t region_code;
	const sli_si91x_set_region_ap_request_t *sdk_reg;
} region_map_t;

extern const sli_si91x_set_region_ap_request_t default_US_region_2_4GHZ_configurations;
extern const sli_si91x_set_region_ap_request_t default_EU_region_2_4GHZ_configurations;
extern const sli_si91x_set_region_ap_request_t default_JP_region_2_4GHZ_configurations;
extern const sli_si91x_set_region_ap_request_t default_KR_region_2_4GHZ_configurations;
extern const sli_si91x_set_region_ap_request_t default_SG_region_2_4GHZ_configurations;
extern const sli_si91x_set_region_ap_request_t default_CN_region_2_4GHZ_configurations;

static const char *const us_codes[] = {
	"AE", "AR", "AS", "BB", "BM", "BR", "BS", "CA", "CO", "CR", "CU", "CX", "DM", "DO",
	"EC", "FM", "GD", "GY", "GU", "HN", "HT", "JM", "KY", "LB", "LK", "MH", "MN", "MP",
	"MO", "MY", "NI", "PA", "PE", "PG", "PH", "PK", "PR", "PW", "PY", "SG", "MX", "SV",
	"TC", "TH", "TT", "US", "UY", "VE", "VI", "VN", "VU", "00"
	/* Map "00" (world domain) to US region as using the world domain is not recommended */
};
static const char *const eu_codes[] = {
	"AD", "AF", "AI", "AL", "AM", "AN", "AT", "AW", "AU", "AZ", "BA", "BE", "BG", "BH", "BL",
	"BT", "BY", "CH", "CY", "CZ", "DE", "DK", "EE", "ES", "FR", "GB", "GE", "GF", "GL", "GP",
	"GR", "GT", "HK", "HR", "HU", "ID", "IE", "IL", "IN", "IR", "IS", "IT", "JO", "KH", "FI",
	"KN", "KW", "KZ", "LC", "LI", "LT", "LU", "LV", "MD", "ME", "MK", "MF", "MT", "MV", "MQ",
	"NL", "NO", "NZ", "OM", "PF", "PL", "PM", "PT", "QA", "RO", "RS", "RU", "SA", "SE", "SI",
	"SK", "SR", "SY", "TR", "TW", "UA", "UZ", "VC", "WF", "WS", "YE", "RE", "YT"
};
static const char *const jp_codes[] = {"BD", "BN", "BO", "CL", "BZ", "JP", "NP"};
static const char *const kr_codes[] = {"KR", "KP"};
static const char *const cn_codes[] = {"CN"};
static const region_map_t region_maps[] = {
	{us_codes, ARRAY_SIZE(us_codes), SL_WIFI_REGION_US,
	 &default_US_region_2_4GHZ_configurations},
	{eu_codes, ARRAY_SIZE(eu_codes), SL_WIFI_REGION_EU,
	 &default_EU_region_2_4GHZ_configurations},
	{jp_codes, ARRAY_SIZE(jp_codes), SL_WIFI_REGION_JP,
	 &default_JP_region_2_4GHZ_configurations},
	{kr_codes, ARRAY_SIZE(kr_codes), SL_WIFI_REGION_KR,
	 &default_KR_region_2_4GHZ_configurations},
	{cn_codes, ARRAY_SIZE(cn_codes), SL_WIFI_REGION_CN,
	 &default_CN_region_2_4GHZ_configurations},
};

int siwx91x_store_country_code(const struct device *dev, const char *country_code)
{
	__ASSERT(country_code, "country_code cannot be NULL");
	struct siwx91x_nwp_data *data = dev->data;

	memcpy(data->current_country_code, country_code, WIFI_COUNTRY_CODE_LEN);
	return 0;
}

const char *siwx91x_get_country_code(const struct device *dev)
{
	const struct siwx91x_nwp_data *data = dev->data;

	return data->current_country_code;
}

sl_wifi_region_code_t siwx91x_map_country_code_to_region(const char *country_code)
{
	__ASSERT(country_code, "country_code cannot be NULL");

	ARRAY_FOR_EACH(region_maps, i) {
		for (size_t j = 0; j < region_maps[i].num_codes; j++) {
			if (memcmp(country_code, region_maps[i].codes[j],
			       WIFI_COUNTRY_CODE_LEN) == 0) {
				return region_maps[i].region_code;
			}
		}
	}
	return SL_WIFI_DEFAULT_REGION;
}

const sli_si91x_set_region_ap_request_t *siwx91x_find_sdk_region_table(uint8_t region_code)
{
	ARRAY_FOR_EACH(region_maps, i) {
		if (region_maps[i].region_code == region_code) {
			return region_maps[i].sdk_reg;
		}
	}
	return NULL;
}

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
	boot_config->ext_custom_feature_bit_map |=
		SL_SI91X_EXT_FEAT_IEEE_80211W |
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
		SL_SI91X_BLE_NUM_REC_BYTES(RSI_BLE_NUM_REC_BYTES) | SL_SI91X_BLE_ENABLE_ADV_EXTN |
		SL_SI91X_BLE_AE_MAX_ADV_SETS(RSI_BLE_AE_MAX_ADV_SETS) |
		SL_SI91X_BT_BLE_STACK_BYPASS_ENABLE;
#endif
}

static void siwx91x_configure_ap_mode(sl_si91x_boot_configuration_t *boot_config, bool hidden_ssid,
				      uint8_t max_num_sta)
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

static int siwx91x_check_nwp_version(void)
{
	sl_wifi_firmware_version_t expected_version;
	sl_wifi_firmware_version_t version;
	int ret;

	ret = sl_wifi_get_firmware_version(&version);
	if (ret != SL_STATUS_OK) {
		return -EINVAL;
	}

	sscanf(SIWX91X_NWP_FW_EXPECTED_VERSION, "%hhX.%hhd.%hhd.%hhd.%hhd.%hhd.%hd",
	       &expected_version.rom_id,
	       &expected_version.major,
	       &expected_version.minor,
	       &expected_version.security_version,
	       &expected_version.patch_num,
	       &expected_version.customer_id,
	       &expected_version.build_num);

	/* Ignore rom_id:
	 * B is parsed as an hex value and we get 11 in expected_version.rom_id
	 * We received rom_id=17 in version.rom_id, we suspect a double hex->decimal conversion
	 */
	if (expected_version.major != version.major) {
		return -EINVAL;
	}
	if (expected_version.minor != version.minor) {
		return -EINVAL;
	}
	if (expected_version.security_version != version.security_version) {
		return -EINVAL;
	}
	if (expected_version.patch_num != version.patch_num) {
		return -EINVAL;
	}
	if (expected_version.customer_id != version.customer_id) {
		LOG_DBG("customer_id diverge: expected %d, actual %d", expected_version.customer_id,
			version.customer_id);
	}
	if (expected_version.build_num != version.build_num) {
		LOG_DBG("build_num diverge: expected %d, actual %d", expected_version.build_num,
			version.build_num);
	}

	return 0;
}

static int siwx91x_get_nwp_config(const struct device *dev,
				  sl_wifi_device_configuration_t *get_config,
				  uint8_t wifi_oper_mode, bool hidden_ssid, uint8_t max_num_sta)
{
	sl_wifi_device_configuration_t default_config = {
		.region_code = siwx91x_map_country_code_to_region(DEFAULT_COUNTRY_CODE),
		.band = SL_SI91X_WIFI_BAND_2_4GHZ,
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

	siwx91x_store_country_code(dev, DEFAULT_COUNTRY_CODE);
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
		siwx91x_configure_network_stack(boot_config, wifi_oper_mode);
	}

	memcpy(get_config, &default_config, sizeof(default_config));
	return 0;
}

int siwx91x_nwp_mode_switch(const struct device *dev, uint8_t oper_mode, bool hidden_ssid,
			    uint8_t max_num_sta)
{
	sl_wifi_device_configuration_t nwp_config;
	int status;

	status = siwx91x_get_nwp_config(dev, &nwp_config, oper_mode, hidden_ssid, max_num_sta);
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

static int siwx91x_nwp_init(const struct device *dev)
{
	const struct siwx91x_nwp_config *config = dev->config;
	__maybe_unused sl_wifi_performance_profile_t performance_profile = {
		.profile = config->power_profile};
	__maybe_unused sl_bt_performance_profile_t bt_performance_profile = {
		.profile = config->power_profile};
	sl_wifi_device_configuration_t network_config;
	int ret;

	siwx91x_get_nwp_config(dev, &network_config, WIFI_STA_MODE, false, 0);
	/* TODO: If sl_net_*_profile() functions will be needed for WiFi then call
	 * sl_net_set_profile() here. Currently these are unused.
	 * Despite its name, this function need to be called even if wifi is not used.
	 */
	ret = sl_wifi_init(&network_config, NULL, sl_wifi_default_event_handler);
	if (ret) {
		return -EINVAL;
	}

	/* Check if the NWP firmware version is correct */
	ret = siwx91x_check_nwp_version();
	if (ret < 0) {
		LOG_ERR("Unexpected NWP firmware version (expected: %s)",
			SIWX91X_NWP_FW_EXPECTED_VERSION);
	}

	if (IS_ENABLED(CONFIG_SOC_SIWX91X_PM_BACKEND_PMGR)) {
		if (IS_ENABLED(CONFIG_BT_SILABS_SIWX91X)) {
			ret = sl_si91x_bt_set_performance_profile(&bt_performance_profile);
			if (ret) {
				LOG_ERR("Failed to initiate power save in BLE mode");
				return -EINVAL;
			}
		}
		/*
		 * Note: the WiFi related sources are always imported (because of
		 * CONFIG_SILABS_SIWX91X_NWP) whatever the value of CONFIG_WIFI. However,
		 * because of boot_config->coex_mode, sl_wifi_set_performance_profile() is a no-op
		 * if CONFIG_WIFI=n and CONFIG_BT=y. We could probably remove the dependency to the
		 * WiFi sources in this case. However, outside of the code size, this dependency
		 * does not hurt.
		 */
		ret = sl_wifi_set_performance_profile(&performance_profile);
		if (ret) {
			return -EINVAL;
		}
		/* Remove the previously added PS4 power state requirement */
		sl_si91x_power_manager_remove_ps_requirement(SL_SI91X_POWER_MANAGER_PS4);
	}

	config->config_irq(dev);

	return 0;
}

#if defined(CONFIG_MBEDTLS_INIT)
BUILD_ASSERT(CONFIG_SIWX91X_NWP_INIT_PRIORITY < CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
	     "mbed TLS must be initialized after the NWP.");
#endif

#define SIWX91X_NWP_DEFINE(inst)                                                                   \
                                                                                                   \
	static void silabs_siwx91x_nwp_irq_configure_##inst(const struct device *dev)              \
	{                                                                                          \
		ARG_UNUSED(dev);                                                                   \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(inst, nwp_irq, irq),                               \
			    DT_INST_IRQ_BY_NAME(inst, nwp_irq, priority), IRQ074_Handler, NULL, 0);\
		irq_enable(DT_INST_IRQ_BY_NAME(inst, nwp_irq, irq));                               \
	};                                                                                         \
                                                                                                   \
	static struct siwx91x_nwp_data siwx91x_nwp_data_##inst = {};                               \
                                                                                                   \
	static const struct siwx91x_nwp_config siwx91x_nwp_config_##inst = {                       \
		.config_irq = silabs_siwx91x_nwp_irq_configure_##inst,                             \
		.power_profile = DT_ENUM_IDX(DT_DRV_INST(inst), power_profile),                    \
		.stack_size = DT_INST_PROP(inst, stack_size)                                       \
	};                                                                                         \
                                                                                                   \
	/* Coprocessor uses value stored in IVT to store its stack. We can't use Z_ISR_DECLARE() */\
	static uint8_t __aligned(8) siwx91x_nwp_stack_##inst[DT_INST_PROP(inst, stack_size)];      \
	static Z_DECL_ALIGN(struct _isr_list) Z_GENERIC_SECTION(.intList)                          \
		__used __isr_siwg917_coprocessor_stack_irq_##inst = {                              \
			.irq = DT_IRQ_BY_NAME(DT_DRV_INST(inst), nwp_stack, irq),                  \
			.flags = ISR_FLAG_DIRECT,                                                  \
			.func = &siwx91x_nwp_stack_##inst[sizeof(siwx91x_nwp_stack_##inst) - 1],   \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &siwx91x_nwp_init, NULL, &siwx91x_nwp_data_##inst,             \
			      &siwx91x_nwp_config_##inst, POST_KERNEL,                             \
			      CONFIG_SIWX91X_NWP_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(SIWX91X_NWP_DEFINE)
