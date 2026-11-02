/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Swarovski Optik AG & Co. KG
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_tmc2300

#include <inttypes.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/stepper/stepper.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <adi_tmc_uart.h>
#include <step_dir_stepper_common.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tmc2300, CONFIG_STEPPER_LOG_LEVEL);

/* Register addresses */
#define TMC2300_GCONF      0x00U
#define TMC2300_GSTAT      0x01U
#define TMC2300_IFCNT      0x02U
#define TMC2300_IOIN       0x06U
#define TMC2300_IHOLD_IRUN 0x10U
#define TMC2300_TPOWERDOWN 0x11U
#define TMC2300_CHOPCONF   0x6CU

#define TMC2300_GCONF_EXTCAP         BIT(1)
#define TMC2300_GCONF_MULTISTEP_FILT BIT(6)

#define TMC2300_GSTAT_RESET   BIT(0)
#define TMC2300_GSTAT_DRV_ERR BIT(1)

#define TMC2300_IFCNT_MASK GENMASK(7, 0)

#define TMC2300_IOIN_STEPPER      BIT(5)
#define TMC2300_IOIN_MODE         BIT(7)
#define TMC2300_IOIN_VERSION_MASK GENMASK(31, 24)
#define TMC2300_IOIN_VERSION      0x40U

#define TMC2300_IHOLD_IRUN_IHOLD_MASK      GENMASK(4, 0)
#define TMC2300_IHOLD_IRUN_IRUN_MASK       GENMASK(12, 8)
#define TMC2300_IHOLD_IRUN_IHOLDDELAY_MASK GENMASK(19, 16)

#define TMC2300_CHOPCONF_ENABLEDRV BIT(0)
#define TMC2300_CHOPCONF_MRES_MASK GENMASK(27, 24)
#define TMC2300_CHOPCONF_DEDGE     BIT(29)

/* CHOPCONF reset default: intpol=1, MRES=32 microsteps, TBL=%01, enabledrv=1 */
#define TMC2300_CHOPCONF_RESET 0x13008001U

struct tmc2300_config {
	struct step_dir_stepper_common_config common;
	const struct device *uart;
	uint8_t uart_addr;
	uint8_t ihold;
	uint8_t irun;
	uint8_t iholddelay;
	uint8_t tpowerdown;
	bool extcap;
};

struct tmc2300_data {
	struct k_sem sem;
	enum stepper_micro_step_resolution resolution;
	bool enabled;
};

STEP_DIR_STEPPER_STRUCT_CHECK(struct tmc2300_config);

struct tmc2300_reg_value {
	uint8_t reg;
	uint32_t value;
};

/*
 * Several instances may share one EN line. It is only pulled low when no other instance using
 * it is enabled. Protects the enabled flag of all instances.
 */
static K_SEM_DEFINE(tmc2300_en_sem, 1, 1);

#define TMC2300_DEVICE_GET(inst) DEVICE_DT_INST_GET(inst),
static const struct device *const tmc2300_devices[] = {
	DT_INST_FOREACH_STATUS_OKAY(TMC2300_DEVICE_GET)};

static int tmc2300_read(const struct device *dev, uint8_t reg, uint32_t *value)
{
	const struct tmc2300_config *config = dev->config;
	int ret;

	ret = tmc_uart_read_register(config->uart, config->uart_addr, reg, value);
	if (ret < 0) {
		LOG_ERR("%s: failed to read register 0x%02x: %d", dev->name, reg, ret);
	}

	return ret;
}

static int tmc2300_write(const struct device *dev, uint8_t reg, uint32_t value)
{
	const struct tmc2300_config *config = dev->config;
	int ret;

	ret = tmc_uart_write_register(config->uart, config->uart_addr, reg, value);
	if (ret < 0) {
		LOG_ERR("%s: failed to write register 0x%02x: %d", dev->name, reg, ret);
	}

	return ret;
}

/* Write registers and verify via IFCNT that all writes were accepted */
static int tmc2300_write_verified(const struct device *dev, const struct tmc2300_reg_value *regs,
				  size_t count)
{
	uint32_t ifcnt_before;
	uint32_t ifcnt_after;
	int ret;

	ret = tmc2300_read(dev, TMC2300_IFCNT, &ifcnt_before);
	if (ret < 0) {
		return ret;
	}

	for (size_t i = 0; i < count; i++) {
		ret = tmc2300_write(dev, regs[i].reg, regs[i].value);
		if (ret < 0) {
			return ret;
		}
	}

	/* Writes are not acknowledged, IFCNT counts each accepted write datagram */
	ret = tmc2300_read(dev, TMC2300_IFCNT, &ifcnt_after);
	if (ret < 0) {
		return ret;
	}

	if (((ifcnt_after - ifcnt_before) & TMC2300_IFCNT_MASK) != count) {
		LOG_ERR("%s: write not accepted (IFCNT %" PRIu32 " -> %" PRIu32 ")", dev->name,
			ifcnt_before, ifcnt_after);
		return -EIO;
	}

	return 0;
}

static uint32_t tmc2300_mres(enum stepper_micro_step_resolution res)
{
	return (uint32_t)(MICRO_STEP_RES_INDEX(STEPPER_MICRO_STEP_256) - MICRO_STEP_RES_INDEX(res));
}

static int tmc2300_check_mode(const struct device *dev)
{
	uint32_t ioin;
	uint32_t version;
	int ret;

	ret = tmc2300_read(dev, TMC2300_IOIN, &ioin);
	if (ret < 0) {
		return ret;
	}

	version = FIELD_GET(TMC2300_IOIN_VERSION_MASK, ioin);
	if (version != TMC2300_IOIN_VERSION) {
		LOG_ERR("%s: unsupported version 0x%02" PRIx32, dev->name, version);
		return -ENODEV;
	}

	if (((ioin & TMC2300_IOIN_STEPPER) == 0U) || ((ioin & TMC2300_IOIN_MODE) != 0U)) {
		LOG_ERR("%s: not in UART controlled stepper mode (IOIN 0x%08" PRIx32 ")", dev->name,
			ioin);
		return -ENOTSUP;
	}

	return 0;
}

/* Write the complete configuration. Caller holds data->sem. */
static int tmc2300_configure(const struct device *dev, bool enable_driver)
{
	const struct tmc2300_config *config = dev->config;
	struct tmc2300_data *data = dev->data;
	uint32_t gconf = 0U;
	uint32_t chopconf;
	int ret;

	ret = tmc2300_check_mode(dev);
	if (ret < 0) {
		return ret;
	}

	/* STEP filtering is not compatible with double edge step pulses */
	if (!config->common.dual_edge) {
		gconf |= TMC2300_GCONF_MULTISTEP_FILT;
	}
	if (config->extcap) {
		gconf |= TMC2300_GCONF_EXTCAP;
	}

	chopconf = TMC2300_CHOPCONF_RESET;
	chopconf &= ~(TMC2300_CHOPCONF_MRES_MASK | TMC2300_CHOPCONF_ENABLEDRV);
	chopconf |= FIELD_PREP(TMC2300_CHOPCONF_MRES_MASK, tmc2300_mres(data->resolution));
	if (config->common.dual_edge) {
		chopconf |= TMC2300_CHOPCONF_DEDGE;
	}
	if (enable_driver) {
		chopconf |= TMC2300_CHOPCONF_ENABLEDRV;
	}

	const struct tmc2300_reg_value regs[] = {
		{TMC2300_GSTAT, TMC2300_GSTAT_RESET | TMC2300_GSTAT_DRV_ERR},
		{TMC2300_GCONF, gconf},
		{TMC2300_IHOLD_IRUN,
		 FIELD_PREP(TMC2300_IHOLD_IRUN_IHOLD_MASK, config->ihold) |
			 FIELD_PREP(TMC2300_IHOLD_IRUN_IRUN_MASK, config->irun) |
			 FIELD_PREP(TMC2300_IHOLD_IRUN_IHOLDDELAY_MASK, config->iholddelay)},
		{TMC2300_TPOWERDOWN, config->tpowerdown},
		{TMC2300_CHOPCONF, chopconf},
	};

	return tmc2300_write_verified(dev, regs, ARRAY_SIZE(regs));
}

/*
 * All registers are reset when VIO/NSTDBY is pulled low. Restore the configuration if
 * GSTAT.reset indicates such a reset. Caller holds data->sem.
 */
static int tmc2300_restore_after_reset(const struct device *dev, bool enable_driver)
{
	uint32_t gstat;
	int ret;

	ret = tmc2300_read(dev, TMC2300_GSTAT, &gstat);
	if (ret < 0) {
		return ret;
	}

	if ((gstat & TMC2300_GSTAT_RESET) == 0U) {
		return 0;
	}

	LOG_INF("%s: reset detected, restoring configuration", dev->name);

	return tmc2300_configure(dev, enable_driver);
}

/* Read-modify-write of CHOPCONF. Caller holds data->sem. */
static int tmc2300_update_chopconf(const struct device *dev, uint32_t mask, uint32_t value)
{
	struct tmc2300_reg_value chopconf = {.reg = TMC2300_CHOPCONF};
	int ret;

	ret = tmc2300_read(dev, TMC2300_CHOPCONF, &chopconf.value);
	if (ret < 0) {
		return ret;
	}

	chopconf.value = (chopconf.value & ~mask) | value;

	return tmc2300_write_verified(dev, &chopconf, 1);
}

/* Caller holds tmc2300_en_sem. */
static bool tmc2300_en_pin_in_use(const struct device *dev)
{
	const struct tmc2300_config *config = dev->config;

	ARRAY_FOR_EACH(tmc2300_devices, i) {
		const struct device *other = tmc2300_devices[i];
		const struct tmc2300_config *other_config = other->config;
		const struct tmc2300_data *other_data = other->data;

		if ((other != dev) && other_data->enabled &&
		    (other_config->common.en_pin.port == config->common.en_pin.port) &&
		    (other_config->common.en_pin.pin == config->common.en_pin.pin)) {
			return true;
		}
	}

	return false;
}

static int tmc2300_enable(const struct device *dev)
{
	const struct tmc2300_config *config = dev->config;
	struct tmc2300_data *data = dev->data;
	int ret;

	LOG_DBG("Enabling stepper driver %s", dev->name);

	k_sem_take(&data->sem, K_FOREVER);

	if (data->enabled) {
		/* After a reset while enabled, the driver runs with the reset defaults */
		ret = tmc2300_restore_after_reset(dev, true);
		goto out;
	}

	ret = tmc2300_restore_after_reset(dev, false);
	if (ret < 0) {
		goto out;
	}

	ret = tmc2300_update_chopconf(dev, TMC2300_CHOPCONF_ENABLEDRV, TMC2300_CHOPCONF_ENABLEDRV);
	if (ret < 0) {
		goto out;
	}

	k_sem_take(&tmc2300_en_sem, K_FOREVER);

	if (config->common.en_pin.port != NULL) {
		ret = gpio_pin_set_dt(&config->common.en_pin, 1);
		if (ret < 0) {
			LOG_ERR("%s: failed to set enable pin: %d", dev->name, ret);
		}
	}

	data->enabled = (ret == 0);

	k_sem_give(&tmc2300_en_sem);

out:
	k_sem_give(&data->sem);

	return ret;
}

static int tmc2300_disable(const struct device *dev)
{
	const struct tmc2300_config *config = dev->config;
	struct tmc2300_data *data = dev->data;
	int ret;
	int err;

	LOG_DBG("Disabling stepper driver %s", dev->name);

	k_sem_take(&data->sem, K_FOREVER);

	if (!data->enabled) {
		k_sem_give(&data->sem);
		return 0;
	}

	/* Release the EN line even if the driver is not reachable via UART */
	ret = tmc2300_update_chopconf(dev, TMC2300_CHOPCONF_ENABLEDRV, 0U);

	k_sem_take(&tmc2300_en_sem, K_FOREVER);

	data->enabled = false;

	if ((config->common.en_pin.port != NULL) && !tmc2300_en_pin_in_use(dev)) {
		err = gpio_pin_set_dt(&config->common.en_pin, 0);
		if (err < 0) {
			LOG_ERR("%s: failed to clear enable pin: %d", dev->name, err);
			ret = (ret < 0) ? ret : err;
		}
	}

	k_sem_give(&tmc2300_en_sem);

	k_sem_give(&data->sem);

	return ret;
}

static int tmc2300_set_micro_step_res(const struct device *dev,
				      enum stepper_micro_step_resolution micro_step_res)
{
	struct tmc2300_data *data = dev->data;
	int ret;

	k_sem_take(&data->sem, K_FOREVER);

	ret = tmc2300_update_chopconf(
		dev, TMC2300_CHOPCONF_MRES_MASK,
		FIELD_PREP(TMC2300_CHOPCONF_MRES_MASK, tmc2300_mres(micro_step_res)));
	if (ret == 0) {
		data->resolution = micro_step_res;
	}

	k_sem_give(&data->sem);

	return ret;
}

static int tmc2300_get_micro_step_res(const struct device *dev,
				      enum stepper_micro_step_resolution *micro_step_res)
{
	struct tmc2300_data *data = dev->data;

	*micro_step_res = data->resolution;

	return 0;
}

static int tmc2300_init(const struct device *dev)
{
	const struct tmc2300_config *config = dev->config;
	struct tmc2300_data *data = dev->data;
	int ret;

	k_sem_init(&data->sem, 1, 1);

	if (!device_is_ready(config->uart)) {
		LOG_ERR_DEVICE_NOT_READY(config->uart);
		return -ENODEV;
	}

	if (config->common.en_pin.port != NULL) {
		if (!gpio_is_ready_dt(&config->common.en_pin)) {
			LOG_ERR_DEVICE_NOT_READY(config->common.en_pin.port);
			return -ENODEV;
		}

		/* Harmless for a shared EN line, no instance is enabled during init */
		ret = gpio_pin_configure_dt(&config->common.en_pin, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_ERR("%s: failed to configure enable pin: %d", dev->name, ret);
			return ret;
		}
	}

	k_sem_take(&data->sem, K_FOREVER);
	ret = tmc2300_configure(dev, false);
	k_sem_give(&data->sem);

	return ret;
}

static DEVICE_API(stepper, tmc2300_api) = {
	.enable = tmc2300_enable,
	.disable = tmc2300_disable,
	.set_micro_step_res = tmc2300_set_micro_step_res,
	.get_micro_step_res = tmc2300_get_micro_step_res,
};

#define TMC2300_DEFINE(inst)                                                                       \
	static const struct tmc2300_config tmc2300_config_##inst = {                               \
		.common = STEP_DIR_STEPPER_DT_INST_COMMON_CONFIG_INIT(inst),                       \
		.uart = DEVICE_DT_GET(DT_INST_BUS(inst)),                                          \
		.uart_addr = DT_INST_PROP(inst, uart_device_addr),                                 \
		.ihold = DT_INST_PROP(inst, ihold),                                                \
		.irun = DT_INST_PROP(inst, irun),                                                  \
		.iholddelay = DT_INST_PROP(inst, iholddelay),                                      \
		.tpowerdown = DT_INST_PROP(inst, tpowerdown),                                      \
		.extcap = DT_INST_PROP(inst, extcap),                                              \
	};                                                                                         \
	static struct tmc2300_data tmc2300_data_##inst = {                                         \
		.resolution = DT_INST_PROP(inst, micro_step_res),                                  \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, tmc2300_init, NULL, &tmc2300_data_##inst,                      \
			      &tmc2300_config_##inst, POST_KERNEL, CONFIG_STEPPER_INIT_PRIORITY,   \
			      &tmc2300_api);

DT_INST_FOREACH_STATUS_OKAY(TMC2300_DEFINE)
