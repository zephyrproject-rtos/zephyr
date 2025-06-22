/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Prevas A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#include <zephyr/drivers/stepper.h>

#include <adi_tmc_bus.h>
#include "adi_tmc_reg.h"
#include "adi_tmc5xxx_core.h"

#include "tmc51xx.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(tmc5xxx, CONFIG_STEPPER_LOG_LEVEL);

static void rampstat_work_handler(struct k_work *work);
static void tmc51xx_diag0_gpio_callback_handler(const struct device *port, struct gpio_callback *cb,
						gpio_port_pins_t pins);

static int tmc51xx_stepper_set_event_callback(const struct device *dev,
					      stepper_event_callback_t callback, void *user_data)
{
	struct tmc5xxx_stepper_data *data = dev->data;
	const struct tmc5xxx_core_context *ctx = &data->core;
	__maybe_unused const struct tmc5xxx_controller_config *config = ctx->controller_dev->config;

	__maybe_unused int err;

	data->callback = callback;
	data->callback_user_data = user_data;

	/* Configure DIAG0 GPIO interrupt pin */
	IF_ENABLED(TMC51XX_BUS_SPI, ({
	if (config->comm_type == TMC_COMM_SPI && config->diag0_gpio.port) {
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

		err = tmc5xxx_rampstat_read_clear(dev, &rampstat_value);
		if (err != 0) {
			return -EIO;
		}
	}}))

	return 0;
}

static void rampstat_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);

	struct tmc5xxx_stepper_data *stepper_data =
		CONTAINER_OF(dwork, struct tmc5xxx_stepper_data, rampstat_callback_dwork);
	const struct tmc5xxx_core_context *ctx = &stepper_data->core;
	__maybe_unused const struct tmc5xxx_stepper_config *config = stepper_data->core.dev->config;
	int err;
	uint32_t drv_status;

	err = tmc5xxx_read_reg(ctx, TMC5XXX_DRVSTATUS(ctx->motor_index), &drv_status);
	if (err != 0) {
		LOG_ERR("%s: Failed to read DRVSTATUS register", ctx->dev->name);
		return;
	}
#ifdef CONFIG_STEPPER_ADI_TMC51XX_RAMPSTAT_POLL_STALLGUARD_LOG
	tmc5xxx_log_stallguard(dev, drv_status);
#endif
	if (FIELD_GET(TMC5XXX_DRV_STATUS_SG_STATUS_MASK, drv_status) == 1U) {
		LOG_INF("%s: Stall detected", ctx->dev->name);
		err = tmc5xxx_write_reg(ctx, TMC5XXX_RAMPMODE(ctx->motor_index),
					TMC5XXX_RAMPMODE_HOLD_MODE);
		if (err != 0) {
			LOG_ERR("%s: Failed to stop motor", ctx->dev->name);
			return;
		}
	}

	uint32_t rampstat_value;

	err = tmc5xxx_rampstat_read_clear(ctx->dev, &rampstat_value);
	if (err != 0) {
		LOG_ERR("%s: Failed to read RAMPSTAT register", ctx->dev->name);
		return;
	}

	const uint8_t ramp_stat_values = FIELD_GET(TMC5XXX_RAMPSTAT_INT_MASK, rampstat_value);

	if (ramp_stat_values > 0) {
		switch (ramp_stat_values) {
		case TMC5XXX_STOP_LEFT_EVENT:
			LOG_DBG("RAMPSTAT %s:Left end-stop detected", ctx->dev->name);
			tmc5xxx_trigger_callback(ctx->dev, STEPPER_EVENT_LEFT_END_STOP_DETECTED);
			break;

		case TMC5XXX_STOP_RIGHT_EVENT:
			LOG_DBG("RAMPSTAT %s:Right end-stop detected", ctx->dev->name);
			tmc5xxx_trigger_callback(ctx->dev, STEPPER_EVENT_RIGHT_END_STOP_DETECTED);
			break;

		case TMC5XXX_POS_REACHED_EVENT:
		case TMC5XXX_POS_REACHED:
		case TMC5XXX_POS_REACHED_AND_EVENT:
			LOG_DBG("RAMPSTAT %s:Position reached", ctx->dev->name);
			tmc5xxx_trigger_callback(ctx->dev, STEPPER_EVENT_STEPS_COMPLETED);
			break;

		case TMC5XXX_STOP_SG_EVENT:
			LOG_DBG("RAMPSTAT %s:Stall detected", ctx->dev->name);
			tmc5xxx_stallguard_enable(ctx->dev, false);
			tmc5xxx_trigger_callback(ctx->dev, STEPPER_EVENT_STALL_DETECTED);
			break;
		default:
			LOG_ERR("Illegal ramp stat bit field 0x%x", ramp_stat_values);
			break;
		}
	} else {
		/* For SPI with DIAG0 pin, we use interrupt-driven approach */
		IF_ENABLED(TMC51XX_BUS_SPI, ({
			const struct tmc5xxx_controller_config
			*ctrl_config = ctx->controller_dev->config;

			if (ctrl_config->comm_type == TMC_COMM_SPI &&
			    ctrl_config->diag0_gpio.port) {
				/* Using interrupt-driven approach - no polling needed */
				return;
			}
			}))

		/* For UART or SPI without DIAG0, reschedule RAMPSTAT polling */
#ifdef CONFIG_STEPPER_ADI_TMC51XX_RAMPSTAT_POLL_INTERVAL_IN_MSEC
		k_work_reschedule(&stepper_data->rampstat_callback_dwork,
				  K_MSEC(CONFIG_STEPPER_ADI_TMC51XX_RAMPSTAT_POLL_INTERVAL_IN_MSEC)
				  );
#endif /* CONFIG_STEPPER_ADI_TMC51XX_RAMPSTAT_POLL_INTERVAL_IN_MSEC */
	}
}

static void __maybe_unused tmc51xx_diag0_gpio_callback_handler(const struct device *port,
							       struct gpio_callback *cb,
							       gpio_port_pins_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	struct tmc5xxx_stepper_data *stepper_data =
		CONTAINER_OF(cb, struct tmc5xxx_stepper_data, diag0_cb);

	k_work_reschedule(&stepper_data->rampstat_callback_dwork, K_NO_WAIT);
}

#ifdef CONFIG_STEPPER_ADI_TMC51XX_RAMP_GEN

static int tmc51xx_stepper_set_ramp(const struct device *dev,
				    const struct tmc_ramp_generator_data *ramp_data)
{
	struct tmc5xxx_stepper_data const *data = dev->data;
	const struct tmc5xxx_core_context *ctx = &data->core;

	LOG_DBG("Stepper motor controller %s set ramp", dev->name);
	int err;

	err = tmc5xxx_write_reg(ctx, TMC5XXX_VSTART(ctx->motor_index), ramp_data->vstart);
	if (err != 0) {
		return -EIO;
	}
	err = tmc5xxx_write_reg(ctx, TMC5XXX_A1(ctx->motor_index), ramp_data->a1);
	if (err != 0) {
		return -EIO;
	}
	err = tmc5xxx_write_reg(ctx, TMC5XXX_AMAX(ctx->motor_index), ramp_data->amax);
	if (err != 0) {
		return -EIO;
	}
	err = tmc5xxx_write_reg(ctx, TMC5XXX_D1(ctx->motor_index), ramp_data->d1);
	if (err != 0) {
		return -EIO;
	}
	err = tmc5xxx_write_reg(ctx, TMC5XXX_DMAX(ctx->motor_index), ramp_data->dmax);
	if (err != 0) {
		return -EIO;
	}
	err = tmc5xxx_write_reg(ctx, TMC5XXX_V1(ctx->motor_index), ramp_data->v1);
	if (err != 0) {
		return -EIO;
	}
	err = tmc5xxx_write_reg(ctx, TMC5XXX_VMAX(ctx->motor_index), ramp_data->vmax);
	if (err != 0) {
		return -EIO;
	}
	err = tmc5xxx_write_reg(ctx, TMC5XXX_VSTOP(ctx->motor_index), ramp_data->vstop);
	if (err != 0) {
		return -EIO;
	}
	err = tmc5xxx_write_reg(ctx, TMC5XXX_TZEROWAIT(ctx->motor_index), ramp_data->tzerowait);
	if (err != 0) {
		return -EIO;
	}
	err = tmc5xxx_write_reg(ctx, TMC51XX_THIGH, ramp_data->thigh);
	if (err != 0) {
		return -EIO;
	}
	err = tmc5xxx_write_reg(ctx, TMC51XX_TCOOLTHRS, ramp_data->tcoolthrs);
	if (err != 0) {
		return -EIO;
	}
	err = tmc5xxx_write_reg(ctx, TMC51XX_TPWMTHRS, ramp_data->tpwmthrs);
	if (err != 0) {
		return -EIO;
	}
	err = tmc5xxx_write_reg(ctx, TMC51XX_TPOWER_DOWN, ramp_data->tpowerdown);
	if (err != 0) {
		return -EIO;
	}
	err = tmc5xxx_write_reg(ctx, TMC5XXX_IHOLD_IRUN(ctx->motor_index), ramp_data->iholdrun);

	if (err != 0) {
		return -EIO;
	}

	return 0;
}

#endif

static int tmc51xx_controller_init(const struct device *dev)
{
	LOG_DBG("TMC51XX stepper motor controller %s initialized", dev->name);
	const struct tmc5xxx_controller_config *config = dev->config;
	int err;

	err = tmc5xxx_bus_check(dev);
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

	err = tmc5xxx_controller_write_reg(dev, TMC5XXX_GCONF, config->gconf);
	if (err != 0) {
		return -EIO;
	}

	/* Read and write GSTAT register to clear any SPI Datagram errors. */
	uint32_t gstat_value;

	err = tmc5xxx_controller_read_reg(dev, TMC5XXX_GSTAT, &gstat_value);
	if (err != 0) {
		return -EIO;
	}

	LOG_DBG("Device %s initialized", dev->name);
	return 0;
}

static int tmc51xx_stepper_init(const struct device *dev)
{
	struct tmc5xxx_stepper_data *data = dev->data;
	const struct tmc5xxx_core_context *ctx = &data->core;
	const struct tmc5xxx_stepper_config *stepper_config = dev->config;
	int err;

	if (stepper_config->is_sg_enabled) {
		k_work_init_delayable(&data->stallguard_dwork, tmc5xxx_stallguard_work_handler);

		err = tmc5xxx_write_reg(ctx, TMC5XXX_SWMODE(ctx->motor_index), BIT(10));
		if (err != 0) {
			return -EIO;
		}

		LOG_DBG("Setting stall guard to %d with delay %d ms", stepper_config->sg_threshold,
			stepper_config->sg_velocity_check_interval_ms);
		if (!IN_RANGE(stepper_config->sg_threshold, TMC5XXX_SG_MIN_VALUE,
			      TMC5XXX_SG_MAX_VALUE)) {
			LOG_ERR("Stallguard threshold out of range");
			return -EINVAL;
		}

		int32_t stall_guard_threshold = (int32_t)stepper_config->sg_threshold;

		err = tmc5xxx_write_reg(ctx, TMC5XXX_COOLCONF(ctx->motor_index),
					stall_guard_threshold
						<< TMC5XXX_COOLCONF_SG2_THRESHOLD_VALUE_SHIFT);
		if (err != 0) {
			return -EIO;
		}
		k_work_reschedule(&data->stallguard_dwork, K_NO_WAIT);
	}

#ifdef CONFIG_STEPPER_ADI_TMC51XX_RAMP_GEN
	err = tmc51xx_stepper_set_ramp(dev, &stepper_config->default_ramp_config);
	if (err != 0) {
		return -EIO;
	}
#endif

	k_work_init_delayable(&data->rampstat_callback_dwork, rampstat_work_handler);
	uint32_t rampstat_value;
	(void)tmc5xxx_rampstat_read_clear(dev, &rampstat_value);

	err = tmc5xxx_set_micro_step_res(dev, stepper_config->default_micro_step_res);
	if (err != 0) {
		return -EIO;
	}
	return 0;
}

static DEVICE_API(stepper, tmc5xxx_stepper_api) = {
	.enable = tmc5xxx_enable,
	.disable = tmc5xxx_disable,
	.is_moving = tmc5xxx_is_moving,
	.move_by = tmc5xxx_move_by,
	.set_micro_step_res = tmc5xxx_set_micro_step_res,
	.get_micro_step_res = tmc5xxx_get_micro_step_res,
	.set_reference_position = tmc5xxx_set_reference_position,
	.get_actual_position = tmc5xxx_get_actual_position,
	.move_to = tmc5xxx_move_to,
	.run = tmc5xxx_run,
	.set_event_callback = tmc51xx_stepper_set_event_callback,
};

/* Initializes a struct tmc51xx_config for an instance on SPI bus. */
#define TMC51XX_CONFIG_SPI(inst)                                                                   \
	.comm_type = TMC_COMM_SPI,                                                                 \
	.bus.spi = SPI_DT_SPEC_INST_GET(inst,                                                      \
					(SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_MODE_CPOL |   \
					 SPI_MODE_CPHA | SPI_WORD_SET(8)),                         \
					0),                                                        \
	.bus_io = &tmc5xxx_spi_bus_io,                                                             \
	.diag0_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, diag0_gpios, {0})

/* Initializes a struct tmc51xx_config for an instance on UART bus. */
#define TMC51XX_CONFIG_UART(inst)                                                                  \
	.comm_type = TMC_COMM_UART, .bus.uart = DEVICE_DT_GET(DT_INST_BUS(inst)),                  \
	.bus_io = &tmc5xxx_uart_bus_io,                                                            \
	.sw_sel_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, sw_sel_gpios, {0}),                          \
	.uart_addr = DT_INST_PROP_OR(inst, uart_device_addr, 1U)

#define TMC5XXX_SHAFT_CONFIG(child) (DT_PROP(child, invert_direction) << TMC51XX_GCONF_SHAFT_SHIFT)

#define TMC51XX_STEPPER_CONFIG_DEFINE(child)                                                       \
	COND_CODE_1(DT_PROP_EXISTS(child, stallguard_threshold_velocity),			   \
	BUILD_ASSERT(DT_PROP(child, stallguard_threshold_velocity),				   \
		     "stallguard threshold velocity must be a positive value"), ());               \
	IF_ENABLED(CONFIG_STEPPER_ADI_TMC51XX_RAMP_GEN, (CHECK_RAMP_DT_DATA(child)));              \
	static const struct tmc5xxx_stepper_config tmc5xxx_stepper_config_##child = {              \
		.default_micro_step_res = DT_PROP(child, micro_step_res),                          \
		.sg_threshold = DT_PROP(child, stallguard2_threshold),                             \
		.sg_threshold_velocity = DT_PROP(child, stallguard_threshold_velocity),            \
		.sg_velocity_check_interval_ms =                                                   \
			DT_PROP(child, stallguard_velocity_check_interval_ms),                     \
		.is_sg_enabled = DT_PROP(child, activate_stallguard2),                             \
		IF_ENABLED(CONFIG_STEPPER_ADI_TMC51XX_RAMP_GEN,					   \
		(.default_ramp_config = TMC_RAMP_DT_SPEC_GET_TMC51XX(child))) };

#define TMC51XX_STEPPER_DATA_DEFINE(child)                                                         \
	static struct tmc5xxx_stepper_data tmc5xxx_stepper_data_##child = {                        \
		.core =                                                                            \
			{                                                                          \
				.dev = DEVICE_DT_GET(child),                                       \
				.controller_dev = DEVICE_DT_GET(DT_PARENT(child)),                 \
				.motor_index = 0,                                                  \
			},                                                                         \
	};

#define TMC51XX_STEPPER_DEFINE(child)                                                              \
	DEVICE_DT_DEFINE(child, tmc51xx_stepper_init, NULL, &tmc5xxx_stepper_data_##child,         \
			 &tmc5xxx_stepper_config_##child, POST_KERNEL,                             \
			 CONFIG_STEPPER_INIT_PRIORITY, &tmc5xxx_stepper_api);

#define TMC51XX_DEFINE(inst)                                                                       \
	BUILD_ASSERT((DT_INST_PROP(inst, clock_frequency) > 0),                                    \
		     "clock frequency must be non-zero positive value");                           \
                                                                                                   \
	/* Controller data with bus semaphore */                                                   \
	static struct tmc5xxx_controller_data tmc5xxx_controller_data_##inst = {                   \
		.bus_sem = Z_SEM_INITIALIZER((tmc5xxx_controller_data_##inst).bus_sem, 1, 1)};     \
                                                                                                   \
	/* Controller configuration */                                                             \
	static const struct tmc5xxx_controller_config tmc5xxx_controller_config_##inst = {         \
		COND_CODE_1(DT_INST_ON_BUS(inst, spi),						   \
			   (TMC51XX_CONFIG_SPI(inst)), (TMC51XX_CONFIG_UART(inst))),               \
			 .gconf = ((DT_INST_PROP(inst, en_pwm_mode)                                \
				    << TMC51XX_GCONF_EN_PWM_MODE_SHIFT) |                          \
				   (DT_INST_PROP(inst, test_mode)                                  \
				    << TMC51XX_GCONF_TEST_MODE_SHIFT) |                            \
				   DT_INST_FOREACH_CHILD(inst, TMC5XXX_SHAFT_CONFIG) |             \
				   (DT_INST_NODE_HAS_PROP(inst, diag0_gpios)                       \
					    ? BIT(TMC51XX_GCONF_DIAG0_INT_PUSHPULL_SHIFT)          \
					    : 0)),                                                 \
			 .clock_frequency = DT_INST_PROP(inst, clock_frequency)};                  \
                                                                                                   \
	/* Define stepper configs, data, and devices for each child */                             \
	DT_INST_FOREACH_CHILD_STATUS_OKAY(inst, TMC51XX_STEPPER_CONFIG_DEFINE);                    \
	DT_INST_FOREACH_CHILD_STATUS_OKAY(inst, TMC51XX_STEPPER_DATA_DEFINE);                      \
	DT_INST_FOREACH_CHILD_STATUS_OKAY(inst, TMC51XX_STEPPER_DEFINE);                           \
                                                                                                   \
	/* Define the controller device */                                                         \
	DEVICE_DT_INST_DEFINE(inst, tmc51xx_controller_init, NULL,                                 \
			      &tmc5xxx_controller_data_##inst, &tmc5xxx_controller_config_##inst,  \
			      POST_KERNEL, CONFIG_STEPPER_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(TMC51XX_DEFINE)
