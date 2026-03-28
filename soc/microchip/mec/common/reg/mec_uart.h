/*
 * Copyright (c) 2021 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MEC_UART_H
#define _MEC_UART_H

#include <stdint.h>
#include <stddef.h>
#include <zephyr/sys/util.h>

#define XEC_UART_RX_FIFO_MAX_LEN 16u
#define XEC_UART_TX_FIFO_MAX_LEN 16u

#define XEC_UART_BAUD_RATE_MIN 50u
#define XEC_UART_BAUD_RATE_MAX 1500000u

#define XEC_UART_SPACING 0x400u

/*
 * LCR DLAB=0
 * Transmit buffer(WO), Receive buffer(RO)
 * LCR DLAB=1, BAUD rate divisor LSB
 */
#define XEC_UART_RTXB_OFS     0u
#define XEC_UART_BRGD_LSB_OFS 0u

/*
 * LCR DLAB=0
 * Interrupt Enable Register, R/W
 * LCR DLAB=1, BAUD rate divisor MSB
 */
#define XEC_UART_BRGD_MSB_OFS   1u
#define XEC_UART_IER_OFS        1u
#define XEC_UART_IER_MASK       0x0fu
#define XEC_UART_IER_ERDAI_POS  0
#define XEC_UART_IER_ETHREI_POS 1
#define XEC_UART_IER_ELSI_POS   2
#define XEC_UART_IER_EMSI_POS   3

#define XEC_UART_IER_ERDAI  0x01u /* Received data available and timeouts */
#define XEC_UART_IER_ETHREI 0x02u /* TX Holding register empty */
#define XEC_UART_IER_ELSI   0x04u /* Errors: Overrun, Parity, Framing, and Break */
#define XEC_UART_IER_EMSI   0x08u /* Modem Status */
#define XEC_UART_IER_ALL    0x0fu

/* FIFO Control Register, Write-Only */
#define XEC_UART_FCR_OFS                2u
#define XEC_UART_FCR_MSK                0xcfu
#define XEC_UART_FCR_EXRF               0x01u /* Enable TX & RX FIFO's */
#define XEC_UART_FCR_CLR_RX_FIFO        0x02u /* Clear RX FIFO, bit is self-clearing */
#define XEC_UART_FCR_CLR_TX_FIFO        0x04u /* Clear TX FIFO, bit is self-clearing */
#define XEC_UART_FCR_DMA_EN             0x08u /* DMA Mode Enable. Not implemented */
#define XEC_UART_FCR_RX_FIFO_LVL_MSK    GENMASK(7, 6)
#define XEC_UART_FCR_RX_FIFO_LVL_1      0
#define XEC_UART_FCR_RX_FIFO_LVL_4      1u
#define XEC_UART_FCR_RX_FIFO_LVL_8      2u
#define XEC_UART_FCR_RX_FIFO_LVL_14     3u
#define XEC_UART_FCR_RX_FIFO_LVL_SET(f) FIELD_PREP(XEC_UART_FCR_RX_FIFO_LVL_MSK, (f))
#define XEC_UART_FCR_RX_FIFO_LVL_GET(r) FIELD_GET(XEC_UART_FCR_RX_FIFO_LVL_MSK, (r))

/* Interrupt Identification Register, Read-Only
 * IIR encodes one interrupt event at a time by event priority.
 * It must be read and each interrupt event ID processed until
 * bit[0] -> 1 indicating no UART interrupt event is pending.
 * 16550 HW event priority and clearing method.
 * Highest: Errors: overrun, parity, framing or break. Clear by reading LSR
 * Second Highest: RX data available or RX FIFO trigger level reached
 *                 Clear by reading RX data register until below RX FIFO threshold
 * Second Highest: RX data timeout. RX character received without error and has not
 *   been removed from RX buffer/FIFO for 4 character times. Indicates servicing
 *   FW is not able to keep up. Clear by reading RX data.
 * Third Highest: TX Holding Register Empty. Clear by reading IIR or writing next
 *   byte to transmit into TX holding register or FIFO.
 * Lowest: MODEM status CTS, DSR, or RI. Clear by reading MSR.
 */
#define XEC_UART_IIR_OFS           2u
#define XEC_UART_IIR_MASK          0xcfu
#define XEC_UART_IIR_NOT_IPEND_POS 0
#define XEC_UART_IIR_INTID_POS     1u
#define XEC_UART_IIR_INTID_MSK     GENMASK(3, 1)
#define XEC_UART_IIR_INTID_MDM     0
#define XEC_UART_IIR_INTID_THRE    1u
#define XEC_UART_IIR_INTID_RXD     2u
#define XEC_UART_IIR_INTID_LSR     3u
#define XEC_UART_IIR_INTID_RXTM    6u
#define XEC_UART_IIR_INTID_GET(r)  FIELD_GET(XEC_UART_IIR_INTID_MSK, (r))

#define XEC_UART_IIR_INTID_MASK0  0x0fu
#define XEC_UART_IIR_INTID_MASK   0x0eu
#define XEC_UART_IIR_FIFO_EN_MASK 0xc0u

/* Line Control Register R/W */
#define XEC_UART_LCR_OFS             3u
#define XEC_UART_LCR_WORD_LEN_MSK    GENMASK(1, 0)
#define XEC_UART_LCR_WORD_LEN_5      0x00u
#define XEC_UART_LCR_WORD_LEN_6      0x01u
#define XEC_UART_LCR_WORD_LEN_7      0x02u
#define XEC_UART_LCR_WORD_LEN_8      0x03u
#define XEC_UART_LCR_WORD_LEN_SET(l) FIELD_PREP(XEC_UART_LCR_WORD_LEN_MSK, (l))
#define XEC_UART_LCR_WORD_LEN_GET(r) FIELD_GET(XEC_UART_LCR_WORD_LEN_MSK, (r))
#define XEC_UART_LCR_STOP_BIT_1      0x00u
#define XEC_UART_LCR_STOP_BIT_2      0x04u
#define XEC_UART_LCR_PARITY_MSK      GENMASK(5, 3)
#define XEC_UART_LCR_PARITY_NONE     0
#define XEC_UART_LCR_PARITY_ODD      0x1u
#define XEC_UART_LCR_PARITY_EVEN     0x3u
#define XEC_UART_LCR_PARITY_MARK     0x5u
#define XEC_UART_LCR_PARITY_SPACE    0x7u
#define XEC_UART_LCR_PARITY_SET(p)   FIELD_PREP(XEC_UART_LCR_PARITY_MSK, (p))
#define XEC_UART_LCR_PARITY_GET(r)   FIELD_GET(XEC_UART_LCR_PARITY_MSK, (r))
#define XEC_UART_LCR_BREAK_EN        0x40u
#define XEC_UART_LCR_DLAB_EN         0x80u

/* MODEM Control Register R/W */
#define XEC_UART_MCR_OFS        4u
#define XEC_UART_MCR_MASK       0x1fu
#define XEC_UART_MCR_DTRn       0x01u
#define XEC_UART_MCR_RTSn       0x02u
#define XEC_UART_MCR_OUT1       0x04u
#define XEC_UART_MCR_OUT2       0x08u
#define XEC_UART_MCR_LOOPBCK_EN 0x10u

/* Line Status Register RO */
#define XEC_UART_LSR_OFS          5u
#define XEC_UART_LSR_DATA_RDY_POS 0
#define XEC_UART_LSR_OVERRUN_POS  1
#define XEC_UART_LSR_PARITY_POS   2
#define XEC_UART_LSR_FRAME_POS    3
#define XEC_UART_LSR_RX_BREAK_POS 4
#define XEC_UART_LSR_THRE_POS     5
#define XEC_UART_LSR_TEMT_POS     6
#define XEC_UART_LSR_FIFO_ERR_POS 7

#define XEC_UART_LSR_DATA_RDY 0x01u
#define XEC_UART_LSR_OVERRUN  0x02u
#define XEC_UART_LSR_PARITY   0x04u
#define XEC_UART_LSR_FRAME    0x08u
#define XEC_UART_LSR_RX_BREAK 0x10u
#define XEC_UART_LSR_THRE     0x20u
#define XEC_UART_LSR_TEMT     0x40u
#define XEC_UART_LSR_FIFO_ERR 0x80u
#define XEC_UART_LSR_ANY      0xffu
#define XEC_UART_LSR_ANY_ERR  0x1eu

/* MODEM Status Register RO */
#define XEC_UART_MSR_OFS      6u
#define XEC_UART_MSR_DCTS_POS 0
#define XEC_UART_MSR_DDSR_POS 1
#define XEC_UART_MSR_TERI_POS 2
#define XEC_UART_MSR_DDCD_POS 3
#define XEC_UART_MSR_CTS_POS  4
#define XEC_UART_MSR_DSR_POS  5
#define XEC_UART_MSR_RI_POS   6
#define XEC_UART_MSR_DCD_POS  7

#define XEC_UART_MSR_DCTS 0x01u
#define XEC_UART_MSR_DDSR 0x02u
#define XEC_UART_MSR_TERI 0x04u
#define XEC_UART_MSR_DDCD 0x08u
#define XEC_UART_MSR_CTS  0x10u
#define XEC_UART_MSR_DSR  0x20u
#define XEC_UART_MSR_RI   0x40u
#define XEC_UART_MSR_DCD  0x80u

/* Scratch Register RO */
#define XEC_UART_SCR_OFS 7u

/* UART Logical Device Activate Register */
#define XEC_UART_LD_ACT_OFS      0x330u
#define XEC_UART_LD_ACTIVATE_POS 0
#define XEC_UART_LD_ACTIVATE     0x01u

/* UART Logical Device Config Register */
#define XEC_UART_LD_CFG_OFS       0x3f0u
#define XEC_UART_LD_CFG_INTCLK    0u
#define XEC_UART_LD_CFG_NO_INVERT 0u
#define XEC_UART_LD_CFG_RESET_SYS 0u
#define XEC_UART_LD_CFG_EXTCLK    BIT(0)
#define XEC_UART_LD_CFG_RESET_VCC BIT(1)
#define XEC_UART_LD_CFG_INVERT    BIT(2)

/* MEC174x and onwards have a new register only visible to the EC at offset 0x8
 * Line State 2 register. It has fields indicating the number of bytes currenly
 * in the TX FIFO and if the TX FIFO is full.
 */
#define XEC_UART_LSR2_OFS              8u
#define XEC_UART_LSR2_TX_FIFO_FULL_POS 0
#define XEC_UART_LSR2_TX_FIFO_CNT_POS  4u
#define XEC_UART_LSR2_TX_FIFO_CNT_MSK  0xf0u

/* BAUD rate generator */
#define XEC_UART_INT_CLK_24M BIT(15)

/* 1.8MHz internal clock source */
#define XEC_UART_1P8M_BAUD_50     2304u
#define XEC_UART_1P8M_BAUD_110    1536u
#define XEC_UART_1P8M_BAUD_150    768u
#define XEC_UART_1P8M_BAUD_300    384u
#define XEC_UART_1P8M_BAUD_1200   96u
#define XEC_UART_1P8M_BAUD_2400   48u
#define XEC_UART_1P8M_BAUD_9600   12u
#define XEC_UART_1P8M_BAUD_19200  6u
#define XEC_UART_1P8M_BAUD_38400  3u
#define XEC_UART_1P8M_BAUD_57600  2u
#define XEC_UART_1P8M_BAUD_115200 1u

/* 24MHz internal clock source. n = 24e6 / (BAUD * 16) = 1500000 / BAUD */
#define XEC_UART_24M_BAUD_115200 ((13u) + (XEC_UART_INT_CLK_24M))
#define XEC_UART_24M_BAUD_57600  ((26u) + (XEC_UART_INT_CLK_24M))

#endif /* #ifndef _MEC_UART_H */
