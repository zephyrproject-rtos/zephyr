/*
 * Copyright 2023 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT mock_vbus_adc

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usbc_vbus_adc, CONFIG_USBC_LOG_LEVEL);

#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/usb_c/usbc_pd.h>
#include <zephyr/drivers/usb_c/usbc_vbus.h>
#include <stddef.h>

#include "mock_tester.h"

/**
 * @brief Measure VBUS
 *
 * @retval 0 on success
 */
static int mt_adc_vbus_measure(const struct device *dev, int *meas)
{
	__ASSERT(meas != NULL, "ADC VBUS meas must not be NULL");

	*meas = tester_get_vbus();

	return 0;
}

/**
 * @brief Checks if VBUS is at a particular level
 *
 * @retval true if VBUS is at the level voltage, else false
 */
static bool mt_adc_vbus_check_level(const struct device *dev, enum tc_vbus_level level)
{
	int meas;

	meas = tester_get_vbus();

	switch (level) {
	case TC_VBUS_SAFE0V:
		return (meas < PD_V_SAFE_0V_MAX_MV);
	case TC_VBUS_PRESENT:
		return (meas >= PD_V_SAFE_5V_MIN_MV);
	case TC_VBUS_REMOVED:
		return (meas < TC_V_SINK_DISCONNECT_MAX_MV);
	}

	return false;
}

/**
 * @brief Initializes the ADC VBUS Driver
 *
 * @retval 0 on success
 */
static int mt_adc_vbus_init(const struct device *dev)
{
	/* Do nothing */
	return 0;
}

static const struct usbc_vbus_driver_api driver_api = {.measure = mt_adc_vbus_measure,
						       .check_level = mt_adc_vbus_check_level,
						       .discharge = NULL,
						       .enable = NULL};

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) > 0,
	     "No compatible USB-C VBUS Measurement instance found");

#define MOCK_DRIVER_INIT(inst)                                                                     \
	DEVICE_DT_INST_DEFINE(inst, &mt_adc_vbus_init, NULL, NULL, NULL, POST_KERNEL,              \
			      CONFIG_USBC_INIT_PRIORITY, &driver_api);

DT_INST_FOREACH_STATUS_OKAY(MOCK_DRIVER_INIT)
