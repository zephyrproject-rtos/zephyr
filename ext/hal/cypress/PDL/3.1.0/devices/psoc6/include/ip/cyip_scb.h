/***************************************************************************//**
* \file cyip_scb.h
*
* \brief
* SCB IP definitions
*
* \note
* Generator version: 1.3.0.1146
* Database revision: rev#1050929
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation. All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

#ifndef _CYIP_SCB_H_
#define _CYIP_SCB_H_

#include "cyip_headers.h"

/*******************************************************************************
*                                     SCB
*******************************************************************************/

#define SCB_SECTION_SIZE                        0x00010000UL

/**
  * \brief Serial Communications Block (SPI/UART/I2C) (CySCB)
  */
typedef struct {
  __IOM uint32_t CTRL;                          /*!< 0x00000000 Generic control */
   __IM uint32_t STATUS;                        /*!< 0x00000004 Generic status */
  __IOM uint32_t CMD_RESP_CTRL;                 /*!< 0x00000008 Command/response control */
   __IM uint32_t CMD_RESP_STATUS;               /*!< 0x0000000C Command/response status */
   __IM uint32_t RESERVED[4];
  __IOM uint32_t SPI_CTRL;                      /*!< 0x00000020 SPI control */
   __IM uint32_t SPI_STATUS;                    /*!< 0x00000024 SPI status */
   __IM uint32_t RESERVED1[6];
  __IOM uint32_t UART_CTRL;                     /*!< 0x00000040 UART control */
  __IOM uint32_t UART_TX_CTRL;                  /*!< 0x00000044 UART transmitter control */
  __IOM uint32_t UART_RX_CTRL;                  /*!< 0x00000048 UART receiver control */
   __IM uint32_t UART_RX_STATUS;                /*!< 0x0000004C UART receiver status */
  __IOM uint32_t UART_FLOW_CTRL;                /*!< 0x00000050 UART flow control */
   __IM uint32_t RESERVED2[3];
  __IOM uint32_t I2C_CTRL;                      /*!< 0x00000060 I2C control */
   __IM uint32_t I2C_STATUS;                    /*!< 0x00000064 I2C status */
  __IOM uint32_t I2C_M_CMD;                     /*!< 0x00000068 I2C master command */
  __IOM uint32_t I2C_S_CMD;                     /*!< 0x0000006C I2C slave command */
  __IOM uint32_t I2C_CFG;                       /*!< 0x00000070 I2C configuration */
   __IM uint32_t RESERVED3[35];
  __IOM uint32_t DDFT_CTRL;                     /*!< 0x00000100 Digital DfT control */
   __IM uint32_t RESERVED4[63];
  __IOM uint32_t TX_CTRL;                       /*!< 0x00000200 Transmitter control */
  __IOM uint32_t TX_FIFO_CTRL;                  /*!< 0x00000204 Transmitter FIFO control */
   __IM uint32_t TX_FIFO_STATUS;                /*!< 0x00000208 Transmitter FIFO status */
   __IM uint32_t RESERVED5[13];
   __OM uint32_t TX_FIFO_WR;                    /*!< 0x00000240 Transmitter FIFO write */
   __IM uint32_t RESERVED6[47];
  __IOM uint32_t RX_CTRL;                       /*!< 0x00000300 Receiver control */
  __IOM uint32_t RX_FIFO_CTRL;                  /*!< 0x00000304 Receiver FIFO control */
   __IM uint32_t RX_FIFO_STATUS;                /*!< 0x00000308 Receiver FIFO status */
   __IM uint32_t RESERVED7;
  __IOM uint32_t RX_MATCH;                      /*!< 0x00000310 Slave address and mask */
   __IM uint32_t RESERVED8[11];
   __IM uint32_t RX_FIFO_RD;                    /*!< 0x00000340 Receiver FIFO read */
   __IM uint32_t RX_FIFO_RD_SILENT;             /*!< 0x00000344 Receiver FIFO read silent */
   __IM uint32_t RESERVED9[46];
  __IOM uint32_t EZ_DATA[512];                  /*!< 0x00000400 Memory buffer */
   __IM uint32_t RESERVED10[128];
   __IM uint32_t INTR_CAUSE;                    /*!< 0x00000E00 Active clocked interrupt signal */
   __IM uint32_t RESERVED11[31];
  __IOM uint32_t INTR_I2C_EC;                   /*!< 0x00000E80 Externally clocked I2C interrupt request */
   __IM uint32_t RESERVED12;
  __IOM uint32_t INTR_I2C_EC_MASK;              /*!< 0x00000E88 Externally clocked I2C interrupt mask */
   __IM uint32_t INTR_I2C_EC_MASKED;            /*!< 0x00000E8C Externally clocked I2C interrupt masked */
   __IM uint32_t RESERVED13[12];
  __IOM uint32_t INTR_SPI_EC;                   /*!< 0x00000EC0 Externally clocked SPI interrupt request */
   __IM uint32_t RESERVED14;
  __IOM uint32_t INTR_SPI_EC_MASK;              /*!< 0x00000EC8 Externally clocked SPI interrupt mask */
   __IM uint32_t INTR_SPI_EC_MASKED;            /*!< 0x00000ECC Externally clocked SPI interrupt masked */
   __IM uint32_t RESERVED15[12];
  __IOM uint32_t INTR_M;                        /*!< 0x00000F00 Master interrupt request */
  __IOM uint32_t INTR_M_SET;                    /*!< 0x00000F04 Master interrupt set request */
  __IOM uint32_t INTR_M_MASK;                   /*!< 0x00000F08 Master interrupt mask */
   __IM uint32_t INTR_M_MASKED;                 /*!< 0x00000F0C Master interrupt masked request */
   __IM uint32_t RESERVED16[12];
  __IOM uint32_t INTR_S;                        /*!< 0x00000F40 Slave interrupt request */
  __IOM uint32_t INTR_S_SET;                    /*!< 0x00000F44 Slave interrupt set request */
  __IOM uint32_t INTR_S_MASK;                   /*!< 0x00000F48 Slave interrupt mask */
   __IM uint32_t INTR_S_MASKED;                 /*!< 0x00000F4C Slave interrupt masked request */
   __IM uint32_t RESERVED17[12];
  __IOM uint32_t INTR_TX;                       /*!< 0x00000F80 Transmitter interrupt request */
  __IOM uint32_t INTR_TX_SET;                   /*!< 0x00000F84 Transmitter interrupt set request */
  __IOM uint32_t INTR_TX_MASK;                  /*!< 0x00000F88 Transmitter interrupt mask */
   __IM uint32_t INTR_TX_MASKED;                /*!< 0x00000F8C Transmitter interrupt masked request */
   __IM uint32_t RESERVED18[12];
  __IOM uint32_t INTR_RX;                       /*!< 0x00000FC0 Receiver interrupt request */
  __IOM uint32_t INTR_RX_SET;                   /*!< 0x00000FC4 Receiver interrupt set request */
  __IOM uint32_t INTR_RX_MASK;                  /*!< 0x00000FC8 Receiver interrupt mask */
   __IM uint32_t INTR_RX_MASKED;                /*!< 0x00000FCC Receiver interrupt masked request */
} CySCB_V1_Type;                                /*!< Size = 4048 (0xFD0) */


/* SCB.CTRL */
#define SCB_CTRL_OVS_Pos                        0UL
#define SCB_CTRL_OVS_Msk                        0xFUL
#define SCB_CTRL_EC_AM_MODE_Pos                 8UL
#define SCB_CTRL_EC_AM_MODE_Msk                 0x100UL
#define SCB_CTRL_EC_OP_MODE_Pos                 9UL
#define SCB_CTRL_EC_OP_MODE_Msk                 0x200UL
#define SCB_CTRL_EZ_MODE_Pos                    10UL
#define SCB_CTRL_EZ_MODE_Msk                    0x400UL
#define SCB_CTRL_BYTE_MODE_Pos                  11UL
#define SCB_CTRL_BYTE_MODE_Msk                  0x800UL
#define SCB_CTRL_CMD_RESP_MODE_Pos              12UL
#define SCB_CTRL_CMD_RESP_MODE_Msk              0x1000UL
#define SCB_CTRL_ADDR_ACCEPT_Pos                16UL
#define SCB_CTRL_ADDR_ACCEPT_Msk                0x10000UL
#define SCB_CTRL_BLOCK_Pos                      17UL
#define SCB_CTRL_BLOCK_Msk                      0x20000UL
#define SCB_CTRL_MODE_Pos                       24UL
#define SCB_CTRL_MODE_Msk                       0x3000000UL
#define SCB_CTRL_ENABLED_Pos                    31UL
#define SCB_CTRL_ENABLED_Msk                    0x80000000UL
/* SCB.STATUS */
#define SCB_STATUS_EC_BUSY_Pos                  0UL
#define SCB_STATUS_EC_BUSY_Msk                  0x1UL
/* SCB.CMD_RESP_CTRL */
#define SCB_CMD_RESP_CTRL_BASE_RD_ADDR_Pos      0UL
#define SCB_CMD_RESP_CTRL_BASE_RD_ADDR_Msk      0x1FFUL
#define SCB_CMD_RESP_CTRL_BASE_WR_ADDR_Pos      16UL
#define SCB_CMD_RESP_CTRL_BASE_WR_ADDR_Msk      0x1FF0000UL
/* SCB.CMD_RESP_STATUS */
#define SCB_CMD_RESP_STATUS_CURR_RD_ADDR_Pos    0UL
#define SCB_CMD_RESP_STATUS_CURR_RD_ADDR_Msk    0x1FFUL
#define SCB_CMD_RESP_STATUS_CURR_WR_ADDR_Pos    16UL
#define SCB_CMD_RESP_STATUS_CURR_WR_ADDR_Msk    0x1FF0000UL
#define SCB_CMD_RESP_STATUS_CMD_RESP_EC_BUS_BUSY_Pos 30UL
#define SCB_CMD_RESP_STATUS_CMD_RESP_EC_BUS_BUSY_Msk 0x40000000UL
#define SCB_CMD_RESP_STATUS_CMD_RESP_EC_BUSY_Pos 31UL
#define SCB_CMD_RESP_STATUS_CMD_RESP_EC_BUSY_Msk 0x80000000UL
/* SCB.SPI_CTRL */
#define SCB_SPI_CTRL_SSEL_CONTINUOUS_Pos        0UL
#define SCB_SPI_CTRL_SSEL_CONTINUOUS_Msk        0x1UL
#define SCB_SPI_CTRL_SELECT_PRECEDE_Pos         1UL
#define SCB_SPI_CTRL_SELECT_PRECEDE_Msk         0x2UL
#define SCB_SPI_CTRL_CPHA_Pos                   2UL
#define SCB_SPI_CTRL_CPHA_Msk                   0x4UL
#define SCB_SPI_CTRL_CPOL_Pos                   3UL
#define SCB_SPI_CTRL_CPOL_Msk                   0x8UL
#define SCB_SPI_CTRL_LATE_MISO_SAMPLE_Pos       4UL
#define SCB_SPI_CTRL_LATE_MISO_SAMPLE_Msk       0x10UL
#define SCB_SPI_CTRL_SCLK_CONTINUOUS_Pos        5UL
#define SCB_SPI_CTRL_SCLK_CONTINUOUS_Msk        0x20UL
#define SCB_SPI_CTRL_SSEL_POLARITY0_Pos         8UL
#define SCB_SPI_CTRL_SSEL_POLARITY0_Msk         0x100UL
#define SCB_SPI_CTRL_SSEL_POLARITY1_Pos         9UL
#define SCB_SPI_CTRL_SSEL_POLARITY1_Msk         0x200UL
#define SCB_SPI_CTRL_SSEL_POLARITY2_Pos         10UL
#define SCB_SPI_CTRL_SSEL_POLARITY2_Msk         0x400UL
#define SCB_SPI_CTRL_SSEL_POLARITY3_Pos         11UL
#define SCB_SPI_CTRL_SSEL_POLARITY3_Msk         0x800UL
#define SCB_SPI_CTRL_LOOPBACK_Pos               16UL
#define SCB_SPI_CTRL_LOOPBACK_Msk               0x10000UL
#define SCB_SPI_CTRL_MODE_Pos                   24UL
#define SCB_SPI_CTRL_MODE_Msk                   0x3000000UL
#define SCB_SPI_CTRL_SSEL_Pos                   26UL
#define SCB_SPI_CTRL_SSEL_Msk                   0xC000000UL
#define SCB_SPI_CTRL_MASTER_MODE_Pos            31UL
#define SCB_SPI_CTRL_MASTER_MODE_Msk            0x80000000UL
/* SCB.SPI_STATUS */
#define SCB_SPI_STATUS_BUS_BUSY_Pos             0UL
#define SCB_SPI_STATUS_BUS_BUSY_Msk             0x1UL
#define SCB_SPI_STATUS_SPI_EC_BUSY_Pos          1UL
#define SCB_SPI_STATUS_SPI_EC_BUSY_Msk          0x2UL
#define SCB_SPI_STATUS_CURR_EZ_ADDR_Pos         8UL
#define SCB_SPI_STATUS_CURR_EZ_ADDR_Msk         0xFF00UL
#define SCB_SPI_STATUS_BASE_EZ_ADDR_Pos         16UL
#define SCB_SPI_STATUS_BASE_EZ_ADDR_Msk         0xFF0000UL
/* SCB.UART_CTRL */
#define SCB_UART_CTRL_LOOPBACK_Pos              16UL
#define SCB_UART_CTRL_LOOPBACK_Msk              0x10000UL
#define SCB_UART_CTRL_MODE_Pos                  24UL
#define SCB_UART_CTRL_MODE_Msk                  0x3000000UL
/* SCB.UART_TX_CTRL */
#define SCB_UART_TX_CTRL_STOP_BITS_Pos          0UL
#define SCB_UART_TX_CTRL_STOP_BITS_Msk          0x7UL
#define SCB_UART_TX_CTRL_PARITY_Pos             4UL
#define SCB_UART_TX_CTRL_PARITY_Msk             0x10UL
#define SCB_UART_TX_CTRL_PARITY_ENABLED_Pos     5UL
#define SCB_UART_TX_CTRL_PARITY_ENABLED_Msk     0x20UL
#define SCB_UART_TX_CTRL_RETRY_ON_NACK_Pos      8UL
#define SCB_UART_TX_CTRL_RETRY_ON_NACK_Msk      0x100UL
/* SCB.UART_RX_CTRL */
#define SCB_UART_RX_CTRL_STOP_BITS_Pos          0UL
#define SCB_UART_RX_CTRL_STOP_BITS_Msk          0x7UL
#define SCB_UART_RX_CTRL_PARITY_Pos             4UL
#define SCB_UART_RX_CTRL_PARITY_Msk             0x10UL
#define SCB_UART_RX_CTRL_PARITY_ENABLED_Pos     5UL
#define SCB_UART_RX_CTRL_PARITY_ENABLED_Msk     0x20UL
#define SCB_UART_RX_CTRL_POLARITY_Pos           6UL
#define SCB_UART_RX_CTRL_POLARITY_Msk           0x40UL
#define SCB_UART_RX_CTRL_DROP_ON_PARITY_ERROR_Pos 8UL
#define SCB_UART_RX_CTRL_DROP_ON_PARITY_ERROR_Msk 0x100UL
#define SCB_UART_RX_CTRL_DROP_ON_FRAME_ERROR_Pos 9UL
#define SCB_UART_RX_CTRL_DROP_ON_FRAME_ERROR_Msk 0x200UL
#define SCB_UART_RX_CTRL_MP_MODE_Pos            10UL
#define SCB_UART_RX_CTRL_MP_MODE_Msk            0x400UL
#define SCB_UART_RX_CTRL_LIN_MODE_Pos           12UL
#define SCB_UART_RX_CTRL_LIN_MODE_Msk           0x1000UL
#define SCB_UART_RX_CTRL_SKIP_START_Pos         13UL
#define SCB_UART_RX_CTRL_SKIP_START_Msk         0x2000UL
#define SCB_UART_RX_CTRL_BREAK_WIDTH_Pos        16UL
#define SCB_UART_RX_CTRL_BREAK_WIDTH_Msk        0xF0000UL
/* SCB.UART_RX_STATUS */
#define SCB_UART_RX_STATUS_BR_COUNTER_Pos       0UL
#define SCB_UART_RX_STATUS_BR_COUNTER_Msk       0xFFFUL
/* SCB.UART_FLOW_CTRL */
#define SCB_UART_FLOW_CTRL_TRIGGER_LEVEL_Pos    0UL
#define SCB_UART_FLOW_CTRL_TRIGGER_LEVEL_Msk    0xFFUL
#define SCB_UART_FLOW_CTRL_RTS_POLARITY_Pos     16UL
#define SCB_UART_FLOW_CTRL_RTS_POLARITY_Msk     0x10000UL
#define SCB_UART_FLOW_CTRL_CTS_POLARITY_Pos     24UL
#define SCB_UART_FLOW_CTRL_CTS_POLARITY_Msk     0x1000000UL
#define SCB_UART_FLOW_CTRL_CTS_ENABLED_Pos      25UL
#define SCB_UART_FLOW_CTRL_CTS_ENABLED_Msk      0x2000000UL
/* SCB.I2C_CTRL */
#define SCB_I2C_CTRL_HIGH_PHASE_OVS_Pos         0UL
#define SCB_I2C_CTRL_HIGH_PHASE_OVS_Msk         0xFUL
#define SCB_I2C_CTRL_LOW_PHASE_OVS_Pos          4UL
#define SCB_I2C_CTRL_LOW_PHASE_OVS_Msk          0xF0UL
#define SCB_I2C_CTRL_M_READY_DATA_ACK_Pos       8UL
#define SCB_I2C_CTRL_M_READY_DATA_ACK_Msk       0x100UL
#define SCB_I2C_CTRL_M_NOT_READY_DATA_NACK_Pos  9UL
#define SCB_I2C_CTRL_M_NOT_READY_DATA_NACK_Msk  0x200UL
#define SCB_I2C_CTRL_S_GENERAL_IGNORE_Pos       11UL
#define SCB_I2C_CTRL_S_GENERAL_IGNORE_Msk       0x800UL
#define SCB_I2C_CTRL_S_READY_ADDR_ACK_Pos       12UL
#define SCB_I2C_CTRL_S_READY_ADDR_ACK_Msk       0x1000UL
#define SCB_I2C_CTRL_S_READY_DATA_ACK_Pos       13UL
#define SCB_I2C_CTRL_S_READY_DATA_ACK_Msk       0x2000UL
#define SCB_I2C_CTRL_S_NOT_READY_ADDR_NACK_Pos  14UL
#define SCB_I2C_CTRL_S_NOT_READY_ADDR_NACK_Msk  0x4000UL
#define SCB_I2C_CTRL_S_NOT_READY_DATA_NACK_Pos  15UL
#define SCB_I2C_CTRL_S_NOT_READY_DATA_NACK_Msk  0x8000UL
#define SCB_I2C_CTRL_LOOPBACK_Pos               16UL
#define SCB_I2C_CTRL_LOOPBACK_Msk               0x10000UL
#define SCB_I2C_CTRL_SLAVE_MODE_Pos             30UL
#define SCB_I2C_CTRL_SLAVE_MODE_Msk             0x40000000UL
#define SCB_I2C_CTRL_MASTER_MODE_Pos            31UL
#define SCB_I2C_CTRL_MASTER_MODE_Msk            0x80000000UL
/* SCB.I2C_STATUS */
#define SCB_I2C_STATUS_BUS_BUSY_Pos             0UL
#define SCB_I2C_STATUS_BUS_BUSY_Msk             0x1UL
#define SCB_I2C_STATUS_I2C_EC_BUSY_Pos          1UL
#define SCB_I2C_STATUS_I2C_EC_BUSY_Msk          0x2UL
#define SCB_I2C_STATUS_S_READ_Pos               4UL
#define SCB_I2C_STATUS_S_READ_Msk               0x10UL
#define SCB_I2C_STATUS_M_READ_Pos               5UL
#define SCB_I2C_STATUS_M_READ_Msk               0x20UL
#define SCB_I2C_STATUS_CURR_EZ_ADDR_Pos         8UL
#define SCB_I2C_STATUS_CURR_EZ_ADDR_Msk         0xFF00UL
#define SCB_I2C_STATUS_BASE_EZ_ADDR_Pos         16UL
#define SCB_I2C_STATUS_BASE_EZ_ADDR_Msk         0xFF0000UL
/* SCB.I2C_M_CMD */
#define SCB_I2C_M_CMD_M_START_Pos               0UL
#define SCB_I2C_M_CMD_M_START_Msk               0x1UL
#define SCB_I2C_M_CMD_M_START_ON_IDLE_Pos       1UL
#define SCB_I2C_M_CMD_M_START_ON_IDLE_Msk       0x2UL
#define SCB_I2C_M_CMD_M_ACK_Pos                 2UL
#define SCB_I2C_M_CMD_M_ACK_Msk                 0x4UL
#define SCB_I2C_M_CMD_M_NACK_Pos                3UL
#define SCB_I2C_M_CMD_M_NACK_Msk                0x8UL
#define SCB_I2C_M_CMD_M_STOP_Pos                4UL
#define SCB_I2C_M_CMD_M_STOP_Msk                0x10UL
/* SCB.I2C_S_CMD */
#define SCB_I2C_S_CMD_S_ACK_Pos                 0UL
#define SCB_I2C_S_CMD_S_ACK_Msk                 0x1UL
#define SCB_I2C_S_CMD_S_NACK_Pos                1UL
#define SCB_I2C_S_CMD_S_NACK_Msk                0x2UL
/* SCB.I2C_CFG */
#define SCB_I2C_CFG_SDA_IN_FILT_TRIM_Pos        0UL
#define SCB_I2C_CFG_SDA_IN_FILT_TRIM_Msk        0x3UL
#define SCB_I2C_CFG_SDA_IN_FILT_SEL_Pos         4UL
#define SCB_I2C_CFG_SDA_IN_FILT_SEL_Msk         0x10UL
#define SCB_I2C_CFG_SCL_IN_FILT_TRIM_Pos        8UL
#define SCB_I2C_CFG_SCL_IN_FILT_TRIM_Msk        0x300UL
#define SCB_I2C_CFG_SCL_IN_FILT_SEL_Pos         12UL
#define SCB_I2C_CFG_SCL_IN_FILT_SEL_Msk         0x1000UL
#define SCB_I2C_CFG_SDA_OUT_FILT0_TRIM_Pos      16UL
#define SCB_I2C_CFG_SDA_OUT_FILT0_TRIM_Msk      0x30000UL
#define SCB_I2C_CFG_SDA_OUT_FILT1_TRIM_Pos      18UL
#define SCB_I2C_CFG_SDA_OUT_FILT1_TRIM_Msk      0xC0000UL
#define SCB_I2C_CFG_SDA_OUT_FILT2_TRIM_Pos      20UL
#define SCB_I2C_CFG_SDA_OUT_FILT2_TRIM_Msk      0x300000UL
#define SCB_I2C_CFG_SDA_OUT_FILT_SEL_Pos        28UL
#define SCB_I2C_CFG_SDA_OUT_FILT_SEL_Msk        0x30000000UL
/* SCB.DDFT_CTRL */
#define SCB_DDFT_CTRL_DDFT_IN0_SEL_Pos          0UL
#define SCB_DDFT_CTRL_DDFT_IN0_SEL_Msk          0x1UL
#define SCB_DDFT_CTRL_DDFT_IN1_SEL_Pos          4UL
#define SCB_DDFT_CTRL_DDFT_IN1_SEL_Msk          0x10UL
#define SCB_DDFT_CTRL_DDFT_OUT0_SEL_Pos         16UL
#define SCB_DDFT_CTRL_DDFT_OUT0_SEL_Msk         0x70000UL
#define SCB_DDFT_CTRL_DDFT_OUT1_SEL_Pos         20UL
#define SCB_DDFT_CTRL_DDFT_OUT1_SEL_Msk         0x700000UL
/* SCB.TX_CTRL */
#define SCB_TX_CTRL_DATA_WIDTH_Pos              0UL
#define SCB_TX_CTRL_DATA_WIDTH_Msk              0xFUL
#define SCB_TX_CTRL_MSB_FIRST_Pos               8UL
#define SCB_TX_CTRL_MSB_FIRST_Msk               0x100UL
#define SCB_TX_CTRL_OPEN_DRAIN_Pos              16UL
#define SCB_TX_CTRL_OPEN_DRAIN_Msk              0x10000UL
/* SCB.TX_FIFO_CTRL */
#define SCB_TX_FIFO_CTRL_TRIGGER_LEVEL_Pos      0UL
#define SCB_TX_FIFO_CTRL_TRIGGER_LEVEL_Msk      0xFFUL
#define SCB_TX_FIFO_CTRL_CLEAR_Pos              16UL
#define SCB_TX_FIFO_CTRL_CLEAR_Msk              0x10000UL
#define SCB_TX_FIFO_CTRL_FREEZE_Pos             17UL
#define SCB_TX_FIFO_CTRL_FREEZE_Msk             0x20000UL
/* SCB.TX_FIFO_STATUS */
#define SCB_TX_FIFO_STATUS_USED_Pos             0UL
#define SCB_TX_FIFO_STATUS_USED_Msk             0x1FFUL
#define SCB_TX_FIFO_STATUS_SR_VALID_Pos         15UL
#define SCB_TX_FIFO_STATUS_SR_VALID_Msk         0x8000UL
#define SCB_TX_FIFO_STATUS_RD_PTR_Pos           16UL
#define SCB_TX_FIFO_STATUS_RD_PTR_Msk           0xFF0000UL
#define SCB_TX_FIFO_STATUS_WR_PTR_Pos           24UL
#define SCB_TX_FIFO_STATUS_WR_PTR_Msk           0xFF000000UL
/* SCB.TX_FIFO_WR */
#define SCB_TX_FIFO_WR_DATA_Pos                 0UL
#define SCB_TX_FIFO_WR_DATA_Msk                 0xFFFFUL
/* SCB.RX_CTRL */
#define SCB_RX_CTRL_DATA_WIDTH_Pos              0UL
#define SCB_RX_CTRL_DATA_WIDTH_Msk              0xFUL
#define SCB_RX_CTRL_MSB_FIRST_Pos               8UL
#define SCB_RX_CTRL_MSB_FIRST_Msk               0x100UL
#define SCB_RX_CTRL_MEDIAN_Pos                  9UL
#define SCB_RX_CTRL_MEDIAN_Msk                  0x200UL
/* SCB.RX_FIFO_CTRL */
#define SCB_RX_FIFO_CTRL_TRIGGER_LEVEL_Pos      0UL
#define SCB_RX_FIFO_CTRL_TRIGGER_LEVEL_Msk      0xFFUL
#define SCB_RX_FIFO_CTRL_CLEAR_Pos              16UL
#define SCB_RX_FIFO_CTRL_CLEAR_Msk              0x10000UL
#define SCB_RX_FIFO_CTRL_FREEZE_Pos             17UL
#define SCB_RX_FIFO_CTRL_FREEZE_Msk             0x20000UL
/* SCB.RX_FIFO_STATUS */
#define SCB_RX_FIFO_STATUS_USED_Pos             0UL
#define SCB_RX_FIFO_STATUS_USED_Msk             0x1FFUL
#define SCB_RX_FIFO_STATUS_SR_VALID_Pos         15UL
#define SCB_RX_FIFO_STATUS_SR_VALID_Msk         0x8000UL
#define SCB_RX_FIFO_STATUS_RD_PTR_Pos           16UL
#define SCB_RX_FIFO_STATUS_RD_PTR_Msk           0xFF0000UL
#define SCB_RX_FIFO_STATUS_WR_PTR_Pos           24UL
#define SCB_RX_FIFO_STATUS_WR_PTR_Msk           0xFF000000UL
/* SCB.RX_MATCH */
#define SCB_RX_MATCH_ADDR_Pos                   0UL
#define SCB_RX_MATCH_ADDR_Msk                   0xFFUL
#define SCB_RX_MATCH_MASK_Pos                   16UL
#define SCB_RX_MATCH_MASK_Msk                   0xFF0000UL
/* SCB.RX_FIFO_RD */
#define SCB_RX_FIFO_RD_DATA_Pos                 0UL
#define SCB_RX_FIFO_RD_DATA_Msk                 0xFFFFUL
/* SCB.RX_FIFO_RD_SILENT */
#define SCB_RX_FIFO_RD_SILENT_DATA_Pos          0UL
#define SCB_RX_FIFO_RD_SILENT_DATA_Msk          0xFFFFUL
/* SCB.EZ_DATA */
#define SCB_EZ_DATA_EZ_DATA_Pos                 0UL
#define SCB_EZ_DATA_EZ_DATA_Msk                 0xFFUL
/* SCB.INTR_CAUSE */
#define SCB_INTR_CAUSE_M_Pos                    0UL
#define SCB_INTR_CAUSE_M_Msk                    0x1UL
#define SCB_INTR_CAUSE_S_Pos                    1UL
#define SCB_INTR_CAUSE_S_Msk                    0x2UL
#define SCB_INTR_CAUSE_TX_Pos                   2UL
#define SCB_INTR_CAUSE_TX_Msk                   0x4UL
#define SCB_INTR_CAUSE_RX_Pos                   3UL
#define SCB_INTR_CAUSE_RX_Msk                   0x8UL
#define SCB_INTR_CAUSE_I2C_EC_Pos               4UL
#define SCB_INTR_CAUSE_I2C_EC_Msk               0x10UL
#define SCB_INTR_CAUSE_SPI_EC_Pos               5UL
#define SCB_INTR_CAUSE_SPI_EC_Msk               0x20UL
/* SCB.INTR_I2C_EC */
#define SCB_INTR_I2C_EC_WAKE_UP_Pos             0UL
#define SCB_INTR_I2C_EC_WAKE_UP_Msk             0x1UL
#define SCB_INTR_I2C_EC_EZ_STOP_Pos             1UL
#define SCB_INTR_I2C_EC_EZ_STOP_Msk             0x2UL
#define SCB_INTR_I2C_EC_EZ_WRITE_STOP_Pos       2UL
#define SCB_INTR_I2C_EC_EZ_WRITE_STOP_Msk       0x4UL
#define SCB_INTR_I2C_EC_EZ_READ_STOP_Pos        3UL
#define SCB_INTR_I2C_EC_EZ_READ_STOP_Msk        0x8UL
/* SCB.INTR_I2C_EC_MASK */
#define SCB_INTR_I2C_EC_MASK_WAKE_UP_Pos        0UL
#define SCB_INTR_I2C_EC_MASK_WAKE_UP_Msk        0x1UL
#define SCB_INTR_I2C_EC_MASK_EZ_STOP_Pos        1UL
#define SCB_INTR_I2C_EC_MASK_EZ_STOP_Msk        0x2UL
#define SCB_INTR_I2C_EC_MASK_EZ_WRITE_STOP_Pos  2UL
#define SCB_INTR_I2C_EC_MASK_EZ_WRITE_STOP_Msk  0x4UL
#define SCB_INTR_I2C_EC_MASK_EZ_READ_STOP_Pos   3UL
#define SCB_INTR_I2C_EC_MASK_EZ_READ_STOP_Msk   0x8UL
/* SCB.INTR_I2C_EC_MASKED */
#define SCB_INTR_I2C_EC_MASKED_WAKE_UP_Pos      0UL
#define SCB_INTR_I2C_EC_MASKED_WAKE_UP_Msk      0x1UL
#define SCB_INTR_I2C_EC_MASKED_EZ_STOP_Pos      1UL
#define SCB_INTR_I2C_EC_MASKED_EZ_STOP_Msk      0x2UL
#define SCB_INTR_I2C_EC_MASKED_EZ_WRITE_STOP_Pos 2UL
#define SCB_INTR_I2C_EC_MASKED_EZ_WRITE_STOP_Msk 0x4UL
#define SCB_INTR_I2C_EC_MASKED_EZ_READ_STOP_Pos 3UL
#define SCB_INTR_I2C_EC_MASKED_EZ_READ_STOP_Msk 0x8UL
/* SCB.INTR_SPI_EC */
#define SCB_INTR_SPI_EC_WAKE_UP_Pos             0UL
#define SCB_INTR_SPI_EC_WAKE_UP_Msk             0x1UL
#define SCB_INTR_SPI_EC_EZ_STOP_Pos             1UL
#define SCB_INTR_SPI_EC_EZ_STOP_Msk             0x2UL
#define SCB_INTR_SPI_EC_EZ_WRITE_STOP_Pos       2UL
#define SCB_INTR_SPI_EC_EZ_WRITE_STOP_Msk       0x4UL
#define SCB_INTR_SPI_EC_EZ_READ_STOP_Pos        3UL
#define SCB_INTR_SPI_EC_EZ_READ_STOP_Msk        0x8UL
/* SCB.INTR_SPI_EC_MASK */
#define SCB_INTR_SPI_EC_MASK_WAKE_UP_Pos        0UL
#define SCB_INTR_SPI_EC_MASK_WAKE_UP_Msk        0x1UL
#define SCB_INTR_SPI_EC_MASK_EZ_STOP_Pos        1UL
#define SCB_INTR_SPI_EC_MASK_EZ_STOP_Msk        0x2UL
#define SCB_INTR_SPI_EC_MASK_EZ_WRITE_STOP_Pos  2UL
#define SCB_INTR_SPI_EC_MASK_EZ_WRITE_STOP_Msk  0x4UL
#define SCB_INTR_SPI_EC_MASK_EZ_READ_STOP_Pos   3UL
#define SCB_INTR_SPI_EC_MASK_EZ_READ_STOP_Msk   0x8UL
/* SCB.INTR_SPI_EC_MASKED */
#define SCB_INTR_SPI_EC_MASKED_WAKE_UP_Pos      0UL
#define SCB_INTR_SPI_EC_MASKED_WAKE_UP_Msk      0x1UL
#define SCB_INTR_SPI_EC_MASKED_EZ_STOP_Pos      1UL
#define SCB_INTR_SPI_EC_MASKED_EZ_STOP_Msk      0x2UL
#define SCB_INTR_SPI_EC_MASKED_EZ_WRITE_STOP_Pos 2UL
#define SCB_INTR_SPI_EC_MASKED_EZ_WRITE_STOP_Msk 0x4UL
#define SCB_INTR_SPI_EC_MASKED_EZ_READ_STOP_Pos 3UL
#define SCB_INTR_SPI_EC_MASKED_EZ_READ_STOP_Msk 0x8UL
/* SCB.INTR_M */
#define SCB_INTR_M_I2C_ARB_LOST_Pos             0UL
#define SCB_INTR_M_I2C_ARB_LOST_Msk             0x1UL
#define SCB_INTR_M_I2C_NACK_Pos                 1UL
#define SCB_INTR_M_I2C_NACK_Msk                 0x2UL
#define SCB_INTR_M_I2C_ACK_Pos                  2UL
#define SCB_INTR_M_I2C_ACK_Msk                  0x4UL
#define SCB_INTR_M_I2C_STOP_Pos                 4UL
#define SCB_INTR_M_I2C_STOP_Msk                 0x10UL
#define SCB_INTR_M_I2C_BUS_ERROR_Pos            8UL
#define SCB_INTR_M_I2C_BUS_ERROR_Msk            0x100UL
#define SCB_INTR_M_SPI_DONE_Pos                 9UL
#define SCB_INTR_M_SPI_DONE_Msk                 0x200UL
/* SCB.INTR_M_SET */
#define SCB_INTR_M_SET_I2C_ARB_LOST_Pos         0UL
#define SCB_INTR_M_SET_I2C_ARB_LOST_Msk         0x1UL
#define SCB_INTR_M_SET_I2C_NACK_Pos             1UL
#define SCB_INTR_M_SET_I2C_NACK_Msk             0x2UL
#define SCB_INTR_M_SET_I2C_ACK_Pos              2UL
#define SCB_INTR_M_SET_I2C_ACK_Msk              0x4UL
#define SCB_INTR_M_SET_I2C_STOP_Pos             4UL
#define SCB_INTR_M_SET_I2C_STOP_Msk             0x10UL
#define SCB_INTR_M_SET_I2C_BUS_ERROR_Pos        8UL
#define SCB_INTR_M_SET_I2C_BUS_ERROR_Msk        0x100UL
#define SCB_INTR_M_SET_SPI_DONE_Pos             9UL
#define SCB_INTR_M_SET_SPI_DONE_Msk             0x200UL
/* SCB.INTR_M_MASK */
#define SCB_INTR_M_MASK_I2C_ARB_LOST_Pos        0UL
#define SCB_INTR_M_MASK_I2C_ARB_LOST_Msk        0x1UL
#define SCB_INTR_M_MASK_I2C_NACK_Pos            1UL
#define SCB_INTR_M_MASK_I2C_NACK_Msk            0x2UL
#define SCB_INTR_M_MASK_I2C_ACK_Pos             2UL
#define SCB_INTR_M_MASK_I2C_ACK_Msk             0x4UL
#define SCB_INTR_M_MASK_I2C_STOP_Pos            4UL
#define SCB_INTR_M_MASK_I2C_STOP_Msk            0x10UL
#define SCB_INTR_M_MASK_I2C_BUS_ERROR_Pos       8UL
#define SCB_INTR_M_MASK_I2C_BUS_ERROR_Msk       0x100UL
#define SCB_INTR_M_MASK_SPI_DONE_Pos            9UL
#define SCB_INTR_M_MASK_SPI_DONE_Msk            0x200UL
/* SCB.INTR_M_MASKED */
#define SCB_INTR_M_MASKED_I2C_ARB_LOST_Pos      0UL
#define SCB_INTR_M_MASKED_I2C_ARB_LOST_Msk      0x1UL
#define SCB_INTR_M_MASKED_I2C_NACK_Pos          1UL
#define SCB_INTR_M_MASKED_I2C_NACK_Msk          0x2UL
#define SCB_INTR_M_MASKED_I2C_ACK_Pos           2UL
#define SCB_INTR_M_MASKED_I2C_ACK_Msk           0x4UL
#define SCB_INTR_M_MASKED_I2C_STOP_Pos          4UL
#define SCB_INTR_M_MASKED_I2C_STOP_Msk          0x10UL
#define SCB_INTR_M_MASKED_I2C_BUS_ERROR_Pos     8UL
#define SCB_INTR_M_MASKED_I2C_BUS_ERROR_Msk     0x100UL
#define SCB_INTR_M_MASKED_SPI_DONE_Pos          9UL
#define SCB_INTR_M_MASKED_SPI_DONE_Msk          0x200UL
/* SCB.INTR_S */
#define SCB_INTR_S_I2C_ARB_LOST_Pos             0UL
#define SCB_INTR_S_I2C_ARB_LOST_Msk             0x1UL
#define SCB_INTR_S_I2C_NACK_Pos                 1UL
#define SCB_INTR_S_I2C_NACK_Msk                 0x2UL
#define SCB_INTR_S_I2C_ACK_Pos                  2UL
#define SCB_INTR_S_I2C_ACK_Msk                  0x4UL
#define SCB_INTR_S_I2C_WRITE_STOP_Pos           3UL
#define SCB_INTR_S_I2C_WRITE_STOP_Msk           0x8UL
#define SCB_INTR_S_I2C_STOP_Pos                 4UL
#define SCB_INTR_S_I2C_STOP_Msk                 0x10UL
#define SCB_INTR_S_I2C_START_Pos                5UL
#define SCB_INTR_S_I2C_START_Msk                0x20UL
#define SCB_INTR_S_I2C_ADDR_MATCH_Pos           6UL
#define SCB_INTR_S_I2C_ADDR_MATCH_Msk           0x40UL
#define SCB_INTR_S_I2C_GENERAL_Pos              7UL
#define SCB_INTR_S_I2C_GENERAL_Msk              0x80UL
#define SCB_INTR_S_I2C_BUS_ERROR_Pos            8UL
#define SCB_INTR_S_I2C_BUS_ERROR_Msk            0x100UL
#define SCB_INTR_S_SPI_EZ_WRITE_STOP_Pos        9UL
#define SCB_INTR_S_SPI_EZ_WRITE_STOP_Msk        0x200UL
#define SCB_INTR_S_SPI_EZ_STOP_Pos              10UL
#define SCB_INTR_S_SPI_EZ_STOP_Msk              0x400UL
#define SCB_INTR_S_SPI_BUS_ERROR_Pos            11UL
#define SCB_INTR_S_SPI_BUS_ERROR_Msk            0x800UL
/* SCB.INTR_S_SET */
#define SCB_INTR_S_SET_I2C_ARB_LOST_Pos         0UL
#define SCB_INTR_S_SET_I2C_ARB_LOST_Msk         0x1UL
#define SCB_INTR_S_SET_I2C_NACK_Pos             1UL
#define SCB_INTR_S_SET_I2C_NACK_Msk             0x2UL
#define SCB_INTR_S_SET_I2C_ACK_Pos              2UL
#define SCB_INTR_S_SET_I2C_ACK_Msk              0x4UL
#define SCB_INTR_S_SET_I2C_WRITE_STOP_Pos       3UL
#define SCB_INTR_S_SET_I2C_WRITE_STOP_Msk       0x8UL
#define SCB_INTR_S_SET_I2C_STOP_Pos             4UL
#define SCB_INTR_S_SET_I2C_STOP_Msk             0x10UL
#define SCB_INTR_S_SET_I2C_START_Pos            5UL
#define SCB_INTR_S_SET_I2C_START_Msk            0x20UL
#define SCB_INTR_S_SET_I2C_ADDR_MATCH_Pos       6UL
#define SCB_INTR_S_SET_I2C_ADDR_MATCH_Msk       0x40UL
#define SCB_INTR_S_SET_I2C_GENERAL_Pos          7UL
#define SCB_INTR_S_SET_I2C_GENERAL_Msk          0x80UL
#define SCB_INTR_S_SET_I2C_BUS_ERROR_Pos        8UL
#define SCB_INTR_S_SET_I2C_BUS_ERROR_Msk        0x100UL
#define SCB_INTR_S_SET_SPI_EZ_WRITE_STOP_Pos    9UL
#define SCB_INTR_S_SET_SPI_EZ_WRITE_STOP_Msk    0x200UL
#define SCB_INTR_S_SET_SPI_EZ_STOP_Pos          10UL
#define SCB_INTR_S_SET_SPI_EZ_STOP_Msk          0x400UL
#define SCB_INTR_S_SET_SPI_BUS_ERROR_Pos        11UL
#define SCB_INTR_S_SET_SPI_BUS_ERROR_Msk        0x800UL
/* SCB.INTR_S_MASK */
#define SCB_INTR_S_MASK_I2C_ARB_LOST_Pos        0UL
#define SCB_INTR_S_MASK_I2C_ARB_LOST_Msk        0x1UL
#define SCB_INTR_S_MASK_I2C_NACK_Pos            1UL
#define SCB_INTR_S_MASK_I2C_NACK_Msk            0x2UL
#define SCB_INTR_S_MASK_I2C_ACK_Pos             2UL
#define SCB_INTR_S_MASK_I2C_ACK_Msk             0x4UL
#define SCB_INTR_S_MASK_I2C_WRITE_STOP_Pos      3UL
#define SCB_INTR_S_MASK_I2C_WRITE_STOP_Msk      0x8UL
#define SCB_INTR_S_MASK_I2C_STOP_Pos            4UL
#define SCB_INTR_S_MASK_I2C_STOP_Msk            0x10UL
#define SCB_INTR_S_MASK_I2C_START_Pos           5UL
#define SCB_INTR_S_MASK_I2C_START_Msk           0x20UL
#define SCB_INTR_S_MASK_I2C_ADDR_MATCH_Pos      6UL
#define SCB_INTR_S_MASK_I2C_ADDR_MATCH_Msk      0x40UL
#define SCB_INTR_S_MASK_I2C_GENERAL_Pos         7UL
#define SCB_INTR_S_MASK_I2C_GENERAL_Msk         0x80UL
#define SCB_INTR_S_MASK_I2C_BUS_ERROR_Pos       8UL
#define SCB_INTR_S_MASK_I2C_BUS_ERROR_Msk       0x100UL
#define SCB_INTR_S_MASK_SPI_EZ_WRITE_STOP_Pos   9UL
#define SCB_INTR_S_MASK_SPI_EZ_WRITE_STOP_Msk   0x200UL
#define SCB_INTR_S_MASK_SPI_EZ_STOP_Pos         10UL
#define SCB_INTR_S_MASK_SPI_EZ_STOP_Msk         0x400UL
#define SCB_INTR_S_MASK_SPI_BUS_ERROR_Pos       11UL
#define SCB_INTR_S_MASK_SPI_BUS_ERROR_Msk       0x800UL
/* SCB.INTR_S_MASKED */
#define SCB_INTR_S_MASKED_I2C_ARB_LOST_Pos      0UL
#define SCB_INTR_S_MASKED_I2C_ARB_LOST_Msk      0x1UL
#define SCB_INTR_S_MASKED_I2C_NACK_Pos          1UL
#define SCB_INTR_S_MASKED_I2C_NACK_Msk          0x2UL
#define SCB_INTR_S_MASKED_I2C_ACK_Pos           2UL
#define SCB_INTR_S_MASKED_I2C_ACK_Msk           0x4UL
#define SCB_INTR_S_MASKED_I2C_WRITE_STOP_Pos    3UL
#define SCB_INTR_S_MASKED_I2C_WRITE_STOP_Msk    0x8UL
#define SCB_INTR_S_MASKED_I2C_STOP_Pos          4UL
#define SCB_INTR_S_MASKED_I2C_STOP_Msk          0x10UL
#define SCB_INTR_S_MASKED_I2C_START_Pos         5UL
#define SCB_INTR_S_MASKED_I2C_START_Msk         0x20UL
#define SCB_INTR_S_MASKED_I2C_ADDR_MATCH_Pos    6UL
#define SCB_INTR_S_MASKED_I2C_ADDR_MATCH_Msk    0x40UL
#define SCB_INTR_S_MASKED_I2C_GENERAL_Pos       7UL
#define SCB_INTR_S_MASKED_I2C_GENERAL_Msk       0x80UL
#define SCB_INTR_S_MASKED_I2C_BUS_ERROR_Pos     8UL
#define SCB_INTR_S_MASKED_I2C_BUS_ERROR_Msk     0x100UL
#define SCB_INTR_S_MASKED_SPI_EZ_WRITE_STOP_Pos 9UL
#define SCB_INTR_S_MASKED_SPI_EZ_WRITE_STOP_Msk 0x200UL
#define SCB_INTR_S_MASKED_SPI_EZ_STOP_Pos       10UL
#define SCB_INTR_S_MASKED_SPI_EZ_STOP_Msk       0x400UL
#define SCB_INTR_S_MASKED_SPI_BUS_ERROR_Pos     11UL
#define SCB_INTR_S_MASKED_SPI_BUS_ERROR_Msk     0x800UL
/* SCB.INTR_TX */
#define SCB_INTR_TX_TRIGGER_Pos                 0UL
#define SCB_INTR_TX_TRIGGER_Msk                 0x1UL
#define SCB_INTR_TX_NOT_FULL_Pos                1UL
#define SCB_INTR_TX_NOT_FULL_Msk                0x2UL
#define SCB_INTR_TX_EMPTY_Pos                   4UL
#define SCB_INTR_TX_EMPTY_Msk                   0x10UL
#define SCB_INTR_TX_OVERFLOW_Pos                5UL
#define SCB_INTR_TX_OVERFLOW_Msk                0x20UL
#define SCB_INTR_TX_UNDERFLOW_Pos               6UL
#define SCB_INTR_TX_UNDERFLOW_Msk               0x40UL
#define SCB_INTR_TX_BLOCKED_Pos                 7UL
#define SCB_INTR_TX_BLOCKED_Msk                 0x80UL
#define SCB_INTR_TX_UART_NACK_Pos               8UL
#define SCB_INTR_TX_UART_NACK_Msk               0x100UL
#define SCB_INTR_TX_UART_DONE_Pos               9UL
#define SCB_INTR_TX_UART_DONE_Msk               0x200UL
#define SCB_INTR_TX_UART_ARB_LOST_Pos           10UL
#define SCB_INTR_TX_UART_ARB_LOST_Msk           0x400UL
/* SCB.INTR_TX_SET */
#define SCB_INTR_TX_SET_TRIGGER_Pos             0UL
#define SCB_INTR_TX_SET_TRIGGER_Msk             0x1UL
#define SCB_INTR_TX_SET_NOT_FULL_Pos            1UL
#define SCB_INTR_TX_SET_NOT_FULL_Msk            0x2UL
#define SCB_INTR_TX_SET_EMPTY_Pos               4UL
#define SCB_INTR_TX_SET_EMPTY_Msk               0x10UL
#define SCB_INTR_TX_SET_OVERFLOW_Pos            5UL
#define SCB_INTR_TX_SET_OVERFLOW_Msk            0x20UL
#define SCB_INTR_TX_SET_UNDERFLOW_Pos           6UL
#define SCB_INTR_TX_SET_UNDERFLOW_Msk           0x40UL
#define SCB_INTR_TX_SET_BLOCKED_Pos             7UL
#define SCB_INTR_TX_SET_BLOCKED_Msk             0x80UL
#define SCB_INTR_TX_SET_UART_NACK_Pos           8UL
#define SCB_INTR_TX_SET_UART_NACK_Msk           0x100UL
#define SCB_INTR_TX_SET_UART_DONE_Pos           9UL
#define SCB_INTR_TX_SET_UART_DONE_Msk           0x200UL
#define SCB_INTR_TX_SET_UART_ARB_LOST_Pos       10UL
#define SCB_INTR_TX_SET_UART_ARB_LOST_Msk       0x400UL
/* SCB.INTR_TX_MASK */
#define SCB_INTR_TX_MASK_TRIGGER_Pos            0UL
#define SCB_INTR_TX_MASK_TRIGGER_Msk            0x1UL
#define SCB_INTR_TX_MASK_NOT_FULL_Pos           1UL
#define SCB_INTR_TX_MASK_NOT_FULL_Msk           0x2UL
#define SCB_INTR_TX_MASK_EMPTY_Pos              4UL
#define SCB_INTR_TX_MASK_EMPTY_Msk              0x10UL
#define SCB_INTR_TX_MASK_OVERFLOW_Pos           5UL
#define SCB_INTR_TX_MASK_OVERFLOW_Msk           0x20UL
#define SCB_INTR_TX_MASK_UNDERFLOW_Pos          6UL
#define SCB_INTR_TX_MASK_UNDERFLOW_Msk          0x40UL
#define SCB_INTR_TX_MASK_BLOCKED_Pos            7UL
#define SCB_INTR_TX_MASK_BLOCKED_Msk            0x80UL
#define SCB_INTR_TX_MASK_UART_NACK_Pos          8UL
#define SCB_INTR_TX_MASK_UART_NACK_Msk          0x100UL
#define SCB_INTR_TX_MASK_UART_DONE_Pos          9UL
#define SCB_INTR_TX_MASK_UART_DONE_Msk          0x200UL
#define SCB_INTR_TX_MASK_UART_ARB_LOST_Pos      10UL
#define SCB_INTR_TX_MASK_UART_ARB_LOST_Msk      0x400UL
/* SCB.INTR_TX_MASKED */
#define SCB_INTR_TX_MASKED_TRIGGER_Pos          0UL
#define SCB_INTR_TX_MASKED_TRIGGER_Msk          0x1UL
#define SCB_INTR_TX_MASKED_NOT_FULL_Pos         1UL
#define SCB_INTR_TX_MASKED_NOT_FULL_Msk         0x2UL
#define SCB_INTR_TX_MASKED_EMPTY_Pos            4UL
#define SCB_INTR_TX_MASKED_EMPTY_Msk            0x10UL
#define SCB_INTR_TX_MASKED_OVERFLOW_Pos         5UL
#define SCB_INTR_TX_MASKED_OVERFLOW_Msk         0x20UL
#define SCB_INTR_TX_MASKED_UNDERFLOW_Pos        6UL
#define SCB_INTR_TX_MASKED_UNDERFLOW_Msk        0x40UL
#define SCB_INTR_TX_MASKED_BLOCKED_Pos          7UL
#define SCB_INTR_TX_MASKED_BLOCKED_Msk          0x80UL
#define SCB_INTR_TX_MASKED_UART_NACK_Pos        8UL
#define SCB_INTR_TX_MASKED_UART_NACK_Msk        0x100UL
#define SCB_INTR_TX_MASKED_UART_DONE_Pos        9UL
#define SCB_INTR_TX_MASKED_UART_DONE_Msk        0x200UL
#define SCB_INTR_TX_MASKED_UART_ARB_LOST_Pos    10UL
#define SCB_INTR_TX_MASKED_UART_ARB_LOST_Msk    0x400UL
/* SCB.INTR_RX */
#define SCB_INTR_RX_TRIGGER_Pos                 0UL
#define SCB_INTR_RX_TRIGGER_Msk                 0x1UL
#define SCB_INTR_RX_NOT_EMPTY_Pos               2UL
#define SCB_INTR_RX_NOT_EMPTY_Msk               0x4UL
#define SCB_INTR_RX_FULL_Pos                    3UL
#define SCB_INTR_RX_FULL_Msk                    0x8UL
#define SCB_INTR_RX_OVERFLOW_Pos                5UL
#define SCB_INTR_RX_OVERFLOW_Msk                0x20UL
#define SCB_INTR_RX_UNDERFLOW_Pos               6UL
#define SCB_INTR_RX_UNDERFLOW_Msk               0x40UL
#define SCB_INTR_RX_BLOCKED_Pos                 7UL
#define SCB_INTR_RX_BLOCKED_Msk                 0x80UL
#define SCB_INTR_RX_FRAME_ERROR_Pos             8UL
#define SCB_INTR_RX_FRAME_ERROR_Msk             0x100UL
#define SCB_INTR_RX_PARITY_ERROR_Pos            9UL
#define SCB_INTR_RX_PARITY_ERROR_Msk            0x200UL
#define SCB_INTR_RX_BAUD_DETECT_Pos             10UL
#define SCB_INTR_RX_BAUD_DETECT_Msk             0x400UL
#define SCB_INTR_RX_BREAK_DETECT_Pos            11UL
#define SCB_INTR_RX_BREAK_DETECT_Msk            0x800UL
/* SCB.INTR_RX_SET */
#define SCB_INTR_RX_SET_TRIGGER_Pos             0UL
#define SCB_INTR_RX_SET_TRIGGER_Msk             0x1UL
#define SCB_INTR_RX_SET_NOT_EMPTY_Pos           2UL
#define SCB_INTR_RX_SET_NOT_EMPTY_Msk           0x4UL
#define SCB_INTR_RX_SET_FULL_Pos                3UL
#define SCB_INTR_RX_SET_FULL_Msk                0x8UL
#define SCB_INTR_RX_SET_OVERFLOW_Pos            5UL
#define SCB_INTR_RX_SET_OVERFLOW_Msk            0x20UL
#define SCB_INTR_RX_SET_UNDERFLOW_Pos           6UL
#define SCB_INTR_RX_SET_UNDERFLOW_Msk           0x40UL
#define SCB_INTR_RX_SET_BLOCKED_Pos             7UL
#define SCB_INTR_RX_SET_BLOCKED_Msk             0x80UL
#define SCB_INTR_RX_SET_FRAME_ERROR_Pos         8UL
#define SCB_INTR_RX_SET_FRAME_ERROR_Msk         0x100UL
#define SCB_INTR_RX_SET_PARITY_ERROR_Pos        9UL
#define SCB_INTR_RX_SET_PARITY_ERROR_Msk        0x200UL
#define SCB_INTR_RX_SET_BAUD_DETECT_Pos         10UL
#define SCB_INTR_RX_SET_BAUD_DETECT_Msk         0x400UL
#define SCB_INTR_RX_SET_BREAK_DETECT_Pos        11UL
#define SCB_INTR_RX_SET_BREAK_DETECT_Msk        0x800UL
/* SCB.INTR_RX_MASK */
#define SCB_INTR_RX_MASK_TRIGGER_Pos            0UL
#define SCB_INTR_RX_MASK_TRIGGER_Msk            0x1UL
#define SCB_INTR_RX_MASK_NOT_EMPTY_Pos          2UL
#define SCB_INTR_RX_MASK_NOT_EMPTY_Msk          0x4UL
#define SCB_INTR_RX_MASK_FULL_Pos               3UL
#define SCB_INTR_RX_MASK_FULL_Msk               0x8UL
#define SCB_INTR_RX_MASK_OVERFLOW_Pos           5UL
#define SCB_INTR_RX_MASK_OVERFLOW_Msk           0x20UL
#define SCB_INTR_RX_MASK_UNDERFLOW_Pos          6UL
#define SCB_INTR_RX_MASK_UNDERFLOW_Msk          0x40UL
#define SCB_INTR_RX_MASK_BLOCKED_Pos            7UL
#define SCB_INTR_RX_MASK_BLOCKED_Msk            0x80UL
#define SCB_INTR_RX_MASK_FRAME_ERROR_Pos        8UL
#define SCB_INTR_RX_MASK_FRAME_ERROR_Msk        0x100UL
#define SCB_INTR_RX_MASK_PARITY_ERROR_Pos       9UL
#define SCB_INTR_RX_MASK_PARITY_ERROR_Msk       0x200UL
#define SCB_INTR_RX_MASK_BAUD_DETECT_Pos        10UL
#define SCB_INTR_RX_MASK_BAUD_DETECT_Msk        0x400UL
#define SCB_INTR_RX_MASK_BREAK_DETECT_Pos       11UL
#define SCB_INTR_RX_MASK_BREAK_DETECT_Msk       0x800UL
/* SCB.INTR_RX_MASKED */
#define SCB_INTR_RX_MASKED_TRIGGER_Pos          0UL
#define SCB_INTR_RX_MASKED_TRIGGER_Msk          0x1UL
#define SCB_INTR_RX_MASKED_NOT_EMPTY_Pos        2UL
#define SCB_INTR_RX_MASKED_NOT_EMPTY_Msk        0x4UL
#define SCB_INTR_RX_MASKED_FULL_Pos             3UL
#define SCB_INTR_RX_MASKED_FULL_Msk             0x8UL
#define SCB_INTR_RX_MASKED_OVERFLOW_Pos         5UL
#define SCB_INTR_RX_MASKED_OVERFLOW_Msk         0x20UL
#define SCB_INTR_RX_MASKED_UNDERFLOW_Pos        6UL
#define SCB_INTR_RX_MASKED_UNDERFLOW_Msk        0x40UL
#define SCB_INTR_RX_MASKED_BLOCKED_Pos          7UL
#define SCB_INTR_RX_MASKED_BLOCKED_Msk          0x80UL
#define SCB_INTR_RX_MASKED_FRAME_ERROR_Pos      8UL
#define SCB_INTR_RX_MASKED_FRAME_ERROR_Msk      0x100UL
#define SCB_INTR_RX_MASKED_PARITY_ERROR_Pos     9UL
#define SCB_INTR_RX_MASKED_PARITY_ERROR_Msk     0x200UL
#define SCB_INTR_RX_MASKED_BAUD_DETECT_Pos      10UL
#define SCB_INTR_RX_MASKED_BAUD_DETECT_Msk      0x400UL
#define SCB_INTR_RX_MASKED_BREAK_DETECT_Pos     11UL
#define SCB_INTR_RX_MASKED_BREAK_DETECT_Msk     0x800UL


#endif /* _CYIP_SCB_H_ */


/* [] END OF FILE */
