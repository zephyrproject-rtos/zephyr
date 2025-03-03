/*
 * Copyright (c) 2025 Cameron Ewing
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_AS5600_AS5600_H_
#define ZEPHYR_DRIVERS_SENSOR_AS5600_AS5600_H_

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>

/* Register Addresses */
#define AS5600_ZMCO             0x00
#define AS5600_ZPOS_H           0x01
#define AS5600_ZPOS_L           0x02
#define AS5600_MPOS_H           0x03
#define AS5600_MPOS_L           0x04
#define AS5600_MANG_H           0x05
#define AS5600_MANG_L           0x06
#define AS5600_CONF_H           0x07
#define AS5600_CONF_L           0x08
#define AS5600_STATUS           0x0B
#define AS5600_RAW_STEPS_H      0x0C
#define AS5600_RAW_STEPS_L      0x0D
#define AS5600_FILTERED_STEPS_H 0x0E
#define AS5600_FILTERED_STEPS_L 0x0F
#define AS5600_AGC              0x1A
#define AS5600_MAGNITUDE_H      0x1B
#define AS5600_MAGNITUDE_L      0x1C
#define AS5600_I2CADDR          0x20
#define AS5600_I2CUPDT          0x21
#define AS5600_BURN             0xFF

/* Register Masks */
#define AS5600_ZMCO_MASK    (BIT(1) | BIT(0))
#define AS5600_12BIT_H_MASK (BIT(3) | BIT(2) | BIT(1) | BIT(0))
#define AS5600_CONF_H_MASK  (BIT(5) | BIT(4) | BIT(3) | BIT(2) | BIT(1) | BIT(0))
#define AS5600_STATUS_MASK  (BIT(5) | BIT(4) | BIT(3))
#define AS5600_I2CADDR_MASK (BIT(7) | BIT(6) | BIT(5) | BIT(4) | BIT(3) | BIT(2) | BIT(1))

/* Configuration Masks
 * CONF_H:  Watch Dog:5 | Fast Filter Threshold:4:2 | Slow Filter:1:0
 */
#define AS5600_CONF_H_SF   (BIT(1) | BIT(0))
#define AS5600_CONF_H_FTH  (BIT(4) | BIT(3) | BIT(2))
#define AS5600_CONF_H_WD   (BIT(5))
/* CONF_L:  PWM Frequency:7:6 | Output Stage:5:4 | Hysteresis:3:2 | Power Mode:1:0 */
#define AS5600_CONF_L_PM   (BIT(1) | BIT(0))
#define AS5600_CONF_L_HYST (BIT(3) | BIT(2))
#define AS5600_CONF_L_OUTS (BIT(5) | BIT(4))
#define AS5600_CONF_L_PWMF (BIT(7) | BIT(6))

/* Status Masks
 * STATUS:  Magnet Detected:5 | Magnet Too Weak:4 | Magnet Too Strong:3
 */
#define AS5600_STATUS_MH (BIT(3))
#define AS5600_STATUS_ML (BIT(4))
#define AS5600_STATUS_MD (BIT(5))

/* Burn Commands */
#define AS5600_BURN_ANGLE   0x80
#define AS5600_BURN_SETTING 0x40

/* Register Sizes */
#define AS5600_TWO_REG 2

/* Unconfigured configuration value */
#define AS5600_UNCONFIGURED_2_REG USHRT_MAX
#define AS5600_UNCONFIGURED_1_REG UCHAR_MAX

/* Combine two 8-bit registers into a 12-bit value using supplied mask and values */
#define AS5600_REG_TO_12BIT(reg_h, mask_h, reg_l) (((reg_h & mask_h) << 8) | reg_l)

/* Mask and shift bitfield right using supplied register, mask, and shift */
#define AS5600_BITFIELD_TO_VAL(reg, mask, shift) ((reg & mask) >> shift)

/* Mask and shift a value left into a bitfield using supplied value, mask, and shift */
#define AS5600_VAL_TO_BITFIELD(val, mask, shift) ((val << shift) & mask)

/* Sensor Channels */
enum as5600_sensor_channel {
	AS5600_SENSOR_CHAN_RAW_STEPS = SENSOR_CHAN_PRIV_START,
	AS5600_SENSOR_CHAN_FILTERED_STEPS,
	AS5600_SENSOR_CHAN_AGC,
	AS5600_SENSOR_CHAN_MAGNITUDE,
	AS5600_SENSOR_CHAN_END,
};

/* Sensor Attributes
 *
 * NOTE: END_POSITION and ANGULAR_RANGE are mutually exclusive:
 * The last set value will be used and the other will be cleared to 0 on the device.
 */
enum as5600_sensor_attribute {
	AS5600_ATTR_BURNS = SENSOR_ATTR_PRIV_START, /* Read only */
	AS5600_ATTR_START_POSITION,                 /* Read / Write */
	AS5600_ATTR_END_POSITION,                   /* Read / Write */
	AS5600_ATTR_ANGULAR_RANGE,                  /* Read / Write */
	AS5600_ATTR_POWER_MODE,                     /* Read / Write */
	AS5600_ATTR_OUTPUT_STAGE,                   /* Read / Write */
	AS5600_ATTR_PWM_FREQ,                       /* Read / Write */
	AS5600_ATTR_SLOW_FILTER,                    /* Read / Write */
	AS5600_ATTR_FAST_FILTER_THRESHOLD,          /* Read / Write */
	AS5600_ATTR_WATCH_DOG,                      /* Read / Write */
	AS5600_ATTR_I2CADDR,                        /* Read / Write */
	AS5600_ATTR_STATUS,                         /* Read only */
	AS5600_ATTR_END,
};

enum as5600_hysteresis {
	AS5600_HYSTERESIS_0,
	AS5600_HYSTERESIS_1,
	AS5600_HYSTERESIS_2,
	AS5600_HYSTERESIS_3,
};

enum as5600_power_mode {
	AS5600_POWER_MODE_NOM,
	AS5600_POWER_MODE_LPM1,
	AS5600_POWER_MODE_LPM2,
	AS5600_POWER_MODE_LPM3,
};

enum as5600_output_stage {
	AS5600_OUTPUT_STAGE_ANALOG,
	AS5600_OUTPUT_STAGE_REDUCED,
	AS5600_OUTPUT_STAGE_PWM,
};

enum as5600_pwm_frequency {
	AS5600_PWM_FREQ_115HZ,
	AS5600_PWM_FREQ_230HZ,
	AS5600_PWM_FREQ_460HZ,
	AS5600_PWM_FREQ_920HZ,
};

enum as5600_slow_filter {
	AS5600_SLOW_FILTER_16,
	AS5600_SLOW_FILTER_8,
	AS5600_SLOW_FILTER_4,
	AS5600_SLOW_FILTER_2,
};

enum as5600_fast_filter_threshold {
	AS5600_FAST_FILTER_THRESHOLD_OFF,
	AS5600_FAST_FILTER_THRESHOLD_6_1,
	AS5600_FAST_FILTER_THRESHOLD_7_1,
	AS5600_FAST_FILTER_THRESHOLD_9_1,
	AS5600_FAST_FILTER_THRESHOLD_18_2,
	AS5600_FAST_FILTER_THRESHOLD_21_2,
	AS5600_FAST_FILTER_THRESHOLD_24_2,
	AS5600_FAST_FILTER_THRESHOLD_10_4,
};

enum as5600_status {
	AS5600_STATUS_MAGNET_DETECTED = 0b100,
	AS5600_STATUS_MAGNET_TOO_WEAK = 0b110,
	AS5600_STATUS_MAGNET_TOO_STRONG = 0b101,
};

enum as5600_watch_dog {
	AS5600_WATCH_DOG_DISABLE,
	AS5600_WATCH_DOG_ENABLE,
};

struct as5600_config {
	const struct i2c_dt_spec i2c;
	const uint16_t start_position;
	const uint16_t end_position;
	const uint16_t angular_range;
	const enum as5600_power_mode power_mode;
	const enum as5600_hysteresis hysteresis;
	const enum as5600_output_stage output_stage;
	const enum as5600_pwm_frequency pwm_frequency;
	const enum as5600_slow_filter slow_filter;
	const enum as5600_fast_filter_threshold fast_filter_threshold;
	const enum as5600_watch_dog watch_dog;
};

struct as5600_data {
	uint16_t raw_steps;
	uint16_t filtered_steps;
	uint8_t agc;
	uint16_t magnitude;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_AS5600_AS5600_H_ */
