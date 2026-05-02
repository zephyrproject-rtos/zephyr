/*
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_MAX30210_MAX30210_H_
#define ZEPHYR_DRIVERS_SENSOR_MAX30210_MAX30210_H_

#include <stdint.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/max30210.h>
#include <zephyr/types.h>
#include <zephyr/sys/util.h>
#include <zephyr/device.h>
#include <zephyr/sys/byteorder.h>

/** Register addresses */
#define STATUS                0x00u
#define INTERRUPT_ENABLE      0x02u
#define FIFO_WR_PTR           0x04u
#define FIFO_RD_PTR           0x05u
#define FIFO_COUNTER_1        0x06u
#define FIFO_COUNTER_2        0x07u
#define FIFO_DATA             0x08u
#define FIFO_CONFIG_1         0x09u
#define FIFO_CONFIG_2         0x0Au
#define SYS_CONFIG            0x11u
#define PIN_CONFIG            0x12u
#define TEMP_ALARM_HIGH_SETUP 0x20u
#define TEMP_ALARM_LOW_SETUP  0x21u
#define TEMP_ALARM_HIGH_MSB   0x22u
#define TEMP_ALARM_HIGH_LSB   0x23u
#define TEMP_ALARM_LOW_MSB    0x24u
#define TEMP_ALARM_LOW_LSB    0x25u
#define TEMP_INC_FAST_THRESH  0x26u
#define TEMP_DEC_FAST_THRESH  0x27u
#define TEMP_CONFIG_1         0x28u
#define TEMP_CONFIG_2         0x29u
#define TEMP_CONVERT          0x2Au
#define TEMP_DATA_MSB         0x2Bu
#define TEMP_DATA_LSB         0x2Cu
#define TEMP_SLOPE_MSB        0x2Du
#define TEMP_SLOPE_LSB        0x2Eu
#define UNIQUE_ID1            0x30u
#define UNIQUE_ID2            0x31u
#define UNIQUE_ID3            0x32u
#define UNIQUE_ID4            0x33u
#define UNIQUE_ID5            0x34u
#define UNIQUE_ID6            0x35u
#define PART_ID               0xFFu

/** Status Register Masks */
#define PWR_RDY_MASK       BIT(0)
#define TEMP_HI_MASK       BIT(2)
#define TEMP_LO_MASK       BIT(3)
#define TEMP_INC_FAST_MASK BIT(4)
#define TEMP_DEC_FAST_MASK BIT(5)
#define TEMP_RDY_MASK      BIT(6)
#define FIFO_FULL_MASK     BIT(7)

/** Interrupt Enable Register Masks */
#define TEMP_HI_EN_MASK       BIT(2)
#define TEMP_LO_EN_MASK       BIT(3)
#define TEMP_INC_FAST_EN_MASK BIT(4)
#define TEMP_DEC_FAST_EN_MASK BIT(5)
#define TEMP_RDY_EN_MASK      BIT(6)
#define FIFO_FULL_EN_MASK     BIT(7)

/** FIFO Write Pointer Masks */
#define WRITE_PTR_MASK GENMASK(5, 0)

/** FIFO Read Pointer Masks */
#define READ_PTR_MASK GENMASK(5, 0)

/** FIFO Counter 1 Register Masks */
#define FIFO_OVERFLOW_COUNTER_MASK GENMASK(5, 0)

/** FIFO Counter 2 Register Masks */
#define FIFO_DATA_COUNT_MSB_MASK GENMASK(5, 0)

/** FIFO Data Masks */
#define FIFO_DATA_MASK GENMASK(7, 0)

/** FIFO Config 1 Register Masks */
#define FIFO_A_FULL_MASK GENMASK(5, 0)

/** FIFO Config 2 Register Masks */
#define FLUSH_FIFO_MASK      BIT(4)
#define FIFO_STAT_CLEAR_MASK BIT(3)
#define FIFO_FULL_TYPE_MASK  BIT(2)
#define FIFO_ROLL_OVER_MASK  BIT(1) /* Ignored when in Auto Mode */

/** System Configuration Register Masks */
#define RESET_MASK BIT(0)

/** Pin Configuration Register Masks */
#define EXT_CVT_EN_MASK   BIT(7)
#define EXT_CVT_ICFG_MASK BIT(6)
#define INT_FCFG_MASK     GENMASK(3, 2)
#define INT_OCFG_MASK     GENMASK(1, 0)

/** Temperature Alarm High Setup Register Masks */
#define TEMP_HI_DET_COUNTER_MASK  GENMASK(7, 5)
#define TEMP_HI_ALARM_TRIP_MASK   BIT(3)
#define TEMP_HI_TRIP_COUNTER_MASK GENMASK(2, 1)
#define TEMP_RST_HI_COUNTER       BIT(0)

/** Temperature Alarm Low Setup Register Masks */
#define TEMP_LO_DET_COUNTER_MASK  GENMASK(7, 5)
#define TEMP_LO_ALARM_TRIP_MASK   BIT(3)
#define TEMP_LO_TRIP_COUNTER_MASK GENMASK(2, 1)
#define TEMP_RST_LO_COUNTER       BIT(0)

/** Temperature Alarm High MSB Register Masks */
#define TEMP_HI_MSB_MASK GENMASK(7, 0)

/** Temperature Alarm High LSB Register Masks */
#define TEMP_HI_LSB_MASK GENMASK(7, 0)

/** Temperature Alarm Low MSB Register Masks */
#define TEMP_LO_MSB_MASK GENMASK(7, 0)

/** Temperature Alarm Low LSB Register Masks */
#define TEMP_LO_LSB_MASK GENMASK(7, 0)

/** Temperature Increment Fast Threshold Register Masks */
#define TEMP_INC_FAST_THRESH_MASK GENMASK(7, 0)

/** Temperature Decrement Fast Threshold Register Masks */
#define TEMP_DEC_FAST_THRESH_MASK GENMASK(7, 0)

/** Temperature Configuration 1 Register Masks */
#define CHG_DET_EN_MASK       BIT(3)
#define RATE_CHRG_FILTER_MASK GENMASK(2, 0)

/** Temperature Configuration 2 Register Masks */
#define ALERT_MODE_MASK  BIT(7)
#define TEMP_PERIOD_MASK GENMASK(3, 0)

/** Temperature Convert Register Masks */
#define AUTO_MODE_MASK BIT(1)
#define CONVERT_T_MASK BIT(0)

/** Temperature Data MSB Register Masks */
#define TEMP_DATA_MSB_MASK GENMASK(7, 0)

/** Temperature Data LSB Register Masks */
#define TEMP_DATA_LSB_MASK GENMASK(7, 0)

/** Temperature Slope MSB Register Masks */
#define TEMP_SLOPE_MSB_MASK BIT(0)

/** Temperature Slope LSB Register Masks */
#define TEMP_SLOPE_LSB_MASK GENMASK(7, 0)

/** Unique ID 1 Register Masks */
#define UNIQUE_ID_1_MASK GENMASK(7, 0)

/** Unique ID 2 Register Masks */
#define UNIQUE_ID_2_MASK GENMASK(7, 0)

/** Unique ID 3 Register Masks */
#define UNIQUE_ID_3_MASK GENMASK(7, 0)

/** Unique ID 4 Register Masks */
#define UNIQUE_ID_4_MASK GENMASK(7, 0)

/** Unique ID 5 Register Masks */
#define UNIQUE_ID_5_MASK GENMASK(7, 0)

/** Unique ID 6 Register Masks */
#define UNIQUE_ID_6_MASK GENMASK(7, 0)

/** Part ID Register Masks */
#define PART_ID_MASK GENMASK(7, 0)

#define MAX30210_PART_ID 0x45

#define MAX30210_BYTES_PER_SAMPLE 3 /* 2 bytes for temperature data, 1 byte for status */

#define MAX30210_FIFO_DEPTH 64

#define MAX30210_TEMP_MAX_C               164
#define MAX30210_TEMP_FRAC_MAX_UC         1000000
#define MAX30210_TEMP_FRAC_STEP_UC        5000
#define MAX30210_TEMP_COUNTS_PER_C        200
#define MAX30210_TEMP_REG_BYTES           2
#define MAX30210_TEMP_SLOPE_MAX_REG_VALUE 255

struct max30210_config {
	struct i2c_dt_spec i2c;

#if defined(CONFIG_MAX30210_TRIGGER)
	struct gpio_dt_spec interrupt_gpio; /* GPIO for interrupt */
#endif
	int32_t alarm_high_setup;
	int32_t alarm_low_setup;
	uint8_t inc_fast_thresh;
	uint8_t dec_fast_thresh;
	uint8_t sampling_rate;
	uint8_t hi_trip_count;
	uint8_t lo_trip_count;
	bool hi_trip_non_consecutive;
	bool lo_trip_non_consecutive;
	bool init_start;
};

struct max30210_data {
	uint16_t temp_data;
	uint16_t temp_slope;
	uint8_t int_status;
	uint8_t fifo_data;
	uint8_t ovf_counter;

	/** Temperature Configuration */
	uint16_t temp_alarm_high_setup;
	uint16_t temp_alarm_low_setup;
	uint8_t temp_inc_fast_thresh;
	uint8_t temp_dec_fast_thresh;
	uint8_t temp_config_1;
	uint8_t temp_config_2;

#ifdef CONFIG_MAX30210_TRIGGER
	const struct device *dev;
	struct gpio_callback gpio_cb;

	sensor_trigger_handler_t a_fifo_full_handler;
	sensor_trigger_handler_t temp_hi_handler;
	sensor_trigger_handler_t temp_lo_handler;
	sensor_trigger_handler_t temp_inc_fast_handler;
	sensor_trigger_handler_t temp_dec_fast_handler;
	sensor_trigger_handler_t temp_rdy_handler;

	const struct sensor_trigger *a_fifo_full_trigger;
	const struct sensor_trigger *temp_hi_trigger;
	const struct sensor_trigger *temp_lo_trigger;
	const struct sensor_trigger *temp_inc_fast_trigger;
	const struct sensor_trigger *temp_dec_fast_trigger;
	const struct sensor_trigger *temp_rdy_trigger;

#if defined(CONFIG_MAX30210_TRIGGER_OWN_THREAD)
	struct k_thread thread;

	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_MAX30210_THREAD_STACK_SIZE);
	struct k_sem gpio_sem;
#elif defined(CONFIG_MAX30210_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif /* CONFIG_MAX30210_TRIGGER_OWN_THREAD || CONFIG_MAX30210_TRIGGER_GLOBAL_THREAD*/

#endif /* CONFIG_MAX30210_TRIGGER */
};

#ifdef CONFIG_MAX30210_TRIGGER
int max30210_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler);

int max30210_init_interrupt(const struct device *dev);
#endif

int max30210_reg_read(const struct device *dev, uint8_t reg_addr, uint8_t *val, uint8_t length);

int max30210_reg_write(const struct device *dev, uint8_t reg_addr, uint8_t val);

int max30210_reg_write_multiple(const struct device *dev, uint8_t reg_addr, uint8_t *val,
				uint8_t length);

int max30210_reg_update(const struct device *dev, uint8_t reg_addr, uint8_t mask, uint8_t val);
#endif /* ZEPHYR_DRIVERS_SENSOR_MAX30210_MAX30210_H_ */
