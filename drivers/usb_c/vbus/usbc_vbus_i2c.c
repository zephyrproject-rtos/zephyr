/*
 * Copyright (c) 2023, ithinx GmbH
 * Copyright (c) 2023, Tonies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/sys/byteorder.h"
#define DT_DRV_COMPAT zephyr_usb_c_vbus_i2c

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usbc_vbus_i2c, CONFIG_USBC_LOG_LEVEL);

#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/usb_c/usbc_pd.h>
#include <zephyr/drivers/usb_c/usbc_vbus.h>
#include <soc.h>
#include <stddef.h>

#include "usbc_vbus_i2c_priv.h"

static int tps25750_i2c_read(const struct device *dev, uint8_t reg, uint8_t *data, size_t len)
{
	const struct usbc_vbus_config *const cfg = dev->config;
	uint8_t read_data[5];

	const int ret = i2c_burst_read_dt(&cfg->i2c, reg, read_data, len + 1);

	if (ret != 0) {
		return ret;
	}

	memcpy(data, &read_data[1], len);

	return 0;
}

/**
 * @brief Reads and returns VBUS measured in mV
 *
 * @retval 0 on success
 * @retval -EIO on failure
 */
static int adc_vbus_measure(const struct device *dev, int *meas)
{
	return -EIO;
}

/**
 * @brief Checks if VBUS is at a particular level
 *
 * @retval true if VBUS is at the level voltage, else false
 */
static bool adc_vbus_check_level(const struct device *dev, enum tc_vbus_level level)
{
	uint32_t meas;
	int ret;
	uint8_t buf[4];

	ret = tps25750_i2c_read(dev, 0x1A, buf, 4);
	if (ret) {
		return false;
	}

	meas = sys_get_le32(buf);
	const int vbus_status = (meas >> 20) & 0x3;

	if (vbus_status == 3) {
		return false;
	}

	switch (level) {
	case TC_VBUS_SAFE0V:
		return vbus_status == 0;
	case TC_VBUS_PRESENT:
		return vbus_status == 1;
	case TC_VBUS_REMOVED:
		return vbus_status == 2;
	}

	return false;
}

/**
 * @brief Sets pin to discharge VBUS
 *
 * @retval 0 on success
 * @retval -EIO on failure
 * @retval -ENOENT if enable pin isn't defined
 */
static int adc_vbus_discharge(const struct device *dev, bool enable)
{
	return -ENOENT;
}

/**
 * @brief Sets pin to enable VBUS measurments
 *
 * @retval 0 on success
 * @retval -EIO on failure
 * @retval -ENOENT if enable pin isn't defined
 */
static int adc_vbus_enable(const struct device *dev, bool enable)
{
	return -ENOENT;
}

/**
 * @brief Initializes the ADC VBUS Driver
 *
 * @retval 0 on success
 * @retval -EIO on failure
 */
static int adc_vbus_init(const struct device *dev)
{
	return 0;
}

static const struct usbc_vbus_driver_api driver_api = {.measure = adc_vbus_measure,
						       .check_level = adc_vbus_check_level,
						       .discharge = adc_vbus_discharge,
						       .enable = adc_vbus_enable};

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) > 0,
	     "No compatible USB-C VBUS Measurement instance found");

#define DRIVER_INIT(inst)                                                                          \
	static const struct usbc_vbus_config drv_config_##inst = {                                 \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, &adc_vbus_init, NULL, NULL, &drv_config_##inst, POST_KERNEL,   \
			      CONFIG_USBC_INIT_PRIORITY, &driver_api);

DT_INST_FOREACH_STATUS_OKAY(DRIVER_INIT)
