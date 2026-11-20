/*
 * Copyright (c) 2026, Iomico
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_DFROBOT_SEN0466_SEN0466_H_
#define ZEPHYR_DRIVERS_SENSOR_DFROBOT_SEN0466_SEN0466_H_

#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>

/* Requests and responses are both a nine byte frame. */
#define SEN0466_FRAME_LEN 9U

/* Register the frame is written to and read from. */
#define SEN0466_REG 0x00U

/* Commands, carried at SEN0466_TX_CMD. */
#define SEN0466_CMD_SET_MODE     0x78U
#define SEN0466_CMD_GET_ALL_DATA 0x88U

/* Acquisition modes for SEN0466_CMD_SET_MODE. */
#define SEN0466_MODE_ACTIVE  0x03U
#define SEN0466_MODE_PASSIVE 0x04U

/* Request layout. */
#define SEN0466_TX_HEAD 0U
#define SEN0466_TX_ADDR 1U
#define SEN0466_TX_CMD  2U
#define SEN0466_TX_ARG  3U

/* Response layout. SEN0466_RX_ACK only applies to SEN0466_CMD_SET_MODE. */
#define SEN0466_RX_HEAD     0U
#define SEN0466_RX_CMD      1U
#define SEN0466_RX_ACK      2U
#define SEN0466_RX_CONC_HI  2U
#define SEN0466_RX_CONC_LO  3U
#define SEN0466_RX_GAS_TYPE 4U
#define SEN0466_RX_DECIMALS 5U
#define SEN0466_RX_TEMP_HI  6U
#define SEN0466_RX_TEMP_LO  7U

/* Checksum position, and the bytes it covers. */
#define SEN0466_FRAME_CHECKSUM 8U
#define SEN0466_CHECKSUM_FIRST 1U
#define SEN0466_CHECKSUM_LAST  6U

#define SEN0466_FRAME_HEAD  0xFFU
#define SEN0466_FRAME_ADDR  0x01U
#define SEN0466_GAS_TYPE_CO 0x04U
#define SEN0466_ACK_OK      0x01U

/* Most decimal places the concentration can carry. */
#define SEN0466_DECIMALS_MAX 2U

/* Time the part needs before its answer can be read. */
#define SEN0466_RESPONSE_DELAY_MS 10U

/* Reads to try before giving up on frames the part sends on its own. */
#define SEN0466_MAX_ATTEMPTS 3U

/* Measuring range of the part, in milli-ppm. */
#define SEN0466_CO_MIN_MILLI_PPM 0
#define SEN0466_CO_MAX_MILLI_PPM 1000000

/* Range where the compensation is defined, in milli-degrees. */
#define SEN0466_COMP_MIN_MILLI_C (-20000)
#define SEN0466_COMP_MID_MILLI_C 20000
#define SEN0466_COMP_MAX_MILLI_C 40000

/* Converter readings the NTC table covers. */
#define SEN0466_NTC_ADC_MIN  301U
#define SEN0466_NTC_ADC_MAX  903U
#define SEN0466_NTC_ADC_STEP 16U

struct sen0466_config {
	struct i2c_dt_spec i2c;
};

struct sen0466_data {
	struct k_mutex lock;
	int32_t co_milli_ppm;
	int32_t temp_milli_c;
	bool co_valid;
	bool temp_valid;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_DFROBOT_SEN0466_SEN0466_H_ */
