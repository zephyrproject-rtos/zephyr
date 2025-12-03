/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Prevas A/S
 * SPDX-FileCopyrightText: Copyright (c) 2025 Dipak Shetty
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_tmc51xx

#include <stdlib.h>

#include <zephyr/drivers/stepper.h>
#include <zephyr/drivers/stepper/stepper_trinamic.h>

#include <adi_tmc_bus.h>
#include <adi_tmc_spi.h>
#include <adi_tmc_uart.h>
#include <adi_tmc5xxx_common.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tmc51xx, CONFIG_STEPPER_LOG_LEVEL);

/* Check for supported bus types */
#define TMC51XX_BUS_SPI  DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#define TMC51XX_BUS_UART DT_ANY_INST_ON_BUS_STATUS_OKAY(uart)

/* Common configuration structure for TMC51xx */
struct tmc51xx_config {
	union tmc_bus bus;
	const struct tmc_bus_io *bus_io;
	uint8_t comm_type;
	const uint32_t gconf;
	const uint32_t clock_frequency;
	const uint16_t default_micro_step_res;
	const int8_t sg_threshold;
	const bool is_sg_enabled;
	const uint32_t sg_velocity_check_interval_ms;
	const uint32_t sg_threshold_velocity;
#ifdef CONFIG_STEPPER_ADI_TMC51XX_RAMP_GEN
	const struct tmc_ramp_generator_data default_ramp_config;
#endif
#if TMC51XX_BUS_UART
	const struct gpio_dt_spec sw_sel_gpio;
	uint8_t uart_addr;
#endif
#if TMC51XX_BUS_SPI
	struct gpio_dt_spec diag0_gpio;
#endif
};

struct tmc51xx_data {
	struct k_sem sem;
	struct k_work_delayable stallguard_dwork;
	struct k_work_delayable rampstat_callback_dwork;
	struct gpio_callback diag0_cb;
	const struct device *stepper;
	stepper_event_callback_t callback;
	void *event_cb_user_data;
};

#if TMC51XX_BUS_SPI

static int tmc51xx_bus_check_spi(const union tmc_bus *bus, uint8_t comm_type)
{
	if (comm_type != TMC_COMM_SPI) {
		return -ENOTSUP;
	}
	return spi_is_ready_dt(&bus->spi) ? 0 : -ENODEV;
}

static int tmc51xx_reg_write_spi(const struct device *dev, const uint8_t reg_addr,
				 const uint32_t reg_val)
{
	const struct tmc51xx_config *config = dev->config;
	int err;

	err = tmc_spi_write_register(&config->bus.spi, TMC5XXX_WRITE_BIT, reg_addr, reg_val);
	if (err < 0) {
		LOG_ERR("Failed to write register 0x%x with value 0x%x", reg_addr, reg_val);
	}

	return err;
}

static int tmc51xx_reg_read_spi(const struct device *dev, const uint8_t reg_addr, uint32_t *reg_val)
{
	const struct tmc51xx_config *config = dev->config;
	int err;

	err = tmc_spi_read_register(&config->bus.spi, TMC5XXX_ADDRESS_MASK, reg_addr, reg_val);
	if (err < 0) {
		LOG_ERR("Failed to read register 0x%x", reg_addr);
	}

	return err;
}

const struct tmc_bus_io tmc51xx_spi_bus_io = {
	.check = tmc51xx_bus_check_spi,
	.read = tmc51xx_reg_read_spi,
	.write = tmc51xx_reg_write_spi,
};
#endif /* TMC51XX_BUS_SPI */

#if TMC51XX_BUS_UART

static int tmc51xx_bus_check_uart(const union tmc_bus *bus, uint8_t comm_type)
{
	if (comm_type != TMC_COMM_UART) {
		return -ENOTSUP;
	}
	return device_is_ready(bus->uart) ? 0 : -ENODEV;
}

static int tmc51xx_reg_write_uart(const struct device *dev, const uint8_t reg_addr,
				  const uint32_t reg_val)
{
	const struct tmc51xx_config *config = dev->config;
	int err;

	/* Route to the adi_tmc_uart.h implementation */
	err = tmc_uart_write_register(config->bus.uart, config->uart_addr, reg_addr, reg_val);
	if (err < 0) {
		LOG_ERR("Failed to write register 0x%x with value 0x%x", reg_addr, reg_val);
	}

	return err;
}

static int tmc51xx_reg_read_uart(const struct device *dev, const uint8_t reg_addr,
				 uint32_t *reg_val)
{
	const struct tmc51xx_config *config = dev->config;
	int err;

	/* Route to the adi_tmc_uart.h implementation */
	err = tmc_uart_read_register(config->bus.uart, config->uart_addr, reg_addr, reg_val);
	if (err < 0) {
		LOG_ERR("Failed to read register 0x%x", reg_addr);
	}

	return err;
}

const struct tmc_bus_io tmc51xx_uart_bus_io = {
	.check = tmc51xx_bus_check_uart,
	.read = tmc51xx_reg_read_uart,
	.write = tmc51xx_reg_write_uart,
};
#endif /* TMC51XX_BUS_UART */

static inline int tmc51xx_bus_check(const struct device *dev)
{
	const struct tmc51xx_config *config = dev->config;

	return config->bus_io->check(&config->bus, config->comm_type);
}

static int read_actual_position(const struct device *dev, int32_t *position);
static void rampstat_work_handler(struct k_work *work);
static void tmc51xx_diag0_gpio_callback_handler(const struct device *port, struct gpio_callback *cb,
						gpio_port_pins_t pins);
static int rampstat_read_clear(const struct device *dev, uint32_t *rampstat_value);

static int tmc51xx_write(const struct device *dev, const uint8_t reg_addr, const uint32_t reg_val)
{
	const struct tmc51xx_config *config = dev->config;
	struct tmc51xx_data *data = dev->data;
	int err;

	k_sem_take(&data->sem, K_FOREVER);

	err = config->bus_io->write(dev, reg_addr, reg_val);

	k_sem_give(&data->sem);

	if (err < 0) {
		LOG_ERR("Failed to write register 0x%x with value 0x%x", reg_addr, reg_val);
		return err;
	}
	return 0;
}

static int tmc51xx_read(const struct device *dev, const uint8_t reg_addr, uint32_t *reg_val)
{
	const struct tmc51xx_config *config = dev->config;
	struct tmc51xx_data *data = dev->data;
	int err;

	k_sem_take(&data->sem, K_FOREVER);

	err = config->bus_io->read(dev, reg_addr, reg_val);

	k_sem_give(&data->sem);

	if (err < 0) {
		LOG_ERR("Failed to read register 0x%x", reg_addr);
		return err;
	}
	return 0;
}

static int tmc51xx_stepper_set_event_callback(const struct device *dev,
					      stepper_event_callback_t callback, void *user_data)
{
	struct tmc51xx_data *data = dev->data;
	__maybe_unused const struct tmc51xx_config *config = dev->config;

	__maybe_unused int err;

	data->callback = callback;
	data->event_cb_user_data = user_data;

	/* Configure DIAG0 GPIO interrupt pin */
	IF_ENABLED(TMC51XX_BUS_SPI, ({
	if ((config->comm_type == TMC_COMM_SPI) && config->diag0_gpio.port) {
		LOG_INF("Configuring DIAG0 GPIO interrupt pin");
		if (!gpio_is_ready_dt(&config->diag0_gpio)) {
			LOG_ERR("DIAG0 interrupt GPIO not ready");
			return -ENODEV;
		}

		err = gpio_pin_configure_dt(&config->diag0_gpio, GPIO_INPUT);
		if (err < 0) {
			LOG_ERR("Could not configure DIAG0 GPIO (%d)", err);
			return err;
		}
		k_work_init_delayable(&data->rampstat_callback_dwork, rampstat_work_handler);

		err = gpio_pin_interrupt_configure_dt(&config->diag0_gpio, GPIO_INT_EDGE_RISING);
		if (err) {
			LOG_ERR("failed to configure DIAG0 interrupt (err %d)", err);
			return -EIO;
		}

		/* Initialize and add GPIO callback */
		gpio_init_callback(&data->diag0_cb, tmc51xx_diag0_gpio_callback_handler,
				   BIT(config->diag0_gpio.pin));

		err = gpio_add_callback(config->diag0_gpio.port, &data->diag0_cb);
		if (err < 0) {
			LOG_ERR("Could not add DIAG0 pin GPIO callback (%d)", err);
			return -EIO;
		}

		/* Clear any pending interrupts */
		uint32_t rampstat_value;

		err = rampstat_read_clear(dev, &rampstat_value);
		if (err != 0) {
			return -EIO;
		}
	}}))

	return 0;
}

static int read_vactual(const struct device *dev, int32_t *actual_velocity)
{
	int err;
	uint32_t raw_value;

	err = tmc51xx_read(dev, TMC51XX_VACTUAL, &raw_value);
	if (err) {
		LOG_ERR("Failed to read VACTUAL register");
		return err;
	}

	*actual_velocity = sign_extend(raw_value, TMC_RAMP_VACTUAL_SHIFT);
	if (*actual_velocity) {
		LOG_DBG("actual velocity: %d", *actual_velocity);
	}
	return 0;
}

static int stallguard_enable(const struct device *dev, const bool enable)
{
	const struct tmc51xx_config *config = dev->config;
	uint32_t reg_value;
	int err;

	err = tmc51xx_read(dev, TMC51XX_SWMODE, &reg_value);
	if (err) {
		LOG_ERR("Failed to read SWMODE register");
		return -EIO;
	}

	if (enable) {
		reg_value |= TMC5XXX_SW_MODE_SG_STOP_ENABLE;

		int32_t actual_velocity;

		err = read_vactual(dev, &actual_velocity);
		if (err) {
			return -EIO;
		}
		if (abs(actual_velocity) < config->sg_threshold_velocity) {
			return -EAGAIN;
		}
	} else {
		reg_value &= ~TMC5XXX_SW_MODE_SG_STOP_ENABLE;
	}
	err = tmc51xx_write(dev, TMC51XX_SWMODE, reg_value);
	if (err) {
		LOG_ERR("Failed to write SWMODE register");
		return -EIO;
	}

	LOG_DBG("Stallguard %s", enable ? "enabled" : "disabled");
	return 0;
}

static void stallguard_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct tmc51xx_data const *stepper_data =
		CONTAINER_OF(dwork, struct tmc51xx_data, stallguard_dwork);
	const struct device *dev = stepper_data->stepper;
	const struct tmc51xx_config *config = dev->config;
	int err;

	err = stallguard_enable(dev, true);
	if (err == -EAGAIN) {
		k_work_reschedule(dwork, K_MSEC(config->sg_velocity_check_interval_ms));
	}
	if (err == -EIO) {
		LOG_ERR("Failed to enable stallguard because of I/O error");
	}
}

static void stepper_trigger_callback(const struct device *dev, const enum stepper_event event)
{
	struct tmc51xx_data *data = dev->data;

	if (!data->callback) {
		LOG_WRN_ONCE("No callback registered");
		return;
	}
	data->callback(dev, event, data->event_cb_user_data);
}

#ifdef CONFIG_STEPPER_ADI_TMC51XX_RAMPSTAT_POLL_STALLGUARD_LOG

static void log_stallguard(const struct device *dev, const uint32_t drv_status)
{
	int32_t position;
	int err;

	err = read_actual_position(dev, &position);
	if (err != 0) {
		LOG_ERR("%s: Failed to read XACTUAL register", dev->name);
		return;
	}

	const uint8_t sg_result = FIELD_GET(TMC5XXX_DRV_STATUS_SG_RESULT_MASK, drv_status);
	const bool sg_status = FIELD_GET(TMC5XXX_DRV_STATUS_SG_STATUS_MASK, drv_status);

	LOG_DBG("%s position: %d | sg result: %3d status: %d", dev->name, position, sg_result,
		sg_status);
}

#endif /* CONFIG_STEPPER_ADI_TMC51XX_RAMPSTAT_POLL_STALLGUARD_LOG */

static int rampstat_read_clear(const struct device *dev, uint32_t *rampstat_value)
{
	int err;

	err = tmc51xx_read(dev, TMC51XX_RAMPSTAT, rampstat_value);
	if (err == 0) {
		err = tmc51xx_write(dev, TMC51XX_RAMPSTAT, *rampstat_value);
	}
	return err;
}

static void rampstat_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);

	struct tmc51xx_data *stepper_data =
		CONTAINER_OF(dwork, struct tmc51xx_data, rampstat_callback_dwork);
	const struct device *dev = stepper_data->stepper;
	__maybe_unused const struct tmc51xx_config *config = dev->config;

	__ASSERT_NO_MSG(dev);

	uint32_t drv_status;
	int err;

	err = tmc51xx_read(dev, TMC51XX_DRVSTATUS, &drv_status);
	if (err != 0) {
		LOG_ERR("%s: Failed to read DRVSTATUS register", dev->name);
		return;
	}
#ifdef CONFIG_STEPPER_ADI_TMC51XX_RAMPSTAT_POLL_STALLGUARD_LOG
	log_stallguard(dev, drv_status);
#endif
	if (FIELD_GET(TMC5XXX_DRV_STATUS_SG_STATUS_MASK, drv_status) == 1U) {
		LOG_INF("%s: Stall detected", dev->name);
		err = tmc51xx_write(dev, TMC51XX_RAMPMODE, TMC5XXX_RAMPMODE_HOLD_MODE);
		if (err != 0) {
			LOG_ERR("%s: Failed to stop motor", dev->name);
			return;
		}
	}

	uint32_t rampstat_value;

	err = rampstat_read_clear(dev, &rampstat_value);
	if (err != 0) {
		LOG_ERR("%s: Failed to read RAMPSTAT register", dev->name);
		return;
	}

	const uint8_t ramp_stat_values = FIELD_GET(TMC5XXX_RAMPSTAT_INT_MASK, rampstat_value);

	if (ramp_stat_values > 0) {
		switch (ramp_stat_values) {
		case TMC5XXX_STOP_LEFT_EVENT:
			LOG_DBG("RAMPSTAT %s:Left end-stop detected", dev->name);
			stepper_trigger_callback(dev, STEPPER_EVENT_LEFT_END_STOP_DETECTED);
			break;

		case TMC5XXX_STOP_RIGHT_EVENT:
			LOG_DBG("RAMPSTAT %s:Right end-stop detected", dev->name);
			stepper_trigger_callback(dev, STEPPER_EVENT_RIGHT_END_STOP_DETECTED);
			break;

		case TMC5XXX_POS_REACHED_EVENT:
		case TMC5XXX_POS_REACHED:
		case TMC5XXX_POS_REACHED_AND_EVENT:
			LOG_DBG("RAMPSTAT %s:Position reached", dev->name);
			stepper_trigger_callback(dev, STEPPER_EVENT_STEPS_COMPLETED);
			break;

		case TMC5XXX_STOP_SG_EVENT:
			LOG_DBG("RAMPSTAT %s:Stall detected", dev->name);
			stallguard_enable(dev, false);
			stepper_trigger_callback(dev, STEPPER_EVENT_STALL_DETECTED);
			break;
		default:
			LOG_ERR("Illegal ramp stat bit field 0x%x", ramp_stat_values);
			break;
		}
	} else {
		/* For SPI with DIAG0 pin, we use interrupt-driven approach */
		IF_ENABLED(TMC51XX_BUS_SPI, ({
			if (config->comm_type == TMC_COMM_SPI && config->diag0_gpio.port) {
				/* Using interrupt-driven approach - no polling needed */
				return;
			}
			}))

		/* For UART or SPI without DIAG0, reschedule RAMPSTAT polling */
#ifdef CONFIG_STEPPER_ADI_TMC51XX_RAMPSTAT_POLL_INTERVAL_IN_MSEC
		k_work_reschedule(
			&stepper_data->rampstat_callback_dwork,
			K_MSEC(CONFIG_STEPPER_ADI_TMC51XX_RAMPSTAT_POLL_INTERVAL_IN_MSEC));
#endif /* CONFIG_STEPPER_ADI_TMC51XX_RAMPSTAT_POLL_INTERVAL_IN_MSEC */
	}
}

static void __maybe_unused tmc51xx_diag0_gpio_callback_handler(const struct device *port,
							       struct gpio_callback *cb,
							       gpio_port_pins_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	struct tmc51xx_data *stepper_data = CONTAINER_OF(cb, struct tmc51xx_data, diag0_cb);

	k_work_reschedule(&stepper_data->rampstat_callback_dwork, K_NO_WAIT);
}

static int tmc51xx_stepper_enable(const struct device *dev)
{
	LOG_DBG("Enabling Stepper motor controller %s", dev->name);
	uint32_t reg_value;
	int err;

	err = tmc51xx_read(dev, TMC51XX_CHOPCONF, &reg_value);
	if (err != 0) {
		return -EIO;
	}

	reg_value |= TMC5XXX_CHOPCONF_DRV_ENABLE_MASK;

	return tmc51xx_write(dev, TMC51XX_CHOPCONF, reg_value);
}

static int tmc51xx_stepper_disable(const struct device *dev)
{
	LOG_DBG("Disabling Stepper motor controller %s", dev->name);
	uint32_t reg_value;
	int err;

	err = tmc51xx_read(dev, TMC51XX_CHOPCONF, &reg_value);
	if (err != 0) {
		return -EIO;
	}

	reg_value &= ~TMC5XXX_CHOPCONF_DRV_ENABLE_MASK;

	return tmc51xx_write(dev, TMC51XX_CHOPCONF, reg_value);
}

static int tmc51xx_stepper_is_moving(const struct device *dev, bool *is_moving)
{
	uint32_t reg_value;
	int err;

	err = tmc51xx_read(dev, TMC51XX_DRVSTATUS, &reg_value);

	if (err != 0) {
		LOG_ERR("%s: Failed to read DRVSTATUS register", dev->name);
		return -EIO;
	}

	*is_moving = (FIELD_GET(TMC5XXX_DRV_STATUS_STST_BIT, reg_value) != 1U);
	LOG_DBG("Stepper motor controller %s is moving: %d", dev->name, *is_moving);
	return 0;
}

int tmc51xx_stepper_set_max_velocity(const struct device *dev, uint32_t velocity)
{
	const struct tmc51xx_config *config = dev->config;
	const uint32_t clock_frequency = config->clock_frequency;
	uint32_t velocity_fclk;
	int err;

	velocity_fclk = tmc5xxx_calculate_velocity_from_hz_to_fclk(velocity, clock_frequency);

	err = tmc51xx_write(dev, TMC51XX_VMAX, velocity_fclk);
	if (err != 0) {
		LOG_ERR("%s: Failed to set max velocity", dev->name);
		return -EIO;
	}
	return 0;
}

static int tmc51xx_stepper_set_micro_step_res(const struct device *dev,
					      enum stepper_micro_step_resolution res)
{
	uint32_t reg_value;
	int err;

	err = tmc51xx_read(dev, TMC51XX_CHOPCONF, &reg_value);
	if (err != 0) {
		return -EIO;
	}

	reg_value &= ~TMC5XXX_CHOPCONF_MRES_MASK;
	reg_value |= ((MICRO_STEP_RES_INDEX(STEPPER_MICRO_STEP_256) - LOG2(res))
		      << TMC5XXX_CHOPCONF_MRES_SHIFT);

	err = tmc51xx_write(dev, TMC51XX_CHOPCONF, reg_value);
	if (err != 0) {
		return -EIO;
	}

	LOG_DBG("Stepper motor controller %s set micro step resolution to 0x%x", dev->name,
		reg_value);
	return 0;
}

static int tmc51xx_stepper_get_micro_step_res(const struct device *dev,
					      enum stepper_micro_step_resolution *res)
{
	uint32_t reg_value;
	int err;

	err = tmc51xx_read(dev, TMC51XX_CHOPCONF, &reg_value);
	if (err != 0) {
		return -EIO;
	}
	reg_value &= TMC5XXX_CHOPCONF_MRES_MASK;
	reg_value >>= TMC5XXX_CHOPCONF_MRES_SHIFT;
	*res = (1 << (MICRO_STEP_RES_INDEX(STEPPER_MICRO_STEP_256) - reg_value));
	LOG_DBG("Stepper motor controller %s get micro step resolution: %d", dev->name, *res);
	return 0;
}

static int tmc51xx_stepper_set_reference_position(const struct device *dev, const int32_t position)
{
	int err;

	err = tmc51xx_write(dev, TMC51XX_RAMPMODE, TMC5XXX_RAMPMODE_HOLD_MODE);
	if (err != 0) {
		return -EIO;
	}

	err = tmc51xx_write(dev, TMC51XX_XACTUAL, position);
	if (err != 0) {
		return -EIO;
	}
	LOG_DBG("Stepper motor controller %s set actual position to %d", dev->name, position);
	return 0;
}

static int read_actual_position(const struct device *dev, int32_t *position)
{
	const struct tmc51xx_config *config = dev->config;

	int err;
	uint32_t raw_value;

	/* Check if device is using UART and is currently moving */
	if (config->comm_type == TMC_COMM_UART) {
		bool is_moving;

		err = tmc51xx_stepper_is_moving(dev, &is_moving);
		if (err != 0) {
			return -EIO;
		}

		if (is_moving) {
			LOG_WRN("%s: Reading position while moving over UART is not supported",
				dev->name);
			return -ENOTSUP;
		}
	}

	err = tmc51xx_read(dev, TMC51XX_XACTUAL, &raw_value);
	if (err != 0) {
		return -EIO;
	}

	*position = sign_extend(raw_value, TMC_RAMP_XACTUAL_SHIFT);
	return 0;
}

static int tmc51xx_stepper_get_actual_position(const struct device *dev, int32_t *position)
{
	int err;

	err = read_actual_position(dev, position);
	if (err != 0) {
		return -EIO;
	}
	LOG_DBG("%s actual position: %d", dev->name, *position);
	return 0;
}

static int tmc51xx_stepper_move_to(const struct device *dev, const int32_t micro_steps)
{
	LOG_DBG("%s set target position to %d", dev->name, micro_steps);
	const struct tmc51xx_config *config = dev->config;
	struct tmc51xx_data *data = dev->data;
	int err;

	if (config->is_sg_enabled) {
		stallguard_enable(dev, false);
	}

	err = tmc51xx_write(dev, TMC51XX_RAMPMODE, TMC5XXX_RAMPMODE_POSITIONING_MODE);
	if (err != 0) {
		return -EIO;
	}
	err = tmc51xx_write(dev, TMC51XX_XTARGET, micro_steps);
	if (err != 0) {
		return -EIO;
	}

	if (config->is_sg_enabled) {
		k_work_reschedule(&data->stallguard_dwork,
				  K_MSEC(config->sg_velocity_check_interval_ms));
	}
	if (data->callback) {
		/* For SPI with DIAG0 pin, we use interrupt-driven approach */
		IF_ENABLED(TMC51XX_BUS_SPI, ({
		if (config->comm_type == TMC_COMM_SPI && config->diag0_gpio.port) {
			/* Using interrupt-driven approach - no polling needed */
			return 0;
		}
		}))

		/* For UART or SPI without DIAG0, reschedule RAMPSTAT polling */
#ifdef CONFIG_STEPPER_ADI_TMC51XX_RAMPSTAT_POLL_INTERVAL_IN_MSEC
		k_work_reschedule(
			&data->rampstat_callback_dwork,
			K_MSEC(CONFIG_STEPPER_ADI_TMC51XX_RAMPSTAT_POLL_INTERVAL_IN_MSEC));
#endif /* CONFIG_STEPPER_ADI_TMC51XX_RAMPSTAT_POLL_INTERVAL_IN_MSEC */
	}
	return 0;
}

static int tmc51xx_stepper_move_by(const struct device *dev, const int32_t micro_steps)
{
	int err;
	int32_t position;

	err = tmc51xx_stepper_get_actual_position(dev, &position);
	if (err != 0) {
		return -EIO;
	}
	int32_t target_position = position + micro_steps;

	LOG_DBG("%s moved to %d by steps: %d", dev->name, target_position, micro_steps);

	return tmc51xx_stepper_move_to(dev, target_position);
}

static int tmc51xx_stepper_run(const struct device *dev, const enum stepper_direction direction)
{
	LOG_DBG("Stepper motor controller %s run", dev->name);
	const struct tmc51xx_config *config = dev->config;
	struct tmc51xx_data *data = dev->data;
	int err;

	if (config->is_sg_enabled) {
		err = stallguard_enable(dev, false);
		if (err != 0) {
			return -EIO;
		}
	}

	switch (direction) {
	case STEPPER_DIRECTION_POSITIVE:
		err = tmc51xx_write(dev, TMC51XX_RAMPMODE, TMC5XXX_RAMPMODE_POSITIVE_VELOCITY_MODE);
		if (err != 0) {
			return -EIO;
		}
		break;

	case STEPPER_DIRECTION_NEGATIVE:
		err = tmc51xx_write(dev, TMC51XX_RAMPMODE, TMC5XXX_RAMPMODE_NEGATIVE_VELOCITY_MODE);
		if (err != 0) {
			return -EIO;
		}
		break;
	}

	if (config->is_sg_enabled) {
		k_work_reschedule(&data->stallguard_dwork,
				  K_MSEC(config->sg_velocity_check_interval_ms));
	}
	if (data->callback) {
		/* For SPI with DIAG0 pin, we use interrupt-driven approach */
		IF_ENABLED(TMC51XX_BUS_SPI, ({
		if (config->comm_type == TMC_COMM_SPI && config->diag0_gpio.port) {
			/* Using interrupt-driven approach - no polling needed */
			return 0;
		}
		}))

		/* For UART or SPI without DIAG0, reschedule RAMPSTAT polling */
#ifdef CONFIG_STEPPER_ADI_TMC51XX_RAMPSTAT_POLL_INTERVAL_IN_MSEC
		k_work_reschedule(
			&data->rampstat_callback_dwork,
			K_MSEC(CONFIG_STEPPER_ADI_TMC51XX_RAMPSTAT_POLL_INTERVAL_IN_MSEC));
#endif /* CONFIG_STEPPER_ADI_TMC51XX_RAMPSTAT_POLL_INTERVAL_IN_MSEC */
	}
	return 0;
}

#ifdef CONFIG_STEPPER_ADI_TMC51XX_RAMP_GEN

int tmc51xx_stepper_set_ramp(const struct device *dev,
			     const struct tmc_ramp_generator_data *ramp_data)
{
	LOG_DBG("Stepper motor controller %s set ramp", dev->name);
	int err;

	err = tmc51xx_write(dev, TMC51XX_VSTART, ramp_data->vstart);
	if (err != 0) {
		return -EIO;
	}
	err = tmc51xx_write(dev, TMC51XX_A1, ramp_data->a1);
	if (err != 0) {
		return -EIO;
	}
	err = tmc51xx_write(dev, TMC51XX_AMAX, ramp_data->amax);
	if (err != 0) {
		return -EIO;
	}
	err = tmc51xx_write(dev, TMC51XX_D1, ramp_data->d1);
	if (err != 0) {
		return -EIO;
	}
	err = tmc51xx_write(dev, TMC51XX_DMAX, ramp_data->dmax);
	if (err != 0) {
		return -EIO;
	}
	err = tmc51xx_write(dev, TMC51XX_V1, ramp_data->v1);
	if (err != 0) {
		return -EIO;
	}
	err = tmc51xx_write(dev, TMC51XX_VMAX, ramp_data->vmax);
	if (err != 0) {
		return -EIO;
	}
	err = tmc51xx_write(dev, TMC51XX_VSTOP, ramp_data->vstop);
	if (err != 0) {
		return -EIO;
	}
	err = tmc51xx_write(dev, TMC51XX_TZEROWAIT, ramp_data->tzerowait);
	if (err != 0) {
		return -EIO;
	}
	err = tmc51xx_write(dev, TMC51XX_THIGH, ramp_data->thigh);
	if (err != 0) {
		return -EIO;
	}
	err = tmc51xx_write(dev, TMC51XX_TCOOLTHRS, ramp_data->tcoolthrs);
	if (err != 0) {
		return -EIO;
	}
	err = tmc51xx_write(dev, TMC51XX_TPWMTHRS, ramp_data->tpwmthrs);
	if (err != 0) {
		return -EIO;
	}
	err = tmc51xx_write(dev, TMC51XX_TPOWER_DOWN, ramp_data->tpowerdown);
	if (err != 0) {
		return -EIO;
	}
	err = tmc51xx_write(dev, TMC51XX_IHOLD_IRUN, ramp_data->iholdrun);
	if (err != 0) {
		return -EIO;
	}
	return 0;
}

#endif

static int tmc51xx_init(const struct device *dev)
{
	LOG_DBG("TMC51XX stepper motor controller %s initialized", dev->name);
	struct tmc51xx_data *data = dev->data;
	const struct tmc51xx_config *config = dev->config;
	int err;

	k_sem_init(&data->sem, 1, 1);

	err = tmc51xx_bus_check(dev);
	if (err < 0) {
		LOG_ERR("Bus not ready for '%s'", dev->name);
		return err;
	}

#if TMC51XX_BUS_UART
	/* Initialize SW_SEL GPIO if using UART and GPIO is specified */
	if (config->comm_type == TMC_COMM_UART && config->sw_sel_gpio.port) {
		if (!gpio_is_ready_dt(&config->sw_sel_gpio)) {
			LOG_ERR("SW_SEL GPIO not ready");
			return -ENODEV;
		}

		err = gpio_pin_configure_dt(&config->sw_sel_gpio, GPIO_OUTPUT_ACTIVE);
		if (err < 0) {
			LOG_ERR("Failed to configure SW_SEL GPIO");
			return err;
		}
	}
#endif

	LOG_DBG("GCONF: %d", config->gconf);
	err = tmc51xx_write(dev, TMC5XXX_GCONF, config->gconf);
	if (err != 0) {
		return -EIO;
	}

	/* Read and write GSTAT register to clear any SPI Datagram errors. */
	uint32_t gstat_value;

	err = tmc51xx_read(dev, TMC5XXX_GSTAT, &gstat_value);
	if (err != 0) {
		return -EIO;
	}

	err = tmc51xx_write(dev, TMC5XXX_GSTAT, gstat_value);
	if (err != 0) {
		return -EIO;
	}

	if (config->is_sg_enabled) {
		k_work_init_delayable(&data->stallguard_dwork, stallguard_work_handler);

		err = tmc51xx_write(dev, TMC51XX_SWMODE, BIT(10));
		if (err != 0) {
			return -EIO;
		}

		LOG_DBG("Setting stall guard to %d with delay %d ms", config->sg_threshold,
			config->sg_velocity_check_interval_ms);
		if (!IN_RANGE(config->sg_threshold, TMC5XXX_SG_MIN_VALUE, TMC5XXX_SG_MAX_VALUE)) {
			LOG_ERR("Stallguard threshold out of range");
			return -EINVAL;
		}

		int32_t stall_guard_threshold = (int32_t)config->sg_threshold;

		err = tmc51xx_write(dev, TMC51XX_COOLCONF,
				    stall_guard_threshold
					    << TMC5XXX_COOLCONF_SG2_THRESHOLD_VALUE_SHIFT);
		if (err != 0) {
			return -EIO;
		}
		k_work_reschedule(&data->stallguard_dwork, K_NO_WAIT);
	}

#ifdef CONFIG_STEPPER_ADI_TMC51XX_RAMP_GEN
	err = tmc51xx_stepper_set_ramp(dev, &config->default_ramp_config);
	if (err != 0) {
		return -EIO;
	}
#endif

	k_work_init_delayable(&data->rampstat_callback_dwork, rampstat_work_handler);
	uint32_t rampstat_value;
	(void)rampstat_read_clear(dev, &rampstat_value);

	err = tmc51xx_stepper_set_micro_step_res(dev, config->default_micro_step_res);
	if (err != 0) {
		return -EIO;
	}
	return 0;
}

static int tmc51xx_stepper_stop(const struct device *dev)
{
	int err;

	err = tmc51xx_write(dev, TMC51XX_RAMPMODE, TMC5XXX_RAMPMODE_POSITIVE_VELOCITY_MODE);
	if (err != 0) {
		return -EIO;
	}

	err = tmc51xx_write(dev, TMC51XX_VMAX, 0);
	if (err != 0) {
		return -EIO;
	}

	return 0;
}

static DEVICE_API(stepper, tmc51xx_api) = {
	.enable = tmc51xx_stepper_enable,
	.disable = tmc51xx_stepper_disable,
	.is_moving = tmc51xx_stepper_is_moving,
	.move_by = tmc51xx_stepper_move_by,
	.set_micro_step_res = tmc51xx_stepper_set_micro_step_res,
	.get_micro_step_res = tmc51xx_stepper_get_micro_step_res,
	.set_reference_position = tmc51xx_stepper_set_reference_position,
	.get_actual_position = tmc51xx_stepper_get_actual_position,
	.move_to = tmc51xx_stepper_move_to,
	.run = tmc51xx_stepper_run,
	.stop = tmc51xx_stepper_stop,
	.set_event_callback = tmc51xx_stepper_set_event_callback,
};

/* Initializes a struct tmc51xx_config for an instance on a SPI bus. */
#define TMC51XX_CONFIG_SPI(inst)                                                                   \
	.comm_type = TMC_COMM_SPI,                                                                 \
	.bus.spi = SPI_DT_SPEC_INST_GET(inst,                                                      \
					(SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_MODE_CPOL |   \
					 SPI_MODE_CPHA | SPI_WORD_SET(8))),                        \
	.bus_io = &tmc51xx_spi_bus_io,                                                             \
	.diag0_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, diag0_gpios, {0})

/* Initializes a struct tmc51xx_config for an instance on a UART bus. */
#define TMC51XX_CONFIG_UART(inst)                                                                  \
	.comm_type = TMC_COMM_UART, .bus.uart = DEVICE_DT_GET(DT_INST_BUS(inst)),                  \
	.bus_io = &tmc51xx_uart_bus_io, .uart_addr = DT_INST_PROP_OR(inst, uart_device_addr, 1U),  \
	.sw_sel_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, sw_sel_gpios, {0})

/* Device initialization macros */
#define TMC51XX_DEFINE(inst)                                                                       \
	BUILD_ASSERT((DT_INST_PROP(inst, clock_frequency) > 0),                                    \
		     "clock frequency must be non-zero positive value");                           \
	static struct tmc51xx_data tmc51xx_data_##inst = {                                         \
		.stepper = DEVICE_DT_GET(DT_DRV_INST(inst))};                                      \
	COND_CODE_1(DT_PROP_EXISTS(inst, stallguard_threshold_velocity),			   \
	BUILD_ASSERT(DT_PROP(inst, stallguard_threshold_velocity),				   \
		     "stallguard threshold velocity must be a positive value"), ());               \
	IF_ENABLED(CONFIG_STEPPER_ADI_TMC51XX_RAMP_GEN, (CHECK_RAMP_DT_DATA(inst)));               \
	static const struct tmc51xx_config tmc51xx_config_##inst = {COND_CODE_1			   \
		(DT_INST_ON_BUS(inst, spi),							   \
		(TMC51XX_CONFIG_SPI(inst)),							   \
		(TMC51XX_CONFIG_UART(inst))),							   \
		 .gconf = ((DT_INST_PROP(inst, en_pwm_mode) << TMC51XX_GCONF_EN_PWM_MODE_SHIFT) |  \
			   (DT_INST_PROP(inst, test_mode) << TMC51XX_GCONF_TEST_MODE_SHIFT) |      \
			   (DT_INST_PROP(inst, invert_direction) << TMC51XX_GCONF_SHAFT_SHIFT) |   \
			   (DT_INST_NODE_HAS_PROP(inst, diag0_gpios)                               \
				    ? BIT(TMC51XX_GCONF_DIAG0_INT_PUSHPULL_SHIFT)                  \
				    : 0)),                                                         \
		 .clock_frequency = DT_INST_PROP(inst, clock_frequency),                           \
		 .default_micro_step_res = DT_INST_PROP(inst, micro_step_res),                     \
		 .sg_threshold = DT_INST_PROP(inst, stallguard2_threshold),                        \
		 .sg_threshold_velocity = DT_INST_PROP(inst, stallguard_threshold_velocity),       \
		 .sg_velocity_check_interval_ms =                                                  \
			 DT_INST_PROP(inst, stallguard_velocity_check_interval_ms),                \
		 .is_sg_enabled = DT_INST_PROP(inst, activate_stallguard2),                        \
		 IF_ENABLED(CONFIG_STEPPER_ADI_TMC51XX_RAMP_GEN,                                   \
		(.default_ramp_config = TMC_RAMP_DT_SPEC_GET_TMC51XX(inst)))};			   \
	DEVICE_DT_INST_DEFINE(inst, tmc51xx_init, NULL, &tmc51xx_data_##inst,                      \
			      &tmc51xx_config_##inst, POST_KERNEL, CONFIG_STEPPER_INIT_PRIORITY,   \
			      &tmc51xx_api);

DT_INST_FOREACH_STATUS_OKAY(TMC51XX_DEFINE)
