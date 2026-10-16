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
 * Defines the MTP address map, the MAX40109-specific sensor channels
 * and attributes, and the MTP access helpers.
 *
 * SENSOR_CHAN_PRESS and SENSOR_CHAN_AMBIENT_TEMP report the compensated
 * readings in kPa and degrees Celsius. The device reports them as fractions
 * of full scale, so each channel needs its full scale in devicetree
 * (pressure-full-scale-pascal, temperature-full-scale-millicelsius) and
 * returns -ENOTSUP without it. The codes themselves are always available on
 * SENSOR_CHAN_MAX40109_PRESS_COMPENSATED and
 * SENSOR_CHAN_MAX40109_AMBIENT_TEMP_COMPENSATED.
 *
 * Standard attributes:
 * - SENSOR_ATTR_SAMPLING_FREQUENCY, in Hz: 1000, 2000, 4000, 8000 or 16000 on
 *   SENSOR_CHAN_PRESS, 1 or 10 on SENSOR_CHAN_AMBIENT_TEMP. Both rates share
 *   one register, so setting one keeps the other.
 * - SENSOR_ATTR_GAIN, in V/V: a pressure PGA gain from 5 to 2520 on
 *   SENSOR_CHAN_PRESS, a temperature PGA gain from 1.5 to 90 on
 *   SENSOR_CHAN_AMBIENT_TEMP. Only the gains the PGA supports are accepted.
 * - SENSOR_ATTR_CALIB_TARGET, on the coefficient channels of
 *   @ref max40109_sensor_channel.
 * @ingroup max40109_interface
 */

/**
 * @name MTP calibration coefficient words
 *
 * Storage for the 20 calibration coefficients. Each 32-bit coefficient
 * occupies two consecutive words, bits [15:0] first.
 * @{
 */
#define MAX40109_MTP_REG_CAL_DATA0  0x00u /**< Coefficient K0, bits [15:0]. */
#define MAX40109_MTP_REG_CAL_DATA1  0x01u /**< Coefficient K0, bits [31:16]. */
#define MAX40109_MTP_REG_CAL_DATA2  0x02u /**< Coefficient K1, bits [15:0]. */
#define MAX40109_MTP_REG_CAL_DATA3  0x03u /**< Coefficient K1, bits [31:16]. */
#define MAX40109_MTP_REG_CAL_DATA4  0x04u /**< Coefficient K2, bits [15:0]. */
#define MAX40109_MTP_REG_CAL_DATA5  0x05u /**< Coefficient K2, bits [31:16]. */
#define MAX40109_MTP_REG_CAL_DATA6  0x06u /**< Coefficient K3, bits [15:0]. */
#define MAX40109_MTP_REG_CAL_DATA7  0x07u /**< Coefficient K3, bits [31:16]. */
#define MAX40109_MTP_REG_CAL_DATA8  0x08u /**< Coefficient H0, bits [15:0]. */
#define MAX40109_MTP_REG_CAL_DATA9  0x09u /**< Coefficient H0, bits [31:16]. */
#define MAX40109_MTP_REG_CAL_DATA10 0x0au /**< Coefficient H1, bits [15:0]. */
#define MAX40109_MTP_REG_CAL_DATA11 0x0bu /**< Coefficient H1, bits [31:16]. */
#define MAX40109_MTP_REG_CAL_DATA12 0x0cu /**< Coefficient H2, bits [15:0]. */
#define MAX40109_MTP_REG_CAL_DATA13 0x0du /**< Coefficient H2, bits [31:16]. */
#define MAX40109_MTP_REG_CAL_DATA14 0x0eu /**< Coefficient H3, bits [15:0]. */
#define MAX40109_MTP_REG_CAL_DATA15 0x0fu /**< Coefficient H3, bits [31:16]. */
#define MAX40109_MTP_REG_CAL_DATA16 0x10u /**< Coefficient G0, bits [15:0]. */
#define MAX40109_MTP_REG_CAL_DATA17 0x11u /**< Coefficient G0, bits [31:16]. */
#define MAX40109_MTP_REG_CAL_DATA18 0x12u /**< Coefficient G1, bits [15:0]. */
#define MAX40109_MTP_REG_CAL_DATA19 0x13u /**< Coefficient G1, bits [31:16]. */
#define MAX40109_MTP_REG_CAL_DATA20 0x14u /**< Coefficient G2, bits [15:0]. */
#define MAX40109_MTP_REG_CAL_DATA21 0x15u /**< Coefficient G2, bits [31:16]. */
#define MAX40109_MTP_REG_CAL_DATA22 0x16u /**< Coefficient G3, bits [15:0]. */
#define MAX40109_MTP_REG_CAL_DATA23 0x17u /**< Coefficient G3, bits [31:16]. */
#define MAX40109_MTP_REG_CAL_DATA24 0x18u /**< Coefficient N0, bits [15:0]. */
#define MAX40109_MTP_REG_CAL_DATA25 0x19u /**< Coefficient N0, bits [31:16]. */
#define MAX40109_MTP_REG_CAL_DATA26 0x1au /**< Coefficient N1, bits [15:0]. */
#define MAX40109_MTP_REG_CAL_DATA27 0x1bu /**< Coefficient N1, bits [31:16]. */
#define MAX40109_MTP_REG_CAL_DATA28 0x1cu /**< Coefficient N2, bits [15:0]. */
#define MAX40109_MTP_REG_CAL_DATA29 0x1du /**< Coefficient N2, bits [31:16]. */
#define MAX40109_MTP_REG_CAL_DATA30 0x1eu /**< Coefficient N3, bits [15:0]. */
#define MAX40109_MTP_REG_CAL_DATA31 0x1fu /**< Coefficient N3, bits [31:16]. */
#define MAX40109_MTP_REG_CAL_DATA32 0x20u /**< Coefficient M0, bits [15:0]. */
#define MAX40109_MTP_REG_CAL_DATA33 0x21u /**< Coefficient M0, bits [31:16]. */
#define MAX40109_MTP_REG_CAL_DATA34 0x22u /**< Coefficient M1, bits [15:0]. */
#define MAX40109_MTP_REG_CAL_DATA35 0x23u /**< Coefficient M1, bits [31:16]. */
#define MAX40109_MTP_REG_CAL_DATA36 0x24u /**< Coefficient M2, bits [15:0]. */
#define MAX40109_MTP_REG_CAL_DATA37 0x25u /**< Coefficient M2, bits [31:16]. */
#define MAX40109_MTP_REG_CAL_DATA38 0x26u /**< Coefficient M3, bits [15:0]. */
#define MAX40109_MTP_REG_CAL_DATA39 0x27u /**< Coefficient M3, bits [31:16]. */
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
 * they are set and read with SENSOR_CHAN_ALL. The threshold attributes take
 * an 8-bit code (0 to 255) in val1.
 */
enum max40109_sensor_attr {
	/** Analog filter bandwidth in Hz: 1200, 9000, 37000 or 45000. */
	SENSOR_ATTR_MAX40109_ANALOG_FILTER_BW_SETUP = SENSOR_ATTR_PRIV_START,
	/** ALERT pin function, see @ref max40109_alert_mode. */
	SENSOR_ATTR_MAX40109_ALERT_MODE,
	/** Analog output stage mode, see @ref max40109_aos_mode. */
	SENSOR_ATTR_MAX40109_ANALOG_OUTPUT_STAGE,
	/** Number of samples averaged: 0 (no averaging), 4, 8, 16, 32, 64 or 128. */
	SENSOR_ATTR_MAX40109_DIGITAL_FILTER_SETUP,
	/** Temperature measurement mode, see @ref max40109_temp_mode. */
	SENSOR_ATTR_MAX40109_TEMP_MODE,
	/** DRV input scale factor: 1.0, 0.5, 0.375 or 0.25. */
	SENSOR_ATTR_MAX40109_DRV_SCALE,
	/** Positive over-pressure diagnostic threshold, on SENSOR_CHAN_PRESS. */
	SENSOR_ATTR_MAX40109_OVER_PRESSURE_THRESHOLD_POSITIVE,
	/** Positive under-pressure diagnostic threshold, on SENSOR_CHAN_PRESS. */
	SENSOR_ATTR_MAX40109_UNDER_PRESSURE_THRESHOLD_POSITIVE,
	/** Negative over-pressure diagnostic threshold, on SENSOR_CHAN_PRESS. */
	SENSOR_ATTR_MAX40109_OVER_PRESSURE_THRESHOLD_NEGATIVE,
	/** Negative under-pressure diagnostic threshold, on SENSOR_CHAN_PRESS. */
	SENSOR_ATTR_MAX40109_UNDER_PRESSURE_THRESHOLD_NEGATIVE,
	/** Temperature input over-voltage threshold, on SENSOR_CHAN_AMBIENT_TEMP. */
	SENSOR_ATTR_MAX40109_OVER_TEMPERATURE_VOLTAGE_THRESHOLD,
	/** Temperature input under-voltage threshold, on SENSOR_CHAN_AMBIENT_TEMP. */
	SENSOR_ATTR_MAX40109_UNDER_TEMPERATURE_VOLTAGE_THRESHOLD,
	/** DRV over-voltage diagnostic threshold. */
	SENSOR_ATTR_MAX40109_OVER_VOLTAGE_DRIVE_THRESHOLD,
	/** DRV under-voltage diagnostic threshold. */
	SENSOR_ATTR_MAX40109_UNDER_VOLTAGE_DRIVE_THRESHOLD,
	/** Primary pressure threshold used by the ALERT comparator, on SENSOR_CHAN_PRESS. */
	SENSOR_ATTR_MAX40109_PRIMARY_THRESHOLD_PRESSURE_VALUE,
	/** Hysteresis applied to the primary pressure threshold, on SENSOR_CHAN_PRESS. */
	SENSOR_ATTR_MAX40109_HYSTERESIS_THRESHOLD_PRESSURE_VALUE,
};

/**
 * @brief MAX40109-specific sensor channels
 *
 * These channels extend the standard sensor_channel enum to expose the raw
 * and compensated codes and the MTP calibration coefficients. The codes are
 * signed 16-bit fractions of full scale, 32768 being full scale. The
 * coefficient channels are used with SENSOR_ATTR_CALIB_TARGET and require
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
	/** Compensated ambient temperature code. */
	SENSOR_CHAN_MAX40109_AMBIENT_TEMP_COMPENSATED,
	/** Compensated pressure code. */
	SENSOR_CHAN_MAX40109_PRESS_COMPENSATED,
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
/**
 * @brief Read MTP data from MAX40109
 *
 * @param dev Device Structure
 * @param mtp_addr MTP Address to read from
 * @param val Buffer of at least 2 bytes that receives the 16-bit word, in
 *            order: [MSB, LSB]
 * @return int 0 on success, negative error code on failure
 */
int max40109_mtp_read(const struct device *dev, uint8_t mtp_addr, uint8_t *val);
/**
 * @brief Write MTP data to MAX40109
 *
 * @param dev Device Structure
 * @param mtp_addr MTP Address to write to
 * @param val 2 bytes holding the 16-bit word to write, in order: [MSB, LSB]
 * @return int 0 on success, negative error code on failure
 */
int max40109_mtp_write(const struct device *dev, uint8_t mtp_addr, const uint8_t *val);
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
int max40109_mtp_burn(const struct device *dev, uint8_t mtp_addr, const uint8_t *val);

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_MAX40109_H_ */
