/*
 * Copyright (c) 2023 Antmicro
 * Copyright (c) 2024-2025 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/logging/log.h>

#include <siwx91x_nwp.h>
#include "siwx91x_wifi.h"
#include "siwx91x_wifi_scan.h"

#include "sl_rsi_utility.h"

LOG_MODULE_DECLARE(siwx91x_wifi);

#define SIWX91X_DEFAULT_PASSIVE_SCAN_DWELL_TIME 400

static void siwx91x_report_scan_res(struct siwx91x_dev *sidev, sl_wifi_scan_result_t *result,
				    int item)
{
	static const struct {
		int sl_val;
		int z_val;
	} security_convert[] = {
		{ SL_WIFI_OPEN,            WIFI_SECURITY_TYPE_NONE    },
		{ SL_WIFI_WEP,             WIFI_SECURITY_TYPE_WEP     },
		{ SL_WIFI_WPA,             WIFI_SECURITY_TYPE_WPA_PSK },
		{ SL_WIFI_WPA2,            WIFI_SECURITY_TYPE_PSK     },
		{ SL_WIFI_WPA3,            WIFI_SECURITY_TYPE_SAE     },
		{ SL_WIFI_WPA3_TRANSITION, WIFI_SECURITY_TYPE_SAE     },
		{ SL_WIFI_WPA_ENTERPRISE,  WIFI_SECURITY_TYPE_EAP     },
		{ SL_WIFI_WPA2_ENTERPRISE, WIFI_SECURITY_TYPE_EAP     },
	};
	struct wifi_scan_result tmp = {
		.channel = result->scan_info[item].rf_channel,
		.rssi = result->scan_info[item].rssi_val,
		.ssid_length = strlen(result->scan_info[item].ssid),
		.mac_length = sizeof(result->scan_info[item].bssid),
		.security = WIFI_SECURITY_TYPE_UNKNOWN,
		.mfp = WIFI_MFP_UNKNOWN,
		.band = WIFI_FREQ_BAND_2_4_GHZ,
	};

	if (result->scan_count == 0) {
		return;
	}

	if (result->scan_info[item].rf_channel <= 0 || result->scan_info[item].rf_channel > 14) {
		LOG_WRN("Unexpected scan result");
		tmp.band = WIFI_FREQ_BAND_UNKNOWN;
	}

	memcpy(tmp.ssid, result->scan_info[item].ssid, tmp.ssid_length);
	memcpy(tmp.mac, result->scan_info[item].bssid, tmp.mac_length);

	ARRAY_FOR_EACH(security_convert, i) {
		if (security_convert[i].sl_val == result->scan_info[item].security_mode) {
			tmp.security = security_convert[i].z_val;
		}
	}

	sidev->scan_res_cb(sidev->iface, 0, &tmp);
}

unsigned int siwx91x_on_scan(sl_wifi_event_t event, sl_wifi_scan_result_t *result,
			     uint32_t result_size, void *arg)
{
	struct siwx91x_dev *sidev = arg;
	int i, scan_count;

	if (!sidev->scan_res_cb) {
		return -EFAULT;
	}

	if (event & SL_WIFI_EVENT_FAIL_INDICATION) {
		memset(result, 0, sizeof(*result));
	}

	if (sidev->scan_max_bss_cnt) {
		scan_count = MIN(result->scan_count, sidev->scan_max_bss_cnt);
	} else {
		scan_count = result->scan_count;
	}

	for (i = 0; i < scan_count; i++) {
		siwx91x_report_scan_res(sidev, result, i);
	}

	sidev->scan_res_cb(sidev->iface, 0, NULL);
	sidev->state = sidev->scan_prev_state;

	return 0;
}

static int
siwx91x_configure_scan_dwell_time(sl_wifi_scan_type_t scan_type, uint16_t dwell_time_active,
				  uint16_t dwell_time_passive,
				  sl_wifi_advanced_scan_configuration_t *advanced_scan_config)
{
	int ret;

	if (dwell_time_active && (dwell_time_active < 5 || dwell_time_active > 1000)) {
		LOG_ERR("Invalid active scan dwell time");
		return -EINVAL;
	}

	if (dwell_time_passive && (dwell_time_passive < 10 || dwell_time_passive > 1000)) {
		LOG_ERR("Invalid passive scan dwell time");
		return -EINVAL;
	}

	switch (scan_type) {
	case SL_WIFI_SCAN_TYPE_ACTIVE:
		ret = sl_si91x_configure_timeout(SL_SI91X_CHANNEL_ACTIVE_SCAN_TIMEOUT,
						 dwell_time_active);
		return ret;
	case SL_WIFI_SCAN_TYPE_PASSIVE:
		if (!dwell_time_passive) {
			dwell_time_passive = SIWX91X_DEFAULT_PASSIVE_SCAN_DWELL_TIME;
		}
		ret = sl_si91x_configure_timeout(SL_SI91X_CHANNEL_PASSIVE_SCAN_TIMEOUT,
						 dwell_time_passive);
		return ret;
	case SL_WIFI_SCAN_TYPE_ADV_SCAN:
		__ASSERT(advanced_scan_config, "advanced_scan_config cannot be NULL");

		if (!dwell_time_active) {
			dwell_time_active = CONFIG_WIFI_SILABS_SIWX91X_ADV_ACTIVE_SCAN_DURATION;
		}
		advanced_scan_config->active_channel_time = dwell_time_active;

		if (!dwell_time_passive) {
			dwell_time_passive = CONFIG_WIFI_SILABS_SIWX91X_ADV_PASSIVE_SCAN_DURATION;
		}
		advanced_scan_config->passive_channel_time = dwell_time_passive;
		return 0;
	default:
		return 0;
	}
}

int siwx91x_scan(const struct device *dev, struct wifi_scan_params *z_scan_config,
		 scan_result_cb_t cb)
{
	sl_wifi_interface_t interface = sl_wifi_get_default_interface();
	sl_wifi_scan_configuration_t sl_scan_config = { };
	sl_wifi_advanced_scan_configuration_t advanced_scan_config = {
		.trigger_level = CONFIG_WIFI_SILABS_SIWX91X_ADV_SCAN_THRESHOLD,
		.trigger_level_change = CONFIG_WIFI_SILABS_SIWX91X_ADV_RSSI_TOLERANCE_THRESHOLD,
		.enable_multi_probe = CONFIG_WIFI_SILABS_SIWX91X_ADV_MULTIPROBE,
		.enable_instant_scan = CONFIG_WIFI_SILABS_SIWX91X_ENABLE_INSTANT_SCAN,
	};
	sl_wifi_roam_configuration_t roam_configuration = {
#ifdef CONFIG_WIFI_SILABS_SIWX91X_ENABLE_ROAMING
		.trigger_level = CONFIG_WIFI_SILABS_SIWX91X_ROAMING_TRIGGER_LEVEL,
		.trigger_level_change = CONFIG_WIFI_SILABS_SIWX91X_ROAMING_TRIGGER_LEVEL_CHANGE,
#else
		.trigger_level = SL_WIFI_NEVER_ROAM,
		.trigger_level_change = 0,
#endif
	};
	struct siwx91x_dev *sidev = dev->data;
	sl_wifi_ssid_t ssid = { };
	int ret;

	__ASSERT(z_scan_config, "z_scan_config cannot be NULL");

	if (FIELD_GET(SIWX91X_INTERFACE_MASK, interface) != SL_WIFI_CLIENT_INTERFACE) {
		LOG_ERR("Interface not in STA mode");
		return -EINVAL;
	}

	if (sidev->state != WIFI_STATE_DISCONNECTED && sidev->state != WIFI_STATE_INACTIVE &&
	    sidev->state != WIFI_STATE_COMPLETED) {
		LOG_ERR("Command given in invalid state");
		return -EBUSY;
	}

	if (z_scan_config->bands & ~(BIT(WIFI_FREQ_BAND_UNKNOWN) | BIT(WIFI_FREQ_BAND_2_4_GHZ))) {
		LOG_ERR("Invalid band entered");
		return -EINVAL;
	}

	if (sidev->state == WIFI_STATE_COMPLETED) {
		siwx91x_configure_scan_dwell_time(SL_WIFI_SCAN_TYPE_ADV_SCAN,
						  z_scan_config->dwell_time_active,
						  z_scan_config->dwell_time_passive,
						  &advanced_scan_config);

		ret = sl_wifi_set_advanced_scan_configuration(&advanced_scan_config);
		if (ret) {
			LOG_ERR("Advanced scan configuration failed with status %x", ret);
			return -EINVAL;
		}

		ret = sl_wifi_set_roam_configuration(interface, &roam_configuration);
		if (ret) {
			LOG_ERR("Roaming configuration failed with status %x", ret);
			return -EINVAL;
		}

		sl_scan_config.type = SL_WIFI_SCAN_TYPE_ADV_SCAN;
		sl_scan_config.periodic_scan_interval =
			CONFIG_WIFI_SILABS_SIWX91X_ADV_SCAN_PERIODICITY;
	} else {
		if (z_scan_config->scan_type == WIFI_SCAN_TYPE_ACTIVE) {
			sl_scan_config.type = SL_WIFI_SCAN_TYPE_ACTIVE;
			ret = siwx91x_configure_scan_dwell_time(SL_WIFI_SCAN_TYPE_ACTIVE,
								z_scan_config->dwell_time_active,
								z_scan_config->dwell_time_passive,
								NULL);
		} else {
			sl_scan_config.type = SL_WIFI_SCAN_TYPE_PASSIVE;
			ret = siwx91x_configure_scan_dwell_time(SL_WIFI_SCAN_TYPE_PASSIVE,
								z_scan_config->dwell_time_active,
								z_scan_config->dwell_time_passive,
								NULL);
		}
		if (ret) {
			LOG_ERR("Failed to configure timeout");
			return -EINVAL;
		}
	}

	for (int i = 0; i < ARRAY_SIZE(z_scan_config->band_chan); i++) {
		/* End of channel list */
		if (z_scan_config->band_chan[i].channel == 0) {
			break;
		}

		if (z_scan_config->band_chan[i].band == WIFI_FREQ_BAND_2_4_GHZ) {
			sl_scan_config.channel_bitmap_2g4 |=
				BIT(z_scan_config->band_chan[i].channel - 1);
		}
	}

	if (z_scan_config->band_chan[0].channel && !sl_scan_config.channel_bitmap_2g4) {
		LOG_ERR("No supported channels in the request");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_WIFI_MGMT_SCAN_SSID_FILT_MAX)) {
		if (z_scan_config->ssids[0]) {
			strncpy(ssid.value, z_scan_config->ssids[0], WIFI_SSID_MAX_LEN);
			ssid.length = strlen(z_scan_config->ssids[0]);
		}
	}

	sidev->scan_max_bss_cnt = z_scan_config->max_bss_cnt;
	sidev->scan_res_cb = cb;
	ret = sl_wifi_start_scan(SL_WIFI_CLIENT_2_4GHZ_INTERFACE, (ssid.length > 0) ? &ssid : NULL,
				 &sl_scan_config);
	if (ret != SL_STATUS_IN_PROGRESS) {
		return -EIO;
	}
	sidev->scan_prev_state = sidev->state;
	sidev->state = WIFI_STATE_SCANNING;

	return 0;
}
