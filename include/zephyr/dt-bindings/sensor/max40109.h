/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for the Analog Devices MAX40109 AFE.
 * @ingroup max40109_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_MAX40109_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_MAX40109_H_

/**
 * @defgroup max40109_interface MAX40109
 * @ingroup sensor_interface_ext_adi
 * @brief Analog Devices MAX40109 precision sensor interface AFE
 * @{
 */

/**
 * @name Digital filter setting
 *
 * Values for the `digital-filter` devicetree property. The averaging depth is
 * 2^(N+1) samples for every code except "no average".
 * @{
 */
/** No averaging */
#define MAX40109_DT_FILTER_AVG_NONE 0x00
/** Average among 4 samples */
#define MAX40109_DT_FILTER_AVG_4    0x01
/** Average among 8 samples */
#define MAX40109_DT_FILTER_AVG_8    0x02
/** Average among 16 samples */
#define MAX40109_DT_FILTER_AVG_16   0x03
/** Average among 32 samples */
#define MAX40109_DT_FILTER_AVG_32   0x04
/** Average among 64 samples */
#define MAX40109_DT_FILTER_AVG_64   0x05
/** Average among 128 samples */
#define MAX40109_DT_FILTER_AVG_128  0x06
/** @} */

/**
 * @name ALERT pin mode
 *
 * Values for the `alert-mode` devicetree property.
 * @{
 */
/** ALERT issues interrupts */
#define MAX40109_DT_ALERT_INTERRUPT      0x00
/** ALERT issues temperature data in PWM fashion */
#define MAX40109_DT_ALERT_TEMP_PWM       0x01
/** ALERT issues pressure digital output - case 1 */
#define MAX40109_DT_ALERT_PRESSURE_CASE1 0x02
/** ALERT issues pressure digital output - case 2 */
#define MAX40109_DT_ALERT_PRESSURE_CASE2 0x03
/** ALERT issues pressure digital output - case 3 */
#define MAX40109_DT_ALERT_PRESSURE_CASE3 0x04
/** ALERT issues pressure digital output - case 4 */
#define MAX40109_DT_ALERT_PRESSURE_CASE4 0x05
/** @} */

/**
 * @name Temperature sensor current
 *
 * Values for the `temp-current` devicetree property.
 * @{
 */
/** 150 uA */
#define MAX40109_DT_TEMP_CURRENT_150UA 0x00
/** 250 uA */
#define MAX40109_DT_TEMP_CURRENT_250UA 0x01
/** 500 uA */
#define MAX40109_DT_TEMP_CURRENT_500UA 0x02
/** 750 uA */
#define MAX40109_DT_TEMP_CURRENT_750UA 0x03
/** @} */

/**
 * @name Pressure PGA gain
 *
 * Values for the `pga-pressure-gain` devicetree property. Note the codes are
 * not monotonic in gain: 0x0F sits below 0x00, and 0x10-0x13 continue above
 * 0x0E.
 * @{
 */
/** Gain = 5 */
#define MAX40109_DT_PGA_PRESSURE_GAIN_5    0x0F
/** Gain = 10 */
#define MAX40109_DT_PGA_PRESSURE_GAIN_10   0x00
/** Gain = 15 */
#define MAX40109_DT_PGA_PRESSURE_GAIN_15   0x01
/** Gain = 20 */
#define MAX40109_DT_PGA_PRESSURE_GAIN_20   0x02
/** Gain = 24 */
#define MAX40109_DT_PGA_PRESSURE_GAIN_24   0x03
/** Gain = 40 */
#define MAX40109_DT_PGA_PRESSURE_GAIN_40   0x04
/** Gain = 60 */
#define MAX40109_DT_PGA_PRESSURE_GAIN_60   0x05
/** Gain = 72 */
#define MAX40109_DT_PGA_PRESSURE_GAIN_72   0x06
/** Gain = 90 */
#define MAX40109_DT_PGA_PRESSURE_GAIN_90   0x07
/** Gain = 108 */
#define MAX40109_DT_PGA_PRESSURE_GAIN_108  0x08
/** Gain = 126 */
#define MAX40109_DT_PGA_PRESSURE_GAIN_126  0x09
/** Gain = 144 */
#define MAX40109_DT_PGA_PRESSURE_GAIN_144  0x0A
/** Gain = 160 */
#define MAX40109_DT_PGA_PRESSURE_GAIN_160  0x0B
/** Gain = 180 */
#define MAX40109_DT_PGA_PRESSURE_GAIN_180  0x0C
/** Gain = 200 */
#define MAX40109_DT_PGA_PRESSURE_GAIN_200  0x0D
/** Gain = 252 */
#define MAX40109_DT_PGA_PRESSURE_GAIN_252  0x0E
/** Gain = 540 */
#define MAX40109_DT_PGA_PRESSURE_GAIN_540  0x10
/** Gain = 1080 */
#define MAX40109_DT_PGA_PRESSURE_GAIN_1080 0x11
/** Gain = 1440 */
#define MAX40109_DT_PGA_PRESSURE_GAIN_1440 0x12
/** Gain = 2520 */
#define MAX40109_DT_PGA_PRESSURE_GAIN_2520 0x13
/** @} */

/**
 * @name Excitation current source
 *
 * Values for the `current-source` devicetree property.
 * @{
 */
/** 250 uA */
#define MAX40109_DT_CURRENT_SOURCE_250UA 0x00
/** 300 uA */
#define MAX40109_DT_CURRENT_SOURCE_300UA 0x01
/** 400 uA */
#define MAX40109_DT_CURRENT_SOURCE_400UA 0x02
/** 450 uA */
#define MAX40109_DT_CURRENT_SOURCE_450UA 0x03
/** 500 uA */
#define MAX40109_DT_CURRENT_SOURCE_500UA 0x04
/** 550 uA */
#define MAX40109_DT_CURRENT_SOURCE_550UA 0x05
/** 650 uA */
#define MAX40109_DT_CURRENT_SOURCE_650UA 0x06
/** 750 uA */
#define MAX40109_DT_CURRENT_SOURCE_750UA 0x07
/** @} */

/**
 * @name ADC sample rate
 *
 * Values for the `adc-sample-rate` devicetree property. Each code selects a
 * pressure rate and a temperature rate together.
 * @{
 */
/** Pressure 1 kSPS, temperature 1 SPS */
#define MAX40109_DT_SAMPLE_RATE_1KSPS_1SPS   0x00
/** Pressure 1 kSPS, temperature 10 SPS */
#define MAX40109_DT_SAMPLE_RATE_1KSPS_10SPS  0x01
/** Pressure 2 kSPS, temperature 1 SPS */
#define MAX40109_DT_SAMPLE_RATE_2KSPS_1SPS   0x02
/** Pressure 2 kSPS, temperature 10 SPS */
#define MAX40109_DT_SAMPLE_RATE_2KSPS_10SPS  0x03
/** Pressure 4 kSPS, temperature 1 SPS */
#define MAX40109_DT_SAMPLE_RATE_4KSPS_1SPS   0x04
/** Pressure 4 kSPS, temperature 10 SPS */
#define MAX40109_DT_SAMPLE_RATE_4KSPS_10SPS  0x05
/** Pressure 8 kSPS, temperature 1 SPS */
#define MAX40109_DT_SAMPLE_RATE_8KSPS_1SPS   0x06
/** Pressure 8 kSPS, temperature 10 SPS */
#define MAX40109_DT_SAMPLE_RATE_8KSPS_10SPS  0x07
/** Pressure 16 kSPS, temperature 1 SPS */
#define MAX40109_DT_SAMPLE_RATE_16KSPS_1SPS  0x08
/** Pressure 16 kSPS, temperature 10 SPS */
#define MAX40109_DT_SAMPLE_RATE_16KSPS_10SPS 0x09
/** @} */

/**
 * @name Temperature PGA gain
 *
 * Values for the `pga-temperature-gain` devicetree property.
 * @{
 */
/** Gain = 1.5 */
#define MAX40109_DT_PGA_TEMP_GAIN_1_5 0x00
/** Gain = 2 */
#define MAX40109_DT_PGA_TEMP_GAIN_2   0x01
/** Gain = 3 */
#define MAX40109_DT_PGA_TEMP_GAIN_3   0x02
/** Gain = 5 */
#define MAX40109_DT_PGA_TEMP_GAIN_5   0x03
/** Gain = 6 */
#define MAX40109_DT_PGA_TEMP_GAIN_6   0x04
/** Gain = 10 */
#define MAX40109_DT_PGA_TEMP_GAIN_10  0x05
/** Gain = 15 */
#define MAX40109_DT_PGA_TEMP_GAIN_15  0x06
/** Gain = 20 */
#define MAX40109_DT_PGA_TEMP_GAIN_20  0x07
/** Gain = 24 */
#define MAX40109_DT_PGA_TEMP_GAIN_24  0x08
/** Gain = 30 */
#define MAX40109_DT_PGA_TEMP_GAIN_30  0x09
/** Gain = 36 */
#define MAX40109_DT_PGA_TEMP_GAIN_36  0x0A
/** Gain = 40 */
#define MAX40109_DT_PGA_TEMP_GAIN_40  0x0B
/** Gain = 45 */
#define MAX40109_DT_PGA_TEMP_GAIN_45  0x0C
/** Gain = 60 */
#define MAX40109_DT_PGA_TEMP_GAIN_60  0x0D
/** Gain = 72 */
#define MAX40109_DT_PGA_TEMP_GAIN_72  0x0E
/** Gain = 90 */
#define MAX40109_DT_PGA_TEMP_GAIN_90  0x0F
/** @} */

/**
 * @name Bridge drive
 *
 * Values for the `bridge-drive` devicetree property.
 * @{
 */
/** Disconnected. Default mode at device power up */
#define MAX40109_DT_BRIDGE_DRIVE_DISCONNECTED   0x00
/** Current source. The ADC reference is connected to the internal reference */
#define MAX40109_DT_BRIDGE_DRIVE_CURRENT_SOURCE 0x01
/** Voltage source at 4 V. The ADC reference is connected to the bridge voltage */
#define MAX40109_DT_BRIDGE_DRIVE_VOLTAGE_4V     0x02
/** Voltage source at 3.3 V. The ADC reference is connected to the bridge voltage */
#define MAX40109_DT_BRIDGE_DRIVE_VOLTAGE_3V3    0x03
/** Voltage source at 1.8 V. The ADC reference is connected to the bridge voltage */
#define MAX40109_DT_BRIDGE_DRIVE_VOLTAGE_1V8    0x04
/** Voltage source at 2.3 V. The ADC reference is connected to the bridge voltage */
#define MAX40109_DT_BRIDGE_DRIVE_VOLTAGE_2V3    0x05
/** @} */

/**
 * @name Temperature measurement mode
 *
 * Values for the `temp-mode` devicetree property.
 * @{
 */
/** Single-ended with G = 1, input pin INT */
#define MAX40109_DT_TEMP_MODE_INT_SE_GAIN_1     0x00
/** Single-ended with G > 1, input pin INT */
#define MAX40109_DT_TEMP_MODE_INT_SE_GAIN_GT_1  0x01
/** Differential with DAC's offset, input pin INT */
#define MAX40109_DT_TEMP_MODE_INT_DIFF_DAC_OFFS 0x02
/** Single-ended with G = 1, input pin DRV */
#define MAX40109_DT_TEMP_MODE_DRV_SE_GAIN_1     0x03
/** Differential with DAC's offset, input pin DRV */
#define MAX40109_DT_TEMP_MODE_DRV_DIFF_DAC_OFFS 0x04
/** @} */

/**
 * @name DRV input scale
 *
 * Values for the `drv-scale` devicetree property.
 * @{
 */
/** 1.0x */
#define MAX40109_DT_DRV_SCALE_1_0   0x00
/** 0.5x */
#define MAX40109_DT_DRV_SCALE_0_5   0x01
/** 0.375x */
#define MAX40109_DT_DRV_SCALE_0_375 0x02
/** 0.25x */
#define MAX40109_DT_DRV_SCALE_0_25  0x03
/** @} */

/**
 * @name Analog filter bandwidth
 *
 * Values for the `analog-filter-bw` devicetree property.
 * @{
 */
/** 1.2 kHz */
#define MAX40109_DT_ANALOG_FILTER_BW_1200HZ  0x00
/** 9 kHz */
#define MAX40109_DT_ANALOG_FILTER_BW_9000HZ  0x01
/** 37 kHz */
#define MAX40109_DT_ANALOG_FILTER_BW_37000HZ 0x02
/** 45 kHz */
#define MAX40109_DT_ANALOG_FILTER_BW_45000HZ 0x03
/** @} */

/**
 * @name Analog output stage (AOS)
 *
 * Values for the `analog-output-stage` devicetree property. Each code selects
 * the function, reference and full-scale of the analog output. Code 0x01 is
 * reserved and has no macro. Codes 0x0D-0x0F behave the same as 0x02.
 * @{
 */
/** Shutdown */
#define MAX40109_DT_AOS_SHUTDOWN             0x00
/** Ratio-metric voltage, internal, 1 x VDD5V */
#define MAX40109_DT_AOS_RATIO_VDD5V_1X       0x02
/** Ratio-metric voltage, internal, 0.91 x VDD5V. Use when VDDHV = 3.3 V */
#define MAX40109_DT_AOS_RATIO_VDD5V_0_91X    0x03
/** Ratio-metric voltage, internal, 0.6 x VDD5V. Use when VDDHV = 5.0 V */
#define MAX40109_DT_AOS_RATIO_VDD5V_0_6X     0x04
/** Ratio-metric voltage, external, 0.1515 x VDD5V x extGain */
#define MAX40109_DT_AOS_RATIO_EXT_0_1515X    0x05
/** Ratio-metric voltage, external, 0.25 x VDD5V x extGain */
#define MAX40109_DT_AOS_RATIO_EXT_0_25X      0x06
/** Absolute current, 0.96 x REFIN / RSENSE. Needs VDDHV >= 5 V */
#define MAX40109_DT_AOS_ABS_CURRENT          0x07
/** Absolute voltage, internal, 4 x REFIN. REFIN = 1.25 V, 5 V full scale */
#define MAX40109_DT_AOS_ABS_VOLTAGE_4X       0x08
/** Absolute voltage, internal, 6 x REFIN. REFIN = 0.5 V, 3 V full scale */
#define MAX40109_DT_AOS_ABS_VOLTAGE_6X_1     0x09
/** Absolute voltage, internal, 6 x REFIN. REFIN = 0.5 V, 3 V full scale */
#define MAX40109_DT_AOS_ABS_VOLTAGE_6X_2     0x0A
/** Absolute voltage, external, 0.4 x REFIN x extGain */
#define MAX40109_DT_AOS_ABS_VOLTAGE_EXT_0_4X 0x0B
/** Absolute voltage, external, 1 x REFIN x extGain */
#define MAX40109_DT_AOS_ABS_VOLTAGE_EXT_1X   0x0C
/** Ratio-metric voltage, internal, 1 x VDD5V (alias of 0x02) */
#define MAX40109_DT_AOS_RATIO_VDD5V_1X_ALT1  0x0D
/** Ratio-metric voltage, internal, 1 x VDD5V (alias of 0x02) */
#define MAX40109_DT_AOS_RATIO_VDD5V_1X_ALT2  0x0E
/** Ratio-metric voltage, internal, 1 x VDD5V (alias of 0x02) */
#define MAX40109_DT_AOS_RATIO_VDD5V_1X_ALT3  0x0F
/** @} */

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_MAX40109_H_ */
