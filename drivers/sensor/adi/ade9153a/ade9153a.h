/*
 * Copyright (c) 2024 Plentify (Pty) Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ADI_ADE9153A_H
#define ZEPHYR_DRIVERS_SENSOR_ADI_ADE9153A_H

#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/sensor/ade9153a.h>

#define DECODE_Q1_31(_var)                                                                         \
	(((double)(CONFIG_ADE9153A_CAL_##_var##_CC) / INT32_MAX) *                                 \
	 (1U << CONFIG_ADE9153A_CAL_##_var##_CC_SHIFT))

#define CAL_IRMS_CC   DECODE_Q1_31(IRMS) /* (uA/code) */
#define CAL_VRMS_CC   DECODE_Q1_31(VRMS) /* (uV/code) */
/* (uW/code) Applicable for Active, reactive and apparent power */
#define CAL_POWER_CC  DECODE_Q1_31(POWER)
/* (uWhr/xTHR_HI code)Applicable for Active, reactive and apparent energy */
#define CAL_ENERGY_CC DECODE_Q1_31(ENERGY)

struct ade9153a_data {
	int32_t active_energy_reg;
	int32_t fund_reactive_energy_reg;
	int32_t apparent_energy_reg;
	int32_t active_power_reg;
	int32_t fund_reactive_power_reg;
	int32_t apparent_power_reg;
	int32_t current_rms_reg;
	int32_t voltage_rms_reg;
	int32_t half_current_rms_reg;
	int32_t half_voltage_rms_reg;
	int32_t power_factor_reg;
	int32_t period_reg;
	int16_t acc_mode_reg;
	int32_t angle_reg_av_ai_reg;
	uint32_t temperature_trim;
	uint16_t temperature_offset;
	uint32_t temperature_gain;
	uint16_t temperature_reg;
	union ade9153a_status_reg status_reg;

#ifdef CONFIG_ADE9153A_TRIGGER
	const struct device *dev;
	struct gpio_callback gpio_cb;

	sensor_trigger_handler_t irq_handler;
	const struct sensor_trigger *irq_trigger;
	sensor_trigger_handler_t cf_handler;
	const struct sensor_trigger *cf_trigger;

	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_ADE9153A_THREAD_STACK_SIZE);
	struct k_msgq trigger_pins_msgq;
	char trigger_msgq_buffer[10 * sizeof(uint32_t)];
	struct k_thread thread;
#endif /* CONFIG_ADE9153A_TRIGGER */
};

struct ade9153a_config {
	struct spi_dt_spec spi_dt_spec;
	struct gpio_dt_spec reset_gpio_dt_spec;
#ifdef CONFIG_ADE9153A_TRIGGER
	struct gpio_dt_spec cf_gpio_dt_spec;
	struct gpio_dt_spec irq_gpio_dt_spec;
#endif
};

#if defined(CONFIG_ADE9153A_TRIGGER) || defined(__DOXYGEN__)
/**
 * @brief Set the trigger for an specific GPIO.
 *
 * This sensor generates interruptions related to IRQ and CF pins.
 *
 * @param dev The sensor device.
 * @param trig Trigger to set.
 * @param handler The handler function to be called.
 * @retval 0 on success.
 */
int ade9153a_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler);

int ade9153a_init_interrupt(const struct device *dev);
#endif /* CONFIG_ADE9153A_TRIGGER */

#endif /* end ifndef ZEPHYR_DRIVERS_SENSOR_ADI_ADE9153A_H */
