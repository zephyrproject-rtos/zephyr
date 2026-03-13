/**
 * @file
 * @brief SiWx91x NCP NWP driver over SPI.
 *
 * This file implements the NWP (Network Wireless Processor) driver for the
 * SiWx91x in NCP (Network Co-Processor) mode. The SiWx91x module is
 * connected to the host MCU via SPI with GPIO signals for reset and
 * interrupt handshaking.
 *
 * This driver:
 * 1. Implements sl_si91x_host_interface.h (platform abstraction for SPI/GPIO)
 * 2. Performs NWP device initialization (reset, boot, sl_wifi_init)
 * 3. Exports siwx91x_nwp_mode_switch() and country code helpers for the
 *    bus-agnostic WiFi driver (siwx91x_wifi.c)
 *
 * Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT silabs_siwx91x_nwp_spi

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/wifi.h>
#include <zephyr/sys/util.h>

#include "siwx91x_nwp.h"
#include "nwp_fw_version.h"
#include "sl_wifi_callback_framework.h"
#include "sl_si91x_host_interface.h"

#ifdef CONFIG_BT_SILABS_SIWX91X
#include "rsi_ble_common_config.h"
#endif

LOG_MODULE_REGISTER(siwx91x_nwp_ncp, 4);

/* ========================================================================== */
/* DT-driven hardware configuration                                           */
/* ========================================================================== */

#define SIWX91X_NWP_NCP_INST(inst)  DT_DRV_INST(inst)

struct siwx91x_ncp_config {
	struct spi_dt_spec spi;
	struct gpio_dt_spec reset_gpio;
	struct gpio_dt_spec irq_gpio;
	struct gpio_dt_spec cs_gpio;
#if DT_INST_NODE_HAS_PROP(0, sleep_request_gpios)
	struct gpio_dt_spec sleep_gpio;
#endif
#if DT_INST_NODE_HAS_PROP(0, wake_indicator_gpios)
	struct gpio_dt_spec wake_gpio;
#endif
};

struct siwx91x_ncp_data {
	char current_country_code[WIFI_COUNTRY_CODE_LEN];
	sl_si91x_host_rx_irq_handler rx_irq_handler;
	struct gpio_callback irq_cb_data;
	struct spi_config spi_cfg;
	volatile bool bus_irq_enabled;
};

/* We support exactly one instance (singleton) */
BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) <= 1,
	     "Only one SiWx91x NCP SPI instance is supported");

/* Pointer to the singleton device — needed by the sl_si91x_host_*() functions
 * which have no device parameter (they are global platform callbacks).
 */
static const struct device *ncp_dev_instance;

/* ========================================================================== */
/* Country code / region mapping (shared with SoC NWP driver)                  */
/* ========================================================================== */

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
	struct siwx91x_ncp_data *data = dev->data;

	memcpy(data->current_country_code, country_code, WIFI_COUNTRY_CODE_LEN);
	return 0;
}

const char *siwx91x_get_country_code(const struct device *dev)
{
	const struct siwx91x_ncp_data *data = dev->data;

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

/* ========================================================================== */
/* sl_si91x_host_interface.h implementation                                   */
/* ========================================================================== */

static void siwx91x_ncp_irq_callback(const struct device *port, struct gpio_callback *cb,
				      gpio_port_pins_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	if (ncp_dev_instance == NULL) {
		return;
	}

	struct siwx91x_ncp_data *data = ncp_dev_instance->data;

	if (data->bus_irq_enabled && data->rx_irq_handler != NULL) {
		data->rx_irq_handler();
	}
}

sl_status_t sl_si91x_host_init(const sl_si91x_host_init_configuration_t *config)
{
	if (ncp_dev_instance == NULL) {
		return SL_STATUS_NOT_INITIALIZED;
	}

	struct siwx91x_ncp_data *data = ncp_dev_instance->data;
	const struct siwx91x_ncp_config *cfg = ncp_dev_instance->config;
	int ret;

	if (config != NULL) {
		data->rx_irq_handler = config->rx_irq;
	}

	/* Verify SPI device is ready */
	if (!spi_is_ready_dt(&cfg->spi)) {
		LOG_ERR("SPI bus not ready");
		return SL_STATUS_NOT_INITIALIZED;
	}

	/* Configure RESET GPIO */
	if (!gpio_is_ready_dt(&cfg->reset_gpio)) {
		LOG_ERR("Reset GPIO device not ready");
		return SL_STATUS_NOT_INITIALIZED;
	}
	ret = gpio_pin_configure_dt(&cfg->reset_gpio, GPIO_OUTPUT_INACTIVE | GPIO_OPEN_DRAIN);
	if (ret < 0) {
		LOG_ERR("Failed to configure reset GPIO: %d", ret);
		return SL_STATUS_FAIL;
	}

	/* Configure CS GPIO */
	if (!gpio_is_ready_dt(&cfg->cs_gpio)) {
		LOG_ERR("CS GPIO device not ready");
		return SL_STATUS_NOT_INITIALIZED;
	}
	ret = gpio_pin_configure_dt(&cfg->cs_gpio, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure CS GPIO: %d", ret);
		return SL_STATUS_FAIL;
	}

	if (!gpio_is_ready_dt(&cfg->sleep_gpio)) {
		LOG_ERR("Sleep GPIO device not ready");
		return SL_STATUS_NOT_INITIALIZED;
	}
	ret = gpio_pin_configure_dt(&cfg->sleep_gpio, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure Sleep GPIO: %d", ret);
		return SL_STATUS_FAIL;
	}

	if (!gpio_is_ready_dt(&cfg->wake_gpio)) {
		LOG_ERR("Wake GPIO device not ready");
		return SL_STATUS_NOT_INITIALIZED;
	}
	ret = gpio_pin_configure_dt(&cfg->wake_gpio, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure Wake GPIO: %d", ret);
		return SL_STATUS_FAIL;
	}

	/* Configure IRQ GPIO as input with interrupt on rising edge */
	if (!gpio_is_ready_dt(&cfg->irq_gpio)) {
		LOG_ERR("IRQ GPIO device not ready");
		return SL_STATUS_NOT_INITIALIZED;
	}
	ret = gpio_pin_configure_dt(&cfg->irq_gpio, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure IRQ GPIO: %d", ret);
		return SL_STATUS_FAIL;
	}

	ret = gpio_pin_interrupt_configure_dt(&cfg->irq_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure IRQ GPIO interrupt: %d", ret);
		return SL_STATUS_FAIL;
	}

	gpio_init_callback(&data->irq_cb_data, siwx91x_ncp_irq_callback,
			   BIT(cfg->irq_gpio.pin));
	ret = gpio_add_callback(cfg->irq_gpio.port, &data->irq_cb_data);
	if (ret < 0) {
		LOG_ERR("Failed to add IRQ GPIO callback: %d", ret);
		return SL_STATUS_FAIL;
	}

	data->bus_irq_enabled = false;

	LOG_INF("SiWx91x NCP SPI host initialized (freq: %u Hz)",
		data->spi_cfg.frequency);

	return SL_STATUS_OK;
}

sl_status_t sl_si91x_host_deinit(void)
{
	if (ncp_dev_instance == NULL) {
		return SL_STATUS_NOT_INITIALIZED;
	}

	struct siwx91x_ncp_data *data = ncp_dev_instance->data;
	const struct siwx91x_ncp_config *cfg = ncp_dev_instance->config;

	data->bus_irq_enabled = false;
	gpio_pin_interrupt_configure_dt(&cfg->irq_gpio, GPIO_INT_DISABLE);
	gpio_remove_callback(cfg->irq_gpio.port, &data->irq_cb_data);

	LOG_INF("SiWx91x NCP SPI host deinitialized");
	return SL_STATUS_OK;
}

/* === SPI Transfer === */

sl_status_t sl_si91x_host_spi_transfer(const void *tx_buffer, void *rx_buffer,
				       uint16_t buffer_length)
{
	if (ncp_dev_instance == NULL) {
		return SL_STATUS_NOT_INITIALIZED;
	}

	struct siwx91x_ncp_data *data = ncp_dev_instance->data;
	const struct siwx91x_ncp_config *cfg = ncp_dev_instance->config;
	int ret;

	struct spi_buf tx_buf = {
		.buf = (void *)tx_buffer,
		.len = buffer_length,
	};
	struct spi_buf_set tx_set = {
		.buffers = &tx_buf,
		.count = 1,
	};

	struct spi_buf rx_buf = {
		.buf = rx_buffer,
		.len = buffer_length,
	};
	struct spi_buf_set rx_set = {
		.buffers = &rx_buf,
		.count = 1,
	};

	/*
	 * CS is managed by the WiseConnect NCP protocol layer via
	 * sl_si91x_host_spi_cs_assert/deassert — not here.
	 */
	if (tx_buffer != NULL && rx_buffer != NULL) {
		ret = spi_transceive(cfg->spi.bus, &data->spi_cfg, &tx_set, &rx_set);
	} else if (tx_buffer != NULL) {
		ret = spi_write(cfg->spi.bus, &data->spi_cfg, &tx_set);
	} else {
		ret = spi_read(cfg->spi.bus, &data->spi_cfg, &rx_set);
	}

	if (ret < 0) {
		LOG_ERR("SPI transfer failed: %d (len=%u)", ret, buffer_length);
		return SL_STATUS_FAIL;
	}

	return SL_STATUS_OK;
}

/* === Chip Select === */

void sl_si91x_host_spi_cs_assert(void)
{
	if (ncp_dev_instance == NULL) {
		return;
	}
	const struct siwx91x_ncp_config *cfg = ncp_dev_instance->config;

	/* CS is on the SPI bus cs-gpios — assert via GPIO for manual control */
	gpio_pin_set_dt(&cfg->cs_gpio, 1);
}

void sl_si91x_host_spi_cs_deassert(void)
{
	if (ncp_dev_instance == NULL) {
		return;
	}
	const struct siwx91x_ncp_config *cfg = ncp_dev_instance->config;

	gpio_pin_set_dt(&cfg->cs_gpio, 0);
}

/* === Reset Control === */

void sl_si91x_host_hold_in_reset(void)
{
	if (ncp_dev_instance == NULL) {
		return;
	}
	const struct siwx91x_ncp_config *cfg = ncp_dev_instance->config;

	gpio_pin_set_dt(&cfg->reset_gpio, 1); /* Assert reset (active-low handled by DT flags) */
	LOG_DBG("SiWx91x held in reset");
}

void sl_si91x_host_release_from_reset(void)
{
	if (ncp_dev_instance == NULL) {
		return;
	}
	const struct siwx91x_ncp_config *cfg = ncp_dev_instance->config;

	gpio_pin_set_dt(&cfg->reset_gpio, 0); /* Deassert reset */
	LOG_DBG("SiWx91x released from reset");
}

/* === Bus Interrupt Control === */

void sl_si91x_host_enable_bus_interrupt(void)
{
	if (ncp_dev_instance == NULL) {
		return;
	}
	struct siwx91x_ncp_data *data = ncp_dev_instance->data;
	const struct siwx91x_ncp_config *cfg = ncp_dev_instance->config;

	data->bus_irq_enabled = true;
	gpio_pin_interrupt_configure_dt(&cfg->irq_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	LOG_DBG("Bus interrupt enabled");
}

void sl_si91x_host_disable_bus_interrupt(void)
{
	if (ncp_dev_instance == NULL) {
		return;
	}
	struct siwx91x_ncp_data *data = ncp_dev_instance->data;
	const struct siwx91x_ncp_config *cfg = ncp_dev_instance->config;

	data->bus_irq_enabled = false;
	gpio_pin_interrupt_configure_dt(&cfg->irq_gpio, GPIO_INT_DISABLE);
	LOG_DBG("Bus interrupt disabled");
}

/* === High Speed Bus === */

void sl_si91x_host_enable_high_speed_bus(void)
{
	if (ncp_dev_instance == NULL) {
		return;
	}
	struct siwx91x_ncp_data *data = ncp_dev_instance->data;

	data->spi_cfg.frequency = CONFIG_SIWX91X_NCP_SPI_HIGH_SPEED_FREQUENCY;
	LOG_INF("SPI switched to high-speed: %u Hz", data->spi_cfg.frequency);
}

/* === Sleep / Wake Indicators === */

void sl_si91x_host_set_sleep_indicator(void)
{
// #if DT_INST_NODE_HAS_PROP(0, sleep_request_gpios)
// 	if (ncp_dev_instance != NULL) {
// 		const struct siwx91x_ncp_config *cfg = ncp_dev_instance->config;
// 		LOG_INF("SET SLEEP INDICATOR");
// 		gpio_pin_set_dt(&cfg->sleep_gpio, 1);
// 	}
// #endif
}

void sl_si91x_host_clear_sleep_indicator(void)
{
// #if DT_INST_NODE_HAS_PROP(0, sleep_request_gpios)
// 	if (ncp_dev_instance != NULL) {
// 		const struct siwx91x_ncp_config *cfg = ncp_dev_instance->config;
// 		LOG_INF("CLEAR SLEEP INDICATOR");
// 		gpio_pin_set_dt(&cfg->sleep_gpio, 0);
// 	}
// 	LOG_DBG("Sleep indicator cleared");
// #endif
}

uint32_t sl_si91x_host_get_wake_indicator(void)
{
// #if DT_INST_NODE_HAS_PROP(0, wake_indicator_gpios)
// 	if (ncp_dev_instance != NULL) {
// 		const struct siwx91x_ncp_config *cfg = ncp_dev_instance->config;
// 		// LOG_INF("GET WAKE INDICATOR");
// 		return gpio_pin_get_dt(&cfg->wake_gpio);
// 	}
// #endif
	/* Default: always awake */
	return 1;
}

/* === UART stubs (unused in SPI mode) === */

sl_status_t sl_si91x_host_uart_transfer(const void *tx_buffer, void *rx_buffer,
					uint16_t buffer_length)
{
	ARG_UNUSED(tx_buffer);
	ARG_UNUSED(rx_buffer);
	ARG_UNUSED(buffer_length);
	return SL_STATUS_NOT_SUPPORTED;
}

void sl_si91x_host_flush_uart_rx(void)
{
	/* Not used in SPI mode */
}

void sl_si91x_host_uart_enable_hardware_flow_control(void)
{
	/* Not used in SPI mode */
}

/* === Context Check === */

bool sl_si91x_host_is_in_irq_context(void)
{
	return k_is_in_isr();
}

/* ========================================================================== */
/* NWP configuration and boot                                                 */
/* ========================================================================== */

#define AP_MAX_NUM_STA 4

static void siwx91x_ncp_configure_sta_mode(sl_si91x_boot_configuration_t *boot_config)
{
	const bool wifi_enabled = IS_ENABLED(CONFIG_WIFI_SILABS_SIWX91X);
	const bool bt_enabled = IS_ENABLED(CONFIG_BT_SILABS_SIWX91X);

	boot_config->oper_mode = SL_SI91X_CLIENT_MODE;

	if (IS_ENABLED(CONFIG_WIFI_SILABS_SIWX91X_ROAMING_USE_DEAUTH)) {
		boot_config->custom_feature_bit_map |=
			SL_SI91X_CUSTOM_FEAT_ROAM_WITH_DEAUTH_OR_NULL_DATA;
	}

	if (wifi_enabled && bt_enabled) {
		LOG_ERR("WiFi + Bluetooth coexistence is not fully supported in STA mode; using WLAN_BLE_MODE which has limited BT features");
		boot_config->coex_mode = SL_SI91X_WLAN_BLE_MODE;
	} else if (wifi_enabled) {
		boot_config->coex_mode = SL_SI91X_WLAN_ONLY_MODE;
	} else if (bt_enabled) {
		boot_config->coex_mode = SL_SI91X_BLE_MODE;
	} else {
		boot_config->coex_mode = SL_SI91X_WLAN_ONLY_MODE;
	}

#ifdef CONFIG_WIFI_SILABS_SIWX91X
	/* 802.11W (management frame protection) — needed for WPA3 */
	boot_config->ext_custom_feature_bit_map |= SL_SI91X_EXT_FEAT_IEEE_80211W | SL_SI91X_EXT_FEAT_FRONT_END_SWITCH_PINS_ULP_GPIO_4_5_0;

	if (IS_ENABLED(CONFIG_WIFI_SILABS_SIWX91X_ENHANCED_MAX_PSP)) {
		boot_config->config_feature_bit_map |= SL_SI91X_CONFIG_FEAT_EXTENSION_VALID
						     | SL_SI91X_ENABLE_ENHANCED_MAX_PSP;
	}
#endif

#ifdef CONFIG_BT_SILABS_SIWX91X
	/* BLE feature bitmaps for NCP + Zephyr HCI host (stack bypass mode).
	 *
	 * SL_SI91X_BT_BLE_STACK_BYPASS_ENABLE (ble_ext_feature_bit_map bit 24):
	 *   REQUIRED — tells the NWP to bypass its internal BLE stack and pass
	 *   raw HCI frames to/from the host MCU via rsi_bt_driver_send_cmd /
	 *   RSI_BLE_ON_RCP_EVENT.  This is what allows Zephyr's BT host to drive
	 *   the SiWx917 controller directly over SPI.
	 *
	 * ATT record / service counts (ble_feature_bit_map bits 0–11):
	 *   Must be non-zero even in bypass mode — the NWP still allocates GATT
	 *   table memory from these values before handing control to the host.
	 *   Zero values cause silent GATT failures or error 0x10063.
	 */
	boot_config->ext_custom_feature_bit_map |= SL_SI91X_EXT_FEAT_BT_CUSTOM_FEAT_ENABLE;
	boot_config->bt_feature_bit_map |= SL_SI91X_BT_RF_TYPE | SL_SI91X_ENABLE_BLE_PROTOCOL;
	boot_config->ble_feature_bit_map |=
		SL_SI91X_BLE_MAX_NBR_PERIPHERALS(RSI_BLE_MAX_NBR_PERIPHERALS) |
		SL_SI91X_BLE_MAX_NBR_CENTRALS(RSI_BLE_MAX_NBR_CENTRALS) |
		SL_SI91X_BLE_MAX_NBR_ATT_SERV(RSI_BLE_MAX_NBR_ATT_SERV) |
		SL_SI91X_BLE_MAX_NBR_ATT_REC(RSI_BLE_MAX_NBR_ATT_REC) |
		SL_SI91X_BLE_PWR_INX(RSI_BLE_PWR_INX) |
		SL_SI91X_916_BLE_COMPATIBLE_FEAT_ENABLE |
		SL_SI91X_FEAT_BLE_CUSTOM_FEAT_EXTENSION_VALID;

	boot_config->ble_ext_feature_bit_map |=
		SL_SI91X_BLE_NUM_CONN_EVENTS(RSI_BLE_NUM_CONN_EVENTS) |
		SL_SI91X_BLE_NUM_REC_BYTES(RSI_BLE_NUM_REC_BYTES) | SL_SI91X_BLE_ENABLE_ADV_EXTN |
		SL_SI91X_BLE_AE_MAX_ADV_SETS(RSI_BLE_AE_MAX_ADV_SETS) |
		SL_SI91X_BT_BLE_STACK_BYPASS_ENABLE;
#endif
}

static void siwx91x_ncp_configure_ap_mode(sl_si91x_boot_configuration_t *boot_config,
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
}

static void siwx91x_ncp_configure_network_stack(sl_si91x_boot_configuration_t *boot_config,
						 uint8_t wifi_oper_mode)
{
	if (!IS_ENABLED(CONFIG_WIFI_SILABS_SIWX91X_NET_STACK_OFFLOAD)) {
		/* Host manages the TCP/IP stack; NWP passes raw frames */
		boot_config->tcp_ip_feature_bit_map = SL_SI91X_TCP_IP_FEAT_BYPASS | SL_SI91X_TCP_IP_FEAT_EXTENSION_VALID;
		// boot_config->ext_tcp_ip_feature_bit_map = 0;
		return;
	}

	/* Offloaded stack — bits in the base config are the SDK defaults,
	 * add only mode-specific extras here.
	 */
	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		boot_config->tcp_ip_feature_bit_map |= SL_SI91X_TCP_IP_FEAT_IPV6;
		if (wifi_oper_mode == WIFI_STA_MODE) {
			boot_config->tcp_ip_feature_bit_map |= SL_SI91X_TCP_IP_FEAT_DHCPV6_CLIENT;
		} else if (wifi_oper_mode == WIFI_SOFTAP_MODE) {
			boot_config->tcp_ip_feature_bit_map |= SL_SI91X_TCP_IP_FEAT_DHCPV6_SERVER;
		}
	}

	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		if (wifi_oper_mode == WIFI_SOFTAP_MODE) {
			boot_config->tcp_ip_feature_bit_map |= SL_SI91X_TCP_IP_FEAT_DHCPV4_SERVER;
		}
		/* STA mode already has DHCPV4_CLIENT in the base config */
	}
}

static int siwx91x_ncp_get_config(const struct device *dev,
				   sl_wifi_device_configuration_t *get_config,
				   uint8_t wifi_oper_mode, bool hidden_ssid, uint8_t max_num_sta)
{
	/*
	 * Base configuration built from the SDK default
	 * sl_wifi_default_client_configuration (NCP path, i.e.
	 * !SLI_SI91X_MCU_INTERFACE).  Only Zephyr-specific Kconfig knobs
	 * should add bits on top of this baseline.
	 *
	 * MEMORY_CONFIG in NCP mode resolves to (BIT(20)|BIT(21)) which
	 * equals SL_SI91X_EXT_FEAT_672K_M4SS_0K — all SRAM goes to the
	 * NWP since the M4 is not executing user code.
	 */
	sl_wifi_device_configuration_t default_config = {
		.boot_option = LOAD_NWP_FW,
		.mac_address = NULL,
		.band        = SL_WIFI_BAND_MODE_2_4GHZ,
		.region_code = SL_WIFI_IGNORE_REGION,
		// .boot_config = {
		// 	.oper_mode              = SL_SI91X_CLIENT_MODE,
		// 	.coex_mode              = SL_SI91X_WLAN_ONLY_MODE,
		// 	.feature_bit_map        = (SL_SI91X_FEAT_SECURITY_OPEN | SL_SI91X_FEAT_SECURITY_PSK
		// 				 | SL_SI91X_FEAT_AGGREGATION | SL_SI91X_FEAT_ULP_GPIO_BASED_HANDSHAKE | SL_SI91X_FEAT_DEV_TO_HOST_ULP_GPIO_1),
		// 	.tcp_ip_feature_bit_map = SL_SI91X_TCP_IP_FEAT_BYPASS,
		// 	.custom_feature_bit_map = SL_SI91X_CUSTOM_FEAT_EXTENSION_VALID,
		// 	.ext_custom_feature_bit_map =
		// 		(SL_SI91X_EXT_FEAT_XTAL_CLK
		// 		| SL_SI91X_EXT_FEAT_UART_SEL_FOR_DEBUG_PRINTS
		// 		| SL_SI91X_EXT_FEAT_672K_M4SS_0K
		// 		| SL_SI91X_EXT_FEAT_FRONT_END_SWITCH_PINS_ULP_GPIO_4_5_0),
		// 	.bt_feature_bit_map         = 0,
		// 	.ext_tcp_ip_feature_bit_map = 0,
		// 	.ble_feature_bit_map        = 0,
		// 	.ble_ext_feature_bit_map    = 0,
		// 	.config_feature_bit_map     = SL_SI91X_FEAT_SLEEP_GPIO_SEL_BITMAP,
		// }
		.boot_config = {
			.oper_mode              = SL_SI91X_CLIENT_MODE,
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
				SL_SI91X_EXT_FEAT_UART_SEL_FOR_DEBUG_PRINTS |
				SL_SI91X_EXT_FEAT_NWP_QSPI_80MHZ_CLK_ENABLE |
				SL_SI91X_EXT_FEAT_672K_M4SS_0K |
				SL_SI91X_EXT_FEAT_FRONT_END_SWITCH_PINS_ULP_GPIO_4_5_0 |
				SL_SI91X_EXT_FEAT_FRONT_END_INTERNAL_SWITCH |
				SL_SI91X_EXT_FEAT_XTAL_CLK,
		}
	};

	sl_si91x_boot_configuration_t *boot_config = &default_config.boot_config;

	__ASSERT(get_config, "get_config cannot be NULL");
	__ASSERT((hidden_ssid == false && max_num_sta == 0) ||
		 wifi_oper_mode == WIFI_SOFTAP_MODE,
		 "hidden_ssid or max_num_sta requires SOFT AP mode");

	if (wifi_oper_mode == WIFI_SOFTAP_MODE && max_num_sta > AP_MAX_NUM_STA) {
		LOG_ERR("Exceeded maximum supported stations (%d)", AP_MAX_NUM_STA);
		return -EINVAL;
	}

	siwx91x_store_country_code(dev, DEFAULT_COUNTRY_CODE);

	switch (wifi_oper_mode) {
	case WIFI_STA_MODE:
		siwx91x_ncp_configure_sta_mode(boot_config);
		LOG_INF("Configured STA mode, coex_mode=%d, bt_feature_bit_map=0x%x, ble_feature_bit_map=0x%x, ble_ext_feature_bit_map=0x%x",
			boot_config->coex_mode, boot_config->bt_feature_bit_map, boot_config->ble_feature_bit_map, boot_config->ble_ext_feature_bit_map);
		break;
	case WIFI_SOFTAP_MODE:
		siwx91x_ncp_configure_ap_mode(boot_config, hidden_ssid, max_num_sta);
		break;
	default:
		return -EINVAL;
	}

	siwx91x_ncp_configure_network_stack(boot_config, wifi_oper_mode);
	

	memcpy(get_config, &default_config, sizeof(default_config));
	return 0;
}

static int siwx91x_ncp_check_fw_version(void)
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

	if (expected_version.major != version.major ||
	    expected_version.minor != version.minor ||
	    expected_version.security_version != version.security_version ||
	    expected_version.patch_num != version.patch_num) {
		return -EINVAL;
	}

	if (expected_version.customer_id != version.customer_id) {
		LOG_DBG("customer_id diverge: expected %d, actual %d",
			expected_version.customer_id, version.customer_id);
	}
	if (expected_version.build_num != version.build_num) {
		LOG_DBG("build_num diverge: expected %d, actual %d",
			expected_version.build_num, version.build_num);
	}

	return 0;
}

/* ========================================================================== */
/* NWP mode switch (called by siwx91x_wifi.c)                                 */
/* ========================================================================== */

int siwx91x_nwp_mode_switch(const struct device *dev, uint8_t oper_mode, bool hidden_ssid,
			    uint8_t max_num_sta)
{
	sl_wifi_device_configuration_t nwp_config;
	int status;

	status = siwx91x_ncp_get_config(dev, &nwp_config, oper_mode, hidden_ssid, max_num_sta);
	if (status < 0) {
		return status;
	}

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

/* ========================================================================== */
/* Device init                                                                 */
/* ========================================================================== */

static int siwx91x_nwp_ncp_init(const struct device *dev)
{
	struct siwx91x_ncp_data *data = dev->data;
	sl_wifi_device_configuration_t network_config;
	int ret;

	/* Store singleton reference for sl_si91x_host_*() callbacks */
	ncp_dev_instance = dev;

	/* Initialize SPI configuration from DT */
	const struct siwx91x_ncp_config *cfg = dev->config;

	data->spi_cfg = cfg->spi.config;
	data->spi_cfg.frequency = CONFIG_SIWX91X_NCP_SPI_FREQUENCY;
	data->spi_cfg.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB;

	/* Get NWP boot configuration */
	ret = siwx91x_ncp_get_config(dev, &network_config, WIFI_STA_MODE, false, 0);
	if (ret < 0) {
		LOG_ERR("Failed to get NWP config: %d", ret);
		return ret;
	}

	/* Boot the NWP via sl_wifi_init — this triggers the SPI handshake,
	 * firmware load/verify, and NWP initialization sequence through the
	 * WiseConnect SDK.
	 */
	ret = sl_wifi_init(&network_config, NULL, sl_wifi_default_event_handler);
	if (ret != SL_STATUS_OK) {
		LOG_ERR("sl_wifi_init failed: 0x%x", ret);
		return -EINVAL;
	}

	sl_wifi_performance_profile_t wifi_profile = { .profile = SL_WIFI_SYSTEM_HIGH_PERFORMANCE };
    ret = sl_wifi_set_performance_profile(&wifi_profile);
    if (ret != SL_STATUS_OK) {
        LOG_ERR("Failed to set WiFi High Performance mode: 0x%x", ret);
    }

#ifdef CONFIG_BT_SILABS_SIWX91X
    sl_bt_performance_profile_t bt_profile = { .profile = SL_WIFI_SYSTEM_HIGH_PERFORMANCE };
    // ret = sl_si91x_bt_set_performance_profile(&bt_profile);
    if (ret != SL_STATUS_OK) {
        LOG_ERR("Failed to set BT High Performance mode: 0x%x", ret);
    }
#endif

	/* Check firmware version */
	ret = siwx91x_ncp_check_fw_version();
	if (ret < 0) {
		LOG_ERR("Unexpected NWP firmware version (expected: %s)",
			SIWX91X_NWP_FW_EXPECTED_VERSION);
		/* Continue — version mismatch is a warning, not fatal */
	}

	LOG_INF("SiWx91x NCP NWP initialized successfully");
	return 0;
}

/* ========================================================================== */
/* Device instantiation                                                        */
/* ========================================================================== */

#define SIWX91X_NWP_NCP_DEFINE(inst)                                                          \
                                                                                               \
	static struct siwx91x_ncp_data siwx91x_ncp_data_##inst = {};                          \
                                                                                               \
	static const struct siwx91x_ncp_config siwx91x_ncp_config_##inst = {                   \
		.spi = SPI_DT_SPEC_INST_GET(inst,                                              \
			SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB),              \
		.reset_gpio = GPIO_DT_SPEC_INST_GET(inst, reset_gpios),                        \
		.cs_gpio = GPIO_DT_SPEC_INST_GET(inst, cs_gpios),                                \
		.irq_gpio = GPIO_DT_SPEC_INST_GET(inst, irq_gpios),                            \
		IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, sleep_request_gpios),                   \
			(.sleep_gpio = GPIO_DT_SPEC_INST_GET(inst, sleep_request_gpios),))     \
		IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, wake_indicator_gpios),                  \
			(.wake_gpio = GPIO_DT_SPEC_INST_GET(inst, wake_indicator_gpios),))     \
	};                                                                                     \
                                                                                               \
	DEVICE_DT_INST_DEFINE(inst, &siwx91x_nwp_ncp_init, NULL,                               \
			      &siwx91x_ncp_data_##inst, &siwx91x_ncp_config_##inst,            \
			      POST_KERNEL, CONFIG_SIWX91X_NCP_SPI_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(SIWX91X_NWP_NCP_DEFINE)
