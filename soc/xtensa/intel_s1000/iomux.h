/*
 * Copyright (c) 2018 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INTEL_S1000_IOMUX_H
#define __INTEL_S1000_IOMUX_H

/*
 * +----------------------+-------------------------------------------------+
 * |                   Table of Possible I/O MUX settings                   |
 * +----------------------+-------------------------------------------------+
 * | Pin group            | FUNC_A (Default) | FUNC_B (Available Alternate) |
 * +----------------------+-------------------------------------------------+
 * | PIN_GROUP(EM_DQ)     | EM_DQ            | MST_DQ                       |
 * | PIN_GROUP(GPIO_PWM0) | GPIO             | PWM                          |
 * | PIN_GROUP(GPIO_PWM1) | GPIO             | PWM                          |
 * | PIN_GROUP(GPIO_PWM2) | GPIO             | PWM                          |
 * | PIN_GROUP(GPIO_PWM3) | GPIO             | PWM                          |
 * | PIN_GROUP(GPIO_PWM4) | GPIO             | PWM                          |
 * | PIN_GROUP(GPIO_PWM5) | GPIO             | PWM                          |
 * | PIN_GROUP(GPIO_PWM6) | GPIO             | PWM                          |
 * | PIN_GROUP(GPIO_PWM7) | GPIO             | PWM                          |
 * | PIN_GROUP(HOST_IRQ)  | HOST_IRQ         | GPIO14                       |
 * | PIN_GROUP(HOST_WAKE) | HOST_WAKE        | GPIO13                       |
 * | PIN_GROUP(I2S0)      | I2S              | PDM                          |
 * | PIN_GROUP(I2S2)      | I2S              | GPIO                         |
 * | PIN_GROUP(I2S3)      | I2S              | GPIO                         |
 * | PIN_GROUP(MST_SS1)   | MST_SS1          | GPIO25                       |
 * | PIN_GROUP(PDM_0_1)   | PDM_0_1          | GPIO                         |
 * | PIN_GROUP(UART)      | UART             | GPIO                         |
 * | PIN_GROUP(I2C)       | I2C0             | I2C1                         |
 * +----------------------+-------------------------------------------------+
 */

#define PIN_GROUP(group)		IOMUX_PINGROUP_ ## group

/*
 * IO Selector is a bit encoding of the mux control register and the
 * bits selecting the mux in the mux control register
 */
#define IO_SEL(iomux, lsb, msb)		((iomux) | ((lsb) << 8) | ((msb) << 16))
#define IOMUX_INDEX(pingroup)		((pingroup) & BIT_MASK(8))
#define IOMUX_LSB(pingroup)		(((pingroup) >> 8) & BIT_MASK(8))
#define IOMUX_MSB(pingroup)		(((pingroup) >> 16) & BIT_MASK(8))

/* PSRAM DQ/WAIT or SPI Master DQ/DQS */
#define IOMUX_PINGROUP_EM_DQ		IO_SEL(0, 25, 25)

/* GPIO or PWM */
#define IOMUX_PINGROUP_GPIO_PWM0	IO_SEL(1, 0, 1)
#define IOMUX_PINGROUP_GPIO_PWM1	IO_SEL(1, 2, 3)
#define IOMUX_PINGROUP_GPIO_PWM2	IO_SEL(1, 4, 5)
#define IOMUX_PINGROUP_GPIO_PWM3	IO_SEL(1, 6, 7)
#define IOMUX_PINGROUP_GPIO_PWM4	IO_SEL(1, 8, 9)
#define IOMUX_PINGROUP_GPIO_PWM5	IO_SEL(1, 10, 11)
#define IOMUX_PINGROUP_GPIO_PWM6	IO_SEL(1, 12, 13)
#define IOMUX_PINGROUP_GPIO_PWM7	IO_SEL(1, 14, 15)


/* HOST_IRQ or GPIO14 */
#define IOMUX_PINGROUP_HOST_IRQ		IO_SEL(0, 1, 1)

/* HOST_WAKE or GPIO13 */
#define IOMUX_PINGROUP_HOST_WAKE	IO_SEL(0, 0, 0)

/* I2S0 or PDM2/3 */
#define IOMUX_PINGROUP_I2S0		IO_SEL(0, 8, 8)

/* I2S2 or GPIO */
#define IOMUX_PINGROUP_I2S2		IO_SEL(0, 9, 9)
/* I2S3 or GPIO */
#define IOMUX_PINGROUP_I2S3		IO_SEL(0, 10, 10)

/* SPI Master SS #1 MST_SS1 or GPIO 25 */
#define IOMUX_PINGROUP_MST_SS1		IO_SEL(0, 26, 26)

/* PDM0/1 or GPIO */
#define IOMUX_PINGROUP_PDM_0_1		IO_SEL(0, 11, 11)

/* UART CTS/RTS or GPIO */
#define IOMUX_PINGROUP_UART		IO_SEL(0, 16, 16)

/* I2C0 or I2C1 */
#define IOMUX_PINGROUP_I2C		IO_SEL(2, 0, 0)

#endif /* __INTEL_S1000_IOMUX_H */
