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

/** @file qmspi.h
 *MEC1501 Quad Master SPI Registers
 */
/** @defgroup MEC1501 Peripherals QMSPI
 */

#ifndef _QMSPI_H_
#define _QMSPI_H_

#define QMPSPI_HW_VER               3

#define QMSPI_BASE_ADDR             0x40070000ul

#define QMSPI_PORT_GPIO_BASE        0x40081000ul
#define QMSPI_PORT_GPIO_BASE2       0x40081500ul

#define QMSPI_MAX_DESCR             16ul

#define QMSPI_INPUT_CLOCK_FREQ_HZ   48000000ul
#define QMSPI_MAX_FREQ_KHZ          ((QMSPI_INPUT_CLOCK_FREQ_HZ) / 1000)
#define QMSPI_MIN_FREQ_KHZ          (QMSPI_MAX_FREQ_KHZ / 256)

#define QMSPI_GIRQ_NUM              18
#define QMSPI_GIRQ_POS              1
#define QMSPI_GIRQ_OFS              (((QMSPI0_GIRQ_NUM) - 8) * 20)
#define QMSPI_GIRQ_BASE_ADDR        0x4000E0C8ul
/* Sleep Enable 4 bit 8 */
#define QMSPI_PCR_SLP_EN_ADDR       0x40080140ul
#define QMSPI_PCR_SLP_EN_BITPOS     8

#define QMSPI_GIRQ_SRC_ADDR        (QMSPI_GIRQ_BASE_ADDR)
#define QMSPI_GIRQ_ENSET_ADDR      (QMSPI_GIRQ_BASE_ADDR + 0x04)
#define QMSPI_GIRQ_RESULT_ADDR     (QMSPI_GIRQ_BASE_ADDR + 0x08)
#define QMSPI_GIRQ_ENCLR_ADDR      (QMSPI_GIRQ_BASE_ADDR + 0x0C)

#define QMSPI_GIRQ_EN              (1ul << (QMSPI_GIRQ_POS))
#define QMSPI_GIRQ_STS             (1ul << (QMSPI_GIRQ_POS))

/* Mode 0: Clock idle = Low. Data changes on falling edge, sample on rising edge */
#define QMSPI_SPI_MODE0             0
/* Mode 1: Clock idle = Low. Data changes on rising edge, sample on falling edge */
#define QMSPI_SPI_MODE1             0x06
/* Mode 2: Clock idle = High. Data changes on rising edge, sample on falling edge */
#define QMSPI_SPI_MODE2             0x06
/* Mode 3: Clock idle = High. Data changes on falling edge, sample on rising edge */
#define QMSPI_SPI_MODE3             0x07

/* DMA Main */
#define QMSPI_DMA_MAIN_ACT_ADDR     0x40002400ul
#define QMSPI_DMA_MAIN_PKT_ADDR     0x40002404ul

/* Choose DMA channel 10 for QMSPI transmit */
#define QMSPI_TX_DMA_CHAN           10
#define QMSPI_TX_DMA_ACT_ADDR       0x400026C0ul
#define QMSPI_TX_DMA_MSTART_ADDR    0x400026C4ul
#define QMSPI_TX_DMA_MEND_ADDR      0x400026C8ul
#define QMSPI_TX_DMA_DEV_ADDR       0x400026CCul
#define QMSPI_TX_DMA_CTRL_ADDR      0x400026D0ul
#define QMSPI_TX_DMA_ISTS_ADDR      0x400026D4ul
#define QMSPI_TX_DMA_IEN_ADDR       0x400026D8ul
#define QMSPI_TX_DMA_FSM_ADDR       0x400026DCul

/* Choose DMA channel 11 for QMSPI receive */
#define QMSPI_RX_DMA_CHAN           11
#define QMSPI_RX_DMA_ACT_ADDR       0x40002700ul
#define QMSPI_RX_DMA_MSTART_ADDR    0x40002704ul
#define QMSPI_RX_DMA_MEND_ADDR      0x40002708ul
#define QMSPI_RX_DMA_DEV_ADDR       0x4000270Cul
#define QMSPI_RX_DMA_CTRL_ADDR      0x40002710ul
#define QMSPI_RX_DMA_ISTS_ADDR      0x40002714ul
#define QMSPI_RX_DMA_IEN_ADDR       0x40002718ul
#define QMSPI_RX_DMA_FSM_ADDR       0x4000271Cul

// Device ID used with DMA block
#define QMSPI_TX_DMA_REQ_ID         10u
#define QMSPI_RX_DMA_REQ_ID         11u

/*
 * QMSPI DMA ID and DMA direction 8-bit value
 * b[7:1] = QMSPI DMA ID
 * b[0] = direction
 *      = 0 dev to mem (read from device write to memory)
 *      = 1 mem to dev (read from memory write to device)
 */
#define QMSPI_TX_DMA_DEVDIR     ((QMSPI_TX_DMA_REQ_ID << 1) + 0x01)
#define QMSPI_RX_DMA_DEVDIR     ((QMSPI_RX_DMA_REQ_ID << 1) + 0x00)

/* Default DMA.Control values without unit length
 * b[16]=1 increment memory address
 * b[15:9]=QMSPI DMA ID for TX or RX
 * b[8]=direction: 1(Mem2Device), 0(Dev2Mem)
 * b[0]=1 RUN
 */
#define QMSPI_TX_DMA_CTRL   ((1ul << 16) + (12ul << 9) + (1ul << 8) + (1ul << 0))
#define QMSPI_RX_DMA_CTRL   ((1ul << 16) + (13ul << 9) + (0ul << 8) + (1ul << 0))

#define QMSPI_TX_FIFO_LEN           8
#define QMSPI_RX_FIFO_LEN           8

#define QMSPI_M_ACT_SRST_OFS        0
#define QMSPI_M_SPI_MODE_OFS        1
#define QMSPI_M_CLK_DIV_OFS         2
#define QMSPI_CTRL_OFS              4
#define QMSPI_EXE_OFS               8
#define QMSPI_IF_CTRL_OFS           0x0C
#define QMSPI_STS_OFS               0x10
#define QMSPI_BUF_CNT_STS_OFS       0x14
#define QMSPI_IEN_OFS               0x18
#define QMSPI_BUF_CNT_TRIG_OFS      0x1C
#define QMSPI_TX_FIFO_OFS           0x20
#define QMSPI_RX_FIFO_OFS           0x24
#define QMSPI_CSTM_OFS              0x28
/* 0 <= n < QMSPI_MAX_DESCR */
#define QMSPI_DESCR_OFS(n)          (0x30 + ((uint32_t)(n) << 2))

#define QMSPI_MODE_ADDR             (QMSPI_BASE_ADDR + 0x00)
#define QMSPI_CTRL_ADDR             (QMSPI_BASE_ADDR + 0x04)
#define QMSPI_EXE_ADDR              (QMSPI_BASE_ADDR + 0x08)
#define QMSPI_IFC_ADDR              (QMSPI_BASE_ADDR + 0x0C)
#define QMSPI_STS_ADDR              (QMSPI_BASE_ADDR + 0x10)
#define QMSPI_BUFCNT_STS_ADDR       (QMSPI_BASE_ADDR + 0x14)
#define QMSPI_TX_BCNT_STS_ADDR      (QMSPI_BASE_ADDR + 0x14)
#define QMSPI_RX_BCNT_STS_ADDR      (QMSPI_BASE_ADDR + 0x16)
#define QMSPI_IEN_ADDR              (QMSPI_BASE_ADDR + 0x18)
#define QMSPI_TXB_ADDR              (QMSPI_BASE_ADDR + 0x20)
#define QMSPI_RXB_ADDR              (QMSPI_BASE_ADDR + 0x24)
#define QMSPI_CSTM_ADDR             (QMSPI_BASE_ADDR + 0x28)
#define QMSPI_DESCR_ADDR(n)         (QMSPI_BASE_ADDR + (0x30 + ((uint32_t)(n) << 2)))

// Mode Register
#define QMSPI_M_SRST            0x02
#define QMSPI_M_ACTIVATE        0x01
#define QMSPI_M_SIG_POS         8
#define QMSPI_M_SIG_MASK0       0x07
#define QMSPI_M_SIG_MASK        0x0700ul
#define QMSPI_M_SIG_MODE0_VAL   0x00
#define QMSPI_M_SIG_MODE1_VAL   0x06
#define QMSPI_M_SIG_MODE2_VAL   0x01
#define QMSPI_M_SIG_MODE3_VAL   0x07
#define QMSPI_M_SIG_MODE0       (0x00 << QMSPI_M_SIG_POS)
#define QMSPI_M_SIG_MODE1       (0x06 << QMSPI_M_SIG_POS)
#define QMSPI_M_SIG_MODE2       (0x01 << QMSPI_M_SIG_POS)
#define QMSPI_M_SIG_MODE3       (0x07 << QMSPI_M_SIG_POS)
#define QMSPI_M_CS_POS          12
#define QMSPI_M_CS_MASK0        0x03
#define QMSPI_M_CS_MASK         (0x03ul << 12)
#define QMSPI_M_CS0             (0x00ul << 12)
#define QMSPI_M_CS1             (0x01ul << 12)
#define QMSPI_M_CS(n)           (((n) & QMSPI_M_CS_MASK0) << QMSPI_M_CS_POS)
#define QMSPI_M_FDIV_POS        16
#define QMSPI_M_FDIV_MASK0      0xFF
#define QMSPI_M_FDIV_MASK       0x00FF0000

// Control/Descriptors
#define QMSPI_C_IFM_MASK        0x03ul
#define QMSPI_C_IFM_1X          0x00ul
#define QMSPI_C_IFM_2X          0x01ul
#define QMSPI_C_IFM_4X          0x02ul
#define QMSPI_C_TX_MASK         (0x03ul << 2)
#define QMSPI_C_TX_DIS          (0x00ul << 2)
#define QMSPI_C_TX_DATA         (0x01ul << 2)
#define QMSPI_C_TX_ZEROS        (0x02ul << 2)
#define QMSPI_C_TX_ONES         (0x03ul << 2)
#define QMSPI_C_TX_DMA_POS      4
#define QMSPI_C_TX_DMA_MASK     (0x03ul << 4)
#define QMSPI_C_TX_DMA_DIS      (0x00ul << 4)
#define QMSPI_C_TX_DMA_1B       (0x01ul << 4)
#define QMSPI_C_TX_DMA_2B       (0x02ul << 4)
#define QMSPI_C_TX_DMA_4B       (0x03ul << 4)
#define QMSPI_C_RX_DIS          (0ul << 6)
#define QMSPI_C_RX_EN           (1ul << 6)
#define QMSPI_C_RX_DMA_POS      7
#define QMSPI_C_RX_DMA_MASK     (0x03ul << 7)
#define QMSPI_C_RX_DMA_DIS      (0x00ul << 7)
#define QMSPI_C_RX_DMA_1B       (0x01ul << 7)
#define QMSPI_C_RX_DMA_2B       (0x02ul << 7)
#define QMSPI_C_RX_DMA_4B       (0x03ul << 7)
#define QMSPI_C_NO_CLOSE        (0ul << 9)
#define QMSPI_C_CLOSE           (1ul << 9)
#define QMSPI_C_XFR_UNITS_POS   10u
#define QMSPI_C_XFR_UNITS_MASK  (0x03ul << 10)
#define QMSPI_C_XFR_UNITS_BITS  (0x00ul << 10)
#define QMSPI_C_XFR_UNITS_1     (0x01ul << 10)
#define QMSPI_C_XFR_UNITS_4     (0x02ul << 10)
#define QMSPI_C_XFR_UNITS_16    (0x03ul << 10)
#define QMSPI_C_NEXT_DESCR_POS  12
#define QMSPI_C_NEXT_DESCR_MASK0    0x0Ful
#define QMSPI_C_NEXT_DESCR_MASK (0x0Ful << 12)
#define QMSPI_C_DESCR0          (0 << 12)
#define QMSPI_C_DESCR1          (1ul << 12)
#define QMSPI_C_DESCR2          (2ul << 12)
#define QMSPI_C_DESCR3          (3ul << 12)
#define QMSPI_C_DESCR4          (4ul << 12)
#define QMSPI_C_DESCR(n)        (((uint32_t)(n) & 0x0f) << 12)
#define QMSPI_C_NEXT_DESCR(n)   (((uint32_t)(n) & 0x0f) << 12)
#define QMSPI_C_DESCR_EN_POS    16
#define QMSPI_C_DESCR_EN        (1ul << 16)
#define QMSPI_C_DESCR_LAST      (1ul << 16)
#define QMSPI_C_MAX_UNITS       0x7FFFul
#define QMSPI_C_MAX_UNITS_MASK  0x7FFFul
#define QMSPI_C_XFR_NUNITS_POS  17
#define QMSPI_C_XFR_NUNITS_MASK (0x7FFFul << 17)
#define QMSPI_C_XFR_NUNITS(n)       ((uint32_t)(n) << 17)
#define QMSPI_C_XFR_NUNITS_GET(n)   (((uint32_t)(n) >> 17) & QMSPI_C_MAX_UNITS_MASK)

/* Exe */
#define QMSPI_EXE_START         0x01
#define QMSPI_EXE_STOP          0x02
#define QMSPI_EXE_CLR_FIFOS     0x04

/* Interface Control */
#define QMSPI_IFC_DFLT          0x00

/* Status Register */
#define QMSPI_STS_DONE          (1ul << 0)
#define QMSPI_STS_DMA_DONE      (1ul << 1)
#define QMSPI_STS_TXB_ERR       (1ul << 2)
#define QMSPI_STS_RXB_ERR       (1ul << 3)
#define QMSPI_STS_PROG_ERR      (1ul << 4)
#define QMSPI_STS_TXBF          (1ul << 8)
#define QMSPI_STS_TXBE          (1ul << 9)
#define QMSPI_STS_TXBR          (1ul << 10)
#define QMSPI_STS_TXBS          (1ul << 11)
#define QMSPI_STS_RXBF          (1ul << 12)
#define QMSPI_STS_RXBE          (1ul << 13)
#define QMSPI_STS_RXBR          (1ul << 14)
#define QMSPI_STS_RXBS          (1ul << 15)
#define QMSPI_STS_ACTIVE        (1ul << 16)

/* Buffer Count Status (RO) */
#define QMSPI_TX_BUF_CNT_STS_POS    0
#define QMSPI_RX_BUF_CNT_STS_POS    16

/* Interrupt Enable Register */
#define QMSPI_IEN_XFR_DONE      (1ul << 0)
#define QMSPI_IEN_DMA_DONE      (1ul << 1)
#define QMSPI_IEN_TXB_ERR       (1ul << 2)
#define QMSPI_IEN_RXB_ERR       (1ul << 3)
#define QMSPI_IEN_PROG_ERR      (1ul << 4)
#define QMSPI_IEN_TXB_FULL      (1ul << 8)
#define QMSPI_IEN_TXB_EMPTY     (1ul << 9)
#define QMSPI_IEN_TXB_REQ       (1ul << 10)
#define QMSPI_IEN_RXB_FULL      (1ul << 12)
#define QMSPI_IEN_RXB_EMPTY     (1ul << 13)
#define QMSPI_IEN_RXB_REQ       (1ul << 14)

/* Buffer Count Trigger (RW) */
#define QMSPI_TX_BUF_CNT_TRIG_POS   0
#define QMSPI_RX_BUF_CNT_TRIG_POS   16

/* Chip Select Timing (RW) */
#define QMSPI_CSTM_MASK                 0xFF0F0F0Ful
#define QMSPI_CSTM_DFLT                 0x06060406ul
#define QMSPI_DLY_CS_ON_CK_STR_POS      0
#define QMSPI_DLY_CS_ON_CK_STR_MASK     0x0Ful
#define QMSPI_DLY_CK_STP_CS_OFF_POS     8
#define QMSPI_DLY_CK_STP_CS_OFF_MASK    (0x0Ful << 8)
#define QMSPI_DLY_LST_DAT_HLD_POS       16
#define QMSPI_DLY_LST_DAT_HLD_MASK      (0x0Ful << 16)
#define QMSPI_DLY_CS_OFF_CS_ON_POS      24
#define QMSPI_DLY_CS_OFF_CS_ON_MASK     (0x0Ful << 24)

#define QMSPI_PORT_MAX_IO_PINS  4
#define QMSPI_PORT_MAX_CS       4

/* Full duplex and Dual I/O:
 * CS#, CLK, IO0(MOSI), IO1(MISO)
 * do not connect IO2(WP#) or IO3(HOLD#) to QMSPI.
 */
#define QMSPI_PORT_MASK_FULL_DUPLEX     0x0F
#define QMSPI_PORT_MASK_DUAL            0x0F

#define QMSPI_PIN_IO0_POS   0
#define QMSPI_PIN_IO1_POS   1
#define QMSPI_PIN_IO2_POS   2
#define QMSPI_PIN_IO3_POS   3
#define QMSPI_PIN_CLK_POS   4
#define QMSPI_PIN_CS0_POS   8
#define QMSPI_PIN_CS1_POS   9

#define QMSPI_PIN_IO0       (1ul << QMSPI_PIN_IO0_POS)
#define QMSPI_PIN_IO1       (1ul << QMSPI_PIN_IO1_POS)
#define QMSPI_PIN_IO2       (1ul << QMSPI_PIN_IO2_POS)
#define QMSPI_PIN_IO3       (1ul << QMSPI_PIN_IO3_POS)
#define QMSPI_PIN_CLK       (1ul << QMSPI_PIN_CLK_POS)
#define QMSPI_PIN_CS0       (1ul << QMSPI_PIN_CS0_POS)
#define QMSPI_PIN_CS1       (1ul << QMSPI_PIN_CS1_POS)


/* =========================================================================*/
/* ================            QMSPI                       ================ */
/* =========================================================================*/
#define QMSPI_MAX_DESCRIPTOR    16

typedef union
{
    uint32_t u32;
    uint16_t u16[2];
    uint8_t  u8[4];
} QMSPI_TX_FIFO;

typedef union
{
    uint32_t u32;
    uint16_t u16[2];
    uint8_t  u8[4];
} QMSPI_RX_FIFO;

typedef struct
{
    __IOM uint32_t u32;
} QMSPI_DESCR;

/**
  * @brief Quad Master SPI (QMSPI)
  */
typedef struct
{                       /*!< (@ 0x40070000) QMSPI Structure   */
    __IOM uint32_t  MODE;           /*!< (@ 0x00000000) QMSPI Mode */
    __IOM uint32_t  CTRL;           /*!< (@ 0x00000004) QMSPI Control */
    __IOM uint32_t  EXE;            /*!< (@ 0x00000008) QMSPI Execute */
    __IOM uint32_t  IFCTRL;         /*!< (@ 0x0000000C) QMSPI Interface control */
    __IOM uint32_t  STS;            /*!< (@ 0x00000010) QMSPI Status */
    __IOM uint32_t  BCNT_STS;       /*!< (@ 0x00000014) QMSPI Buffer Count Status (RO) */
    __IOM uint32_t  IEN;            /*!< (@ 0x00000018) QMSPI Interrupt Enable */
    __IOM uint32_t  BCNT_TRIG;      /*!< (@ 0x0000001C) QMSPI Buffer Count Trigger */
    QMSPI_TX_FIFO   TX_FIFO;        /*!< (@ 0x00000020) QMSPI TX FIFO */
    QMSPI_RX_FIFO   RX_FIFO;        /*!< (@ 0x00000024) QMSPI RX FIFO */
    __IOM uint32_t  CSTM;           /*!< (@ 0x00000028) QMSPI CS Timing */
          uint8_t   RSVD1[4];
    QMSPI_DESCR     DESCR[QMSPI_MAX_DESCRIPTOR]; /*!< (@ 0x00000030) QMSPI Descriptors 0-15 */
} QMSPI_Type;

#endif /* #ifndef _QMSPI_H */
/* end qmspi.h */
/**   @}
 */
