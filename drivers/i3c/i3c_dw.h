/*
 * Copyright (c) 2023, Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_I3C_DW_H_
#define ZEPHYR_DRIVERS_I3C_DW_H_

/* Device Control Register */
#define DEVICE_CTRL				(0x0)
#define DEV_CTRL_ENABLE				BIT(31)
#define DEV_CTRL_RESUME				BIT(30)
#define DEV_CTRL_HOT_JOIN_NACK			BIT(8)
#define DEV_CTRL_I2C_SLAVE_PRESENT		BIT(7)

/* Device Address Register */
#define DEVICE_ADDR				(0x4)
#define DEV_ADDR_DYNAMIC_ADDR_VALID		BIT(31)
#define DEV_ADDR_DYNAMIC(x)			(((x) << 16) & GENMASK(22, 16))

/* Hardware Capability register */
#define HW_CAPABILITY				(0x8)
#define HW_CAPABILITY_MASK(x)		((x) & GENMASK(2, 0))


/* COMMAND_QUEUE_PORT register */
#define COMMAND_QUEUE_PORT			(0xc)
#define COMMAND_PORT_TOC			BIT(30)
#define COMMAND_PORT_READ_TRANSFER		BIT(28)
#define COMMAND_PORT_ROC			BIT(26)
#define COMMAND_PORT_SPEED(x)			(((x) << 21) & GENMASK(23, 21))
#define COMMAND_PORT_DEV_INDEX(x)		(((x) << 16) & GENMASK(20, 16))
#define COMMAND_PORT_CP				BIT(15)
#define COMMAND_PORT_CMD(x)			(((x) << 7) & GENMASK(14, 7))
#define COMMAND_PORT_TID(x)			(((x) << 3) & GENMASK(6, 3))
#define I3C_DDR_SPEED				(0x6)


#define COMMAND_PORT_ARG_DATA_LEN(x)		(((x) << 16) & GENMASK(31, 16))
#define COMMAND_PORT_TRANSFER_ARG		(0x01)

#define COMMAND_PORT_DEV_COUNT(x)		(((x) << 21) & GENMASK(25, 21))
#define COMMAND_PORT_ADDR_ASSGN_CMD		(0x03)

/* RESPONSE_QUEUE_PORT register */
#define RESPONSE_QUEUE_PORT			(0x10)
#define RESPONSE_PORT_ERR_STATUS(x)		(((x) & GENMASK(31, 28)) >> 28)
#define RESPONSE_NO_ERROR			(0)
#define RESPONSE_ERROR_CRC			(1)
#define RESPONSE_ERROR_PARITY			(2)
#define RESPONSE_ERROR_FRAME			(3)
#define RESPONSE_ERROR_IBA_NACK			(4)
#define RESPONSE_ERROR_ADDRESS_NACK		(5)
#define RESPONSE_ERROR_OVER_UNDER_FLOW		(6)
#define RESPONSE_ERROR_TRANSF_ABORT		(8)
#define RESPONSE_ERROR_I2C_W_NACK_ERR		(9)
#define RESPONSE_PORT_TID(x)			(((x) & GENMASK(27, 24)) >> 24)
#define RESPONSE_PORT_DATA_LEN(x)		((x) & GENMASK(15, 0))

#define RX_TX_BUFFER_DATA_PACKET_SIZE		(0x4)

/* Receive/Transmit Data Port Register */
#define RX_TX_DATA_PORT				(0x14)

/* In-Band Interrupt Queue Status Register */
#define IBI_QUEUE_STATUS			(0x18)
#define IBI_QUEUE_IBI_ID(x)			(((x) & GENMASK(15, 8)) >> 8)
#define IBI_QUEUE_IBI_ID_ADDR(x)		((x) >> 1)

/* Queue Threshold Control Register */
#define QUEUE_THLD_CTRL				(0x1c)
#define QUEUE_THLD_CTRL_IBI_STS_MASK		GENMASK(31, 24)
#define QUEUE_THLD_CTRL_RESP_BUF_MASK		GENMASK(15, 8)
#define QUEUE_THLD_CTRL_RESP_BUF(x)		(((x)-1) << 8)

/* Data Buffer Threshold Control Register */
#define DATA_BUFFER_THLD_CTRL			(0x20)
#define DATA_BUFFER_THLD_CTRL_RX_BUF		GENMASK(11, 8)

/*  IBI MR Request Rejection Control Register */
#define IBI_MR_REQ_REJECT			0x2C

/*  IBI SIR Request Rejection Control Register */
#define IBI_SIR_REQ_REJECT			0x30
#define IBI_SIR_REQ_ID(x)			((((x) & GENMASK(6, 5)) >> 5) +\
						((x) & GENMASK(4, 0)))
#define IBI_REQ_REJECT_ALL			GENMASK(31, 0)

/* Reset Control Register */
#define RESET_CTRL				(0x34)
#define RESET_CTRL_IBI_QUEUE			BIT(5)
#define RESET_CTRL_RX_FIFO			BIT(4)
#define RESET_CTRL_TX_FIFO			BIT(3)
#define RESET_CTRL_RESP_QUEUE			BIT(2)
#define RESET_CTRL_CMD_QUEUE			BIT(1)
#define RESET_CTRL_SOFT				BIT(0)
#define RESET_CTRL_ALL				\
	(RESET_CTRL_IBI_QUEUE | RESET_CTRL_RX_FIFO |	\
	 RESET_CTRL_TX_FIFO | RESET_CTRL_RESP_QUEUE|	\
	 RESET_CTRL_CMD_QUEUE | RESET_CTRL_SOFT)

/* Interrupt Status Register */
#define INTR_STATUS				(0x3c)

/* Interrupt Status Enable Register */
#define INTR_STATUS_EN				(0x40)

/* Interrupt Signal Enable Register */
#define INTR_SIGNAL_EN				(0x44)
#define INTR_BUSOWNER_UPDATE_STAT		BIT(13)
#define INTR_IBI_UPDATED_STAT			BIT(12)
#define INTR_READ_REQ_RECV_STAT			BIT(11)
#define INTR_TRANSFER_ERR_STAT			BIT(9)
#define INTR_DYN_ADDR_ASSGN_STAT		BIT(8)
#define INTR_CCC_UPDATED_STAT			BIT(6)
#define INTR_TRANSFER_ABORT_STAT		BIT(5)
#define INTR_RESP_READY_STAT			BIT(4)
#define INTR_CMD_QUEUE_READY_STAT		BIT(3)
#define INTR_IBI_THLD_STAT			BIT(2)
#define INTR_RX_THLD_STAT			BIT(1)
#define INTR_TX_THLD_STAT			BIT(0)
#define INTR_ALL				\
	(INTR_BUSOWNER_UPDATE_STAT | INTR_IBI_UPDATED_STAT |	\
	 INTR_READ_REQ_RECV_STAT | INTR_TRANSFER_ERR_STAT |	\
	 INTR_DYN_ADDR_ASSGN_STAT | INTR_CCC_UPDATED_STAT |	\
	 INTR_TRANSFER_ABORT_STAT | INTR_RESP_READY_STAT |	\
	 INTR_CMD_QUEUE_READY_STAT | INTR_IBI_THLD_STAT |	\
	 INTR_TX_THLD_STAT | INTR_RX_THLD_STAT)

#define INTR_CONTROLLER_MASK			\
	(INTR_TRANSFER_ERR_STAT | INTR_RESP_READY_STAT | INTR_IBI_THLD_STAT)

/* Queue Status Level Register */
#define QUEUE_STATUS_LEVEL			(0x4c)
#define QUEUE_STATUS_IBI_BUF_BLR(x)		(((x) & GENMASK(23, 16)) >> 16)
#define QUEUE_STATUS_LEVEL_RESP(x)		(((x) & GENMASK(15, 8)) >> 8)
#define QUEUE_STATUS_LEVEL_CMD(x)	((x) & GENMASK(7, 0))

/* Data Buffer Status Level Register */
#define DATA_BUFFER_STATUS_LEVEL	(0x50)
#define DATA_BUFFER_STATUS_LEVEL_TX(x)	((x) & GENMASK(7, 0))

/* Pointer for Device Address Table Registers */
#define DEVICE_ADDR_TABLE_POINTER		(0x5c)
#define DEVICE_ADDR_TABLE_ADDR(x)		((x) & GENMASK(15, 0))

/* Pointer for Device Characteristics Table */
#define DEV_CHAR_TABLE_POINTER			(0x60)
#define DEVICE_CHAR_TABLE_ADDR(x)		((x) & GENMASK(11, 0))

/* SCL I3C Open Drain Timing Register */
#define SCL_I3C_OD_TIMING			(0xb4)

/* SCL I3C Push Pull Timing Register */
#define SCL_I3C_PP_TIMING			(0xb8)
#define SCL_I3C_TIMING_HCNT(x)			((((uint32_t)x) << 16) & GENMASK(23, 16))
#define SCL_I3C_TIMING_LCNT(x)			((x) & GENMASK(7, 0))
#define SCL_I3C_TIMING_CNT_MIN			(5)

/* SCL I2C Fast Mode Timing Register */
#define SCL_I2C_FM_TIMING			(0xbc)
#define SCL_I2C_FM_TIMING_HCNT(x)		((((uint32_t)x) << 16) & GENMASK(31, 16))
#define SCL_I2C_FM_TIMING_LCNT(x)		((x) & GENMASK(15, 0))

/* SCL I2C Fast Mode Plus Timing Register */
#define SCL_I2C_FMP_TIMING			(0xc0)
#define SCL_I2C_FMP_TIMING_HCNT(x)		((((uint32_t)x) << 16) & GENMASK(23, 16))
#define SCL_I2C_FMP_TIMING_LCNT(x)		((x) & GENMASK(15, 0))

/*SCL Extended Low Count Timing Register*/
#define SCL_EXT_LCNT_TIMING			(0xc8)
#define SCL_EXT_LCNT_4(x)			((((uint32_t)x) << 24) & GENMASK(31, 24))
#define SCL_EXT_LCNT_3(x)			((((uint32_t)x) << 16) & GENMASK(23, 16))
#define SCL_EXT_LCNT_2(x)			((((uint32_t)x) << 8) & GENMASK(15, 8))
#define SCL_EXT_LCNT_1(x)			((x) & GENMASK(7, 0))

/* Bus Free Timing Register */
#define BUS_FREE_TIMING				(0xd4)
#define BUS_I3C_MST_FREE(x)			((x) & GENMASK(15, 0))

#define DEV_ADDR_TABLE_LEGACY_I2C_DEV	BIT(31)
#define DEV_ADDR_TABLE_STATIC_ADDR(x)	((x) & GENMASK(6, 0))

#define DEV_ADDR_TABLE_DYNAMIC_ADDR(x)		(((x) << 16) & GENMASK(23, 16))
#define DEV_ADDR_TABLE_LOC(start, idx)		((start) + ((idx) << 2))

#define DEV_CHAR_TABLE_LOC1(start, idx)		((start) + ((idx) << 4))
#define DEV_CHAR_TABLE_MSB_PID(x)		((x) & GENMASK(31, 16))
#define DEV_CHAR_TABLE_LSB_PID(x)		((x) & GENMASK(15, 0))
#define DEV_CHAR_TABLE_LOC2(start, idx)		((DEV_CHAR_TABLE_LOC1(start,\
						 idx)) + 4)
#define DEV_CHAR_TABLE_LOC3(start, idx)		((DEV_CHAR_TABLE_LOC1(start,\
						 idx)) + 8)
#define DEV_CHAR_TABLE_DCR(x)			((x) & GENMASK(7, 0))
#define DEV_CHAR_TABLE_BCR(x)			(((x) & GENMASK(15, 8)) >> 8)

#define I3C_BUS_SDR1_SCL_RATE			(8000000) /* 8 MHz */
#define I3C_BUS_SDR2_SCL_RATE			(6000000) /* 6 MHz */
#define I3C_BUS_SDR3_SCL_RATE			(4000000) /* 4 MHz */
#define I3C_BUS_SDR4_SCL_RATE			(2000000) /* 2 MHz */
#define I3C_BUS_I2C_FM_TLOW_MIN_NS		(1300)
#define I3C_BUS_I2C_FMP_TLOW_MIN_NS		(500)
#define I3C_BUS_THIGH_MAX_NS			(41)
#define I3C_PERIOD_NS				(1000000000)

#define I3C_SDR_MODE				(0x0)
#define I3C_HDR_MODE				(0x1)
#define I2C_SLAVE				(2)
#define I3C_SLAVE				(3)
#define I3C_GETMXDS_FORMAT_1_LEN		(2)
#define I3C_GETMXDS_FORMAT_2_LEN		(5)

/*
 * CCC event bits.
 */
#define I3C_CCC_EVENT_SIR			BIT(0)
#define I3C_CCC_EVENT_MR			BIT(1)
#define I3C_CCC_EVENT_HJ			BIT(3)

#define I3C_BUS_TYP_I3C_SCL_RATE		(12500000) /* 12.5 MHz */
#define I3C_BUS_I2C_FM_PLUS_SCL_RATE		(1000000) /*1 MHz*/
#define I3C_BUS_I2C_FM_SCL_RATE			(400000) /* 400 KHz */
#define I3C_BUS_TLOW_OD_MIN_NS			(200)

#define I3C_LVR_I2C_INDEX_MASK			GENMASK(7, 5)
#define I3C_LVR_I2C_INDEX(x)			((x) << 5)
#define I3C_LVR_I2C_FM_MODE			BIT(4)

#define I3C_HOT_JOIN_ADDR			(0x2)

#endif /* ZEPHYR_DRIVERS_I3C_DW_H_ */
