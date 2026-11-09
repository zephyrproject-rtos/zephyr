/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ADI_ADT7604_ADT7604_H_
#define ZEPHYR_DRIVERS_SENSOR_ADI_ADT7604_ADT7604_H_

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/rtio/rtio.h>

/* ADT7604 Register Addresses */
#define ADT7604_REG_STATUS              0x0000
#define ADT7604_REG_TEMP_RES_START      0x0010
#define ADT7604_REG_RES_RES_START       0x0060
#define ADT7604_REG_GLOBAL_CONFIG       0x00F0
#define ADT7604_REG_MUX_CONFIG          0x00FF
#define ADT7604_REG_CHAN_ASSIGN_START   0x0200
#define ADT7604_REG_CUST_SENS_TBL_START 0x0250
#define ADT7604_REG_CUST_SENS_TBL_END   0x03CF

/* SPI Commands */
#define ADT7604_SPI_READ_CMD  0x03
#define ADT7604_SPI_WRITE_CMD 0x02

/* Status register bits */
#define ADT7604_STATUS_START         BIT(7)
#define ADT7604_STATUS_DONE_MASK     GENMASK(7, 6)
#define ADT7604_STATUS_CHAN_SEL_MASK GENMASK(4, 0)

/* Sleep mode command (written to address 0x000) */
#define ADT7604_SLEEP_MODE_CMD 0x97

/* Global config bits */
#define ADT7604_NOTCH_FREQ_MASK      GENMASK(1, 0)
#define ADT7604_TEMP_UNIT_FAHRENHEIT BIT(3) /* 0 = °C (default), 1 = °F */

/* Result register bits */
#define ADT7604_RES_HARD_SENSOR_FAULT       BIT(31)
#define ADT7604_RES_HARD_ADC_OUT_OF_RANGE   BIT(30)
#define ADT7604_RES_SOFT_FAULT_MASK         GENMASK(27, 25)
#define ADT7604_RES_SOFT_SENSOR_OVER_RANGE  BIT(27)
#define ADT7604_RES_SOFT_SENSOR_UNDER_RANGE BIT(26)
#define ADT7604_RES_SOFT_ADC_OUT_OF_RANGE   BIT(25)
#define ADT7604_RES_VALID                   BIT(24)

/*
 * Thermocouple-only cold junction fault bits.
 * Checked in addition to the common hard/soft fault bits above when the
 * channel's sensor type is a thermocouple.
 */
#define ADT7604_RES_TC_CJ_HARD_FAULT BIT(29) /* Hard: CJ sensor has a hard fault */
#define ADT7604_RES_TC_CJ_SOFT_FAULT BIT(28) /* Soft: CJ result beyond normal range */

#define ADT7604_DATA_SIGN_BIT  23
#define ADT7604_TEMP_DATA_MASK GENMASK(ADT7604_DATA_SIGN_BIT, 0)

#define ADT7604_RES_DATA_MASK GENMASK(30, 0)

/* Channel assignment bits */
#define ADT7604_CHAN_TYPE_MASK   GENMASK(31, 27)
#define ADT7604_CHAN_ASSIGN_MASK GENMASK(26, 22)

/* RTD sensor configuration (wire config + excitation mode) */
#define ADT7604_RTD_SENSOR_CONFIG_MASK GENMASK(21, 18)

/* RTD excitation current */
#define ADT7604_EXCITATION_CURRENT_MASK GENMASK(17, 14)

/* Copper trace excitation current */
#define ADT7604_CT_EXCITATION_CURRENT_MASK GENMASK(17, 14)

/* Thermistor sensor configuration (SGL + Excitation Mode) */
#define ADT7604_THERMISTOR_SENSOR_CONFIG_MASK GENMASK(21, 19)

/* Thermistor excitation current */
#define ADT7604_THERMISTOR_EXCITATION_CURRENT_MASK GENMASK(18, 15)

/* Custom sensor data offset */
#define ADT7604_CUSTOM_DATA_OFFSET_MASK GENMASK(11, 6)

/* Custom sensor data length */
#define ADT7604_CUSTOM_DATA_LENGTH_MASK GENMASK(5, 0)

/* RTD curve type */
#define ADT7604_RTD_CURVE_TYPE_MASK GENMASK(13, 12)

/* Sensor configuration bits */
#define ADT7604_COPPER_TRACE_SENSOR_CONFIG_MASK GENMASK(21, 18)
#define ADT7604_COPPER_TRACE_SENSOR_CONFIG      0x9

/* Leak sensor configuration */
#define ADT7604_LEAK_SENSOR_CONFIG_MASK GENMASK(21, 19)
#define ADT7604_LEAK_SENSOR_CONFIG      0x1 /* Fixed value: 0b001 */

/* Leak sensor excitation current */
#define ADT7604_LEAK_EXCITATION_CURRENT_MASK GENMASK(18, 15)

/* RSENSE resistor value */
#define ADT7604_RSENSE_VALUE_INT_MASK  GENMASK(26, 10)
#define ADT7604_RSENSE_VALUE_FRAC_MASK GENMASK(9, 0)

#define LTC2984_TC_SENSOR_CONFIG_MASK GENMASK(21, 18)

#define LTC2984_DIODE_SENSOR_CONFIG_MASK      GENMASK(26, 24)
#define LTC2984_DIODE_EXCITATION_CURRENT_MASK GENMASK(23, 22)
#define LTC2984_DIODE_IDEALITY_FACTOR_MASK    GENMASK(21, 0)
#define LTC2984_DIODE_IDEALITY_FACTOR_SHIFT   20 /* 2^-20 resolution (Q2.20) */

/*
 * Full word is 0xF000_0000 for differential readings, 0xF400_0000 for
 * single-ended readings; no other bits are used.
 */
#define LTC2984_DIRECT_ADC_SGL_MASK BIT(26)

/* Helper macros */
#define ADT7604_CHAN_START_ADDR(ch) (((ch)-1) * 4 + ADT7604_REG_CHAN_ASSIGN_START)
#define ADT7604_TEMP_RES_ADDR(ch)   (((ch)-1) * 4 + ADT7604_REG_TEMP_RES_START)
#define ADT7604_RES_RES_ADDR(ch)    (((ch)-1) * 4 + ADT7604_REG_RES_RES_START)

/* Maximum channels */
#define ADT7604_MAX_CHANNELS 20

/**
 * @brief ADT7604 / LTC2984 sensor types
 *
 * Codes 18 and 27 are overloaded between the two devices (same numeric
 * value, different meaning). They must never appear together as separate
 * case labels in the same switch statement; disambiguate using the
 * driver's device_type (is_ltc2984()) first.
 */
enum adt7604_sensor_type {
	ADT7604_SENSOR_TYPE_UNASSIGNED = 0,

	/* LTC2984-only: thermocouple types */
	LTC2984_SENSOR_TYPE_TC_TYPE_J = 1,
	LTC2984_SENSOR_TYPE_TC_TYPE_K = 2,
	LTC2984_SENSOR_TYPE_TC_TYPE_E = 3,
	LTC2984_SENSOR_TYPE_TC_TYPE_N = 4,
	LTC2984_SENSOR_TYPE_TC_TYPE_R = 5,
	LTC2984_SENSOR_TYPE_TC_TYPE_S = 6,
	LTC2984_SENSOR_TYPE_TC_TYPE_T = 7,
	LTC2984_SENSOR_TYPE_TC_TYPE_B = 8,
	LTC2984_SENSOR_TYPE_TC_CUSTOM = 9,

	ADT7604_SENSOR_TYPE_RTD_PT_10 = 10,
	ADT7604_SENSOR_TYPE_RTD_PT_50 = 11,
	ADT7604_SENSOR_TYPE_RTD_PT_100 = 12,
	ADT7604_SENSOR_TYPE_RTD_PT_200 = 13,
	ADT7604_SENSOR_TYPE_RTD_PT_500 = 14,
	ADT7604_SENSOR_TYPE_RTD_PT_1000 = 15,
	ADT7604_SENSOR_TYPE_RTD_1000 = 16,
	ADT7604_SENSOR_TYPE_RTD_NI_120 = 17,

	/* Code 18: Copper Trace (ADT7604) vs RTD Custom (LTC2984) */
	ADT7604_SENSOR_TYPE_COPPER_TRACE = 18,
	LTC2984_SENSOR_TYPE_RTD_CUSTOM = 18,

	ADT7604_SENSOR_TYPE_THERMISTOR_44004 = 19,
	ADT7604_SENSOR_TYPE_THERMISTOR_44005 = 20,
	ADT7604_SENSOR_TYPE_THERMISTOR_44007 = 21,
	ADT7604_SENSOR_TYPE_THERMISTOR_44006 = 22,
	ADT7604_SENSOR_TYPE_THERMISTOR_44008 = 23,
	ADT7604_SENSOR_TYPE_THERMISTOR_YSI_400 = 24,
	ADT7604_SENSOR_TYPE_THERMISTOR_SPECTRUM = 25,
	ADT7604_SENSOR_TYPE_THERMISTOR_CUSTOM = 26,

	/* Code 27: Leak Detector (ADT7604) vs Thermistor Custom Table (LTC2984) */
	ADT7604_SENSOR_TYPE_LEAK_DETECTOR = 27,
	LTC2984_SENSOR_TYPE_THERMISTOR_CUSTOM_TABLE = 27,

	/* LTC2984-only */
	LTC2984_SENSOR_TYPE_DIODE = 28,

	ADT7604_SENSOR_TYPE_RSENSE = 29,

	/* LTC2984-only */
	LTC2984_SENSOR_TYPE_DIRECT_ADC = 30,
};

/**
 * @brief ADT7604 RTD curve type
 */
#define ADT7604_RTD_CURVE_EUROPEAN_CURVE 0
#define ADT7604_RTD_CURVE_AMERICAN       1
#define ADT7604_RTD_CURVE_JAPANESE       2
#define ADT7604_RTD_CURVE_ITS_90         3
#define ADT7604_RTD_CURVE_RTD1000_375    4
#define ADT7604_RTD_CURVE_NI_120         5

/**
 * @brief Notch frequency for digital filter
 */
enum adt7604_filter_notch {
	ADT7604_NOTCH_50_60_HZ = 0,
	ADT7604_NOTCH_60_HZ = 1,
	ADT7604_NOTCH_50_HZ = 2,
};

/**
 * @brief Board/device type backed by this driver instance
 *
 * The driver source is shared between the ADI ADT7604 and LTC2984, which are
 * register-compatible 20-channel temperature measurement systems. The device
 * type is recorded at instantiation time (from the devicetree compatible
 * string) so runtime behavior can be adjusted per device where needed.
 */
enum adt7604_device_type {
	ADT7604_DEVICE_TYPE_ADT7604 = 0,
	ADT7604_DEVICE_TYPE_LTC2984 = 1,
};

/**
 * @brief Channel configuration from devicetree
 */
struct adt7604_channel_config {
	uint8_t chan_num;
	uint8_t sensor_type;
	uint8_t rsense_chan;
	uint32_t rsense_int;
	uint16_t rsense_frac;
	bool rsense_mohm;
	bool sub_ohm;
	uint8_t sensor_config;
	uint8_t excitation_current;
	uint8_t rtd_curve;
	uint32_t diode_ideality_factor_raw;
	bool direct_adc_single_ended;
};

/**
 * @brief ADT7604 device configuration (from devicetree)
 */
struct adt7604_config {
	struct gpio_dt_spec reset_gpio;
	struct gpio_dt_spec int_gpio;
	uint32_t mux_delay_us;
	uint8_t filter_notch;
	uint8_t num_channels;
	const struct adt7604_channel_config *channels;
	enum adt7604_device_type device_type;
};

/**
 * @brief ADT7604 runtime data
 */
struct adt7604_data {
	struct rtio_iodev *iodev;
	struct rtio *ctx;
	struct gpio_callback int_cb;
	struct k_sem gpio_sem;
	int32_t *sensor_data;
	bool channels_ready;
	bool temp_unit_celsius; /* true = °C, false = °F */
};

/**
 * @brief Buffer written by adt7604_submit and read by the decoder.
 */
struct adt7604_encoded_data {
	uint64_t timestamp_ns;
	uint8_t num_channels;
	uint8_t temp_unit_celsius; /* 1 = °C, 0 = °F (bit 3 of global config) */
	/* Needed by the decoder (no access to struct device) to disambiguate
	 * sensor type codes overloaded between ADT7604 and LTC2984 (18, 27).
	 */
	enum adt7604_device_type device_type;
	struct {
		uint8_t chan_num;
		uint8_t sensor_type;
		int32_t raw;
	} readings[ADT7604_MAX_CHANNELS];
} __attribute__((__packed__));

/**
 * @brief Sensor measurement type classification
 */
enum adt7604_measurement_type {
	ADT7604_MEASUREMENT_NONE,        /**< Not a measurement sensor (e.g., RSENSE) */
	ADT7604_MEASUREMENT_TEMPERATURE, /**< Temperature measurement */
	ADT7604_MEASUREMENT_RESISTANCE,  /**< Resistance measurement */
	ADT7604_MEASUREMENT_VOLTAGE,     /**< Voltage measurement (LTC2984 Direct ADC) */
};

/**
 * @brief Get the measurement type for a sensor
 *
 * @param sensor_type ADT7604 sensor type
 * @param device_type Device instance type, used to disambiguate sensor type
 *                    codes overloaded between ADT7604 and LTC2984 (18, 27)
 * @return Measurement type classification
 */
static inline enum adt7604_measurement_type
adt7604_get_measurement_type(uint8_t sensor_type, enum adt7604_device_type device_type)
{
	switch (sensor_type) {
	/* RTD temperature sensors */
	case ADT7604_SENSOR_TYPE_RTD_PT_10:
	case ADT7604_SENSOR_TYPE_RTD_PT_50:
	case ADT7604_SENSOR_TYPE_RTD_PT_100:
	case ADT7604_SENSOR_TYPE_RTD_PT_200:
	case ADT7604_SENSOR_TYPE_RTD_PT_500:
	case ADT7604_SENSOR_TYPE_RTD_PT_1000:
	case ADT7604_SENSOR_TYPE_RTD_1000:
	case ADT7604_SENSOR_TYPE_RTD_NI_120:
		return ADT7604_MEASUREMENT_TEMPERATURE;

	/* Thermistor temperature sensors */
	case ADT7604_SENSOR_TYPE_THERMISTOR_44004:
	case ADT7604_SENSOR_TYPE_THERMISTOR_44005:
	case ADT7604_SENSOR_TYPE_THERMISTOR_44007:
	case ADT7604_SENSOR_TYPE_THERMISTOR_44006:
	case ADT7604_SENSOR_TYPE_THERMISTOR_44008:
	case ADT7604_SENSOR_TYPE_THERMISTOR_YSI_400:
	case ADT7604_SENSOR_TYPE_THERMISTOR_SPECTRUM:
	case ADT7604_SENSOR_TYPE_THERMISTOR_CUSTOM:
		return ADT7604_MEASUREMENT_TEMPERATURE;

	/* Code 18: Copper Trace (ADT7604, resistance) vs RTD Custom (LTC2984,
	 * reuses the RTD channel layout and TEMP_RES register, temperature)
	 */
	case ADT7604_SENSOR_TYPE_COPPER_TRACE:
		return (device_type == ADT7604_DEVICE_TYPE_LTC2984)
			       ? ADT7604_MEASUREMENT_TEMPERATURE
			       : ADT7604_MEASUREMENT_RESISTANCE;

	/* Code 27: Leak Detector (ADT7604, resistance) vs Thermistor Custom
	 * Table (LTC2984, reuses the thermistor channel layout and TEMP_RES
	 * register, temperature)
	 */
	case ADT7604_SENSOR_TYPE_LEAK_DETECTOR:
		return (device_type == ADT7604_DEVICE_TYPE_LTC2984)
			       ? ADT7604_MEASUREMENT_TEMPERATURE
			       : ADT7604_MEASUREMENT_RESISTANCE;

	/* LTC2984 thermocouple types: read via TEMP_RES register */
	case LTC2984_SENSOR_TYPE_TC_TYPE_J:
	case LTC2984_SENSOR_TYPE_TC_TYPE_K:
	case LTC2984_SENSOR_TYPE_TC_TYPE_E:
	case LTC2984_SENSOR_TYPE_TC_TYPE_N:
	case LTC2984_SENSOR_TYPE_TC_TYPE_R:
	case LTC2984_SENSOR_TYPE_TC_TYPE_S:
	case LTC2984_SENSOR_TYPE_TC_TYPE_T:
	case LTC2984_SENSOR_TYPE_TC_TYPE_B:
	case LTC2984_SENSOR_TYPE_TC_CUSTOM:
		return ADT7604_MEASUREMENT_TEMPERATURE;

	/* LTC2984 diode: read via TEMP_RES register */
	case LTC2984_SENSOR_TYPE_DIODE:
		return ADT7604_MEASUREMENT_TEMPERATURE;

	/* LTC2984 Direct ADC: read via TEMP_RES register, but reports a
	 * voltage rather than a temperature
	 */
	case LTC2984_SENSOR_TYPE_DIRECT_ADC:
		return ADT7604_MEASUREMENT_VOLTAGE;

	/* Reference/calibration - not measured */
	case ADT7604_SENSOR_TYPE_RSENSE:
	case ADT7604_SENSOR_TYPE_UNASSIGNED:
	default:
		return ADT7604_MEASUREMENT_NONE;
	}
}

/* Internal helpers exposed to adt7604_rtio.c */
int adt7604_start_conversion(const struct device *dev, uint8_t chan);
int adt7604_wait_status_done(const struct device *dev);
int adt7604_read_resistance_result(const struct device *dev, uint8_t chan, int32_t *result);
int adt7604_read_temperature_result(const struct device *dev, uint8_t chan, int32_t *result);
int adt7604_read_voltage_result(const struct device *dev, uint8_t chan, int32_t *result);

void adt7604_submit(const struct device *sensor, struct rtio_iodev_sqe *iodev_sqe);
int adt7604_get_decoder(const struct device *sensor, const struct sensor_decoder_api **api);

#endif /* ZEPHYR_DRIVERS_SENSOR_ADI_ADT7604_ADT7604_H_ */
