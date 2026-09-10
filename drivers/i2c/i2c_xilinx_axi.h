/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright © 2023 Calian Ltd.  All rights reserved.
 *
 * Driver for the Xilinx AXI IIC Bus Interface.
 * This is an FPGA logic core as described by Xilinx document PG090.
 */

#ifndef ZEPHYR_DRIVERS_I2C_I2C_XILINX_AXI_H_
#define ZEPHYR_DRIVERS_I2C_I2C_XILINX_AXI_H_

#include <zephyr/sys/util_macro.h>

/* Register offsets */
enum xilinx_axi_i2c_register {
	REG_GIE = 0x01C,	  /* Global Interrupt Enable */
	REG_ISR = 0x020,	  /* Interrupt Status */
	REG_IER = 0x028,	  /* Interrupt Enable */
	REG_SOFTR = 0x040,	  /* Soft Reset */
	REG_CR = 0x100,		  /* Control */
	REG_SR = 0x104,		  /* Status */
	REG_TX_FIFO = 0x108,	  /* Transmit FIFO */
	REG_RX_FIFO = 0x10C,	  /* Receive FIFO */
	REG_ADR = 0x110,	  /* Target Address */
	REG_TX_FIFO_OCY = 0x114,  /* Transmit FIFO Occupancy */
	REG_RX_FIFO_OCY = 0x118,  /* Receive FIFO Occupancy */
	REG_TEN_ADR = 0x11C,	  /* Target Ten Bit Address */
	REG_RX_FIFO_PIRQ = 0x120, /* Receive FIFO Programmable Depth Interrupt */
	REG_GPO = 0x124,	  /* General Purpose Output */
	REG_TSUSTA = 0x128,	  /* Timing Parameter */
	REG_TSUSTO = 0x12C,	  /* Timing Parameter */
	REG_THDSTA = 0x130,	  /* Timing Parameter */
	REG_TSUDAT = 0x134,	  /* Timing Parameter */
	REG_TBUF = 0x138,	  /* Timing Parameter */
	REG_THIGH = 0x13C,	  /* Timing Parameter */
	REG_TLOW = 0x140,	  /* Timing Parameter */
	REG_THDDAT = 0x144,	  /* Timing Parameter */
};

/* Register bits */
/* Global Interrupt Enable */
enum xilinx_axi_i2c_gie_bits {
	GIE_ENABLE = BIT(31),
};

/* Interrupt Status/Interrupt Enable */
enum xilinx_axi_i2c_isr_bits {
	ISR_TX_HALF_EMPTY = BIT(7),	 /* Transmit FIFO Half Empty */
	ISR_NOT_ADDR_TARGET = BIT(6),	 /* Not Addressed As Target */
	ISR_ADDR_TARGET = BIT(5),	 /* Addressed As Target */
	ISR_BUS_NOT_BUSY = BIT(4),	 /* IIC Bus is Not Busy */
	ISR_RX_FIFO_FULL = BIT(3),	 /* Receive FIFO Full */
	ISR_TX_FIFO_EMPTY = BIT(2),	 /* Transmit FIFO Empty */
	ISR_TX_ERR_TARGET_COMP = BIT(1), /* Transmit Error/Target Transmit Complete */
	ISR_ARB_LOST = BIT(0),		 /* Arbitration Lost */
};

/* Soft Reset */
enum xilinx_axi_i2c_softr_vals {
	SOFTR_KEY = 0xA,
};

/* Control */
enum xilinx_axi_i2c_cr_bits {
	CR_GC_EN = BIT(6),	 /* General Call Enable */
	CR_RSTA = BIT(5),	 /* Repeated Start */
	CR_TXAK = BIT(4),	 /* Transmit Acknowledge Enable */
	CR_TX = BIT(3),		 /* Transmit/Receive Mode Select */
	CR_MSMS = BIT(2),	 /* Controller/Target Mode Select */
	CR_TX_FIFO_RST = BIT(1), /* Transmit FIFO Reset */
	CR_EN = BIT(0),		 /* AXI IIC Enable */
};

/* Status */
enum xilinx_axi_i2c_sr_bits {
	SR_TX_FIFO_EMPTY = BIT(7), /* Transmit FIFO empty */
	SR_RX_FIFO_EMPTY = BIT(6), /* Receive FIFO empty */
	SR_RX_FIFO_FULL = BIT(5),  /* Receive FIFO full */
	SR_TX_FIFO_FULL = BIT(4),  /* Transmit FIFO full */
	SR_SRW = BIT(3),	   /* Target Read/Write */
	SR_BB = BIT(2),		   /* Bus Busy */
	SR_AAS = BIT(1),	   /* Addressed As Target */
	SR_ABGC = BIT(0),	   /* Addressed By a General Call */
};

/* TX FIFO */
enum xilinx_axi_i2c_tx_fifo_bits {
	TX_FIFO_STOP = BIT(9),
	TX_FIFO_START = BIT(8),
};

/* RX FIFO */
enum xilinx_axi_i2c_rx_fifo_bits {
	RX_FIFO_DATA_MASK = 0xFF,
};

/* TX_FIFO_OCY */
enum xilinx_axi_i2c_tx_fifo_ocy_bits {
	TX_FIFO_OCY_MASK = 0x0F,
};

/* RX_FIFO_OCY */
enum xilinx_axi_i2c_rx_fifo_ocy_bits {
	RX_FIFO_OCY_MASK = 0x0F,
};

/* other constants */
enum {
	/* Size of RX/TX FIFO */
	FIFO_SIZE = 16,

	/* Maximum number of bytes that can be read in dynamic mode */
	MAX_DYNAMIC_READ_LEN = 255,
};

#endif
