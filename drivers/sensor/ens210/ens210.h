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
	u16_t val;
	u8_t  valid : 1;
	u8_t  crc7  : 7;
} __packed;

struct ens210_sys_ctrl {
	u8_t  low_power : 1;
	u8_t  reserved  : 6;
	u8_t  reset     : 1;
} __packed;

struct ens210_sys_stat {
	u8_t  sys_active : 1;
	u8_t  reserved   : 7;
} __packed;

struct ens210_sens_run {
	u8_t  t_run    : 1;
	u8_t  h_run    : 1;
	u8_t  reserved : 6;
} __packed;

struct ens210_sens_start {
	u8_t  t_start  : 1;
	u8_t  h_start  : 1;
	u8_t  reserved : 6;
} __packed;

struct ens210_sens_stop {
	u8_t  t_stop  : 1;
	u8_t  h_stop  : 1;
	u8_t  reserved : 6;
} __packed;

struct ens210_sens_stat {
	u8_t  t_stat  : 1;
	u8_t  h_stat  : 1;
	u8_t  reserved : 6;
} __packed;

struct ens210_data {
	struct device *i2c;
	struct ens210_value_data temp;
	struct ens210_value_data humidity;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_ENS210_ENS210_H_ */
