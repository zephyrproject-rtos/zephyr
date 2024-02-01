/*
 * SPDX-FileCopyrightText: Copyright (c) 2023 Carl Zeiss Meditec AG
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT trinamic_tmc5041

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/stepper_motor_controller.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#ifdef CONFIG_TMC5041_SPI
#include "tmc5041_spi.h"
#endif

#include "zephyr/drivers/stepper_motor/tmc5041.h"

LOG_MODULE_REGISTER(tmc5041, CONFIG_STEPPER_MOTOR_CONTROLLER_LOG_LEVEL);

struct tmc5041_data {
	/** Mutex to prevent further access  on the SPI Bus when a read is underway. */
	struct k_mutex mutex;
	/** INT pin GPIO callback. */
	struct gpio_callback int_cb;
};

struct tmc5041_config {
	/** SPI instance. */
	struct spi_dt_spec spi;
#ifdef CONFIG_TMC5041_INT
	/** INT pin input (optional). */
	struct gpio_dt_spec int_pin;
#endif
};

#ifdef CONFIG_TMC5041_INT
static void tmc5041_int_pin_callback_handler(const struct device *port, struct gpio_callback *cb,
					     gpio_port_pins_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	/* for now, a message is shown */
	LOG_INF("INT pin interrupt detected");
}
#endif /* CONFIG_TMC5041_INT */

static inline int tmc5041_read(const struct device *dev, const uint8_t reg_addr, uint32_t *reg_val)
{
	const struct tmc5041_config *config = dev->config;
	struct tmc5041_data *data = dev->data;
	const struct spi_dt_spec bus = config->spi;
	int status;

	(void)k_mutex_lock(&data->mutex, K_FOREVER);
	status = tmc_spi_read_register(&bus, reg_addr, reg_val);
	(void)k_mutex_unlock(&data->mutex);
	return status;
}

static inline int tmc5041_write(const struct device *dev, const uint8_t reg_addr,
				const uint32_t reg_val)
{
	const struct tmc5041_config *config = dev->config;
	struct tmc5041_data *data = dev->data;
	const struct spi_dt_spec bus = config->spi;
	int status;

	(void)k_mutex_lock(&data->mutex, K_FOREVER);
	status = tmc_spi_write_register(&bus, reg_addr, reg_val);
	(void)k_mutex_unlock(&data->mutex);
	return status;
}

static int tmc5041_init(const struct device *dev)
{
	const struct tmc5041_config *config = dev->config;
	struct tmc5041_data *data = dev->data;
	int status = 0;

	(void)k_mutex_init(&data->mutex);

	/* configure SPI */
	if (!spi_is_ready_dt(&config->spi)) {
		LOG_ERR("SPI bus is not ready");
		return -ENODEV;
	}

#ifdef CONFIG_TMC5041_INT
	/* configure int GPIO */
	if (config->int_pin.port != NULL) {
		if (!device_is_ready(config->int_pin.port)) {
			LOG_ERR("INT GPIO controller not ready");
			return -ENODEV;
		}

		status = gpio_pin_configure_dt(&config->int_pin, GPIO_INPUT);
		if (status < 0) {
			LOG_ERR("Could not configure INT GPIO (%d)", status);
			return status;
		}

		gpio_init_callback(&data->int_cb, tmc5041_int_pin_callback_handler,
				   BIT(config->int_pin.pin));

		status = gpio_add_callback(config->int_pin.port, &data->int_cb);
		if (status < 0) {
			LOG_ERR("Could not add INT pin GPIO callback (%d)", status);
			return status;
		}

		status = gpio_pin_interrupt_configure_dt(&config->int_pin, GPIO_INT_EDGE_RISING);
		if (status < 0) {
			LOG_ERR("failed to configure INT interrupt (err %d)", status);
			return -EINVAL;
		}
	}
#endif /* CONFIG_TMC5041_INT */

	/* Read GSTAT register values to clear any errors SPI Datagram. */
	uint32_t gstat_value;
	(void)tmc5041_read(dev, TMC5041_GSTAT, &gstat_value);
	LOG_INF("Device %s initialized", dev->name);
	return status;
}

int32_t tmc5041_controller_reset(const struct stepper_motor_dt_spec *bus)
{
	/** Doing default initialization of one motor in order to get the driver up and running */

	int32_t register_address, register_value;
	const struct device *dev = bus->bus;

	register_address = TMC5041_GCONF;
	register_value = 0x8;
	tmc5041_write(dev, register_address, register_value);

	register_address = TMC5041_XACTUAL(bus->addr);
	register_value = 0;
	tmc5041_write(dev, register_address, register_value);

	register_address = TMC5041_XTARGET(bus->addr);
	register_value = 0;
	tmc5041_write(dev, register_address, register_value);

	register_address = TMC5041_CHOPCONF(bus->addr);
	register_value = 0x100C5;
	tmc5041_write(dev, register_address, register_value);

	register_address = TMC5041_TZEROWAIT(bus->addr);
	register_value = 100;
	tmc5041_write(dev, register_address, register_value);

	register_address = TMC5041_PWMCONF(bus->addr);
	register_value = 0x401C8;
	tmc5041_write(dev, register_address, register_value);

	register_address = TMC5041_VHIGH(bus->addr);
	register_value = 180000;
	tmc5041_write(dev, register_address, register_value);

	register_address = TMC5041_VCOOLTHRS(bus->addr);
	register_value = 150000;
	tmc5041_write(dev, register_address, register_value);

	register_address = TMC5041_A1(bus->addr);
	register_value = 0xFE80;
	tmc5041_write(dev, register_address, register_value);

	register_address = TMC5041_V1(bus->addr);
	register_value = 0xC350;
	tmc5041_write(dev, register_address, register_value);

	register_address = TMC5041_AMAX(bus->addr);
	register_value = 0x1f000;
	tmc5041_write(dev, register_address, register_value);

	register_address = TMC5041_DMAX(bus->addr);
	register_value = 0x2BCF;
	tmc5041_write(dev, register_address, register_value);

	register_address = TMC5041_VMAX(bus->addr);
	register_value = 270000;
	tmc5041_write(dev, register_address, register_value);

	register_address = TMC5041_D1(bus->addr);
	register_value = 0x578;
	tmc5041_write(dev, register_address, register_value);

	register_address = TMC5041_VSTOP(bus->addr);
	register_value = 0x10;
	tmc5041_write(dev, register_address, register_value);

	register_address = TMC5041_RAMPMODE(bus->addr);
	register_value = 0x00;
	tmc5041_write(dev, register_address, register_value);

	/*Set Micro Stepping*/
	tmc5041_write(dev, TMC5041_MSLUT0(bus->addr), 0xAAAAB554);
	tmc5041_write(dev, TMC5041_MSLUT1(bus->addr), 0x4A9554AA);
	tmc5041_write(dev, TMC5041_MSLUT2(bus->addr), 0x24492929);
	tmc5041_write(dev, TMC5041_MSLUT3(bus->addr), 0x10104222);
	tmc5041_write(dev, TMC5041_MSLUT4(bus->addr), 0xFBFFFFFF);
	tmc5041_write(dev, TMC5041_MSLUT5(bus->addr), 0xB5BB777D);
	tmc5041_write(dev, TMC5041_MSLUT6(bus->addr), 0x49295556);
	tmc5041_write(dev, TMC5041_MSLUT7(bus->addr), 0x00404222);

	tmc5041_write(dev, TMC5041_MSLUTSEL(bus->addr), 0xFFFF8056);
	tmc5041_write(dev, TMC5041_MSLUTSTART(bus->addr), 0x00F70000);

	/* Disable to release motor after stop event */
	tmc5041_write(dev, TMC5041_SWMODE(bus->addr), 0);

	return 0;
}

int32_t tmc5041_controller_write(const struct stepper_motor_dt_spec *bus, int32_t motor_channel,
				 int32_t data)
{

	LOG_DBG("trying to move %s with steps %d", bus->bus->name, data);
	const struct device *dev = bus->bus;

	switch (motor_channel) {
	case FREE_WHEELING: {
		tmc5041_write(dev, TMC5041_RAMPMODE(bus->addr), (uint32_t)data);
		break;
	}
	case TARGET_POSITION: {
		tmc5041_write(dev, TMC5041_XTARGET(bus->addr), (uint32_t)data);
		break;
	}
	case ACTUAL_POSITION: {
		tmc5041_write(dev, TMC5041_XACTUAL(bus->addr), (uint32_t)data);
		break;
	}
	case STALL_DETECTION: {
		tmc5041_write(dev, TMC5041_COOLCONF(bus->addr),
			      data << TMC5041_COOLCONF_SG2_THRESHOLD_VALUE_SHIFT);
		break;
	}
	case STALL_GUARD: {
		tmc5041_write(dev, TMC5041_SWMODE(bus->addr), data);
		break;
	}
	default: {
		break;
	}
	}

	return 0;
}

int32_t tmc5041_controller_read(const struct stepper_motor_dt_spec *bus, int32_t motor_channel,
				int32_t *data)
{
	const struct device *dev = bus->bus;

	switch (motor_channel) {
	case ACTUAL_POSITION: {
		tmc5041_read(dev, TMC5041_XACTUAL(bus->addr), data);
		break;
	}
	case TARGET_POSITION: {
		tmc5041_read(dev, TMC5041_XTARGET(bus->addr), data);
		break;
	}
	case STALL_DETECTION: {
		tmc5041_read(dev, TMC5041_DRVSTATUS(bus->addr), data);
		break;
	}
	case ACTUAL_VELOCITY: {
		tmc5041_read(dev, TMC5041_VACTUAL(bus->addr), data);
		break;
	}
	default:
		break;
	}
	return 0;
}

static const struct stepper_motor_controller_api tmc5041_stepper_motor_controller_api = {
	.stepper_motor_controller_reset = tmc5041_controller_reset,
	.stepper_motor_controller_read = tmc5041_controller_read,
	.stepper_motor_controller_write = tmc5041_controller_write,
	.stepper_motor_controller_write_reg = tmc5041_write,
	.stepper_motor_controller_read_reg = tmc5041_read,
};

#define TMC5041_DEFINE(inst)                                                                       \
	static struct tmc5041_data tmc5041_data_##inst;                                            \
                                                                                                   \
	static const struct tmc5041_config tmc5041_config_##inst = {                               \
		.spi = SPI_DT_SPEC_INST_GET(inst,                                                  \
					    SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB |                \
						    SPI_MODE_CPOL | SPI_MODE_CPHA |                \
						    SPI_WORD_SET(8),                               \
					    0),                                                    \
		IF_ENABLED(CONFIG_TMC5041_INT,                                                 \
			   (.int_pin = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {})))};           \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, tmc5041_init, NULL, &tmc5041_data_##inst,                      \
			      &tmc5041_config_##inst, POST_KERNEL,                                 \
			      CONFIG_APPLICATION_INIT_PRIORITY,                                    \
			      &tmc5041_stepper_motor_controller_api);

DT_INST_FOREACH_STATUS_OKAY(TMC5041_DEFINE)
