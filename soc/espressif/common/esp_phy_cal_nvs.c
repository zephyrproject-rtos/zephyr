/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Persist ESP32 radio PHY RF calibration data across reboots.
 *
 * The PHY code (esp_phy_load_cal_and_init) expects these NVS hooks to load and
 * store calibration data so that only a partial calibration is done on a warm
 * boot. The HAL ships no-op weak stubs, which forces a calibration against
 * empty data on every boot and makes the first connection after a cold boot or
 * reflash unreliable. These strong definitions back the hooks with the generic
 * radio calibration storage facility. The PHY is shared by Wi-Fi, BLE and
 * 802.15.4, so this serves whichever radio brings the PHY up first.
 */

#include <inttypes.h>
#include <string.h>
#include <zephyr/sys/util.h>
#include <zephyr/radio/radio_cal.h>
#include <zephyr/logging/log.h>

#include "esp_phy_init.h"
#include "esp_private/phy.h"
#include "esp_mac.h"

LOG_MODULE_REGISTER(esp_phy_cal, CONFIG_RADIO_CAL_LOG_LEVEL);

#define PHY_CAL_DATA_KEY    "esp_phy_cal_data"
#define PHY_CAL_VERSION_KEY "esp_phy_cal_ver"
#define PHY_CAL_MAC_KEY     "esp_phy_cal_mac"

esp_err_t esp_phy_load_cal_data_from_nvs(esp_phy_calibration_data_t *out_cal_data)
{
	uint32_t cal_version;
	uint32_t cur_version;
	uint8_t cal_mac[6];
	uint8_t cur_mac[6];
	int rc;

	if (out_cal_data == NULL) {
		return ESP_ERR_INVALID_ARG;
	}

	rc = radio_cal_load(PHY_CAL_VERSION_KEY, &cal_version, sizeof(cal_version));
	if (rc != 0) {
		LOG_INF("no stored RF calibration data (%d), full calibration needed", rc);
		return ESP_FAIL;
	}

	cur_version = phy_get_rf_cal_version() & (~BIT(16));
	if (cal_version != cur_version) {
		LOG_INF("RF cal version mismatch (stored %" PRIu32 ", current %" PRIu32
			"), full calibration needed", cal_version, cur_version);
		return ESP_FAIL;
	}

	rc = radio_cal_load(PHY_CAL_MAC_KEY, cal_mac, sizeof(cal_mac));
	if (rc != 0) {
		LOG_INF("no stored RF cal MAC (%d), full calibration needed", rc);
		return ESP_FAIL;
	}

	if (esp_efuse_mac_get_default(cur_mac) != ESP_OK) {
		return ESP_FAIL;
	}

	if (memcmp(cal_mac, cur_mac, sizeof(cur_mac)) != 0) {
		LOG_INF("RF cal MAC mismatch, full calibration needed");
		return ESP_FAIL;
	}

	rc = radio_cal_load(PHY_CAL_DATA_KEY, out_cal_data, sizeof(*out_cal_data));
	if (rc != 0) {
		LOG_INF("RF cal data load failed (%d), full calibration needed", rc);
		return ESP_FAIL;
	}

	LOG_INF("RF calibration data loaded from NVS (partial calibration)");
	return ESP_OK;
}

esp_err_t esp_phy_store_cal_data_to_nvs(const esp_phy_calibration_data_t *cal_data)
{
	uint32_t cal_version;
	uint8_t cal_mac[6];
	int rc;

	if (cal_data == NULL) {
		return ESP_ERR_INVALID_ARG;
	}

	rc = radio_cal_store(PHY_CAL_DATA_KEY, cal_data, sizeof(*cal_data));
	if (rc != 0) {
		return ESP_FAIL;
	}

	if (esp_efuse_mac_get_default(cal_mac) != ESP_OK) {
		return ESP_FAIL;
	}

	rc = radio_cal_store(PHY_CAL_MAC_KEY, cal_mac, sizeof(cal_mac));
	if (rc != 0) {
		return ESP_FAIL;
	}

	cal_version = phy_get_rf_cal_version() & (~BIT(16));
	rc = radio_cal_store(PHY_CAL_VERSION_KEY, &cal_version, sizeof(cal_version));
	if (rc != 0) {
		return ESP_FAIL;
	}

	LOG_INF("RF calibration data stored to NVS");
	return ESP_OK;
}

esp_err_t esp_phy_erase_cal_data_in_nvs(void)
{
	(void)radio_cal_erase(PHY_CAL_DATA_KEY);
	(void)radio_cal_erase(PHY_CAL_MAC_KEY);
	(void)radio_cal_erase(PHY_CAL_VERSION_KEY);

	LOG_INF("RF calibration data erased from NVS");
	return ESP_OK;
}
