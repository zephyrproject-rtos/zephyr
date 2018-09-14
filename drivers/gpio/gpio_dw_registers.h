/*gpio_dw_registers.h - Private gpio's registers header*/

/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_GPIO_GPIO_DW_REGISTERS_H_
#define ZEPHYR_DRIVERS_GPIO_GPIO_DW_REGISTERS_H_

#ifdef CONFIG_SOC_QUARK_SE_C1000_SS
#define SWPORTA_DR     0x00
#define SWPORTA_DDR    0x01
#define INTEN          0x03
#define INTMASK        0x04
#define INTTYPE_LEVEL  0x05
#define INT_POLARITY   0x06
#define INTSTATUS      0x07
#define PORTA_DEBOUNCE 0x08
#define PORTA_EOI      0x09
#define EXT_PORTA      0x0A
#define INT_CLOCK_SYNC 0x0B

#define CLK_ENA_POS	(31)

#else
/** This definition of GPIO related registers supports four ports: A, B, C, D
 * yet only PORTA supports interrupts and debounce.
 */
#define SWPORTA_DR     0x00
#define SWPORTA_DDR    0x04
#define SWPORTA_CTL    0x08
#define SWPORTB_DR     0x0c
#define SWPORTB_DDR    0x10
#define SWPORTB_CTL    0x14
#define SWPORTC_DR     0x18
#define SWPORTC_DDR    0x1c
#define SWPORTC_CTL    0x20
#define SWPORTD_DR     0x24
#define SWPORTD_DDR    0x28
#define SWPORTD_CTL    0x2c
#define INTEN          0x30
#define INTMASK        0x34
#define INTTYPE_LEVEL  0x38
#define INT_POLARITY   0x3c
#define INTSTATUS      0x40
#define RAW_INTSTATUS  0x44
#define PORTA_DEBOUNCE 0x48
#define PORTA_EOI      0x4c
#define EXT_PORTA      0x50
#define EXT_PORTB      0x54
#define EXT_PORTC      0x58
#define EXT_PORTD      0x5c
#define INT_CLOCK_SYNC 0x60 /* alias LS_SYNC */
#define INT_BOTHEDGE   0x68
#define VER_ID_CODE    0x6c
#define CONFIG_REG2    0x70
#define CONFIG_REG1    0x74
#endif

#define LS_SYNC_POS	(0)

#endif /* ZEPHYR_DRIVERS_GPIO_GPIO_DW_REGISTERS_H_ */
