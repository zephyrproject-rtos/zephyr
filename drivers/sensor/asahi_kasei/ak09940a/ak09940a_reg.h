/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ASAHI_KASEI_AK09940A_AK09940A_REG_H_
#define ZEPHYR_DRIVERS_SENSOR_ASAHI_KASEI_AK09940A_AK09940A_REG_H_

#include <zephyr/sys/util_macro.h>

#include "ak099xx.h"

#define AK09940A_REG_WIA1   0x00
#define AK09940A_REG_WIA2   0x01
#define AK09940A_REG_RSV2   0x03
#define AK09940A_REG_ST     0x0F
#define AK09940A_REG_ST1    0x10
#define AK09940A_REG_HXL    0x11
#define AK09940A_REG_HYL    0x14
#define AK09940A_REG_HZL    0x17
#define AK09940A_REG_TMPS   0x1A
#define AK09940A_REG_ST2    0x1B
#define AK09940A_REG_CNTL1  0x30
#define AK09940A_REG_CNTL2  0x31
#define AK09940A_REG_CNTL3  0x32
#define AK09940A_REG_CNTL4  0x33
#define AK09940A_REG_I2CDIS 0x36

#define AK09940A_WIA2 0xA3

/* ST2 values */
#define AK09940A_ST2_DOR BIT(0)
#define AK09940A_ST2_INV BIT(1)

/* CNTL1 values */
#define AK09940A_CNTL1_MT2 BIT(7)

/* CNTL2 values */
#define AK09940A_CNTL2_TEM BIT(6)

/* CNTL3 values */
#define AK09940A_CNTL3_MODE GENMASK(4, 0)
#define AK09940A_CNTL3_MT   GENMASK(6, 5)
#define AK09940A_CNTL3_FIFO BIT(7)

/* CNTL4 values */
#define AK09940A_CNTL4_SRST BIT(0)

/* I2CDIS value that disables the I2C interface */
#define AK09940A_I2CDIS_DISABLE 0x1B

/* Continuous measurement modes on top of the common AK099xx ones */
#define AK09940A_MODE_CONT_200HZ  0x0A
#define AK09940A_MODE_CONT_400HZ  0x0C
#define AK09940A_MODE_CONT_1000HZ 0x0E
#define AK09940A_MODE_CONT_2500HZ 0x0F

/* SPI read/write bit of the address byte */
#define AK09940A_SPI_READ BIT(7)

#endif /* ZEPHYR_DRIVERS_SENSOR_ASAHI_KASEI_AK09940A_AK09940A_REG_H_ */
