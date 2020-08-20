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

/*
 * Get voltage level for CPU to run at 240 MHz, or for flash/PSRAM to run at 80 MHz.
 * 0x0: level 7; 0x1: level 6; 0x2: level 5; 0x3: level 4. (RO)
 */
#define RTC_CNTL_DBIAS_HP_VOLT          (RTC_CNTL_DBIAS_1V25 - (REG_GET_FIELD(EFUSE_BLK0_RDATA5_REG, EFUSE_RD_VOL_LEVEL_HP_INV)))
#ifdef CONFIG_ESPTOOLPY_FLASHFREQ_80M
#define DIG_DBIAS_80M_160M  RTC_CNTL_DBIAS_HP_VOLT
#else
#define DIG_DBIAS_80M_160M  RTC_CNTL_DBIAS_1V10
#endif
#define DIG_DBIAS_240M      RTC_CNTL_DBIAS_HP_VOLT
#define DIG_DBIAS_XTAL      RTC_CNTL_DBIAS_1V10
#define DIG_DBIAS_2M        RTC_CNTL_DBIAS_1V00

#define DELAY_PLL_DBIAS_RAISE   3

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

extern uint32_t esp32_rom_g_ticks_per_us_pro;
extern uint32_t esp32_rom_g_ticks_per_us_app;
extern void esp32_rom_ets_delay_us(uint32_t us);

#endif /* ZEPHYR_DRIVERS_CLOCK_CONTROL_ESP32_CLOCK_H_ */
