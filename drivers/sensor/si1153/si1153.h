/*
 * Copyright (c) 2017 IpTronix S.r.l.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SENSOR_SI1153_H__
#define __SENSOR_SI1153_H__

#include <stdint.h>
#include <device.h>


/**
 * I2C Registers
 */
#define SI115x_REG_PART_ID      0x00
#define SI115x_REG_REV_ID       0x01
#define SI115x_REG_MFR_ID       0x02
#define SI115x_REG_INFO0        0x03
#define SI115x_REG_INFO1        0x04
#define SI115x_REG_HOSTIN3      0x07
#define SI115x_REG_HOSTIN2      0x08
#define SI115x_REG_HOSTIN1      0x09
#define SI115x_REG_HOSTIN0      0x0A
#define SI115x_REG_COMMAND      0x0B
#define SI115x_REG_IRQ_ENABLE   0x0F
#define SI115x_REG_RESPONSE1    0x10
#define SI115x_REG_RESPONSE0    0x11
#define SI115x_REG_IRQ_STATUS   0x12
#define SI115x_REG_HOSTOUT0     0x13
#define SI115x_REG_HOSTOUT1     0x14
#define SI115x_REG_HOSTOUT2     0x15
#define SI115x_REG_HOSTOUT3     0x16
#define SI115x_REG_HOSTOUT4     0x17
#define SI115x_REG_HOSTOUT5     0x18
#define SI115x_REG_HOSTOUT6     0x19
#define SI115x_REG_HOSTOUT7     0x1A
#define SI115x_REG_HOSTOUT8     0x1B
#define SI115x_REG_HOSTOUT9     0x1C
#define SI115x_REG_HOSTOUT10    0x1D
#define SI115x_REG_HOSTOUT11    0x1E
#define SI115x_REG_HOSTOUT12    0x1F
#define SI115x_REG_HOSTOUT13    0x20
#define SI115x_REG_HOSTOUT14    0x21
#define SI115x_REG_HOSTOUT15    0x22
#define SI115x_REG_HOSTOUT16    0x23
#define SI115x_REG_HOSTOUT17    0x24
#define SI115x_REG_HOSTOUT18    0x25
#define SI115x_REG_HOSTOUT19    0x26
#define SI115x_REG_HOSTOUT20    0x27
#define SI115x_REG_HOSTOUT21    0x28
#define SI115x_REG_HOSTOUT22    0x29
#define SI115x_REG_HOSTOUT23    0x2A
#define SI115x_REG_HOSTOUT24    0x2B
#define SI115x_REG_HOSTOUT25    0x2C
#define SI115x_REG_OTP_CONTROL  0x2F
#define SI115x_REG_CHIP_STAT    0x30

/*  Si115x I2C Parameter Offsets */
#define PARAM_I2C_ADDR          0x00
#define PARAM_CH_LIST           0x01
#define PARAM_ADCCONFIG0        0x02
#define PARAM_ADCSENS0          0x03
#define PARAM_ADCPOST0          0x04
#define PARAM_MEASCONFIG0       0x05
#define PARAM_ADCCONFIG1        0x06
#define PARAM_ADCSENS1          0x07
#define PARAM_ADCPOST1          0x08
#define PARAM_MEASCONFIG1       0x09
#define PARAM_ADCCONFIG2        0x0A
#define PARAM_ADCSENS2          0x0B
#define PARAM_ADCPOST2          0x0C
#define PARAM_MEASCONFIG2       0x0D
#define PARAM_ADCCONFIG3        0x0E
#define PARAM_ADCSENS3          0x0F
#define PARAM_ADCPOST3          0x10
#define PARAM_MEASCONFIG3       0x11
#define PARAM_ADCCONFIG4        0x12
#define PARAM_ADCSENS4          0x13
#define PARAM_ADCPOST4          0x14
#define PARAM_MEASCONFIG4       0x15
#define PARAM_ADCCONFIG5        0x16
#define PARAM_ADCSENS5          0x17
#define PARAM_ADCPOST5          0x18
#define PARAM_MEASCONFIG5       0x19
#define PARAM_MEASRATE_H        0x1A
#define PARAM_MEASRATE_L        0x1B
#define PARAM_MEASCOUNT0        0x1C
#define PARAM_MEASCOUNT1        0x1D
#define PARAM_MEASCOUNT2        0x1E
#define PARAM_LED1_A            0x1F
#define PARAM_LED1_B            0x20
#define PARAM_LED2_A            0x21
#define PARAM_LED2_B            0x22
#define PARAM_LED3_A            0x23
#define PARAM_LED3_B            0x24
#define PARAM_THRESHOLD0_H      0x25
#define PARAM_THRESHOLD0_L      0x26
#define PARAM_THRESHOLD1_H      0x27
#define PARAM_THRESHOLD1_L      0x28
#define PARAM_THRESHOLD2_H      0x29
#define PARAM_THRESHOLD2_L      0x2A
#define PARAM_BURST             0x2B

#define CMD_NOP                 0x00
#define CMD_RESET               0x01
#define CMD_NEW_ADDR            0x02
#define CMD_FORCE_CH            0x11
#define CMD_PAUSE_CH            0x12
#define CMD_AUTO_CH             0x13
#define CMD_PARAM_SET           0x80
#define CMD_PARAM_QUERY         0x40

/*
 *
 *    Si115x Register and Parameter Bit Definitions
 */
#define RSP0_CHIPSTAT_MASK      0xe0
#define RSP0_COUNTER_MASK       0x1f
#define RSP0_SLEEP              0x20



/**
 * ADCCONFIGx
 * bit  description
 *   7  reserved
 * 6.5  DECIM_RATE[1:0] No of 21 MHz Clocks
 * 4.0  ADCMUX[4:0]     Selects which photodiode(s) are
 *                      connected to the ADCs for measurement.
 */
#define ADCCFG_DR_1024 0x00
#define ADCCFG_DR_2048 0x20
#define ADCCFG_DR_4096 0x40
#define ADCCFG_DR_512  0x60

#define ADCCFG_AM_SMALL_IR     0x00
#define ADCCFG_AM_MEDIUM_IR    0x01
#define ADCCFG_AM_LARGE_IR     0x02
#define ADCCFG_AM_WHITE        0x0B
#define ADCCFG_AM_LARGE_WHITE  0x0D

/**
 * ADCSENSx
 * Parameter Addresses: 0x03, 0x07, 0x0B, 0x0F, 0x13, 0x17
 * Bit       7      6       5       4       3       2       1       0
 * Name  HSIG  SW_GAIN[2:0]  HW_GAIN[2:0]
 * Reset  0  0  0  0  0  0  0  0  *
 */

/**
 * This is the Ranging bit for the A/D.
 * Normal gain at 0 and
 * High range (sensitivity is divided by 14.5) when set to 1.
 */
#define ADCSENS_HSIG_NORM  0x00
#define ADCSENS_HSIG_HIGH  0x80

/**
 * Causes an internal accumulation of samples with no pause between readings
 * when in FORCED Mode.
 * In Autonomous mode the the accumulation happens at the measurement
 * rate selected.
 * The calculations are accumulated in 24 bits and an optional shift is
 * applied later. See ADC-POSTx.ADC_MISC[1:0]
 */
#define ADCSENS_SW_GAIN_1MEAS     0x00
#define ADCSENS_SW_GAIN_2MEAS     0x10
#define ADCSENS_SW_GAIN_4MEAS     0x20
#define ADCSENS_SW_GAIN_8MEAS     0x30
#define ADCSENS_SW_GAIN_16MEAS    0x40
#define ADCSENS_SW_GAIN_32MEAS    0x50
#define ADCSENS_SW_GAIN_64MEAS    0x60
#define ADCSENS_SW_GAIN_128MEAS   0x70

/**
 * HW_GAIN[3:0]
 * Nominal Measurement time for 512 clocks
 */
#define ADCSENS_HW_GAIN_24_4US        0x00
#define ADCSENS_HW_GAIN_48_8US        0x01
#define ADCSENS_HW_GAIN_97_5US        0x02
#define ADCSENS_HW_GAIN_195US         0x03
#define ADCSENS_HW_GAIN_390US         0x04
#define ADCSENS_HW_GAIN_780US         0x05
#define ADCSENS_HW_GAIN_1_56MS        0x06
#define ADCSENS_HW_GAIN_3_12MS        0x07
#define ADCSENS_HW_GAIN_6_24MS        0x08
#define ADCSENS_HW_GAIN_12_5MS        0x09
#define ADCSENS_HW_GAIN_25MS          0x0A
#define ADCSENS_HW_GAIN_50MS          0x0B

/**
 * ADCPOSTx
 * Parameter Addresses: 0x04, 0x08, 0x0C, 0x10, 0x14, 0x18
 * Bit    7     6     5     4     3     2     1     0
 * Name   Reserved      24BIT_OUT  POSTSHIFT[2:0]  UNUSED  THRESH_EN[1:0]
 * Reset 0 0 0 0 0 0 0 0
 *
 */
#define ADCPOST_16BIT 0x00
#define ADCPOST_24BIT 0x40

/**
 * The number of bits to shift right after SW accumulation.
 * Allows the results of many additions not to overflow the output.
 * Especially useful when the output is in 16 bit mode.
 */
#define ADCPOST_POSTSHIFT_0  0x00
#define ADCPOST_POSTSHIFT_1  0x80
#define ADCPOST_POSTSHIFT_2  0x10
#define ADCPOST_POSTSHIFT_3  0x18
#define ADCPOST_POSTSHIFT_4  0x20
#define ADCPOST_POSTSHIFT_5  0x28
#define ADCPOST_POSTSHIFT_6  0x30
#define ADCPOST_POSTSHIFT_7  0x38

/* Do not use THRESHOLDs */
#define ADCPOST_THRESH_EN_0 0x00
/* Interrupt when the measurement is larger than the
 * THRESHOLD0 Global Parameters
 */
#define ADCPOST_THRESH_EN_1 0x01
/* Interrupt when the measurement is larger than the
 * THRESHOLD1 Global Parameters
 */
#define ADCPOST_THRESH_EN_2 0x02
/* Interrupt when the measurement is larger than the
 * THRESHOLD2 Global Parameters
 */
#define ADCPOST_THRESH_EN_3 0x03


/**
 * MEASCONFIGx
 * Parameter Addresses: 0x05, 0x0A, 0x0D, 0x11, 0x15, 0x19
 * Bit     7          6         5       4       3          2         1         0
 * Name    COUNTER_INDEX[1:0]   LED_TRIM[1:0]   BANK_SEL   LED2_EN   LED3_EN   LED1_EN
 * Reset   0          0         0       0       0          0         0         0
 *
 */
#define MEASCFG_NO_MEAS     0x00
#define MEASCFG_MEASCOUNT0  0x40
#define MEASCFG_MEASCOUNT1  0x80
#define MEASCFG_MEASCOUNT2  0xC0

#define MEASCFG_LED_NOM   0x00  /* Nominal LED Currents */
#define MEASCFG_LED_P9    0x20  /* LED currents increased by 9% */
#define MEASCFG_LED_M10   0x30  /* LED currents decreased by 10% */

/* LED Current Registers Selected in Global Register Area */
#define MEASCFG_BANK_SEL_A  0x00        /* LED1_A, LED2_A, LED3_A */
#define MEASCFG_BANK_SEL_B  0x08        /* LED1_B, LED2_B, LED3_B */

#define MEASCFG_LED2_ENA  0x04
#define MEASCFG_LED3_ENA  0x02
#define MEASCFG_LED1_ENA  0x01

/**
 * Led current definition
 */
#define LED_CURRENT_5_5     0x00
#define LED_CURRENT_11      0x08
#define LED_CURRENT_17      0x10
#define LED_CURRENT_22      0x18
#define LED_CURRENT_28      0x20
#define LED_CURRENT_33      0x28
#define LED_CURRENT_39      0x30
#define LED_CURRENT_44      0x38
#define LED_CURRENT_50      0x12
#define LED_CURRENT_55      0x21
#define LED_CURRENT_66      0x29
#define LED_CURRENT_77      0x31
#define LED_CURRENT_83      0x22
#define LED_CURRENT_88      0x39
#define LED_CURRENT_100     0x2A
#define LED_CURRENT_111     0x23
#define LED_CURRENT_116     0x32
#define LED_CURRENT_133     0x3A
#define LED_CURRENT_138     0x24
#define LED_CURRENT_155     0x33
#define LED_CURRENT_166     0x2C
#define LED_CURRENT_177     0x3B
#define LED_CURRENT_194     0x34
#define LED_CURRENT_199     0x2D
#define LED_CURRENT_221     0x3C
#define LED_CURRENT_232     0x35
#define LED_CURRENT_265     0x3D
#define LED_CURRENT_271     0x36
#define LED_CURRENT_310     0x3E
#define LED_CURRENT_354     0x3F


struct si1153_data {
	struct device *i2c_master;
	u16_t i2c_slave_addr;
#ifdef CONFIG_SI1153_INTERRUPT
	const char    *gpio_port;
	u8_t int_pin;
#endif

#if defined(CONFIG_SI1153_OWN_THREAD)
	K_THREAD_STACK_MEMBER(thread_stack, CONFIG_SI1153_THREAD_STACK_SIZE);
#endif

	s32_t ch0;
	s32_t ch1;
	s32_t ch2;
	s32_t ch3;
	u8_t gesture;
};



#define SYS_LOG_DOMAIN "SI1153"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_SENSOR_LEVEL
#include <logging/sys_log.h>
#endif /* __SENSOR_SI1153_H__ */
