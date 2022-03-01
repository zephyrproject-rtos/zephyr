/*
 * Copyright (c) 2022 Andes Technology Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file I2C driver for AndesTech atciic100 IP
 */
#include <errno.h>
#include <kernel.h>
#include <device.h>
#include <soc.h>
#include <drivers/i2c.h>
#include <sys/util.h>
#include <sys/sys_io.h>

#ifndef BIT
#define BIT(n)			(1 << (n))
#define BITS2(m, n)		(BIT(m) | BIT(n))

/* bits range: for example BITS(16,23) = 0xFF0000
 *   ==>  (BIT(m)-1)   = 0x0000FFFF     ~(BIT(m)-1)   => 0xFFFF0000
 *   ==>  (BIT(n+1)-1) = 0x00FFFFFF
 */
#endif

#ifndef BITS
#define BITS(m, n)		(~(BIT(m)-1) & ((BIT(n)-1) | BIT(n)))
#endif /* BIT */


#define RED_IDR			0x00
#define REG_CFG			0x10
#define REG_INTE		0x14
#define REG_STAT		0x18
#define REG_ADDR		0x1C
#define REG_DATA		0x20
#define REG_CTRL		0x24
#define REG_CMD			0x28
#define REG_SET			0x2C

#define DEV_I2C_CFG(dev)					\
	((const struct i2c_atciic100_device_config *const)(dev)->config)

#define I2C_CFG(dev)  (DEV_I2C_CFG(dev)->i2c_base_addr + REG_CFG)
#define I2C_INTE(dev) (DEV_I2C_CFG(dev)->i2c_base_addr + REG_INTE)
#define I2C_STAT(dev) (DEV_I2C_CFG(dev)->i2c_base_addr + REG_STAT)
#define I2C_ADDR(dev) (DEV_I2C_CFG(dev)->i2c_base_addr + REG_ADDR)
#define I2C_CMD(dev)  (DEV_I2C_CFG(dev)->i2c_base_addr + REG_CMD)
#define I2C_SET(dev)  (DEV_I2C_CFG(dev)->i2c_base_addr + REG_SET)
#define I2C_DATA(dev) (DEV_I2C_CFG(dev)->i2c_base_addr + REG_DATA)
#define I2C_CTRL(dev)  (DEV_I2C_CFG(dev)->i2c_base_addr + REG_CTRL)

#define MEMSET(s, c, n)         __builtin_memset((s), (c), (n))
#define MEMCPY(des, src, n)     __builtin_memcpy((des), (src), (n))
#define INWORD(x)     sys_read32(x)
#define OUTWORD(x, d) sys_write32(d, x)

#define SLAVE_ADDR_MSK			BITS(0, 9)
#define DATA_MSK			BITS(0, 7)

/* Interrupt Enable Register(RW) */
#define IEN_ALL				BITS(0, 9)
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
#define STATUS_W1C_ALL			BITS(3, 9)
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
#define CTRL_DATA_COUNT			BITS(0, 7)

/* Command Register(RW) */
#define CMD_MSK				BITS(0, 2)
#define CMD_NO_ACT			(0x0)
#define CMD_ISSUE_TRANSACTION		(0x1)
#define CMD_ACK				(0x2)
#define CMD_NACK			(0x3)
#define CMD_CLEAR_FIFO			(0x4)
#define CMD_RESET_I2C			(0x5)

/* Setup Register(RW) */
#define SETUP_T_SUDAT			BITS(24, 28)
#define SETUP_T_SP			BITS(21, 23)
#define SETUP_T_HDDAT			BITS(16, 20)
#define SETUP_T_SCL_RATIO		BIT(13)
#define SETUP_T_SCLHI			BITS(4, 12)
#define SETUP_DMA_EN			BIT(3)
#define SETUP_MASTER			BIT(2)
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

#define DRIVER_OK			0
#define DRIVER_ERROR			-1
#define DRIVER_ERROR_BUSY		-2
#define DRIVER_ERROR_TIMEOUT		-3
#define DRIVER_ERROR_UNSUPPORTED	-4
#define DRIVER_ERROR_PARAMETER		-5
#define DRIVER_ERROR_SPECIFIC		-6

#define CB_OK                           0
#define CB_FAIL                         -7
#define DUMMY                           0xff

/****** I2C Event *****/
#define I2C_EVENT_TRANSFER_DONE		(1UL << 0)
#define I2C_FLAG_SLAVE_TX_START		(1UL << 0)
#define I2C_FLAG_SLAVE_TX_DONE		(1UL << 1)
#define I2C_FLAG_SLAVE_RX_START		(1UL << 2)
#define I2C_FLAG_SLAVE_RX_DONE		(1UL << 3)
#define I2C_FLAG_GENERAL_CALL		(1UL << 4)

#define MAX_XFER_SZ			(256)

enum _i2c_ctrl_reg_item_dir {
	I2C_MASTER_TX = 0x0,
	I2C_MASTER_RX = 0x1,
	I2C_SLAVE_TX = 0x1,
	I2C_SLAVE_RX = 0x0,
};


/* I2C driver running state */
enum _i2c_driver_state {
	I2C_DRV_NONE = 0x0,
	I2C_DRV_INIT = BIT(0),
	I2C_DRV_POWER = BIT(1),
	I2C_DRV_CFG_PARAM = BIT(2),
	I2C_DRV_MASTER_TX = BIT(3),
	I2C_DRV_MASTER_RX = BIT(4),
	I2C_DRV_SLAVE_TX = BIT(5),
	I2C_DRV_SLAVE_RX = BIT(6),
	I2C_DRV_MASTER_TX_CMPL = BIT(7),
	I2C_DRV_MASTER_RX_CMPL = BIT(8),
	I2C_DRV_SLAVE_TX_CMPL = BIT(9),
	I2C_DRV_SLAVE_RX_CMPL = BIT(10),
};

/* brief I2C Status */
struct _i2c_status {
	uint32_t mode             : 1;	/* /< Mode: 0=Slave, 1=Master */
	uint32_t direction        : 1;	/* /< Direction: 0=Transmit, 1=Receive */
	uint32_t general_call     : 1;
	uint32_t arbitration_lost : 1;
	uint32_t bus_error        : 1;
};

typedef void (*i2c_signalevent_t) (uint32_t event);

struct _i2c_info {
	struct k_sem			i2c_transfer_sem;
	struct k_sem			i2c_busy_sem;
	volatile uint32_t		driver_state;
	uint8_t				*middleware_rx_buf;
	uint8_t				*middleware_tx_buf;
	/* flags for flow control */
	uint32_t			last_rx_data_count;	/* salve mode only  */
	uint32_t			middleware_rx_ptr;	/* salve mode only  */
	uint8_t				nack_assert;	/* salve mode only  */
	uint32_t			fifo_depth;
	uint32_t			slave_addr;
	uint32_t			xfer_wt_num;
	uint32_t			xfer_rd_num;
	uint32_t			xfered_data_wt_ptr;	/* write pointer  */
	uint32_t			xfered_data_rd_ptr;	/* read pointer  */
	uint32_t			xfered_data_rd_ptr_ow; /* read pointer overwrite */
	uint8_t				xfer_data_rd_buf[MAX_XFER_SZ];
	uint32_t			slave_rx_cmpl_ctrl_reg_val;
	volatile struct _i2c_status	status;
	uint8_t				xfer_cmpl_count;
};
