/*
 * Copyright (c) 2026 ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ENE_KB106X_KBC_H
#define ENE_KB106X_KBC_H

/**
 * brief  Structure type to access Keyboard Controller (KBC).
 */
struct kbc_regs {
	volatile uint16_t kbc_cfg;      /*Configuration Register */
	volatile uint16_t reserved_0;   /*Reserved */
	volatile uint8_t kbc_ie;        /*Interrupt Enable Register */
	volatile uint8_t reserved_1[3]; /*Reserved */
	volatile uint8_t kbc_pf;        /*Event Pending Flag Register */
	volatile uint8_t reserved_2[3]; /*Reserved */
	volatile uint32_t reserved_3;   /*Reserved */
	volatile uint8_t kbc_sts;       /*Status Register */
	volatile uint8_t reserved_4[3]; /*Reserved */
	volatile uint8_t kbc_cmd;       /*Command Register */
	volatile uint8_t reserved_5[3]; /*Reserved */
	volatile uint8_t kbc_odp;       /*Output Data Port Register */
	volatile uint8_t reserved_6[3]; /*Reserved */
	volatile uint8_t kbc_idp;       /*Input Data Port Register */
	volatile uint8_t reserved_7[3]; /*Reserved */
	volatile uint8_t kbc_cb;        /*Command Byte Register */
	volatile uint8_t reserved_8[3]; /*Reserved */
	volatile uint8_t kbc_dath;      /*Write Data from H/W Register */
	volatile uint8_t reserved_9[3]; /*Reserved */
};

#define KBC_OBE_EVENT 0x01
#define KBC_IBF_EVENT 0x02
#define KBCIE_OBF     0x01
#define KBCIE_IBF     0x02
#define KBCPF_HBF     0x04

#define KBC_FUNCTION_ENABLE        0x01
#define KBC_OUTPUT_READ_CLR_ENABLE 0x02

/* KCCB */
#define KBC_SCAN_CODE_CONVERSION 0x40
#define KBC_AUX_DEVICE_DISABLE   0x20
#define KBC_KB_DEVICE_DISABLE    0x10
#define KBC_SYSTEM_FLAG          0x04
#define KBC_IRQ12_ENABLE         0x02
#define KBC_IRQ1_ENABLE          0x01

/* KBSTS */
#define KBSTS_PARITY_ERROR 0x80
#define KBSTS_TIME_OUT     0x40
#define KBSTS_AUX_FLAG     0x20
#define KBSTS_ADDRESS_64   0x08
#define KBSTS_SYS_FLAG     0x04
#define KBSTS_IBF          0x02
#define KBSTS_OBF          0x01

#endif /*ENE_KB106X_KBC_H*/
