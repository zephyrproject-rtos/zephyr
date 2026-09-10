/*
 * Copyright (C) 2020 Samsung Electronics Co., Ltd.
 * Copyright (C) 2023 Meta Platforms
 * Copyright (c) 2026 Microchip Technology Inc.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/drivers/i3c.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/util.h>
#include <assert.h>

#include "i3c_dw.h"

#if defined(CONFIG_PINCTRL)
#include <zephyr/drivers/pinctrl.h>
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(microchip_xec_i3c)
#include <zephyr/drivers/clock_control/mchp_xec_clock_control.h>
#include <zephyr/dt-bindings/clock/mchp_xec_pcr.h>
#include <zephyr/dt-bindings/interrupt-controller/mchp-xec-ecia.h>
#include <soc_ecia.h>
#include <soc_pcr.h>
#endif

#define NANO_SEC        1000000000ULL
#define BYTES_PER_DWORD 4

LOG_MODULE_REGISTER(i3c_dw, CONFIG_I3C_DW_LOG_LEVEL);

#define DEVICE_CTRL                0x0
#define DEV_CTRL_ENABLE            BIT(31)
#define DEV_CTRL_RESUME            BIT(30)
#define DEV_CTRL_HOT_JOIN_NACK     BIT(8)
#define DEV_CTRL_I2C_SLAVE_PRESENT BIT(7)
#define DEV_CTRL_IBA_INCLUDE       BIT(0)

#define DEVICE_ADDR                    0x4
#define DEVICE_ADDR_DYNAMIC_ADDR_VALID BIT(31)
#define DEVICE_ADDR_DYNAMIC(x)         (((x) << 16) & GENMASK(22, 16))
#define DEVICE_ADDR_STATIC_ADDR_VALID  BIT(15)
#define DEVICE_ADDR_STATIC_MASK        GENMASK(6, 0)
#define DEVICE_ADDR_STATIC(x)          ((x) & DEVICE_ADDR_STATIC_MASK)

#define HW_CAPABILITY                         0x8
#define HW_CAPABILITY_SLV_IBI_CAP             BIT(19)
#define HW_CAPABILITY_SLV_HJ_CAP              BIT(18)
#define HW_CAPABILITY_HDR_TS_EN               BIT(4)
#define HW_CAPABILITY_HDR_DDR_EN              BIT(3)
#define HW_CAPABILITY_DEVICE_ROLE_CONFIG_MASK GENMASK(2, 0)

/* HW_CAPABILITY[2:0] DEVICE_ROLE_CONFIG (IC_DEVICE_ROLE): fixed at synthesis. */
#define HW_CAP_DEVICE_ROLE_MASTER     0x1 /* controller only */
#define HW_CAP_DEVICE_ROLE_SEC_MASTER 0x3 /* dual-role       */
#define HW_CAP_DEVICE_ROLE_SLAVE      0x4 /* target only     */

/* Dual-role rejects SIR/MR via the reject registers; controller-only uses the DAT
 * entry (databook Table 2-1).
 */
#define DW_IBI_REJECT_VIA_REG(role) ((role) == HW_CAP_DEVICE_ROLE_SEC_MASTER)
/* BUS_AVAILABLE_TIME/BUS_IDLE_TIMING exist for 1 < IC_DEVICE_ROLE < 5. */
#define DW_HAS_BUS_IDLE(role)                                                                      \
	((role) == HW_CAP_DEVICE_ROLE_SEC_MASTER || (role) == HW_CAP_DEVICE_ROLE_SLAVE)

#define COMMAND_QUEUE_PORT         0xc
#define COMMAND_PORT_TOC           BIT(30)
#define COMMAND_PORT_READ_TRANSFER BIT(28)
#define COMMAND_PORT_SDAP          BIT(27)
#define COMMAND_PORT_ROC           BIT(26)
#define COMMAND_PORT_DBP           BIT(25)
#define COMMAND_PORT_SPEED(x)      (((x) << 21) & GENMASK(23, 21))
#define COMMAND_PORT_SPEED_I2C_FM  0
#define COMMAND_PORT_SPEED_I2C_FMP 1
#define COMMAND_PORT_SPEED_I3C_DDR 6
#define COMMAND_PORT_SPEED_I3C_TS  7
#define COMMAND_PORT_DEV_INDEX(x)  (((x) << 16) & GENMASK(20, 16))
#define COMMAND_PORT_CP            BIT(15)
#define COMMAND_PORT_CMD(x)        (((x) << 7) & GENMASK(14, 7))
#define COMMAND_PORT_TID(x)        (((x) << 3) & GENMASK(6, 3))

#define COMMAND_PORT_ARG_DATA_LEN(x)  (((x) << 16) & GENMASK(31, 16))
#define COMMAND_PORT_ARG_DB(x)        (((x) << 8) & GENMASK(15, 8))
#define COMMAND_PORT_ARG_DATA_LEN_MAX 65536
#define COMMAND_PORT_TRANSFER_ARG     0x01

#define COMMAND_PORT_SDA_DATA_BYTE_3(x) (((x) << 24) & GENMASK(31, 24))
#define COMMAND_PORT_SDA_DATA_BYTE_2(x) (((x) << 16) & GENMASK(23, 16))
#define COMMAND_PORT_SDA_DATA_BYTE_1(x) (((x) << 8) & GENMASK(15, 8))
#define COMMAND_PORT_SDA_BYTE_STRB_3    BIT(5)
#define COMMAND_PORT_SDA_BYTE_STRB_2    BIT(4)
#define COMMAND_PORT_SDA_BYTE_STRB_1    BIT(3)
#define COMMAND_PORT_SHORT_DATA_ARG     0x02

#define COMMAND_PORT_DEV_COUNT(x)   (((x) << 21) & GENMASK(25, 21))
#define COMMAND_PORT_ADDR_ASSGN_CMD 0x03

#define RESPONSE_QUEUE_PORT            0x10
#define RESPONSE_PORT_ERR_STATUS(x)    (((x) & GENMASK(31, 28)) >> 28)
#define RESPONSE_NO_ERROR              0
#define RESPONSE_ERROR_CRC             1
#define RESPONSE_ERROR_PARITY          2
#define RESPONSE_ERROR_FRAME           3
#define RESPONSE_ERROR_IBA_NACK        4
#define RESPONSE_ERROR_ADDRESS_NACK    5
#define RESPONSE_ERROR_OVER_UNDER_FLOW 6
#define RESPONSE_ERROR_TRANSF_ABORT    8
#define RESPONSE_ERROR_I2C_W_NACK_ERR  9
#define RESPONSE_PORT_TID(x)           (((x) & GENMASK(27, 24)) >> 24)
#define RESPONSE_PORT_DATA_LEN(x)      ((x) & GENMASK(15, 0))

#define RX_TX_DATA_PORT              0x14
#define IBI_QUEUE_STATUS             0x18
#define IBI_QUEUE_STATUS_IBI_STS(x)  (((x) & GENMASK(31, 28)) >> 28)
#define IBI_QUEUE_STATUS_IBI_ID(x)   (((x) & GENMASK(15, 8)) >> 8)
#define IBI_QUEUE_STATUS_DATA_LEN(x) ((x) & GENMASK(7, 0))
#define IBI_QUEUE_IBI_ADDR(x)        (IBI_QUEUE_STATUS_IBI_ID(x) >> 1)
#define IBI_QUEUE_IBI_RNW(x)         (IBI_QUEUE_STATUS_IBI_ID(x) & BIT(0))
#define IBI_TYPE_MR(x) \
	((IBI_QUEUE_IBI_ADDR(x) != I3C_HOT_JOIN_ADDR) && !IBI_QUEUE_IBI_RNW(x))
#define IBI_TYPE_HJ(x) \
	((IBI_QUEUE_IBI_ADDR(x) == I3C_HOT_JOIN_ADDR) && !IBI_QUEUE_IBI_RNW(x))
#define IBI_TYPE_SIRQ(x) \
	((IBI_QUEUE_IBI_ADDR(x) != I3C_HOT_JOIN_ADDR) && IBI_QUEUE_IBI_RNW(x))

#define QUEUE_THLD_CTRL               0x1c
#define QUEUE_THLD_CTRL_IBI_STS_MASK  GENMASK(31, 24)
#define QUEUE_THLD_CTRL_RESP_BUF_MASK GENMASK(15, 8)
#define QUEUE_THLD_CTRL_RESP_BUF(x)   (((x) - 1) << 8)

#define QUEUE_THLD_CTRL_IBI_DATA_MASK    GENMASK(23, 16)
#define QUEUE_THLD_CTRL_IBI_DATA(x)      (((x) << 16) & QUEUE_THLD_CTRL_IBI_DATA_MASK)
#define QUEUE_THLD_CTRL_IBI_DATA_DEFAULT 0x01U

#define DATA_BUFFER_THLD_CTRL                    0x20
#define DATA_BUFFER_THLD_CTRL_RX_BUF             GENMASK(11, 8)
#define DATA_BUFFER_THLD_CTRL_TX_START_THLD_MASK GENMASK(18, 16)
#define DATA_BUFFER_THLD_CTRL_TX_START_THLD(x)                                                     \
	(((x) << 16) & DATA_BUFFER_THLD_CTRL_TX_START_THLD_MASK)
#define DATA_BUFFER_THLD_CTRL_TX_BUF_MASK GENMASK(2, 0)
#define DATA_BUFFER_THLD_CTRL_TX_BUF(x)   ((x) & DATA_BUFFER_THLD_CTRL_TX_BUF_MASK)

#define IBI_QUEUE_CTRL     0x24
#define IBI_MR_REQ_REJECT  0x2C
#define IBI_SIR_REQ_REJECT 0x30
#define IBI_SIR_REQ_ID(x)  ((((x) & GENMASK(6, 5)) >> 5) + ((x) & GENMASK(4, 0)))
#define IBI_REQ_REJECT_ALL GENMASK(31, 0)

#define RESET_CTRL            0x34
#define RESET_CTRL_IBI_QUEUE  BIT(5)
#define RESET_CTRL_RX_FIFO    BIT(4)
#define RESET_CTRL_TX_FIFO    BIT(3)
#define RESET_CTRL_RESP_QUEUE BIT(2)
#define RESET_CTRL_CMD_QUEUE  BIT(1)
#define RESET_CTRL_SOFT       BIT(0)
#define RESET_CTRL_ALL                                                                             \
	(RESET_CTRL_IBI_QUEUE | RESET_CTRL_RX_FIFO | RESET_CTRL_TX_FIFO | RESET_CTRL_RESP_QUEUE |  \
	 RESET_CTRL_CMD_QUEUE | RESET_CTRL_SOFT)

#define SLV_EVENT_STATUS        0x38
#define SLV_EVENT_STATUS_HJ_EN  BIT(3)
#define SLV_EVENT_STATUS_MR_EN  BIT(1)
#define SLV_EVENT_STATUS_SIR_EN BIT(0)

#define INTR_STATUS               0x3c
#define INTR_STATUS_EN            0x40
#define INTR_SIGNAL_EN            0x44
#define INTR_FORCE                0x48
#define INTR_BUSOWNER_UPDATE_STAT BIT(13)
#define INTR_IBI_UPDATED_STAT     BIT(12)
#define INTR_READ_REQ_RECV_STAT   BIT(11)
#define INTR_DEFSLV_STAT          BIT(10)
#define INTR_TRANSFER_ERR_STAT    BIT(9)
#define INTR_DYN_ADDR_ASSGN_STAT  BIT(8)
#define INTR_CCC_UPDATED_STAT     BIT(6)
#define INTR_TRANSFER_ABORT_STAT  BIT(5)
#define INTR_RESP_READY_STAT      BIT(4)
#define INTR_CMD_QUEUE_READY_STAT BIT(3)
#define INTR_IBI_THLD_STAT        BIT(2)
#define INTR_RX_THLD_STAT         BIT(1)
#define INTR_TX_THLD_STAT         BIT(0)
#define INTR_ALL                                                                                   \
	(INTR_BUSOWNER_UPDATE_STAT | INTR_IBI_UPDATED_STAT | INTR_READ_REQ_RECV_STAT |             \
	 INTR_DEFSLV_STAT | INTR_TRANSFER_ERR_STAT | INTR_DYN_ADDR_ASSGN_STAT |                    \
	 INTR_CCC_UPDATED_STAT | INTR_TRANSFER_ABORT_STAT | INTR_RESP_READY_STAT |                 \
	 INTR_CMD_QUEUE_READY_STAT | INTR_IBI_THLD_STAT | INTR_TX_THLD_STAT | INTR_RX_THLD_STAT)

#ifdef CONFIG_I3C_USE_IBI
#define INTR_MASTER_MASK (INTR_TRANSFER_ERR_STAT | INTR_RESP_READY_STAT | INTR_IBI_THLD_STAT)
#define INTR_SLAVE_MASK                                                                            \
	(INTR_TRANSFER_ERR_STAT | INTR_IBI_UPDATED_STAT | INTR_READ_REQ_RECV_STAT |                \
	 INTR_DYN_ADDR_ASSGN_STAT | INTR_RESP_READY_STAT | INTR_CCC_UPDATED_STAT)
#else
#define INTR_MASTER_MASK (INTR_TRANSFER_ERR_STAT | INTR_RESP_READY_STAT)
#define INTR_SLAVE_MASK                                                                            \
	(INTR_TRANSFER_ERR_STAT | INTR_READ_REQ_RECV_STAT | INTR_DYN_ADDR_ASSGN_STAT |             \
	 INTR_RESP_READY_STAT | INTR_CCC_UPDATED_STAT)
#endif

#define QUEUE_STATUS_LEVEL             0x4c
#define QUEUE_STATUS_IBI_STATUS_CNT(x) (((x) & GENMASK(28, 24)) >> 24)
#define QUEUE_STATUS_IBI_BUF_BLR(x)    (((x) & GENMASK(23, 16)) >> 16)
#define QUEUE_STATUS_LEVEL_RESP(x)     (((x) & GENMASK(15, 8)) >> 8)
#define QUEUE_STATUS_LEVEL_CMD(x)      ((x) & GENMASK(7, 0))

#define DATA_BUFFER_STATUS_LEVEL       0x50
#define DATA_BUFFER_STATUS_LEVEL_RX(x) (((x) & GENMASK(23, 16)) >> 16)
#define DATA_BUFFER_STATUS_LEVEL_TX(x) ((x) & GENMASK(7, 0))

#define PRESENT_STATE                   0x54
#define PRESENT_STATE_CURRENT_MASTER    BIT(2)
#define PRESENT_STATE_CONTROLLER_IDLE   BIT(28)
#define PRESENT_STATE_CM_TFR_STS_MASK   GENMASK(13, 8)
#define PRESENT_STATE_CM_TFR_STS(x)     (((x) & PRESENT_STATE_CM_TFR_STS_MASK) >> 8)
/* Sub-state within the transfer CM_TFR_STS reports; logged for diagnostics. */
#define PRESENT_STATE_CM_TFR_ST_STS(x)  (((x) & GENMASK(21, 16)) >> 16)
#define CM_TFR_STS_IDLE                 0x0
/* Specifically controller HALT value.  Target has different bit values for CM_TFR_STATUS */
#define CM_TFR_STS_CTRL_HALT            0xF

#define CCC_DEVICE_STATUS          0x58
#define DEVICE_ADDR_TABLE_POINTER  0x5c
#define DEVICE_ADDR_TABLE_DEPTH(x) (((x) & GENMASK(31, 16)) >> 16)
#define DEVICE_ADDR_TABLE_ADDR(x)  ((x) & GENMASK(15, 0))

#define DEV_CHAR_TABLE_POINTER      0x60
#define DEVICE_CHAR_TABLE_ADDR(x)   ((x) & GENMASK(11, 0))
#define VENDOR_SPECIFIC_REG_POINTER 0x6c

#define SLV_MIPI_ID_VALUE                      0x70
#define SLV_MIPI_ID_VALUE_SLV_MIPI_MFG_ID_MASK GENMASK(15, 1)
#define SLV_MIPI_ID_VALUE_SLV_MIPI_MFG_ID(x)   ((x) & SLV_MIPI_ID_VALUE_SLV_MIPI_MFG_ID_MASK)
#define SLV_MIPI_ID_VALUE_SLV_PROV_ID_SEL      BIT(0)

#define SLV_PID_VALUE 0x74

#define SLV_CHAR_CTRL                      0x78
#define SLV_CHAR_CTRL_MAX_DATA_SPEED_LIMIT BIT(0)
#define SLV_CHAR_CTRL_IBI_REQUEST_CAPABLE  BIT(1)
#define SLV_CHAR_CTRL_IBI_PAYLOAD          BIT(2)
#define SLV_CHAR_CTRL_BCR_MASK             GENMASK(7, 0)
#define SLV_CHAR_CTRL_BCR(x)               ((x) & SLV_CHAR_CTRL_BCR_MASK)
#define SLV_CHAR_CTRL_DCR_MASK             GENMASK(15, 8)
#define SLV_CHAR_CTRL_DCR(x)               (((x) & SLV_CHAR_CTRL_DCR_MASK) >> 8)
#define SLV_CHAR_CTRL_HDR_CAP_MASK         GENMASK(23, 16)
#define SLV_CHAR_CTRL_HDR_CAP(x)           (((x) & SLV_CHAR_CTRL_HDR_CAP_MASK) >> 16)

#define SLV_MAX_LEN        0x7c
#define SLV_MAX_LEN_MRL(x) (((x) & GENMASK(31, 16)) >> 16)
#define SLV_MAX_LEN_MWL(x) ((x) & GENMASK(15, 0))

#define TGT_EVENT_STATUS             0x38
#define TGT_EVENT_STATUS_MWL_UPDATED BIT(7)
#define TGT_EVENT_STATUS_MRL_UPDATED BIT(6)

#define MAX_READ_TURNAROUND                     0x80
#define MAX_READ_TURNAROUND_MXDX_MAX_RD_TURN(x) ((x) & GENMASK(23, 0))

#define MAX_DATA_SPEED   0x84
#define SLV_DEBUG_STATUS 0x88

#define SLV_INTR_REQ                        0x8c
#define SLV_INTR_REQ_SIR_DATA_LENGTH(x)     (((x) << 16) & GENMASK(23, 16))
#define SLV_INTR_REQ_MDB(x)                 (((x) << 8) & GENMASK(15, 8))
#define SLV_INTR_REQ_IBI_STS(x)             (((x) & GENMASK(9, 8)) >> 8)
#define SLV_INTR_REQ_IBI_STS_IBI_ACCEPT     0x01
#define SLV_INTR_REQ_IBI_STS_IBI_NO_ATTEMPT 0x03
#define SLV_INTR_REQ_TS                     BIT(4)
#define SLV_INTR_REQ_MR                     BIT(3)
#define SLV_INTR_REQ_SIR_CTRL(x)            (((x) & GENMASK(2, 1)) >> 1)
#define SLV_INTR_REQ_SIR                    BIT(0)

#define SLV_SIR_DATA          0x94
#define SLV_SIR_DATA_BYTE3(x) (((x) << 24) & GENMASK(31, 24))
#define SLV_SIR_DATA_BYTE2(x) (((x) << 16) & GENMASK(23, 16))
#define SLV_SIR_DATA_BYTE1(x) (((x) << 8) & GENMASK(15, 8))
#define SLV_SIR_DATA_BYTE0(x) ((x) & GENMASK(7, 0))

#define SLV_IBI_RESP                         0x98
#define SLV_IBI_RESP_DATA_LENGTH(x)          (((x) & GENMASK(23, 8)) >> 8)
#define SLV_IBI_RESP_IBI_STS(x)              ((x) & GENMASK(1, 0))
#define SLV_IBI_RESP_IBI_STS_ACK             0x01
#define SLV_IBI_RESP_IBI_STS_EARLY_TERMINATE 0x02
#define SLV_IBI_RESP_IBI_STS_NACK            0x03

#define SLV_NACK_REQ               0x9c
#define SLV_NACK_REQ_NACK_REQ(x)   ((x) & GENMASK(1, 0))
#define SLV_NACK_REQ_NACK_REQ_ACK  0x00
#define SLV_NACK_REQ_NACK_REQ_NACK 0x01

#define DEVICE_CTRL_EXTENDED                           0xb0
#define DEVICE_CTRL_EXTENDED_DEV_OPERATION_MODE(x)     ((x) & GENMASK(1, 0))
#define DEVICE_CTRL_EXTENDED_DEV_OPERATION_MODE_MASTER 0
#define DEVICE_CTRL_EXTENDED_DEV_OPERATION_MODE_SLAVE  1

#define SCL_I3C_OD_TIMING      0xb4
#define SCL_I3C_PP_TIMING      0xb8
#define SCL_I3C_TIMING_HCNT(x) (((x) << 16) & GENMASK(23, 16))
#define SCL_I3C_TIMING_LCNT(x) ((x) & GENMASK(7, 0))
#define SCL_I3C_TIMING_CNT_MIN 5
#define SCL_I3C_TIMING_CNT_MAX 255

#define SCL_I2C_FM_TIMING         0xbc
#define SCL_I2C_FM_TIMING_HCNT(x) (((x) << 16) & GENMASK(31, 16))
#define SCL_I2C_FM_TIMING_LCNT(x) ((x) & GENMASK(15, 0))
#define SCL_I2C_FM_TIMING_CNT_MAX 0xffff

#define SCL_I2C_FMP_TIMING         0xc0
#define SCL_I2C_FMP_TIMING_HCNT(x) (((x) << 16) & GENMASK(23, 16))
#define SCL_I2C_FMP_TIMING_LCNT(x) ((x) & GENMASK(15, 0))

#define SCL_EXT_LCNT_TIMING 0xc8
#define SCL_EXT_LCNT_4(x)   (((x) << 24) & GENMASK(31, 24))
#define SCL_EXT_LCNT_3(x)   (((x) << 16) & GENMASK(23, 16))
#define SCL_EXT_LCNT_2(x)   (((x) << 8) & GENMASK(15, 8))
#define SCL_EXT_LCNT_1(x)   ((x) & GENMASK(7, 0))

#define SCL_EXT_TERMN_LCNT_TIMING 0xcc

#define SDA_HOLD_SWITCH_DLY_TIMING                         0xd0
#define SDA_HOLD_SWITCH_DLY_TIMING_SDA_TX_HOLD(x)          (((x)&GENMASK(18, 16)) >> 16)
#define SDA_HOLD_SWITCH_DLY_TIMING_SDA_PP_OD_SWITCH_DLY(x) (((x)&GENMASK(10, 8)) >> 8)
#define SDA_HOLD_SWITCH_DLY_TIMING_SDA_OD_PP_SWITCH_DLY(x) ((x)&GENMASK(2, 0))

#define BUS_FREE_TIMING           0xd4
/* Bus available time of 1us in ns */
#define I3C_BUS_AVAILABLE_TIME_NS 1000U
#define BUS_I3C_MST_FREE(x)       ((x) & GENMASK(15, 0))
#define BUS_I3C_AVAIL_TIME(x)     ((x << 16) & GENMASK(31, 16))

#define BUS_IDLE_TIMING      0xd8
/* Bus Idle time of 1ms in ns */
#define I3C_BUS_IDLE_TIME_NS 1000000U
#define BUS_I3C_IDLE_TIME(x) ((x) & GENMASK(19, 0))

#define I3C_VER_ID         0xe0
#define I3C_VER_TYPE       0xe4
#define RELEASE_SDA_TIMING 0xec

#define QUEUE_SIZE_CAPABILITY                        0xe8
#define QUEUE_SIZE_CAPABILITY_IBI_BUF_DWORD_SIZE(x)  (2 << (((x) & GENMASK(19, 16)) >> 16))
#define QUEUE_SIZE_CAPABILITY_RESP_BUF_DWORD_SIZE(x) (2 << (((x) & GENMASK(15, 12)) >> 12))
#define QUEUE_SIZE_CAPABILITY_CMD_BUF_DWORD_SIZE(x)  (2 << (((x) & GENMASK(11, 8)) >> 8))
#define QUEUE_SIZE_CAPABILITY_RX_BUF_DWORD_SIZE(x)   (2 << (((x) & GENMASK(7, 4)) >> 4))
#define QUEUE_SIZE_CAPABILITY_TX_BUF_DWORD_SIZE(x)   (2 << ((x) & GENMASK(3, 0)))

#define DEV_ADDR_TABLE_LEGACY_I2C_DEV    BIT(31)
#define DEV_ADDR_TABLE_DYNAMIC_ADDR_MASK GENMASK(23, 16)
#define DEV_ADDR_TABLE_DYNAMIC_ADDR(x)   (((x) << 16) & GENMASK(23, 16))
#define DEV_ADDR_TABLE_MR_REJECT         BIT(14)
#define DEV_ADDR_TABLE_SIR_REJECT        BIT(13)
#define DEV_ADDR_TABLE_IBI_WITH_DATA     BIT(12)
#define DEV_ADDR_TABLE_STATIC_ADDR(x)    ((x) & GENMASK(6, 0))
#define DEV_ADDR_TABLE_LOC(start, idx)   ((start) + ((idx) << 2))

#define DEV_CHAR_TABLE_LOC1(start, idx) ((start) + ((idx) << 4))
#define DEV_CHAR_TABLE_MSB_PID(x)       ((x) & GENMASK(31, 16))
#define DEV_CHAR_TABLE_LSB_PID(x)       ((x) & GENMASK(15, 0))
#define DEV_CHAR_TABLE_LOC2(start, idx) ((DEV_CHAR_TABLE_LOC1(start, idx)) + 4)
#define DEV_CHAR_TABLE_LOC3(start, idx) ((DEV_CHAR_TABLE_LOC1(start, idx)) + 8)
#define DEV_CHAR_TABLE_LOC4(start, idx) ((DEV_CHAR_TABLE_LOC1(start, idx)) + 12)
#define DEV_CHAR_TABLE_DYNAMIC_ADDR(x)  ((x) & GENMASK(7, 0))
#define DEV_CHAR_TABLE_DCR(x)           ((x) & GENMASK(7, 0))
#define DEV_CHAR_TABLE_BCR(x)           (((x) & GENMASK(15, 8)) >> 8)

#define SDCT_DYNAMIC_ADDR(x) ((x) & GENMASK(7, 0))
#define SDCT_DCR(x)          (((x) & GENMASK(15, 8)) >> 8)
#define SDCT_BCR(x)          (((x) & GENMASK(23, 16)) >> 16)
#define SDCT_STATIC_ADDR(x)  (((x) & GENMASK(31, 24)) >> 24)

#define I3C_BUS_SDR1_SCL_RATE       8000000
#define I3C_BUS_SDR2_SCL_RATE       6000000
#define I3C_BUS_SDR3_SCL_RATE       4000000
#define I3C_BUS_SDR4_SCL_RATE       2000000
#define I3C_BUS_I2C_SM_TLOW_MIN_NS  4700
#define I3C_BUS_I2C_FM_TLOW_MIN_NS  1300
#define I3C_BUS_I2C_FMP_TLOW_MIN_NS 500
#define I3C_BUS_THIGH_MAX_NS        41
#define I3C_BUS_TCAS_PS             38400
#define I3C_PERIOD_NS               1000000000ULL
#define I3C_PERIOD_PS               I3C_PERIOD_NS * 1000ULL

#define I3C_BUS_MAX_I3C_SCL_RATE     12900000
#define I3C_BUS_TYP_I3C_SCL_RATE     12500000
#define I3C_BUS_I2C_FM_PLUS_SCL_RATE 1000000
#define I3C_BUS_I2C_FM_SCL_RATE      400000
#define I3C_BUS_I2C_SM_SCL_RATE      100000
#define I3C_BUS_TLOW_OD_MIN_NS       200

#define I3C_HOT_JOIN_ADDR 0x02

/* Microchip XEC wrapper register: HOST_CFG offset from the I3C base.
 * Outside the Synopsys IP register map.
 */
#define MCHP_HOST_CFG_OFS 0x300U

#define DW_I3C_MAX_DEVS         32
#define DW_I3C_MAX_CMD_BUF_SIZE 16

/* Recovery timings and drain bounds. */
#define DW_I3C_RESUME_TIMEOUT_US     2000U
#define DW_I3C_CTRL_IDLE_TIMEOUT_US  2000U
#define DW_I3C_FLUSH_TIMEOUT_US      2000U
/* RESET_CTRL readback faults while the flush is held off, so wait it out blind. */
#define DW_I3C_FLUSH_SETTLE_US       50U
#define DW_I3C_SOFT_RESET_SETTLE_US  200U
#define DW_I3C_DRAIN_PASSES          4
#define DW_I3C_DRAIN_SETTLE_US       5U
#define DW_I3C_DRAIN_MAX_RESP        128U
#define DW_I3C_DRAIN_MAX_RX          256U
#define DW_I3C_DRAIN_MAX_IBI         64U
#define DW_I3C_DRAIN_MAX_IBI_DATA    256U
/* Longest a halt is allowed to clear on its own before recovery runs. */
#define DW_I3C_HALT_SETTLE_US        1000U

/* Snps I3C/I2C Device Private Data */
struct dw_i3c_i2c_dev_data {
	/* Device id within the retaining registers. This is set after bus initialization by the
	 * controller.
	 */
	uint8_t id;
};

struct dw_i3c_cmd {
	uint32_t cmd_lo;
	uint32_t cmd_hi;
	void *buf;
	uint16_t tx_len;
	uint16_t rx_len;
	uint8_t error;
};

struct dw_i3c_xfer {
	int32_t ret;
	uint32_t ncmds;
	struct dw_i3c_cmd cmds[DW_I3C_MAX_CMD_BUF_SIZE];
};

struct dw_i3c_config {
	struct i3c_driver_config common;
	const struct device *clock;
	bool target_mode;

	/* Clock control subsys related struct */
	clock_control_subsys_t clock_subsys;
	uint32_t regs;

	void (*irq_config_func)();

#if defined(CONFIG_PINCTRL)
	const struct pinctrl_dev_config *pcfg;
#endif

	/* Optional vendor platform hooks; NULL selects the no-op fallbacks. */
	const struct dw_i3c_platform_ops *ops;

#if DT_HAS_COMPAT_STATUS_OKAY(microchip_xec_i3c)
	/* Microchip XEC-specific fields */
	bool is_mchp;
	uint8_t port_sel; /* HOST_CFG[2:0] port selection */
	uint8_t enc_pcr;  /* encoded PCR sleep-enable idx/bitpos, from the clocks cells */
	uint8_t girq_id;  /* ECIA GIRQ number, from girqs[0] */
	uint8_t girq_pos; /* bit position within that GIRQ, from girqs[0] */
#endif
};

/**
 * @brief Return the MMIO base address of this DW I3C instance.
 *
 * Exported so vendor glue translation units can reach the registers without
 * a copy of struct dw_i3c_config.
 */
uint32_t dw_i3c_get_regs(const struct device *dev)
{
	const struct dw_i3c_config *config = dev->config;

	return config->regs;
}

/**
 * @brief Return whether devicetree requests target mode for this instance
 */
bool dw_i3c_is_secondary_requested(const struct device *dev)
{
	const struct dw_i3c_config *config = dev->config;

	return config->target_mode;
}

struct dw_i3c_data {
	struct i3c_driver_data common;
	uint32_t free_pos;

	uint16_t datstartaddr;
	uint16_t dctstartaddr;
	uint16_t maxdevs;

	/* IC_DEVICE_ROLE, cached from HW_CAPABILITY */
	uint8_t role;

	/* fifo depth is in words (32b) */
	uint8_t ibififodepth;
	uint8_t respfifodepth;
	uint8_t cmdfifodepth;
	uint8_t rxfifodepth;
	uint8_t txfifodepth;

#ifdef CONFIG_I3C_TARGET
	struct i3c_target_config *target_config;
	bool target_da_valid_last;
#endif /* CONFIG_I3C_TARGET */
	struct k_sem sem_xfer;
	struct k_mutex mt;

#ifdef CONFIG_I3C_USE_IBI
	struct k_sem ibi_sts_sem;
	struct k_sem sem_hj;
#endif

	struct dw_i3c_xfer xfer;
#ifdef CONFIG_I3C_CONTROLLER
	enum i3c_bus_mode mode;
	struct dw_i3c_i2c_dev_data dw_i3c_i2c_priv_data[DW_I3C_MAX_DEVS];
#endif /* CONFIG_I3C_CONTROLLER */
#if defined(CONFIG_I3C_CONTROLLER) && defined(CONFIG_I3C_TARGET)
	const struct device *dev;
	struct k_work deftgts_work;
	uint8_t deftgts_count;
#endif /* CONFIG_I3C_CONTROLLER && CONFIG_I3C_TARGET */
};

/**
 * @brief Post-reset hook: re-open vendor wrapper gate after RESET_CTRL_ALL.
 *
 * Called unconditionally from dw_i3c_full_reset().  When no vendor ops are
 * registered the DW core registers are directly accessible so a no-op is
 * sufficient.
 */
static int dw_i3c_post_reset(const struct device *dev)
{
	const struct dw_i3c_config *config = dev->config;

	if (DW_I3C_OPS(config) && config->ops->post_reset) {
		return config->ops->post_reset(dev);
	}
	return 0;
}

/**
 * @brief Pre-resume-ctrl hook: re-open wrapper gate before writing RESUME.
 *
 * Called unconditionally from dw_i3c_recover_bus().  No-op when no ops.
 */
static __maybe_unused int dw_i3c_pre_resume_ctrl(const struct device *dev)
{
	const struct dw_i3c_config *config = dev->config;

	if (DW_I3C_OPS(config) && config->ops->pre_resume_ctrl) {
		return config->ops->pre_resume_ctrl(dev);
	}

	return 0;
}

/**
 * @brief Pre-init hook: open any vendor wrapper gates before MMIO access.
 *
 * Called from dw_i3c_init() after clock control and before the first DW core
 * register access. No-op when no vendor hooks are registered.
 */
static int dw_i3c_pre_init(const struct device *dev)
{
	const struct dw_i3c_config *config = dev->config;

	if (DW_I3C_OPS(config) && config->ops->pre_init) {
		return config->ops->pre_init(dev);
	}
	return 0;
}

/**
 * @brief Clock-on hook for vendor glue.
 *
 * Some SoCs auto-enable the divider or need additional wrapper sequencing.
 * This dispatches to vendor code when present and otherwise falls back to a
 * plain success return because clock_control_on() has already been called.
 */
static int dw_i3c_clock_on(const struct device *dev)
{
	const struct dw_i3c_config *config = dev->config;

	if (DW_I3C_OPS(config) && config->ops->clock_on) {
		return config->ops->clock_on(dev);
	}
	return 0;
}

/**
 * @brief Ask the platform hook how to retry after a transfer or DAA timeout.
 */
static __maybe_unused enum dw_i3c_retry_action
dw_i3c_timeout_retry_action(const struct device *dev, enum dw_i3c_timeout_op op,
			    bool retried_after_recover, bool first_cmd_addr_nack)
{
	const struct dw_i3c_config *config = dev->config;
	enum dw_i3c_retry_action action = DW_I3C_RETRY_NONE;

	if (DW_I3C_OPS(config) && config->ops->should_retry_timeout) {
		if (config->ops->should_retry_timeout(dev, op, retried_after_recover,
						      first_cmd_addr_nack,
						      &action) == 0) {
			return action;
		}
	}

	return DW_I3C_RETRY_NONE;
}

/**
 * @brief Ask the platform hook whether the ISR may recover in place.
 */
static enum dw_i3c_isr_error_action
dw_i3c_isr_error_recovery_action(const struct device *dev, int xfer_error)
{
	const struct dw_i3c_config *config = dev->config;

	if (DW_I3C_OPS(config) && config->ops->isr_error_action) {
		return config->ops->isr_error_action(dev, xfer_error);
	}

	return DW_I3C_ISR_ERROR_RECOVER_NOW;
}

/**
 * @brief Ask the platform hook how to retry after a CCC timeout.
 */
static inline __maybe_unused enum dw_i3c_retry_action
dw_i3c_ccc_timeout_retry_action(const struct device *dev,
					bool retried_after_recover,
					bool first_cmd_error_none,
					bool is_setdasa_direct,
					bool is_enec_broadcast)
{
	const struct dw_i3c_config *config = dev->config;
	enum dw_i3c_retry_action action = DW_I3C_RETRY_NONE;

	if (DW_I3C_OPS(config) && config->ops->should_retry_ccc_timeout) {
		if (config->ops->should_retry_ccc_timeout(dev, retried_after_recover,
							  first_cmd_error_none,
							  is_setdasa_direct,
							  is_enec_broadcast,
							  &action) == 0) {
			return action;
		}
	}

	return DW_I3C_RETRY_NONE;
}

/**
 * @brief Get the DW core clock rate, using vendor glue when needed.
 */
static int dw_i3c_get_core_rate(const struct device *dev, uint32_t *core_rate)
{
	const struct dw_i3c_config *config = dev->config;

	if (DW_I3C_OPS(config) && config->ops->get_clock_rate) {
		return config->ops->get_clock_rate(dev, core_rate);
	}

	return clock_control_get_rate(config->clock, config->clock_subsys, core_rate);
}

static inline bool dw_i3c_is_current_controller(const struct device *dev)
{
	const struct dw_i3c_config *config = dev->config;

	return !!(sys_read32(config->regs + PRESENT_STATE) & PRESENT_STATE_CURRENT_MASTER);
}

#ifdef CONFIG_I3C_CONTROLLER

static int dw_i3c_recover_bus(const struct device *dev);
static int dw_i3c_full_reset(const struct device *dev);
static int dw_i3c_recover_bus_locked_light(const struct device *dev);
static void dw_i3c_force_drain_paths(const struct device *dev);

/*
 * Returns the index of the first free slot, or -1 when the table is full.
 * The return type must stay signed: truncating to uint8_t turns the
 * exhaustion result into 255 and defeats every caller's bounds check.
 */
static int get_free_pos(uint32_t free_pos)
{
	return find_lsb_set(free_pos) - 1;
}

/**
 * @brief Read data from the Receive FIFO of the I3C device.
 *
 * This function reads data from the Receive FIFO of the I3C device specified by
 * the given device structure and stores it in the provided buffer.
 *
 * @param dev Pointer to the I3C device structure.
 * @param buf Pointer to the buffer where the received data will be stored.
 * @param nbytes Number of bytes to read from the Receive FIFO.
 */
static void read_rx_fifo(const struct device *dev, uint8_t *buf, int32_t nbytes)
{
	__ASSERT((buf != NULL), "Rx buffer should not be NULL");

	const struct dw_i3c_config *config = dev->config;
	int32_t i;
	uint32_t tmp;

	if (nbytes >= 4) {
		for (i = 0; i <= nbytes - 4; i += 4) {
			tmp = sys_read32(config->regs + RX_TX_DATA_PORT);
			memcpy(buf + i, &tmp, 4);
		}
	}
	if (nbytes & 3) {
		tmp = sys_read32(config->regs + RX_TX_DATA_PORT);
		memcpy(buf + (nbytes & ~3), &tmp, nbytes & 3);
	}
}
#endif /* CONFIG_I3C_CONTROLLER */
/**
 * @brief Write data to the Transmit FIFO of the I3C device.
 *
 * This function writes data to the Transmit FIFO of the I3C device specified by
 * the given device structure from the provided buffer.
 *
 * @param dev Pointer to the I3C device structure.
 * @param buf Pointer to the buffer containing the data to be written.
 * @param nbytes Number of bytes to write to the Transmit FIFO.
 */
static void write_tx_fifo(const struct device *dev, const uint8_t *buf, int32_t nbytes)
{
	__ASSERT((buf != NULL), "Tx buffer should not be NULL");

	const struct dw_i3c_config *config = dev->config;
	int32_t i;
	uint32_t tmp;

	if (nbytes >= 4) {
		for (i = 0; i <= nbytes - 4; i += 4) {
			memcpy(&tmp, buf + i, 4);
			sys_write32(tmp, config->regs + RX_TX_DATA_PORT);
		}
	}

	if (nbytes & 3) {
		tmp = 0;
		memcpy(&tmp, buf + (nbytes & ~3), nbytes & 3);
		sys_write32(tmp, config->regs + RX_TX_DATA_PORT);
	}
}

#ifdef CONFIG_I3C_USE_IBI
#ifdef CONFIG_I3C_CONTROLLER
/**
 * @brief Read data from the In-Band Interrupt (IBI) FIFO of the I3C device.
 *
 * This function reads data from the In-Band Interrupt (IBI) FIFO of the I3C device
 * specified by the given device structure and stores it in the provided buffer.
 *
 * @param dev Pointer to the I3C device structure.
 * @param buf Pointer to the buffer where the received IBI data will be stored.
 * @param nbytes Number of bytes to read from the IBI FIFO.
 */
static void read_ibi_fifo(const struct device *dev, uint8_t *buf, int32_t nbytes)
{
	__ASSERT((buf != NULL), "Rx IBI buffer should not be NULL");

	const struct dw_i3c_config *config = dev->config;
	int32_t i;
	uint32_t tmp;

	if (nbytes >= 4) {
		for (i = 0; i <= nbytes - 4; i += 4) {
			tmp = sys_read32(config->regs + IBI_QUEUE_STATUS);
			memcpy(buf + i, &tmp, 4);
		}
	}
	if (nbytes & 3) {
		tmp = sys_read32(config->regs + IBI_QUEUE_STATUS);
		memcpy(buf + (nbytes & ~3), &tmp, nbytes & 3);
	}
}
#endif /* CONFIG_I3C_CONTROLLER */
#endif /* CONFIG_I3C_USE_IBI */

#if defined(CONFIG_I3C_CONTROLLER) && defined(CONFIG_I3C_TARGET)
static void dw_i3c_deftgts_work_fn(struct k_work *work)
{
	struct dw_i3c_data *data = CONTAINER_OF(work, struct dw_i3c_data, deftgts_work);
	const struct device *dev = data->dev;
	const struct dw_i3c_config *config = dev->config;
	uint16_t count = data->deftgts_count;
	uint32_t sdct_val;
	uint8_t n = 0;

	if (data->common.deftgts) {
		k_free(data->common.deftgts);
		data->common.deftgts = NULL;
	}

	data->common.deftgts =
		k_malloc(sizeof(uint8_t) + sizeof(struct i3c_ccc_deftgts_active_controller) +
			 ((count - 1) * sizeof(struct i3c_ccc_deftgts_target)));
	if (!data->common.deftgts) {
		LOG_ERR("%s: Failed to allocate memory for DEFTGTS", dev->name);
		return;
	}

	data->common.deftgts->count = count - 1;

	/* First SDCT entry is the active controller */
	sdct_val = sys_read32(config->regs + data->dctstartaddr);
	data->common.deftgts->active_controller.addr = SDCT_DYNAMIC_ADDR(sdct_val);
	data->common.deftgts->active_controller.dcr = SDCT_DCR(sdct_val);
	data->common.deftgts->active_controller.bcr = SDCT_BCR(sdct_val);
	data->common.deftgts->active_controller.static_addr = SDCT_STATIC_ADDR(sdct_val);

	/* Remaining SDCT entries are targets */
	for (uint16_t i = 1; i < count; i++) {
		sdct_val = sys_read32(config->regs + data->dctstartaddr + (i * 4));
		uint8_t addr = SDCT_DYNAMIC_ADDR(sdct_val);
		uint8_t bcr = SDCT_BCR(sdct_val);
		uint8_t dcr_lvr = SDCT_DCR(sdct_val);
		uint8_t static_addr = SDCT_STATIC_ADDR(sdct_val);

		if (addr != 0) {
			data->common.deftgts->targets[n].addr = addr;
			data->common.deftgts->targets[n].dcr = dcr_lvr;
			data->common.deftgts->targets[n].bcr = bcr;
			data->common.deftgts->targets[n].static_addr = static_addr;
		} else {
			data->common.deftgts->targets[n].addr = 0;
			data->common.deftgts->targets[n].lvr = dcr_lvr;
			data->common.deftgts->targets[n].bcr = 0;
			data->common.deftgts->targets[n].static_addr = static_addr;
		}
		n++;
	}

	data->common.deftgts_refreshed = true;
}
#endif /* CONFIG_I3C_CONTROLLER && CONFIG_I3C_TARGET */

/**
 * @brief End the I3C transfer and process responses.
 *
 * This function is responsible for ending the I3C transfer on the specified
 * I3C device. It processes the responses received from the I3C bus, updating the
 * status and error information in the transfer structure.
 *
 * @param dev Pointer to the I3C device structure.
 */
static void dw_i3c_end_xfer(const struct device *dev)
{
	const struct dw_i3c_config *config = dev->config;
	struct dw_i3c_data *data = dev->data;
	struct dw_i3c_xfer *xfer = &data->xfer;
	struct dw_i3c_cmd *cmd;
	uint32_t nresp, resp;
	int i, ret = 0;
#ifdef CONFIG_I3C_TARGET
	uint32_t rx_data;
	int j, k;
#endif /* CONFIG_I3C_TARGET */

	nresp = QUEUE_STATUS_LEVEL_RESP(sys_read32(config->regs + QUEUE_STATUS_LEVEL));
	for (i = 0; i < nresp; i++) {
		uint8_t tid;

		resp = sys_read32(config->regs + RESPONSE_QUEUE_PORT);
		tid = RESPONSE_PORT_TID(resp);
		if (tid == 0xf) {
#if defined(CONFIG_I3C_CONTROLLER) && defined(CONFIG_I3C_TARGET)
			data->deftgts_count = RESPONSE_PORT_DATA_LEN(resp);
			k_work_submit(&data->deftgts_work);
#endif
			continue;
		}

		cmd = &xfer->cmds[tid];
		cmd->rx_len = RESPONSE_PORT_DATA_LEN(resp);
		cmd->error = RESPONSE_PORT_ERR_STATUS(resp);
#ifdef CONFIG_I3C_TARGET
		/* if we are in target mode */
		if (!dw_i3c_is_current_controller(dev)) {
			const struct i3c_target_callbacks *target_cb =
				data->target_config->callbacks;

			for (j = 0; j < cmd->rx_len; j += 4) {
				rx_data = sys_read32(config->regs + RX_TX_DATA_PORT);
				if (target_cb != NULL && target_cb->write_received_cb != NULL) {
					/* Call write received cb for each remaining byte  */
					for (k = 0; k < MIN(4, cmd->rx_len - j); k++) {
						target_cb->write_received_cb(data->target_config,
								(rx_data >> (8 * k)) & 0xff);
					}
				}
			}

			if (target_cb != NULL && target_cb->stop_cb != NULL) {
				/*
				 * TODO: modify API to include status, such as success or aborted
				 * transfer
				 */
				target_cb->stop_cb(data->target_config);
			}
		}
#endif /* CONFIG_I3C_TARGET */
	}

	for (i = 0; i < nresp; i++) {
		switch (xfer->cmds[i].error) {
		case RESPONSE_NO_ERROR:
			break;
		case RESPONSE_ERROR_PARITY:
		case RESPONSE_ERROR_IBA_NACK:
		case RESPONSE_ERROR_TRANSF_ABORT:
		case RESPONSE_ERROR_CRC:
		case RESPONSE_ERROR_FRAME:
			ret = -EIO;
			break;
		case RESPONSE_ERROR_OVER_UNDER_FLOW:
			ret = -ENOSPC;
			break;
		case RESPONSE_ERROR_I2C_W_NACK_ERR:
		case RESPONSE_ERROR_ADDRESS_NACK:
			ret = -ENXIO;
			break;
		default:
			ret = -EINVAL;
			break;
		}
	}
	xfer->ret = ret;

	if (ret < 0) {
		enum dw_i3c_isr_error_action isr_action =
			dw_i3c_isr_error_recovery_action(dev, ret);

		if (isr_action == DW_I3C_ISR_ERROR_DEFER_RECOVER) {
			k_sem_give(&data->sem_xfer);
			return;
		}

		sys_write32(RESET_CTRL_RX_FIFO | RESET_CTRL_TX_FIFO | RESET_CTRL_RESP_QUEUE |
			    RESET_CTRL_CMD_QUEUE,
			    config->regs + RESET_CTRL);
		sys_write32(sys_read32(config->regs + DEVICE_CTRL) | DEV_CTRL_RESUME,
			    config->regs + DEVICE_CTRL);
	}
	k_sem_give(&data->sem_xfer);
}

/**
 * @brief Start an I3C transfer on the specified device.
 *
 * This function initiates an I3C transfer on the specified I3C device by pushing
 * data to the Transmit FIFO (TXFIFO) and enqueuing commands to the command queue.
 *
 * @param dev Pointer to the I3C device structure.
 */
static void start_xfer(const struct device *dev)
{
	const struct dw_i3c_config *config = dev->config;
	struct dw_i3c_data *data = dev->data;
	struct dw_i3c_xfer *xfer = &data->xfer;
	struct dw_i3c_cmd *cmd;
	uint32_t thld_ctrl;
	int32_t i;

	/* Push data to TXFIFO */
	for (i = 0; i < xfer->ncmds; i++) {
		cmd = &xfer->cmds[i];
		/* Not all the commands use write_tx_fifo function */
		if (cmd->buf != NULL) {
			write_tx_fifo(dev, cmd->buf, cmd->tx_len);
		}
	}

	thld_ctrl = sys_read32(config->regs + QUEUE_THLD_CTRL);
	thld_ctrl &= ~QUEUE_THLD_CTRL_RESP_BUF_MASK;
	thld_ctrl |= QUEUE_THLD_CTRL_RESP_BUF(xfer->ncmds);
	sys_write32(thld_ctrl, config->regs + QUEUE_THLD_CTRL);

	/* Enqueue CMD */
	for (i = 0; i < xfer->ncmds; i++) {
		cmd = &xfer->cmds[i];
		/* Only cmd_lo is used when it is a target */
		if (dw_i3c_is_current_controller(dev)) {
			sys_write32(cmd->cmd_hi, config->regs + COMMAND_QUEUE_PORT);
		}
		sys_write32(cmd->cmd_lo, config->regs + COMMAND_QUEUE_PORT);
	}
}

#ifdef CONFIG_I3C_CONTROLLER
/**
 * @brief Poll until the controller transfer state machine reports idle.
 *
 * @retval 0 Controller is idle.
 * @retval -EBUSY Still busy when the timeout expired.
 */
static int dw_i3c_wait_ctrl_idle(const struct device *dev, uint32_t timeout_us)
{
	const struct dw_i3c_config *config = dev->config;
	uint32_t present_state;

	while (timeout_us--) {
		present_state = sys_read32(config->regs + PRESENT_STATE);
		if (PRESENT_STATE_CM_TFR_STS(present_state) == CM_TFR_STS_IDLE) {
			return 0;
		}
		k_busy_wait(1);
	}

	return -EBUSY;
}

/**
 * @brief Wait for a written RESUME to be consumed.
 *
 * RESUME reads back set until the controller has acted on it
 *
 * @retval 0 RESUME cleared.
 * @retval -ETIMEDOUT Still set when the timeout expired.
 */
static int dw_i3c_wait_resume_clear(const struct device *dev, uint32_t timeout_us)
{
	const struct dw_i3c_config *config = dev->config;

	while (timeout_us--) {
		if ((sys_read32(config->regs + DEVICE_CTRL) & DEV_CTRL_RESUME) == 0U) {
			return 0;
		}
		k_busy_wait(1);
	}

	return -ETIMEDOUT;
}

/**
 * @brief Drop a RESUME request the controller has not consumed
 */
static void dw_i3c_clear_resume(const struct device *dev)
{
	const struct dw_i3c_config *config = dev->config;
	uint32_t dev_ctrl = sys_read32(config->regs + DEVICE_CTRL);

	if ((dev_ctrl & DEV_CTRL_RESUME) != 0U) {
		sys_write32(dev_ctrl & ~DEV_CTRL_RESUME, config->regs + DEVICE_CTRL);
	}
}

/**
 * @brief Pulse RESUME and wait for the controller to take it.
 *
 * Leaves RESUME clear whether or not the controller consumed the pulse
 *
 * @retval 0 Pulse consumed.
 * @retval -ETIMEDOUT Pulse was not consumed before the timeout.
 */
static int dw_i3c_pulse_resume(const struct device *dev)
{
	const struct dw_i3c_config *config = dev->config;
	int ret;

	sys_write32(sys_read32(config->regs + DEVICE_CTRL) | DEV_CTRL_RESUME,
		    config->regs + DEVICE_CTRL);

	ret = dw_i3c_wait_resume_clear(dev, DW_I3C_RESUME_TIMEOUT_US);
	if (ret != 0) {
		dw_i3c_clear_resume(dev);
	}

	return ret;
}

/**
 * @brief Flush the FIFOs and queues
 *
 * Signalling is masked across the flush so it cannot raise an ISR
 */
static void dw_i3c_flush_queues(const struct device *dev)
{
	const struct dw_i3c_config *config = dev->config;
	unsigned int irq_key = irq_lock();
	uint32_t saved_intr = sys_read32(config->regs + INTR_SIGNAL_EN);

	sys_write32(0U, config->regs + INTR_SIGNAL_EN);
	sys_write32(0xFFFFFFFFU, config->regs + INTR_STATUS);
	sys_write32(RESET_CTRL_RX_FIFO | RESET_CTRL_TX_FIFO | RESET_CTRL_RESP_QUEUE |
			    RESET_CTRL_CMD_QUEUE | RESET_CTRL_IBI_QUEUE,
		    config->regs + RESET_CTRL);
	k_busy_wait(DW_I3C_FLUSH_SETTLE_US);
	sys_write32(saved_intr, config->regs + INTR_SIGNAL_EN);
	irq_unlock(irq_key);
}

/**
 * @brief Settle the controller before bus initialization
 *
 * Leaves the controller idle with no RESUME pending, so the first CCC of bus
 * initialization does not inherit state from whatever ran before reset
 *
 * @retval 0 Controller is idle
 * @retval -errno Propagated from a platform hook or from the full reset
 */
static int dw_i3c_prepare_bus_init(const struct device *dev)
{
	const struct dw_i3c_config *config = dev->config;
	const uint32_t flush_mask = RESET_CTRL_RX_FIFO | RESET_CTRL_TX_FIFO |
					 RESET_CTRL_RESP_QUEUE | RESET_CTRL_CMD_QUEUE;
	uint32_t timeout;
	int ret;

	ret = dw_i3c_wait_ctrl_idle(dev, DW_I3C_CTRL_IDLE_TIMEOUT_US);
	if (ret == 0) {
		if ((sys_read32(config->regs + DEVICE_CTRL) & DEV_CTRL_RESUME) == 0U) {
			return 0;
		}

		dw_i3c_clear_resume(dev);
		if (dw_i3c_wait_resume_clear(dev, DW_I3C_RESUME_TIMEOUT_US) == 0) {
			return 0;
		}
	}

	/* Flush stale state so first CCC does not inherit old queue contents. */
	sys_write32(flush_mask, config->regs + RESET_CTRL);

	ret = dw_i3c_pre_resume_ctrl(dev);
	if (ret != 0) {
		return ret;
	}

	timeout = DW_I3C_FLUSH_TIMEOUT_US;
	while ((sys_read32(config->regs + RESET_CTRL) & flush_mask) && --timeout) {
		k_busy_wait(1);
	}
	if (timeout == 0U) {
		LOG_WRN("%s: FIFO/queue flush did not self-clear; continuing init", dev->name);
	}

	/* Clear any stale sticky interrupt status from pre-init sequencing. */
	sys_write32(INTR_ALL, config->regs + INTR_STATUS);

	if (DW_I3C_OPS(config) && config->ops->pre_resume_ctrl) {
		dw_i3c_clear_resume(dev);
	} else {
		(void)dw_i3c_pulse_resume(dev);
	}

	ret = dw_i3c_wait_ctrl_idle(dev, DW_I3C_CTRL_IDLE_TIMEOUT_US);
	if (ret != 0) {
		return dw_i3c_full_reset(dev);
	}

	return 0;
}

#endif /* CONFIG_I3C_CONTROLLER */

#ifdef CONFIG_I3C_CONTROLLER
/**
 * @brief Get the position of an I3C device with the specified address.
 *
 * This function retrieves the position (ID) of an I3C device with the specified
 * address on the I3C bus associated with the provided I3C device structure. This
 * utilizes the controller private data for where the id reg is stored.
 *
 * @param dev Pointer to the I3C device structure.
 * @param addr I3C address of the device whose position is to be retrieved.
 * @param sa True if looking up by Static Address, False if by Dynamic Address
 *
 * @return The position (ID) of the device on success, or a negative error code
 *         if the device with the given address is not found.
 */
static int get_i3c_addr_pos(const struct device *dev, uint8_t addr, bool sa)
{
	struct dw_i3c_i2c_dev_data *dw_i3c_device_data;
	struct i3c_device_desc *desc = sa ? i3c_dev_list_i3c_static_addr_find(dev, addr)
					  : i3c_dev_list_i3c_addr_find(dev, addr);

	if (desc == NULL) {
		return -ENODEV;
	}

	dw_i3c_device_data = desc->controller_priv;

	return dw_i3c_device_data->id;
}

/**
 * @brief Recover the controller when it is not idle before a submission
 *
 * @retval 0 Controller was already idle, or recovery succeeded
 * @retval -errno Propagated from @ref dw_i3c_recover_bus
 */
static int dw_i3c_ensure_xfer_ready(const struct device *dev)
{
	const struct dw_i3c_config *config = dev->config;
	uint32_t present_state = sys_read32(config->regs + PRESENT_STATE);
	uint32_t cm_tfr_sts = PRESENT_STATE_CM_TFR_STS(present_state);

	if (cm_tfr_sts == CM_TFR_STS_IDLE) {
		/* Only the transfer state machine matters here: CONTROLLER_IDLE
		 * additionally requires the queues and data buffers to be empty,
		 * so it can read 0 with an idle FSM and nothing to recover.
		 */
		return 0;
	}

	return dw_i3c_recover_bus(dev);
}

/**
 * @brief Transfer messages in I3C mode.
 *
 * @param dev Pointer to device driver instance.
 * @param target Pointer to target device descriptor.
 * @param msgs Pointer to I3C messages.
 * @param num_msgs Number of messages to transfers.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 * @retval -EINVAL Address not registered
 */
static int dw_i3c_xfers(const struct device *dev, struct i3c_device_desc *target,
			struct i3c_msg *msgs, uint8_t num_msgs)
{
	const struct dw_i3c_config *config = dev->config;
	struct dw_i3c_data *data = dev->data;
	struct dw_i3c_xfer *xfer = &data->xfer;
	enum dw_i3c_retry_action retry_action;
	int64_t timeout_ms;
	int64_t deadline;
	bool first_cmd_addr_nack;
	bool retried_after_recover = false;
	int32_t ret, i, pos, nrxwords = 0, ntxwords = 0;

	if (!dw_i3c_is_current_controller(dev)) {
		return -EACCES;
	}

	if (num_msgs > data->cmdfifodepth) {
		return -ENOTSUP;
	}

	pos = get_i3c_addr_pos(dev, target->dynamic_addr, false);
	if (pos < 0) {
		LOG_ERR("%s: Invalid slave device", dev->name);
		return -EINVAL;
	}

	for (i = 0; i < num_msgs; i++) {
		if (msgs[i].flags & I2C_MSG_READ) {
			nrxwords += DIV_ROUND_UP(msgs[i].len, 4);
		} else {
			ntxwords += DIV_ROUND_UP(msgs[i].len, 4);
		}
	}

	if (ntxwords > data->txfifodepth || nrxwords > data->rxfifodepth) {
		return -ENOTSUP;
	}

	ret = k_mutex_lock(&data->mt, K_MSEC(1000));
	if (ret) {
		LOG_ERR("%s: Mutex err (%d)", dev->name, ret);
		return ret;
	}

	/* Under the mutex: a concurrent transfer cannot halt the controller
	 * between this check and the submission below.
	 */
	ret = dw_i3c_ensure_xfer_ready(dev);
	if (ret != 0) {
		k_mutex_unlock(&data->mt);
		return ret;
	}

	pm_device_busy_set(dev);
	timeout_ms = CONFIG_I3C_DW_RW_TIMEOUT_MS;

	memset(xfer, 0, sizeof(struct dw_i3c_xfer));

	xfer->ncmds = num_msgs;
	xfer->ret = -1;

	for (i = 0; i < num_msgs; i++) {
		struct dw_i3c_cmd *cmd = &xfer->cmds[i];

		cmd->cmd_hi = COMMAND_PORT_ARG_DATA_LEN(msgs[i].len) | COMMAND_PORT_TRANSFER_ARG;
		cmd->cmd_lo = COMMAND_PORT_TID(i) | COMMAND_PORT_DEV_INDEX(pos) | COMMAND_PORT_ROC;

		cmd->buf = msgs[i].buf;

		if (msgs[i].flags & I3C_MSG_NBCH) {
			sys_write32(sys_read32(config->regs + DEVICE_CTRL) & ~DEV_CTRL_IBA_INCLUDE,
				    config->regs + DEVICE_CTRL);
		} else {
			sys_write32(sys_read32(config->regs + DEVICE_CTRL) | DEV_CTRL_IBA_INCLUDE,
				    config->regs + DEVICE_CTRL);
		}

		if (msgs[i].flags & I3C_MSG_READ) {
			uint8_t rd_speed;

			if (msgs[i].flags & I3C_MSG_HDR) {
				/* Set read command bit for DDR and TS */
				cmd->cmd_lo |= COMMAND_PORT_CP |
					       COMMAND_PORT_CMD(BIT(7) | (msgs[i].hdr_cmd_code &
									  GENMASK(6, 0)));
				if (msgs[i].hdr_mode & I3C_MSG_HDR_DDR) {
					if (data->common.ctrl_config.supported_hdr &
					    I3C_MSG_HDR_DDR) {
						rd_speed = COMMAND_PORT_SPEED_I3C_DDR;
					} else {
						/* DDR support not configured with this */
						LOG_ERR("%s: HDR-DDR not supported", dev->name);
						ret = -ENOTSUP;
						goto error;
					}
				} else if (msgs[i].hdr_mode & I3C_MSG_HDR_TSP ||
					   msgs[i].hdr_mode & I3C_MSG_HDR_TSL) {
					if (data->common.ctrl_config.supported_hdr &
					    (I3C_MSG_HDR_TSP | I3C_MSG_HDR_TSL)) {
						rd_speed = COMMAND_PORT_SPEED_I3C_TS;
					} else {
						/* TS support not configured with this */
						LOG_ERR("%s: HDR-TS not supported", dev->name);
						ret = -ENOTSUP;
						goto error;
					}
				} else {
					LOG_ERR("%s: HDR %d not supported", dev->name,
						msgs[i].hdr_mode);
					ret = -ENOTSUP;
					goto error;
				}
			} else {
				rd_speed = I3C_CCC_GETMXDS_MAXRD_MAX_SDR_FSCL(
					target->data_speed.maxrd);
			}

			cmd->cmd_lo |= (COMMAND_PORT_READ_TRANSFER | COMMAND_PORT_SPEED(rd_speed));
			cmd->rx_len = msgs[i].len;
		} else {
			uint8_t wr_speed;

			if (msgs[i].flags & I3C_MSG_HDR) {
				cmd->cmd_lo |=
					COMMAND_PORT_CP |
					COMMAND_PORT_CMD(msgs[i].hdr_cmd_code & GENMASK(6, 0));
				if (msgs[i].hdr_mode & I3C_MSG_HDR_DDR) {
					if (data->common.ctrl_config.supported_hdr &
					    I3C_MSG_HDR_DDR) {
						wr_speed = COMMAND_PORT_SPEED_I3C_DDR;
					} else {
						/* DDR support not configured with this */
						LOG_ERR("%s: HDR-DDR not supported", dev->name);
						ret = -ENOTSUP;
						goto error;
					}
				} else if (msgs[i].hdr_mode & I3C_MSG_HDR_TSP ||
					   msgs[i].hdr_mode & I3C_MSG_HDR_TSL) {
					if (data->common.ctrl_config.supported_hdr &
					    (I3C_MSG_HDR_TSP | I3C_MSG_HDR_TSL)) {
						wr_speed = COMMAND_PORT_SPEED_I3C_TS;
					} else {
						/* TS support not configured with this */
						LOG_ERR("%s: HDR-TS not supported", dev->name);
						ret = -ENOTSUP;
						goto error;
					}
				} else {
					LOG_ERR("%s: HDR %d not supported", dev->name,
						msgs[i].hdr_mode);
					ret = -ENOTSUP;
					goto error;
				}
			} else {
				wr_speed = I3C_CCC_GETMXDS_MAXWR_MAX_SDR_FSCL(
					target->data_speed.maxwr);
			}

			cmd->cmd_lo |= COMMAND_PORT_SPEED(wr_speed);
			cmd->tx_len = msgs[i].len;
		}

		if (i == (num_msgs - 1)) {
			cmd->cmd_lo |= COMMAND_PORT_TOC;
		}
	}

	while (true) {
		k_sem_reset(&data->sem_xfer);
		start_xfer(dev);

		deadline = k_uptime_get() + timeout_ms;
		ret = -EAGAIN;

		while (k_uptime_get() < deadline) {
			ret = k_sem_take(&data->sem_xfer, K_MSEC(1));
			if (ret == 0) {
				break;
			}
		}

		if (ret == 0) {
			break;
		}

		LOG_ERR("%s: Semaphore err (%d)", dev->name, ret);

		first_cmd_addr_nack = xfer->ncmds > 0 &&
				      xfer->cmds[0].error == RESPONSE_ERROR_ADDRESS_NACK;

		retry_action = dw_i3c_timeout_retry_action(dev, DW_I3C_TIMEOUT_OP_XFERS,
							   retried_after_recover,
							   first_cmd_addr_nack);

		if (!retried_after_recover && retry_action == DW_I3C_RETRY_LIGHT_RECOVER) {
			ret = dw_i3c_recover_bus_locked_light(dev);
			if (ret != 0) {
				goto error;
			}
			retried_after_recover = true;
			continue;
		}

		if (!retried_after_recover && retry_action == DW_I3C_RETRY_FULL_RECOVER) {
			ret = dw_i3c_recover_bus(dev);
			if (ret != 0) {
				goto error;
			}
			retried_after_recover = true;
			continue;
		}

		/* Final timeout path: sanitize local controller state before exit. */
		(void)dw_i3c_recover_bus_locked_light(dev);

		goto error;
	}

	for (i = 0; i < xfer->ncmds; i++) {
		msgs[i].num_xfer = (msgs[i].flags & I3C_MSG_READ) ? xfer->cmds[i].rx_len
								  : xfer->cmds[i].tx_len;
		if (xfer->cmds[i].rx_len && !xfer->cmds[i].error) {
			read_rx_fifo(dev, xfer->cmds[i].buf, xfer->cmds[i].rx_len);
		}
	}

	ret = xfer->ret;

error:
	pm_device_busy_clear(dev);
	k_mutex_unlock(&data->mt);

	return ret;
}

static int dw_i3c_i2c_attach_device(const struct device *dev, struct i3c_i2c_device_desc *desc)
{
	const struct dw_i3c_config *config = dev->config;
	struct dw_i3c_data *data = dev->data;
	int pos;

	pos = get_free_pos(data->free_pos);
	if (pos < 0) {
		return -ENOSPC;
	}

	data->dw_i3c_i2c_priv_data[pos].id = pos;
	desc->controller_priv = &(data->dw_i3c_i2c_priv_data[pos]);
	data->free_pos &= ~BIT(pos);

	sys_write32(DEV_ADDR_TABLE_LEGACY_I2C_DEV | DEV_ADDR_TABLE_STATIC_ADDR(desc->addr),
		    config->regs + DEV_ADDR_TABLE_LOC(data->datstartaddr, pos));

	return 0;
}

static void dw_i3c_i2c_detach_device(const struct device *dev, struct i3c_i2c_device_desc *desc)
{
	const struct dw_i3c_config *config = dev->config;
	struct dw_i3c_data *data = dev->data;
	struct dw_i3c_i2c_dev_data *dw_i2c_device_data = desc->controller_priv;

	__ASSERT_NO_MSG(dw_i2c_device_data != NULL);

	sys_write32(0,
		    config->regs + DEV_ADDR_TABLE_LOC(data->datstartaddr, dw_i2c_device_data->id));
	data->free_pos |= BIT(dw_i2c_device_data->id);
	desc->controller_priv = NULL;
}

/**
 * @brief Transfer messages in I2C mode.
 *
 * @param dev Pointer to device driver instance.
 * @param target Pointer to target device descriptor.
 * @param msgs Pointer to I2C messages.
 * @param num_msgs Number of messages to transfers.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 * @retval -EINVAL Address not registered
 */
static int dw_i3c_i2c_transfer(const struct device *dev, struct i3c_i2c_device_desc *target,
			       struct i2c_msg *msgs, uint8_t num_msgs)
{
	const struct dw_i3c_config *config = dev->config;
	struct dw_i3c_data *data = dev->data;
	struct dw_i3c_xfer *xfer = &data->xfer;
	int32_t ret, i, pos, nrxwords = 0, ntxwords = 0;

	if (!dw_i3c_is_current_controller(dev)) {
		return -EACCES;
	}

	if (num_msgs > data->cmdfifodepth) {
		return -ENOTSUP;
	}

	for (i = 0; i < num_msgs; i++) {
		if (msgs[i].flags & I2C_MSG_READ) {
			nrxwords += DIV_ROUND_UP(msgs[i].len, 4);
		} else {
			ntxwords += DIV_ROUND_UP(msgs[i].len, 4);
		}
	}

	if (ntxwords > data->txfifodepth || nrxwords > data->rxfifodepth) {
		return -ENOTSUP;
	}

	ret = k_mutex_lock(&data->mt, K_MSEC(1000));
	if (ret) {
		LOG_ERR("%s: Mutex err (%d)", dev->name, ret);
		return ret;
	}

	pm_device_busy_set(dev);

	/* In order limit the number of retaining registers occupied by connected devices,
	 * I2C devices are only configured during transfers. This allows the number of devices
	 * to be larger than the number of retaining registers on mixed buses.
	 */
	ret = dw_i3c_i2c_attach_device(dev, target);
	if (ret != 0) {
		LOG_ERR("%s: Failed to attach I2C device (%d)", dev->name, ret);
		goto error_attach;
	}
	pos = ((struct dw_i3c_i2c_dev_data *)target->controller_priv)->id;

	memset(xfer, 0, sizeof(struct dw_i3c_xfer));

	xfer->ncmds = num_msgs;
	xfer->ret = -1;

	for (i = 0; i < num_msgs; i++) {
		struct dw_i3c_cmd *cmd = &xfer->cmds[i];

		cmd->cmd_hi = COMMAND_PORT_ARG_DATA_LEN(msgs[i].len) | COMMAND_PORT_TRANSFER_ARG;
		cmd->cmd_lo = COMMAND_PORT_TID(i) | COMMAND_PORT_DEV_INDEX(pos) | COMMAND_PORT_ROC;

		cmd->buf = msgs[i].buf;

		if (msgs[i].flags & I2C_MSG_READ) {
			uint8_t rd_speed = I3C_LVR_I2C_MODE(target->lvr) == I3C_LVR_I2C_FM_MODE
						   ? COMMAND_PORT_SPEED_I2C_FM
						   : COMMAND_PORT_SPEED_I2C_FMP;

			cmd->cmd_lo |= (COMMAND_PORT_READ_TRANSFER | COMMAND_PORT_SPEED(rd_speed));
			cmd->rx_len = msgs[i].len;
		} else {
			uint8_t wr_speed = I3C_LVR_I2C_MODE(target->lvr) == I3C_LVR_I2C_FM_MODE
						   ? COMMAND_PORT_SPEED_I2C_FM
						   : COMMAND_PORT_SPEED_I2C_FMP;

			cmd->cmd_lo |= COMMAND_PORT_SPEED(wr_speed);
			cmd->tx_len = msgs[i].len;
		}

		if (i == (num_msgs - 1)) {
			cmd->cmd_lo |= COMMAND_PORT_TOC;
		}
	}

	/* Do not send broadcast address (0x7E) with I2C transfers */
	sys_write32(sys_read32(config->regs + DEVICE_CTRL) & ~DEV_CTRL_IBA_INCLUDE,
		    config->regs + DEVICE_CTRL);

	start_xfer(dev);

	ret = k_sem_take(&data->sem_xfer, K_MSEC(CONFIG_I3C_DW_RW_TIMEOUT_MS));
	if (ret) {
		LOG_ERR("%s: Semaphore err (%d)", dev->name, ret);
		goto error;
	}

	for (i = 0; i < xfer->ncmds; i++) {
		if (xfer->cmds[i].rx_len && !xfer->cmds[i].error) {
			read_rx_fifo(dev, xfer->cmds[i].buf, xfer->cmds[i].rx_len);
		}
	}

	ret = xfer->ret;

error:
	dw_i3c_i2c_detach_device(dev, target);
error_attach:
	pm_device_busy_clear(dev);
	k_mutex_unlock(&data->mt);

	return ret;
}

/**
 * Find a registered I2C target device.
 *
 * Controller only API.
 *
 * This returns the I2C device descriptor of the I2C device
 * matching the device address @p addr.
 *
 * @param dev Pointer to controller device driver instance.
 * @param id I2C target device address.
 *
 * @return @see i3c_i2c_device_find.
 */
static struct i3c_i2c_device_desc *dw_i3c_i2c_device_find(const struct device *dev, uint16_t addr)
{
	return i3c_dev_list_i2c_addr_find(dev, addr);
}

/**
 * @brief Transfer messages in I2C mode.
 *
 * @see i2c_transfer
 *
 * @param dev Pointer to device driver instance.
 * @param msgs Pointer to I2C messages.
 * @param num_msgs Number of messages to transfers.
 * @param addr Address of the I2C target device.
 *
 * @return @see i2c_transfer
 */
static int dw_i3c_i2c_api_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
				   uint16_t addr)
{
	struct i3c_i2c_device_desc *i2c_dev = dw_i3c_i2c_device_find(dev, addr);

	if (i2c_dev == NULL) {
		return -ENODEV;
	}

	return dw_i3c_i2c_transfer(dev, i2c_dev, msgs, num_msgs);
}

static int dw_i3c_init_scl_timing(const struct device *dev, struct i3c_config_controller *ctrl_cfg);

/**
 * @brief Configure I2C operation of a host controller.
 *
 * @see i2c_configure
 *
 * @param dev        Pointer to device driver instance.
 * @param dev_config @see i2c_configure
 *
 * @return @see i2c_configure
 */
static int dw_i3c_i2c_api_configure(const struct device *dev, uint32_t dev_config)
{
	struct dw_i3c_data *data = dev->data;
	struct i3c_config_controller *ctrl_config = &data->common.ctrl_config;
	uint32_t i2c_scl_hz;
	int ret;

	/* Note: this only affects devices configured for FM in the device tree. */
	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		i2c_scl_hz = 100000;
		break;
	case I2C_SPEED_FAST:
		i2c_scl_hz = 400000;
		break;
	case I2C_SPEED_FAST_PLUS:
		i2c_scl_hz = 1000000;
		break;
	default:
		return -EINVAL;
	}

	k_mutex_lock(&data->mt, K_FOREVER);

	ctrl_config->scl.i2c = i2c_scl_hz;
	ret = dw_i3c_init_scl_timing(dev, ctrl_config);

	k_mutex_unlock(&data->mt);

	return ret;
}
#endif /* CONFIG_I3C_CONTROLLER */
#ifdef CONFIG_I3C_USE_IBI
#ifdef CONFIG_I3C_CONTROLLER
static int dw_i3c_controller_ibi_hj_response(const struct device *dev, bool ack)
{
	const struct dw_i3c_config *config = dev->config;
	uint32_t ctrl = sys_read32(config->regs + DEVICE_CTRL);

	if (ack) {
		ctrl &= ~DEV_CTRL_HOT_JOIN_NACK;
	} else {
		ctrl |= DEV_CTRL_HOT_JOIN_NACK;
	}

	sys_write32(ctrl, config->regs + DEVICE_CTRL);

	return 0;
}

static int dw_i3c_controller_ibi_crr_response(struct i3c_device_desc *target, bool ack)
{
	const struct device *dev = target->bus;
	const struct dw_i3c_config *config = dev->config;
	struct dw_i3c_data *data = dev->data;
	int pos;
	uint32_t reg;

	pos = get_i3c_addr_pos(dev, target->dynamic_addr, false);
	if (pos < 0) {
		return pos;
	}

	reg = sys_read32(config->regs + DEV_ADDR_TABLE_LOC(data->datstartaddr, pos));
	if (ack) {
		reg &= ~DEV_ADDR_TABLE_MR_REJECT;
	} else {
		reg |= DEV_ADDR_TABLE_MR_REJECT;
	}
	sys_write32(reg, config->regs + DEV_ADDR_TABLE_LOC(data->datstartaddr, pos));

	return 0;
}

static int i3c_dw_endis_ibi(const struct device *dev, struct i3c_device_desc *target, bool en)
{
	struct dw_i3c_data *data = dev->data;
	const struct dw_i3c_config *config = dev->config;
	uint32_t role = data->role;
	uint32_t bitpos, sir_con;
	struct i3c_ccc_events i3c_events;
	int ret;
	int pos;

	if (!DW_IBI_REJECT_VIA_REG(role)) {

		/* controller-only: SIR via DAT entry */

		pos = get_i3c_addr_pos(dev, target->dynamic_addr, false);
		if (pos < 0) {
			LOG_ERR("%s: Invalid Slave address", dev->name);
			return pos;
		}

		uint32_t reg =
			sys_read32(config->regs + DEV_ADDR_TABLE_LOC(data->datstartaddr, pos));

		if (i3c_ibi_has_payload(target)) {
			reg |= DEV_ADDR_TABLE_IBI_WITH_DATA;
		} else {
			reg &= ~DEV_ADDR_TABLE_IBI_WITH_DATA;
		}
		if (en) {
			reg &= ~DEV_ADDR_TABLE_SIR_REJECT;
		} else {
			reg |= DEV_ADDR_TABLE_SIR_REJECT;
		}
		sys_write32(reg, config->regs + DEV_ADDR_TABLE_LOC(data->datstartaddr, pos));

	} else {

		/* dual-role: SIR via IBI_SIR_REQ_REJECT */

		sir_con = sys_read32(config->regs + IBI_SIR_REQ_REJECT);
		/* TODO: what is this macro doing?? */
		bitpos = IBI_SIR_REQ_ID(target->dynamic_addr);

		if (en) {
			sir_con &= ~BIT(bitpos);
		} else {
			sir_con |= BIT(bitpos);
		}
		sys_write32(sir_con, config->regs + IBI_SIR_REQ_REJECT);
	}

	/* Tell target to enable IBI */
	i3c_events.events = I3C_CCC_EVT_INTR;
	ret = i3c_ccc_do_events_set(target, en, &i3c_events);
	if (ret != 0) {
		LOG_ERR("%s: Error sending IBI ENEC for 0x%02x (%d)", dev->name,
			target->dynamic_addr, ret);
		return ret;
	}

	return 0;
}

static int dw_i3c_controller_enable_ibi(const struct device *dev, struct i3c_device_desc *target)
{
	return i3c_dw_endis_ibi(dev, target, true);
}

static int dw_i3c_controller_disable_ibi(const struct device *dev, struct i3c_device_desc *target)
{
	return i3c_dw_endis_ibi(dev, target, false);
}

static void dw_i3c_handle_tir(const struct device *dev, uint32_t ibi_status)
{
	uint8_t ibi_data[CONFIG_I3C_IBI_MAX_PAYLOAD_SIZE];
	uint8_t addr, len;
	int pos;

	addr = IBI_QUEUE_IBI_ADDR(ibi_status);
	len = IBI_QUEUE_STATUS_DATA_LEN(ibi_status);

	pos = get_i3c_addr_pos(dev, addr, false);
	if (pos < 0) {
		LOG_ERR("%s: Invalid Slave address", dev->name);
		return;
	}

	struct i3c_device_desc *desc = i3c_dev_list_i3c_addr_find(dev, addr);

	if (desc == NULL) {
		return;
	}

	if (len > 0) {
		read_ibi_fifo(dev, ibi_data, len);
	}

	if (i3c_ibi_work_enqueue_target_irq(desc, ibi_data, len) != 0) {
		LOG_ERR("%s: Error enqueue IBI IRQ work", dev->name);
	}
}

static void dw_i3c_handle_hj(const struct device *dev, uint32_t ibi_status)
{
	if (IBI_QUEUE_STATUS_IBI_STS(ibi_status) & BIT(3)) {
		LOG_DBG("%s: NAK for HJ", dev->name);
		return;
	}

	if (i3c_ibi_work_enqueue_hotjoin(dev) != 0) {
		LOG_ERR("%s: Error enqueue IBI HJ work", dev->name);
	}
}

static void dw_i3c_handle_mr(const struct device *dev, uint32_t ibi_status)
{
	uint8_t addr = IBI_QUEUE_IBI_ADDR(ibi_status);

	if (IBI_QUEUE_STATUS_IBI_STS(ibi_status) & BIT(3)) {
		LOG_DBG("%s: NAK for MR from 0x%02x", dev->name, addr);
		return;
	}

	struct i3c_device_desc *desc = i3c_dev_list_i3c_addr_find(dev, addr);

	if (desc == NULL) {
		LOG_ERR("%s: MR from unknown addr 0x%02x", dev->name, addr);
		return;
	}

	if (i3c_ibi_work_enqueue_controller_request(desc) != 0) {
		LOG_ERR("%s: Error enqueue IBI MR work", dev->name);
	}
}

static void ibis_handle(const struct device *dev)
{
	const struct dw_i3c_config *config = dev->config;
	uint32_t nibis, ibi_stat;
	int32_t i;

	nibis = sys_read32(config->regs + QUEUE_STATUS_LEVEL);
	nibis = QUEUE_STATUS_IBI_STATUS_CNT(nibis);
	for (i = 0; i < nibis; i++) {
		ibi_stat = sys_read32(config->regs + IBI_QUEUE_STATUS);
		if (IBI_TYPE_SIRQ(ibi_stat)) {
			dw_i3c_handle_tir(dev, ibi_stat);
		} else if (IBI_TYPE_HJ(ibi_stat)) {
			dw_i3c_handle_hj(dev, ibi_stat);
		} else if (IBI_TYPE_MR(ibi_stat)) {
			dw_i3c_handle_mr(dev, ibi_stat);
		} else {
			LOG_ERR("%s: Unknown IBI type", dev->name);
		}
	}
}
#endif /* CONFIG_I3C_CONTROLLER */
#ifdef CONFIG_I3C_TARGET
static int dw_i3c_target_ibi_raise_hj(const struct device *dev)
{
	const struct dw_i3c_config *config = dev->config;
	struct dw_i3c_data *data = dev->data;
	int ret;

	if (!(sys_read32(config->regs + HW_CAPABILITY) & HW_CAPABILITY_SLV_HJ_CAP)) {
		LOG_ERR("%s: HJ not supported", dev->name);
		return -ENOTSUP;
	}
	if (sys_read32(config->regs + DEVICE_ADDR) & DEVICE_ADDR_DYNAMIC_ADDR_VALID) {
		LOG_ERR("%s: HJ not available, DA already assigned", dev->name);
		return -EACCES;
	}
	/* if this is set, then it is assumed it is already trying */
	if ((sys_read32(config->regs + SLV_EVENT_STATUS) & SLV_EVENT_STATUS_HJ_EN)) {
		LOG_ERR("%s: HJ requests are currently disabled by DISEC", dev->name);
		return -EAGAIN;
	}

	/*
	 * This is issued auto-magically by the IP when certain conditions are meet.
	 * These include:
	 * 1. SLV_EVENT_STATUS[HJ_EN] = 1 (or a controller issues Enables HJ events with
	 * the CCC ENEC, This can be set to 0 with CCC DISEC from a controller)
	 * 2. The Dynamic address is invalid. (not assigned yet)
	 * 3. Bus Idle condition is met (1ms) as programmed in the Bus Timing Register
	 */

	/* enable HJ */
	sys_write32(sys_read32(config->regs + SLV_EVENT_STATUS) | SLV_EVENT_STATUS_HJ_EN,
		    config->regs + SLV_EVENT_STATUS);

	ret = k_sem_take(&data->sem_hj, K_MSEC(CONFIG_I3C_DW_RW_TIMEOUT_MS));
	if (ret) {
		return ret;
	}

	return 0;
}

static int dw_i3c_target_ibi_raise_tir(const struct device *dev, struct i3c_ibi *request)
{
	const struct dw_i3c_config *config = dev->config;
	struct dw_i3c_data *data = dev->data;
	int status;
	uint32_t slv_intr_req, slv_ibi_resp;

	if (!(sys_read32(config->regs + HW_CAPABILITY) & HW_CAPABILITY_SLV_IBI_CAP)) {
		LOG_ERR("%s: IBI TIR not supported", dev->name);
		return -ENOTSUP;
	}

	if (!(sys_read32(config->regs + DEVICE_ADDR) & DEVICE_ADDR_DYNAMIC_ADDR_VALID)) {
		LOG_ERR("%s: IBI TIR not available, DA not assigned", dev->name);
		return -EACCES;
	}

	if (!(sys_read32(config->regs + SLV_EVENT_STATUS) & SLV_EVENT_STATUS_SIR_EN)) {
		LOG_ERR("%s: IBI TIR requests are currently disabled by DISEC", dev->name);
		return -EAGAIN;
	}

	slv_intr_req = sys_read32(config->regs + SLV_INTR_REQ);
	if (sys_read32(config->regs + SLV_CHAR_CTRL) & SLV_CHAR_CTRL_IBI_PAYLOAD) {
		uint32_t tir_data = 0;

		/* max support length is DA + MDB (1 byte) + 4 data bytes, MDB must be at least
		 * included
		 */
		if ((request->payload_len > 5) || (request->payload_len == 0)) {
			return -EINVAL;
		}

		/* MDB should be the first byte of the payload */
		slv_intr_req |= SLV_INTR_REQ_MDB(request->payload[0]) |
				SLV_INTR_REQ_SIR_DATA_LENGTH(request->payload_len - 1);

		/* program the tir data packet */
		tir_data |=
			SLV_SIR_DATA_BYTE0((request->payload_len > 1) ? request->payload[1] : 0);
		tir_data |=
			SLV_SIR_DATA_BYTE1((request->payload_len > 2) ? request->payload[2] : 0);
		tir_data |=
			SLV_SIR_DATA_BYTE2((request->payload_len > 3) ? request->payload[3] : 0);
		tir_data |=
			SLV_SIR_DATA_BYTE3((request->payload_len > 4) ? request->payload[4] : 0);
		sys_write32(tir_data, config->regs + SLV_SIR_DATA);
	}

	/* kick off the ibi tir request */
	slv_intr_req |= SLV_INTR_REQ_SIR;
	sys_write32(slv_intr_req, config->regs + SLV_INTR_REQ);

	/* wait for SLV_IBI_RESP update */
	status = k_sem_take(&data->ibi_sts_sem, K_MSEC(100));
	if (status != 0) {
		return -ETIMEDOUT;
	}

	slv_ibi_resp = sys_read32(config->regs + SLV_IBI_RESP);
	switch (SLV_IBI_RESP_IBI_STS(slv_ibi_resp)) {
	case SLV_IBI_RESP_IBI_STS_ACK:
		LOG_DBG("%s: Controller ACKed IBI TIR", dev->name);
		return 0;
	case SLV_IBI_RESP_IBI_STS_NACK:
		LOG_ERR("%s: Controller NACKed IBI TIR", dev->name);
		return -EAGAIN;
	case SLV_IBI_RESP_IBI_STS_EARLY_TERMINATE:
		LOG_ERR("%s: Controller aborted IBI TIR with %lu remaining", dev->name,
			SLV_IBI_RESP_DATA_LENGTH(slv_ibi_resp));
		return -EIO;
	default:
		return -EIO;
	}
}

static int dw_i3c_target_ibi_raise_mr(const struct device *dev)
{
	const struct dw_i3c_config *config = dev->config;
	struct dw_i3c_data *data = dev->data;
	int ret;

	if (!(sys_read32(config->regs + HW_CAPABILITY) & HW_CAPABILITY_SLV_IBI_CAP)) {
		LOG_ERR("%s: IBI not supported by HW", dev->name);
		return -ENOTSUP;
	}

	if (!(sys_read32(config->regs + DEVICE_ADDR) & DEVICE_ADDR_DYNAMIC_ADDR_VALID)) {
		LOG_ERR("%s: MR not available, DA not assigned", dev->name);
		return -EACCES;
	}

	if (!(sys_read32(config->regs + SLV_EVENT_STATUS) & SLV_EVENT_STATUS_MR_EN)) {
		LOG_ERR("%s: MR requests are currently disabled by DISEC", dev->name);
		return -EAGAIN;
	}

	sys_write32(sys_read32(config->regs + SLV_INTR_REQ) | SLV_INTR_REQ_MR,
		    config->regs + SLV_INTR_REQ);

	ret = k_sem_take(&data->ibi_sts_sem, K_MSEC(CONFIG_I3C_DW_RW_TIMEOUT_MS));
	if (ret) {
		return -ETIMEDOUT;
	}

	uint32_t slv_ibi_resp = sys_read32(config->regs + SLV_IBI_RESP);

	switch (SLV_IBI_RESP_IBI_STS(slv_ibi_resp)) {
	case SLV_IBI_RESP_IBI_STS_ACK:
		LOG_DBG("%s: Controller ACKed MR", dev->name);
		return 0;
	case SLV_IBI_RESP_IBI_STS_NACK:
		LOG_ERR("%s: Controller NACKed MR", dev->name);
		return -EAGAIN;
	default:
		return -EIO;
	}
}

static int dw_i3c_target_ibi_raise(const struct device *dev, struct i3c_ibi *request)
{
	if (request == NULL) {
		return -EINVAL;
	}

	switch (request->ibi_type) {
	case I3C_IBI_TARGET_INTR:
		return dw_i3c_target_ibi_raise_tir(dev, request);
	case I3C_IBI_CONTROLLER_ROLE_REQUEST:
		return dw_i3c_target_ibi_raise_mr(dev);
	case I3C_IBI_HOTJOIN:
		return dw_i3c_target_ibi_raise_hj(dev);
	default:
		return -EINVAL;
	}
}
#endif /* CONFIG_I3C_TARGET */

#if defined(CONFIG_I3C_CONTROLLER) && defined(CONFIG_I3C_TARGET)
static void dw_i3c_role_switch_resume(const struct device *dev)
{
	const struct dw_i3c_config *config = dev->config;

	sys_write32(RESET_CTRL_RX_FIFO | RESET_CTRL_TX_FIFO | RESET_CTRL_CMD_QUEUE,
		    config->regs + RESET_CTRL);
	sys_write32(sys_read32(config->regs + DEVICE_CTRL) | DEV_CTRL_RESUME,
		    config->regs + DEVICE_CTRL);
}

static void dw_i3c_update_interrupt_mask(const struct device *dev)
{
	const struct dw_i3c_config *config = dev->config;
	uint32_t intr_mask;

	if (dw_i3c_is_current_controller(dev)) {
		intr_mask = INTR_MASTER_MASK | INTR_BUSOWNER_UPDATE_STAT;
	} else {
		intr_mask = INTR_SLAVE_MASK | INTR_BUSOWNER_UPDATE_STAT;
	}

	sys_write32(INTR_ALL, config->regs + INTR_STATUS);
	sys_write32(intr_mask, config->regs + INTR_STATUS_EN);
	sys_write32(intr_mask, config->regs + INTR_SIGNAL_EN);
}
#endif /* CONFIG_I3C_CONTROLLER && CONFIG_I3C_TARGET */
#endif /* CONFIG_I3C_USE_IBI */

static int i3c_dw_irq(const struct device *dev)
{
	const struct dw_i3c_config *config = dev->config;
	uint32_t status;
#ifdef CONFIG_I3C_TARGET
	struct dw_i3c_data *data = dev->data;
#endif /* CONFIG_I3C_TARGET */

	status = sys_read32(config->regs + INTR_STATUS);
	if (status & (INTR_TRANSFER_ERR_STAT | INTR_RESP_READY_STAT)) {
		dw_i3c_end_xfer(dev);

		if (status & INTR_TRANSFER_ERR_STAT) {
			sys_write32(INTR_TRANSFER_ERR_STAT, config->regs + INTR_STATUS);
		}
	}
#ifdef CONFIG_I3C_CONTROLLER
	if (status & INTR_IBI_THLD_STAT) {
#ifdef CONFIG_I3C_USE_IBI
		ibis_handle(dev);
#endif /* CONFIG_I3C_USE_IBI */
	}
#endif /* CONFIG_I3C_CONTROLLER */
#ifdef CONFIG_I3C_TARGET
	/* target mode related interrupts */
	if (!dw_i3c_is_current_controller(dev)) {
		const struct i3c_target_callbacks *target_cb =
			data->target_config ? data->target_config->callbacks : NULL;

		/* Read Requested when the CMDQ is empty*/
		if (status & INTR_READ_REQ_RECV_STAT) {
			if (target_cb != NULL && target_cb->read_requested_cb != NULL) {
				/* Inform app so that it can send data. */
				target_cb->read_requested_cb(data->target_config, NULL);
			}
			sys_write32(INTR_READ_REQ_RECV_STAT, config->regs + INTR_STATUS);
		}
		/* CCC updated a target register: ack event flags and RESUME to
		 * clear SLAVE_BUSY (databook section 6.1.24).  Only RESUME for
		 * MRL/MWL updates; unconditional RESUME here was empirically
		 * observed to NACK subsequent GETSTATUS.
		 */
		if (status & INTR_CCC_UPDATED_STAT) {
			uint32_t ev = sys_read32(config->regs + TGT_EVENT_STATUS);
			bool need_resume_ack = false;

			if (ev & (TGT_EVENT_STATUS_MRL_UPDATED | TGT_EVENT_STATUS_MWL_UPDATED)) {
				sys_write32(ev & (TGT_EVENT_STATUS_MRL_UPDATED |
						  TGT_EVENT_STATUS_MWL_UPDATED),
					    config->regs + TGT_EVENT_STATUS);
				need_resume_ack = true;
			}

			/* RESUME only for MRL/MWL ack (see above). */
			if (need_resume_ack) {
				sys_write32(sys_read32(config->regs + DEVICE_CTRL) |
						    DEV_CTRL_RESUME,
					    config->regs + DEVICE_CTRL);
			}
			sys_write32(INTR_CCC_UPDATED_STAT, config->regs + INTR_STATUS);

			uint32_t da_after = sys_read32(config->regs + DEVICE_ADDR);
			bool da_valid_now = (da_after & DEVICE_ADDR_DYNAMIC_ADDR_VALID) != 0U;

			/* Detect RSTDAA: DA-valid transitions true->false.
			 * Some variants then ignore the next ENTDAA until
			 * the platform re-arm hook kicks the bus detector.
			 */
			if (data->target_da_valid_last && !da_valid_now) {
				if (DW_I3C_OPS(config) && config->ops->re_arm_target) {
					config->ops->re_arm_target(dev);
				}
			}
			data->target_da_valid_last = da_valid_now;
		}
#ifdef CONFIG_I3C_USE_IBI
		/* IBI TIR request register is addressed and status is updated*/
		if (status & INTR_IBI_UPDATED_STAT) {
			k_sem_give(&data->ibi_sts_sem);
			sys_write32(INTR_IBI_UPDATED_STAT, config->regs + INTR_STATUS);
		}
#endif /* CONFIG_I3C_USE_IBI */
		/* DA has been assigned, could happen after a IBI HJ request */
		if (status & INTR_DYN_ADDR_ASSGN_STAT) {
			data->target_da_valid_last = true;
#ifdef CONFIG_I3C_USE_IBI
			k_sem_give(&data->sem_hj);
#endif /* CONFIG_I3C_USE_IBI */
			sys_write32(INTR_DYN_ADDR_ASSGN_STAT, config->regs + INTR_STATUS);
		}
	}
#endif /* CONFIG_I3C_TARGET */

#if defined(CONFIG_I3C_CONTROLLER) && defined(CONFIG_I3C_TARGET) && defined(CONFIG_I3C_USE_IBI)
	if (status & INTR_BUSOWNER_UPDATE_STAT) {
		dw_i3c_role_switch_resume(dev);
		dw_i3c_update_interrupt_mask(dev);

		if (dw_i3c_is_current_controller(dev)) {
			i3c_ibi_work_enqueue_cb(dev, i3c_sec_handoffed);
			if (data->target_config != NULL &&
			    data->target_config->callbacks != NULL &&
			    data->target_config->callbacks->controller_handoff_cb != NULL) {
				data->target_config->callbacks->controller_handoff_cb(
					data->target_config);
			}
		}

		sys_write32(INTR_BUSOWNER_UPDATE_STAT, config->regs + INTR_STATUS);
	}
#endif /* CONFIG_I3C_CONTROLLER && CONFIG_I3C_TARGET && CONFIG_I3C_USE_IBI */

#if DT_HAS_COMPAT_STATUS_OKAY(microchip_xec_i3c)
	if (config->is_mchp) {
		/* The XEC aggregator latches the peripheral interrupt; clear it after the
		 * IP-level status bits have been handled above.
		 */
		soc_ecia_girq_status_clear(config->girq_id, config->girq_pos);
	}
#endif

	return 0;
}

#ifdef CONFIG_I3C_CONTROLLER
/**
 * @brief Return true if any i2c device only supports fast mode
 *
 * @param dev_list Pointer to device list
 *
 * @retval true if any i2c device only supports fast mode
 * @retval false if all devices support fast mode plus
 */
static bool i3c_any_i2c_fast_mode(const struct i3c_dev_list *dev_list)
{
	for (int i = 0; i < dev_list->num_i2c; i++) {
		if (I3C_LVR_I2C_MODE(dev_list->i2c[i].lvr) == I3C_LVR_I2C_FM_MODE) {
			return true;
		}
	}
	return false;
}
#endif

static int dw_i3c_init_scl_timing(const struct device *dev, struct i3c_config_controller *ctrl_cfg)
{
	const struct dw_i3c_config *config = dev->config;
	struct dw_i3c_data *data = dev->data;
	uint32_t core_rate, scl_timing;
#ifdef CONFIG_I3C_CONTROLLER
	uint32_t hcnt, lcnt, fmlcnt, fmplcnt, free_cnt, i2c_scl_hz, tlow_min_ns;
#endif /* CONFIG_I3C_CONTROLLER */

	if ((ctrl_cfg != NULL) && (ctrl_cfg->scl.i2c > I3C_BUS_I2C_FM_PLUS_SCL_RATE)) {
		return -EINVAL;
	}

	if ((ctrl_cfg != NULL) && (ctrl_cfg->scl.i3c == 0U)) {
		return -EINVAL;
	}

	if (dw_i3c_get_core_rate(dev, &core_rate) != 0) {
		LOG_ERR("%s: get clock rate failed", dev->name);
		return -EINVAL;
	}

#ifdef CONFIG_I3C_CONTROLLER

	__ASSERT((ctrl_cfg != NULL), "Controller configuration should not be NULL");

	if (ctrl_cfg->scl_od_min.low_ns < I3C_OD_TLOW_MIN_NS) {
		LOG_ERR("%s: Open Drain Low Period is out of range", dev->name);
		return -EINVAL;
	}

	/* I3C_OD */
	hcnt = DIV_ROUND_UP(ctrl_cfg->scl_od_min.high_ns * (uint64_t)core_rate, I3C_PERIOD_NS) - 1;
	hcnt = CLAMP(hcnt, SCL_I3C_TIMING_CNT_MIN, SCL_I3C_TIMING_CNT_MAX);

	lcnt = DIV_ROUND_UP(ctrl_cfg->scl_od_min.low_ns * (uint64_t)core_rate, I3C_PERIOD_NS);
	lcnt = CLAMP(lcnt, SCL_I3C_TIMING_CNT_MIN, SCL_I3C_TIMING_CNT_MAX);

	scl_timing = SCL_I3C_TIMING_HCNT(hcnt) | SCL_I3C_TIMING_LCNT(lcnt);
	sys_write32(scl_timing, config->regs + SCL_I3C_OD_TIMING);

	/* I3C_PP */
	hcnt = DIV_ROUND_UP(I3C_BUS_THIGH_MAX_NS * (uint64_t)core_rate, I3C_PERIOD_NS) - 1;
	hcnt = CLAMP(hcnt, SCL_I3C_TIMING_CNT_MIN, SCL_I3C_TIMING_CNT_MAX);

	lcnt = DIV_ROUND_UP(core_rate, ctrl_cfg->scl.i3c) - hcnt;
	lcnt = CLAMP(lcnt, SCL_I3C_TIMING_CNT_MIN, SCL_I3C_TIMING_CNT_MAX);

	scl_timing = SCL_I3C_TIMING_HCNT(hcnt) | SCL_I3C_TIMING_LCNT(lcnt);
	sys_write32(scl_timing, config->regs + SCL_I3C_PP_TIMING);

	/* I3C */
	lcnt = DIV_ROUND_UP(core_rate, I3C_BUS_SDR1_SCL_RATE) - hcnt;
	scl_timing = SCL_EXT_LCNT_1(lcnt);
	lcnt = DIV_ROUND_UP(core_rate, I3C_BUS_SDR2_SCL_RATE) - hcnt;
	scl_timing |= SCL_EXT_LCNT_2(lcnt);
	lcnt = DIV_ROUND_UP(core_rate, I3C_BUS_SDR3_SCL_RATE) - hcnt;
	scl_timing |= SCL_EXT_LCNT_3(lcnt);
	lcnt = DIV_ROUND_UP(core_rate, I3C_BUS_SDR4_SCL_RATE) - hcnt;
	scl_timing |= SCL_EXT_LCNT_4(lcnt);
	sys_write32(scl_timing, config->regs + SCL_EXT_LCNT_TIMING);

	/* I2C FM+ */
	fmplcnt = DIV_ROUND_UP(I3C_BUS_I2C_FMP_TLOW_MIN_NS * (uint64_t)core_rate, I3C_PERIOD_NS);
	hcnt = DIV_ROUND_UP(core_rate, I3C_BUS_I2C_FM_PLUS_SCL_RATE) - fmplcnt;
	scl_timing = SCL_I2C_FMP_TIMING_HCNT(hcnt) | SCL_I2C_FMP_TIMING_LCNT(fmplcnt);
	sys_write32(scl_timing, config->regs + SCL_I2C_FMP_TIMING);

	/* I2C FM */
	i2c_scl_hz = ctrl_cfg->scl.i2c;
	if (i2c_scl_hz == 0) {
		/* Not set in devicetree: derive from the LVRs of the attached I2C devices. */
		i2c_scl_hz = i3c_any_i2c_fast_mode(&config->common.dev_list)
				     ? I3C_BUS_I2C_FM_SCL_RATE
				     : I3C_BUS_I2C_FM_PLUS_SCL_RATE;
	}
	i2c_scl_hz = MIN(i2c_scl_hz, I3C_BUS_I2C_FM_SCL_RATE);

	if (i2c_scl_hz <= I3C_BUS_I2C_SM_SCL_RATE) {
		tlow_min_ns = I3C_BUS_I2C_SM_TLOW_MIN_NS;
	} else {
		tlow_min_ns = I3C_BUS_I2C_FM_TLOW_MIN_NS;
	}

	fmlcnt = DIV_ROUND_UP(tlow_min_ns * (uint64_t)core_rate, I3C_PERIOD_NS);
	fmlcnt = MIN(fmlcnt, SCL_I2C_FM_TIMING_CNT_MAX);
	hcnt = MIN(DIV_ROUND_UP(core_rate, i2c_scl_hz) - fmlcnt, SCL_I2C_FM_TIMING_CNT_MAX);
	scl_timing = SCL_I2C_FM_TIMING_HCNT(hcnt) | SCL_I2C_FM_TIMING_LCNT(fmlcnt);
	sys_write32(scl_timing, config->regs + SCL_I2C_FM_TIMING);

	if (data->mode != I3C_BUS_MODE_PURE) {
		/*
		 * Mixed bus: Set bus free timing to match tLOW of I2C timing. If any i2c devices
		 * only support fast mode, then it to the tLOW of that, otherwise set to the tLOW
		 * of fast mode plus.
		 */
		sys_write32(BUS_I3C_MST_FREE(i3c_any_i2c_fast_mode(&config->common.dev_list)
						     ? fmlcnt
						     : fmplcnt),
			    config->regs + BUS_FREE_TIMING);
		sys_write32(sys_read32(config->regs + DEVICE_CTRL) | DEV_CTRL_I2C_SLAVE_PRESENT,
			    config->regs + DEVICE_CTRL);
	} else {
		/* Pure bus: Set bus free timing to t_cas of 38.4ns */
		free_cnt = DIV_ROUND_UP(I3C_BUS_TCAS_PS * (uint64_t)core_rate, I3C_PERIOD_PS);
		sys_write32(BUS_I3C_MST_FREE(free_cnt), config->regs + BUS_FREE_TIMING);
		sys_write32(sys_read32(config->regs + DEVICE_CTRL) & ~DEV_CTRL_I2C_SLAVE_PRESENT,
			    config->regs + DEVICE_CTRL);
	}
#endif /* CONFIG_I3C_CONTROLLER */
#ifdef CONFIG_I3C_TARGET
	/* Target bus timing (0xd4[31:16], 0xd8) exists for 1 < IC_DEVICE_ROLE < 5. */
	if (DW_HAS_BUS_IDLE(data->role)) {
		/* I3C Bus Available Time */
		scl_timing = DIV_ROUND_UP(I3C_BUS_AVAILABLE_TIME_NS * (uint64_t)core_rate,
					  I3C_PERIOD_NS);
		sys_write32(BUS_I3C_AVAIL_TIME(scl_timing), config->regs + BUS_FREE_TIMING);

		/* I3C Bus Idle Time */
		scl_timing =
			DIV_ROUND_UP(I3C_BUS_IDLE_TIME_NS * (uint64_t)core_rate, I3C_PERIOD_NS);
		sys_write32(BUS_I3C_IDLE_TIME(scl_timing), config->regs + BUS_IDLE_TIMING);
	}
#endif /* CONFIG_I3C_TARGET */

	return 0;
}

#ifdef CONFIG_I3C_CONTROLLER
static int dw_i3c_attach_device(const struct device *dev, struct i3c_device_desc *desc)
{
	const struct dw_i3c_config *config = dev->config;
	struct dw_i3c_data *data = dev->data;
	int pos = get_free_pos(data->free_pos);
	uint32_t dat = 0U;

	if (pos < 0) {
		LOG_ERR("%s: no space for i3c device: %s", dev->name, desc->dev->name);
		return -ENOSPC;
	}

	data->dw_i3c_i2c_priv_data[pos].id = pos;
	desc->controller_priv = &(data->dw_i3c_i2c_priv_data[pos]);
	data->free_pos &= ~BIT(pos);

	LOG_DBG("%s: Attaching %s", dev->name, desc->dev->name);

	if (desc->dynamic_addr != 0U) {
		dat |= DEV_ADDR_TABLE_DYNAMIC_ADDR(desc->dynamic_addr);
	}

	if (desc->static_addr != 0U) {
		dat |= DEV_ADDR_TABLE_STATIC_ADDR(desc->static_addr);
	}
	dat |= DEV_ADDR_TABLE_SIR_REJECT;

	sys_write32(dat, config->regs + DEV_ADDR_TABLE_LOC(data->datstartaddr, pos));

	return 0;
}

static int dw_i3c_reattach_device(const struct device *dev, struct i3c_device_desc *desc,
				  uint8_t old_dyn_addr)
{
	ARG_UNUSED(old_dyn_addr);

	const struct dw_i3c_config *config = dev->config;
	struct dw_i3c_data *data = dev->data;
	struct dw_i3c_i2c_dev_data *dw_i3c_device_data = desc->controller_priv;
	uint32_t dat;

	if (dw_i3c_device_data == NULL) {
		LOG_ERR("%s: %s: device not attached", dev->name, desc->dev->name);
		return -EINVAL;
	}
	/* TODO: investigate clearing table beforehand */

	LOG_DBG("Reattaching %s", desc->dev->name);

	dat = sys_read32(config->regs +
			 DEV_ADDR_TABLE_LOC(data->datstartaddr, dw_i3c_device_data->id));
	dat &= ~DEV_ADDR_TABLE_DYNAMIC_ADDR_MASK;
	sys_write32(DEV_ADDR_TABLE_DYNAMIC_ADDR(desc->dynamic_addr) | dat,
		    config->regs + DEV_ADDR_TABLE_LOC(data->datstartaddr, dw_i3c_device_data->id));

	return 0;
}

static int dw_i3c_detach_device(const struct device *dev, struct i3c_device_desc *desc)
{
	const struct dw_i3c_config *config = dev->config;
	struct dw_i3c_data *data = dev->data;
	struct dw_i3c_i2c_dev_data *dw_i3c_device_data = desc->controller_priv;

	if (dw_i3c_device_data == NULL) {
		return -EALREADY;
	}

	LOG_DBG("%s: Detaching %s", dev->name, desc->dev->name);

	sys_write32(0,
		    config->regs + DEV_ADDR_TABLE_LOC(data->datstartaddr, dw_i3c_device_data->id));
	data->free_pos |= BIT(dw_i3c_device_data->id);
	desc->controller_priv = NULL;

	return 0;
}

static int set_controller_info(const struct device *dev)
{
	const struct dw_i3c_config *config = dev->config;
	struct dw_i3c_data *data = dev->data;
	uint8_t controller_da;

	if (config->common.primary_controller_da) {
		if (!i3c_addr_slots_is_free(&data->common.attached_dev.addr_slots,
					    config->common.primary_controller_da)) {
			controller_da = i3c_addr_slots_next_free_find(
				&data->common.attached_dev.addr_slots, 0);
			LOG_WRN("%s: 0x%02x DA selected for controller as 0x%02x is unavailable",
				dev->name, controller_da, config->common.primary_controller_da);
		} else {
			controller_da = config->common.primary_controller_da;
		}
	} else {
		controller_da =
			i3c_addr_slots_next_free_find(&data->common.attached_dev.addr_slots, 0);
	}

	sys_write32(DEVICE_ADDR_DYNAMIC_ADDR_VALID | DEVICE_ADDR_DYNAMIC(controller_da),
		    config->regs + DEVICE_ADDR);
	/* Mark the address as I3C device */
	i3c_addr_slots_mark_i3c(&data->common.attached_dev.addr_slots, controller_da);

	return 0;
}
#endif /* CONFIG_I3C_CONTROLLER */

static void enable_interrupts(const struct device *dev)
{
	const struct dw_i3c_config *config = dev->config;
	uint32_t thld_ctrl, intr_mask;

	config->irq_config_func();

	thld_ctrl = sys_read32(config->regs + QUEUE_THLD_CTRL);
	thld_ctrl &= (~QUEUE_THLD_CTRL_RESP_BUF_MASK & ~QUEUE_THLD_CTRL_IBI_STS_MASK &
		      ~QUEUE_THLD_CTRL_IBI_DATA_MASK);
	thld_ctrl |= QUEUE_THLD_CTRL_IBI_DATA(QUEUE_THLD_CTRL_IBI_DATA_DEFAULT);
	sys_write32(thld_ctrl, config->regs + QUEUE_THLD_CTRL);

	thld_ctrl = sys_read32(config->regs + DATA_BUFFER_THLD_CTRL);
	thld_ctrl &= ~DATA_BUFFER_THLD_CTRL_RX_BUF;
	sys_write32(thld_ctrl, config->regs + DATA_BUFFER_THLD_CTRL);

	sys_write32(INTR_ALL, config->regs + INTR_STATUS);

	/* Enable interrupts */
#if defined(CONFIG_I3C_CONTROLLER) && defined(CONFIG_I3C_TARGET)
	intr_mask = INTR_MASTER_MASK | INTR_SLAVE_MASK;
#if defined(CONFIG_I3C_USE_IBI)
	intr_mask |= INTR_BUSOWNER_UPDATE_STAT;
#endif /* CONFIG_I3C_USE_IBI */
#elif defined(CONFIG_I3C_CONTROLLER)
	intr_mask = INTR_MASTER_MASK;
#elif defined(CONFIG_I3C_TARGET)
	intr_mask = INTR_SLAVE_MASK;
#endif
	sys_write32(intr_mask, config->regs + INTR_STATUS_EN);
	sys_write32(intr_mask, config->regs + INTR_SIGNAL_EN);
}
#ifdef CONFIG_I3C_CONTROLLER
/**
 * @brief Calculate the odd parity of a byte.
 *
 * This function calculates the odd parity of the input byte, returning 1 if the
 * number of set bits is odd and 0 otherwise.
 *
 * @param p The byte for which odd parity is to be calculated.
 *
 * @return The odd parity result (1 if odd, 0 if even).
 */
static uint8_t odd_parity(uint8_t p)
{
	p ^= p >> 4;
	p &= 0xf;
	return (0x9669 >> p) & 1;
}

/**
 * @brief Send Common Command Code (CCC).
 *
 * @see i3c_do_ccc
 *
 * @param dev Pointer to controller device driver instance.
 * @param payload Pointer to CCC payload.
 *
 * @return @see i3c_do_ccc
 */
static int dw_i3c_do_ccc(const struct device *dev, struct i3c_ccc_payload *payload)
{
	const struct dw_i3c_config *config = dev->config;
	struct dw_i3c_data *data = dev->data;
	struct dw_i3c_xfer *xfer = &data->xfer;
	struct dw_i3c_cmd *cmd;
	enum dw_i3c_retry_action retry_action;
	uint32_t pstate;
	uint32_t settle_us;
	int64_t timeout_ms;
	int64_t deadline;
	bool first_cmd_error_none;
	bool retried_after_recover = false;
	bool is_setdasa_direct = !i3c_ccc_is_payload_broadcast(payload) &&
		(payload->ccc.id == I3C_CCC_SETDASA);
	bool is_enec_broadcast = i3c_ccc_is_payload_broadcast(payload) &&
		(payload->ccc.id == I3C_CCC_ENEC(true));
	int ret, i, pos;

	if (!dw_i3c_is_current_controller(dev)) {
		return -EACCES;
	}

	ret = k_mutex_lock(&data->mt, K_MSEC(1000));
	if (ret) {
		LOG_DBG("%s: Mutex err (%d)", dev->name, ret);
		return ret;
	}

	/* Under the mutex: a concurrent transfer cannot halt the controller
	 * between this check and the submission below.
	 */
	ret = dw_i3c_ensure_xfer_ready(dev);
	if (ret != 0) {
		k_mutex_unlock(&data->mt);
		return ret;
	}

	timeout_ms = CONFIG_I3C_DW_RW_TIMEOUT_MS;

	pm_device_busy_set(dev);

	memset(xfer, 0, sizeof(struct dw_i3c_xfer));
	xfer->ret = -1;

	/* in the case of multiple targets in a CCC, each command queue must have the same CCC ID
	 * loaded along with different dev index fields pointing to the targets
	 */
	if (i3c_ccc_is_payload_broadcast(payload)) {
		xfer->ncmds = 1;
		cmd = &xfer->cmds[0];
		cmd->buf = payload->ccc.data;

		cmd->cmd_hi = COMMAND_PORT_ARG_DATA_LEN(payload->ccc.data_len) |
			      COMMAND_PORT_TRANSFER_ARG;
		cmd->cmd_lo = COMMAND_PORT_CP | COMMAND_PORT_TOC | COMMAND_PORT_ROC |
			      COMMAND_PORT_CMD(payload->ccc.id);

		if ((payload->targets.payloads) && (payload->targets.payloads[0].rnw)) {
			cmd->cmd_lo |= COMMAND_PORT_READ_TRANSFER;
			cmd->rx_len = payload->ccc.data_len;
		} else {
			cmd->tx_len = payload->ccc.data_len;
		}
	} else {
		if (!(payload->targets.payloads)) {
			LOG_ERR("%s: Direct CCC Payload structure Empty", dev->name);
			ret = -EINVAL;
			goto error;
		}
		xfer->ncmds = payload->targets.num_targets;
		for (i = 0; i < payload->targets.num_targets; i++) {
			cmd = &xfer->cmds[i];
			/* Look up position, SETDASA will perform the look up by static addr */
			pos = get_i3c_addr_pos(dev, payload->targets.payloads[i].addr,
					       payload->ccc.id == I3C_CCC_SETDASA);
			if (pos < 0) {
				LOG_ERR("%s: Invalid Slave address with pos %d", dev->name, pos);
				ret = -ENOSPC;
				goto error;
			}
			cmd->buf = payload->targets.payloads[i].data;

			cmd->cmd_hi =
				COMMAND_PORT_ARG_DATA_LEN(payload->targets.payloads[i].data_len) |
				COMMAND_PORT_TRANSFER_ARG;
			cmd->cmd_lo = COMMAND_PORT_CP | COMMAND_PORT_DEV_INDEX(pos) |
				      COMMAND_PORT_ROC | COMMAND_PORT_CMD(payload->ccc.id);
			/* last command queue with multiple targets must have TOC set */
			if (i == (payload->targets.num_targets - 1)) {
				cmd->cmd_lo |= COMMAND_PORT_TOC;
			}
			/* If there is a defining byte for direct CCC */
			if (payload->ccc.data_len == 1) {
				cmd->cmd_lo |= COMMAND_PORT_DBP;
				cmd->cmd_hi |= COMMAND_PORT_ARG_DB(payload->ccc.data[0]);
			} else if (payload->ccc.data_len > 1) {
				LOG_ERR("%s: direct CCCs defining byte >1", dev->name);
				ret = -EINVAL;
				goto error;
			}

			if (payload->targets.payloads[i].rnw) {
				cmd->cmd_lo |= COMMAND_PORT_READ_TRANSFER;
				cmd->rx_len = payload->targets.payloads[i].data_len;
			} else {
				cmd->tx_len = payload->targets.payloads[i].data_len;
			}
		}
	}

	/* Pre-CCC sanity: if the controller is still halted from a previous
	 * error, recover before attempting the next submission.
	 */
	settle_us = DW_I3C_HALT_SETTLE_US;
	do {
		pstate = sys_read32(config->regs + PRESENT_STATE);
		if (PRESENT_STATE_CM_TFR_STS(pstate) != CM_TFR_STS_CTRL_HALT) {
			break;
		}
		k_busy_wait(1);
	} while (--settle_us > 0U);

	if (PRESENT_STATE_CM_TFR_STS(pstate) == CM_TFR_STS_CTRL_HALT) {
		ret = dw_i3c_recover_bus(dev);
		if (ret != 0) {
			goto error;
		}
	}

	while (true) {
		k_sem_reset(&data->sem_xfer);
		start_xfer(dev);
		deadline = k_uptime_get() + timeout_ms;
		ret = -EAGAIN;

		while (k_uptime_get() < deadline) {
			ret = k_sem_take(&data->sem_xfer, K_MSEC(1));
			if (ret == 0) {
				break;
			}
		}

		if (ret == 0) {
			break;
		}

		LOG_ERR("%s: Semaphore err (%d)", dev->name, ret);

		first_cmd_error_none = xfer->ncmds > 0 &&
				       xfer->cmds[0].error == RESPONSE_NO_ERROR;

		retry_action = dw_i3c_ccc_timeout_retry_action(dev, retried_after_recover,
							      first_cmd_error_none,
							      is_setdasa_direct,
							      is_enec_broadcast);

		if (!retried_after_recover && retry_action == DW_I3C_RETRY_FULL_RECOVER) {
			ret = dw_i3c_recover_bus(dev);
			if (ret != 0) {
				goto error;
			}
			ret = dw_i3c_prepare_bus_init(dev);
			if (ret != 0) {
				goto error;
			}
			retried_after_recover = true;
			continue;
		}

		/* Ensure next submission does not inherit a silently halted state. */
		(void)dw_i3c_recover_bus(dev);

		goto error;
	}

	/* the only way data_len would not equal num_xfer would be if an abort happened */
	payload->ccc.num_xfer = payload->ccc.data_len;
	for (i = 0; i < xfer->ncmds; i++) {
		/* if this is a direct ccc, then write back the number of bytes tx or rx */
		if (!i3c_ccc_is_payload_broadcast(payload)) {
			payload->targets.payloads[i].num_xfer = payload->targets.payloads[i].rnw
									? xfer->cmds[i].rx_len
									: xfer->cmds[i].tx_len;
		}
		if (xfer->cmds[i].rx_len && !xfer->cmds[i].error) {
			read_rx_fifo(dev, xfer->cmds[i].buf, xfer->cmds[i].rx_len);
		}
	}

	ret = xfer->ret;

	/* Post-RSTDAA cleanup: all targets dropped their DAs, so clear
	 * controller-side bookkeeping (DAT, addr_slots, free_pos,
	 * controller_priv).  Without this the next ENTDAA allocates new
	 * slots while stale DAT entries persist, causing directed CCCs
	 * to NACK.  Targets with a static_addr keep their DAT slot
	 * reserved for potential SETDASA re-assignment.
	 */
	if (ret == 0 && i3c_ccc_is_payload_broadcast(payload) &&
	    payload->ccc.id == I3C_CCC_RSTDAA) {
		struct i3c_device_desc *desc;

		I3C_BUS_FOR_EACH_I3CDEV(dev, desc) {
			struct dw_i3c_i2c_dev_data *priv = desc->controller_priv;

			if (priv == NULL) {
				continue;
			}

			if (desc->dynamic_addr != 0U) {
				i3c_addr_slots_mark_free(&data->common.attached_dev.addr_slots,
							 desc->dynamic_addr);
				desc->dynamic_addr = 0U;
			}

			if (desc->static_addr == 0U) {
				uint32_t dat =
					DEV_ADDR_TABLE_LOC(data->datstartaddr, priv->id);

				sys_write32(0, config->regs + dat);
				data->free_pos |= BIT(priv->id);
				desc->controller_priv = NULL;
			}
		}
	}

error:
	pm_device_busy_clear(dev);
	k_mutex_unlock(&data->mt);

	return ret;
}

/**
 * @brief Add a slave device from Dynamic Address Assignment (DAA) information.
 *
 * This function adds a slave device to the I3C controller based on the Dynamic
 * Address Assignment (DAA) information at the specified position. It retrieves
 * the dynamic address, PID (Provisional ID), and additional device characteristics
 * from the corresponding tables and associates the device with a registered device
 * descriptor if the PID is known.
 *
 * @param dev Pointer to the I3C device structure.
 * @param pos Position of the device in the DAA and DCT tables.
 *
 * @return 0 on success, or a negative error code on failure.
 */
static int add_slave_from_daa(const struct device *dev, int32_t pos,
			      struct i3c_device_desc **desc_out)
{
	const struct dw_i3c_config *config = dev->config;
	struct dw_i3c_data *data = dev->data;
	uint32_t dat_word;
	uint32_t dct4_word;
	uint32_t dct1_word;
	uint32_t dct2_word;
	uint32_t dct3_word;
	uint32_t dat_patched;
	uint64_t pid;
	uint8_t dyn_addr;
	uint8_t dyn_addr_parity;

	*desc_out = NULL;

	/* The IP records the DA it assigned in DCT.LOC4[7:0] (databook figure 2-14).
	 * Read it from there rather than from the DAT entry the driver programmed.
	 * The controller DCT stride is 16 bytes (LOC1..LOC4).
	 */
	dat_word = sys_read32(config->regs + DEV_ADDR_TABLE_LOC(data->datstartaddr, pos));
	dct4_word = sys_read32(config->regs + DEV_CHAR_TABLE_LOC4(data->dctstartaddr, pos));
	dyn_addr = DEV_CHAR_TABLE_DYNAMIC_ADDR(dct4_word);

	/* retrieve pid */
	dct1_word = sys_read32(config->regs + DEV_CHAR_TABLE_LOC1(data->dctstartaddr, pos));
	pid = ((uint64_t)DEV_CHAR_TABLE_MSB_PID(dct1_word) << 16) +
	      (DEV_CHAR_TABLE_LSB_PID(dct1_word) << 16);
	dct2_word = sys_read32(config->regs + DEV_CHAR_TABLE_LOC2(data->dctstartaddr, pos));
	pid |= DEV_CHAR_TABLE_LSB_PID(dct2_word);

	dct3_word = sys_read32(config->regs + DEV_CHAR_TABLE_LOC3(data->dctstartaddr, pos));
	uint8_t bcr = DEV_CHAR_TABLE_BCR(dct3_word);
	uint8_t dcr = DEV_CHAR_TABLE_DCR(dct3_word);

	/* Keep DAT dynamic address/parity aligned with the HW-assigned DA. */
	dat_patched = dat_word;
	dyn_addr_parity = odd_parity(dyn_addr) << 7;
	dat_patched &= ~DEV_ADDR_TABLE_DYNAMIC_ADDR_MASK;
	dat_patched |= DEV_ADDR_TABLE_DYNAMIC_ADDR(dyn_addr | dyn_addr_parity);
	sys_write32(dat_patched, config->regs + DEV_ADDR_TABLE_LOC(data->datstartaddr, pos));

	/* lookup known pids */
	const struct i3c_device_id i3c_id = I3C_DEVICE_ID(pid);
	struct i3c_device_desc *target = i3c_device_find(dev, &i3c_id);

	if (target != NULL) {
		/* Known device (DT or previously discovered): refresh state. */
		target->dynamic_addr = dyn_addr;
		target->bcr = bcr;
		target->dcr = dcr;

		if (!i3c_is_i3c_device_attached(target)) {
			/* Defensive: DT device should have been attached via
			 * i3c_attach_i3c_device() before DAA ran. Handle the
			 * unexpected case instead of leaving the device absent
			 * from the list (which would cause ENODEV on first transfer).
			 */
			data->dw_i3c_i2c_priv_data[pos].id = pos;
			target->controller_priv = &data->dw_i3c_i2c_priv_data[pos];
			data->free_pos &= ~BIT(pos);

			sys_slist_append(&data->common.attached_dev.devices.i3c,
					 &target->node);
		} else {
			struct dw_i3c_i2c_dev_data *priv = target->controller_priv;

			if (priv->id != pos) {
				/* The DW IP picks DAT slots during ENTDAA in arbitration
				 * order, which is independent of the slot chosen by
				 * dw_i3c_attach_device(). Any DT-declared target that
				 * participates in ENTDAA (i.e., no "assigned-address")
				 * will typically land in a different DAT slot than the
				 * one bound at attach time.
				 *
				 * Re-bind controller_priv to the slot HW actually used
				 * and return the old slot to the free pool, so that
				 * subsequent transfers and attach/detach operations see
				 * a consistent view of hardware.
				 */
				LOG_DBG("%s: rebinding PID 0x%012llx from DAT slot %u to %d "
						"(ENTDAA placed target in different slot)",
						dev->name, pid, priv->id, pos);

				data->free_pos |= BIT(priv->id);
				data->dw_i3c_i2c_priv_data[pos].id = pos;
				target->controller_priv = &data->dw_i3c_i2c_priv_data[pos];
				data->free_pos &= ~BIT(pos);
			}
		}

		LOG_DBG("%s: PID 0x%012llx assigned dynamic address 0x%02x",
			dev->name, pid, dyn_addr);
	} else {
		/* Unknown device (not in DT). Allocate a descriptor so the
		 * driver can track it for address-slot accounting and future
		 * lookups. Zephyr's public I3C API is DT-gated, so user code
		 * cannot address this device — but tracking it here prevents
		 * duplicate allocation on re-DAA (see dw_i3c_device_find()).
		 */
		target = i3c_device_desc_alloc();
		if (target == NULL) {
			LOG_WRN("%s: PID 0x%012llx DA 0x%02x — descriptor pool "
				"exhausted, device untracked",
				dev->name, pid, dyn_addr);
		} else {
			*(const struct device **)&target->bus = dev;
			*(uint64_t *)&target->pid = pid;
			target->dynamic_addr = dyn_addr;
			target->bcr = bcr;
			target->dcr = dcr;

			data->dw_i3c_i2c_priv_data[pos].id = pos;
			target->controller_priv = &data->dw_i3c_i2c_priv_data[pos];
			data->free_pos &= ~BIT(pos);

			sys_slist_append(&data->common.attached_dev.devices.i3c,
					 &target->node);

			LOG_INF("%s: PID 0x%012llx not in DT, given DA 0x%02x",
				dev->name, pid, dyn_addr);
		}
	}
	i3c_addr_slots_mark_i3c(&data->common.attached_dev.addr_slots, dyn_addr);
	*desc_out = target;

	return 0;
}

/**
 * @brief Perform Dynamic Address Assignment.
 *
 * @see i3c_do_daa
 *
 * @param dev Pointer to controller device driver instance.
 *
 * @return @see i3c_do_daa
 */
static int dw_i3c_do_daa(const struct device *dev)
{
	const struct dw_i3c_config *config = dev->config;
	struct dw_i3c_data *data = dev->data;
	struct dw_i3c_xfer *xfer = &data->xfer;
	struct dw_i3c_cmd *cmd;
	enum dw_i3c_retry_action retry_action;
	uint32_t olddevs, newdevs;
	uint8_t p, idx, last_addr = 0;
	int64_t timeout_ms;
	int64_t deadline;
	bool retried_after_recover = false;
	int32_t pos, addr, ret;

	if (!dw_i3c_is_current_controller(dev)) {
		return -EACCES;
	}

	olddevs = ~(data->free_pos);

	/* Prepare DAT before launching DAA. */
	for (pos = 0; pos < data->maxdevs; pos++) {
		if (olddevs & BIT(pos)) {
			continue;
		}

		addr = i3c_addr_slots_next_free_find(&data->common.attached_dev.addr_slots,
						     last_addr + 1);
		if (addr == 0) {
			return -ENOSPC;
		}

		p = odd_parity(addr);
		last_addr = addr;
		addr |= (p << 7);
		sys_write32(DEV_ADDR_TABLE_DYNAMIC_ADDR(addr) | DEV_ADDR_TABLE_SIR_REJECT,
			    config->regs + DEV_ADDR_TABLE_LOC(data->datstartaddr, pos));
	}

	pos = get_free_pos(data->free_pos);
	if (pos < 0) {
		LOG_ERR("%s: find free pos failed", dev->name);
		return -ENOSPC;
	}

	ret = k_mutex_lock(&data->mt, K_MSEC(1000));
	if (ret) {
		LOG_ERR("%s: Mutex err (%d)", dev->name, ret);
		return ret;
	}

	pm_device_busy_set(dev);
	timeout_ms = CONFIG_I3C_DW_RW_TIMEOUT_MS;

	memset(xfer, 0, sizeof(struct dw_i3c_xfer));

	xfer->ncmds = 1;
	xfer->ret = -1;

	cmd = &xfer->cmds[0];
	cmd->cmd_hi = COMMAND_PORT_TRANSFER_ARG;
	cmd->cmd_lo = COMMAND_PORT_TOC | COMMAND_PORT_ROC |
		      COMMAND_PORT_DEV_COUNT(data->maxdevs - pos) | COMMAND_PORT_DEV_INDEX(pos) |
		      COMMAND_PORT_CMD(I3C_CCC_ENTDAA) | COMMAND_PORT_ADDR_ASSGN_CMD;

	while (true) {
		k_sem_reset(&data->sem_xfer);
		start_xfer(dev);
		deadline = k_uptime_get() + timeout_ms;
		ret = -EAGAIN;

		while (k_uptime_get() < deadline) {
			ret = k_sem_take(&data->sem_xfer, K_MSEC(1));
			if (ret == 0) {
				break;
			}
		}

		if (ret == 0) {
			break;
		}

		LOG_ERR("%s: Semaphore err (%d)", dev->name, ret);

		retry_action = dw_i3c_timeout_retry_action(dev, DW_I3C_TIMEOUT_OP_DAA,
							   retried_after_recover, false);

		if (!retried_after_recover && retry_action == DW_I3C_RETRY_FULL_RECOVER) {
			ret = dw_i3c_recover_bus(dev);
			if (ret != 0) {
				break;
			}
			retried_after_recover = true;
			continue;
		}

		/* Final timeout path: sanitize local controller state before exit. */
		(void)dw_i3c_recover_bus_locked_light(dev);

		break;
	}

	pm_device_busy_clear(dev);
	k_mutex_unlock(&data->mt);

	if (ret) {
		return ret;
	}

	if (data->maxdevs == cmd->rx_len) {
		newdevs = 0;
	} else {
		newdevs = GENMASK(data->maxdevs - cmd->rx_len - 1, 0);
	}
	newdevs &= ~olddevs;

	for (pos = find_lsb_set(newdevs); pos <= find_msb_set(newdevs); pos++) {
		idx = pos - 1;
		if (newdevs & BIT(idx)) {
			struct i3c_device_desc *added = NULL;

			add_slave_from_daa(dev, idx, &added);

			/* Silence this target only: DAA also runs at runtime for a
			 * hot join, where a broadcast would clear these events on
			 * every target enabled earlier while the driver still
			 * believes they are enabled. An untracked device (descriptor
			 * pool exhausted) keeps the HW reset defaults.
			 *
			 * HJ is deliberately not cleared: DISEC state outlives the
			 * dynamic address, so disabling it here would stop the device
			 * rejoining after a later RSTDAA.
			 */
			if (added != NULL) {
				struct i3c_ccc_events disec_events = {
					.events = I3C_CCC_EVT_INTR | I3C_CCC_EVT_CR,
				};
				int disec_ret = i3c_ccc_do_events_set(added, false, &disec_events);

				if (disec_ret != 0) {
					LOG_WRN("%s: post-DAA DISEC to 0x%02x failed (%d); "
						"target may assert IBI/MR before it is expected",
						dev->name, added->dynamic_addr, disec_ret);
				}
			}
		}
	}

	return 0;
}
#endif /* CONFIG_I3C_CONTROLLER */

static void dw_i3c_enable_controller(const struct dw_i3c_config *config, bool enable)
{
	uint32_t reg = sys_read32(config->regs + DEVICE_CTRL);

	if (enable) {
		reg |= DEV_CTRL_ENABLE;
	} else {
		reg &= ~DEV_CTRL_ENABLE;
	}

	sys_write32(reg, config->regs + DEVICE_CTRL);
}

/**
 * @brief Get configuration of the I3C hardware.
 *
 * This provides a way to get the current configuration of the I3C hardware.
 *
 * This can return cached config or probed hardware parameters, but it has to
 * be up to date with current configuration.
 *
 * @param[in] dev Pointer to controller device driver instance.
 * @param[in] type Type of configuration parameters being passed
 *                 in @p config.
 * @param[in,out] config Pointer to the configuration parameters.
 *
 * Note that if @p type is @c I3C_CONFIG_CUSTOM, @p config must contain
 * the ID of the parameter to be retrieved.
 *
 * @retval 0 If successful.
 * @retval -EIO General Input/Output errors.
 * @retval -ENOSYS If not implemented.
 */
static int dw_i3c_config_get(const struct device *dev, enum i3c_config_type type, void *config)
{
#ifdef CONFIG_I3C_TARGET
	const struct dw_i3c_config *dev_config = dev->config;
#endif /* CONFIG_I3C_TARGET */
#ifdef CONFIG_I3C_CONTROLLER
	struct dw_i3c_data *data = dev->data;
#endif /* CONFIG_I3C_CONTROLLER */
	int ret = 0;

	if (type == I3C_CONFIG_CONTROLLER) {
#ifdef CONFIG_I3C_CONTROLLER
		(void)memcpy(config, &data->common.ctrl_config, sizeof(data->common.ctrl_config));
#else
		return -ENOTSUP;
#endif /* CONFIG_I3C_CONTROLLER */
	} else if (type == I3C_CONFIG_TARGET) {
#ifdef CONFIG_I3C_TARGET
		struct i3c_config_target *target_config = config;
		uint32_t reg;

		reg = sys_read32(dev_config->regs + SLV_MAX_LEN);
		target_config->max_read_len = SLV_MAX_LEN_MRL(reg);
		target_config->max_write_len = SLV_MAX_LEN_MWL(reg);

		reg = sys_read32(dev_config->regs + DEVICE_ADDR);
		if (reg & DEVICE_ADDR_STATIC_ADDR_VALID) {
			target_config->static_addr = DEVICE_ADDR_STATIC(reg);
		} else {
			target_config->static_addr = 0x00;
		}

		reg = sys_read32(dev_config->regs + SLV_CHAR_CTRL);
		target_config->bcr = SLV_CHAR_CTRL_BCR(reg);
		target_config->dcr = SLV_CHAR_CTRL_DCR(reg);
		target_config->supported_hdr = SLV_CHAR_CTRL_HDR_CAP(reg);

		reg = sys_read32(dev_config->regs + SLV_MIPI_ID_VALUE);
		target_config->pid = ((uint64_t)reg) << 32;
		target_config->pid_random = (bool)!!(reg & SLV_MIPI_ID_VALUE_SLV_PROV_ID_SEL);
		reg = sys_read32(dev_config->regs + SLV_PID_VALUE);
		target_config->pid |= reg;

		target_config->enabled = !dw_i3c_is_current_controller(dev);
#else
		return -ENOTSUP;
#endif /* CONFIG_I3C_TARGET */
	} else {
		return -EINVAL;
	}

	return ret;
}

/**
 * @brief Configure I3C hardware.
 *
 * @param dev Pointer to controller device driver instance.
 * @param type Type of configuration parameters being passed
 *             in @p config.
 * @param config Pointer to the configuration parameters.
 *
 * @retval 0 If successful.
 * @retval -EINVAL If invalid configure parameters.
 * @retval -EIO General Input/Output errors.
 * @retval -ENOSYS If not implemented.
 */
static int dw_i3c_configure(const struct device *dev, enum i3c_config_type type, void *config)
{
#ifdef CONFIG_I3C_CONTROLLER
	struct dw_i3c_data *data = dev->data;
	int ret;
#endif /* CONFIG_I3C_CONTROLLER */
#ifdef CONFIG_I3C_TARGET
	const struct dw_i3c_config *dev_config = dev->config;
#endif /* CONFIG_I3C_TARGET */

	__ASSERT((config != NULL), "Configuration should not be NULL");

	if (type == I3C_CONFIG_CONTROLLER) {
#ifdef CONFIG_I3C_CONTROLLER
		ret = dw_i3c_init_scl_timing(dev, config);
		if (ret != 0) {
			return ret;
		}
		(void)memcpy(&data->common.ctrl_config,
			     config, sizeof(data->common.ctrl_config));
#else
		return -ENOTSUP;
#endif /* CONFIG_I3C_CONTROLLER */
	} else if (type == I3C_CONFIG_TARGET) {
#ifdef CONFIG_I3C_TARGET
		struct i3c_config_target *target_cfg = (struct i3c_config_target *)config;
		struct dw_i3c_data *tgt_data = dev->data;
		uint32_t val;

		/* TODO: some how randomly generate pid */
		if (target_cfg->pid_random) {
			return -EINVAL;
		}

		val = SLV_MAX_LEN_MWL(target_cfg->max_write_len) |
		      (SLV_MAX_LEN_MRL(target_cfg->max_read_len) << 16);
		sys_write32(val, dev_config->regs + SLV_MAX_LEN);

		/* set static address */
		val = sys_read32(dev_config->regs + DEVICE_ADDR);
		/* if static address is set to 0x00, then disable static_addr_en */
		if (target_cfg->static_addr != 0x00) {
			val |= DEVICE_ADDR_STATIC_ADDR_VALID;
		} else {
			val &= ~DEVICE_ADDR_STATIC_ADDR_VALID;
		}
		val &= ~DEVICE_ADDR_STATIC_MASK;
		val |= DEVICE_ADDR_STATIC(target_cfg->static_addr);
		sys_write32(val, dev_config->regs + DEVICE_ADDR);

		val = sys_read32(dev_config->regs + SLV_CHAR_CTRL);
		val &= ~(SLV_CHAR_CTRL_BCR_MASK | SLV_CHAR_CTRL_DCR_MASK);
		/* Bridge identifier, offline capable, ibi_payload, ibi_request_capable can not be
		 * written to in bcr
		 */
		val |= SLV_CHAR_CTRL_BCR(target_cfg->bcr);
		val |= SLV_CHAR_CTRL_DCR(target_cfg->dcr) << 8;
		/* HDR CAPs is not settable */
		sys_write32(val, dev_config->regs + SLV_CHAR_CTRL);

		val = sys_read32(dev_config->regs + SLV_MIPI_ID_VALUE);
		val &= ~(SLV_MIPI_ID_VALUE_SLV_MIPI_MFG_ID_MASK |
			 SLV_MIPI_ID_VALUE_SLV_PROV_ID_SEL);
		val |= (uint32_t)(target_cfg->pid >> 16);
		sys_write32(val, dev_config->regs + SLV_MIPI_ID_VALUE);

		val = (uint32_t)(target_cfg->pid & 0xFFFFFFFF);
		sys_write32(val, dev_config->regs + SLV_PID_VALUE);

		/* TX_START_THLD = 0 (1 byte) so the target ACKs private
		 * reads regardless of staged length
		 */
		val = sys_read32(dev_config->regs + DATA_BUFFER_THLD_CTRL);
		val &= ~(DATA_BUFFER_THLD_CTRL_TX_START_THLD_MASK |
			 DATA_BUFFER_THLD_CTRL_TX_BUF_MASK);
		val |= DATA_BUFFER_THLD_CTRL_TX_START_THLD(0) | DATA_BUFFER_THLD_CTRL_TX_BUF(0);
		sys_write32(val, dev_config->regs + DATA_BUFFER_THLD_CTRL);

		/* Re-enable with ENABLE only (no HOT_JOIN_NACK) and fire
		 * the platform post_enable hook (EXT_CMD workaround).
		 */
		sys_write32(DEV_CTRL_ENABLE, dev_config->regs + DEVICE_CTRL);

		if (DW_I3C_OPS(dev_config) && dev_config->ops->post_enable) {
			dev_config->ops->post_enable(dev, true);
		}

		/* Clear HJ_EN so the target does not Hot-Join before DAA.
		 * SIR_EN/MR_EN (bits 0-1) are read-only - only the
		 * controller can clear them via directed DISEC.
		 */
		sys_write32(sys_read32(dev_config->regs + SLV_EVENT_STATUS) &
				    ~SLV_EVENT_STATUS_HJ_EN,
			    dev_config->regs + SLV_EVENT_STATUS);

		/* Resync the RSTDAA edge detector: this path leaves the DA untouched. */
		val = sys_read32(dev_config->regs + DEVICE_ADDR);
		tgt_data->target_da_valid_last = (val & DEVICE_ADDR_DYNAMIC_ADDR_VALID) != 0U;
#else
		return -ENOTSUP;
#endif /* CONFIG_I3C_TARGET */
	} else {
		return -EINVAL;
	}

	return 0;
}
#ifdef CONFIG_I3C_CONTROLLER
/**
 * @brief Find a registered I3C target device.
 *
 * This returns the I3C device descriptor of the I3C device
 * matching the incoming @p id.
 *
 * @param dev Pointer to controller device driver instance.
 * @param id Pointer to I3C device ID.
 *
 * @return @see i3c_device_find.
 */
static struct i3c_device_desc *dw_i3c_device_find(const struct device *dev,
						  const struct i3c_device_id *id)
{
	const struct dw_i3c_config *config = dev->config;
	struct i3c_device_desc *desc;

	/* First look in the DT-defined static list. */
	desc = i3c_dev_list_find(&config->common.dev_list, id);
	if (desc != NULL) {
		return desc;
	}

	/* Fall back to the runtime attached-device list — covers targets
	 * discovered via DAA that are not declared in the Device Tree.
	 */
	I3C_BUS_FOR_EACH_I3CDEV(dev, desc) {
		if (desc->pid == id->pid) {
			return desc;
		}
	}

	return NULL;
}

/**
 * @brief Hard-reset and re-initialize the DW I3C controller IP.
 *
 * Used when @ref dw_i3c_recover_bus is insufficient (target keeps NACKing
 * even after halt-recovery and FIFO flush).  Issues RESET_CTRL_ALL which
 * clears the IP's internal state machine, then replays the platform
 * post-reset hook plus the same register-restore sequence used by the
 * PM_DEVICE_ACTION_RESUME path: SCL timing, IBI reject masks, hot-join
 * NACK, every attached I3C device's DAT entry, and the controller's own
 * dynamic address.
 *
 * The Device Address Table contents are wiped by SOFT_RST but are
 * reconstructed from @c config->common.dev_list and the descriptor's
 * controller_priv (DAT slot id) so target devices remain addressable
 * without re-issuing ENTDAA.
 *
 * @param dev Pointer to controller device driver instance.
 *
 * @retval 0 on success.
 * @retval -EACCES Controller is not the active controller.
 * @retval -EBUSY Transfer mutex could not be taken.
 * @retval -errno Propagated from a platform hook or from SCL retiming.
 */
static int dw_i3c_full_reset(const struct device *dev)
{
	const struct dw_i3c_config *config = dev->config;
	struct dw_i3c_data *data = dev->data;
	struct i3c_config_controller *ctrl_config = &data->common.ctrl_config;
	const uint32_t flush_mask = RESET_CTRL_RX_FIFO | RESET_CTRL_TX_FIFO |
					RESET_CTRL_RESP_QUEUE | RESET_CTRL_CMD_QUEUE |
					RESET_CTRL_IBI_QUEUE;
	uint32_t saved_device_addr;
	uint32_t saved_sir_reject;
	uint32_t saved_mr_reject;
	uint32_t saved_dev_ctrl;
	uint32_t saved_dat[DW_I3C_MAX_DEVS];
	uint32_t nibis;
	int ret;

	if (!dw_i3c_is_current_controller(dev)) {
		return -EACCES;
	}

	LOG_WRN("%s: full IP reset (pstate 0x%08x)", dev->name,
		sys_read32(config->regs + PRESENT_STATE));

	ret = k_mutex_lock(&data->mt, K_MSEC(1000));
	if (ret) {
		return ret;
	}

	/* Snapshot the controller's own dynamic address; SOFT_RST wipes it. */
	saved_device_addr = sys_read32(config->regs + DEVICE_ADDR);

	/* Recovery has to be transparent, so the IBI policy the application set
	 * through i3c_ibi_enable()/i3c_ibi_hj_response() is carried across the
	 * reset rather than reverted to the init-time reject-everything defaults.
	 */
	saved_sir_reject = sys_read32(config->regs + IBI_SIR_REQ_REJECT);
	saved_mr_reject = sys_read32(config->regs + IBI_MR_REQ_REJECT);
	saved_dev_ctrl = sys_read32(config->regs + DEVICE_CTRL);
	for (int i = 0; i < MIN(data->maxdevs, DW_I3C_MAX_DEVS); i++) {
		saved_dat[i] = sys_read32(config->regs +
					  DEV_ADDR_TABLE_LOC(data->datstartaddr, i));
	}

	/* Drain pending IBIs so they don't surface on the queue post-reset. */
	nibis = QUEUE_STATUS_IBI_STATUS_CNT(sys_read32(config->regs + QUEUE_STATUS_LEVEL));
	while (nibis--) {
		(void)sys_read32(config->regs + IBI_QUEUE_STATUS);
	}

	unsigned int irq_key = irq_lock();

	sys_write32(0U, config->regs + INTR_SIGNAL_EN);
	sys_write32(0xFFFFFFFFU, config->regs + INTR_STATUS);

	/* Hard reset of the entire IP. */
	sys_write32(RESET_CTRL_ALL, config->regs + RESET_CTRL);
	k_busy_wait(DW_I3C_SOFT_RESET_SETTLE_US);
	irq_unlock(irq_key);

	/* Platform post-reset hook (re-opens wrapper gate, polls SOFT_RST
	 * self-clear, restores DEVICE_CTRL_EXTENDED).
	 */
	ret = dw_i3c_post_reset(dev);
	if (ret != 0) {
		k_mutex_unlock(&data->mt);
		return ret;
	}

	/* Restore SCL timing while ENABLE = 0. */
	ret = dw_i3c_init_scl_timing(dev, ctrl_config);
	if (ret != 0) {
		k_mutex_unlock(&data->mt);
		return ret;
	}

	/* Restore IBI reject masks and hot-join NACK as they were before the reset. */
	sys_write32(saved_sir_reject, config->regs + IBI_SIR_REQ_REJECT);
	sys_write32(saved_mr_reject, config->regs + IBI_MR_REQ_REJECT);
	sys_write32((sys_read32(config->regs + DEVICE_CTRL) & ~DEV_CTRL_HOT_JOIN_NACK) |
			    (saved_dev_ctrl & DEV_CTRL_HOT_JOIN_NACK),
		    config->regs + DEVICE_CTRL);

	/* Replay every attached I3C device's DAT entry, as dw_i3c_attach_device() built it. */
	for (int i = 0; i < config->common.dev_list.num_i3c; i++) {
		struct i3c_device_desc *desc = &config->common.dev_list.i3c[i];
		uint32_t dat = 0U;
		uint8_t pos;

		if (desc->controller_priv == NULL) {
			continue;
		}
		pos = ((struct dw_i3c_i2c_dev_data *)desc->controller_priv)->id;

		if (desc->dynamic_addr != 0U) {
			/* DAT[23:16] carries the dynamic address with its odd parity. */
			dat |= DEV_ADDR_TABLE_DYNAMIC_ADDR(
				desc->dynamic_addr | (odd_parity(desc->dynamic_addr) << 7));
		}

		if (desc->static_addr != 0U) {
			dat |= DEV_ADDR_TABLE_STATIC_ADDR(desc->static_addr);
		}

		/* The per-device IBI bits are not derivable from the descriptor,
		 * so carry them over from the pre-reset entry.
		 */
		if (pos < DW_I3C_MAX_DEVS) {
			dat |= saved_dat[pos] & (DEV_ADDR_TABLE_SIR_REJECT |
						 DEV_ADDR_TABLE_MR_REJECT |
						 DEV_ADDR_TABLE_IBI_WITH_DATA);
		} else {
			dat |= DEV_ADDR_TABLE_SIR_REJECT;
		}

		sys_write32(dat, config->regs + DEV_ADDR_TABLE_LOC(data->datstartaddr, pos));
	}

	/* Replay every attached I2C device's DAT entry. */
	for (int i = 0; i < config->common.dev_list.num_i2c; i++) {
		struct i3c_i2c_device_desc *desc = &config->common.dev_list.i2c[i];

		if (desc->controller_priv == NULL) {
			continue;
		}
		uint8_t pos = ((struct dw_i3c_i2c_dev_data *)desc->controller_priv)->id;

		sys_write32(DEV_ADDR_TABLE_LEGACY_I2C_DEV | DEV_ADDR_TABLE_STATIC_ADDR(desc->addr),
			    config->regs + DEV_ADDR_TABLE_LOC(data->datstartaddr, pos));
	}

	/* Restore the controller's own dynamic address. */
	sys_write32(saved_device_addr, config->regs + DEVICE_ADDR);

	/* Strong re-init window after full reset:
	 * disable core, replay vendor resume hook (wrapper/role clock path),
	 * retime while disabled, then re-enable.
	 */
	dw_i3c_enable_controller(config, false);
	k_busy_wait(100);

	if (DW_I3C_OPS(config) && config->ops->pm_resume) {
		ret = config->ops->pm_resume(dev);
		if (ret != 0) {
			LOG_ERR("%s: pm_resume failed during full reset (%d)", dev->name, ret);
			k_mutex_unlock(&data->mt);
			return ret;
		}
	}

	ret = dw_i3c_init_scl_timing(dev, ctrl_config);
	if (ret != 0) {
		k_mutex_unlock(&data->mt);
		return ret;
	}

	dw_i3c_enable_controller(config, true);
	if (DW_I3C_OPS(config) && config->ops->post_enable) {
		config->ops->post_enable(dev, ctrl_config->is_secondary);
	}

	/* Re-prime queue/data paths after reset before re-enabling IRQ signaling.
	 * This is intentionally redundant with RESET_CTRL_ALL to handle wrappered
	 * variants that can retain stale queue/data levels across the first restart.
	 */
	dw_i3c_force_drain_paths(dev);

	sys_write32(INTR_ALL, config->regs + INTR_STATUS);
	sys_write32(flush_mask, config->regs + RESET_CTRL);
	ret = dw_i3c_pre_resume_ctrl(dev);
	if (ret != 0) {
		k_mutex_unlock(&data->mt);
		return ret;
	}

	sys_write32(INTR_TRANSFER_ERR_STAT, config->regs + INTR_STATUS);
	if (IS_ENABLED(CONFIG_I3C_DW_FULL_RESET_SKIP_RESUME_PULSE)) {
		dw_i3c_clear_resume(dev);
	} else {
		(void)dw_i3c_pulse_resume(dev);
	}

	dw_i3c_force_drain_paths(dev);

	/* Not fatal: the next transfer runs ensure_xfer_ready() and recovers again. */
	ret = dw_i3c_wait_ctrl_idle(dev, DW_I3C_CTRL_IDLE_TIMEOUT_US);
	if (ret != 0) {
		LOG_WRN("%s: controller not idle after full reset", dev->name);
	}

	enable_interrupts(dev);

	/* Forget any stale completion that the wedged transfer left behind. */
	k_sem_reset(&data->sem_xfer);

	k_mutex_unlock(&data->mt);
	return 0;
}

/**
 * @brief Drain the response, RX and IBI paths until they report empty
 */
static void dw_i3c_force_drain_paths(const struct device *dev)
{
	const struct dw_i3c_config *config = dev->config;
	uint32_t drained_resp = 0U;
	uint32_t drained_rx = 0U;
	uint32_t drained_ibi_hdr = 0U;
	uint32_t drained_ibi_data = 0U;

	for (int pass = 0; pass < DW_I3C_DRAIN_PASSES; pass++) {
		uint32_t qlvl = sys_read32(config->regs + QUEUE_STATUS_LEVEL);
		uint32_t db = sys_read32(config->regs + DATA_BUFFER_STATUS_LEVEL);
		uint32_t resp_lvl = QUEUE_STATUS_LEVEL_RESP(qlvl);
		uint32_t rx_lvl = DATA_BUFFER_STATUS_LEVEL_RX(db);
		uint32_t ibi_cnt = QUEUE_STATUS_IBI_STATUS_CNT(qlvl);

		/* On DW I3C, cmd_lvl/tx_lvl report available slots, not pending data.
		 * Only response level, RX level, and IBI entries represent drainable occupancy.
		 */
		if (resp_lvl == 0U && rx_lvl == 0U && ibi_cnt == 0U) {
			break;
		}

		while (resp_lvl-- && drained_resp < DW_I3C_DRAIN_MAX_RESP) {
			(void)sys_read32(config->regs + RESPONSE_QUEUE_PORT);
			drained_resp++;
		}

		while (rx_lvl-- && drained_rx < DW_I3C_DRAIN_MAX_RX) {
			(void)sys_read32(config->regs + RX_TX_DATA_PORT);
			drained_rx++;
		}

		while (ibi_cnt-- && drained_ibi_hdr < DW_I3C_DRAIN_MAX_IBI) {
			uint32_t ibi = sys_read32(config->regs + IBI_QUEUE_STATUS);
			uint32_t ibi_words = DIV_ROUND_UP(IBI_QUEUE_STATUS_DATA_LEN(ibi), 4);

			drained_ibi_hdr++;
			while (ibi_words-- && drained_ibi_data < DW_I3C_DRAIN_MAX_IBI_DATA) {
				(void)sys_read32(config->regs + RX_TX_DATA_PORT);
				drained_ibi_data++;
			}
		}

		k_busy_wait(DW_I3C_DRAIN_SETTLE_US);
	}
}

/**
 * @brief Flush and resume the controller without touching the DAT or DCT
 *
 * Caller must already hold the transfer mutex
 *
 * @retval 0 on success.
 * @retval -errno Propagated from the pre-resume platform hook
 */
static int dw_i3c_recover_bus_locked_light(const struct device *dev)
{
	const struct dw_i3c_config *config = dev->config;
	uint32_t nibis;
	int ret;

	nibis = QUEUE_STATUS_IBI_STATUS_CNT(sys_read32(config->regs + QUEUE_STATUS_LEVEL));
	while (nibis--) {
		(void)sys_read32(config->regs + IBI_QUEUE_STATUS);
	}

	dw_i3c_flush_queues(dev);

	ret = dw_i3c_pre_resume_ctrl(dev);
	if (ret != 0) {
		return ret;
	}

	sys_write32(INTR_TRANSFER_ERR_STAT, config->regs + INTR_STATUS);
	(void)dw_i3c_pulse_resume(dev);

	return 0;
}

/**
 * @brief Recover the I3C bus.
 *
 * Attempts to bring the DesignWare I3C controller back to an idle/ready state
 * without destroying the driver's device table. Used by i3c_recover_bus()
 * and the i3c shell.
 *
 * Deliberately does NOT issue RESET_CTRL_SOFT or RESET_CTRL_ALL, since those
 * clear the Device Address Table (DAT) and Device Characteristic Table (DCT)
 * which hold per-target state the driver relies on. Only the FIFOs and
 * response/command/IBI queues are flushed, and the controller is RESUMEd if
 * halted by a prior error.
 *
 * @param dev Pointer to controller device driver instance.
 *
 * @retval 0 on success.
 * @retval -EACCES if controller is not in master mode.
 */
static int dw_i3c_recover_bus(const struct device *dev)
{
	const struct dw_i3c_config *config = dev->config;
	struct dw_i3c_data *data = dev->data;
	uint32_t nibis;
	uint32_t pstate;
	bool halted;
	int ret;

	if (!dw_i3c_is_current_controller(dev)) {
		return -EACCES;
	}

	ret = k_mutex_lock(&data->mt, K_MSEC(1000));
	if (ret) {
		LOG_ERR("%s: recover_bus mutex err (%d)", dev->name, ret);
		return ret;
	}

	/* Drain any pending IBIs so the controller is not blocked by
	 * an unread IBI queue when we try to resume.
	 */
	nibis = QUEUE_STATUS_IBI_STATUS_CNT(sys_read32(config->regs + QUEUE_STATUS_LEVEL));
	while (nibis--) {
		(void)sys_read32(config->regs + IBI_QUEUE_STATUS);
	}

	pstate = sys_read32(config->regs + PRESENT_STATE);
	halted = (PRESENT_STATE_CM_TFR_STS(pstate) == CM_TFR_STS_CTRL_HALT);

	LOG_DBG("%s: recover_bus pstate 0x%08x (tfr_sts 0x%02x st_sts 0x%02x)", dev->name, pstate,
		(unsigned int)PRESENT_STATE_CM_TFR_STS(pstate),
		(unsigned int)PRESENT_STATE_CM_TFR_ST_STS(pstate));

	/* Stuck mid-transfer: only full_reset can unwedge. */
	if (!halted && PRESENT_STATE_CM_TFR_STS(pstate) != 0U &&
	    !(pstate & PRESENT_STATE_CONTROLLER_IDLE)) {
		k_mutex_unlock(&data->mt);
		return dw_i3c_full_reset(dev);
	}

	/* SOFT reset is NOT asserted here, so DAT/DCT survive. */
	dw_i3c_flush_queues(dev);

	if (halted) {
		/* Without pre_resume_ctrl and the INTR_STATUS clear, RESUME does not
		 * take effect on wrappered variants.
		 */
		ret = dw_i3c_pre_resume_ctrl(dev);
		if (ret != 0) {
			k_mutex_unlock(&data->mt);
			return ret;
		}

		sys_write32(INTR_TRANSFER_ERR_STAT, config->regs + INTR_STATUS);

		/* A RESUME still set from an earlier attempt swallows the next pulse. */
		if ((sys_read32(config->regs + DEVICE_CTRL) & DEV_CTRL_RESUME) != 0U) {
			dw_i3c_clear_resume(dev);

			if ((sys_read32(config->regs + DEVICE_CTRL) & DEV_CTRL_RESUME) != 0U) {
				dw_i3c_enable_controller(config, false);
				k_busy_wait(10);
				dw_i3c_enable_controller(config, true);
				dw_i3c_clear_resume(dev);
			}

			(void)dw_i3c_wait_resume_clear(dev, DW_I3C_RESUME_TIMEOUT_US);
		}

		if (dw_i3c_pulse_resume(dev) != 0) {
			k_mutex_unlock(&data->mt);
			return dw_i3c_full_reset(dev);
		}
	}

	k_mutex_unlock(&data->mt);

	return 0;
}
#endif /* CONFIG_I3C_CONTROLLER */
#ifdef CONFIG_I3C_TARGET
/**
 * @brief Writes to the Target's TX FIFO
 *
 * The Synopsys I3C will then ACK read requests to it's TX FIFO from a
 * Controller, if there is no tx cmd in cmd Q. Then it will NACK.
 *
 * @param dev Pointer to the device structure for an I3C controller
 *            driver configured in target mode.
 * @param buf Pointer to the buffer
 * @param len Length of the buffer
 *
 * @retval Total number of bytes written
 * @retval -EACCES Not in Target Mode
 * @retval -ENOSPC No space in Tx FIFO
 */
static int dw_i3c_target_tx_write(const struct device *dev, uint8_t *buf, uint16_t len,
				  uint8_t hdr_mode)
{
	struct dw_i3c_data *data = dev->data;
	struct dw_i3c_xfer *xfer = &data->xfer;

	/* check if we are in target mode */
	if (dw_i3c_is_current_controller(dev)) {
		return -EACCES;
	}

	/*
	 * TODO: if len is greater than fifo size, then it will need to be written to based
	 * on the threshold interrupt
	 */
	if (len > (data->txfifodepth * BYTES_PER_DWORD)) {
		return -ENOSPC;
	}

	k_mutex_lock(&data->mt, K_FOREVER);

	if ((hdr_mode == 0) || (hdr_mode & data->common.ctrl_config.supported_hdr)) {
		/* Write to CMD */
		memset(xfer, 0, sizeof(struct dw_i3c_xfer));
		xfer->ncmds = 1;

		/* TODO: write_tx_fifo needs to check that the fifo doesn't fill up */
		struct dw_i3c_cmd *cmd = &xfer->cmds[0];

		cmd->cmd_hi = 0;
		cmd->cmd_lo = COMMAND_PORT_TID(0) | COMMAND_PORT_ARG_DATA_LEN(len);
		cmd->buf = buf;
		cmd->tx_len = len;

		start_xfer(dev);
	} else {
		k_mutex_unlock(&data->mt);
		LOG_ERR("%s: Unsupported HDR Mode %d", dev->name, hdr_mode);
		return -ENOTSUP;
	}

	k_mutex_unlock(&data->mt);

	/* return total bytes written */
	return (int)len;
}

/**
 * @brief Instructs the I3C Target device to register itself to the I3C Controller
 *
 * This routine instructs the I3C Target device to register itself to the I3C
 * Controller via its parent controller's i3c_target_register() API.
 *
 * @param dev Pointer to target device driver instance.
 * @param cfg Config struct with functions and parameters used by the I3C driver
 * to send bus events
 *
 * @return @see i3c_device_find.
 */
static int dw_i3c_target_register(const struct device *dev, struct i3c_target_config *cfg)
{
	struct dw_i3c_data *data = dev->data;

	data->target_config = cfg;
	return 0;
}

/**
 * @brief Unregisters the provided config as Target device
 *
 * This routine disables I3C target mode for the 'dev' I3C bus driver using
 * the provided 'config' struct containing the functions and parameters
 * to send bus events.
 *
 * @param dev Pointer to target device driver instance.
 * @param cfg Config struct with functions and parameters used by the I3C driver
 * to send bus events
 *
 * @return @see i3c_device_find.
 */
static int dw_i3c_target_unregister(const struct device *dev, struct i3c_target_config *cfg)
{
	/* no way to disable? maybe write DA to 0? */
	return 0;
}
#endif /* CONFIG_I3C_TARGET */

static int dw_i3c_pinctrl_enable(const struct device *dev, bool enable)
{
#ifdef CONFIG_PINCTRL
	const struct dw_i3c_config *config = dev->config;
	uint8_t state = enable ? PINCTRL_STATE_DEFAULT : PINCTRL_STATE_SLEEP;
	int ret;

	ret = pinctrl_apply_state(config->pcfg, state);
	if (ret == -ENOENT) {
		/* State not defined; ignore and return success. */
		ret = 0;
	}

	return ret;
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(enable);
	return 0;
#endif
}

static int dw_i3c_init(const struct device *dev)
{
	const struct dw_i3c_config *config = dev->config;
	struct dw_i3c_data *data = dev->data;
	struct i3c_config_controller *ctrl_config = &data->common.ctrl_config;
	int ret;
	uint32_t hw_capabilities;
	uint32_t queue_capability;
	uint32_t role;

	if (!device_is_ready(config->clock)) {
		return -ENODEV;
	}

	/* A platform clock_on hook owns the clock entirely. */
	if (!(DW_I3C_OPS(config) && config->ops->clock_on)) {
		ret = clock_control_on(config->clock, config->clock_subsys);
		if (ret < 0) {
			return ret;
		}
	}

	ret = dw_i3c_clock_on(dev);
	if (ret < 0) {
		return ret;
	}

#if DT_HAS_COMPAT_STATUS_OKAY(microchip_xec_i3c)
	if (config->is_mchp) {
		/* Clear the per-peripheral PCR sleep-enable bit */
		soc_xec_pcr_sleep_en_clear(config->enc_pcr);
	}
#endif

	ret = dw_i3c_pre_init(dev);
	if (ret < 0) {
		return ret;
	}

#ifdef CONFIG_I3C_USE_IBI
	k_sem_init(&data->ibi_sts_sem, 0, 1);
	k_sem_init(&data->sem_hj, 0, 1);
#endif /* CONFIG_I3C_USE_IBI */
	k_sem_init(&data->sem_xfer, 0, 1);
	k_mutex_init(&data->mt);
#if defined(CONFIG_I3C_CONTROLLER) && defined(CONFIG_I3C_TARGET)
	data->dev = dev;
	k_work_init(&data->deftgts_work, dw_i3c_deftgts_work_fn);
#endif /* CONFIG_I3C_CONTROLLER && CONFIG_I3C_TARGET */

	ret = dw_i3c_pinctrl_enable(dev, true);
	if (ret != 0) {
		return ret;
	}
#ifdef CONFIG_I3C_CONTROLLER
	data->mode = i3c_bus_mode(&config->common.dev_list);
#endif /* CONFIG_I3C_CONTROLLER */
	/* reset all */
	sys_write32(RESET_CTRL_ALL, config->regs + RESET_CTRL);

	/* SOFT_RST closes the register gate on wrappered integrations, so the
	 * next core access faults until the hook reopens it.
	 */
	ret = dw_i3c_post_reset(dev);
	if (ret != 0) {
		return ret;
	}

#if DT_HAS_COMPAT_STATUS_OKAY(microchip_xec_i3c)
	if (config->is_mchp) {
		/* XEC wrapper: select the pad group this controller drives */
		sys_write32((uint32_t)config->port_sel, config->regs + MCHP_HOST_CFG_OFS);
	}
#endif

	/* get DAT, DCT pointer */
	data->datstartaddr =
		DEVICE_ADDR_TABLE_ADDR(sys_read32(config->regs + DEVICE_ADDR_TABLE_POINTER));
	data->dctstartaddr =
		DEVICE_CHAR_TABLE_ADDR(sys_read32(config->regs + DEV_CHAR_TABLE_POINTER));

	/* get max devices based on table depth */
	data->maxdevs =
		DEVICE_ADDR_TABLE_DEPTH(sys_read32(config->regs + DEVICE_ADDR_TABLE_POINTER));
	data->free_pos = GENMASK(data->maxdevs - 1, 0);

	/* get fifo sizes */
	queue_capability = sys_read32(config->regs + QUEUE_SIZE_CAPABILITY);
	data->txfifodepth = QUEUE_SIZE_CAPABILITY_TX_BUF_DWORD_SIZE(queue_capability);
	data->rxfifodepth = QUEUE_SIZE_CAPABILITY_RX_BUF_DWORD_SIZE(queue_capability);
	data->cmdfifodepth = QUEUE_SIZE_CAPABILITY_CMD_BUF_DWORD_SIZE(queue_capability);
	data->respfifodepth = QUEUE_SIZE_CAPABILITY_RESP_BUF_DWORD_SIZE(queue_capability);
	data->ibififodepth = QUEUE_SIZE_CAPABILITY_IBI_BUF_DWORD_SIZE(queue_capability);

	/* get HDR capabilities and the device role (both live in HW_CAPABILITY) */
	ctrl_config->supported_hdr = 0;
	hw_capabilities = sys_read32(config->regs + HW_CAPABILITY);
	if (hw_capabilities & HW_CAPABILITY_HDR_TS_EN) {
		ctrl_config->supported_hdr |= I3C_MSG_HDR_TSP | I3C_MSG_HDR_TSL;
	}
	if (hw_capabilities & HW_CAPABILITY_HDR_DDR_EN) {
		ctrl_config->supported_hdr |= I3C_MSG_HDR_DDR;
	}

	/* Cache the synthesis-constant role from the HW_CAPABILITY word just read. */
	data->role = FIELD_GET(HW_CAPABILITY_DEVICE_ROLE_CONFIG_MASK, hw_capabilities);

	/* Derive is_secondary from the role and ASSERT it against the Kconfig selection.
	 * DEV_OPERATION_MODE (0xb0) is read only for the dual-role part.
	 */
	role = data->role;
	switch (role) {
	case HW_CAP_DEVICE_ROLE_MASTER:
		__ASSERT(IS_ENABLED(CONFIG_I3C_CONTROLLER),
			 "HW is controller-only but CONFIG_I3C_CONTROLLER is not set");
		ctrl_config->is_secondary = false;
		break;
	case HW_CAP_DEVICE_ROLE_SLAVE:
		__ASSERT(IS_ENABLED(CONFIG_I3C_TARGET),
			 "HW is target-only but CONFIG_I3C_TARGET is not set");
		ctrl_config->is_secondary = true;
		break;
	case HW_CAP_DEVICE_ROLE_SEC_MASTER: {
		uint32_t device_ctrl_ext;

		if (DW_I3C_OPS(config) && config->ops->dev_operation_mode_write_only) {
			ctrl_config->is_secondary = dw_i3c_is_secondary_requested(dev);
		} else {
			/* dual-role: DEV_OPERATION_MODE selects the boot mode */
			device_ctrl_ext = sys_read32(config->regs + DEVICE_CTRL_EXTENDED);
			ctrl_config->is_secondary =
				(DEVICE_CTRL_EXTENDED_DEV_OPERATION_MODE(device_ctrl_ext) ==
				 DEVICE_CTRL_EXTENDED_DEV_OPERATION_MODE_SLAVE);
		}
		__ASSERT((ctrl_config->is_secondary && IS_ENABLED(CONFIG_I3C_TARGET)) ||
				 (!ctrl_config->is_secondary && IS_ENABLED(CONFIG_I3C_CONTROLLER)),
			 "boot mode not supported by the selected Kconfig");
		break;
	}
	default:
		__ASSERT(false, "unexpected DEVICE_ROLE_CONFIG 0x%x", role);
		return -ENOTSUP;
	}

	/* disable ibi (dual-role rejects via these regs */
	if (DW_IBI_REJECT_VIA_REG(role)) {
		sys_write32(IBI_REQ_REJECT_ALL, config->regs + IBI_SIR_REQ_REJECT);
		sys_write32(IBI_REQ_REJECT_ALL, config->regs + IBI_MR_REQ_REJECT);
	}

	/* disable hot-join */
	sys_write32(sys_read32(config->regs + DEVICE_CTRL) | (DEV_CTRL_HOT_JOIN_NACK),
		    config->regs + DEVICE_CTRL);
#ifdef CONFIG_I3C_CONTROLLER
	ret = i3c_addr_slots_init(dev);
	if (ret != 0) {
		return ret;
	}

	if (!(ctrl_config->is_secondary)) {
		ret = set_controller_info(dev);
		if (ret) {
			return ret;
		}
	}
#endif /* CONFIG_I3C_CONTROLLER */
	dw_i3c_enable_controller(config, true);

	ret = dw_i3c_init_scl_timing(dev, ctrl_config);
	if (ret != 0) {
		LOG_ERR("%s: Clock setting failed", dev->name);
		return ret;
	}

	if (DW_I3C_OPS(config) && config->ops->post_enable) {
		config->ops->post_enable(dev, ctrl_config->is_secondary);
	}

	enable_interrupts(dev);

#ifdef CONFIG_I3C_CONTROLLER
	if (!(ctrl_config->is_secondary)) {
		ret = dw_i3c_prepare_bus_init(dev);
		if (ret != 0) {
			return ret;
		}
	}
#endif /* CONFIG_I3C_CONTROLLER */

#ifdef CONFIG_I3C_CONTROLLER
	if (!(ctrl_config->is_secondary)) {
		/* Perform bus initialization - skip if no I3C devices are known. */
		if (config->common.dev_list.num_i3c > 0 &&
		    !(config->common.flags & I3C_CONTROLLER_FLAG_DISABLE_BUS_INIT)) {
			ret = i3c_bus_init(dev, &config->common.dev_list);
			if (ret != 0) {
				/* Not fatal: targets that missed enumeration can hot-join. */
				LOG_WRN("%s: bus initialization failed (%d)", dev->name, ret);
			}
		}
		/* Bus Initialization Complete, allow HJ ACKs if not disabled */
		if (!(config->common.flags & I3C_CONTROLLER_FLAG_DISABLE_HJ_AT_INIT)) {
			sys_write32(sys_read32(config->regs + DEVICE_CTRL) &
				    ~(DEV_CTRL_HOT_JOIN_NACK),
				    config->regs + DEVICE_CTRL);
		}
	}
#endif /* CONFIG_I3C_CONTROLLER */

	return 0;
}

#if defined(CONFIG_PM_DEVICE)
static int dw_i3c_pm_ctrl(const struct device *dev, enum pm_device_action action)
{
	const struct dw_i3c_config *config = dev->config;
	int ret;

	LOG_DBG("PM action: %d", (int)action);

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		dw_i3c_enable_controller(config, false);
		return dw_i3c_pinctrl_enable(dev, false);

	case PM_DEVICE_ACTION_RESUME:
		ret = dw_i3c_pinctrl_enable(dev, true);
		if (ret != 0) {
			return ret;
		}
		if (DW_I3C_OPS(config) && config->ops->pm_resume) {
			ret = config->ops->pm_resume(dev);
			if (ret != 0) {
				return ret;
			}
		}
		dw_i3c_enable_controller(config, true);
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif

static DEVICE_API(i3c, dw_i3c_api) = {
#ifdef CONFIG_I3C_CONTROLLER
	.i2c_api.configure = dw_i3c_i2c_api_configure,
	.i2c_api.transfer = dw_i3c_i2c_api_transfer,
	.i2c_api.recover_bus = dw_i3c_recover_bus,
#ifdef CONFIG_I2C_RTIO
	.i2c_api.iodev_submit = i2c_iodev_submit_fallback,
#endif
#endif /* CONFIG_I3C_CONTROLLER */

	.configure = dw_i3c_configure,
	.config_get = dw_i3c_config_get,
#ifdef CONFIG_I3C_CONTROLLER
	.attach_i3c_device = dw_i3c_attach_device,
	.reattach_i3c_device = dw_i3c_reattach_device,
	.detach_i3c_device = dw_i3c_detach_device,

	.do_daa = dw_i3c_do_daa,
	.do_ccc = dw_i3c_do_ccc,

	.i3c_device_find = dw_i3c_device_find,
	.recover_bus = dw_i3c_recover_bus,
	.i3c_xfers = dw_i3c_xfers,
#endif /* CONFIG_I3C_CONTROLLER */
#ifdef CONFIG_I3C_TARGET
	.target_tx_write = dw_i3c_target_tx_write,
	.target_register = dw_i3c_target_register,
	.target_unregister = dw_i3c_target_unregister,
#endif /* CONFIG_I3C_TARGET */
#ifdef CONFIG_I3C_USE_IBI
#ifdef CONFIG_I3C_CONTROLLER
	.ibi_hj_response = dw_i3c_controller_ibi_hj_response,
	.ibi_crr_response = dw_i3c_controller_ibi_crr_response,
	.ibi_enable = dw_i3c_controller_enable_ibi,
	.ibi_disable = dw_i3c_controller_disable_ibi,
#endif /* CONFIG_I3C_CONTROLLER */
#ifdef CONFIG_I3C_TARGET
	.ibi_raise = dw_i3c_target_ibi_raise,
#endif /* CONFIG_I3C_TARGET */
#endif /* CONFIG_I3C_USE_IBI */

#ifdef CONFIG_I3C_RTIO
	.iodev_submit = i3c_iodev_submit_fallback,
#endif
};

#define I3C_DW_IRQ_HANDLER(n)                                                                      \
	static void i3c_dw_irq_config_##n(void)                                                    \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), i3c_dw_irq,                 \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}

#if defined(CONFIG_PINCTRL)
#define I3C_DW_PINCTRL_DEFINE(n) PINCTRL_DT_INST_DEFINE(n)
#define I3C_DW_PINCTRL_INIT(n)   .pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),
#else
#define I3C_DW_PINCTRL_DEFINE(n)
#define I3C_DW_PINCTRL_INIT(n)
#endif

/* Per-vendor platform-ops selection; vendors redefine this alongside their table. */
#define DW_I3C_PLATFORM_OPS_INIT(n)

#if defined(CONFIG_PM_DEVICE)
#define I3C_DW_PM_DEFINE(n) PM_DEVICE_DT_INST_DEFINE(n, dw_i3c_pm_ctrl)
#define I3C_DW_PM_GET(n)    PM_DEVICE_DT_INST_GET(n)
#else
#define I3C_DW_PM_DEFINE(n)
#define I3C_DW_PM_GET(n) NULL
#endif

/* clang-format off */
#define DEFINE_DEVICE_FN(n)                                                                        \
	I3C_DW_IRQ_HANDLER(n)                                                                      \
	I3C_DW_PINCTRL_DEFINE(n);                                                                  \
	IF_ENABLED(CONFIG_I3C_CONTROLLER,                                                          \
		   (static struct i3c_device_desc dw_i3c_device_array_##n[] =                      \
			    I3C_DEVICE_ARRAY_DT_INST(n);                                           \
		    static struct i3c_i2c_device_desc dw_i3c_i2c_device_array_##n[] =              \
			    I3C_I2C_DEVICE_ARRAY_DT_INST(n);))                                     \
	static struct dw_i3c_data dw_i3c_data_##n = {                                              \
		.common.ctrl_config.scl.i3c =                                                      \
			DT_INST_PROP_OR(n, i3c_scl_hz, I3C_BUS_TYP_I3C_SCL_RATE),                  \
		.common.ctrl_config.scl.i2c = DT_INST_PROP_OR(n, i2c_scl_hz, 0),                   \
		.common.ctrl_config.scl_od_min.high_ns = DT_INST_PROP(n, od_thigh_min_ns),         \
		.common.ctrl_config.scl_od_min.low_ns = DT_INST_PROP(n, od_tlow_min_ns),           \
	};                                                                                         \
	static const struct dw_i3c_config dw_i3c_cfg_##n = {                                       \
		.regs = DT_INST_REG_ADDR(n),                                                       \
		.target_mode = DT_INST_PROP_OR(n, target_mode, 0),                                \
		.clock = DEVICE_DT_GET_OR_NULL(DT_INST_CLOCKS_CTLR(n)),                            \
		.clock_subsys = COND_CODE_1(DT_INST_PHA_HAS_CELL(n, clocks, clkid),                \
				((clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, clkid)),           \
				((clock_control_subsys_t)0)),                                      \
		.irq_config_func = &i3c_dw_irq_config_##n,                                         \
		IF_ENABLED(CONFIG_I3C_CONTROLLER,                                                  \
			(.common.dev_list.i3c = dw_i3c_device_array_##n,                           \
			.common.dev_list.num_i3c = ARRAY_SIZE(dw_i3c_device_array_##n),            \
			.common.dev_list.i2c = dw_i3c_i2c_device_array_##n,                        \
			.common.dev_list.num_i2c = ARRAY_SIZE(dw_i3c_i2c_device_array_##n),        \
			.common.primary_controller_da =                                            \
				DT_INST_PROP_OR(n, primary_controller_da, 0x00),                   \
			.common.flags = I3C_CONTROLLER_CONFIG_FLAGS_DT_INST(n),))                  \
		I3C_DW_PINCTRL_INIT(n)                                                             \
		DW_I3C_PLATFORM_OPS_INIT(n)                                                        \
	};                                                                                         \
	I3C_DW_PM_DEFINE(n);                                                                       \
	DEVICE_DT_INST_DEFINE(n, dw_i3c_init, I3C_DW_PM_GET(n), &dw_i3c_data_##n,                 \
			      &dw_i3c_cfg_##n, POST_KERNEL, CONFIG_I3C_CONTROLLER_INIT_PRIORITY,   \
			      &dw_i3c_api)

#define DT_DRV_COMPAT snps_designware_i3c
DT_INST_FOREACH_STATUS_OKAY(DEFINE_DEVICE_FN);

/* ---------------------------------------------------------------------------
 * Microchip XEC (microchip,xec-i3c) instance registration.
 *
 * The XEC variant is the same DesignWare IP behind a Microchip wrapper, so it
 * reuses all of the driver logic above. It differs only in:
 *   - interrupt delivery through the XEC Interrupt Aggregator (ECIA),
 *   - the HOST_CFG port-select register,
 *   - the PCR sleep-enable bit, which the XEC clock control driver's on() does
 *     not clear,
 *   - the form of the clock_control subsys argument, which on XEC is a pointer
 *     to a union clock_mchp_xec_subsys rather than a packed integer.
 * ---------------------------------------------------------------------------
 */
#if DT_HAS_COMPAT_STATUS_OKAY(microchip_xec_i3c)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT microchip_xec_i3c

BUILD_ASSERT(IS_ENABLED(CONFIG_HAS_MCHP_MEC_I3C),
	     "microchip,xec-i3c requires an SoC that selects HAS_MCHP_MEC_I3C, otherwise "
	     "the XEC clock control driver does not report the I3C domain rate");

#define MCHP_I3C_GIRQ(n)     MCHP_XEC_ECIA_GIRQ(DT_INST_PROP_BY_IDX(n, girqs, 0))
#define MCHP_I3C_GIRQ_POS(n) MCHP_XEC_ECIA_GIRQ_POS(DT_INST_PROP_BY_IDX(n, girqs, 0))

#define MCHP_I3C_IRQ_HANDLER(n)                                                                    \
	static void xec_i3c_irq_config_##n(void)                                                   \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), i3c_dw_irq,                 \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		soc_ecia_girq_status_clear(MCHP_I3C_GIRQ(n), MCHP_I3C_GIRQ_POS(n));                \
		soc_ecia_girq_ctrl(MCHP_I3C_GIRQ(n), MCHP_I3C_GIRQ_POS(n), true);                  \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}

#define MCHP_I3C_DEVICE(n)                                                                         \
	MCHP_I3C_IRQ_HANDLER(n)                                                                    \
	I3C_DW_PINCTRL_DEFINE(n);                                                                  \
	static const union clock_mchp_xec_subsys xec_i3c_clk_subsys_##n = {                        \
		.val = MCHP_XEC_PCR_SCR_ENCODE(DT_INST_CLOCKS_CELL(n, regidx),                     \
					       DT_INST_CLOCKS_CELL(n, bitpos),                     \
					       DT_INST_CLOCKS_CELL(n, clkid)),                     \
	};                                                                                         \
	IF_ENABLED(CONFIG_I3C_CONTROLLER,                                                     \
		   (static struct i3c_device_desc xec_i3c_device_array_##n[] =                    \
			    I3C_DEVICE_ARRAY_DT_INST(n);                                         \
		    static struct i3c_i2c_device_desc xec_i3c_i2c_device_array_##n[] =           \
			    I3C_I2C_DEVICE_ARRAY_DT_INST(n);))    \
	static struct dw_i3c_data xec_i3c_data_##n = {                                             \
		.common.ctrl_config.scl.i3c =                                                      \
			DT_INST_PROP_OR(n, i3c_scl_hz, I3C_BUS_TYP_I3C_SCL_RATE),                  \
		.common.ctrl_config.scl.i2c = DT_INST_PROP_OR(n, i2c_scl_hz, 0),                   \
		.common.ctrl_config.scl_od_min.high_ns = DT_INST_PROP(n, od_thigh_min_ns),         \
		.common.ctrl_config.scl_od_min.low_ns = DT_INST_PROP(n, od_tlow_min_ns),           \
	};                                                                                         \
	static const struct dw_i3c_config xec_i3c_cfg_##n = {                                      \
		.regs = DT_INST_REG_ADDR(n),                                                       \
		.clock = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                    \
		.clock_subsys = (clock_control_subsys_t)(&xec_i3c_clk_subsys_##n),                 \
		.irq_config_func = &xec_i3c_irq_config_##n,                                        \
		.is_mchp = true,                                                                   \
		.port_sel = DT_INST_PROP_OR(n, port_sel, 0),                                       \
		.enc_pcr = MCHP_XEC_ENC_PCR_SCR(DT_INST_CLOCKS_CELL(n, regidx),                    \
						DT_INST_CLOCKS_CELL(n, bitpos)),                   \
		.girq_id = MCHP_I3C_GIRQ(n),                                                       \
		.girq_pos = MCHP_I3C_GIRQ_POS(n),                                                  \
		IF_ENABLED(CONFIG_I3C_CONTROLLER,                                     \
			(.common.dev_list.i3c = xec_i3c_device_array_##n,                        \
			.common.dev_list.num_i3c = ARRAY_SIZE(xec_i3c_device_array_##n),         \
			.common.dev_list.i2c = xec_i3c_i2c_device_array_##n,                     \
			.common.dev_list.num_i2c = ARRAY_SIZE(xec_i3c_i2c_device_array_##n),     \
			.common.primary_controller_da =                                          \
				DT_INST_PROP_OR(n, primary_controller_da, 0x00),                 \
			.common.flags = I3C_CONTROLLER_CONFIG_FLAGS_DT_INST(n),))                \
		I3C_DW_PINCTRL_INIT(n)};                                                         \
	I3C_DW_PM_DEFINE(n);                                                                       \
	DEVICE_DT_INST_DEFINE(n, dw_i3c_init, I3C_DW_PM_GET(n), &xec_i3c_data_##n,                 \
			      &xec_i3c_cfg_##n, POST_KERNEL, CONFIG_I3C_CONTROLLER_INIT_PRIORITY,  \
			      &dw_i3c_api);

DT_INST_FOREACH_STATUS_OKAY(MCHP_I3C_DEVICE);

#endif /* DT_HAS_COMPAT_STATUS_OKAY(microchip_xec_i3c) */
