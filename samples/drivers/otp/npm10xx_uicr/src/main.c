/*
 * Copyright (C) 2026 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/mfd/npm10xx.h>
#include <zephyr/drivers/charger.h>
#include <zephyr/drivers/otp.h>
#include <zephyr/drivers/otp/npm10xx.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/npm10xx.h>
#include <zephyr/logging/log.h>

#include "npm1012_otp.h"

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

static const uint8_t uicr_data[OTP_NPM10XX_UICR_SIZE] = NPM1012_OTP_ARRAY;

static const struct device *npm1012 = DEVICE_DT_GET(DT_NODELABEL(npm1012));
static const struct device *npm1012_adc = DEVICE_DT_GET(DT_NODELABEL(npm1012_adc));
static const struct device *npm1012_charger = DEVICE_DT_GET(DT_NODELABEL(npm1012_charger));
static const struct device *npm1012_uicr = DEVICE_DT_GET(DT_NODELABEL(npm1012_uicr));

#define VBUS_STATE_RETRY_MS 500
/* ref. datasheet chapter 3.5.15 for electrical specifications on UICR programming */
#define VBUS_WITHIN_RANGE(vbus_mv)    ((vbus_mv) > 4750 && (vbus_mv) < 5500)
#define DIE_TEMP_WITHIN_RANGE(die_mC) ((die_mC) > 10000 && (die_mC) < 40000)

static bool devices_are_ready(void)
{
	if (!device_is_ready(npm1012)) {
		LOG_ERR("MFD not ready");
		return false;
	}
	if (!device_is_ready(npm1012_adc)) {
		LOG_ERR("ADC not ready");
		return false;
	}
	if (!device_is_ready(npm1012_charger)) {
		LOG_ERR("Charger not ready");
		return false;
	}
	if (!device_is_ready(npm1012_uicr)) {
		LOG_ERR("UICR not ready");
		return false;
	}

	return true;
}

static int wait_for_vbus(enum charger_online state)
{
	int ret;
	union charger_propval val;

	do {
		k_msleep(VBUS_STATE_RETRY_MS);

		ret = charger_get_prop(npm1012_charger, CHARGER_PROP_ONLINE, &val);
		if (ret < 0) {
			LOG_ERR("failed to get VBUS status (%d)", ret);
			return ret;
		}

	} while (val.online != state);

	return 0;
}

static int check_params(void)
{
	int ret;
	struct sensor_value val;
	int64_t vbus_mv, die_mC;

	ret = sensor_sample_fetch_chan(npm1012_adc, (enum sensor_channel)NPM10XX_SENSOR_CHAN_VBUS);
	if (ret < 0) {
		LOG_ERR("failed to fetch VBUS sensor channel (%d)", ret);
		return ret;
	}

	ret = sensor_sample_fetch_chan(npm1012_adc, SENSOR_CHAN_DIE_TEMP);
	if (ret < 0) {
		LOG_ERR("failed to fetch die temperature sensor channel (%d)", ret);
		return ret;
	}

	(void)sensor_channel_get(npm1012_adc, (enum sensor_channel)NPM10XX_SENSOR_CHAN_VBUS, &val);
	vbus_mv = sensor_value_to_milli(&val);

	(void)sensor_channel_get(npm1012_adc, SENSOR_CHAN_DIE_TEMP, &val);
	die_mC = sensor_value_to_milli(&val);

	if (!VBUS_WITHIN_RANGE(vbus_mv)) {
		LOG_ERR("VBUS is outside of spec for UICR programming (%lld mV)", vbus_mv);
		return -EINVAL;
	}

	if (!DIE_TEMP_WITHIN_RANGE(die_mC)) {
		LOG_ERR("Die temp is outside of spec for UICR programming (%lld m°C)", die_mC);
		return -EINVAL;
	}

	return 0;
}

int main(void)
{
	int ret;

	if (!devices_are_ready()) {
		return -ENODEV;
	}

	if (IS_ENABLED(CONFIG_OTP_NPM10XX_DRY_RUN)) {
		LOG_WRN("This is a dry run - nothing will actually get programmed.");
	}

	LOG_WRN("VBUS connection is required for UICR programming to proceed.");

	ret = wait_for_vbus(CHARGER_ONLINE_FIXED);
	if (ret < 0) {
		return ret;
	}

	ret = check_params();
	if (ret < 0) {
		return ret;
	}

	LOG_INF("VBUS connected, parameters within range, starting UICR programming...");

	ret = otp_program(npm1012_uicr, 0U, uicr_data, sizeof(uicr_data));

	if (ret < 0) {
		LOG_ERR("UICR programming failed (%d)", ret);
		return ret;
	}

	LOG_INF("UICR programming success. Remove VBUS...");

	ret = wait_for_vbus(CHARGER_ONLINE_OFFLINE);
	if (ret < 0) {
		return ret;
	}

	LOG_INF("VBUS disconnected, resetting PMIC...");
	(void)mfd_npm10xx_reset(npm1012);

	return ret;
}
