/*
 * Copyright (c) 2018, Sensirion AG
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of Sensirion AG nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * SPDX-License-Identifier: BSD-3-CLAUSE
 */

#ifndef ZEPHYR_DRIVER_SENSOR_SCD30_SCD30_H_
#define ZEPHYR_DRIVER_SENSOR_SCD30_SCD30_H_

#include <device.h>
#include <kernel.h>
#include <drivers/gpio.h>

#define SCD30_CMD_START_PERIODIC_MEASUREMENT 0x0010
#define SCD30_CMD_STOP_PERIODIC_MEASUREMENT 0x0104
#define SCD30_CMD_READ_MEASUREMENT 0x0300
#define SCD30_CMD_SET_MEASUREMENT_INTERVAL 0x4600
#define SCD30_CMD_GET_DATA_READY 0x0202
#define SCD30_CMD_SET_TEMPERATURE_OFFSET 0x5403
#define SCD30_CMD_SET_ALTITUDE 0x5102
#define SCD30_CMD_SET_FORCED_RECALIBRATION 0x5204
#define SCD30_CMD_AUTO_SELF_CALIBRATION 0x5306
#define SCD30_CMD_READ_SERIAL 0xD033
#define SCD30_SERIAL_NUM_WORDS 16U
#define SCD30_WRITE_DELAY_US 20000U

#define SCD30_MEASUREMENT_DATA_WORDS 6U
#define SCD30_MEASUREMENT_DEF_AMBIENT_PRESSURE 0x0000

#define SCD30_CRC8_POLYNOMIAL 0x31
#define SCD30_CRC8_INIT 0xFF
#define SCD30_CRC8_LEN 1U

#define SCD30_WORD_SIZE 2U
#define SCD30_COMMAND_SIZE 2U

#define SCD30_MIN_SAMPLE_TIME 2U
#define SCD30_MAX_SAMPLE_TIME 1800U

#define SCD30_MAX_BUFFER_WORDS 24U
#define SCD30_CMD_SINGLE_WORD_BUF_LEN (SCD30_COMMAND_SIZE + SCD30_WORD_SIZE + SCD30_CRC8_LEN)

struct scd30_config {
	const struct i2c_dt_spec bus;
};

struct scd30_data {
	uint16_t error;
	uint16_t sample_time;
	char serial[SCD30_SERIAL_NUM_WORDS + 1];
	float co2_ppm;
	float temp;
	float rel_hum;
};

struct scd30_word {
	uint8_t word[SCD30_WORD_SIZE];
	uint8_t crc;
};

#endif /* ZEPHYR_DRIVER_SENSOR_SCD30_SCD30_H_ */
