/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Dipak Shetty
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/stepper.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/stepper/stepper_trinamic.h>

#include "adi_tmc5xxx_core.h"
#include "adi_tmc_reg.h"

#include <stdlib.h>

LOG_MODULE_DECLARE(tmc5xxx, CONFIG_STEPPER_LOG_LEVEL);

int tmc5xxx_bus_check(const struct device *dev)
{
	const struct tmc5xxx_controller_config *config = dev->config;

	return config->bus_io->check(&config->bus, config->comm_type);
}

/**
 * @brief Calculate the velocity in full clock cycles from the velocity in Hz
 *
 * @param velocity_hz Velocity in Hz
 * @param clock_frequency Clock frequency in Hz
 *
 * @return Calculated velocity in full clock cycles
 */
static uint32_t tmc5xxx_calculate_velocity_from_hz_to_fclk(uint64_t velocity_hz,
							   uint32_t clock_frequency)
{
	__ASSERT_NO_MSG(clock_frequency);
	return (uint32_t)(velocity_hz << TMC5XXX_CLOCK_FREQ_SHIFT) / clock_frequency;
}

int tmc5xxx_controller_write_reg(const struct device *controller_dev, uint8_t reg, uint32_t value)
{
	const struct tmc5xxx_controller_config *config = controller_dev->config;
	struct tmc5xxx_controller_data *data = controller_dev->data;
	int err;

	k_sem_take(&data->bus_sem, K_FOREVER);

	LOG_DBG("%s: Writing 0x%08x to register 0x%02x", controller_dev->name, value, reg);
	err = config->bus_io->write(controller_dev, reg, value);

	k_sem_give(&data->bus_sem);

	if (err != 0) {
		LOG_ERR("%s: Failed to write register 0x%02x", controller_dev->name, reg);
		return -EIO;
	}

	return 0;
}

int tmc5xxx_controller_read_reg(const struct device *controller_dev, uint8_t reg, uint32_t *value)
{
	const struct tmc5xxx_controller_config *config = controller_dev->config;
	struct tmc5xxx_controller_data *data = controller_dev->data;
	int err;

	k_sem_take(&data->bus_sem, K_FOREVER);

	err = config->bus_io->read(controller_dev, reg, value);

	k_sem_give(&data->bus_sem);

	if (err != 0) {
		LOG_ERR("%s: Failed to read register 0x%02x", controller_dev->name, reg);
		return -EIO;
	}

	LOG_DBG("%s: Read 0x%08x from register 0x%02x", controller_dev->name, *value, reg);
	return 0;
}

int tmc5xxx_write_reg(const struct tmc5xxx_core_context *ctx, uint8_t reg, uint32_t value)
{
	const struct tmc5xxx_controller_config *config = ctx->controller_dev->config;
	struct tmc5xxx_controller_data *controller_data = ctx->controller_dev->data;
	int err;

	k_sem_take(&controller_data->bus_sem, K_FOREVER);

	LOG_DBG("%s: Writing 0x%08x to register 0x%02x", ctx->dev->name, value, reg);
	err = config->bus_io->write(ctx->controller_dev, reg, value);

	k_sem_give(&controller_data->bus_sem);

	if (err != 0) {
		LOG_ERR("%s: Failed to write register 0x%02x", ctx->dev->name, reg);
		return -EIO;
	}

	return 0;
}

int tmc5xxx_read_reg(const struct tmc5xxx_core_context *ctx, uint8_t reg, uint32_t *value)
{
	const struct tmc5xxx_controller_config *config = ctx->controller_dev->config;
	struct tmc5xxx_controller_data *controller_data = ctx->controller_dev->data;

	int err;

	k_sem_take(&controller_data->bus_sem, K_FOREVER);

	err = config->bus_io->read(ctx->controller_dev, reg, value);

	k_sem_give(&controller_data->bus_sem);

	if (err != 0) {
		LOG_ERR("%s: Failed to read register 0x%02x", ctx->dev->name, reg);
		return -EIO;
	}

	LOG_DBG("%s: Read 0x%08x from register 0x%02x", ctx->dev->name, *value, reg);
	return 0;
}

int tmc5xxx_enable(const struct device *dev)
{
	struct tmc5xxx_stepper_data const *data = dev->data;
	const struct tmc5xxx_core_context *ctx = &data->core;

	LOG_DBG("%s: Enabling stepper motor", ctx->dev->name);
	uint32_t reg_value;
	int err;

	err = tmc5xxx_read_reg(ctx, TMC5XXX_CHOPCONF(ctx->motor_index), &reg_value);
	if (err != 0) {
		return err;
	}

	reg_value |= TMC5XXX_CHOPCONF_DRV_ENABLE_MASK;

	err = tmc5xxx_write_reg(ctx, TMC5XXX_CHOPCONF(ctx->motor_index), reg_value);
	if (err != 0) {
		return err;
	}
	atomic_set_bit(data->state, TMC5XXX_MOTOR_ENABLED);

	return 0;
}

int tmc5xxx_disable(const struct device *dev)
{
	struct tmc5xxx_stepper_data const *data = dev->data;
	const struct tmc5xxx_core_context *ctx = &data->core;

	LOG_DBG("%s: Disabling stepper motor", ctx->dev->name);
	uint32_t reg_value;
	int err;

	err = tmc5xxx_read_reg(ctx, TMC5XXX_CHOPCONF(ctx->motor_index), &reg_value);
	if (err != 0) {
		return err;
	}

	reg_value &= ~TMC5XXX_CHOPCONF_DRV_ENABLE_MASK;

	err = tmc5xxx_write_reg(ctx, TMC5XXX_CHOPCONF(ctx->motor_index), reg_value);
	if (err != 0) {
		return err;
	}
	atomic_set_bit(data->state, TMC5XXX_MOTOR_DISABLED);
	return 0;
}

int tmc5xxx_stepper_stop(const struct device *dev)
{
    struct tmc5xxx_stepper_data const *data = dev->data;
    const struct tmc5xxx_core_context *ctx = &data->core;
    const struct tmc5xxx_stepper_config *config = dev->config;
    uint32_t current_mode;
    int err;

    LOG_DBG("%s: Stopping stepper motor", ctx->dev->name);

    /* Read current ramp mode */
    err = tmc5xxx_read_reg(ctx, TMC5XXX_RAMPMODE(ctx->motor_index), &current_mode);
    if (err != 0) {
        LOG_ERR("%s: Failed to read ramp mode", ctx->dev->name);
        return -EIO;
    }

    if (current_mode == TMC5XXX_RAMPMODE_POSITIONING_MODE) {
        /* Stop in positioning mode (option b):
         * Set VSTART=0 and VMAX=0 to decelerate using AMAX/A1
         */
        err = tmc5xxx_write_reg(ctx, TMC5XXX_VSTART(ctx->motor_index), 0);
        if (err != 0) {
            LOG_ERR("%s: Failed to set VSTART to 0", ctx->dev->name);
            return -EIO;
        }

        err = tmc5xxx_write_reg(ctx, TMC5XXX_VMAX(ctx->motor_index), 0);
        if (err != 0) {
            LOG_ERR("%s: Failed to set VMAX to 0", ctx->dev->name);
            return -EIO;
        }

        LOG_DBG("%s: Stopping in positioning mode", ctx->dev->name);
    } else if (current_mode == TMC5XXX_RAMPMODE_POSITIVE_VELOCITY_MODE || 
               current_mode == TMC5XXX_RAMPMODE_NEGATIVE_VELOCITY_MODE) {
        /* Stop in velocity mode (option a):
         * Set AMAX to desired deceleration value and VMAX=0
         */
        err = tmc5xxx_write_reg(ctx, TMC5XXX_AMAX(ctx->motor_index), 
                              config->default_ramp_config.amax);
        if (err != 0) {
            LOG_ERR("%s: Failed to set AMAX for deceleration", ctx->dev->name);
            return -EIO;
        }

        err = tmc5xxx_write_reg(ctx, TMC5XXX_VMAX(ctx->motor_index), 0);
        if (err != 0) {
            LOG_ERR("%s: Failed to set VMAX to 0", ctx->dev->name);
            return -EIO;
        }

        LOG_DBG("%s: Stopping in velocity mode", ctx->dev->name);
    } else {
        /* In hold mode or unknown mode: switch to velocity mode and stop */
        err = tmc5xxx_write_reg(ctx, TMC5XXX_RAMPMODE(ctx->motor_index),
                              TMC5XXX_RAMPMODE_POSITIVE_VELOCITY_MODE);
        if (err != 0) {
            LOG_ERR("%s: Failed to set ramp mode", ctx->dev->name);
            return -EIO;
        }

        err = tmc5xxx_write_reg(ctx, TMC5XXX_VMAX(ctx->motor_index), 0);
        if (err != 0) {
            LOG_ERR("%s: Failed to set VMAX to 0", ctx->dev->name);
            return -EIO;
        }

        LOG_DBG("%s: Switching to velocity mode and stopping", ctx->dev->name);
    }

    /* Update motor state */
    atomic_clear_bit(data->state, TMC5XXX_MOTOR_STOPPED);
    
    return 0;
}

int tmc5xxx_is_moving(const struct device *dev, bool *is_moving)
{
	struct tmc5xxx_stepper_data const *data = dev->data;
	const struct tmc5xxx_core_context *ctx = &data->core;
	uint32_t reg_value;
	int err;

	err = tmc5xxx_read_reg(ctx, TMC5XXX_DRVSTATUS(ctx->motor_index), &reg_value);
	if (err != 0) {
		LOG_ERR("%s: Failed to read DRVSTATUS register", ctx->dev->name);
		return err;
	}

	/* STST bit indicates if motor is standing still (1) or moving (0) */
	*is_moving = (FIELD_GET(TMC5XXX_DRV_STATUS_STST_BIT, reg_value) != 1U);
	LOG_DBG("%s: Motor is %s", ctx->dev->name, *is_moving ? "moving" : "not moving");

	return 0;
}

int tmc5xxx_get_actual_position(const struct device *dev, int32_t *position)
{
	struct tmc5xxx_stepper_data const *data = dev->data;
	const struct tmc5xxx_core_context *ctx = &data->core;
	uint32_t raw_value;
	int err;

	err = tmc5xxx_read_reg(ctx, TMC5XXX_XACTUAL(ctx->motor_index), &raw_value);
	if (err != 0) {
		LOG_ERR("%s: Failed to read XACTUAL register", ctx->dev->name);
		return err;
	}

	/* Sign extend the position value */
	*position = sign_extend(raw_value, TMC_RAMP_XACTUAL_SHIFT);
	LOG_DBG("%s: Actual position: %d", ctx->dev->name, *position);

	return 0;
}

int tmc5xxx_set_reference_position(const struct device *dev, int32_t position)
{
	struct tmc5xxx_stepper_data const *data = dev->data;
	const struct tmc5xxx_core_context *ctx = &data->core;
	int err;

	err = tmc5xxx_write_reg(ctx, TMC5XXX_RAMPMODE(ctx->motor_index),
				TMC5XXX_RAMPMODE_HOLD_MODE);
	if (err != 0) {
		return -EIO;
	}

	err = tmc5xxx_write_reg(ctx, TMC5XXX_XACTUAL(ctx->motor_index), position);
	if (err != 0) {
		return -EIO;
	}
	LOG_DBG("%s: Setting reference position to %d", ctx->dev->name, position);
	return 0;
}

int tmc5xxx_stepper_set_max_velocity(const struct device *dev, uint32_t velocity)
{
	struct tmc5xxx_stepper_data const *data = dev->data;
	const struct tmc5xxx_core_context *ctx = &data->core;
	const struct tmc5xxx_controller_config *config = ctx->controller_dev->config;

	const uint32_t clock_frequency = config->clock_frequency;
	uint32_t velocity_fclk;
	int err;

	velocity_fclk = tmc5xxx_calculate_velocity_from_hz_to_fclk(velocity, clock_frequency);

	err = tmc5xxx_write_reg(ctx, TMC5XXX_VMAX(ctx->motor_index), velocity_fclk);
	if (err != 0) {
		LOG_ERR("%s: Failed to set max velocity", dev->name);
		return -EIO;
	}
	return 0;
}

int tmc5xxx_move_to(const struct device *dev, int32_t position)
{
	struct tmc5xxx_stepper_data *data = dev->data;
	const struct tmc5xxx_core_context *ctx = &data->core;
	const struct tmc5xxx_stepper_config *config = dev->config;
	int err;

	LOG_DBG("%s: Moving to position %d", ctx->dev->name, position);

	/* Disable stallguard if enabled */
	if (config->is_sg_enabled) {
		tmc5xxx_stallguard_enable(dev, false);
	}

	/* Set the ramp mode to positioning mode */
	err = tmc5xxx_write_reg(ctx, TMC5XXX_RAMPMODE(ctx->motor_index),
				TMC5XXX_RAMPMODE_POSITIONING_MODE);
	if (err != 0) {
		return err;
	}

	/* Clear any pending events in RAMPSTAT */
	uint32_t rampstat;

	err = tmc5xxx_rampstat_read_clear(dev, &rampstat);
	if (err != 0) {
		return err;
	}

	/* Set the target position */
	err = tmc5xxx_write_reg(ctx, TMC5XXX_XTARGET(ctx->motor_index), (uint32_t)position);
	if (err != 0) {
		return err;
	}

	/* Re-enable stallguard check if configured */
	if (config->is_sg_enabled) {
		k_work_reschedule(&data->stallguard_dwork,
				  K_MSEC(config->sg_velocity_check_interval_ms));
	}

	/* Set up position monitoring if callback is registered */
	if (data->callback) {
		const struct tmc5xxx_controller_config *ctrl_config = ctx->controller_dev->config;

		/* For SPI with DIAG0 pin, we use interrupt-driven approach */
		if (ctrl_config->comm_type == TMC_COMM_SPI && ctrl_config->diag0_gpio.port) {
			/* Using interrupt-driven approach - no polling needed */
			return 0;
		}

		/* For UART or SPI without DIAG0, schedule RAMPSTAT polling */
#if defined(CONFIG_STEPPER_ADI_TMC50XX_RAMPSTAT_POLL_INTERVAL_IN_MSEC)
		k_work_reschedule(&data->rampstat_callback_dwork,
				  K_MSEC(CONFIG_STEPPER_ADI_TMC50XX_RAMPSTAT_POLL_INTERVAL_IN_MSEC)
				  );
#elif defined(CONFIG_STEPPER_ADI_TMC51XX_RAMPSTAT_POLL_INTERVAL_IN_MSEC)
		k_work_reschedule(&data->rampstat_callback_dwork,
				  K_MSEC(CONFIG_STEPPER_ADI_TMC51XX_RAMPSTAT_POLL_INTERVAL_IN_MSEC)
				  );
#endif
	}
	atomic_set_bit(data->state, TMC5XXX_MOTOR_MOVING);
	return 0;
}

int tmc5xxx_move_by(const struct device *dev, int32_t steps)
{
	struct tmc5xxx_stepper_data const *data = dev->data;
	const struct tmc5xxx_core_context *ctx = &data->core;
	int32_t current_pos;
	int err;

	err = tmc5xxx_get_actual_position(dev, &current_pos);
	if (err != 0) {
		return err;
	}

	LOG_DBG("%s: Moving by %d steps from position %d", ctx->dev->name, steps, current_pos);
	return tmc5xxx_move_to(dev, current_pos + steps);
}

int tmc5xxx_run(const struct device *dev, const enum stepper_direction direction)
{
	struct tmc5xxx_stepper_data *data = dev->data;
	const struct tmc5xxx_core_context *ctx = &data->core;
	const struct tmc5xxx_stepper_config *config = dev->config;
	uint8_t ramp_mode;
	uint32_t rampstat;
	int err;

	/* Set the appropriate ramp mode based on direction */
	if (direction == STEPPER_DIRECTION_POSITIVE) {
		ramp_mode = TMC5XXX_RAMPMODE_POSITIVE_VELOCITY_MODE;
	} else {
		ramp_mode = TMC5XXX_RAMPMODE_NEGATIVE_VELOCITY_MODE;
	}

	/* Clear any pending events in RAMPSTAT */
	err = tmc5xxx_rampstat_read_clear(dev, &rampstat);
	if (err != 0) {
		return err;
	}

	LOG_DBG("%s: Running in %s direction", ctx->dev->name,
	(direction == STEPPER_DIRECTION_POSITIVE) ? "positive" : "negative");
	err = tmc5xxx_write_reg(ctx, TMC5XXX_RAMPMODE(ctx->motor_index), ramp_mode);
	if (err != 0) {
		return -EIO;
	}

	/* Re-enable stallguard check if configured */
	if (config->is_sg_enabled) {
		k_work_reschedule(&data->stallguard_dwork,
				  K_MSEC(config->sg_velocity_check_interval_ms));
	}

	/* Set up position monitoring if callback is registered */
	if (data->callback) {
		const struct tmc5xxx_controller_config *ctrl_config = ctx->controller_dev->config;

		/* For SPI with DIAG0 pin, we use interrupt-driven approach */
		if (ctrl_config->comm_type == TMC_COMM_SPI && ctrl_config->diag0_gpio.port) {
			/* Using interrupt-driven approach - no polling needed */
			return 0;
		}

		/* For UART or SPI without DIAG0, schedule RAMPSTAT polling */
#if defined(CONFIG_STEPPER_ADI_TMC50XX_RAMPSTAT_POLL_INTERVAL_IN_MSEC)
		k_work_reschedule(&data->rampstat_callback_dwork,
				  K_MSEC(CONFIG_STEPPER_ADI_TMC50XX_RAMPSTAT_POLL_INTERVAL_IN_MSEC)
				  );
#elif defined(CONFIG_STEPPER_ADI_TMC51XX_RAMPSTAT_POLL_INTERVAL_IN_MSEC)
		k_work_reschedule(&data->rampstat_callback_dwork,
				  K_MSEC(CONFIG_STEPPER_ADI_TMC51XX_RAMPSTAT_POLL_INTERVAL_IN_MSEC)
				  );
#endif
	}
	atomic_set_bit(data->state, TMC5XXX_MOTOR_MOVING);
	return 0;
}

int tmc5xxx_set_micro_step_res(const struct device *dev,
			       const enum stepper_micro_step_resolution res)
{
	struct tmc5xxx_stepper_data const *data = dev->data;
	const struct tmc5xxx_core_context *ctx = &data->core;

	if (!VALID_MICRO_STEP_RES(res)) {
		LOG_ERR("Invalid micro step resolution %d", res);
		return -ENOTSUP;
	}

	uint32_t reg_value;
	int err;

	err = tmc5xxx_read_reg(ctx, TMC5XXX_CHOPCONF(ctx->motor_index), &reg_value);
	if (err != 0) {
		return -EIO;
	}

	reg_value &= ~TMC5XXX_CHOPCONF_MRES_MASK;
	reg_value |= ((MICRO_STEP_RES_INDEX(STEPPER_MICRO_STEP_256) - LOG2(res))
		      << TMC5XXX_CHOPCONF_MRES_SHIFT);

	err = tmc5xxx_write_reg(ctx, TMC5XXX_CHOPCONF(ctx->motor_index), reg_value);
	if (err != 0) {
		return -EIO;
	}

	LOG_DBG("%s: Set microstep resolution to %d", ctx->dev->name, res);

	return 0;
}

int tmc5xxx_get_micro_step_res(const struct device *dev, enum stepper_micro_step_resolution *res)
{
	struct tmc5xxx_stepper_data const *data = dev->data;
	const struct tmc5xxx_core_context *ctx = &data->core;
	uint32_t reg_value;
	int err;

	err = tmc5xxx_read_reg(ctx, TMC5XXX_CHOPCONF(ctx->motor_index), &reg_value);
	if (err != 0) {
		return -EIO;
	}

	reg_value &= TMC5XXX_CHOPCONF_MRES_MASK;
	reg_value >>= TMC5XXX_CHOPCONF_MRES_SHIFT;
	*res = (1 << (MICRO_STEP_RES_INDEX(STEPPER_MICRO_STEP_256) - reg_value));

	LOG_DBG("%s: Current microstep resolution: %d", ctx->dev->name, *res);
	return 0;
}

int tmc5xxx_stallguard_enable(const struct device *dev, bool enable)
{
	struct tmc5xxx_stepper_data const *data = dev->data;
	const struct tmc5xxx_core_context *ctx = &data->core;
	const struct tmc5xxx_stepper_config *config = dev->config;

	uint32_t reg_value;
	int err;

	err = tmc5xxx_read_reg(ctx, TMC5XXX_SWMODE(ctx->motor_index), &reg_value);
	if (err != 0) {
		return -EIO;
	}

	if (enable) {
		reg_value |= TMC5XXX_SW_MODE_SG_STOP_ENABLE;
		int32_t actual_velocity;

		err = tmc5xxx_read_vactual(dev, &actual_velocity);
		if (err != 0) {
			return -EIO;
		}
		if (abs(actual_velocity) < config->sg_threshold_velocity) {
			LOG_ERR("%s: StallGuard not enabled, actual velocity below threshold",
				ctx->dev->name);
			return -EAGAIN;
		}
	} else {
		reg_value &= ~TMC5XXX_SW_MODE_SG_STOP_ENABLE;
	}

	err = tmc5xxx_write_reg(ctx, TMC5XXX_SWMODE(ctx->motor_index), reg_value);
	if (err != 0) {
		LOG_ERR("%s: Failed to write SWMODE register", ctx->dev->name);
		return -EIO;
	}
	LOG_DBG("%s: StallGuard %s", ctx->dev->name, enable ? "enabled" : "disabled");
	return 0;
}

int tmc5xxx_read_vactual(const struct device *dev, int32_t *velocity)
{
	struct tmc5xxx_stepper_data const *data = dev->data;
	const struct tmc5xxx_core_context *ctx = &data->core;
	uint32_t raw_value;
	int err;

	err = tmc5xxx_read_reg(ctx, TMC5XXX_VACTUAL(ctx->motor_index), &raw_value);
	if (err != 0) {
		LOG_ERR("%s: Failed to read VACTUAL register", ctx->dev->name);
		return err;
	}

	/* Sign extend the velocity value */
	*velocity = sign_extend(raw_value, TMC_RAMP_VACTUAL_SHIFT);

	LOG_DBG("%s: Actual velocity: %d", ctx->dev->name, *velocity);
	return 0;
}

int tmc5xxx_rampstat_read_clear(const struct device *dev, uint32_t *rampstat)
{
	struct tmc5xxx_stepper_data const *data = dev->data;
	const struct tmc5xxx_core_context *ctx = &data->core;
	int err;

	/* Read the RAMPSTAT register */
	err = tmc5xxx_read_reg(ctx, TMC5XXX_RAMPSTAT(ctx->motor_index), rampstat);
	if (err != 0) {
		LOG_ERR("%s: Failed to read RAMPSTAT register", ctx->dev->name);
		return err;
	}

	/* Write back the value to clear the flags */
	err = tmc5xxx_write_reg(ctx, TMC5XXX_RAMPSTAT(ctx->motor_index), *rampstat);
	if (err != 0) {
		LOG_ERR("%s: Failed to clear RAMPSTAT register", ctx->dev->name);
		return err;
	}

	return 0;
}

void tmc5xxx_trigger_callback(const struct device *dev, enum stepper_event event)
{
	struct tmc5xxx_stepper_data *data = dev->data;

	if (!data->callback) {
		LOG_WRN_ONCE("No callback registered");
		return;
	}
	data->callback(data->core.dev, event, data->callback_user_data);
}

void tmc5xxx_stallguard_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct tmc5xxx_stepper_data const *data =
		CONTAINER_OF(dwork, struct tmc5xxx_stepper_data, stallguard_dwork);
	const struct tmc5xxx_core_context *ctx = &data->core;
	const struct tmc5xxx_stepper_config *config = data->core.dev->config;
	int err;

	if (!config->is_sg_enabled) {
		return;
	}

	err = tmc5xxx_stallguard_enable(ctx->dev, true);
	if (err == -EAGAIN) {
		/* Velocity too low, reschedule check */
		k_work_reschedule(dwork, K_MSEC(config->sg_velocity_check_interval_ms));
	}
}

#if defined(CONFIG_STEPPER_ADI_TMC50XX_RAMPSTAT_POLL_STALLGUARD_LOG) ||                            \
	defined(CONFIG_STEPPER_ADI_TMC51XX_RAMPSTAT_POLL_STALLGUARD_LOG)

void tmc5xxx_log_stallguard(struct tmc5xxx_stepper_data *const stepper_data, uint32_t drv_status)
{
	const struct tmc5xxx_core_context *ctx = &stepper_data->core;
	int32_t position;
	int err;

	err = tmc5xxx_get_actual_position(ctx->dev, &position);
	if (err != 0) {
		LOG_ERR("%s: Failed to read XACTUAL register", ctx->dev->name);
		return;
	}

	const uint8_t sg_result = FIELD_GET(TMC5XXX_DRV_STATUS_SG_RESULT_MASK, drv_status);
	const bool sg_status = FIELD_GET(TMC5XXX_DRV_STATUS_SG_STATUS_MASK, drv_status);

	LOG_DBG("%s position: %d | sg result: %3d status: %d", ctx->dev->name, position, sg_result,
		sg_status);
}

#endif
