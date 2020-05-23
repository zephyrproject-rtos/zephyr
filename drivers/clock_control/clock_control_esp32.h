/*
 * Copyright (c) 2020 Mohamed ElShahawi.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_CLOCK_CONTROL_ESP32_CLOCK_H_
#define ZEPHYR_DRIVERS_CLOCK_CONTROL_ESP32_CLOCK_H_

#include <soc/efuse_reg.h>

/*
 * Convenience macros for the above functions.
 */
#define I2C_WRITEREG_RTC(block, reg_add, indata) \
	esp32_rom_i2c_writeReg(block, block##_HOSTID,  reg_add, indata)

#define I2C_READREG_RTC(block, reg_add)	\
	esp32_rom_i2c_readReg(block, block##_HOSTID,  reg_add)


# define XTHAL_GET_CCOUNT()     ({ int __ccount;					     \
				   __asm__ __volatile__ ("rsr.ccount %0" : "=a" (__ccount)); \
				   __ccount; })

# define XTHAL_SET_CCOUNT(v)    do { int __ccount = (int)(v);						   \
				     __asm__ __volatile__ ("wsr.ccount %0" : : "a" (__ccount) : "memory"); \
				} while (0)

/*
 * Get voltage level for CPU to run at 240 MHz, or for flash/PSRAM to run at 80 MHz.
 * 0x0: level 7; 0x1: level 6; 0x2: level 5; 0x3: level 4. (RO)
 */
#define GET_HP_VOLTAGE          (RTC_CNTL_DBIAS_1V25 - ((EFUSE_BLK0_RDATA5_REG >> 22) & 0x3))
#define DIG_DBIAS_240M          GET_HP_VOLTAGE
#define DIG_DBIAS_80M_160M      RTC_CNTL_DBIAS_1V10     /* FIXME: This macro should be GET_HP_VOLTAGE in case of 80Mhz flash frequency */
#define DIG_DBIAS_XTAL          RTC_CNTL_DBIAS_1V10

/**
 * Register definitions for digital PLL (BBPLL)
 * This file lists register fields of BBPLL, located on an internal configuration
 * bus.
 */
#define I2C_BBPLL                0x66
#define I2C_BBPLL_HOSTID         4
#define I2C_BBPLL_IR_CAL_DELAY   0
#define I2C_BBPLL_IR_CAL_EXT_CAP 1
#define I2C_BBPLL_OC_LREF        2
#define I2C_BBPLL_OC_DIV_7_0     3
#define I2C_BBPLL_OC_ENB_FCAL    4
#define I2C_BBPLL_OC_DCUR        5
#define I2C_BBPLL_BBADC_DSMP     9
#define I2C_BBPLL_OC_ENB_VCON    10
#define I2C_BBPLL_ENDIV5         11
#define I2C_BBPLL_BBADC_CAL_7_0  12

/* BBPLL configuration values */
#define BBPLL_ENDIV5_VAL_320M       0x43
#define BBPLL_BBADC_DSMP_VAL_320M   0x84
#define BBPLL_ENDIV5_VAL_480M       0xc3
#define BBPLL_BBADC_DSMP_VAL_480M   0x74
#define BBPLL_IR_CAL_DELAY_VAL      0x18
#define BBPLL_IR_CAL_EXT_CAP_VAL    0x20
#define BBPLL_OC_ENB_FCAL_VAL       0x9a
#define BBPLL_OC_ENB_VCON_VAL       0x00
#define BBPLL_BBADC_CAL_7_0_VAL     0x00

#endif /* ZEPHYR_DRIVERS_CLOCK_CONTROL_ESP32_CLOCK_H_ */
