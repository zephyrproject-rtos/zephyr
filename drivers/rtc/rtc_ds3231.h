/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2024 Gergo Vari <work@gergovari.com>
 */

/*
 * REGISTERS
 */

/* Time registers */
#define DS3231_REG_TIME_SECONDS     0x00
#define DS3231_REG_TIME_MINUTES     0x01
#define DS3231_REG_TIME_HOURS       0x02
#define DS3231_REG_TIME_DAY_OF_WEEK 0x03
#define DS3231_REG_TIME_DATE        0x04
#define DS3231_REG_TIME_MONTH       0x05
#define DS3231_REG_TIME_YEAR        0x06

/* Alarm 1 registers */
#define DS3231_REG_ALARM_1_SECONDS 0x07
#define DS3231_REG_ALARM_1_MINUTES 0x08
#define DS3231_REG_ALARM_1_HOURS   0x09
#define DS3231_REG_ALARM_1_DATE    0x0A

/* Alarm 2 registers */
/* Alarm 2 has no seconds to set, it only has minute accuracy. */
#define DS3231_REG_ALARM_2_MINUTES 0x0B
#define DS3231_REG_ALARM_2_HOURS   0x0C
#define DS3231_REG_ALARM_2_DATE    0x0D

/* Control registers */
#define DS3231_REG_CTRL     0x0E
#define DS3231_REG_CTRL_STS 0x0F

/* Aging offset register */
#define DS3231_REG_AGING_OFFSET 0x10

/*
 * BITMASKS
 */

/* Time bitmasks */
#define DS3231_BITS_TIME_SECONDS     GENMASK(6, 0)
#define DS3231_BITS_TIME_MINUTES     GENMASK(6, 0)
#define DS3231_BITS_TIME_HOURS       GENMASK(5, 0)
#define DS3231_BITS_TIME_PM          BIT(5)
#define DS3231_BITS_TIME_12HR        BIT(6)
#define DS3231_BITS_TIME_DAY_OF_WEEK GENMASK(2, 0)
#define DS3231_BITS_TIME_DATE        GENMASK(5, 0)
#define DS3231_BITS_TIME_MONTH       GENMASK(4, 0)
#define DS3231_BITS_TIME_CENTURY     BIT(7)
#define DS3231_BITS_TIME_YEAR        GENMASK(7, 0)

/* Alarm bitmasks */
/* All alarm bitmasks match with time other than date and the alarm rate bit. */
#define DS3231_BITS_ALARM_RATE        BIT(7)
#define DS3231_BITS_ALARM_DATE_W_OR_M BIT(6)

#define DS3231_BITS_SIGN       BIT(7)
/* Control bitmasks */
#define DS3231_BITS_CTRL_EOSC  BIT(7) /* enable oscillator, active low */
#define DS3231_BITS_CTRL_BBSQW BIT(6) /* enable battery-backed square-wave */

/*  Setting the CONV bit to 1 forces the temperature sensor to
 *  convert the temperature into digital code and
 *  execute the TCXO algorithm to update
 *  the capacitance array to the oscillator. This can only
 *  happen when a conversion is not already in progress.
 *  The user should check the status bit BSY before
 *  forcing the controller to start a new TCXO execution.
 *  A user-initiated temperature conversion
 *  does not affect the internal 64-second update cycle.
 */
#define DS3231_BITS_CTRL_CONV BIT(6)

/* Rate selectors */
/* RS2 | RS1 | SQW FREQ
 *  0  |  0  | 1Hz
 *  0  |  1  | 1.024kHz
 *  1  |  0  | 4.096kHz
 *  1  |  1  | 8.192kHz
 */
#define DS3231_BITS_CTRL_RS2 BIT(4)
#define DS3231_BITS_CTRL_RS1 BIT(3)

#define DS3231_BITS_CTRL_INTCTRL    BIT(2)
#define DS3231_BITS_CTRL_ALARM_2_EN BIT(1)
#define DS3231_BITS_CTRL_ALARM_1_EN BIT(0)

/* Control status bitmasks */
/* For some reason you can access OSF in both control and control status registers. */
#define DS3231_BITS_CTRL_STS_OSF          BIT(7) /* oscillator stop flag */ /* read only */
#define DS3231_BITS_CTRL_STS_32_EN        BIT(3)                            /* 32kHz square-wave */
/* set when TXCO is busy, see CONV flag: read only */
#define DS3231_BITS_CTRL_STS_BSY          BIT(2)
#define DS3231_BITS_CTRL_STS_ALARM_2_FLAG BIT(1) /* can only be set to 0 */
#define DS3231_BITS_CTRL_STS_ALARM_1_FLAG BIT(0) /* can only be set to 0 */

/* Aging offset bitmask */
#define DS3231_BITS_DATA BIT(6, 0)

/* Settings bitmasks */
#define DS3231_BITS_STS_OSC     BIT(0)
#define DS3231_BITS_STS_INTCTRL BIT(1)
#define DS3231_BITS_STS_SQW     BIT(2)
#define DS3231_BITS_STS_32KHZ   BIT(3)
#define DS3231_BITS_STS_ALARM_1 BIT(4)
#define DS3231_BITS_STS_ALARM_2 BIT(5)
