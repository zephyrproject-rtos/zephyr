/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/basic_radio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(basic_radio_cw, LOG_LEVEL_INF);

#define BASIC_RADIO_NODE 	DT_NODELABEL(basic_radio0)

static const struct device *const rf_dev = DEVICE_DT_GET(BASIC_RADIO_NODE);

/* 10 Hz units */
#define CW_FREQUENCY     	245000000U
#define CW_TX_POWER_DBM  	5
#define CW_DURATION_MS   	5000U

int main(void)
{
	int ret;

	if (!device_is_ready(rf_dev)) {
		LOG_ERR("basic_radio device not ready");
		return -ENODEV;
	}

	struct basic_radio_caps caps;

	ret = basic_radio_get_caps(rf_dev, &caps);
	if (ret == 0) {
		LOG_INF("caps=0x%08x tx_power=[%d, %d] dBm", caps.flags, caps.tx_power_min_dbm,
			caps.tx_power_max_dbm);
	}

	LOG_INF("Emitting CW at %u0 Hz, %d dBm, for %u ms", CW_FREQUENCY, CW_TX_POWER_DBM, CW_DURATION_MS);

	ret = basic_radio_test_cw(rf_dev, CW_FREQUENCY, CW_TX_POWER_DBM, CW_DURATION_MS);
	if (ret != 0) {
		LOG_ERR("test_cw failed: %d", ret);
		return ret;
	}

	LOG_INF("CW done");
	return 0;
}