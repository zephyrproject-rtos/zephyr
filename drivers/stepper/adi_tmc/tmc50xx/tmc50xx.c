/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Carl Zeiss Meditec AG
 * SPDX-FileCopyrightText: Copyright (c) 2025 Jilay Sandeep Pandya
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_tmc50xx

#include <stdlib.h>

#include <zephyr/drivers/stepper/stepper.h>
#include <zephyr/drivers/stepper/stepper_ctrl.h>
#include <zephyr/drivers/stepper/stepper_trinamic.h>

#include <adi_tmc_spi.h>
#include "tmc50xx.h"
#include <adi_tmc5xxx_common.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tmc50xx, CONFIG_STEPPER_LOG_LEVEL);

struct tmc50xx_data {
	struct k_sem sem;
	/* Work item to run the callback in a thread context. */
	struct k_work_delayable rampstat_callback_dwork;
	const struct device *dev;
	uint8_t work_index;
};

struct tmc50xx_config {
	const uint32_t gconf;
	struct spi_dt_spec spi;
	const uint32_t clock_frequency;
	const struct device **stepper_drivers;
	uint8_t num_stepper_drivers;
	const struct device **motion_controllers;
	uint8_t num_motion_controllers;
};

int tmc50xx_read_actual_position(const struct device *dev, const uint8_t index, int32_t *position)
{
	int err;

	err = tmc50xx_read(dev, TMC50XX_XACTUAL(index), position);
	if (err != 0) {
		return -EIO;
	}
	return 0;
}

int tmc50xx_get_clock_frequency(const struct device *dev)
{
	const struct tmc50xx_config *config = dev->config;

	return config->clock_frequency;
}

int tmc50xx_write(const struct device *dev, const uint8_t reg_addr, const uint32_t reg_val)
{
	const struct tmc50xx_config *config = dev->config;
	struct tmc50xx_data *data = dev->data;
	const struct spi_dt_spec bus = config->spi;
	int err;

	k_sem_take(&data->sem, K_FOREVER);

	err = tmc_spi_write_register(&bus, TMC5XXX_WRITE_BIT, reg_addr, reg_val);

	k_sem_give(&data->sem);

	if (err < 0) {
		LOG_ERR("Failed to write register 0x%x with value 0x%x", reg_addr, reg_val);
		return err;
	}

	return 0;
}

int tmc50xx_read(const struct device *dev, const uint8_t reg_addr, uint32_t *reg_val)
{
	const struct tmc50xx_config *config = dev->config;
	struct tmc50xx_data *data = dev->data;
	const struct spi_dt_spec bus = config->spi;
	int err;

	k_sem_take(&data->sem, K_FOREVER);

	err = tmc_spi_read_register(&bus, TMC5XXX_ADDRESS_MASK, reg_addr, reg_val);

	k_sem_give(&data->sem);

	if (err < 0) {
		LOG_ERR("Failed to read register 0x%x", reg_addr);
		return err;
	}

	return 0;
}

#ifdef CONFIG_STEPPER_ADI_TMC50XX_RAMPSTAT_POLL_STALLGUARD_LOG

static void log_stallguard(const struct device *dev, const uint32_t drv_status)
{
	int32_t position;
	int err;

	err = read_actual_position(dev, &position);
	if (err != 0) {
		LOG_ERR("%s: Failed to read XACTUAL register", dev->name);
		return;
	}

	__maybe_unused const uint8_t sg_result =
		FIELD_GET(TMC5XXX_DRV_STATUS_SG_RESULT_MASK, drv_status);
	__maybe_unused const bool sg_status =
		FIELD_GET(TMC5XXX_DRV_STATUS_SG_STATUS_MASK, drv_status);

	LOG_DBG("%s position: %d | sg result: %3d status: %d", dev->name, position, sg_result,
		sg_status);
}

#endif

void tmc50xx_rampstat_work_reschedule(const struct device *dev)
{
	struct tmc50xx_data *data = dev->data;

	k_work_reschedule(&data->rampstat_callback_dwork,
			  K_MSEC(CONFIG_STEPPER_ADI_TMC50XX_RAMPSTAT_POLL_INTERVAL_IN_MSEC));
}

static void rampstat_work(const struct device *dev)
{
	struct tmc50xx_data *data = dev->data;
	__maybe_unused const struct tmc50xx_config *config = dev->config;
	uint32_t drv_status;
	int err;

	err = tmc50xx_read(dev, TMC50XX_DRVSTATUS(data->work_index), &drv_status);
	if (err != 0) {
		LOG_ERR("%s: Failed to read DRVSTATUS register", data->dev->name);
		return;
	}
#ifdef CONFIG_STEPPER_ADI_TMC50XX_RAMPSTAT_POLL_STALLGUARD_LOG
	log_stallguard(dev, drv_status);
#endif
	if (FIELD_GET(TMC5XXX_DRV_STATUS_SG_STATUS_MASK, drv_status) == 1U) {
		LOG_INF("%s: Stall detected", data->dev->name);
		err = tmc50xx_write(dev, TMC50XX_RAMPMODE(data->work_index),
				    TMC5XXX_RAMPMODE_HOLD_MODE);
		if (err != 0) {
			LOG_ERR("%s: Failed to stop motor", data->dev->name);
			return;
		}
	}

	uint32_t rampstat_value;

	err = tmc50xx_read(dev, TMC50XX_RAMPSTAT(data->work_index), &rampstat_value);
	if (err != 0) {
		LOG_ERR("%s: Failed to read RAMPSTAT register", data->dev->name);
		return;
	}

	const uint8_t ramp_stat_values = FIELD_GET(TMC5XXX_RAMPSTAT_INT_MASK, rampstat_value);

	if (ramp_stat_values > 0) {
		switch (ramp_stat_values) {
#ifdef CONFIG_STEPPER_ADI_TMC50XX_STEPPER_CTRL
		case TMC5XXX_STOP_LEFT_EVENT:
			LOG_DBG("RAMPSTAT %s:Left end-stop detected", data->dev->name);
			tmc50xx_stepper_ctrl_trigger_cb(
				config->motion_controllers[data->work_index],
				STEPPER_CTRL_EVENT_LEFT_END_STOP_DETECTED);
			break;

		case TMC5XXX_STOP_RIGHT_EVENT:
			LOG_DBG("RAMPSTAT %s:Right end-stop detected", data->dev->name);
			tmc50xx_stepper_ctrl_trigger_cb(
				config->motion_controllers[data->work_index],
				STEPPER_CTRL_EVENT_RIGHT_END_STOP_DETECTED);
			break;

		case TMC5XXX_POS_REACHED_EVENT:
		case TMC5XXX_POS_REACHED:
		case TMC5XXX_POS_REACHED_AND_EVENT:
			LOG_DBG("RAMPSTAT %s:Position reached", data->dev->name);
			tmc50xx_stepper_ctrl_trigger_cb(
				config->motion_controllers[data->work_index],
				STEPPER_CTRL_EVENT_STEPS_COMPLETED);
			break;
#endif
#ifdef CONFIG_STEPPER_ADI_TMC50XX_STEPPER_DRIVER
		case TMC5XXX_STOP_SG_EVENT:
			LOG_DBG("RAMPSTAT %s:Stall detected", data->dev->name);
			tmc50xx_stepper_ctrl_stallguard_enable(data->dev, false);
			tmc50xx_stepper_driver_trigger_cb(config->stepper_drivers[data->work_index],
							  STEPPER_EVENT_STALL_DETECTED);
			break;
#endif
		default:
			LOG_ERR("Illegal ramp stat bit field 0x%x", ramp_stat_values);
			break;
		}
	} else {
		tmc50xx_rampstat_work_reschedule(dev);
	}
}

static void rampstat_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct tmc50xx_data *data =
		CONTAINER_OF(dwork, struct tmc50xx_data, rampstat_callback_dwork);
	const struct device *dev = data->dev;
	const struct tmc50xx_config *config = dev->config;

	for (uint8_t i = 0; i < config->num_stepper_drivers; i++) {
		data->work_index = tmc50xx_stepper_ctrl_index(config->motion_controllers[i]);
		rampstat_work(dev);
	}
}

static int tmc50xx_init(const struct device *dev)
{
	struct tmc50xx_data *data = dev->data;
	const struct tmc50xx_config *config = dev->config;
	int err;

	LOG_DBG("Initializing TMC50XX stepper motor controller %s", dev->name);
	k_sem_init(&data->sem, 1, 1);

	if (!spi_is_ready_dt(&config->spi)) {
		LOG_ERR("SPI bus is not ready");
		return -ENODEV;
	}

	/* Init non motor-index specific registers here. */
	LOG_DBG("GCONF: %d", config->gconf);
	err = tmc50xx_write(dev, TMC5XXX_GCONF, config->gconf);
	if (err != 0) {
		return -EIO;
	}

	/* Read GSTAT register values to clear any errors SPI Datagram. */
	uint32_t gstat_value;

	err = tmc50xx_read(dev, TMC5XXX_GSTAT, &gstat_value);
	if (err != 0) {
		return -EIO;
	}

	LOG_DBG("Num of motion controllers: %d %s", config->num_motion_controllers,
		config->motion_controllers[0]->name);
	for (uint8_t i = 0; i < config->num_stepper_drivers; i++) {
		if (config->stepper_drivers[i] != NULL) {
			LOG_DBG("Stepper driver %s", config->stepper_drivers[i]->name);
		}
	}

	for (uint8_t i = 0; i < config->num_motion_controllers; i++) {
		if (config->motion_controllers[i] != NULL) {
			LOG_DBG("Motion controller %s", config->motion_controllers[i]->name);
		}
	}
	k_work_init_delayable(&data->rampstat_callback_dwork, rampstat_work_handler);

	LOG_DBG("Device %s initialized", dev->name);
	return 0;
}

#define TMC50XX_CHILD_DEVICES_ARRAY(inst, compat)                                                  \
	{DT_INST_FOREACH_CHILD_STATUS_OKAY_VARGS(inst, TMC50XX_CHILD_DEVICE_GET, compat)}

#define TMC50XX_CHILD_DEVICE_GET(node_id, compat)                                                  \
	COND_CODE_1(DT_NODE_HAS_COMPAT(node_id, compat), (DEVICE_DT_GET(node_id),), ())

#define TMC50XX_DEFINE(inst)                                                                       \
	static const struct device *tmc50xx_stepper_drivers_##inst[] =                             \
		TMC50XX_CHILD_DEVICES_ARRAY(inst, adi_tmc50xx_stepper_driver);                     \
	static const struct device *tmc50xx_motion_controllers_##inst[] =                          \
		TMC50XX_CHILD_DEVICES_ARRAY(inst, adi_tmc50xx_stepper_ctrl);                      \
	BUILD_ASSERT(ARRAY_SIZE(tmc50xx_motion_controllers_##inst) <= 2,                           \
		     "tmc50xx can drive two steppers at max");                                     \
	BUILD_ASSERT(ARRAY_SIZE(tmc50xx_stepper_drivers_##inst) <= 2,                              \
		     "tmc50xx can drive two steppers at max");                                     \
	BUILD_ASSERT((DT_INST_PROP(inst, clock_frequency) > 0),                                    \
		     "clock frequency must be non-zero positive value");                           \
	static struct tmc50xx_data tmc50xx_data_##inst = {                                         \
		.dev = DEVICE_DT_GET(DT_DRV_INST(inst))};                                          \
	static const struct tmc50xx_config tmc50xx_config_##inst = {                               \
		.gconf = ((DT_INST_PROP(inst, poscmp_enable)                                       \
			   << TMC50XX_GCONF_POSCMP_ENABLE_SHIFT) |                                 \
			  (DT_INST_PROP(inst, test_mode) << TMC50XX_GCONF_TEST_MODE_SHIFT) |       \
			  DT_INST_PROP(inst, shaft1) << TMC50XX_GCONF_SHAFT_SHIFT(0) |             \
			  DT_INST_PROP(inst, shaft2) << TMC50XX_GCONF_SHAFT_SHIFT(1) |             \
			  (DT_INST_PROP(inst, lock_gconf) << TMC50XX_LOCK_GCONF_SHIFT)),           \
		.spi = SPI_DT_SPEC_INST_GET(inst,                                                  \
					    (SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB |               \
					     SPI_MODE_CPOL | SPI_MODE_CPHA | SPI_WORD_SET(8))),    \
		.clock_frequency =                                                                 \
			DT_INST_PROP(inst, clock_frequency), /* Child device references */         \
		.stepper_drivers = tmc50xx_stepper_drivers_##inst,                                 \
		.num_stepper_drivers = ARRAY_SIZE(tmc50xx_stepper_drivers_##inst),                 \
		.motion_controllers = tmc50xx_motion_controllers_##inst,                           \
		.num_motion_controllers = ARRAY_SIZE(tmc50xx_motion_controllers_##inst),           \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, tmc50xx_init, NULL, &tmc50xx_data_##inst,                      \
			      &tmc50xx_config_##inst, POST_KERNEL, CONFIG_STEPPER_INIT_PRIORITY,   \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(TMC50XX_DEFINE)
