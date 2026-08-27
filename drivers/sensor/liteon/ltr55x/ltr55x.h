/*
 * Copyright (c) 2025 Konrad Sikora
 * Copyright (c) 2025 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_LITEON_LTR55X_LTR55X_H_
#define ZEPHYR_DRIVERS_SENSOR_LITEON_LTR55X_LTR55X_H_

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

/* Register addresses */
#define LTR55X_ALS_CONTR         0x80
#define LTR55X_PS_CONTR          0x81
#define LTR55X_PS_LED            0x82
#define LTR55X_PS_N_PULSES       0x83
#define LTR55X_PS_MEAS_RATE      0x84
#define LTR55X_MEAS_RATE         0x85
#define LTR55X_PART_ID           0x86
#define LTR55X_MANUFAC_ID        0x87
#define LTR55X_ALS_DATA_CH1_0    0x88
#define LTR55X_ALS_DATA_CH1_1    0x89
#define LTR55X_ALS_DATA_CH0_0    0x8A
#define LTR55X_ALS_DATA_CH0_1    0x8B
#define LTR55X_ALS_PS_STATUS     0x8C
#define LTR55X_PS_DATA0          0x8D
#define LTR55X_PS_DATA1          0x8E
#define LTR55X_INTERRUPT         0x8F
#define LTR55X_PS_THRES_UP_0     0x90
#define LTR55X_PS_THRES_UP_1     0x91
#define LTR55X_PS_THRES_LOW_0    0x92
#define LTR55X_PS_THRES_LOW_1    0x93
#define LTR55X_PS_OFFSET_1       0x94
#define LTR55X_PS_OFFSET_0       0x95
#define LTR55X_ALS_THRES_UP_0    0x97
#define LTR55X_ALS_THRES_UP_1    0x98
#define LTR55X_ALS_THRES_LOW_0   0x99
#define LTR55X_ALS_THRES_LOW_1   0x9A
#define LTR55X_INTERRUPT_PERSIST 0x9E

/* Bit masks and shifts for ALS_CONTR register */
#define LTR55X_ALS_CONTR_MODE_MASK      BIT(0)
#define LTR55X_ALS_CONTR_MODE_SHIFT     0
#define LTR55X_ALS_CONTR_SW_RESET_MASK  BIT(1)
#define LTR55X_ALS_CONTR_SW_RESET_SHIFT 1
#define LTR55X_ALS_CONTR_GAIN_MASK      GENMASK(4, 2)
#define LTR55X_ALS_CONTR_GAIN_SHIFT     2

/* Bit masks and shifts for MEAS_RATE register */
#define LTR55X_MEAS_RATE_REPEAT_MASK    GENMASK(2, 0)
#define LTR55X_MEAS_RATE_REPEAT_SHIFT   0
#define LTR55X_MEAS_RATE_INT_TIME_MASK  GENMASK(5, 3)
#define LTR55X_MEAS_RATE_INT_TIME_SHIFT 3

/* Bit masks and shift for PS_CONTR register */
#define LTR55X_PS_CONTR_MODE_MASK     GENMASK(1, 0)
#define LTR55X_PS_CONTR_MODE_SHIFT    0
#define LTR55X_PS_CONTR_SAT_IND_MASK  BIT(5)
#define LTR55X_PS_CONTR_SAT_IND_SHIFT 5

/* Bit masks and shift for PS_LED register */
#define LTR55X_PS_LED_PULSE_FREQ_MASK  GENMASK(7, 5)
#define LTR55X_PS_LED_PULSE_FREQ_SHIFT 5
#define LTR55X_PS_LED_DUTY_CYCLE_MASK  GENMASK(4, 3)
#define LTR55X_PS_LED_DUTY_CYCLE_SHIFT 3
#define LTR55X_PS_LED_CURRENT_MASK     GENMASK(2, 0)
#define LTR55X_PS_LED_CURRENT_SHIFT    0

/* Bit masks and shift for PS_N_PULSES register */
#define LTR55X_PS_N_PULSES_COUNT_MASK  GENMASK(3, 0)
#define LTR55X_PS_N_PULSES_COUNT_SHIFT 0

/* Bit masks and shift for PS_MEAS_RATE register */
#define LTR55X_PS_MEAS_RATE_RATE_MASK  GENMASK(3, 0)
#define LTR55X_PS_MEAS_RATE_RATE_SHIFT 0

/* Bit masks and shifts for PART_ID register */
#define LTR55X_PART_ID_REVISION_MASK  GENMASK(3, 0)
#define LTR55X_PART_ID_REVISION_SHIFT 0
#define LTR55X_PART_ID_NUMBER_MASK    GENMASK(7, 4)
#define LTR55X_PART_ID_NUMBER_SHIFT   4

/* Bit masks and shifts for MANUFAC_ID register */
#define LTR55X_MANUFAC_ID_IDENTIFICATION_MASK  GENMASK(7, 0)
#define LTR55X_MANUFAC_ID_IDENTIFICATION_SHIFT 0

/* Bit masks and shifts for ALS_STATUS register */
#define LTR55X_ALS_PS_STATUS_PS_DATA_STATUS_MASK   BIT(0)
#define LTR55X_ALS_PS_STATUS_PS_DATA_STATUS_SHIFT  0
#define LTR55X_ALS_PS_STATUS_PS_INTR_STATUS_MASK   BIT(1)
#define LTR55X_ALS_PS_STATUS_PS_INTR_STATUS_SHIFT  1
#define LTR55X_ALS_PS_STATUS_ALS_DATA_STATUS_MASK  BIT(2)
#define LTR55X_ALS_PS_STATUS_ALS_DATA_STATUS_SHIFT 2
#define LTR55X_ALS_PS_STATUS_ALS_INTR_STATUS_MASK  BIT(3)
#define LTR55X_ALS_PS_STATUS_ALS_INTR_STATUS_SHIFT 3
#define LTR55X_ALS_PS_STATUS_ALS_GAIN_MASK         GENMASK(6, 4)
#define LTR55X_ALS_PS_STATUS_ALS_GAIN_SHIFT        4
#define LTR55X_ALS_PS_STATUS_ALS_DATA_VALID_MASK   BIT(7)
#define LTR55X_ALS_PS_STATUS_ALS_DATA_VALID_SHIFT  7

/* Bit masks for LTR55X-specific registers */
#define LTR55X_INTERRUPT_PS_MASK        BIT(0)
#define LTR55X_INTERRUPT_PS_SHIFT       0
#define LTR55X_INTERRUPT_ALS_MASK       BIT(1)
#define LTR55X_INTERRUPT_ALS_SHIFT      1
#define LTR55X_INTERRUPT_POLARITY_MASK  BIT(2)
#define LTR55X_INTERRUPT_POLARITY_SHIFT 2

#define LTR55X_INTERRUPT_PERSIST_ALS_MASK  GENMASK(3, 0)
#define LTR55X_INTERRUPT_PERSIST_ALS_SHIFT 0
#define LTR55X_INTERRUPT_PERSIST_PS_MASK   GENMASK(7, 4)
#define LTR55X_INTERRUPT_PERSIST_PS_SHIFT  4

#define LTR55X_PS_DATA_MASK 0x07FF
#define LTR55X_PS_DATA_MAX  LTR55X_PS_DATA_MASK

#define LTR55X_ALS_CONTR_MODE_ACTIVE 0x1
#define LTR55X_PS_CONTR_MODE_ACTIVE  0x02

/* Expected sensor IDs */
#define LTR329_PART_ID_VALUE         0xA0
#define LTR55X_PART_ID_VALUE         0x92
#define LTR55X_MANUFACTURER_ID_VALUE 0x05

/* Timing definitions - refer to LTR-329ALS-01 datasheet */
#define LTR55X_INIT_STARTUP_MS        100
#define LTR55X_WAKEUP_FROM_STANDBY_MS 10

/* Convert als-gain value in device-tree to register values */
#define LTR55X_ALS_GAIN_VALUE_1  0
#define LTR55X_ALS_GAIN_VALUE_2  1
#define LTR55X_ALS_GAIN_VALUE_4  2
#define LTR55X_ALS_GAIN_VALUE_8  3
#define LTR55X_ALS_GAIN_VALUE_48 6
#define LTR55X_ALS_GAIN_VALUE_96 7

/* Convert ps-measurement value in device-tree to register values */
#define LTR55X_PS_MEASUREMENT_RATE_VALUE_50   0
#define LTR55X_PS_MEASUREMENT_RATE_VALUE_70   1
#define LTR55X_PS_MEASUREMENT_RATE_VALUE_100  2
#define LTR55X_PS_MEASUREMENT_RATE_VALUE_200  3
#define LTR55X_PS_MEASUREMENT_RATE_VALUE_500  4
#define LTR55X_PS_MEASUREMENT_RATE_VALUE_1000 5
#define LTR55X_PS_MEASUREMENT_RATE_VALUE_2000 6
#define LTR55X_PS_MEASUREMENT_RATE_VALUE_10   8

/* Macros to set and get register fields */
#define LTR55X_REG_SET(reg, field, value)                                                          \
	(((value) << LTR55X_##reg##_##field##_SHIFT) & LTR55X_##reg##_##field##_MASK)
#define LTR55X_REG_GET(reg, field, value)                                                          \
	(((value) & LTR55X_##reg##_##field##_MASK) >> LTR55X_##reg##_##field##_SHIFT)

struct ltr55x_config {
	const struct i2c_dt_spec bus;
	uint8_t part_id;
	uint8_t als_gain;
	uint8_t als_integration_time;
	uint8_t als_measurement_rate;
	uint8_t ps_led_pulse_freq;
	uint8_t ps_led_duty_cycle;
	uint8_t ps_led_current;
	uint8_t ps_n_pulses;
	uint8_t ps_measurement_rate;
	bool ps_saturation_indicator;
};

struct ltr55x_data {
	uint16_t als_ch0;
	uint16_t als_ch1;
	uint16_t ps_ch0;
	uint16_t ps_offset;
	uint16_t ps_upper_threshold;
	uint16_t ps_lower_threshold;
	bool proximity_state;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_LITEON_LTR55X_LTR55X_H_ */
