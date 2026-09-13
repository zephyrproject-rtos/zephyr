/*
 * Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file maxm86161.h
 * @brief Devicetree binding constants for the MAXM86161 sensor
 *
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_ADI_MAXM86161_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_ADI_MAXM86161_H_

/** FIFO Almost Full Type */
#define MAXM86161_DT_AFULL_RPT  0 /**< Assert repeatedly while FIFO is almost full. */
#define MAXM86161_DT_AFULL_ONCE 1 /**< Assert only once when FIFO becomes almost full. */

/** FIFO Mode */
#define MAXM86161_DT_FIFO_MODE_FIFO   0 /**< FIFO mode, stop on full. */
#define MAXM86161_DT_FIFO_MODE_STREAM 1 /**< Stream mode, rollover on full. */

/** PPG ADC Range */
#define MAXM86161_DT_4096NA  0 /**< 7.8125 pA/LSB */
#define MAXM86161_DT_8192NA  1 /**< 15.625 pA/LSB */
#define MAXM86161_DT_16384NA 2 /**< 31.25 pA/LSB */
#define MAXM86161_DT_32768NA 3 /**< 62.5 pA/LSB */

/** PPG Time Integration */
#define MAXM86161_DT_14p8US  0 /**< 14.8 us */
#define MAXM86161_DT_29p4US  1 /**< 29.4 us */
#define MAXM86161_DT_58p7US  2 /**< 58.7 us */
#define MAXM86161_DT_117p3US 3 /**< 117.3 us */

/** PPG Sampling Rate */
#define MAXM86161_DT_24p995SPS_1PPS  0  /**< 24.995 samples/second, 1 pulse/sample */
#define MAXM86161_DT_50p027SPS_1PPS  1  /**< 50.027 samples/second, 1 pulse/sample */
#define MAXM86161_DT_84p021SPS_1PPS  2  /**< 84.021 samples/second, 1 pulse/sample */
#define MAXM86161_DT_99p902SPS_1PPS  3  /**< 99.902 samples/second, 1 pulse/sample */
#define MAXM86161_DT_199p805SPS_1PPS 4  /**< 199.805 sps, 1 pulse */
#define MAXM86161_DT_399p610SPS_1PPS 5  /**< 399.610 sps, 1 pulse */
#define MAXM86161_DT_24p995SPS_2PPS  6  /**< 24.995 sps, 2 pulses */
#define MAXM86161_DT_50p027SPS_2PPS  7  /**< 50.027 sps, 2 pulses */
#define MAXM86161_DT_84p021SPS_2PPS  8  /**< 84.021 sps, 2 pulses */
#define MAXM86161_DT_99p902SPS_2PPS  9  /**< 99.902 sps, 2 pulses */
#define MAXM86161_DT_8SPS_1PPS       10 /**< 8 samples/second, 1 pulse/sample */
#define MAXM86161_DT_16SPS_1PPS      11 /**< 16 samples/second, 1 pulse/sample */
#define MAXM86161_DT_32SPS_1PPS      12 /**< 32 samples/second, 1 pulse/sample */
#define MAXM86161_DT_64SPS_1PPS      13 /**< 64 samples/second, 1 pulse/sample */
#define MAXM86161_DT_128SPS_1PPS     14 /**< 128 samples/second, 1 pulse/sample */
#define MAXM86161_DT_256SPS_1PPS     15 /**< 256 samples/second, 1 pulse/sample */
#define MAXM86161_DT_512SPS_1PPS     16 /**< 512 samples/second, 1 pulse/sample */
#define MAXM86161_DT_1024SPS_1PPS    17 /**< 1024 samples/second, 1 pulse/sample */
#define MAXM86161_DT_2048SPS_1PPS    18 /**< 2048 samples/second, 1 pulse/sample */
#define MAXM86161_DT_4096SPS_1PPS    19 /**< 4096 samples/second, 1 pulse/sample */

/** GPIO Mode */
#define MAXM86161_DT_GPIO_TRI_MUX               0 /**< GPIO Tristate or MUX control */
#define MAXM86161_DT_GPIO_IN_SAMP_TRIG          2 /**< GPIO Input Sample trigger */
#define MAXM86161_DT_GPIO_IN_EXP_TRIG           6 /**< GPIO Input Exposure trigger */
#define MAXM86161_DT_GPIO_IN_HW_SYNC            9 /**< GPIO Input Hardware Sync */
#define MAXM86161_DT_GPIO_IN_SAMP_SYNC_ONESHOT  10 /**< GPIO Input for Single Sample */

/** Sample Averaging */
#define MAXM86161_DT_SMP_AVG_1   0 /**< No averaging (1 sample) */
#define MAXM86161_DT_SMP_AVG_2   1 /**< Average 2 samples */
#define MAXM86161_DT_SMP_AVG_4   2 /**< Average 4 samples */
#define MAXM86161_DT_SMP_AVG_8   3 /**< Average 8 samples */
#define MAXM86161_DT_SMP_AVG_16  4 /**< Average 16 samples */
#define MAXM86161_DT_SMP_AVG_32  5 /**< Average 32 samples */
#define MAXM86161_DT_SMP_AVG_64  6 /**< Average 64 samples */
#define MAXM86161_DT_SMP_AVG_128 7 /**< Average 128 samples */

/** LED Settling Time */
#define MAXM86161_DT_LED_SETTLNG_4US  0 /**< 4.0 us */
#define MAXM86161_DT_LED_SETTLNG_6US  1 /**< 6.0 us */
#define MAXM86161_DT_LED_SETTLNG_8US  2 /**< 8.0 us */
#define MAXM86161_DT_LED_SETTLNG_12US 3 /**< 12.0 us */

/** Digital Filter Select */
#define MAXM86161_DT_DIG_FILTER_CDM 0 /**< Central Difference Method */
#define MAXM86161_DT_DIG_FILTER_FDM 1 /**< Forward Difference Method */

/** Burst Rate */
#define MAXM86161_DT_BURST_8HZ   0 /**< 8 Hertz */
#define MAXM86161_DT_BURST_32HZ  1 /**< 32 Hertz */
#define MAXM86161_DT_BURST_84HZ  2 /**< 84 Hertz */
#define MAXM86161_DT_BURST_256HZ 3 /**< 256 Hertz */

/** Photodiode Biasing Cap */
#define MAXM86161_DT_PD_BIAS_0PF_TO_65PF    1 /**< 0 pF to 65 pF */
#define MAXM86161_DT_PD_BIAS_65PF_TO_130PF  5 /**< 65 pF to 130 pF */
#define MAXM86161_DT_PD_BIAS_130PF_TO_260PF 6 /**< 130 pF to 260 pF */
#define MAXM86161_DT_PD_BIAS_260PF_TO_520PF 7 /**< 260 pF to 520 pF */

/** LED Current Range */
#define MAXM86161_DT_LED_RANGE_31MA  0 /**< 31 mA */
#define MAXM86161_DT_LED_RANGE_62MA  1 /**< 62 mA */
#define MAXM86161_DT_LED_RANGE_93MA  2 /**< 93 mA */
#define MAXM86161_DT_LED_RANGE_124MA 3 /**< 124 mA */

/** LED Sequence Exposure Type */
#define MAXM86161_DT_EXPOSURE_NONE             0 /**< Stops the sequence */
#define MAXM86161_DT_EXPOSURE_GREEN            1 /**< LED1, Green LED */
#define MAXM86161_DT_EXPOSURE_IR               2 /**< LED2, IR LED */
#define MAXM86161_DT_EXPOSURE_RED              3 /**< LED3, Red LED */
#define MAXM86161_DT_EXPOSURE_PILOT_ON_GREEN   8 /**< Pilot on Green LED */
#define MAXM86161_DT_EXPOSURE_DIRECT_AMB_LIGHT 9 /**< Direct Ambient Light */

/** Picket Fence Threshold Sigma */
#define MAXM86161_DT_PF_THRESH_SIGMA_4      0 /**< Gain = 4 */
#define MAXM86161_DT_PF_THRESH_SIGMA_8      1 /**< Gain = 8 */
#define MAXM86161_DT_PF_THRESH_SIGMA_16     2 /**< Gain = 16 */
#define MAXM86161_DT_PF_THRESH_SIGMA_32     3 /**< Gain = 32 */

/** Picket Fence IIR Filter Initial Value */
#define MAXM86161_DT_PF_IIR_INIT_VAL_64     0 /**< Initialize with value 64 */
#define MAXM86161_DT_PF_IIR_INIT_VAL_48     1 /**< Initialize with value 48 */
#define MAXM86161_DT_PF_IIR_INIT_VAL_32     2 /**< Initialize with value 32 */
#define MAXM86161_DT_PF_IIR_INIT_VAL_24     3 /**< Initialize with value 24 */

/** Picket Fence IIR Filter Time Constant */
#define MAXM86161_DT_PF_IIR_TC_64   0 /**< TC = 1 sample */
#define MAXM86161_DT_PF_IIR_TC_32   1 /**< TC = 2 samples */
#define MAXM86161_DT_PF_IIR_TC_16   2 /**< TC = 4 samples */
#define MAXM86161_DT_PF_IIR_TC_8    3 /**< TC = 8 samples */

/** Picket Fence Order */
#define MAXM86161_DT_PF_ORDER_LAST_SAMPLE   0 /**< Last sample (1 point) */
/** Fit 4 points to a line for prediction (default) */
#define MAXM86161_DT_PF_ORDER_FIT_4_PTS     1

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_ADI_MAXM86161_H_ */
