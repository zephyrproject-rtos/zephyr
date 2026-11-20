/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * "stepper-driver" child device - implements stepper.h.
 * Shared by TMC5241 and TMC2241 parents (both have identical CHOPCONF /
 * COOLCONF / DRVSTATUS layout for these operations). Compatible:
 * "adi,tmc5241-stepper-driver".
 */

#include <zephyr/drivers/stepper/stepper.h>
#include <zephyr/drivers/gpio.h>
#include "tmc5241.h"
#include "tmc5241_reg.h"
#include <adi_tmc5xxx_common.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(tmc5241, CONFIG_STEPPER_LOG_LEVEL);

#define DT_DRV_COMPAT adi_tmc5241_stepper_driver

struct tmc5241_stepper_driver_config {
	const uint16_t default_micro_step_res;
	/* SRS req: CHOPCONF.TOFF must be a DT property (default 5), not hardcoded -
	 * see tmc5241_stepper_driver_enable() for why this differs from tmc51xx.
	 */
	const uint8_t default_toff;
	const int8_t sg_threshold;
	const struct gpio_dt_spec en_gpio;
	/* parent controller required for bus communication */
	const struct device *controller;
};

struct tmc5241_stepper_driver_data {
	stepper_event_cb_t callback;
	void *user_data;
	uint8_t toff; /* configured TOFF value, restored on enable() */
};

void tmc5241_stepper_driver_notify_event(const struct device *dev, enum stepper_event event)
{
	struct tmc5241_stepper_driver_data *data = dev->data;

	if (!data->callback) {
		LOG_WRN_ONCE("No stepper driver callback registered");
		return;
	}
	data->callback(dev, event, data->user_data);
}

static int tmc5241_stepper_driver_set_event_cb(const struct device *stepper,
					       stepper_event_cb_t callback, void *user_data)
{
	struct tmc5241_stepper_driver_data *data = stepper->data;

	data->callback = callback;
	data->user_data = user_data;

	return 0;
}

static int tmc5241_stepper_driver_enable(const struct device *dev)
{
	const struct tmc5241_stepper_driver_config *config = dev->config;
	const struct device *controller = config->controller;
	struct tmc5241_stepper_driver_data *data = dev->data;
	uint32_t reg_value;
	int err;

	LOG_DBG("Enabling Stepper Driver %s", dev->name);

	if (config->en_gpio.port != NULL) {
		/* Assert enable line first so the driver can energize even if SPI config
		 * later fails (e.g., bus wiring issues). The register write still runs so
		 * we get useful errors in logs.
		 */
		err = gpio_pin_set_dt(&config->en_gpio, 1);
		if (err < 0) {
			LOG_ERR("Failed to assert enable GPIO (%d)", err);
			return err;
		}
	}

	err = tmc5241_read(controller, TMC5241_CHOPCONF, &reg_value);
	if (err != 0) {
		return -EIO;
	}

	/* Restore the configured TOFF value. Unlike tmc51xx (which ORs in the
	 * full TMC5XXX_CHOPCONF_DRV_ENABLE_MASK unconditionally, hardcoding
	 * TOFF=0xF), this preserves the DT-configured TOFF instead of corrupting it.
	 */
	reg_value &= ~TMC5XXX_CHOPCONF_DRV_ENABLE_MASK;
	reg_value |= FIELD_PREP(TMC5XXX_CHOPCONF_DRV_ENABLE_MASK, data->toff);

	err = tmc5241_write(controller, TMC5241_CHOPCONF, reg_value);
	if (err != 0) {
		return -EIO;
	}

	/* SRS-DRV-001 / STS Software Item 1 verification: confirm TOFF reads back non-zero. */
	uint32_t chopconf_readback;

	err = tmc5241_read(controller, TMC5241_CHOPCONF, &chopconf_readback);
	if (err == 0) {
		LOG_INF("CHOPCONF readback after enable: 0x%08x, TOFF=%d", chopconf_readback,
			(int)FIELD_GET(TMC5XXX_CHOPCONF_DRV_ENABLE_MASK, chopconf_readback));
	}

	return 0;
}

static int tmc5241_stepper_driver_disable(const struct device *dev)
{
	LOG_DBG("Disabling Stepper Driver %s", dev->name);
	const struct tmc5241_stepper_driver_config *config = dev->config;
	const struct device *controller = config->controller;
	uint32_t reg_value;
	int err;

	if (config->en_gpio.port != NULL) {
		err = gpio_pin_set_dt(&config->en_gpio, 0);
		if (err < 0) {
			LOG_ERR("Failed to deassert enable GPIO (%d)", err);
			return err;
		}
	}

	err = tmc5241_read(controller, TMC5241_CHOPCONF, &reg_value);
	if (err != 0) {
		return -EIO;
	}

	reg_value &= ~TMC5XXX_CHOPCONF_DRV_ENABLE_MASK; /* TOFF = 0 disables the power stage */

	err = tmc5241_write(controller, TMC5241_CHOPCONF, reg_value);
	if (err != 0) {
		return -EIO;
	}

	/* SRS-DRV-002 / STS Software Item 1 verification: confirm TOFF reads back zero. */
	uint32_t chopconf_readback;

	err = tmc5241_read(controller, TMC5241_CHOPCONF, &chopconf_readback);
	if (err == 0) {
		LOG_INF("CHOPCONF readback after disable: 0x%08x, TOFF=%d", chopconf_readback,
			(int)FIELD_GET(TMC5XXX_CHOPCONF_DRV_ENABLE_MASK, chopconf_readback));
	}

	return 0;
}

/* MRES = 8 - log2(res); res must be a power of two in [1, 256].
 * Datasheet-confirmed: CHOPCONF.MRES 0=256 ... 8=fullstep (1), step width = 2^MRES.
 */
static int tmc5241_micro_step_res_to_mres(uint16_t res, uint32_t *mres)
{
	if (res == 0 || res > 256 || !is_power_of_two(res)) {
		return -EINVAL;
	}

	*mres = 8 - LOG2(res);
	return 0;
}

static int tmc5241_stepper_driver_set_micro_step_res(const struct device *dev,
						     enum stepper_micro_step_resolution res)
{
	const struct tmc5241_stepper_driver_config *config = dev->config;
	const struct device *controller = config->controller;
	uint32_t reg_value, mres;
	int err;

	err = tmc5241_micro_step_res_to_mres((uint16_t)res, &mres);
	if (err != 0) {
		return err;
	}

	err = tmc5241_read(controller, TMC5241_CHOPCONF, &reg_value);
	if (err != 0) {
		return -EIO;
	}

	reg_value &= ~TMC5XXX_CHOPCONF_MRES_MASK;
	reg_value |= FIELD_PREP(TMC5XXX_CHOPCONF_MRES_MASK, mres);

	err = tmc5241_write(controller, TMC5241_CHOPCONF, reg_value);
	if (err != 0) {
		return -EIO;
	}

	LOG_DBG("Stepper motor controller %s set micro step resolution to 0x%x", dev->name,
		reg_value);
	return 0;
}

static int tmc5241_stepper_driver_get_micro_step_res(const struct device *dev,
						     enum stepper_micro_step_resolution *res)
{
	const struct tmc5241_stepper_driver_config *config = dev->config;
	const struct device *controller = config->controller;
	uint32_t reg_value;
	int err;

	err = tmc5241_read(controller, TMC5241_CHOPCONF, &reg_value);
	if (err != 0) {
		return -EIO;
	}
	*res = BIT(8 - FIELD_GET(TMC5XXX_CHOPCONF_MRES_MASK, reg_value));
	LOG_DBG("Stepper motor controller %s get micro step resolution: %d", dev->name, *res);
	return 0;
}

static int tmc5241_stepper_driver_init(const struct device *dev)
{
	const struct tmc5241_stepper_driver_config *config = dev->config;
	const struct device *controller = config->controller;
	struct tmc5241_stepper_driver_data *data = dev->data;
	int err;

	data->toff = config->default_toff;

	if (config->en_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&config->en_gpio)) {
			LOG_ERR("Enable GPIO not ready");
			return -ENODEV;
		}

		err = gpio_pin_configure_dt(&config->en_gpio, GPIO_OUTPUT_INACTIVE);
		if (err < 0) {
			LOG_ERR("Failed to configure enable GPIO (%d)", err);
			return err;
		}
	}

	if (!IN_RANGE(config->sg_threshold, TMC5XXX_SG_MIN_VALUE, TMC5XXX_SG_MAX_VALUE)) {
		LOG_ERR("Stallguard threshold out of range");
		return -EINVAL;
	}

	int32_t stall_guard_threshold = (int32_t)config->sg_threshold;

	err = tmc5241_write(controller, TMC5241_COOLCONF,
			    stall_guard_threshold << TMC5XXX_COOLCONF_SG2_THRESHOLD_VALUE_SHIFT);
	if (err != 0) {
		return -EIO;
	}

	/* SRS-DRV-007 / STS Software Item 18 verification: confirm COOLCONF.SGT
	 * reads back the DT-configured threshold immediately after init.
	 */
	uint32_t coolconf_readback;

	err = tmc5241_read(controller, TMC5241_COOLCONF, &coolconf_readback);
	if (err != 0) {
		return -EIO;
	}
	/* SGT is a 7-bit signed field (bits 22:16) - sign-extend from bit 6, not bit 7,
	 * or negative values (e.g. -10) get misread as large positives (e.g. 118).
	 */
	int8_t sgt_readback = (int8_t)(FIELD_GET(TMC5241_COOLCONF_SGT_MASK, coolconf_readback)
					<< 1) >>
			      1;

	LOG_INF("COOLCONF readback: 0x%08x, SGT=%d (expected %d)", coolconf_readback,
		sgt_readback, config->sg_threshold);

	err = tmc5241_stepper_driver_set_micro_step_res(dev, config->default_micro_step_res);
	if (err != 0) {
		return -EIO;
	}
	LOG_DBG("Setting stallguard %d", config->sg_threshold);
	return 0;
}

static DEVICE_API(stepper, tmc5241_stepper_driver_api) = {
	.enable = tmc5241_stepper_driver_enable,
	.disable = tmc5241_stepper_driver_disable,
	.set_micro_step_res = tmc5241_stepper_driver_set_micro_step_res,
	.get_micro_step_res = tmc5241_stepper_driver_get_micro_step_res,
	.set_event_cb = tmc5241_stepper_driver_set_event_cb,
};

#define TMC5241_STEPPER_DRIVER_DEFINE(inst)                                                        \
	static const struct tmc5241_stepper_driver_config tmc5241_stepper_driver_config_##inst = { \
		.controller = DEVICE_DT_GET(DT_PARENT(DT_DRV_INST(inst))),                         \
		.default_micro_step_res = DT_INST_PROP(inst, micro_step_res),                      \
		.default_toff = DT_INST_PROP_OR(inst, toff, 5),                                    \
		.sg_threshold = DT_INST_PROP(inst, stallguard2_threshold),                         \
		.en_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, en_gpios, {0}),                          \
	};                                                                                         \
	static struct tmc5241_stepper_driver_data tmc5241_stepper_driver_data_##inst;              \
	DEVICE_DT_INST_DEFINE(inst, tmc5241_stepper_driver_init, NULL,                             \
			      &tmc5241_stepper_driver_data_##inst,                                 \
			      &tmc5241_stepper_driver_config_##inst, POST_KERNEL,                  \
			      CONFIG_STEPPER_INIT_PRIORITY, &tmc5241_stepper_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TMC5241_STEPPER_DRIVER_DEFINE)
