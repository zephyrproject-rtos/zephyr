/*
 * Copyright (c) 2024 ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ENE_KB1200_KBC_H
#define ENE_KB1200_KBC_H

/**
 *  Structure type to access Keyboard Controller (KBC).
 */
struct kbc_regs {
	volatile uint16_t KBCCFG;      /*Configuration Register */
	volatile uint16_t Reserved0;   /*Reserved */
	volatile uint8_t KBCIE;        /*Interrupt Enable Register */
	volatile uint8_t Reserved1[3]; /*Reserved */
	volatile uint8_t KBCPF;        /*Event Pending Flag Register */
	volatile uint8_t Reserved2[3]; /*Reserved */
	volatile uint32_t Reserved3;   /*Reserved */
	volatile uint8_t KBCSTS;       /*Status Register */
	volatile uint8_t Reserved4[3]; /*Reserved */
	volatile uint8_t KBCCMD;       /*Command Register */
	volatile uint8_t Reserved5[3]; /*Reserved */
	volatile uint8_t KBCODP;       /*Output Data Port Register */
	volatile uint8_t Reserved6[3]; /*Reserved */
	volatile uint8_t KBCIDP;       /*Input Data Port Register */
	volatile uint8_t Reserved7[3]; /*Reserved */
	volatile uint8_t KBCCB;        /*Command Byte Register */
	volatile uint8_t Reserved8[3]; /*Reserved */
	volatile uint8_t KBCDATH;      /*Write Data from H/W Register */
	volatile uint8_t Reserved9[3]; /*Reserved */
};

#define KBC_OBF_EVENT 0x01
#define KBC_IBF_EVENT 0x02

#define KBC_FUNCTION_ENABLE        0x01
#define KBC_OUTPUT_READ_CLR_ENABLE 0x02

/* KBCCB */
#define KBC_SCAN_CODE_CONVERSION 0x40
#define KBC_AUX_DEVICE_DISABLE   0x20
#define KBC_KB_DEVICE_DISABLE    0x10
#define KBC_SYSTEM_FLAG          0x04
#define KBC_IRQ12_ENABLE         0x02
#define KBC_IRQ1_ENABLE          0x01

/* KBCSTS */
#define KBSTS_PARITY_ERROR       0x80
#define KBSTS_TIME_OUT           0x40
#define KBSTS_AUX_FLAG           0x20
#define KBSTS_ADDRESS_64         0x08
#define KBSTS_MULTIPLEXING_ERROR 0x04
#define KBSTS_IBF                0x02
#define KBSTS_OBF                0x01

#endif /* ENE_KB1200_KBC_H */
