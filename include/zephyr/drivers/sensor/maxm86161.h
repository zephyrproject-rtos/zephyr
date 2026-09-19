/*
 * Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file maxm86161.h
 * @brief Header file for extended sensor API of MAXM86161 sensor
 *
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_MAXM86161_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_MAXM86161_H_

#include <zephyr/sys/util.h>
#include <zephyr/dt-bindings/sensor/maxm86161.h>

/**
 * @name MAXM86161 STATUS1 flags
 *
 * Bit masks for the value returned by SENSOR_ATTR_MAXM86161_STATUS1. The
 * status flags are self-clearing after the register is read.
 * @{
 */
/** FIFO almost-full */
#define MAXM86161_MSK_INT_STATUS1_A_FULL       BIT(7)
/** New PPG data ready */
#define MAXM86161_MSK_INT_STATUS1_DATA_RDY     BIT(6)
/** Ambient light cancellation overflow */
#define MAXM86161_MSK_INT_STATUS1_ALC_OVF      BIT(5)
/** Proximity interrupt */
#define MAXM86161_MSK_INT_STATUS1_PROX_INT     BIT(4)
/** LED compliance */
#define MAXM86161_MSK_INT_STATUS1_LED_COMPB    BIT(3)
/** Die temperature measurement ready */
#define MAXM86161_MSK_INT_STATUS1_DIE_TEMP_RDY BIT(2)
/** Power ready */
#define MAXM86161_MSK_INT_STATUS1_PWR_RDY      BIT(0)
/** @} */

/**
 * @name MAXM86161 STATUS2 flags
 *
 * Bit masks for the value returned by SENSOR_ATTR_MAXM86161_SHA_DONE.
 * @{
 */
/** SHA-256 authentication done */
#define MAXM86161_MSK_INT_STATUS2_SHA_DONE     BIT(0)
/** @} */

#if defined(CONFIG_MAXM86161_TRIGGER)
/**
 * @brief MAXM86161 sensor-specific triggers
 *
 * These triggers extend the standard Zephyr sensor triggers and are specific
 * to the MAXM86161 sensor.
 */
enum maxm86161_sensor_trigger {
	/** Ambient light cancellation overflow */
	SENSOR_TRIG_MAXM86161_ALC_OVERFLOW = SENSOR_TRIG_PRIV_START,
	/** Proximity interrupt */
	SENSOR_TRIG_MAXM86161_PROXIMITY,
	/** LED compliance */
	SENSOR_TRIG_MAXM86161_LED_COMPB,
	/** SHA-256 authentication done */
	SENSOR_TRIG_MAXM86161_SHA_DONE,
};
#endif

/**
 * @brief MAXM86161 sensor-specific channels
 *
 * These channels extend the standard Zephyr sensor channels and are specific
 * to the MAXM86161 sensor. Use these with sensor_attr_set() and sensor_attr_get().
 */
enum maxm86161_sensor_channel {
	/** PPG Channel (General, for attribute use) */
	SENSOR_CHAN_MAXM86161_PPG = SENSOR_CHAN_PRIV_START,
	/** PPG LED Channel 1 */
	SENSOR_CHAN_MAXM86161_FIFO_PPG_LEDC1,
	/** PPG LED Channel 2 */
	SENSOR_CHAN_MAXM86161_FIFO_PPG_LEDC2,
	/** PPG LED Channel 3 */
	SENSOR_CHAN_MAXM86161_FIFO_PPG_LEDC3,
	/** PPG LED Channel 4 */
	SENSOR_CHAN_MAXM86161_FIFO_PPG_LEDC4,
	/** PPG LED Channel 5 */
	SENSOR_CHAN_MAXM86161_FIFO_PPG_LEDC5,
	/** PPG LED Channel 6 */
	SENSOR_CHAN_MAXM86161_FIFO_PPG_LEDC6,
	/** PPG LED Channel 1 (Picket Fence Event) */
	SENSOR_CHAN_MAXM86161_FIFO_PPG_LEDC1_PF,
	/** PPG LED Channel 2 (Picket Fence Event) */
	SENSOR_CHAN_MAXM86161_FIFO_PPG_LEDC2_PF,
	/** PPG LED Channel 3 (Picket Fence Event) */
	SENSOR_CHAN_MAXM86161_FIFO_PPG_LEDC3_PF,
	/** Proximity LED  */
	SENSOR_CHAN_MAXM86161_FIFO_PROX,
	/** Sub-DAC Updated Conversion */
	SENSOR_CHAN_MAXM86161_FIFO_SUB_DAC,
	/** FIFO Timestamp Data */
	SENSOR_CHAN_MAXM86161_FIFO_TIMESTAMP,
};

/**
 * @brief MAXM86161 sensor-specific attributes
 *
 * These attributes extend the standard Zephyr sensor attributes and are specific
 * to the MAXM86161 sensor. Use these with sensor_attr_set() and sensor_attr_get().
 */
enum maxm86161_sensor_attr {
	/* Setter attributes (readable unless specified) */

	/** FIFO almost-full threshold */
	SENSOR_ATTR_MAXM86161_FIFO_WATERMARK = SENSOR_ATTR_PRIV_START,
	/** Flush the FIFO buffer (write-only) */
	SENSOR_ATTR_MAXM86161_FIFO_FLUSH,
	/** FIFO interrupt behavior */
	SENSOR_ATTR_MAXM86161_FIFO_A_FULL_TYPE,
	/** FIFO operating mode */
	SENSOR_ATTR_MAXM86161_FIFO_ROLLOVER,
	/** Enable or disable low power mode */
	SENSOR_ATTR_MAXM86161_LOW_POWER_MODE,
	/** Perform software reset. Self-clearing bit after completed reset */
	SENSOR_ATTR_MAXM86161_RESET,
	/** Enable pushing of Timestamp in FIFO */
	SENSOR_ATTR_MAXM86161_TIME_STAMP_ENABLE,
	/** Enable overriding of FIFO data when subranging DAC code changes */
	SENSOR_ATTR_MAXM86161_DAC_CODE_CHANGE_TAG_ENABLE,
	/** Enable SW Force Sync. Parameter not required */
	SENSOR_ATTR_MAXM86161_SW_FORCE_SYNC_ENABLE,
	/** Enable or disable ambient light cancellation (ALC) */
	SENSOR_ATTR_MAXM86161_ALC_DISABLE,
	/** Enable PPG data offset */
	SENSOR_ATTR_MAXM86161_ADD_OFFSET_ENABLE,
	/** ADC measurement range */
	SENSOR_ATTR_MAXM86161_ADC_RANGE,
	/** LED integration time */
	SENSOR_ATTR_MAXM86161_LED_INTEGRATION_TIME,
	/** sample averaging */
	SENSOR_ATTR_MAXM86161_SAMPLE_AVERAGING,
	/** LED settling time */
	SENSOR_ATTR_MAXM86161_LED_SETTLING_TIME,
	/** ALC method (CDM or FDM) */
	SENSOR_ATTR_MAXM86161_DIGITAL_FILTER_SELECT,
	/** burst mode rate */
	SENSOR_ATTR_MAXM86161_BURST_RATE,
	/** Enable or disable burst mode */
	SENSOR_ATTR_MAXM86161_BURST_ENABLE,
	/** proximity interrupt threshold */
	SENSOR_ATTR_MAXM86161_PROX_INT_THRESHOLD,
	/** photodiode bias capacitance */
	SENSOR_ATTR_MAXM86161_PHOTODIODE_BIAS,
	/** LED Sequence 1 */
	SENSOR_ATTR_MAXM86161_LED_SEQUENCE_1,
	/** LED Sequence 2 */
	SENSOR_ATTR_MAXM86161_LED_SEQUENCE_2,
	/** LED Sequence 3 */
	SENSOR_ATTR_MAXM86161_LED_SEQUENCE_3,
	/** LED Sequence 4 */
	SENSOR_ATTR_MAXM86161_LED_SEQUENCE_4,
	/** LED Sequence 5 */
	SENSOR_ATTR_MAXM86161_LED_SEQUENCE_5,
	/** LED Sequence 6 */
	SENSOR_ATTR_MAXM86161_LED_SEQUENCE_6,
	/** LED1 current amplitude */
	SENSOR_ATTR_MAXM86161_LED1_GREEN_CURRENT,
	/** LED2 current amplitude */
	SENSOR_ATTR_MAXM86161_LED2_IR_CURRENT,
	/** LED3 current amplitude */
	SENSOR_ATTR_MAXM86161_LED3_RED_CURRENT,
	/** Pilot current amplitude */
	SENSOR_ATTR_MAXM86161_PILOT_ON_GREEN_LED_CURRENT,
	/** LED1 current range */
	SENSOR_ATTR_MAXM86161_LED1_GREEN_CURRENT_RANGE,
	/** LED2 current range */
	SENSOR_ATTR_MAXM86161_LED2_IR_CURRENT_RANGE,
	/** LED3 current range */
	SENSOR_ATTR_MAXM86161_LED3_RED_CURRENT_RANGE,
	/** Start DAC calibration */
	SENSOR_ATTR_MAXM86161_DAC_CALIBRATION,

	/* Getter attributes (read-only) */

	/** Status1 flags (self-clearing after read) */
	SENSOR_ATTR_MAXM86161_STATUS1,
	/** SHA-256 Authentication Done status */
	SENSOR_ATTR_MAXM86161_SHA_DONE,
	/** FIFO overflow count */
	SENSOR_ATTR_MAXM86161_FIFO_OVERFLOW_COUNT,
	/** FIFO data count */
	SENSOR_ATTR_MAXM86161_FIFO_DATA_COUNT,
};

/**
 * @brief User configurable interrupts
 *
 * These flags can be combined to enable multiple interrupts simultaneously.
 * Note: Some interrupts are enabled internally and cannot be disabled.
 */
enum maxm86161_interrupt_en {
	/** ALC overflow interrupt enable bit */
	ALC_OVF_INT = 0x20,
	/** LED compliance interrupt enable bit */
	LED_COMPB_INT = 0x08,
	/** Die temperature ready interrupt enable bit */
	DIE_TEMP_RDY_INT = 0x04,
	/** SHA operation done interrupt enable bit */
	SHA_DONE_INT = 0x01,
};

/**
 * @brief Measurement enable/disable states
 */
enum maxm86161_measurement_enable {
	/** Start measurements */
	MEASUREMENT_START = 0x0,
	/** Stop measurements */
	MEASUREMENT_STOP,
};

/**
 * @brief ALC detection methods
 */
enum maxm86161_digital_filter {
	/** Use Current Detection Method (CDM) */
	DIG_FILT_USE_CDM = MAXM86161_DT_DIG_FILTER_CDM,
	/** Use Frequency Detection Method (FDM) */
	DIG_FILT_USE_FDM = MAXM86161_DT_DIG_FILTER_FDM,
};

/**
 * @brief ADC measurement range settings
 *
 * Determines the full-scale range of the ADC in nanoamps.
 */
enum maxm86161_adc_range {
	/** 4.096 uA range */
	ADC_RANGE_4096NA = MAXM86161_DT_4096NA,
	/** 8.192 uA range */
	ADC_RANGE_8192NA = MAXM86161_DT_8192NA,
	/** 16.384 uA range */
	ADC_RANGE_16384NA = MAXM86161_DT_16384NA,
	/** 32.768 uA range */
	ADC_RANGE_32768NA = MAXM86161_DT_32768NA,
};

/**
 * @brief LED integration time settings
 *
 * Controls how long each LED is on during measurement.
 */
enum maxm86161_led_integration_time {
	/** 14.8 us integration time */
	LED_INTEGRATION_TIME_14p8US = MAXM86161_DT_14p8US,
	/** 29.4 us integration time */
	LED_INTEGRATION_TIME_29p4US = MAXM86161_DT_29p4US,
	/** 58.7 us integration time */
	LED_INTEGRATION_TIME_58p7US = MAXM86161_DT_58p7US,
	/** 117.3 us integration time */
	LED_INTEGRATION_TIME_117p3US = MAXM86161_DT_117p3US
};

/**
 * @brief PPG sampling rates
 *
 * Available sampling rates for PPG measurements. Format is [rate]SPS_[pulses]PPS
 * where SPS = Samples Per Second and PPS = Pulses Per Sample.
 */
enum maxm86161_ppg_sampling_rate {
	/** 24.995 SPS, 1 pulse per sample */
	PPG_SR_24p995SPS_1PPS = MAXM86161_DT_24p995SPS_1PPS,
	/** 50.027 SPS, 1 pulse per sample */
	PPG_SR_50p027SPS_1PPS = MAXM86161_DT_50p027SPS_1PPS,
	/** 84.021 SPS, 1 pulse per sample */
	PPG_SR_84p021SPS_1PPS = MAXM86161_DT_84p021SPS_1PPS,
	/** 99.902 SPS, 1 pulse per sample */
	PPG_SR_99p902SPS_1PPS = MAXM86161_DT_99p902SPS_1PPS,
	/** 199.805 SPS, 1 pulse per sample */
	PPG_SR_199p805SPS_1PPS = MAXM86161_DT_199p805SPS_1PPS,
	/** 399.610 SPS, 1 pulse per sample */
	PPG_SR_399p610SPS_1PPS = MAXM86161_DT_399p610SPS_1PPS,
	/** 24.995 SPS, 2 pulses per sample */
	PPG_SR_24p995SPS_2PPS = MAXM86161_DT_24p995SPS_2PPS,
	/** 50.027 SPS, 2 pulses per sample */
	PPG_SR_50p027SPS_2PPS = MAXM86161_DT_50p027SPS_2PPS,
	/** 84.021 SPS, 2 pulses per sample */
	PPG_SR_84p021SPS_2PPS = MAXM86161_DT_84p021SPS_2PPS,
	/** 99.902 SPS, 2 pulses per sample */
	PPG_SR_99p902SPS_2PPS = MAXM86161_DT_99p902SPS_2PPS,
	/** 8 SPS, 1 pulse per sample */
	PPG_SR_8SPS_1PPS = MAXM86161_DT_8SPS_1PPS,
	/** 16 SPS, 1 pulse per sample */
	PPG_SR_16SPS_1PPS = MAXM86161_DT_16SPS_1PPS,
	/** 32 SPS, 1 pulse per sample */
	PPG_SR_32SPS_1PPS = MAXM86161_DT_32SPS_1PPS,
	/** 64 SPS, 1 pulse per sample */
	PPG_SR_64SPS_1PPS = MAXM86161_DT_64SPS_1PPS,
	/** 128 SPS, 1 pulse per sample */
	PPG_SR_128SPS_1PPS = MAXM86161_DT_128SPS_1PPS,
	/** 256 SPS, 1 pulse per sample */
	PPG_SR_256SPS_1PPS = MAXM86161_DT_256SPS_1PPS,
	/** 512 SPS, 1 pulse per sample */
	PPG_SR_512SPS_1PPS = MAXM86161_DT_512SPS_1PPS,
	/** 1024 SPS, 1 pulse per sample */
	PPG_SR_1024SPS_1PPS = MAXM86161_DT_1024SPS_1PPS,
	/** 2048 SPS, 1 pulse per sample */
	PPG_SR_2048SPS_1PPS = MAXM86161_DT_2048SPS_1PPS,
	/** 4096 SPS, 1 pulse per sample */
	PPG_SR_4096SPS_1PPS = MAXM86161_DT_4096SPS_1PPS,
};

/**
 * @brief Sample averaging options
 *
 * Number of samples to average for each measurement.
 */
enum maxm86161_sample_averaging {
	/** No averaging (1 sample) */
	SAMPLE_AVERAGING_1 = MAXM86161_DT_SMP_AVG_1,
	/** Average 2 samples */
	SAMPLE_AVERAGING_2 = MAXM86161_DT_SMP_AVG_2,
	/** Average 4 samples */
	SAMPLE_AVERAGING_4 = MAXM86161_DT_SMP_AVG_4,
	/** Average 8 samples */
	SAMPLE_AVERAGING_8 = MAXM86161_DT_SMP_AVG_8,
	/** Average 16 samples */
	SAMPLE_AVERAGING_16 = MAXM86161_DT_SMP_AVG_16,
	/** Average 32 samples */
	SAMPLE_AVERAGING_32 = MAXM86161_DT_SMP_AVG_32,
	/** Average 64 samples */
	SAMPLE_AVERAGING_64 = MAXM86161_DT_SMP_AVG_64,
	/** Average 128 samples */
	SAMPLE_AVERAGING_128 = MAXM86161_DT_SMP_AVG_128
};

/**
 * @brief LED settling time options
 *
 * Time allowed for LED current to settle before measurement.
 */
enum maxm86161_led_settling_time {
	/** 4 us settling time */
	LED_SETTLING_TIME_4US = MAXM86161_DT_LED_SETTLNG_4US,
	/** 6 us settling time */
	LED_SETTLING_TIME_6US = MAXM86161_DT_LED_SETTLNG_6US,
	/** 8 us settling time */
	LED_SETTLING_TIME_8US = MAXM86161_DT_LED_SETTLNG_8US,
	/** 12 us settling time */
	LED_SETTLING_TIME_12US = MAXM86161_DT_LED_SETTLNG_12US
};

/**
 * @brief Burst mode rates
 *
 * Rate at which burst measurements are performed.
 */
enum maxm86161_burst_rate {
	/** 8 Hz burst rate */
	BURST_RATE_8HZ = MAXM86161_DT_BURST_8HZ,
	/** 32 Hz burst rate */
	BURST_RATE_32HZ = MAXM86161_DT_BURST_32HZ,
	/** 84 Hz burst rate */
	BURST_RATE_84HZ = MAXM86161_DT_BURST_84HZ,
	/** 256 Hz burst rate */
	BURST_RATE_256HZ = MAXM86161_DT_BURST_256HZ,
};

/**
 * @brief Photodiode bias capacitance settings
 *
 * Controls the bias capacitance for the photodiode.
 */
enum maxm86161_pd_bias {
	/** 0-65 pF bias capacitance */
	PD_CAP_BIAS_0PF_65PF = MAXM86161_DT_PD_BIAS_0PF_TO_65PF,
	/** 65-130 pF bias capacitance */
	PD_CAP_BIAS_65PF_130PF = MAXM86161_DT_PD_BIAS_65PF_TO_130PF,
	/** 130-260 pF bias capacitance */
	PD_CAP_BIAS_130PF_260PF = MAXM86161_DT_PD_BIAS_130PF_TO_260PF,
	/** 260-520 pF bias capacitance */
	PD_CAP_BIAS_260PF_520PF = MAXM86161_DT_PD_BIAS_260PF_TO_520PF,
};

/**
 * @brief LED sequence control values
 *
 * Defines which LED or operation is active for each sequence step.
 */
enum maxm86161_ledc_sequence {
	/** No LED active */
	LED_SEQ_NONE = MAXM86161_DT_EXPOSURE_NONE,
	/** Green LED active */
	LED_SEQ_GREEN_LED = MAXM86161_DT_EXPOSURE_GREEN,
	/** Infrared LED active */
	LED_SEQ_IR_LED = MAXM86161_DT_EXPOSURE_IR,
	/** Red LED active */
	LED_SEQ_RED_LED = MAXM86161_DT_EXPOSURE_RED,
	/** Pilot LED on Green LED active */
	LED_SEQ_PILOT_ON_GREEN_LED = MAXM86161_DT_EXPOSURE_PILOT_ON_GREEN,
	/** Direct ambient light measurement */
	LED_SEQ_DIRECT_AMBIENT_LIGHT = MAXM86161_DT_EXPOSURE_DIRECT_AMB_LIGHT,
};

/**
 * @brief LED current range settings
 *
 * Maximum current range for LED drive circuits.
 */
enum maxm86161_led_current_range {
	/** 0-31 mA current range */
	LED_CURRENT_RANGE_31MA = MAXM86161_DT_LED_RANGE_31MA,
	/** 0-62 mA current range */
	LED_CURRENT_RANGE_62MA = MAXM86161_DT_LED_RANGE_62MA,
	/** 0-93 mA current range */
	LED_CURRENT_RANGE_93MA = MAXM86161_DT_LED_RANGE_93MA,
	/** 0-124 mA current range */
	LED_CURRENT_RANGE_124MA = MAXM86161_DT_LED_RANGE_124MA
};

/**
 * @brief Initiate a die tempetature reading.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @return 0 If successful.
 * @return Negative errno code if failure.
 */
int maxm86161_start_die_temp_meas(const struct device *dev);

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_MAXM86161_H_ */
