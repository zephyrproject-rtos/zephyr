/*
 * Copyright (c) 2024 ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ENE_KB1200_SPIH_H
#define ENE_KB1200_SPIH_H

/**
 *  Structure type to access SPI Host Controller (SPIH).
 */
struct spih_regs {
	volatile uint8_t SPIHCFG;      /*Configuration Register */
	volatile uint8_t Reserved0[3]; /*Reserved */
	volatile uint8_t SPIHCTR;      /*Control Register */
	volatile uint8_t Reserved1[3]; /*Reserved */
	volatile uint16_t SPIHTBUF;    /*Transmit data buffer Register */
	volatile uint16_t Reserved2;   /*Reserved */
	volatile uint16_t SPIHRBUF;    /*Receive data buffer Register */
	volatile uint16_t Reserved3;   /*Reserved */
};

#define SPIH_FUNCTION_ENABLE 0x01
#define SPIH_PUSH_PULL       0x40

#define SPIH_MODE_MASK 0x30
#define SPIH_MODE_POS  4
#define SPIH_MODE_0    0x00
#define SPIH_MODE_1    0x01
#define SPIH_MODE_2    0x02
#define SPIH_MODE_3    0x03

#define SPIH_CLOCK_MASK 0x0E
#define SPIH_CLOCK_POS  1
#define SPIH_CLOCK_16M  0x00
#define SPIH_CLOCK_8M   0x01
#define SPIH_CLOCK_4M   0x02
#define SPIH_CLOCK_2M   0x03
#define SPIH_CLOCK_1M   0x04
#define SPIH_CLOCK_500K 0x05

#define SPIH_PINOUT_NO_CHANGE 0x00
#define SPIH_PINOUT_KSO       0x01
#define SPIH_PINOUT_SHR_ROM   0x02
#define SPIH_PINOUT_NORMAL    0x03

#define SPIH_DUMMY_BYTE 0xFF

#define SPIH_CS_HIGH 0x00
#define SPIH_CS_LOW  0x01

#define SPIH_BUSY_FLAG   0x80
#define SPIH_BUFF_16BITS 0x02

#endif /* ENE_KB1200_SPIH_H */
