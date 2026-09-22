/*
 * Copyright (c) 2025 ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ENE_KB106X_SER_H
#define ENE_KB106X_SER_H

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

#define DIVIDER_BASE_CLK 16000000

#define SERCTRL_MODE0 0 /* shift  */
#define SERCTRL_MODE1 1 /* 8-bit  */
#define SERCTRL_MODE2 2 /* 9-bit  */
#define SERCTRL_MODE3 3 /* 9-bit  */

#define SERCFG_RX_ENABLE 0x01
#define SERCFG_TX_ENABLE 0x02

/* Pending Flag */
#define SERPF_RX_CNT_FULL 0x01
#define SERPF_TX_EMPTY    0x02

/* Interrupt Enable */
#define SERIE_RX_ENABLE 0x01
#define SERIE_TX_ENABLE 0x02

/* Status Flag */
#define SERSTS_RX_BUSY    0x0008
#define SERSTS_TX_BUSY    0x0004
#define SERSTS_TX_OVERRUN 0x0002
#define SERSTS_TX_FULL    0x0001

#endif /* ENE_KB106X_SER_H */
