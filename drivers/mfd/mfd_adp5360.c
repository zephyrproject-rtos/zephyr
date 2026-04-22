/*
 * Copyright (c) 2026 Analog Devices, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_adp5360

#include <errno.h>
#include <zephyr/drivers/mfd/adp5360.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>

#include "mfd_adp5360.h"

LOG_MODULE_REGISTER(mfd_adp5360, CONFIG_MFD_LOG_LEVEL);

#define ADP5360_MFD_REG_DEVICE_ID          0x00u
#define ADP5360_MFD_REG_FUEL_GAUGE_MODE    0x27u
#define ADP5360_MFD_REG_SOC_RESET          0x28u
#define ADP5360_MFD_REG_SUPERVISORY_CFG    0x2Du
#define ADP5360_MFD_REG_SHIPMENT           0x36u

#define ADP5360_MFD_VOUT1_RST_MASK             BIT(7)
#define ADP5360_MFD_VOUT2_RST_MASK             BIT(6)
#define ADP5360_MFD_RESET_TIME_MASK            BIT(5)
#define ADP5360_MFD_WATCHDOG_TIME_MASK         GENMASK(4, 3)
#define ADP5360_MFD_ENABLE_WATCHDOG_MASK       BIT(2)
#define ADP5360_MFD_ENABLE_SHIPMENT_ON_MR_MASK BIT(1)
#define ADP5360_MFD_RESET_WATCHDOG_TIMER_MASK  BIT(0)

#define ADP5360_DEVICE_ID 0x10

#define ADP5360_MFD_MANUAL_RESET_DEGLITCH	40
#define ADP5360_MFD_RESET_TIME_PERIOD_1600MS	1600
#define ADP5360_MFD_RESET_TIME_PERIOD_200MS	200

#define ADP5360_MFD_SOC_RESET_MASK BIT(7)
#define ADP5360_MFD_FG_MODE_MASK   BIT(1)

int mfd_adp5360_reg_read(const struct device *dev, uint8_t reg, uint8_t *val)
{
	const struct mfd_adp5360_config *config = dev->config;

	return i2c_reg_read_byte_dt(&config->i2c, reg, val);
}

int mfd_adp5360_reg_write(const struct device *dev, uint8_t reg, uint8_t val)
{
	const struct mfd_adp5360_config *config = dev->config;

	return i2c_reg_write_byte_dt(&config->i2c, reg, val);
}

int mfd_adp5360_reg_burst_read(const struct device *dev, uint8_t reg, uint8_t *val, size_t len)
{
	const struct mfd_adp5360_config *config = dev->config;

	return i2c_burst_read_dt(&config->i2c, reg, val, len);
}

int mfd_adp5360_reg_burst_write(const struct device *dev, uint8_t reg, uint8_t *val, size_t len)
{
	const struct mfd_adp5360_config *config = dev->config;

	return i2c_burst_write_dt(&config->i2c, reg, val, len);
}

int mfd_adp5360_reg_update(const struct device *dev, uint8_t reg, uint8_t mask, uint8_t val)
{
	uint8_t reg_val;
	int ret;

	ret = mfd_adp5360_reg_read(dev, reg, &reg_val);
	if (ret < 0) {
		return ret;
	}

	reg_val &= ~mask;
	reg_val |= FIELD_PREP(mask, val);

	return mfd_adp5360_reg_write(dev, reg, reg_val);
}

int mfd_adp5360_shipment_mode_enable(const struct device *dev)
{
	/*
	 * According to the datasheet (pg. 32), There are multiple methods to enable shipment mode.
	 * 1. Pull up ENSD pin to logic high.
	 * 2. write 1 to SHIPMODE register (0x36) bit 0.
	 * 3. enable MR shipment mode in supervisory register (0x2D) bit 1,
	 *    pull MR down for 12 seconds and then release.
	 *
	 * This implements method 2.
	 */
	return mfd_adp5360_reg_write(dev, ADP5360_MFD_REG_SHIPMENT, 1U);
}

int mfd_adp5360_shipment_mode_disable(const struct device *dev)
{
	/*
	 * According to the datasheet (pg. 32), There are multiple methods to disable shipment mode.
	 * 1. Pull down ENSD pin to logic low.
	 * 2. write 0 to SHIPMODE register (0x36) bit 0.
	 * 3. push MR pin for 200ms. This also restarts the device registers to its factory
	 * settings.
	 *
	 * This implements method 2.
	 */
	return mfd_adp5360_reg_write(dev, ADP5360_MFD_REG_SHIPMENT, 0U);
}

int mfd_adp5360_software_reset(const struct device *dev)
{
	int ret = 0;
	uint8_t reset_cfg = 0;

	reset_cfg = FIELD_PREP(ADP5360_MFD_SOC_RESET_MASK, 1);
	ret = mfd_adp5360_reg_write(dev, ADP5360_MFD_REG_SOC_RESET, reset_cfg);
	if (ret < 0) {
		LOG_ERR("Failed to write software reset register");
		return ret;
	}
	reset_cfg = FIELD_PREP(ADP5360_MFD_SOC_RESET_MASK, 0);
	ret = mfd_adp5360_reg_write(dev, ADP5360_MFD_REG_SOC_RESET, reset_cfg);
	if (ret < 0) {
		LOG_ERR("Failed to clear software reset register");
		return ret;
	}
	return 0;
}

int mfd_adp5360_hardware_reset(const struct device *dev)
{
	const struct mfd_adp5360_config *config = dev->config;
	uint8_t status;
	int ret;

	ret = gpio_pin_set_dt(&config->manual_reset_gpio, 0);
	if (ret < 0) {
		LOG_ERR("Failed to set reset GPIO high");
		return ret;
	}

	/* Deglitch Timer for MR pin */
	k_msleep(ADP5360_MFD_MANUAL_RESET_DEGLITCH);

	ret = gpio_pin_set_dt(&config->manual_reset_gpio, 1);
	if (ret < 0) {
		LOG_ERR("Failed to set reset GPIO low");
		return ret;
	}

	/* Reset Timeout Period */
	if (config->reset_time_1p6s) {
		k_sleep(K_MSEC(ADP5360_MFD_RESET_TIME_PERIOD_1600MS));
	} else {
		k_sleep(K_MSEC(ADP5360_MFD_RESET_TIME_PERIOD_200MS));
	}

	/* Read status register to check if manual reset interrupt was triggered */
	ret = mfd_adp5360_reg_read(dev, ADP5360_MFD_REG_PGOOD_STATUS, &status);
	if (ret < 0) {
		LOG_ERR("Failed to read pgood status register");
		return ret;
	}

	/* Check if reset was successful by verifying PGOOD status */
	if ((status & ADP5360_STATUS_MANUAL_RESET_INT_MASK) == 0) {
		LOG_ERR("Manual reset failed, PGOOD status indicates reset not successful");
		return -EIO;
	}
	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int mfd_adp5360_set_fg_mode(const struct device *dev, enum adp5360_fg_mode mode)
{
	uint8_t val;

	switch (mode) {
	case ADP5360_FG_MODE_SLEEP:
		val = 1u;
		break;
	case ADP5360_FG_MODE_ACTIVE:
		val = 0u;
		break;
	default:
		return -EINVAL;
	}

	return mfd_adp5360_reg_update(dev, ADP5360_MFD_REG_FUEL_GAUGE_MODE,
				      ADP5360_MFD_FG_MODE_MASK, val);
}

static int mfd_adp5360_pm_control(const struct device *dev, enum pm_device_action action)
{
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = mfd_adp5360_set_fg_mode(dev, ADP5360_FG_MODE_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Failed to set fuel gauge to active mode");
			return ret;
		}
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		ret = mfd_adp5360_set_fg_mode(dev, ADP5360_FG_MODE_SLEEP);
		if (ret < 0) {
			LOG_ERR("Failed to set fuel gauge to sleep mode");
			return ret;
		}
		break;
	default:
		LOG_ERR("Unsupported PM action %d for ADP5360", action);
		return -EINVAL;
	}
	return 0;
}
#define MFD_ADP5360_PM_DEVICE_DEFINE(inst) PM_DEVICE_DT_INST_DEFINE(inst, mfd_adp5360_pm_control);
#else
#define MFD_ADP5360_PM_DEVICE_DEFINE(inst)
#endif /* CONFIG_PM_DEVICE */

static int mfd_adp5360_init(const struct device *dev)
{
	const struct mfd_adp5360_config *config = dev->config;
	uint8_t val = 0U;
	int ret;

	if (!i2c_is_ready_dt(&config->i2c)) {
		return -ENODEV;
	}

	ret = mfd_adp5360_reg_read(dev, ADP5360_MFD_REG_DEVICE_ID, &val);
	if (ret < 0) {
		return ret;
	}

	if (val != ADP5360_DEVICE_ID) {
		return -EINVAL;
	}

	val = 0u;

	val = FIELD_PREP(ADP5360_MFD_VOUT1_RST_MASK, config->en_vout1_rst) |
	      FIELD_PREP(ADP5360_MFD_VOUT2_RST_MASK, config->en_vout2_rst) |
	      FIELD_PREP(ADP5360_MFD_RESET_TIME_MASK, config->reset_time_1p6s) |
	      FIELD_PREP(ADP5360_MFD_WATCHDOG_TIME_MASK, config->watchdog_time) |
	      FIELD_PREP(ADP5360_MFD_ENABLE_WATCHDOG_MASK, config->en_watchdog) |
	      FIELD_PREP(ADP5360_MFD_ENABLE_SHIPMENT_ON_MR_MASK, config->en_mr_shipment);

	ret = mfd_adp5360_reg_write(dev, ADP5360_MFD_REG_SUPERVISORY_CFG, val);
	if (ret < 0) {
		return ret;
	}

	/* delay init by RESET time to avoid false positive RESET sense */
	if (config->reset_time_1p6s) {
		k_sleep(K_MSEC(ADP5360_MFD_RESET_TIME_PERIOD_1600MS));
	} else {
		k_sleep(K_MSEC(ADP5360_MFD_RESET_TIME_PERIOD_200MS));
	}

	ret = mfd_adp5360_reg_read(dev, ADP5360_MFD_REG_INT_STATUS1, &val);
	if (ret < 0) {
		LOG_ERR("Failed to clear interrupt status 1");
		return ret;
	}

	ret = mfd_adp5360_reg_read(dev, ADP5360_MFD_REG_INT_STATUS2, &val);
	if (ret < 0) {
		LOG_ERR("Failed to clear interrupt status 2");
		return ret;
	}

#ifdef CONFIG_MFD_ADP5360_TRIGGER
	if (config->interrupt_gpio.port) {
		ret = mfd_adp5360_init_interrupt(dev);
		if (ret < 0) {
			LOG_ERR("Failed to initialize interrupt");
			return ret;
		}
	}

	if (config->pgood1_gpio.port || config->pgood2_gpio.port) {
		ret = mfd_adp5360_init_pgood_interrupt(dev);
		if (ret < 0) {
			LOG_ERR("Failed to initialize pgood interrupt");
			return ret;
		}
	}

	if (config->manual_reset_gpio.port) {
		ret = gpio_pin_configure_dt(&config->manual_reset_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Failed to configure reset GPIO pin");
			return ret;
		}
	}

	if (config->reset_status_gpio.port) {
		ret = mfd_adp5360_init_reset_status_interrupt(dev);
		if (ret < 0) {
			LOG_ERR("Failed to Initialize reset status interrupt");
			return ret;
		}
	}
#endif /* CONFIG_MFD_ADP5360_TRIGGER */

	return 0;
}

#define MFD_ADP5360_DEFINE(inst)                                                                   \
                                                                                                   \
	static const struct mfd_adp5360_config mfd_adp5360_config_##inst = {                       \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.interrupt_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, interrupt_gpios, {0}),            \
		.manual_reset_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, manual_reset_gpios, {0}),      \
		.reset_status_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, reset_status_gpios, {0}),      \
		.charger_en_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, charger_en_gpios, {0}),          \
		.pgood1_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, pgood1_gpios, {0}),                  \
		.pgood2_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, pgood2_gpios, {0}),                  \
		.watchdog_time = DT_INST_ENUM_IDX(inst, watchdog_timeout_period),                  \
		.en_vout1_rst = !DT_INST_PROP(inst, disable_vout_buck_reset),                      \
		.en_vout2_rst = DT_INST_PROP(inst, enable_vout_buckboost_reset),                   \
		.reset_time_1p6s = DT_INST_PROP(inst, set_reset_timeout_1p6sec),                   \
		.en_watchdog = DT_INST_PROP(inst, enable_watchdog_timer),                          \
		.en_mr_shipment = DT_INST_PROP(inst, enable_manual_reset_to_shipment),             \
	};                                                                                         \
	static struct mfd_adp5360_data mfd_adp5360_data_##inst;                                    \
                                                                                                   \
	MFD_ADP5360_PM_DEVICE_DEFINE(inst)                                                         \
	DEVICE_DT_INST_DEFINE(inst, &mfd_adp5360_init, PM_DEVICE_DT_INST_GET(inst),                \
			      &mfd_adp5360_data_##inst, &mfd_adp5360_config_##inst, POST_KERNEL,   \
			      CONFIG_MFD_ADP5360_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(MFD_ADP5360_DEFINE)
