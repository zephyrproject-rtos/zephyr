/*
 * Copyright (c) 2022 Andes Technology Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file I2C driver for AndesTech atciic100 IP
 */
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <soc.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/sys_io.h>

#define I2C_MAX_COUNT		256
#define BURST_CMD_COUNT		1

#define RED_IDR			0x00
#define REG_CFG			0x10
#define REG_INTE		0x14
#define REG_STAT		0x18
#define REG_ADDR		0x1C
#define REG_DATA		0x20
#define REG_CTRL		0x24
#define REG_CMD			0x28
#define REG_SET			0x2C

#define I2C_BASE(dev) \
	((const struct i2c_atciic100_config * const)(dev)->config)->base

#define I2C_CFG(dev)	(I2C_BASE(dev) + REG_CFG)
#define I2C_INTE(dev)	(I2C_BASE(dev) + REG_INTE)
#define I2C_STAT(dev)	(I2C_BASE(dev) + REG_STAT)
#define I2C_ADDR(dev)	(I2C_BASE(dev) + REG_ADDR)
#define I2C_CMD(dev)	(I2C_BASE(dev) + REG_CMD)
#define I2C_SET(dev)	(I2C_BASE(dev) + REG_SET)
#define I2C_DATA(dev)	(I2C_BASE(dev) + REG_DATA)
#define I2C_CTRL(dev)	(I2C_BASE(dev) + REG_CTRL)

#define TARGET_ADDR_MSK			BIT_MASK(10)
#define DATA_MSK			BIT_MASK(8)

/* Interrupt Enable Register(RW) */
#define IEN_ALL				BIT_MASK(10)
#define IEN_CMPL			BIT(9)
#define IEN_BYTE_RECV			BIT(8)
#define IEN_BYTE_TRANS			BIT(7)
#define IEN_START			BIT(6)
#define IEN_STOP			BIT(5)
#define IEN_ARB_LOSE			BIT(4)
#define IEN_ADDR_HIT			BIT(3)
#define IEN_FIFO_HALF			BIT(2)
#define IEN_FIFO_FULL			BIT(1)
#define IEN_FIFO_EMPTY			BIT(0)

/* Status Register(RW) */
#define STATUS_W1C_ALL			(BIT_MASK(7) << 3)
#define STATUS_LINE_SDA			BIT(14)
#define STATUS_LINE_SCL			BIT(13)
#define STATUS_GEN_CALL			BIT(12)
#define STATUS_BUS_BUSY			BIT(11)
#define STATUS_ACK			BIT(10)
#define STATUS_CMPL			BIT(9)
#define STATUS_BYTE_RECV		BIT(8)
#define STATUS_BYTE_TRANS		BIT(7)
#define STATUS_START			BIT(6)
#define STATUS_STOP			BIT(5)
#define STATUS_ARB_LOSE			BIT(4)
#define STATUS_ADDR_HIT			BIT(3)
#define STATUS_FIFO_HALF		BIT(2)
#define STATUS_FIFO_FULL		BIT(1)
#define STATUS_FIFO_EMPTY		BIT(0)

/* Control Register(RW) */
#define CTRL_PHASE_START		BIT(12)
#define CTRL_PHASE_ADDR			BIT(11)
#define CTRL_PHASE_DATA			BIT(10)
#define CTRL_PHASE_STOP			BIT(9)
#define CTRL_DIR			BIT(8)
#define CTRL_DATA_COUNT			BIT_MASK(8)

/* Command Register(RW) */
#define CMD_MSK				BIT_MASK(3)
#define CMD_NO_ACT			(0x0)
#define CMD_ISSUE_TRANSACTION		(0x1)
#define CMD_ACK				(0x2)
#define CMD_NACK			(0x3)
#define CMD_CLEAR_FIFO			(0x4)
#define CMD_RESET_I2C			(0x5)

/* Setup Register(RW) */
#define SETUP_T_SUDAT			(BIT_MASK(5) << 24)
#define SETUP_T_SP			(BIT_MASK(3) << 21)
#define SETUP_T_HDDAT			(BIT_MASK(5) << 16)
#define SETUP_T_SCL_RATIO		BIT(13)
#define SETUP_T_SCLHI			(BIT_MASK(9) << 4)
#define SETUP_DMA_EN			BIT(3)
#define SETUP_CONTROLLER		BIT(2)
#define SETUP_ADDRESSING		BIT(1)
#define SETUP_I2C_EN			BIT(0)

#if CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC == 30000000

#define SETUP_T_SUDAT_STD		(0x3)
#define SETUP_T_SP_STD			(0x1)
#define SETUP_T_HDDAT_STD		(5)
#define SETUP_T_SCL_RATIO_STD		(0x0)
#define SETUP_T_SCLHI_STD		(138)

#define SETUP_T_SUDAT_FAST		(0x0)
#define SETUP_T_SP_FAST			(0x1)
#define SETUP_T_HDDAT_FAST		(5)
#define SETUP_T_SCL_RATIO_FAST		(0x1)
#define SETUP_T_SCLHI_FAST		(18)

#define SETUP_T_SUDAT_FAST_P		(0x0)
#define SETUP_T_SP_FAST_P		(0x1)
#define SETUP_T_HDDAT_FAST_P		(0x0)
#define SETUP_T_SCL_RATIO_FAST_P	(0x1)
#define SETUP_T_SCLHI_FAST_P		(6)

#elif CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC == 40000000

#define SETUP_T_SUDAT_STD		(0x4)
#define SETUP_T_SP_STD			(0x2)
#define SETUP_T_HDDAT_STD		(0x6)
#define SETUP_T_SCL_RATIO_STD		(0x0)
#define SETUP_T_SCLHI_STD		(182)

#define SETUP_T_SUDAT_FAST		(0x0)
#define SETUP_T_SP_FAST			(0x2)
#define SETUP_T_HDDAT_FAST		(0x6)
#define SETUP_T_SCL_RATIO_FAST		(0x1)
#define SETUP_T_SCLHI_FAST		(23)

#define SETUP_T_SUDAT_FAST_P		(0x0)
#define SETUP_T_SP_FAST_P		(0x2)
#define SETUP_T_HDDAT_FAST_P		(0x0)
#define SETUP_T_SCL_RATIO_FAST_P	(0x1)
#define SETUP_T_SCLHI_FAST_P		(7)

#else

#define SETUP_T_SUDAT_STD		(0x9)
#define SETUP_T_SP_STD			(0x3)
#define SETUP_T_HDDAT_STD		(12)
#define SETUP_T_SCL_RATIO_STD		(0x0)
#define SETUP_T_SCLHI_STD		(287)

#define SETUP_T_SUDAT_FAST		(0x0)
#define SETUP_T_SP_FAST			(0x3)
#define SETUP_T_HDDAT_FAST		(12)
#define SETUP_T_SCL_RATIO_FAST		(0x1)
#define SETUP_T_SCLHI_FAST		(38)

#define SETUP_T_SUDAT_FAST_P		(0x0)
#define SETUP_T_SP_FAST_P		(0x3)
#define SETUP_T_HDDAT_FAST_P		(0x0)
#define SETUP_T_SCL_RATIO_FAST_P	(0x1)
#define SETUP_T_SCLHI_FAST_P		(13)

#endif

#define SETUP_SPEED_MSK			(SETUP_T_SUDAT		| \
					SETUP_T_SP		| \
					SETUP_T_HDDAT		| \
					SETUP_T_SCL_RATIO	| \
					SETUP_T_SCLHI)

#define SETUP_SPEED_STD			((SETUP_T_SUDAT_STD << 24)	| \
					(SETUP_T_SP_STD  << 21)		| \
					(SETUP_T_HDDAT_STD << 16)	| \
					(SETUP_T_SCL_RATIO_STD << 13)	| \
					(SETUP_T_SCLHI_STD << 4))

#define SETUP_SPEED_FAST		((SETUP_T_SUDAT_FAST << 24)	| \
					(SETUP_T_SP_FAST  << 21)	| \
					(SETUP_T_HDDAT_FAST << 16)	| \
					(SETUP_T_SCL_RATIO_FAST << 13)	| \
					(SETUP_T_SCLHI_FAST << 4))

#define SETUP_SPEED_FAST_PLUS		((SETUP_T_SUDAT_FAST_P << 24)	| \
					(SETUP_T_SP_FAST_P  << 21)	| \
					(SETUP_T_HDDAT_FAST_P << 16)	| \
					(SETUP_T_SCL_RATIO_FAST_P << 13)| \
					(SETUP_T_SCLHI_FAST_P << 4))

#define MAX_XFER_SZ			(256)

enum _i2c_ctrl_reg_item_dir {
	I2C_CONTROLLER_TX = 0x0,
	I2C_CONTROLLER_RX = 0x1,
	I2C_TARGET_TX = 0x1,
	I2C_TARGET_RX = 0x0,
};

/* I2C driver running state */
enum _i2c_driver_state {
	I2C_DRV_NONE = 0x0,
	I2C_DRV_INIT = BIT(0),
	I2C_DRV_POWER = BIT(1),
	I2C_DRV_CFG_PARAM = BIT(2),
	I2C_DRV_CONTROLLER_TX = BIT(3),
	I2C_DRV_CONTROLLER_RX = BIT(4),
	I2C_DRV_TARGET_TX = BIT(5),
	I2C_DRV_TARGET_RX = BIT(6),
	I2C_DRV_CONTROLLER_TX_CMPL = BIT(7),
	I2C_DRV_CONTROLLER_RX_CMPL = BIT(8),
	I2C_DRV_TARGET_TX_CMPL = BIT(9),
	I2C_DRV_TARGET_RX_CMPL = BIT(10),
};

/* brief I2C Status */
struct _i2c_status {
/* /< Mode: 0=Slave, 1=Master */
	uint32_t mode:1;
	uint32_t general_call: 1;
	uint32_t arbitration_lost : 1;
	uint32_t target_ack       : 1;
};

struct i2c_atciic100_dev_data_t {
	struct k_sem			bus_lock;
	struct k_sem			device_sync_sem;
	volatile uint32_t		driver_state;
	uint8_t				*middleware_rx_buf;
	uint8_t				*middleware_tx_buf;
	uint32_t			fifo_depth;
	uint32_t			target_addr;
	uint32_t			xfer_wt_num;
	uint32_t			xfer_rd_num;
	uint32_t			xfered_data_wt_ptr; /* write pointer  */
	uint32_t			xfered_data_rd_ptr; /* read pointer  */
	volatile struct _i2c_status	status;
	const struct i2c_target_callbacks	*target_callbacks;
	struct i2c_target_config	*target_config;
};
