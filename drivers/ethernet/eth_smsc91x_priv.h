/*
 * Copyright (c) 2023 Arm Limited (or its affiliates). All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_ETHERNET_ETH_SMSC91X_PRIV_H_
#define ZEPHYR_DRIVERS_ETHERNET_ETH_SMSC91X_PRIV_H_

#include <zephyr/sys/util.h>

/* All Banks, Offset 0xe: Bank Select Register */
#define BSR		  0xe
#define BSR_BANK_MASK	  GENMASK(2, 0) /* Which bank is currently selected */
#define BSR_IDENTIFY	  0x33
#define BSR_IDENTIFY_MASK GENMASK(15, 8)

/* Bank 0, Offset 0x0: Transmit Control Register */
#define TCR	   0x0
#define TCR_TXENA  0x0001 /* Enable/disable transmitter */
#define TCR_PAD_EN 0x0080 /* Pad TX frames to 64 bytes */

/* Bank 0, Offset 0x02: EPH status register */
#define EPHSR	     0x2
#define EPHSR_TX_SUC 0x0001 /* Last TX was successful */

/* Bank 0, Offset 0x4: Receive Control Register */
#define RCR	      0x4
#define RCR_RXEN      0x0100 /* Enable/disable receiver */
#define RCR_STRIP_CRC 0x0200 /* Strip CRC from RX packets */
#define RCR_SOFT_RST  0x8000 /* Software reset */

/* Bank0, Offset 0x6: Counter Register */
#define ECR		  0x6
#define ECR_SNGLCOL_MASK  GENMASK(3, 0)	  /* Single collisions */
#define ECR_MULCOL_MASK	  GENMASK(7, 4)	  /* Multiple collisions */
#define ECR_TX_DEFR_MASK  GENMASK(11, 8)  /* Transmit deferrals */
#define ECR_EXC_DEFR_MASK GENMASK(15, 12) /* Excessive deferrals */

/* Bank 0, Offset 0x8: Memory information register */
#define MIR	      0x8
#define MIR_SIZE_MASK GENMASK(7, 0)  /* Memory size (2k pages) */
#define MIR_FREE_MASK GENMASK(15, 8) /* Memory free (2k pages) */

/* bank 0, offset 0xa: receive/phy control reigster */
#define RPCR		  0xa
#define RPCR_ANEG	  0x0800 /* Put PHY in autonegotiation mode */
#define RPCR_DPLX	  0x1000 /* Put PHY in full-duplex mode */
#define RPCR_SPEED	  0x2000 /* Manual speed selection */
#define RPCR_LSA_MASK	  GENMASK(7, 5)
#define RPCR_LSB_MASK	  GENMASK(4, 2)
#define RPCR_LED_LINK_ANY 0x0 /* 10baseT or 100baseTX link detected */
#define RPCR_LED_LINK_10  0x2 /* 10baseT link detected */
#define RPCR_LED_LINK_FDX 0x3 /* Full-duplex link detect */
#define RPCR_LED_LINK_100 0x5 /* 100baseTX link detected */
#define RPCR_LED_ACT_ANY  0x4 /* TX or RX activity detected */
#define RPCR_LED_ACT_RX	  0x6 /* RX activity detected */
#define RPCR_LED_ACT_TX	  0x7 /* TX activity detected */

/* Bank 1, Offset 0x0: Configuration Register */
#define CR		0x0
#define CR_EPH_POWER_EN 0x8000 /* Disable/enable low power mode */

/* Bank 1, Offset 0x2: Base Address Register */
#define BAR 0x2

/* Bank 1, Offsets 0x4: Individual Address Registers */
#define IAR0 0x4
#define IAR1 0x5
#define IAR2 0x6
#define IAR3 0x7
#define IAR4 0x8
#define IAR5 0x9

/* Bank 1, Offset 0xc: Control Register */
#define CTR		 0xc
#define CTR_LE_ENABLE	 0x0080 /* Link error causes EPH interrupt */
#define CTR_AUTO_RELEASE 0x0800 /* Automatically release TX packets */

/* Bank 2, Offset 0x0: MMU Command Register */
#define MMUCR		      0x0
#define MMUCR_BUSY	      0x0001	    /* MMU is busy */
#define MMUCR_CMD_MASK	      GENMASK(7, 5) /* MMU command mask */
#define MMUCR_CMD_TX_ALLOC    1		    /* Alloc TX memory (256b chunks) */
#define MMUCR_CMD_MMU_RESET   2		    /* Reset MMU */
#define MMUCR_CMD_RELEASE     4		    /* Remove and release from RX FIFO */
#define MMUCR_CMD_RELEASE_PKT 5		    /* Release packet specified in PNR */
#define MMUCR_CMD_ENQUEUE     6		    /* Enqueue packet for TX */

/* Bank2, Offset 0x2: Packet Number Register */
#define PNR	 0x2
#define PNR_MASK GENMASK(5, 0)

/* Bank2, Offset 0x3: Allocation Result Register */
#define ARR	   0x3
#define ARR_FAILED 0x80
#define ARR_MASK   GENMASK(5, 0)

/* Bank 2, Offset 0x4: FIFO Ports Register */
#define FIFO		 0x04
#define FIFO_TX		 0x4
#define FIFO_RX		 0x5
#define FIFO_EMPTY	 0x80	       /* FIFO empty */
#define FIFO_PACKET_MASK GENMASK(5, 0) /* Packet number mask */

/* Bank2, Offset 0x6: Point Register */
#define PTR	      0x6
#define PTR_MASK      GENMASK(10, 0) /* Address accessible within TX/RX */
#define PTR_NOT_EMPTY 0x0800	     /* Write Data FIFO not empty */
#define PTR_READ      0x2000	     /* Set read/write */
#define PTR_AUTO_INCR 0x4000	     /* Auto increment on read/write */
#define PTR_RCV	      0x8000	     /* Read/write to/from RX/TX */

/* Bank2, Offset 0x8: Data register */
#define DATA0 0x8
#define DATA1 0xa

/* Bank 2, Offset 0xc: Interrupt Status Registers */
#define IST 0xc /* read only */
#define ACK 0xc /* write only */
#define MSK 0xd

#define RCV_INT	     0x0001 /* RX */
#define TX_INT	     0x0002 /* TX */
#define TX_EMPTY_INT 0x0004 /* TX empty */
#define ALLOC_INT    0x0008 /* Allocation complete */
#define RX_OVRN_INT  0x0010 /* RX overrun */
#define EPH_INT	     0x0020 /* EPH interrupt */
#define ERCV_INT     0x0040 /* Early RX */
#define MD_INT	     0x0080 /* MII */

/* Bank 3, Offset 0x8: Management interface register */
#define MGMT	  0x8
#define MGMT_MDO  0x0001 /* MII managememt output */
#define MGMT_MDI  0x0002 /* MII management input */
#define MGMT_MCLK 0x0004 /* MII management clock */
#define MGMT_MDOE 0x0008 /* MII management output enable */

/* Bank 3, Offset 0xa: Revision Register */
#define REV	      0xa
#define REV_CHIP_MASK GENMASK(7, 4)
#define REV_REV_MASK  GENMASK(3, 0)

/* Control Byte */
#define CTRL_CRC 0x10 /* Frame has CRC */
#define CTRL_ODD 0x20 /* Frame has odd bytes count */

/* Receive frame status */
#define RX_TOOSHORT 0x0400 /* Frame was too short */
#define RX_TOOLNG   0x0800 /* Frame was too long */
#define RX_ODDFRM   0x1000 /* Frame has odd number of bytes */
#define RX_BADCRC   0x2000 /* Frame failed CRC */
#define RX_ALIGNERR 0x8000 /* Frame has alignment error */
#define RX_LEN_MASK GENMASK(10, 0)

/* Length of status word + byte count + control bytes for packets */
#define PKT_CTRL_DATA_LEN 6

#endif
