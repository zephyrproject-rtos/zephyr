/**
 * Copyright (c) 2025 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT silabs_siwx91x_nwp
#include <zephyr/net/wifi.h>
#include <zephyr/logging/log.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/pinctrl.h>
#include "siwx91x_nwp.h"
#include "siwx91x_nwp_bus.h"
#include "siwx91x_nwp_api.h"
#include "siwx91x_nwp_fw.h"
#include "siwx91x_nwp_fw_version.h"
#include "device/silabs/si91x/wireless/ble/inc/sl_si91x_ble.h"
#include "device/silabs/si91x/mcu/drivers/service/power_manager/inc/sl_si91x_power_manager.h"
#include "device/silabs/si91x/wireless/ble/inc/rsi_ble_common_config.h"

LOG_MODULE_REGISTER(siwx91x_nwp, CONFIG_SIWX91X_NWP_LOG_LEVEL);

BUILD_ASSERT(DT_REG_SIZE(DT_CHOSEN(zephyr_sram)) == KB(195) ||
	     DT_REG_SIZE(DT_CHOSEN(zephyr_sram)) == KB(255) ||
	     DT_REG_SIZE(DT_CHOSEN(zephyr_sram)) == KB(319));

void siwx91x_nwp_register_wifi(const struct device *dev, const struct siwx91x_nwp_wifi_cb *val)
{
	struct siwx91x_nwp_data *data = dev->data;

	data->wifi = val;
}

void siwx91x_nwp_register_bt(const struct device *dev, const struct siwx91x_nwp_bt_cb *val)
{
	struct siwx91x_nwp_data *data = dev->data;

	data->bt = val;
}

static void siwx91x_apply_sram_config(sl_wifi_system_boot_configuration_t *params)
{
	/* The size does not match exactly because 1 KB is reserved at the start of the RAM */
	size_t sram_size = DT_REG_SIZE(DT_CHOSEN(zephyr_sram));

	if (sram_size == KB(195)) {
		params->ext_custom_feature_bit_map |= SL_SI91X_EXT_FEAT_480K_M4SS_192K;
	} else if (sram_size == KB(255)) {
		params->ext_custom_feature_bit_map |= SL_SI91X_EXT_FEAT_416K_M4SS_256K;
	} else if (sram_size == KB(319)) {
		params->ext_custom_feature_bit_map |= SL_SI91X_EXT_FEAT_352K_M4SS_320K;
	} else {
		k_panic();
	}
}

static void siwx91x_apply_boot_config(const struct device *dev,
				      sl_wifi_system_boot_configuration_t *params)
{
	const struct siwx91x_nwp_config *cfg = dev->config;

	if (cfg->qspi_80mhz_clk) {
		params->ext_custom_feature_bit_map |= SL_SI91X_EXT_FEAT_NWP_QSPI_80MHZ_CLK_ENABLE;
	}
	if (!cfg->enable_xtal_correction) {
		params->ext_custom_feature_bit_map |= SL_SI91X_EXT_FEAT_DISABLE_XTAL_CORRECTION;
	}
	if (cfg->support_1p8v) {
		params->ext_custom_feature_bit_map |= SL_SI91X_EXT_FEAT_1P8V_SUPPORT;
	}
	params->ext_custom_feature_bit_map |= FIELD_PREP(0x60000000, cfg->antenna_selection);
}

static void siwx91x_apply_sta_mode(sl_wifi_system_boot_configuration_t *params)
{
	const bool wifi_enabled = IS_ENABLED(CONFIG_WIFI_SILABS_SIWX91X);
	const bool bt_enabled = IS_ENABLED(CONFIG_BT_SILABS_SIWX91X);

	params->oper_mode = SL_SI91X_CLIENT_MODE;

	if (wifi_enabled) {
		params->feature_bit_map |= SL_SI91X_FEAT_SECURITY_OPEN;
		if (IS_ENABLED(CONFIG_WIFI_SILABS_SIWX91X_FEAT_SECURITY_PSK)) {
			params->feature_bit_map |= SL_SI91X_FEAT_SECURITY_PSK;
		}
	}

	if (wifi_enabled && bt_enabled) {
		params->coex_mode = SL_SI91X_WLAN_BLE_MODE;
	} else if (wifi_enabled) {
		params->coex_mode = SL_SI91X_WLAN_ONLY_MODE;
	} else if (bt_enabled) {
		params->coex_mode = SL_SI91X_WLAN_BLE_MODE;
	} else {
		/*
		 * Even if neither WiFi or BLE is used we have to specify a Coex mode
		 */
		params->coex_mode = SL_SI91X_WLAN_ONLY_MODE;
	}

#ifdef CONFIG_WIFI_SILABS_SIWX91X
	params->ext_tcp_ip_feature_bit_map = SL_SI91X_CONFIG_FEAT_EXTENSION_VALID;
	if (IS_ENABLED(CONFIG_WIFI_SILABS_SIWX91X_MFP)) {
		params->ext_custom_feature_bit_map |= SL_SI91X_EXT_FEAT_IEEE_80211W;
	}
	if (IS_ENABLED(CONFIG_WIFI_SILABS_SIWX91X_ENHANCED_MAX_PSP)) {
		params->config_feature_bit_map = SL_SI91X_ENABLE_ENHANCED_MAX_PSP;
	}
#endif

#ifdef CONFIG_BT_SILABS_SIWX91X
	params->ext_custom_feature_bit_map |= SL_SI91X_EXT_FEAT_BT_CUSTOM_FEAT_ENABLE;
	params->bt_feature_bit_map |= SL_SI91X_BT_RF_TYPE | SL_SI91X_ENABLE_BLE_PROTOCOL;
	params->ble_feature_bit_map |=
		SL_SI91X_BLE_MAX_NBR_PERIPHERALS(RSI_BLE_MAX_NBR_PERIPHERALS) |
		SL_SI91X_BLE_MAX_NBR_CENTRALS(RSI_BLE_MAX_NBR_CENTRALS) |
		SL_SI91X_BLE_MAX_NBR_ATT_SERV(RSI_BLE_MAX_NBR_ATT_SERV) |
		SL_SI91X_BLE_MAX_NBR_ATT_REC(RSI_BLE_MAX_NBR_ATT_REC) |
		SL_SI91X_BLE_PWR_INX(RSI_BLE_PWR_INX) |
		SL_SI91X_BLE_PWR_SAVE_OPTIONS(RSI_BLE_PWR_SAVE_OPTIONS) |
		SL_SI91X_916_BLE_COMPATIBLE_FEAT_ENABLE |
		SL_SI91X_FEAT_BLE_CUSTOM_FEAT_EXTENSION_VALID;

	params->ble_ext_feature_bit_map |=
		SL_SI91X_BLE_NUM_CONN_EVENTS(RSI_BLE_NUM_CONN_EVENTS) |
		SL_SI91X_BLE_NUM_REC_BYTES(RSI_BLE_NUM_REC_BYTES) | SL_SI91X_BLE_ENABLE_ADV_EXTN |
		SL_SI91X_BLE_AE_MAX_ADV_SETS(RSI_BLE_AE_MAX_ADV_SETS) |
		SL_SI91X_BT_BLE_STACK_BYPASS_ENABLE;
#endif
}

static void siwx91x_apply_ap_mode(sl_wifi_system_boot_configuration_t *params,
				  bool hidden_ssid, uint8_t max_num_sta)
{
	if (IS_ENABLED(CONFIG_BT_SILABS_SIWX91X)) {
		LOG_WRN("Bluetooth is not supported in AP mode");
	}

	params->oper_mode = SL_SI91X_ACCESS_POINT_MODE;
	params->coex_mode = SL_SI91X_WLAN_ONLY_MODE;
	if (IS_ENABLED(CONFIG_WIFI_SILABS_SIWX91X_LIMIT_PACKET_BUF_PER_STA)) {
		params->custom_feature_bit_map |= SL_SI91X_CUSTOM_FEAT_LIMIT_PACKETS_PER_STA;
	}
	if (hidden_ssid) {
		params->custom_feature_bit_map |= SL_SI91X_CUSTOM_FEAT_AP_IN_HIDDEN_MODE;
	}
	params->custom_feature_bit_map |= SL_WIFI_CUSTOM_FEAT_MAX_NUM_OF_CLIENTS(max_num_sta);
}

static void siwx91x_apply_network_features(sl_wifi_system_boot_configuration_t *params,
					   uint8_t wifi_oper_mode)
{
	if (!IS_ENABLED(CONFIG_WIFI_SILABS_SIWX91X_NET_STACK_OFFLOAD)) {
		params->tcp_ip_feature_bit_map |= SL_SI91X_TCP_IP_FEAT_BYPASS;
		return;
	}

	params->tcp_ip_feature_bit_map |= SL_SI91X_TCP_IP_FEAT_ICMP;
	params->ext_tcp_ip_feature_bit_map |= SL_SI91X_EXT_TCP_IP_WINDOW_SCALING;
	params->ext_tcp_ip_feature_bit_map |= FIELD_PREP(0x0000F000, SIWX91X_MAX_CONCURRENT_SELECT);

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		params->tcp_ip_feature_bit_map |= SL_SI91X_TCP_IP_FEAT_IPV6;

		if (wifi_oper_mode == WIFI_STA_MODE) {
			params->tcp_ip_feature_bit_map |= SL_SI91X_TCP_IP_FEAT_DHCPV6_CLIENT;
		} else if (wifi_oper_mode == WIFI_SOFTAP_MODE) {
			params->tcp_ip_feature_bit_map |= SL_SI91X_TCP_IP_FEAT_DHCPV6_SERVER;
		} else {
			/* No DHCPv6 configuration needed for other modes */
		}
	}

	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		if (wifi_oper_mode == WIFI_STA_MODE) {
			params->tcp_ip_feature_bit_map |= SL_SI91X_TCP_IP_FEAT_DHCPV4_CLIENT;
		} else if (wifi_oper_mode == WIFI_SOFTAP_MODE) {
			params->tcp_ip_feature_bit_map |= SL_SI91X_TCP_IP_FEAT_DHCPV4_SERVER;
		} else {
			/* No DHCPv4 configuration needed for other modes */
		}
	}
}

static int siwx91x_nwp_compute_config(const struct device *dev,
				      sl_wifi_system_boot_configuration_t *get_config,
				      bool *enable_pll, uint8_t wifi_oper_mode, bool hidden_ssid,
				      uint8_t max_num_sta)
{
	const struct siwx91x_nwp_config *config = dev->config;
	sl_wifi_system_boot_configuration_t boot_config = {
		.feature_bit_map = SL_SI91X_FEAT_WPS_DISABLE |
				   SL_SI91X_FEAT_AGGREGATION |
				   SL_SI91X_FEAT_HIDE_PSK_CREDENTIALS |
				   SLI_SI91X_FEAT_FW_UPDATE_NEW_CODE,
		.tcp_ip_feature_bit_map = SL_SI91X_TCP_IP_FEAT_EXTENSION_VALID,
		.custom_feature_bit_map = SL_SI91X_CUSTOM_FEAT_EXTENSION_VALID |
					  SL_SI91X_CUSTOM_FEAT_ASYNC_CONNECTION_STATUS,
		.ext_custom_feature_bit_map = SL_SI91X_EXT_FEAT_XTAL_CLK,
	};


	__ASSERT(get_config, "get_config cannot be NULL");
	__ASSERT((hidden_ssid == false && max_num_sta == 0) || wifi_oper_mode == WIFI_SOFTAP_MODE,
		 "hidden_ssid or max_num_sta requires SOFT AP mode");

	if (wifi_oper_mode == WIFI_SOFTAP_MODE && max_num_sta > 4) {
		LOG_ERR("Exceeded maximum supported stations");
		return -EINVAL;
	}

	siwx91x_apply_sram_config(&boot_config);
	siwx91x_apply_boot_config(dev, &boot_config);

	/* Apply TA clock configuration based on DT property */
	switch (config->clock_frequency) {
	case 80000000:
		/* no configuration bit needed */
		*enable_pll = false;
		break;
	case 120000000:
		boot_config.custom_feature_bit_map |= SL_SI91X_CUSTOM_FEAT_SOC_CLK_CONFIG_120MHZ;
		*enable_pll = true;
		break;
	case 160000000:
		boot_config.custom_feature_bit_map |= SL_SI91X_CUSTOM_FEAT_SOC_CLK_CONFIG_160MHZ;
		*enable_pll = true;
		break;
	default:
		__ASSERT(0, "Corrupted DT configuration");
		break;
	}

	switch (wifi_oper_mode) {
	case WIFI_STA_MODE:
		siwx91x_apply_sta_mode(&boot_config);
		break;
	case WIFI_SOFTAP_MODE:
		siwx91x_apply_ap_mode(&boot_config, hidden_ssid, max_num_sta);
		break;
	default:
		return -EINVAL;
	}
	if (boot_config.coex_mode == SL_SI91X_WLAN_BLE_MODE) {
		*enable_pll = false;
	}

	if (IS_ENABLED(CONFIG_WIFI_SILABS_SIWX91X)) {
		siwx91x_apply_network_features(&boot_config, wifi_oper_mode);
	}

	memcpy(get_config, &boot_config, sizeof(boot_config));
	return 0;
}

int siwx91x_nwp_reset(const struct device *dev, uint8_t oper_mode,
			bool hidden_ssid, uint8_t max_num_sta)
{
	sl_wifi_system_boot_configuration_t nwp_config;
	bool enable_pll = false;
	int ret;

	ret = siwx91x_nwp_compute_config(dev, &nwp_config, &enable_pll,
					 oper_mode, hidden_ssid, max_num_sta);
	if (ret) {
		return ret;
	}

	siwx91x_nwp_tx_flush_lock(dev);
	/* siwx91x_nwp_soft_reset() is not required for the initial boot, but this allow to use the
	 * same code than for reset.
	 */
	siwx91x_nwp_soft_reset(dev);
	siwx91x_nwp_opermode(dev, &nwp_config);
	siwx91x_nwp_dynamic_pool(dev, 1, 1, 1);
	siwx91x_nwp_feature(dev, enable_pll);
	siwx91x_nwp_tx_unlock(dev);

	return 0;
}

int siwx91x_nwp_apply_power_profile(const struct device *dev)
{
	struct siwx91x_nwp_data *data = dev->data;
	sl_wifi_performance_profile_v2_t performance_profile = {
		.profile = data->power_profile
	};
	sl_bt_performance_profile_t bt_performance_profile = {
		.profile = data->power_profile
	};
	int ret;

	if (!IS_ENABLED(CONFIG_SOC_SIWX91X_PM_BACKEND_PMGR)) {
		/* no_op if PM is not enabled*/
		return 0;
	}

	if (IS_ENABLED(CONFIG_BT_SILABS_SIWX91X)) {
		ret = sl_si91x_bt_set_performance_profile(&bt_performance_profile);
		if (ret) {
			LOG_ERR("Failed to initiate power save in BLE mode");
			return -EINVAL;
		}
	}

	ret = sl_wifi_set_performance_profile_v2(&performance_profile);
	if (ret) {
		return -EINVAL;
	}

	/* Remove the previously added PS4 power state requirement */
	sl_si91x_power_manager_remove_ps_requirement(SL_SI91X_POWER_MANAGER_PS4);

	return 0;
}

static int siwx91x_check_nwp_version(const struct device *dev)
{
	struct siwx91x_nwp_version version;

	siwx91x_nwp_get_firmware_version(dev, &version);

	LOG_INF("Get NWP firmware version: %X.%d.%d.%d.%d.%d.%d",
		version.rom_id,
		version.major,
		version.minor,
		version.security_version,
		version.patch_num,
		version.customer_id,
		version.build_num);

	/* Ignore rom_id:
	 * the right value is 0x0B but we received 17 in version.rom_id, we suspect a double
	 * hex->decimal conversion
	 */
	if (siwx91x_nwp_fw_expected_version.major != version.major ||
	    siwx91x_nwp_fw_expected_version.minor != version.minor ||
	    siwx91x_nwp_fw_expected_version.security_version != version.security_version ||
	    siwx91x_nwp_fw_expected_version.patch_num != version.patch_num) {
		LOG_ERR("Unexpected NWP firmware version (expected: %X.%d.%d.%d.%d.%d.%d)",
			siwx91x_nwp_fw_expected_version.rom_id,
			siwx91x_nwp_fw_expected_version.major,
			siwx91x_nwp_fw_expected_version.minor,
			siwx91x_nwp_fw_expected_version.security_version,
			siwx91x_nwp_fw_expected_version.patch_num,
			siwx91x_nwp_fw_expected_version.customer_id,
			siwx91x_nwp_fw_expected_version.build_num);
		return -EINVAL;
	}
	if (siwx91x_nwp_fw_expected_version.customer_id != version.customer_id) {
		LOG_DBG("customer_id diverge: expected %d, actual %d",
			siwx91x_nwp_fw_expected_version.customer_id, version.customer_id);
	}
	if (siwx91x_nwp_fw_expected_version.build_num != version.build_num) {
		LOG_DBG("build_num diverge: expected %d, actual %d",
			siwx91x_nwp_fw_expected_version.build_num, version.build_num);
	}

	return 0;
}

static int siwx91x_nwp_init(const struct device *dev)
{
	const struct siwx91x_nwp_config *config = dev->config;
	struct siwx91x_nwp_data *data = dev->data;
	struct net_buf_pool *rx_buf_pool;
	int ret, i;

	if (!config->rx_pool) {
		LOG_INF("Use network Rx net_buf pool");
	}
	k_sem_init(&data->global_lock, 1, 1);
	k_sem_init(&data->firmware_ready, 0, 1);
	k_sem_init(&data->tx_data_complete, 0, 1);
	k_sem_init(&data->rx_data_complete, 0, 1);
	k_sem_init(&data->refresh_queues_state, 0, 1);
	data->cmd_queues[0].id = SLI_WLAN_MGMT_Q;
	data->cmd_queues[1].id = SLI_WLAN_DATA_Q;
	data->cmd_queues[2].id = SLI_BT_Q;
	for (i = 0; i < ARRAY_SIZE(data->cmd_queues); i++) {
		k_fifo_init(&data->cmd_queues[i].tx_queue);
		k_sem_init(&data->cmd_queues[i].tx_done, 0, 1);
		k_mutex_init(&data->cmd_queues[i].sync_frame_in_queue);
	}

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0 && ret != -ENOENT) {
		return ret;
	}
	if (config->antenna_selection == 2 && ret == -ENOENT) {
		LOG_WRN("'ext-gpios' expects some pinctrl configuration");
	}

	config->config_irq(dev);
#if CONFIG_NET_BUF_DATA_SIZE < SIWX91X_MAX_PAYLOAD_SIZE
	__ASSERT_NO_MSG(config->rx_pool);
	rx_buf_pool = config->rx_pool;
#else
	__ASSERT_NO_MSG(!config->rx_pool);
	net_pkt_get_info(NULL, NULL, &rx_buf_pool, NULL);
#endif
	k_thread_create(&data->thread_id, config->thread_stack, config->thread_stack_size,
			siwx91x_nwp_thread, (void *)dev, rx_buf_pool, NULL,
			config->thread_priority, 0, K_NO_WAIT);

	ret = siwx91x_nwp_fw_boot(dev);
	if (ret) {
		return ret;
	}
	siwx91x_nwp_fw_reset(dev);
	ret = k_sem_take(&data->firmware_ready, K_SECONDS(5));
	if (ret) {
		return -EIO;
	}
	ret = siwx91x_nwp_reset(dev, WIFI_STA_MODE, false, 0);
	if (ret) {
		return ret;
	}
	ret = siwx91x_check_nwp_version(dev);
	if (ret) {
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_SILABS_SIWX91X) || IS_ENABLED(CONFIG_WIFI_SILABS_SIWX91X)) {
		data->power_profile = ASSOCIATED_POWER_SAVE;
	}
	/* WORKAROUND:
	 * Only set the power profile if Bluetooth is not enabled.
	 *
	 * If bt is enabled, we need to wait for the bt setup to complete
	 * before setting the power profile.
	 *
	 * Because of that, if CONFIG_BT_SILABS_SIWX91X is enabled and
	 * bt_enable() is not called, you will never go in sleep.
	 */
	if (!IS_ENABLED(CONFIG_BT_SILABS_SIWX91X)) {
		ret = siwx91x_nwp_apply_power_profile(dev);
		if (ret) {
			return -EINVAL;
		}
	}

	return 0;
}

#if defined(CONFIG_MBEDTLS_INIT)
BUILD_ASSERT(CONFIG_SIWX91X_NWP_INIT_PRIORITY < CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
	     "mbed TLS must be initialized after the NWP.");
#endif

/* Bounce buffer used to flatten fragmented buffers on Tx */
NET_BUF_POOL_FIXED_DEFINE(siwx91x_nwp_tx_pool, 1, SIWX91X_MAX_PAYLOAD_SIZE, 0, NULL);

/* Bounce buffer for Rx */
#if CONFIG_NET_BUF_DATA_SIZE < SIWX91X_MAX_PAYLOAD_SIZE
NET_BUF_POOL_FIXED_DEFINE(siwx91x_nwp_rx_pool, 2, SIWX91X_MAX_PAYLOAD_SIZE, 0, NULL);
#define SIWX91X_NWP_RX_POOL_PTR &siwx91x_nwp_rx_pool
#else
#define SIWX91X_NWP_RX_POOL_PTR NULL
#endif

#define SIWX91X_NWP_DEFINE(inst)                                                                   \
	static K_KERNEL_STACK_DEFINE(siwx91x_nwp_thread_stack##inst,                               \
				     CONFIG_SIWX91X_NWP_THREAD_STACK_SIZE);                        \
                                                                                                   \
	static void siwx91x_nwp_irq_setup##inst(const struct device *dev)                          \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority),                       \
			    siwx91x_nwp_isr, DEVICE_DT_INST_GET(inst), 0);                         \
		irq_enable(DT_INST_IRQN(inst));                                                    \
	};                                                                                         \
                                                                                                   \
	static struct siwx91x_nwp_data siwx91x_nwp_data##inst = {                                  \
		.power_profile = DEEP_SLEEP_WITH_RAM_RETENTION,                                    \
	};                                                                                         \
												   \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
	static const struct siwx91x_nwp_config siwx91x_nwp_config##inst = {                        \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                      \
		.config_irq = siwx91x_nwp_irq_setup##inst,                                         \
		.ta_regs = (struct siwx91x_nwp_ta_regs *)DT_INST_REG_ADDR_BY_NAME(inst, ta),       \
		.m4_regs = (struct siwx91x_nwp_m4_regs *)DT_INST_REG_ADDR_BY_NAME(inst, m4),       \
		.support_1p8v = DT_INST_PROP(inst, support_1p8v),                                  \
		.enable_xtal_correction = DT_INST_PROP(inst, enable_xtal_correction),              \
		.qspi_80mhz_clk = DT_INST_PROP(inst, qspi_80mhz_clk),                              \
		.antenna_selection = DT_INST_ENUM_IDX(inst, antenna_selection),                    \
		.clock_frequency = DT_INST_PROP(inst, clock_frequency),                            \
		.rx_pool = SIWX91X_NWP_RX_POOL_PTR,                                                \
		.tx_pool = &siwx91x_nwp_tx_pool,                                                   \
		.thread_stack = siwx91x_nwp_thread_stack##inst,                                    \
		.thread_stack_size = CONFIG_SIWX91X_NWP_THREAD_STACK_SIZE,                         \
		.thread_priority = CONFIG_SIWX91X_NWP_THREAD_PRIO,                                 \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &siwx91x_nwp_init, NULL, &siwx91x_nwp_data##inst,              \
			      &siwx91x_nwp_config##inst, POST_KERNEL,                              \
			      CONFIG_SIWX91X_NWP_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(SIWX91X_NWP_DEFINE)
