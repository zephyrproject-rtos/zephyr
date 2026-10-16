/*
 * Copyright (c) 2026 Analog Devices Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_MAX40109_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_MAX40109_H_

#include <zephyr/drivers/sensor.h>
#include <zephyr/dt-bindings/sensor/max40109.h>
#include <zephyr/sys/util.h>

/**
 * @file
 * @brief Extended public API for the Analog Devices MAX40109 pressure sensor AFE.
 *
 * Defines the MTP address map, the MAX40109-specific sensor channels,
 * attributes and trigger types, and the MTP access helpers.
 * @ingroup max40109_interface
 */

/**
 * @name MTP calibration coefficient words
 *
 * Storage for the 20 calibration coefficients. Each coefficient is a 32-bit
 * float occupying two consecutive words, most significant word first.
 * @{
 */
#define MAX40109_MTP_REG_CAL_DATA0  0x00u /**< Coefficient K0, most significant word. */
#define MAX40109_MTP_REG_CAL_DATA1  0x01u /**< Coefficient K0, least significant word. */
#define MAX40109_MTP_REG_CAL_DATA2  0x02u /**< Coefficient K1, most significant word. */
#define MAX40109_MTP_REG_CAL_DATA3  0x03u /**< Coefficient K1, least significant word. */
#define MAX40109_MTP_REG_CAL_DATA4  0x04u /**< Coefficient K2, most significant word. */
#define MAX40109_MTP_REG_CAL_DATA5  0x05u /**< Coefficient K2, least significant word. */
#define MAX40109_MTP_REG_CAL_DATA6  0x06u /**< Coefficient K3, most significant word. */
#define MAX40109_MTP_REG_CAL_DATA7  0x07u /**< Coefficient K3, least significant word. */
#define MAX40109_MTP_REG_CAL_DATA8  0x08u /**< Coefficient H0, most significant word. */
#define MAX40109_MTP_REG_CAL_DATA9  0x09u /**< Coefficient H0, least significant word. */
#define MAX40109_MTP_REG_CAL_DATA10 0x0au /**< Coefficient H1, most significant word. */
#define MAX40109_MTP_REG_CAL_DATA11 0x0bu /**< Coefficient H1, least significant word. */
#define MAX40109_MTP_REG_CAL_DATA12 0x0cu /**< Coefficient H2, most significant word. */
#define MAX40109_MTP_REG_CAL_DATA13 0x0du /**< Coefficient H2, least significant word. */
#define MAX40109_MTP_REG_CAL_DATA14 0x0eu /**< Coefficient H3, most significant word. */
#define MAX40109_MTP_REG_CAL_DATA15 0x0fu /**< Coefficient H3, least significant word. */
#define MAX40109_MTP_REG_CAL_DATA16 0x10u /**< Coefficient G0, most significant word. */
#define MAX40109_MTP_REG_CAL_DATA17 0x11u /**< Coefficient G0, least significant word. */
#define MAX40109_MTP_REG_CAL_DATA18 0x12u /**< Coefficient G1, most significant word. */
#define MAX40109_MTP_REG_CAL_DATA19 0x13u /**< Coefficient G1, least significant word. */
#define MAX40109_MTP_REG_CAL_DATA20 0x14u /**< Coefficient G2, most significant word. */
#define MAX40109_MTP_REG_CAL_DATA21 0x15u /**< Coefficient G2, least significant word. */
#define MAX40109_MTP_REG_CAL_DATA22 0x16u /**< Coefficient G3, most significant word. */
#define MAX40109_MTP_REG_CAL_DATA23 0x17u /**< Coefficient G3, least significant word. */
#define MAX40109_MTP_REG_CAL_DATA24 0x18u /**< Coefficient N0, most significant word. */
#define MAX40109_MTP_REG_CAL_DATA25 0x19u /**< Coefficient N0, least significant word. */
#define MAX40109_MTP_REG_CAL_DATA26 0x1au /**< Coefficient N1, most significant word. */
#define MAX40109_MTP_REG_CAL_DATA27 0x1bu /**< Coefficient N1, least significant word. */
#define MAX40109_MTP_REG_CAL_DATA28 0x1cu /**< Coefficient N2, most significant word. */
#define MAX40109_MTP_REG_CAL_DATA29 0x1du /**< Coefficient N2, least significant word. */
#define MAX40109_MTP_REG_CAL_DATA30 0x1eu /**< Coefficient N3, most significant word. */
#define MAX40109_MTP_REG_CAL_DATA31 0x1fu /**< Coefficient N3, least significant word. */
#define MAX40109_MTP_REG_CAL_DATA32 0x20u /**< Coefficient M0, most significant word. */
#define MAX40109_MTP_REG_CAL_DATA33 0x21u /**< Coefficient M0, least significant word. */
#define MAX40109_MTP_REG_CAL_DATA34 0x22u /**< Coefficient M1, most significant word. */
#define MAX40109_MTP_REG_CAL_DATA35 0x23u /**< Coefficient M1, least significant word. */
#define MAX40109_MTP_REG_CAL_DATA36 0x24u /**< Coefficient M2, most significant word. */
#define MAX40109_MTP_REG_CAL_DATA37 0x25u /**< Coefficient M2, least significant word. */
#define MAX40109_MTP_REG_CAL_DATA38 0x26u /**< Coefficient M3, most significant word. */
#define MAX40109_MTP_REG_CAL_DATA39 0x27u /**< Coefficient M3, least significant word. */
/** @} */

/**
 * @name MTP spare words
 *
 * General-purpose MTP storage. These words are not touched by the driver and
 * are reachable through max40109_mtp_read() and max40109_mtp_write().
 * @{
 */
#define MAX40109_MTP_REG_SP_DATA0  0x28u /**< Spare word 0. */
#define MAX40109_MTP_REG_SP_DATA1  0x29u /**< Spare word 1. */
#define MAX40109_MTP_REG_SP_DATA2  0x2au /**< Spare word 2. */
#define MAX40109_MTP_REG_SP_DATA3  0x2bu /**< Spare word 3. */
#define MAX40109_MTP_REG_SP_DATA4  0x2cu /**< Spare word 4. */
#define MAX40109_MTP_REG_SP_DATA5  0x2du /**< Spare word 5. */
#define MAX40109_MTP_REG_SP_DATA6  0x2eu /**< Spare word 6. */
#define MAX40109_MTP_REG_SP_DATA7  0x2fu /**< Spare word 7. */
#define MAX40109_MTP_REG_SP_DATA8  0x30u /**< Spare word 8. */
#define MAX40109_MTP_REG_SP_DATA9  0x31u /**< Spare word 9. */
#define MAX40109_MTP_REG_SP_DATA10 0x32u /**< Spare word 10. */
#define MAX40109_MTP_REG_SP_DATA11 0x33u /**< Spare word 11. */
#define MAX40109_MTP_REG_SP_DATA12 0x34u /**< Spare word 12. */
#define MAX40109_MTP_REG_SP_DATA13 0x35u /**< Spare word 13. */
#define MAX40109_MTP_REG_SP_DATA14 0x36u /**< Spare word 14. */
#define MAX40109_MTP_REG_SP_DATA15 0x37u /**< Spare word 15. */
/** @} */

/**
 * @name MTP diagnostic threshold words
 *
 * Each word packs an over-range threshold in the upper byte and an
 * under-range threshold in the lower byte.
 * @{
 */
#define MAX40109_MTP_REG_DIAG_DATA0 0x38u /**< Positive pressure input thresholds. */
#define MAX40109_MTP_REG_DIAG_DATA1 0x39u /**< Negative pressure input thresholds. */
#define MAX40109_MTP_REG_DIAG_DATA2 0x3au /**< Temperature input voltage thresholds. */
#define MAX40109_MTP_REG_DIAG_DATA3 0x3bu /**< DRV pin voltage thresholds. */
/** @} */

/**
 * @name MTP configuration words
 * @{
 */
#define MAX40109_MTP_REG_PRESSURE_THRESHOLD   0x3cu /**< Primary and hysteresis thresholds. */
#define MAX40109_MTP_REG_ZERO_PRESSURE_OFFSET 0x3du /**< Zero-pressure offset. */
#define MAX40109_MTP_REG_TEMP_OFFSET          0x3eu /**< Temperature offset. */
#define MAX40109_MTP_REG_CLIPPING_THRESHOLD   0x3fu /**< Output clipping thresholds. */
#define MAX40109_MTP_REG_DIAGNOSTIC_RANGE     0x40u /**< Diagnostic range selection. */
#define MAX40109_MTP_REG_FUNC_CONFIGURATION   0x41u /**< Functional configuration. */
#define MAX40109_MTP_REG_PGA_TEMP_PRES_GAIN   0x42u /**< Temperature and pressure PGA gains. */
#define MAX40109_MTP_REG_MISCELLANEOUS        0x43u /**< Miscellaneous settings. */
#define MAX40109_MTP_REG_CONFIG               0x44u /**< Device configuration. */
/** @} */

/**
 * @name Fields of MAX40109_MTP_REG_DIAG_DATA0
 * @{
 */
/** Positive over-pressure threshold. */
#define MAX40109_OVER_PRESSURE_POSITIVE_MASK  GENMASK(15, 8)
/** Positive under-pressure threshold. */
#define MAX40109_UNDER_PRESSURE_POSITIVE_MASK GENMASK(7, 0)
/** @} */

/**
 * @name Fields of MAX40109_MTP_REG_DIAG_DATA1
 * @{
 */
/** Negative over-pressure threshold. */
#define MAX40109_OVER_PRESSURE_NEGATIVE_MASK  GENMASK(15, 8)
/** Negative under-pressure threshold. */
#define MAX40109_UNDER_PRESSURE_NEGATIVE_MASK GENMASK(7, 0)
/** @} */

/**
 * @name Fields of MAX40109_MTP_REG_DIAG_DATA2
 * @{
 */
/** Temperature input over-voltage threshold. */
#define MAX40109_OVER_TEMPERATURE_VOLTAGE_MASK  GENMASK(15, 8)
/** Temperature input under-voltage threshold. */
#define MAX40109_UNDER_TEMPERATURE_VOLTAGE_MASK GENMASK(7, 0)
/** @} */

/**
 * @name Fields of MAX40109_MTP_REG_DIAG_DATA3
 * @{
 */
/** DRV over-voltage threshold. */
#define MAX40109_OVER_VOLTAGE_DRIVE_MASK  GENMASK(15, 8)
/** DRV under-voltage threshold. */
#define MAX40109_UNDER_VOLTAGE_DRIVE_MASK GENMASK(7, 0)
/** @} */

/**
 * @name Fields of MAX40109_MTP_REG_PRESSURE_THRESHOLD
 * @{
 */
/** Primary pressure threshold. */
#define MAX40109_PRIMARY_THRESHOLD_PRESSURE_VALUE_MASK    GENMASK(15, 8)
/** Hysteresis applied to the primary pressure threshold. */
#define MAX40109_HYSTERESIS_THRESHOLD_PRESSURE_VALUE_MASK GENMASK(7, 0)
/** @} */

/**
 * @brief MAX40109-specific sensor attributes
 *
 * These attributes extend the standard sensor_attribute enum. Unless noted,
 * they are set and read with SENSOR_CHAN_ALL.
 */
enum max40109_sensor_attr {
	/** Analog filter bandwidth, see @ref max40109_analog_filter_bw_settings. */
	SENSOR_ATTR_MAX40109_ANALOG_FILTER_BW_SETUP = SENSOR_ATTR_PRIV_START,
	/** ALERT pin function, see @ref max40109_alert_mode. */
	SENSOR_ATTR_MAX40109_ALERT_MODE,
	/** Analog output stage mode, see @ref max40109_aos_mode. */
	SENSOR_ATTR_MAX40109_ANALOG_OUTPUT_STAGE,
	/** Digital averaging depth, see @ref max40109_digital_filter_setup. */
	SENSOR_ATTR_MAX40109_DIGITAL_FILTER_SETUP,
	/** Temperature measurement mode, see @ref max40109_temp_mode. */
	SENSOR_ATTR_MAX40109_TEMP_MODE,
	/** DRV input scale, see @ref max40109_drv_scale. */
	SENSOR_ATTR_MAX40109_DRV_SCALE,
	/** Positive over-pressure diagnostic threshold. */
	SENSOR_ATTR_MAX40109_OVER_PRESSURE_THRESHOLD_POSITIVE,
	/** Positive under-pressure diagnostic threshold. */
	SENSOR_ATTR_MAX40109_UNDER_PRESSURE_THRESHOLD_POSITIVE,
	/** Negative over-pressure diagnostic threshold. */
	SENSOR_ATTR_MAX40109_OVER_PRESSURE_THRESHOLD_NEGATIVE,
	/** Negative under-pressure diagnostic threshold. */
	SENSOR_ATTR_MAX40109_UNDER_PRESSURE_THRESHOLD_NEGATIVE,
	/** Temperature input over-voltage diagnostic threshold. */
	SENSOR_ATTR_MAX40109_OVER_TEMPERATURE_VOLTAGE_THRESHOLD,
	/** Temperature input under-voltage diagnostic threshold. */
	SENSOR_ATTR_MAX40109_UNDER_TEMPERATURE_VOLTAGE_THRESHOLD,
	/** DRV over-voltage diagnostic threshold. */
	SENSOR_ATTR_MAX40109_OVER_VOLTAGE_DRIVE_THRESHOLD,
	/** DRV under-voltage diagnostic threshold. */
	SENSOR_ATTR_MAX40109_UNDER_VOLTAGE_DRIVE_THRESHOLD,
	/** Primary pressure threshold used by the ALERT comparator. */
	SENSOR_ATTR_MAX40109_PRIMARY_THRESHOLD_PRESSURE_VALUE,
	/** Hysteresis applied to the primary pressure threshold. */
	SENSOR_ATTR_MAX40109_HYSTERESIS_THRESHOLD_PRESSURE_VALUE,
};

/**
 * @brief MAX40109-specific sensor channels
 *
 * These channels extend the standard sensor_channel enum to expose the raw
 * ADC codes and the MTP calibration coefficients. The coefficient channels
 * are used with SENSOR_ATTR_CALIB_TARGET and require
 * @kconfig{CONFIG_MAX40109_CALIBRATION}.
 */
enum max40109_sensor_channel {
	/** Ambient temperature calibration coefficient K0. */
	SENSOR_CHAN_MAX40109_AMBIENT_TEMP_CALIB_COEFF_K0 = SENSOR_CHAN_PRIV_START,
	/** Ambient temperature calibration coefficient K1. */
	SENSOR_CHAN_MAX40109_AMBIENT_TEMP_CALIB_COEFF_K1,
	/** Ambient temperature calibration coefficient K2. */
	SENSOR_CHAN_MAX40109_AMBIENT_TEMP_CALIB_COEFF_K2,
	/** Ambient temperature calibration coefficient K3. */
	SENSOR_CHAN_MAX40109_AMBIENT_TEMP_CALIB_COEFF_K3,
	/** Pressure calibration coefficient H0. */
	SENSOR_CHAN_MAX40109_PRESSURE_CALIB_COEFF_H0,
	/** Pressure calibration coefficient H1. */
	SENSOR_CHAN_MAX40109_PRESSURE_CALIB_COEFF_H1,
	/** Pressure calibration coefficient H2. */
	SENSOR_CHAN_MAX40109_PRESSURE_CALIB_COEFF_H2,
	/** Pressure calibration coefficient H3. */
	SENSOR_CHAN_MAX40109_PRESSURE_CALIB_COEFF_H3,
	/** Pressure calibration coefficient G0. */
	SENSOR_CHAN_MAX40109_PRESSURE_CALIB_COEFF_G0,
	/** Pressure calibration coefficient G1. */
	SENSOR_CHAN_MAX40109_PRESSURE_CALIB_COEFF_G1,
	/** Pressure calibration coefficient G2. */
	SENSOR_CHAN_MAX40109_PRESSURE_CALIB_COEFF_G2,
	/** Pressure calibration coefficient G3. */
	SENSOR_CHAN_MAX40109_PRESSURE_CALIB_COEFF_G3,
	/** Pressure calibration coefficient N0. */
	SENSOR_CHAN_MAX40109_PRESSURE_CALIB_COEFF_N0,
	/** Pressure calibration coefficient N1. */
	SENSOR_CHAN_MAX40109_PRESSURE_CALIB_COEFF_N1,
	/** Pressure calibration coefficient N2. */
	SENSOR_CHAN_MAX40109_PRESSURE_CALIB_COEFF_N2,
	/** Pressure calibration coefficient N3. */
	SENSOR_CHAN_MAX40109_PRESSURE_CALIB_COEFF_N3,
	/** Pressure calibration coefficient M0. */
	SENSOR_CHAN_MAX40109_PRESSURE_CALIB_COEFF_M0,
	/** Pressure calibration coefficient M1. */
	SENSOR_CHAN_MAX40109_PRESSURE_CALIB_COEFF_M1,
	/** Pressure calibration coefficient M2. */
	SENSOR_CHAN_MAX40109_PRESSURE_CALIB_COEFF_M2,
	/** Pressure calibration coefficient M3. */
	SENSOR_CHAN_MAX40109_PRESSURE_CALIB_COEFF_M3,
	/** Uncompensated ambient temperature ADC code. */
	SENSOR_CHAN_MAX40109_AMBIENT_TEMP_RAW,
	/** Uncompensated pressure ADC code. */
	SENSOR_CHAN_MAX40109_PRESS_RAW,
};

/**
 * @brief MAX40109-specific sensor trigger types
 *
 * These triggers extend the standard sensor_trigger_type enum and are
 * reported through the ALERT pin. They require
 * @kconfig{CONFIG_MAX40109_TRIGGER}.
 */
enum max40109_sensor_trigger_type {
	/** New pressure conversion available. */
	SENSOR_TRIG_MAX40109_PRESSURE_DATA_READY = SENSOR_TRIG_PRIV_START,
	/** New temperature conversion available. */
	SENSOR_TRIG_MAX40109_TEMP_DATA_READY,
	/** DRV pin under-voltage. */
	SENSOR_TRIG_MAX40109_DRIVE_VOLTAGE_UV,
	/** DRV pin over-voltage. */
	SENSOR_TRIG_MAX40109_DRIVE_VOLTAGE_OV,
	/** Temperature input under-voltage. */
	SENSOR_TRIG_MAX40109_INPUT_TEMPERATURE_VOLTAGE_UV,
	/** Temperature input over-voltage. */
	SENSOR_TRIG_MAX40109_INPUT_TEMPERATURE_VOLTAGE_OV,
	/** Negative pressure input under-range. */
	SENSOR_TRIG_MAX40109_INPUT_PRESSURE_NEGATIVE_UV,
	/** Negative pressure input over-range. */
	SENSOR_TRIG_MAX40109_INPUT_PRESSURE_NEGATIVE_OV,
	/** Positive pressure input under-range. */
	SENSOR_TRIG_MAX40109_INPUT_PRESSURE_POSITIVE_UV,
	/** Positive pressure input over-range. */
	SENSOR_TRIG_MAX40109_INPUT_PRESSURE_POSITIVE_OV,
};

/**
 * @brief ADC sample rate
 *
 * Runtime values for SENSOR_ATTR_SAMPLING_FREQUENCY. Each code selects a
 * pressure rate and a temperature rate together.
 */
enum sensor_sampling_rate_max40109 {
	/** Pressure 1 kSPS, temperature 1 SPS. */
	SENSOR_SAMPLING_RATE_MAX40109_1000HZ_1HZ = MAX40109_DT_SAMPLE_RATE_1KSPS_1SPS,
	/** Pressure 1 kSPS, temperature 10 SPS. */
	SENSOR_SAMPLING_RATE_MAX40109_1000HZ_10HZ = MAX40109_DT_SAMPLE_RATE_1KSPS_10SPS,
	/** Pressure 2 kSPS, temperature 1 SPS. */
	SENSOR_SAMPLING_RATE_MAX40109_2000HZ_1HZ = MAX40109_DT_SAMPLE_RATE_2KSPS_1SPS,
	/** Pressure 2 kSPS, temperature 10 SPS. */
	SENSOR_SAMPLING_RATE_MAX40109_2000HZ_10HZ = MAX40109_DT_SAMPLE_RATE_2KSPS_10SPS,
	/** Pressure 4 kSPS, temperature 1 SPS. */
	SENSOR_SAMPLING_RATE_MAX40109_4000HZ_1HZ = MAX40109_DT_SAMPLE_RATE_4KSPS_1SPS,
	/** Pressure 4 kSPS, temperature 10 SPS. */
	SENSOR_SAMPLING_RATE_MAX40109_4000HZ_10HZ = MAX40109_DT_SAMPLE_RATE_4KSPS_10SPS,
	/** Pressure 8 kSPS, temperature 1 SPS. */
	SENSOR_SAMPLING_RATE_MAX40109_8000HZ_1HZ = MAX40109_DT_SAMPLE_RATE_8KSPS_1SPS,
	/** Pressure 8 kSPS, temperature 10 SPS. */
	SENSOR_SAMPLING_RATE_MAX40109_8000HZ_10HZ = MAX40109_DT_SAMPLE_RATE_8KSPS_10SPS,
	/** Pressure 16 kSPS, temperature 1 SPS. */
	SENSOR_SAMPLING_RATE_MAX40109_16000HZ_1HZ = MAX40109_DT_SAMPLE_RATE_16KSPS_1SPS,
	/** Pressure 16 kSPS, temperature 10 SPS. */
	SENSOR_SAMPLING_RATE_MAX40109_16000HZ_10HZ = MAX40109_DT_SAMPLE_RATE_16KSPS_10SPS,
};

/**
 * @brief Pressure PGA gain
 *
 * Runtime values for the pressure signal path gain.
 */
enum sensor_max40109_pga_pressure_gain {
	/** Gain = 5. */
	SENSOR_MAX40109_PGA_PRESSURE_GAIN_5 = MAX40109_DT_PGA_PRESSURE_GAIN_5,
	/** Gain = 10. */
	SENSOR_MAX40109_PGA_PRESSURE_GAIN_10 = MAX40109_DT_PGA_PRESSURE_GAIN_10,
	/** Gain = 15. */
	SENSOR_MAX40109_PGA_PRESSURE_GAIN_15 = MAX40109_DT_PGA_PRESSURE_GAIN_15,
	/** Gain = 20. */
	SENSOR_MAX40109_PGA_PRESSURE_GAIN_20 = MAX40109_DT_PGA_PRESSURE_GAIN_20,
	/** Gain = 24. */
	SENSOR_MAX40109_PGA_PRESSURE_GAIN_24 = MAX40109_DT_PGA_PRESSURE_GAIN_24,
	/** Gain = 40. */
	SENSOR_MAX40109_PGA_PRESSURE_GAIN_40 = MAX40109_DT_PGA_PRESSURE_GAIN_40,
	/** Gain = 60. */
	SENSOR_MAX40109_PGA_PRESSURE_GAIN_60 = MAX40109_DT_PGA_PRESSURE_GAIN_60,
	/** Gain = 72. */
	SENSOR_MAX40109_PGA_PRESSURE_GAIN_72 = MAX40109_DT_PGA_PRESSURE_GAIN_72,
	/** Gain = 90. */
	SENSOR_MAX40109_PGA_PRESSURE_GAIN_90 = MAX40109_DT_PGA_PRESSURE_GAIN_90,
	/** Gain = 108. */
	SENSOR_MAX40109_PGA_PRESSURE_GAIN_108 = MAX40109_DT_PGA_PRESSURE_GAIN_108,
	/** Gain = 126. */
	SENSOR_MAX40109_PGA_PRESSURE_GAIN_126 = MAX40109_DT_PGA_PRESSURE_GAIN_126,
	/** Gain = 144. */
	SENSOR_MAX40109_PGA_PRESSURE_GAIN_144 = MAX40109_DT_PGA_PRESSURE_GAIN_144,
	/** Gain = 160. */
	SENSOR_MAX40109_PGA_PRESSURE_GAIN_160 = MAX40109_DT_PGA_PRESSURE_GAIN_160,
	/** Gain = 180. */
	SENSOR_MAX40109_PGA_PRESSURE_GAIN_180 = MAX40109_DT_PGA_PRESSURE_GAIN_180,
	/** Gain = 200. */
	SENSOR_MAX40109_PGA_PRESSURE_GAIN_200 = MAX40109_DT_PGA_PRESSURE_GAIN_200,
	/** Gain = 252. */
	SENSOR_MAX40109_PGA_PRESSURE_GAIN_252 = MAX40109_DT_PGA_PRESSURE_GAIN_252,
	/** Gain = 540. */
	SENSOR_MAX40109_PGA_PRESSURE_GAIN_540 = MAX40109_DT_PGA_PRESSURE_GAIN_540,
	/** Gain = 1080. */
	SENSOR_MAX40109_PGA_PRESSURE_GAIN_1080 = MAX40109_DT_PGA_PRESSURE_GAIN_1080,
	/** Gain = 1440. */
	SENSOR_MAX40109_PGA_PRESSURE_GAIN_1440 = MAX40109_DT_PGA_PRESSURE_GAIN_1440,
	/** Gain = 2520. */
	SENSOR_MAX40109_PGA_PRESSURE_GAIN_2520 = MAX40109_DT_PGA_PRESSURE_GAIN_2520,
};

/**
 * @brief Analog output stage (AOS) mode
 *
 * Runtime values for SENSOR_ATTR_MAX40109_ANALOG_OUTPUT_STAGE. Each code
 * selects the function, reference and full scale of the analog output.
 */
enum max40109_aos_mode {
	/** Output stage shut down. */
	MAX40109_AOS_SHUTDOWN = MAX40109_DT_AOS_SHUTDOWN,

	/* Ratio-metric voltage modes */
	/** Ratio-metric voltage, internal, 1 x VDD5V. */
	MAX40109_AOS_RATIO_METRIC_VDD5V_1X = MAX40109_DT_AOS_RATIO_VDD5V_1X,
	/** Ratio-metric voltage, internal, 0.91 x VDD5V. Use when VDDHV = 3.3 V. */
	MAX40109_AOS_RATIO_METRIC_VDD5V_0_91X = MAX40109_DT_AOS_RATIO_VDD5V_0_91X,
	/** Ratio-metric voltage, internal, 0.6 x VDD5V. Use when VDDHV = 5.0 V. */
	MAX40109_AOS_RATIO_METRIC_VDD5V_0_6X = MAX40109_DT_AOS_RATIO_VDD5V_0_6X,

	/* External ratio-metric voltage modes */
	/** Ratio-metric voltage, external, 0.1515 x VDD5V x extGain. */
	MAX40109_AOS_RATIO_METRIC_EXT_0_1515X = MAX40109_DT_AOS_RATIO_EXT_0_1515X,
	/** Ratio-metric voltage, external, 0.25 x VDD5V x extGain. */
	MAX40109_AOS_RATIO_METRIC_EXT_0_25X = MAX40109_DT_AOS_RATIO_EXT_0_25X,

	/* Absolute current mode */
	/** Absolute current, 0.96 x REFIN / RSENSE. Needs VDDHV >= 5 V. */
	MAX40109_AOS_ABSOLUTE_CURRENT = MAX40109_DT_AOS_ABS_CURRENT,

	/* Absolute voltage modes */
	/** Absolute voltage, internal, 4 x REFIN. REFIN = 1.25 V, 5 V full scale. */
	MAX40109_AOS_ABSOLUTE_VOLTAGE_4X = MAX40109_DT_AOS_ABS_VOLTAGE_4X,
	/** Absolute voltage, internal, 6 x REFIN. REFIN = 0.5 V, 3 V full scale. */
	MAX40109_AOS_ABSOLUTE_VOLTAGE_6X_1 = MAX40109_DT_AOS_ABS_VOLTAGE_6X_1,
	/** Absolute voltage, internal, 6 x REFIN. REFIN = 0.5 V, 3 V full scale. */
	MAX40109_AOS_ABSOLUTE_VOLTAGE_6X_2 = MAX40109_DT_AOS_ABS_VOLTAGE_6X_2,

	/* External absolute voltage modes */
	/** Absolute voltage, external, 0.4 x REFIN x extGain. */
	MAX40109_AOS_ABSOLUTE_VOLTAGE_EXT_0_4X = MAX40109_DT_AOS_ABS_VOLTAGE_EXT_0_4X,
	/** Absolute voltage, external, 1 x REFIN x extGain. */
	MAX40109_AOS_ABSOLUTE_VOLTAGE_EXT_1X = MAX40109_DT_AOS_ABS_VOLTAGE_EXT_1X,

	/* Additional ratio-metric voltage modes (same as 0x2) */
	/** Alias of MAX40109_AOS_RATIO_METRIC_VDD5V_1X. */
	MAX40109_AOS_RATIO_METRIC_VDD5V_1X_ALT1 = MAX40109_DT_AOS_RATIO_VDD5V_1X_ALT1,
	/** Alias of MAX40109_AOS_RATIO_METRIC_VDD5V_1X. */
	MAX40109_AOS_RATIO_METRIC_VDD5V_1X_ALT2 = MAX40109_DT_AOS_RATIO_VDD5V_1X_ALT2,
	/** Alias of MAX40109_AOS_RATIO_METRIC_VDD5V_1X. */
	MAX40109_AOS_RATIO_METRIC_VDD5V_1X_ALT3 = MAX40109_DT_AOS_RATIO_VDD5V_1X_ALT3,
};

/**
 * @brief Temperature PGA gain
 *
 * Runtime values for the temperature signal path gain.
 */
enum sensor_max40109_pga_temperature_gain {
	/** Gain = 1.5. */
	SENSOR_MAX40109_PGA_TEMPERATURE_GAIN_1_5 = MAX40109_DT_PGA_TEMP_GAIN_1_5,
	/** Gain = 2. */
	SENSOR_MAX40109_PGA_TEMPERATURE_GAIN_2 = MAX40109_DT_PGA_TEMP_GAIN_2,
	/** Gain = 3. */
	SENSOR_MAX40109_PGA_TEMPERATURE_GAIN_3 = MAX40109_DT_PGA_TEMP_GAIN_3,
	/** Gain = 5. */
	SENSOR_MAX40109_PGA_TEMPERATURE_GAIN_5 = MAX40109_DT_PGA_TEMP_GAIN_5,
	/** Gain = 6. */
	SENSOR_MAX40109_PGA_TEMPERATURE_GAIN_6 = MAX40109_DT_PGA_TEMP_GAIN_6,
	/** Gain = 10. */
	SENSOR_MAX40109_PGA_TEMPERATURE_GAIN_10 = MAX40109_DT_PGA_TEMP_GAIN_10,
	/** Gain = 15. */
	SENSOR_MAX40109_PGA_TEMPERATURE_GAIN_15 = MAX40109_DT_PGA_TEMP_GAIN_15,
	/** Gain = 20. */
	SENSOR_MAX40109_PGA_TEMPERATURE_GAIN_20 = MAX40109_DT_PGA_TEMP_GAIN_20,
	/** Gain = 24. */
	SENSOR_MAX40109_PGA_TEMPERATURE_GAIN_24 = MAX40109_DT_PGA_TEMP_GAIN_24,
	/** Gain = 30. */
	SENSOR_MAX40109_PGA_TEMPERATURE_GAIN_30 = MAX40109_DT_PGA_TEMP_GAIN_30,
	/** Gain = 36. */
	SENSOR_MAX40109_PGA_TEMPERATURE_GAIN_36 = MAX40109_DT_PGA_TEMP_GAIN_36,
	/** Gain = 40. */
	SENSOR_MAX40109_PGA_TEMPERATURE_GAIN_40 = MAX40109_DT_PGA_TEMP_GAIN_40,
	/** Gain = 45. */
	SENSOR_MAX40109_PGA_TEMPERATURE_GAIN_45 = MAX40109_DT_PGA_TEMP_GAIN_45,
	/** Gain = 60. */
	SENSOR_MAX40109_PGA_TEMPERATURE_GAIN_60 = MAX40109_DT_PGA_TEMP_GAIN_60,
	/** Gain = 72. */
	SENSOR_MAX40109_PGA_TEMPERATURE_GAIN_72 = MAX40109_DT_PGA_TEMP_GAIN_72,
	/** Gain = 90. */
	SENSOR_MAX40109_PGA_TEMPERATURE_GAIN_90 = MAX40109_DT_PGA_TEMP_GAIN_90,
};

/**
 * @brief Analog filter bandwidth
 *
 * Runtime values for SENSOR_ATTR_MAX40109_ANALOG_FILTER_BW_SETUP.
 */
enum max40109_analog_filter_bw_settings {
	/** 1.2 kHz. */
	MAX40109_ANALOG_FILTER_BW_1200HZ = MAX40109_DT_ANALOG_FILTER_BW_1200HZ,
	/** 9 kHz. */
	MAX40109_ANALOG_FILTER_BW_9000HZ = MAX40109_DT_ANALOG_FILTER_BW_9000HZ,
	/** 37 kHz. */
	MAX40109_ANALOG_FILTER_BW_37000HZ = MAX40109_DT_ANALOG_FILTER_BW_37000HZ,
	/** 45 kHz. */
	MAX40109_ANALOG_FILTER_BW_45000HZ = MAX40109_DT_ANALOG_FILTER_BW_45000HZ,
};

/**
 * @brief Digital filter averaging depth
 *
 * Runtime values for SENSOR_ATTR_MAX40109_DIGITAL_FILTER_SETUP.
 */
enum max40109_digital_filter_setup {
	/** No averaging. */
	MAX40109_DIGITAL_FILTER_SETUP_AVERAGE_NONE = MAX40109_DT_FILTER_AVG_NONE,
	/** Average among 4 samples. */
	MAX40109_DIGITAL_FILTER_SETUP_AVERAGE_4 = MAX40109_DT_FILTER_AVG_4,
	/** Average among 8 samples. */
	MAX40109_DIGITAL_FILTER_SETUP_AVERAGE_8 = MAX40109_DT_FILTER_AVG_8,
	/** Average among 16 samples. */
	MAX40109_DIGITAL_FILTER_SETUP_AVERAGE_16 = MAX40109_DT_FILTER_AVG_16,
	/** Average among 32 samples. */
	MAX40109_DIGITAL_FILTER_SETUP_AVERAGE_32 = MAX40109_DT_FILTER_AVG_32,
	/** Average among 64 samples. */
	MAX40109_DIGITAL_FILTER_SETUP_AVERAGE_64 = MAX40109_DT_FILTER_AVG_64,
	/** Average among 128 samples. */
	MAX40109_DIGITAL_FILTER_SETUP_AVERAGE_128 = MAX40109_DT_FILTER_AVG_128,
};

/**
 * @brief ALERT pin mode
 *
 * Runtime values for SENSOR_ATTR_MAX40109_ALERT_MODE. Only
 * MAX40109_ALERT_ISSUES_INTERRUPT drives the trigger API.
 */
enum max40109_alert_mode {
	/** ALERT issues interrupts. */
	MAX40109_ALERT_ISSUES_INTERRUPT = MAX40109_DT_ALERT_INTERRUPT,
	/** ALERT issues temperature data in PWM fashion. */
	MAX40109_ALERT_ISSUES_TEMPERATURE_DATA_IN_PWM_FASHION = MAX40109_DT_ALERT_TEMP_PWM,
	/** ALERT issues pressure digital output, case 1. */
	MAX40109_ALERT_ISSUES_PRESSURE_DIGITAL_OUTPUT_CASE_1 = MAX40109_DT_ALERT_PRESSURE_CASE1,
	/** ALERT issues pressure digital output, case 2. */
	MAX40109_ALERT_ISSUES_PRESSURE_DIGITAL_OUTPUT_CASE_2 = MAX40109_DT_ALERT_PRESSURE_CASE2,
	/** ALERT issues pressure digital output, case 3. */
	MAX40109_ALERT_ISSUES_PRESSURE_DIGITAL_OUTPUT_CASE_3 = MAX40109_DT_ALERT_PRESSURE_CASE3,
	/** ALERT issues pressure digital output, case 4. */
	MAX40109_ALERT_ISSUES_PRESSURE_DIGITAL_OUTPUT_CASE_4 = MAX40109_DT_ALERT_PRESSURE_CASE4,
};

/**
 * @brief Temperature measurement mode
 *
 * Runtime values for SENSOR_ATTR_MAX40109_TEMP_MODE.
 */
enum max40109_temp_mode {
	/** Single-ended with G = 1, input pin INT. */
	MAX40109_TEMP_MODE_INT_SE_GAIN_1 = MAX40109_DT_TEMP_MODE_INT_SE_GAIN_1,
	/** Single-ended with G > 1, input pin INT. */
	MAX40109_TEMP_MODE_INT_SE_GAIN_GT_1 = MAX40109_DT_TEMP_MODE_INT_SE_GAIN_GT_1,
	/** Differential with DAC's offset, input pin INT. */
	MAX40109_TEMP_MODE_INT_DIFF_DAC_OFFSET = MAX40109_DT_TEMP_MODE_INT_DIFF_DAC_OFFS,
	/** Single-ended with G = 1, input pin DRV. */
	MAX40109_TEMP_MODE_DRV_SE_GAIN_1 = MAX40109_DT_TEMP_MODE_DRV_SE_GAIN_1,
	/** Differential with DAC's offset, input pin DRV. */
	MAX40109_TEMP_MODE_DRV_DIFF_DAC_OFFSET = MAX40109_DT_TEMP_MODE_DRV_DIFF_DAC_OFFS,
};

/**
 * @brief DRV input scale
 *
 * Runtime values for SENSOR_ATTR_MAX40109_DRV_SCALE.
 */
enum max40109_drv_scale {
	/** 1.0x. */
	MAX40109_DRV_SCALE_1_0 = MAX40109_DT_DRV_SCALE_1_0,
	/** 0.5x. */
	MAX40109_DRV_SCALE_0_5 = MAX40109_DT_DRV_SCALE_0_5,
	/** 0.375x. */
	MAX40109_DRV_SCALE_0_375 = MAX40109_DT_DRV_SCALE_0_375,
	/** 0.25x. */
	MAX40109_DRV_SCALE_0_25 = MAX40109_DT_DRV_SCALE_0_25,
};

/**
 * @brief MTP calibration coefficients
 *
 * Selector for max40109_mtp_calibration() and
 * max40109_mtp_calibration_read(). K0-K3 compensate ambient temperature;
 * H0-H3, G0-G3, N0-N3 and M0-M3 compensate pressure.
 */
enum max40109_calibration_coefficients {
	/** Ambient temperature calibration coefficient K0. */
	MAX40109_CALIBRATION_K0,
	/** Ambient temperature calibration coefficient K1. */
	MAX40109_CALIBRATION_K1,
	/** Ambient temperature calibration coefficient K2. */
	MAX40109_CALIBRATION_K2,
	/** Ambient temperature calibration coefficient K3. */
	MAX40109_CALIBRATION_K3,
	/** Pressure calibration coefficient H0. */
	MAX40109_CALIBRATION_H0,
	/** Pressure calibration coefficient H1. */
	MAX40109_CALIBRATION_H1,
	/** Pressure calibration coefficient H2. */
	MAX40109_CALIBRATION_H2,
	/** Pressure calibration coefficient H3. */
	MAX40109_CALIBRATION_H3,
	/** Pressure calibration coefficient G0. */
	MAX40109_CALIBRATION_G0,
	/** Pressure calibration coefficient G1. */
	MAX40109_CALIBRATION_G1,
	/** Pressure calibration coefficient G2. */
	MAX40109_CALIBRATION_G2,
	/** Pressure calibration coefficient G3. */
	MAX40109_CALIBRATION_G3,
	/** Pressure calibration coefficient N0. */
	MAX40109_CALIBRATION_N0,
	/** Pressure calibration coefficient N1. */
	MAX40109_CALIBRATION_N1,
	/** Pressure calibration coefficient N2. */
	MAX40109_CALIBRATION_N2,
	/** Pressure calibration coefficient N3. */
	MAX40109_CALIBRATION_N3,
	/** Pressure calibration coefficient M0. */
	MAX40109_CALIBRATION_M0,
	/** Pressure calibration coefficient M1. */
	MAX40109_CALIBRATION_M1,
	/** Pressure calibration coefficient M2. */
	MAX40109_CALIBRATION_M2,
	/** Pressure calibration coefficient M3. */
	MAX40109_CALIBRATION_M3,
};

#if defined(CONFIG_MAX40109_CALIBRATION)
/**
 * @brief Apply MTP calibration (K0-M3) for MAX40109
 *
 * @note Only available when @kconfig{CONFIG_MAX40109_CALIBRATION} is enabled.
 *
 * @param dev Device Structure
 * @param coeff Calibration Coefficient
 * @param value Calibration Value
 * @param burn Burn flag (true to burn, false to write only)
 * @return int 0 on success, negative error code on failure
 */
int max40109_mtp_calibration(const struct device *dev, enum max40109_calibration_coefficients coeff,
			     float value, bool burn);
/**
 * @brief Read calibration coefficient from MAX40109 MTP
 *
 * @note Only available when @kconfig{CONFIG_MAX40109_CALIBRATION} is enabled.
 *
 * @param dev Device Structure
 * @param coeff Calibration Coefficient
 * @param value Pointer to store read value
 * @return int 0 on success, negative error code on failure
 */
int max40109_mtp_calibration_read(const struct device *dev,
				  enum max40109_calibration_coefficients coeff, float *value);
#endif /* CONFIG_MAX40109_CALIBRATION */
/**
 * @brief Read MTP data from MAX40109
 *
 * @param dev Device Structure
 * @param mtp_addr MTP Address to read from
 * @param val Data buffer pointer to store read value
 * @return int 0 on success, negative error code on failure
 */
int max40109_mtp_read(const struct device *dev, uint8_t mtp_addr, uint8_t *val);
/**
 * @brief Write MTP data to MAX40109
 *
 * @param dev Device Structure
 * @param mtp_addr MTP Address to write to
 * @param val Data buffer pointer containing value to write
 * @return int 0 on success, negative error code on failure
 */
int max40109_mtp_write(const struct device *dev, uint8_t mtp_addr, uint8_t *val);
/**
 * @brief Update specific bits in MTP data of MAX40109
 *
 * @param dev Device Structure
 * @param mtp_addr MTP Address
 * @param mask Bit mask to specify which bits to update
 * @param val New value for the specified bits
 * @return int 0 on success, negative error code on failure
 */
int max40109_mtp_update(const struct device *dev, uint8_t mtp_addr, uint16_t mask, uint16_t val);
/**
 * @brief Initialize MTP programming sequence for MAX40109
 *
 * @param dev Device Structure
 * @return int 0 on success, negative error code on failure
 */
int max40109_mtp_initialize(const struct device *dev);
/**
 * @brief Burn MTP data to MAX40109
 *
 * Permanently writes a 16-bit value to the MTP (Multi-Time Programmable) memory.
 * The MTP content persists across power cycles and initializes RAM registers on power-up.
 *
 * @note This operation is irreversible and has limited write cycles. Use with caution.
 *
 * @note The caller must apply at least 12V to VDDHV and run
 *       max40109_mtp_initialize() once before the first burn of a session.
 *       This function performs only the per-register burn procedure.
 *
 * @param dev Device Structure
 * @param mtp_addr MTP Address to write to
 * @param val Data buffer containing 2 bytes to burn, in order: [MSB, LSB]
 *            Example: {0x12, 0x34} writes 0x1234 to MTP
 *            The MSB (val[0]) is written first, followed by LSB (val[1])
 *
 * @return int 0 on success, negative error code on failure
 */
int max40109_mtp_burn(const struct device *dev, uint8_t mtp_addr, uint8_t *val);

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_MAX40109_H_ */
