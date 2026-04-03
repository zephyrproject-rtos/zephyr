/*
 * Copyright (c) 2023 ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ENE_KB1200_SER_H
#define ENE_KB1200_SER_H

/**
 *  Structure type to access Serial Port Interface (SER).
 */
struct serial_regs {
	volatile uint32_t SERCFG;  /*Configuration Register */
	volatile uint32_t SERIE;   /*Interrupt Enable Register */
	volatile uint32_t SERPF;   /*Pending flag Register */
	volatile uint32_t SERSTS;  /*Status Register */
	volatile uint32_t SERRBUF; /*Rx Data Buffer Register */
	volatile uint32_t SERTBUF; /*Tx Data Buffer Register */
	volatile uint32_t SERCTRL; /*Control Register */
};

#define DIVIDER_BASE_CLK 24000000

#define SERCTRL_MODE0 0 /* shift  */
#define SERCTRL_MODE1 1 /* 8-bit  */
#define SERCTRL_MODE2 2 /* 9-bit  */
#define SERCTRL_MODE3 3 /* 9-bit  */

#define SERCFG_RX_ENABLE  0x01
#define SERCFG_TX_ENABLE  0x02

#define SERCFG_PARITY_NONE  0
#define SERCFG_PARITY_ODD   1
#define SERCFG_PARITY_EVEN  3

/* Pending Flag */
#define SERPF_RX_CNT_FULL 0x01
#define SERPF_TX_EMPTY    0x02
#define SERPF_RX_ERROR    0x04

/* Interrupt Enable */
#define SERIE_RX_ENABLE   0x01
#define SERIE_TX_ENABLE   0x02
#define SERIE_RX_ERROR    0x04

/* Status Flag */
#define SERSTS_FRAME_ERROR  0x0200
#define SERSTS_PARITY_ERROR 0x0100
#define SERSTS_RX_TIMEOUT   0x0080
#define SERSTS_RX_BUSY      0x0040
#define SERSTS_RX_OVERRUN   0x0020
#define SERSTS_RX_EMPTY     0x0010
#define SERSTS_TX_BUSY      0x0004
#define SERSTS_TX_OVERRUN   0x0002
#define SERSTS_TX_FULL      0x0001

#endif /* ENE_KB1200_SER_H */
