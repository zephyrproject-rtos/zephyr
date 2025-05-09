/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Dipak Shetty
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_STEPPER_ADI_TMC51XX_H
#define ZEPHYR_DRIVERS_STEPPER_ADI_TMC51XX_H

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/stepper/stepper_trinamic.h>

#include <adi_tmc_bus.h>

#define DT_DRV_COMPAT adi_tmc51xx

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
/* SPI bus I/O operations for TMC51xx devices */
extern const struct tmc_bus_io tmc51xx_spi_bus_io;
#endif

#if TMC51XX_BUS_UART
/* UART bus I/O operations for TMC51xx devices */
extern const struct tmc_bus_io tmc51xx_uart_bus_io;
#endif

#endif /* ZEPHYR_DRIVERS_STEPPER_ADI_TMC51XX_H */
