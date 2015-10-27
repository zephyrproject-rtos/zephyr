/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file Register sets for I2C driver for Quark SE Sensor Subsystem
 */

#ifndef __DRIVERS_I2C_QUARK_SE_SS_REGISTERS_H__
#define __DRIVERS_I2C_QUARK_SE_SS_REGISTERS_H__

/* Register offsets */
#define REG_CON				0x00
#define REG_DATA_CMD			0x01
#define REG_SS_SCL_CNT			0x02
#define REG_FS_SCL_CNT			0x04
#define REG_INTR_STAT			0x06
#define REG_INTR_MASK			0x07
#define REG_TL				0x08
#define REG_INTR_CLR			0x0A
#define REG_STATUS			0x0B
#define REG_TXFLR			0x0C
#define REG_SDA_CONFIG			0x0E
#define REG_TX_ABRT_SOURCE		0x0F
#define REG_ENABLE_STATUS		0x11

/*  IC_CON bits */
#define IC_CON_CLK_ENA			(1 << 31)
#define IC_CON_RSVP_4			(1 << 30)
#define IC_CON_SPKLEN_MASK		(0x8 << 22)
#define IC_CON_SPKLEN_POS		(22)
#define IC_CON_RSVP_3			(0x7 << 19)
#define IC_CON_TAR_SAR_MASK		(0x7FF << 9)
#define IC_CON_TAR_SAR_POS		(9)
#define IC_CON_RSVP_2			(1 << 8)
#define IC_CON_RESTART_EN		(1 << 7)
#define IC_CON_RSVP_1			(1 << 6)
#define IC_CON_10BIT_ADDR		(1 << 5)
#define IC_CON_SPEED_MASK		(2 << 3)
#define IC_CON_SPEED_POS		(3)
#define IC_CON_RSVP_0			(1 << 2)
#define IC_CON_ABORT			(1 << 1)
#define IC_CON_ENABLE			(1 << 0)

/* IC_DATA_CMD bits */
#define IC_DATA_CMD_DATA_MASK		0xFF
#define IC_DATA_CMD_CMD			(1 << 8)
#define IC_DATA_CMD_STOP		(1 << 9)
#define IC_DATA_CMD_RESTART		(1 << 10)
#define IC_DATA_CMD_POP			(1 << 30)
#define IC_DATA_CMD_STROBE		(1 << 31)

/* IC_INTR_STAT, IC_INTR_MASK, IC_INTR_CLR bits */
#define IC_INTR_RX_UNDER		(1 << 0)
#define IC_INTR_RX_OVER			(1 << 1)
#define IC_INTR_RX_FULL			(1 << 2)
#define IC_INTR_TX_OVER			(1 << 3)
#define IC_INTR_TX_EMPTY		(1 << 4)
#define IC_INTR_TX_ABRT			(1 << 6)
#define IC_INTR_RX_DONE			(1 << 7)
#define IC_INTR_ACTIVITY		(1 << 8)
#define IC_INTR_STOP_DET		(1 << 9)
#define IC_INTR_START_DET		(1 << 10)

#define IC_INTR_CLR_ALL			(0xFFFFFFFF)
#define IC_INTR_MASK_ALL		(0x0)

#define IC_INTR_MASK_TX			(IC_INTR_TX_OVER |  \
					 IC_INTR_TX_EMPTY | \
					 IC_INTR_TX_ABRT |  \
					 IC_INTR_STOP_DET)
#define IC_INTR_MASK_RX			(IC_INTR_RX_UNDER | \
					 IC_INTR_RX_OVER |  \
					 IC_INTR_RX_FULL | \
					 IC_INTR_STOP_DET)

/* IC_STATUS */
#define IC_STATUS_ACTIVITY		(1 << 0)
#define IC_STATUS_TFNF			(1 << 1)
#define IC_STATUS_TFE			(1 << 2)
#define IC_STATUS_RFNE			(1 << 3)
#define IC_STATUS_RFF			(1 << 4)
#define IC_STATUS_MST_ACTIVITY		(1 << 5)
#define IC_STATUS_SLV_ACTIVITY		(1 << 6)

#endif /* __DRIVERS_I2C_QUARK_SE_SS_REGISTERS_H__ */
