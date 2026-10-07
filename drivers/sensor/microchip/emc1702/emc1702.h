/*
 * Copyright (c) 2026  Microchip Technology Inc. and its subsidiaries
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_DRIVERS_SENSOR_EMC1702_H_
#define ZEPHYR_DRIVERS_SENSOR_EMC1702_H_

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>

/* Register addresses (from datasheet Table 5.1) */
#define EMC1702_REG_INTERNAL_DIODE_HIGH       0x00
#define EMC1702_REG_INTERNAL_DIODE_LOW        0x29
#define EMC1702_REG_STATUS                    0x02
#define EMC1702_REG_CONFIG                    0x03
#define EMC1702_REG_CONVERSION_RATE           0x04
#define EMC1702_REG_INTERNAL_DIODE_H_LIMIT    0x05
#define EMC1702_REG_INTERNAL_DIODE_L_LIMIT    0x06
#define EMC1702_REG_EXT_DIODE_H_LIMIT_H_BYTE  0x07
#define EMC1702_REG_EXT_DIODE_L_LIMIT_H_BYTE  0x08
#define EMC1702_REG_ONE_SHOT                  0x0F
#define EMC1702_REG_EXT_DIODE_H_LIMIT_L_BYTE  0x13
#define EMC1702_REG_EXT_DIODE_L_LIMIT_L_BYTE  0x14
#define EMC1702_REG_EXT_DIODE_TEMP_CRIT_LIMIT 0x19
#define EMC1702_REG_EXT_DIODE_FAULT           0x1B
#define EMC1702_REG_CHANNEL_MASK              0x1F
#define EMC1702_REG_INT_DIODE_TEMP_CRIT_LIMIT 0x20
#define EMC1702_REG_TEMP_HYST                 0x21
#define EMC1702_REG_CONSECUTIVE_ALERT         0x22
#define EMC1702_REG_BETA_CONFIGURATION        0x25
#define EMC1702_REG_EXT_DIODE_IDEALITY        0x27
#define EMC1702_REG_HIGH_LIMIT_STATUS         0x35
#define EMC1702_REG_LOW_LIMIT_STATUS          0x36
#define EMC1702_REG_CRIT_LIMIT_STATUS         0x37
#define EMC1702_REG_EXTERNAL_DIODE_HIGH       0x3A
#define EMC1702_REG_EXTERNAL_DIODE_LOW        0x3B
#define EMC1702_REG_AVERAGING_CONTROL         0x40

#define EMC1702_REG_V_SAMPLING_CONFIG     0x50
#define EMC1702_REG_CS_SAMPLING_CONFIG    0x51
#define EMC1702_REG_PEAK_DETECTION_CONFIG 0x52
#define EMC1702_REG_SENSE_VOLTAGE_HIGH    0x54
#define EMC1702_REG_SENSE_VOLTAGE_LOW     0x55
#define EMC1702_REG_SOURCE_VOLTAGE_HIGH   0x58
#define EMC1702_REG_SOURCE_VOLTAGE_LOW    0x59
#define EMC1702_REG_POWER_RATIO_HIGH      0x5B
#define EMC1702_REG_POWER_RATIO_LOW       0x5C

#define EMC1702_REG_SENSE_VOLTAGE_HIGH_LIMIT  0x60
#define EMC1702_REG_SENSE_VOLTAGE_LOW_LIMIT   0x61
#define EMC1702_REG_SOURCE_VOLTAGE_HIGH_LIMIT 0x64
#define EMC1702_REG_SOURCE_VOLTAGE_LOW_LIMIT  0x65
#define EMC1702_REG_SENSE_VOLTAGE_CRIT_LIMIT  0x66
#define EMC1702_REG_SOURCE_VOLTAGE_CRIT_LIMIT 0x68
#define EMC1702_REG_SENSE_VOLTAGE_CRIT_HYST   0x69
#define EMC1702_REG_SOURCE_VOLTAGE_CRIT_HYST  0x6A

#define EMC1702_REG_PRODUCT_FEATURE 0xFC
#define EMC1702_REG_PRODUCT_ID      0xFD
#define EMC1702_REG_MANUFACTURER_ID 0xFE
#define EMC1702_REG_REVISION        0xFF

/* Expected ID values (datasheet Tables 5.44 / 5.45) */
#define EMC1702_PRODUCT_ID      0x39
#define EMC1702_MANUFACTURER_ID 0x5D

/* Configuration register bits (datasheet Table 5.5) */
#define EMC1702_CONFIG_MASK_ALL   BIT(7)
#define EMC1702_CONFIG_TMEAS_STOP BIT(6)
#define EMC1702_CONFIG_ALERT_COMP BIT(5)
#define EMC1702_CONFIG_DIS_REC1   BIT(4)
#define EMC1702_CONFIG_IMEAS_STOP BIT(2)
#define EMC1702_CONFIG_DAVG_DIS   BIT(1)

/* High limit status bits */
#define EMC1702_REG_HIGH_LIMIT_STATUS_INT     BIT(0)
#define EMC1702_REG_HIGH_LIMIT_STATUS_EXT     BIT(1)
#define EMC1702_REG_HIGH_LIMIT_STATUS_VSOURCE BIT(6)
#define EMC1702_REG_HIGH_LIMIT_STATUS_VSENSE  BIT(7)

/* Low limit status bits */
#define EMC1702_REG_LOW_LIMIT_STATUS_INT     BIT(0)
#define EMC1702_REG_LOW_LIMIT_STATUS_EXT     BIT(1)
#define EMC1702_REG_LOW_LIMIT_STATUS_VSOURCE BIT(6)
#define EMC1702_REG_LOW_LIMIT_STATUS_VSENSE  BIT(7)

/* Channel Mask register bits (datasheet Table 5.12) */
#define EMC1702_MASK_EXT_CHAN BIT(1)

/*
 * Field widths of the hysteresis registers (datasheet Tables 5.10 and 5.42).
 */
#define EMC1702_TEMP_HYST_MAX      127
#define EMC1702_VOLT_CRIT_HYST_MAX 31

/* Current sense sampling configuration bits */
#define EMC1702_CS_SAMPLING_CONFIG_QUEUE GENMASK(7, 6)
#define EMC1702_CS_SAMPLING_CONFIG_AVG   GENMASK(5, 4)
#define EMC1702_CS_SAMPLING_CONFIG_TIME  GENMASK(3, 2)
#define EMC1702_CS_SAMPLING_CONFIG_RANGE GENMASK(1, 0)

/* Voltage sampling configuration bits */
#define EMC1702_V_SAMPLING_CONFIG_PK_ALERT BIT(7)
#define EMC1702_V_SAMPLING_CONFIG_QUEUE    GENMASK(3, 2)
#define EMC1702_V_SAMPLING_CONFIG_AVG      GENMASK(1, 0)

/* Consecutive Alert bits */
#define EMC1702_CONSEC_ALERT_TIMEOUT BIT(7)
#define EMC1702_CONSEC_ALERT_CTHRM   GENMASK(6, 4)
#define EMC1702_CONSEC_ALERT_CALRT   GENMASK(3, 1)

/*
 * Source-voltage full-scale, in microvolts (datasheet section 4.1.2).
 * FSV = 23.9883 V, applied at register value 0xFFE0 (top 11 bits).
 */
#define EMC1702_FSV_UV 23988300

/*
 * Combined scale for converting a raw VSENSE reading to microamps:
 *   Isense_uA = raw * cs_range_mv * 1000000 / (2^15 * Rsense_mohm)
 * The constant ratio 1000000 / 32768 reduces by 64 to 15625 / 512, keeping
 * the intermediate product small.
 */
#define EMC1702_ISENSE_UA_NUM 15625
#define EMC1702_ISENSE_UA_DEN 512

/* Power-ratio register full-scale count (datasheet Eq. 6 divides by this). */
#define EMC1702_PRATIO_FULL_SCALE 65535

/* Default values */
#define EMC1702_BETA_CONFIGURATION_DEFAULT    0x10
#define EMC1702_EXT_DIODE_IDEALITY_DEFAULT    0x12
#define EMC1702_HIGH_LIMIT_TEMP_VALUE_DEFAULT 0x55 /* 85 degrees C */
#define EMC1702_LOW_LIMIT_TEMP_VALUE_DEFAULT  0x80 /* -128 degrees C */
#define EMC1702_CRIT_LIMIT_TEMP_VALUE_DEFAULT 0x64 /* 100 degrees C */
#define EMC1702_TEMP_CRIT_HYST_DEFAULT        0x0a /* 10 degrees C */
#define EMC1702_HIGH_LIMIT_CURRENT_DEFAULT    0x7f
#define EMC1702_LOW_LIMIT_CURRENT_DEFAULT     0x80
#define EMC1702_CRIT_LIMIT_CURRENT_DEFAULT    0x7f
#define EMC1702_HIGH_LIMIT_VOLTAGE_DEFAULT    0xff
#define EMC1702_CRIT_LIMIT_VOLTAGE_DEFAULT    0xff
#define EMC1702_VOLT_CRIT_HYST_DEFAULT        0x0a

struct emc1702_config {
	struct i2c_dt_spec i2c;
	uint16_t sense_resistor_mohm;
	uint16_t cs_samp_time; /* one of 82, 164, 328 */
	uint16_t conv_rate;    /* 62, 125, 250, 500, 1000, 2000, 4000, 8000 */
	uint8_t cs_range_mv;   /* one of 10, 20, 40, 80 */
	uint8_t volt_queue;    /* one of 1, 2, 3, 4 */
	uint8_t cs_queue;      /* one of 1, 2, 3, 4 */
	uint8_t therm_consec;  /* 1, 2, 3, 4 */
	uint8_t alert_consec;  /* 1, 2, 3, 4 */
	bool rec;
	bool smb_timeout;
	bool external_diode; /* diode wired to DP/DN, external channel usable */
#ifdef CONFIG_EMC1702_TRIGGER
	struct gpio_dt_spec alert_gpio;
#endif
};

struct emc1702_data {
	/* Used to serialise SMBus access and avoid concurrency. */
	struct k_mutex lock;
	int64_t current_max_valid;
	int64_t current_min_valid;
	/* Raw register snapshots taken in sample_fetch. */
	int16_t t_internal_raw; /* signed 11-bit, left-justified to 16 */
	int16_t t_external_raw; /* signed 11-bit, left-justified to 16 */
	int16_t vsense_raw;     /* signed 11-bit, left-justified to 16 */
	uint16_t vsource_raw;   /* unsigned 11-bit, left-justified to 16 */
	uint16_t pratio_raw;    /* 16-bit unsigned */
#ifdef CONFIG_EMC1702_TRIGGER
	const struct device *dev;
	struct gpio_callback alert_cb;
	sensor_trigger_handler_t die_handler;
	const struct sensor_trigger *die_trigger;
	sensor_trigger_handler_t amb_handler;
	const struct sensor_trigger *amb_trigger;
	sensor_trigger_handler_t curr_handler;
	const struct sensor_trigger *curr_trigger;
	sensor_trigger_handler_t volt_handler;
	const struct sensor_trigger *volt_trigger;
	sensor_trigger_handler_t fault_handler;
	const struct sensor_trigger *fault_trigger;
	struct k_work work;
#endif
};

/* --- Register helpers --------------------------------------------------- */

int emc1702_read8(const struct device *dev, uint8_t reg, uint8_t *val);

int emc1702_write8(const struct device *dev, uint8_t reg, uint8_t val);

int emc1702_read16(const struct device *dev, uint8_t reg_hi, uint8_t reg_lo, uint8_t *hi,
		   uint8_t *lo);

#ifdef CONFIG_EMC1702_TRIGGER
int emc1702_trigger_init(const struct device *dev);
int emc1702_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler);
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_EMC1702_H_ */
