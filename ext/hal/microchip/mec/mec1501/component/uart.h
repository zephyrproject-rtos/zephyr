/**
 *
 * Copyright (c) 2019 Microchip Technology Inc. and its subsidiaries.
 *
 * \asf_license_start
 *
 * \page License
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the Licence at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * \asf_license_stop
 *
 */

/** @file uart.h
 *MEC1501 UART Peripheral Library API
 */
/** @defgroup MEC1501 Peripherals UART
 */

#ifndef _UART_H
#define _UART_H

#include <stdint.h>
#include <stddef.h>

#define UART0_ID            0
#define UART1_ID            1
#define UART2_ID            2
#define UART_MAX_ID         3

#define UART_RX_FIFO_MAX_LEN    16u
#define UART_TX_FIFO_MAX_LEN    16u

#define UART_BAUD_RATE_MIN      50
#define UART_BAUD_RATE_MAX      1500000

#define UART0_BASE_ADDRESS      0x400F2400ul
#define UART1_BASE_ADDRESS      0x400F2800ul
#define UART2_BASE_ADDRESS      0x400F2C00ul

#define UART_GIRQ_NUM                   15
#define UART_GIRQ_ID                    7
#define UART_GIRQ_SRC_ADDR              0x4000E08Cul
#define UART_GIRQ_EN_SET_ADDR           0x4000E090ul
#define UART_GIRQ_RESULT_ADDR           0x4000E094ul
#define UART_GIRQ_EN_CLR_ADDR           0x4000E098ul
#define UART0_GIRQ_BIT                  0
#define UART1_GIRQ_BIT                  1
#define UART2_GIRQ_BIT                  4
#define UART0_GIRQ_VAL                  (1ul << (UART0_GIRQ_BIT))
#define UART1_GIRQ_VAL                  (1ul << (UART1_GIRQ_BIT))
#define UART2_GIRQ_VAL                  (1ul << (UART2_GIRQ_BIT))
#define UART0_NVIC_DIRECT_NUM           40
#define UART1_NVIC_DIRECT_NUM           41
#define UART2_NVIC_DIRECT_NUM           44
#define UART_NVIC_SETEN_GIRQ_ADDR       0xE000E100ul
#define UART_NVIC_SETEN_GIRQ_BIT        7             /* aggregated */
#define UART_NVIC_SETEN_DIRECT_ADDR     0xE000E104ul
#define UART0_NVIC_SETEN_DIRECT_BIT     (40 - 32)
#define UART1_NVIC_SETEN_DIRECT_BIT     (41 - 32)
#define UART2_NVIC_SETEN_DIRECT_BIT     (44 - 32)

/* CMSIS NVIC macro IRQn parameter */
#define UART_NVIC_IRQn                  7ul     /* aggregated */
#define UART0_NVIC_IRQn                 41ul    /* UART0 direct mode */
#define UART1_NVIC_IRQn                 42ul    /* UART1 direct mode */
#define UART2_NVIC_IRQn                 44ul    /* UART2 direct mode */


/* Interrupt Enable Register, R/W DLAB=0 */
#define UART_IER_RW             1
#define UART_IER_MASK           0x0Ful
#define UART_IER_ERDAI          0x01ul /* Received data available and timeouts */
#define UART_IER_ETHREI         0x02ul /* TX Holding register empty */
#define UART_IER_ELSI           0x04ul /* Errors: Overrun, Parity, Framing, and Break */
#define UART_IER_EMSI           0x08ul /* Modem Status */
#define UART_IER_ALL            0x0Ful

/* FIFO Contro Register, Write-Only */
#define UART_FCR_WO                 2
#define UART_FCR_MASK               0xCF
#define UART_FCR_EXRF               0x01 /* Enable TX & RX FIFO's */
#define UART_FCR_CLR_RX_FIFO        0x02 /* Clear RX FIFO, bit is self-clearing */
#define UART_FCR_CLR_TX_FIFO        0x04 /* Clear TX FIFO, bit is self-clearing */
#define UART_FCR_DMA_EN             0x08 /* DMA Mode Enable. Not implemented */
#define UART_FCR_RX_FIFO_LVL_MASK   0xC0 /* RX FIFO trigger level mask */
#define UART_FCR_RX_FIFO_LVL_1      0x00
#define UART_FCR_RX_FIFO_LVL_4      0x40
#define UART_FCR_RX_FIFO_LVL_8      0x80
#define UART_FCR_RX_FIFO_LVL_14     0xC0

/* Interrupt Identification Register, Read-Only */
#define UART_IIR_RO                 2
#define UART_IIR_MASK               0xCF
#define UART_IIR_NOT_IPEND          0x01
#define UART_IIR_INTID_MASK0        0x07
#define UART_IIR_INTID_POS          1
#define UART_IIR_INTID_MASK         0x0E
#define UART_IIR_FIFO_EN_MASK       0xC0
/* interrupt values */
#define UART_IIR_INT_NONE           0x01
#define UART_IIR_INT_LS             0x06 /* Highest priority: Line status, overrun, framing, or break */
#define UART_IIR_INT_RX             0x04 /* Highest-1. RX data available or RX FIFO trigger level reached */
#define UART_IIR_INT_RX_TMOUT       0x0C /* Highest-2. RX timeout */
#define UART_IIR_INT_THRE           0x02 /* Highest-3. TX Holding register empty. */
#define UART_IIR_INT_MS             0x00 /* Highest-4. MODEM status. */

/* Line Control Register R/W */
#define UART_LCR_RW                 3
#define UART_LCR_WORD_LEN_MASK      0x03
#define UART_LCR_WORD_LEN_5         0x00
#define UART_LCR_WORD_LEN_6         0x01
#define UART_LCR_WORD_LEN_7         0x02
#define UART_LCR_WORD_LEN_8         0x03
#define UART_LCR_STOP_BIT_1         0x00
#define UART_LCR_STOP_BIT_2         0x04 /* 2 for 6-8 bits or 1.5 for 5 bits */
#define UART_LCR_PARITY_NONE        0x00
#define UART_LCR_PARITY_EN          0x08
#define UART_LCR_PARITY_ODD         0x00
#define UART_LCR_PARITY_EVEN        0x10
#define UART_LCR_STICK_PARITY       0x20
#define UART_LCR_BREAK_EN           0x40
#define UART_LCR_DLAB_EN            0x80

/* MODEM Control Register R/W */
#define UART_MCR_RW                 4
#define UART_MCR_MASK               0x1F
#define UART_MCR_DTRn               0x01
#define UART_MCR_RTSn               0x02
#define UART_MCR_OUT1               0x04
#define UART_MCR_OUT2               0x08
#define UART_MCR_LOOPBCK_EN         0x10

/* Line Status Register RO */
#define UART_LSR_RO                 5
#define UART_LSR_DATA_RDY           0x01
#define UART_LSR_OVERRUN            0x02
#define UART_LSR_PARITY             0x04
#define UART_LSR_FRAME              0x08
#define UART_LSR_RX_BREAK           0x10
#define UART_LSR_THRE               0x20
#define UART_LSR_TEMT               0x40
#define UART_LSR_FIFO_ERR           0x80
#define UART_LSR_ANY                0xFF

/* MODEM Status Register RO */
#define UART_MSR_RO                 6
#define UART_MSR_DCTS               0x01
#define UART_MSR_DDSR               0x02
#define UART_MSR_TERI               0x04
#define UART_MSR_DDCD               0x08
#define UART_MSR_CTS                0x10
#define UART_MSR_DSR                0x20
#define UART_MSR_RI                 0x40
#define UART_MSR_DCD                0x80

/* Scratch Register RO */
#define UART_SCR_RW                 7

/* UART Logical Device Activate Register */
#define UART_LD_ACT                 0x330
#define UART_LD_ACTIVATE            0x01

/* UART Logical Device Config Register */
#define UART_LD_CFG                 0x3F0
#define UART_LD_CFG_INTCLK          (0u << 0)
#define UART_LD_CFG_EXTCLK          (1u << 0)
#define UART_LD_CFG_RESET_SYS       (0u << 1)
#define UART_LD_CFG_RESET_VCC       (1u << 1)
#define UART_LD_CFG_NO_INVERT       (0u << 2)
#define UART_LD_CFG_INVERT          (1u << 2)

/* BAUD rate generator */
#define UART_INT_CLK_24M            (1ul << 15)

/* 1.8MHz internal clock source */
#define UART_1P8M_BAUD_50           2304
#define UART_1P8M_BAUD_110          1536
#define UART_1P8M_BAUD_150          768
#define UART_1P8M_BAUD_300          384
#define UART_1P8M_BAUD_1200         96
#define UART_1P8M_BAUD_2400         48
#define UART_1P8M_BAUD_9600         12
#define UART_1P8M_BAUD_19200        6
#define UART_1P8M_BAUD_38400        3
#define UART_1P8M_BAUD_57600        2
#define UART_1P8M_BAUD_115200       1

/* 24MHz internal clock source. n = 24e6 / (BAUD * 16) = 1500000 / BAUD */
#define UART_24M_BAUD_115200        ((13) + (UART_INT_CLK_24M))
#define UART_24M_BAUD_57600         ((26) + (UART_INT_CLK_24M))


/* =========================================================================*/
/* ================            UART                        ================ */
/* =========================================================================*/

/**
  * @brief UART interface (UART)
  */

#define UART_NUM_INSTANCES       3
#define UART_SPACING             0x400
#define UART_SPACING_PWROF2      10

typedef union
{
    __OM  uint8_t  TXB;         /*!< (@ 0x0000) UART Transmit Buffer(WO) LCR.DLAB=0 */
    __IM  uint8_t  RXB;         /*!< (@ 0x0000) UART Receive Buffer(RO) LCR.DLAB=0 */
    __IOM uint8_t  BRGD_LSB;    /*!< (@ 0x0000) UART BAUD Rate Generator LSB(RW) LCR.DLAB=1 */
} UART_RTXB_BRGL;

typedef union
{
    __IOM uint8_t  IEN;         /*!< (@ 0x0001) UART Interrupt Enable(RW) LCR.DLAB=0 */
    __IOM uint8_t  BRGD_MSB;    /*!< (@ 0x0001) UART BAUD Rate Generator MSB(RW) LCR.DLAB=1 */
} UART_RTXB_BRGH;

typedef union
{
    __IM  uint8_t  IID;         /*!< (@ 0x0002) UART FIFO Control(WO) */
    __OM  uint8_t  FCR;         /*!< (@ 0x0002) UART Interrupt Indentification(RO) */
} UART_FCR_IIR;

typedef struct
{               /*!< (@ 0x400F2400) UART Structure   */
    UART_RTXB_BRGL  RTXB;
    UART_RTXB_BRGH  IER;
    UART_FCR_IIR    FCID;
    __IOM uint8_t   LCR;        /*!< (@ 0x400F2403) UART Line Control(RW) */
    __IOM uint8_t   MCR;        /*!< (@ 0x400F2404) UART Modem Control(RW) */
    __IOM uint8_t   LSR;        /*!< (@ 0x400F2405) UART Line Status(RO) */
    __IOM uint8_t   MSR;        /*!< (@ 0x400F2406) UART Modem Status(RO) */
    __IOM uint8_t   SCR;        /*!< (@ 0x400F2407) UART Scratch(RW) */
          uint8_t   RSVDA[0x330 - 0x08];
    __IOM uint8_t   ACTV;       /*!< (@ 0x400F2730) UART Activate(RW) */
          uint8_t   RSVDB[0x3F0 - 0x331];
    __IOM uint8_t   CFG_SEL;    /*!< (@ 0x400F27F0) UART Configuration Select(RW) */
} UART_Type;


#endif /* #ifndef _UART_H */
/* end uart.h */
/**   @}
 */
