/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_HTU31D_HTU31D_H_
#define ZEPHYR_DRIVERS_SENSOR_HTU31D_HTU31D_H_

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>

/* I2C commands (HTU31D datasheet) */
#define HTU31D_CMD_RESET       0x1EU
#define HTU31D_CMD_READ_T_RH   0x00U
#define HTU31D_CMD_CONV_BASE   0x40U
#define HTU31D_CMD_HEATER_ON   0x04U
#define HTU31D_CMD_HEATER_OFF  0x02U
#define HTU31D_CMD_READ_DIAG   0x08U
#define HTU31D_CMD_READ_SERIAL 0x0AU

/* Response sizes including trailing CRC byte */
#define HTU31D_DIAG_BUF_LEN   2U
#define HTU31D_SERIAL_BUF_LEN 4U

/*
 * Conversion command byte format (datasheet Section 3.1):
 *   bit 7: 0, bit 6: 1, bit 5: 0
 *   bits 4:3 = humidity OSR (0-3)
 *   bits 2:1 = temperature OSR (0-3)
 *   bit 0: 0
 *
 * Final command: HTU31D_CMD_CONV_BASE + (rh_osr << 3) + (t_osr << 1)
 */

/* OSR bit field positions in conversion command byte */
#define HTU31D_OSR_T_SHIFT  1U
#define HTU31D_OSR_RH_SHIFT 3U

/* CRC-8: polynomial x^8 + x^5 + x^4 + 1, init 0, non-reflected */
#define HTU31D_CRC_POLY     0x31U
#define HTU31D_CRC_INIT     0x00U
#define HTU31D_CRC_DATA_LEN 2U

/*
 * Read T+RH response layout:
 *   [0..2]: temp_msb, temp_lsb, temp_crc
 *   [3..5]: humi_msb, humi_lsb, humi_crc
 */
#define HTU31D_WORD_SIZE    3U
#define HTU31D_T_RH_WORDS   2U
#define HTU31D_T_RH_BUF_LEN (HTU31D_T_RH_WORDS * HTU31D_WORD_SIZE)

/* Reset recovery time per datasheet: 15 ms max */
#define HTU31D_RESET_TIME_MS 15U

/*
 * Conversion delay per OSR level in milliseconds.
 *
 * Datasheet Table 3 lists per-channel maxima of (1.57, 3.06, 6.03, 11.98) ms
 * for temperature and (1.11, 2.14, 4.21, 8.34) ms for humidity; both run in
 * parallel, so temperature is the limiting factor. Values here include a
 * margin over the datasheet maximum to cover k_msleep tick rounding, silicon
 * tolerance, and avoid the NACK that the sensor issues when the read command
 * is sent before the conversion has finished (datasheet Section 3.7).
 */
#define HTU31D_CONV_TIME_OSR_0 4U
#define HTU31D_CONV_TIME_OSR_1 8U
#define HTU31D_CONV_TIME_OSR_2 14U
#define HTU31D_CONV_TIME_OSR_3 25U

/*
 * Datasheet conversion formulas, expressed in micro-units for use with
 * sensor_value_from_micro():
 *   T[°C] = -40 + 165 * raw / 65535
 *   RH[%] = 100 * raw / 65535
 */
#define HTU31D_TEMP_OFFSET_MICRO (-40000000LL)
#define HTU31D_TEMP_SCALE_MICRO  165000000LL
#define HTU31D_HUMI_SCALE_MICRO  100000000LL
#define HTU31D_ADC_DIVISOR       UINT16_MAX

struct htu31d_config {
	struct i2c_dt_spec bus;
	uint8_t osr_temp;
	uint8_t osr_humidity;
};

struct htu31d_data {
	uint16_t temp_sample;
	uint16_t humi_sample;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_HTU31D_HTU31D_H_ */
