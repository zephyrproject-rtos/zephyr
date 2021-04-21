/*
 * Copyright (c) 2018 Alexander Wachter.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ENS210_ENS210_H_
#define ZEPHYR_DRIVERS_SENSOR_ENS210_ENS210_H_

#include <device.h>
#include <drivers/gpio.h>
#include <sys/util.h>

/* Registers */
#define ENS210_REG_PART_ID    0x00
#define ENS210_REG_UID        0x04
#define ENS210_REG_SYS_CTRL   0x10
#define ENS210_REG_SYS_STAT   0x11
#define ENS210_REG_SENS_RUN   0x21
#define ENS210_REG_SENS_START 0x22
#define ENS210_REG_SENS_STOP  0x23
#define ENS210_REG_SENS_STAT  0x24
#define ENS210_REG_T_VAL      0x30
#define ENS210_REG_H_VAL      0x33

#define ENS210_PART_ID 0x0210

#if defined CONFIG_ENS210_TEMPERATURE_OFF
#define ENS210_T_RUN   0
#define ENS210_T_START 0
#elif defined CONFIG_ENS210_TEMPERATURE_SINGLE
#define ENS210_T_RUN   0
#define ENS210_T_START 1
#elif defined CONFIG_ENS210_TEMPERATURE_CONTINUOUS
#define ENS210_T_RUN   1
#define ENS210_T_START 1
#endif

#if defined CONFIG_ENS210_HUMIDITY_OFF
#define ENS210_H_RUN   0
#define ENS210_H_START 0
#elif defined CONFIG_ENS210_HUMIDITY_SINGLE
#define ENS210_H_RUN   0
#define ENS210_H_START 1
#elif defined CONFIG_ENS210_HUMIDITY_CONTINUOUS
#define ENS210_H_RUN   1
#define ENS210_H_START 1
#endif

/*
 * Polynomial
 * 0b 1000 1001 ~ x^7+x^3+x^0
 * 0x    8    9
 */
#define ENS210_CRC7_WIDTH      7
#define ENS210_CRC7_POLY       0x89
#define ENS210_CRC7_IVEC       ((1UL << ENS210_CRC7_WIDTH) - 1)
#define ENS210_CRC7_DATA_WIDTH 17
#define ENS210_CRC7_DATA_MASK  ((1UL << ENS210_CRC7_DATA_WIDTH) - 1)
#define ENS210_CRC7_DATA_MSB   (1UL << (ENS210_CRC7_DATA_WIDTH - 1))

struct ens210_value_data {
	uint16_t val;
	uint8_t  valid : 1;
	uint8_t  crc7  : 7;
} __packed;

struct ens210_sys_ctrl {
	uint8_t  low_power : 1;
	uint8_t  reserved  : 6;
	uint8_t  reset     : 1;
} __packed;

struct ens210_sys_stat {
	uint8_t  sys_active : 1;
	uint8_t  reserved   : 7;
} __packed;

struct ens210_sens_run {
	uint8_t  t_run    : 1;
	uint8_t  h_run    : 1;
	uint8_t  reserved : 6;
} __packed;

struct ens210_sens_start {
	uint8_t  t_start  : 1;
	uint8_t  h_start  : 1;
	uint8_t  reserved : 6;
} __packed;

struct ens210_sens_stop {
	uint8_t  t_stop  : 1;
	uint8_t  h_stop  : 1;
	uint8_t  reserved : 6;
} __packed;

struct ens210_sens_stat {
	uint8_t  t_stat  : 1;
	uint8_t  h_stat  : 1;
	uint8_t  reserved : 6;
} __packed;

struct ens210_data {
	const struct device *i2c;
	struct ens210_value_data temp;
	struct ens210_value_data humidity;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_ENS210_ENS210_H_ */
