/*
 * Copyright (c) 2020 Abram Early
 * Copyright (c) 2023 Andriy Gelman
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_CAN_MICROCHIP_MCP251XFD_H_
#define ZEPHYR_DRIVERS_CAN_MICROCHIP_MCP251XFD_H_

#include <stdint.h>

#include <zephyr/drivers/can.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>

#define MCP251XFD_UINT32_FLAG_TO_BYTE_MASK(flag_u32)			\
	((flag_u32) >> ROUND_DOWN(LOG2((flag_u32)), 8))

#define MCP251XFD_RAM_START_ADDR 0x400
#define MCP251XFD_RAM_SIZE       2048
#define MCP251XFD_RAM_ALIGNMENT  4
#define MCP251XFD_PAYLOAD_SIZE   CAN_MAX_DLEN

#define MCP251XFD_FIFO_TYPE_TEF 0
#define MCP251XFD_FIFO_TYPE_RX  1

#define MCP251XFD_TEF_FIFO_ITEM_SIZE 8
#define MCP251XFD_TX_QUEUE_ITEM_SIZE (8 + MCP251XFD_PAYLOAD_SIZE)

#if defined(CONFIG_CAN_RX_TIMESTAMP)
#define MCP251XFD_RX_FIFO_ITEM_SIZE  (4 + 8 + MCP251XFD_PAYLOAD_SIZE)
#else
#define MCP251XFD_RX_FIFO_ITEM_SIZE  (8 + MCP251XFD_PAYLOAD_SIZE)
#endif

#define MCP251XFD_TEF_FIFO_START_ADDR 0
#define MCP251XFD_TEF_FIFO_ITEMS      CONFIG_CAN_MCP251XFD_MAX_TX_QUEUE
#define MCP251XFD_TEF_FIFO_SIZE       (MCP251XFD_TEF_FIFO_ITEMS * MCP251XFD_TEF_FIFO_ITEM_SIZE)

#define MCP251XFD_TX_QUEUE_START_ADDR MCP251XFD_TEF_FIFO_SIZE
#define MCP251XFD_TX_QUEUE_ITEMS      CONFIG_CAN_MCP251XFD_MAX_TX_QUEUE
#define MCP251XFD_TX_QUEUE_SIZE       (MCP251XFD_TX_QUEUE_ITEMS * MCP251XFD_TX_QUEUE_ITEM_SIZE)

#define MCP251XFD_RX_FIFO_START_ADDR (MCP251XFD_TX_QUEUE_START_ADDR + MCP251XFD_TX_QUEUE_SIZE)
#define MCP251XFD_RX_FIFO_SIZE_MAX   (MCP251XFD_RAM_SIZE - MCP251XFD_RX_FIFO_START_ADDR)
#define MCP251XFD_RX_FIFO_ITEMS_MAX  (MCP251XFD_RX_FIFO_SIZE_MAX / MCP251XFD_RX_FIFO_ITEM_SIZE)

#define MCP251XFD_RX_FIFO_ITEMS CONFIG_CAN_MCP251XFD_RX_FIFO_ITEMS
#define MCP251XFD_RX_FIFO_SIZE  (MCP251XFD_RX_FIFO_ITEMS * MCP251XFD_RX_FIFO_ITEM_SIZE)

#define MCP251XFD_RX_FIFO_IDX 1
#define MCP251XFD_REG_SIZE    4

#define MCP251XFD_CRC_POLY 0x8005
#define MCP251XFD_CRC_SEED 0xffff

BUILD_ASSERT(MCP251XFD_TEF_FIFO_SIZE + MCP251XFD_TX_QUEUE_SIZE +
	     MCP251XFD_RX_FIFO_SIZE <= MCP251XFD_RAM_SIZE,
	     "Cannot fit FIFOs into RAM");

/* Timeout for changing mode */
#define MCP251XFD_MODE_CHANGE_TIMEOUT_USEC  200000U
#define MCP251XFD_MODE_CHANGE_RETRIES       100

#define MCP251XFD_PLLRDY_TIMEOUT_USEC 100000
#define MCP251XFD_PLLRDY_RETRIES      100

#define MCP251XFD_MAX_INT_HANDLER_CALLS  10
#define MCP251XFD_INT_HANDLER_SLEEP_USEC 10000


struct mcp251xfd_mailbox {
	can_tx_callback_t cb;
	void *cb_arg;
};

#define MCP251XFD_SPI_CMD_LEN       2
#define MCP251XFD_SPI_LEN_FIELD_LEN 1
#define MCP251XFD_SPI_CRC_LEN       2

/* MPC251x registers - mostly copied from Linux kernel implementation of driver */

/* CAN FD Controller Module SFR */
#define MCP251XFD_REG_CON                   0x00
#define MCP251XFD_REG_CON_TXBWS_MASK        GENMASK(31, 28)
#define MCP251XFD_REG_CON_ABAT              BIT(27)
#define MCP251XFD_REG_CON_REQOP_MASK        GENMASK(26, 24)
#define MCP251XFD_REG_CON_MODE_MIXED        0
#define MCP251XFD_REG_CON_MODE_SLEEP        1
#define MCP251XFD_REG_CON_MODE_INT_LOOPBACK 2
#define MCP251XFD_REG_CON_MODE_LISTENONLY   3
#define MCP251XFD_REG_CON_MODE_CONFIG       4
#define MCP251XFD_REG_CON_MODE_EXT_LOOPBACK 5
#define MCP251XFD_REG_CON_MODE_CAN2_0       6
#define MCP251XFD_REG_CON_MODE_RESTRICTED   7
#define MCP251XFD_REG_CON_OPMOD_MASK        GENMASK(23, 21)
#define MCP251XFD_REG_CON_TXQEN             BIT(20)
#define MCP251XFD_REG_CON_STEF              BIT(19)
#define MCP251XFD_REG_CON_SERR2LOM          BIT(18)
#define MCP251XFD_REG_CON_ESIGM             BIT(17)
#define MCP251XFD_REG_CON_RTXAT             BIT(16)
#define MCP251XFD_REG_CON_BRSDIS            BIT(12)
#define MCP251XFD_REG_CON_BUSY              BIT(11)
#define MCP251XFD_REG_CON_WFT_MASK          GENMASK(10, 9)
#define MCP251XFD_REG_CON_WFT_T00FILTER     0x0
#define MCP251XFD_REG_CON_WFT_T01FILTER     0x1
#define MCP251XFD_REG_CON_WFT_T10FILTER     0x2
#define MCP251XFD_REG_CON_WFT_T11FILTER     0x3
#define MCP251XFD_REG_CON_WAKFIL            BIT(8)
#define MCP251XFD_REG_CON_PXEDIS            BIT(6)
#define MCP251XFD_REG_CON_ISOCRCEN          BIT(5)
#define MCP251XFD_REG_CON_DNCNT_MASK        GENMASK(4, 0)

#define MCP251XFD_REG_CON_B2                (MCP251XFD_REG_CON + 2)
#define MCP251XFD_REG_CON_B3                (MCP251XFD_REG_CON + 3)

#define MCP251XFD_REG_NBTCFG                0x04
#define MCP251XFD_REG_NBTCFG_BRP_MASK       GENMASK(31, 24)
#define MCP251XFD_REG_NBTCFG_TSEG1_MASK     GENMASK(23, 16)
#define MCP251XFD_REG_NBTCFG_TSEG2_MASK     GENMASK(14, 8)
#define MCP251XFD_REG_NBTCFG_SJW_MASK       GENMASK(6, 0)

#define MCP251XFD_REG_DBTCFG                0x08
#define MCP251XFD_REG_DBTCFG_BRP_MASK       GENMASK(31, 24)
#define MCP251XFD_REG_DBTCFG_TSEG1_MASK     GENMASK(20, 16)
#define MCP251XFD_REG_DBTCFG_TSEG2_MASK     GENMASK(11, 8)
#define MCP251XFD_REG_DBTCFG_SJW_MASK       GENMASK(3, 0)

#define MCP251XFD_REG_TDC                   0x0c
#define MCP251XFD_REG_TDC_EDGFLTEN          BIT(25)
#define MCP251XFD_REG_TDC_SID11EN           BIT(24)
#define MCP251XFD_REG_TDC_TDCMOD_MASK       GENMASK(17, 16)
#define MCP251XFD_REG_TDC_TDCMOD_AUTO       2
#define MCP251XFD_REG_TDC_TDCMOD_MANUAL     1
#define MCP251XFD_REG_TDC_TDCMOD_DISABLED   0
#define MCP251XFD_REG_TDC_TDCO_MASK         GENMASK(14, 8)
#define MCP251XFD_REG_TDC_TDCV_MASK         GENMASK(5, 0)
#define MCP251XFD_REG_TDC_TDCO_MIN -64
#define MCP251XFD_REG_TDC_TDCO_MAX 63

#define MCP251XFD_REG_TBC                   0x10

#define MCP251XFD_REG_TSCON                 0x14
#define MCP251XFD_REG_TSCON_TSRES           BIT(18)
#define MCP251XFD_REG_TSCON_TSEOF           BIT(17)
#define MCP251XFD_REG_TSCON_TBCEN           BIT(16)
#define MCP251XFD_REG_TSCON_TBCPRE_MASK     GENMASK(9, 0)

#define MCP251XFD_REG_VEC                   0x18
#define MCP251XFD_REG_VEC_RXCODE_MASK       GENMASK(30, 24)
#define MCP251XFD_REG_VEC_TXCODE_MASK       GENMASK(22, 16)
#define MCP251XFD_REG_VEC_FILHIT_MASK       GENMASK(12, 8)
#define MCP251XFD_REG_VEC_ICODE_MASK        GENMASK(6, 0)

#define MCP251XFD_REG_INT                   0x1c
#define MCP251XFD_REG_INT_IF_MASK           GENMASK(15, 0)
#define MCP251XFD_REG_INT_IE_MASK           GENMASK(31, 16)
#define MCP251XFD_REG_INT_IVMIE             BIT(31)
#define MCP251XFD_REG_INT_WAKIE             BIT(30)
#define MCP251XFD_REG_INT_CERRIE            BIT(29)
#define MCP251XFD_REG_INT_SERRIE            BIT(28)
#define MCP251XFD_REG_INT_RXOVIE            BIT(27)
#define MCP251XFD_REG_INT_TXATIE            BIT(26)
#define MCP251XFD_REG_INT_SPICRCIE          BIT(25)
#define MCP251XFD_REG_INT_ECCIE             BIT(24)
#define MCP251XFD_REG_INT_TEFIE             BIT(20)
#define MCP251XFD_REG_INT_MODIE             BIT(19)
#define MCP251XFD_REG_INT_TBCIE             BIT(18)
#define MCP251XFD_REG_INT_RXIE              BIT(17)
#define MCP251XFD_REG_INT_TXIE              BIT(16)
#define MCP251XFD_REG_INT_IVMIF             BIT(15)
#define MCP251XFD_REG_INT_WAKIF             BIT(14)
#define MCP251XFD_REG_INT_CERRIF            BIT(13)
#define MCP251XFD_REG_INT_SERRIF            BIT(12)
#define MCP251XFD_REG_INT_RXOVIF            BIT(11)
#define MCP251XFD_REG_INT_TXATIF            BIT(10)
#define MCP251XFD_REG_INT_SPICRCIF          BIT(9)
#define MCP251XFD_REG_INT_ECCIF             BIT(8)
#define MCP251XFD_REG_INT_TEFIF             BIT(4)
#define MCP251XFD_REG_INT_MODIF             BIT(3)
#define MCP251XFD_REG_INT_TBCIF             BIT(2)
#define MCP251XFD_REG_INT_RXIF              BIT(1)
#define MCP251XFD_REG_INT_TXIF              BIT(0)

/* These IRQ flags must be cleared by SW in the CAN_INT register */
#define MCP251XFD_REG_INT_IF_CLEARABLE_MASK                                                        \
	(MCP251XFD_REG_INT_IVMIF | MCP251XFD_REG_INT_WAKIF | MCP251XFD_REG_INT_CERRIF |            \
	 MCP251XFD_REG_INT_SERRIF | MCP251XFD_REG_INT_MODIF)

#define MCP251XFD_REG_RXIF                  0x20
#define MCP251XFD_REG_TXIF                  0x24
#define MCP251XFD_REG_RXOVIF                0x28
#define MCP251XFD_REG_TXATIF                0x2c
#define MCP251XFD_REG_TXREQ                 0x30

#define MCP251XFD_REG_TREC                  0x34
#define MCP251XFD_REG_TREC_TXBO             BIT(21)
#define MCP251XFD_REG_TREC_TXBP             BIT(20)
#define MCP251XFD_REG_TREC_RXBP             BIT(19)
#define MCP251XFD_REG_TREC_TXWARN           BIT(18)
#define MCP251XFD_REG_TREC_RXWARN           BIT(17)
#define MCP251XFD_REG_TREC_EWARN            BIT(16)
#define MCP251XFD_REG_TREC_TEC_MASK         GENMASK(15, 8)
#define MCP251XFD_REG_TREC_REC_MASK         GENMASK(7, 0)

#define MCP251XFD_REG_BDIAG0                0x38
#define MCP251XFD_REG_BDIAG0_DTERRCNT_MASK  GENMASK(31, 24)
#define MCP251XFD_REG_BDIAG0_DRERRCNT_MASK  GENMASK(23, 16)
#define MCP251XFD_REG_BDIAG0_NTERRCNT_MASK  GENMASK(15, 8)
#define MCP251XFD_REG_BDIAG0_NRERRCNT_MASK  GENMASK(7, 0)

#define MCP251XFD_REG_BDIAG1                0x3c
#define MCP251XFD_REG_BDIAG1_DLCMM          BIT(31)
#define MCP251XFD_REG_BDIAG1_ESI            BIT(30)
#define MCP251XFD_REG_BDIAG1_DCRCERR        BIT(29)
#define MCP251XFD_REG_BDIAG1_DSTUFERR       BIT(28)
#define MCP251XFD_REG_BDIAG1_DFORMERR       BIT(27)
#define MCP251XFD_REG_BDIAG1_DBIT1ERR       BIT(25)
#define MCP251XFD_REG_BDIAG1_DBIT0ERR       BIT(24)
#define MCP251XFD_REG_BDIAG1_TXBOERR        BIT(23)
#define MCP251XFD_REG_BDIAG1_NCRCERR        BIT(21)
#define MCP251XFD_REG_BDIAG1_NSTUFERR       BIT(20)
#define MCP251XFD_REG_BDIAG1_NFORMERR       BIT(19)
#define MCP251XFD_REG_BDIAG1_NACKERR        BIT(18)
#define MCP251XFD_REG_BDIAG1_NBIT1ERR       BIT(17)
#define MCP251XFD_REG_BDIAG1_NBIT0ERR       BIT(16)
#define MCP251XFD_REG_BDIAG1_BERR_MASK                                                             \
	(MCP251XFD_REG_BDIAG1_DLCMM | MCP251XFD_REG_BDIAG1_ESI | MCP251XFD_REG_BDIAG1_DCRCERR |    \
	 MCP251XFD_REG_BDIAG1_DSTUFERR | MCP251XFD_REG_BDIAG1_DFORMERR |                           \
	 MCP251XFD_REG_BDIAG1_DBIT1ERR | MCP251XFD_REG_BDIAG1_DBIT0ERR |                           \
	 MCP251XFD_REG_BDIAG1_TXBOERR | MCP251XFD_REG_BDIAG1_NCRCERR |                             \
	 MCP251XFD_REG_BDIAG1_NSTUFERR | MCP251XFD_REG_BDIAG1_NFORMERR |                           \
	 MCP251XFD_REG_BDIAG1_NACKERR | MCP251XFD_REG_BDIAG1_NBIT1ERR |                            \
	 MCP251XFD_REG_BDIAG1_NBIT0ERR)
#define MCP251XFD_REG_BDIAG1_EFMSGCNT_MASK GENMASK(15, 0)

#define MCP251XFD_REG_TEFCON               0x40
#define MCP251XFD_REG_TEFCON_FSIZE_MASK    GENMASK(28, 24)
#define MCP251XFD_REG_TEFCON_FRESET        BIT(10)
#define MCP251XFD_REG_TEFCON_UINC          BIT(8)
#define MCP251XFD_REG_TEFCON_TEFTSEN       BIT(5)
#define MCP251XFD_REG_TEFCON_TEFOVIE       BIT(3)
#define MCP251XFD_REG_TEFCON_TEFFIE        BIT(2)
#define MCP251XFD_REG_TEFCON_TEFHIE        BIT(1)
#define MCP251XFD_REG_TEFCON_TEFNEIE       BIT(0)

#define MCP251XFD_REG_TEFSTA               0x44
#define MCP251XFD_REG_TEFSTA_TEFOVIF       BIT(3)
#define MCP251XFD_REG_TEFSTA_TEFFIF        BIT(2)
#define MCP251XFD_REG_TEFSTA_TEFHIF        BIT(1)
#define MCP251XFD_REG_TEFSTA_TEFNEIF       BIT(0)

#define MCP251XFD_REG_TEFUA                  0x48

#define MCP251XFD_REG_TXQCON                 0x50
#define MCP251XFD_REG_TXQCON_PLSIZE_MASK     GENMASK(31, 29)
#define MCP251XFD_REG_TXQCON_PLSIZE_8        0
#define MCP251XFD_REG_TXQCON_PLSIZE_12       1
#define MCP251XFD_REG_TXQCON_PLSIZE_16       2
#define MCP251XFD_REG_TXQCON_PLSIZE_20       3
#define MCP251XFD_REG_TXQCON_PLSIZE_24       4
#define MCP251XFD_REG_TXQCON_PLSIZE_32       5
#define MCP251XFD_REG_TXQCON_PLSIZE_48       6
#define MCP251XFD_REG_TXQCON_PLSIZE_64       7
#define MCP251XFD_REG_TXQCON_FSIZE_MASK      GENMASK(28, 24)
#define MCP251XFD_REG_TXQCON_TXAT_UNLIMITED  3
#define MCP251XFD_REG_TXQCON_TXAT_THREE_SHOT 1
#define MCP251XFD_REG_TXQCON_TXAT_ONE_SHOT   0
#define MCP251XFD_REG_TXQCON_TXAT_MASK       GENMASK(22, 21)
#define MCP251XFD_REG_TXQCON_TXPRI_MASK      GENMASK(20, 16)
#define MCP251XFD_REG_TXQCON_FRESET          BIT(10)
#define MCP251XFD_REG_TXQCON_TXREQ           BIT(9)
#define MCP251XFD_REG_TXQCON_UINC            BIT(8)
#define MCP251XFD_REG_TXQCON_TXEN            BIT(7)
#define MCP251XFD_REG_TXQCON_TXATIE          BIT(4)
#define MCP251XFD_REG_TXQCON_TXQEIE          BIT(2)
#define MCP251XFD_REG_TXQCON_TXQNIE          BIT(0)

#define MCP251XFD_REG_TXQSTA                 0x54
#define MCP251XFD_REG_TXQSTA_TXQCI_MASK      GENMASK(12, 8)
#define MCP251XFD_REG_TXQSTA_TXABT           BIT(7)
#define MCP251XFD_REG_TXQSTA_TXLARB          BIT(6)
#define MCP251XFD_REG_TXQSTA_TXERR           BIT(5)
#define MCP251XFD_REG_TXQSTA_TXATIF          BIT(4)
#define MCP251XFD_REG_TXQSTA_TXQEIF          BIT(2)
#define MCP251XFD_REG_TXQSTA_TXQNIF          BIT(0)

#define MCP251XFD_REG_TXQUA                   0x58

#define MCP251XFD_REG_FIFOCON(x)              (0x50 + 0xc * (x))
#define MCP251XFD_REG_FIFOCON_PLSIZE_MASK     GENMASK(31, 29)
#define MCP251XFD_REG_FIFOCON_PLSIZE_8        0
#define MCP251XFD_REG_FIFOCON_PLSIZE_12       1
#define MCP251XFD_REG_FIFOCON_PLSIZE_16       2
#define MCP251XFD_REG_FIFOCON_PLSIZE_20       3
#define MCP251XFD_REG_FIFOCON_PLSIZE_24       4
#define MCP251XFD_REG_FIFOCON_PLSIZE_32       5
#define MCP251XFD_REG_FIFOCON_PLSIZE_48       6
#define MCP251XFD_REG_FIFOCON_PLSIZE_64       7
#define MCP251XFD_REG_FIFOCON_FSIZE_MASK      GENMASK(28, 24)
#define MCP251XFD_REG_FIFOCON_TXAT_MASK       GENMASK(22, 21)
#define MCP251XFD_REG_FIFOCON_TXAT_ONE_SHOT   0
#define MCP251XFD_REG_FIFOCON_TXAT_THREE_SHOT 1
#define MCP251XFD_REG_FIFOCON_TXAT_UNLIMITED  3
#define MCP251XFD_REG_FIFOCON_TXPRI_MASK      GENMASK(20, 16)
#define MCP251XFD_REG_FIFOCON_FRESET          BIT(10)
#define MCP251XFD_REG_FIFOCON_TXREQ           BIT(9)
#define MCP251XFD_REG_FIFOCON_UINC            BIT(8)
#define MCP251XFD_REG_FIFOCON_TXEN            BIT(7)
#define MCP251XFD_REG_FIFOCON_RTREN           BIT(6)
#define MCP251XFD_REG_FIFOCON_RXTSEN          BIT(5)
#define MCP251XFD_REG_FIFOCON_TXATIE          BIT(4)
#define MCP251XFD_REG_FIFOCON_RXOVIE          BIT(3)
#define MCP251XFD_REG_FIFOCON_TFERFFIE        BIT(2)
#define MCP251XFD_REG_FIFOCON_TFHRFHIE        BIT(1)
#define MCP251XFD_REG_FIFOCON_TFNRFNIE        BIT(0)

#define MCP251XFD_REG_FIFOSTA(x)              (0x54 + 0xc * (x))
#define MCP251XFD_REG_FIFOSTA_FIFOCI_MASK     GENMASK(12, 8)
#define MCP251XFD_REG_FIFOSTA_TXABT           BIT(7)
#define MCP251XFD_REG_FIFOSTA_TXLARB          BIT(6)
#define MCP251XFD_REG_FIFOSTA_TXERR           BIT(5)
#define MCP251XFD_REG_FIFOSTA_TXATIF          BIT(4)
#define MCP251XFD_REG_FIFOSTA_RXOVIF          BIT(3)
#define MCP251XFD_REG_FIFOSTA_TFERFFIF        BIT(2)
#define MCP251XFD_REG_FIFOSTA_TFHRFHIF        BIT(1)
#define MCP251XFD_REG_FIFOSTA_TFNRFNIF        BIT(0)

#define MCP251XFD_REG_FIFOUA(x)               (0x58 + 0xc * (x))

#define MCP251XFD_REG_BYTE_FLTCON(m)	      (0x1d0 + m)
#define MCP251XFD_REG_BYTE_FLTCON_FBP_MASK    GENMASK(4, 0)
#define MCP251XFD_REG_BYTE_FLTCON_FLTEN       BIT(7)

#define MCP251XFD_REG_FLTOBJ(x)               (0x1f0 + 0x8 * (x))
#define MCP251XFD_REG_FLTOBJ_EXIDE            BIT(30)
#define MCP251XFD_REG_FLTOBJ_SID11            BIT(29)
#define MCP251XFD_REG_FLTOBJ_EID_MASK         GENMASK(28, 11)
#define MCP251XFD_REG_FLTOBJ_SID_MASK         GENMASK(10, 0)

#define MCP251XFD_REG_FLTMASK(x)              (0x1f4 + 0x8 * (x))
#define MCP251XFD_REG_MASK_MIDE               BIT(30)
#define MCP251XFD_REG_MASK_MSID11             BIT(29)
#define MCP251XFD_REG_MASK_MEID_MASK          GENMASK(28, 11)
#define MCP251XFD_REG_MASK_MSID_MASK          GENMASK(10, 0)

/* Message Object */
#define MCP251XFD_OBJ_ID_SID11                 BIT(29)
#define MCP251XFD_OBJ_ID_EID_MASK              GENMASK(28, 11)
#define MCP251XFD_OBJ_ID_SID_MASK              GENMASK(10, 0)
#define MCP251XFD_OBJ_FLAGS_SEQ_MCP2518FD_MASK GENMASK(31, 9)
#define MCP251XFD_OBJ_FLAGS_SEQ_MCP2517FD_MASK GENMASK(15, 9)
#define MCP251XFD_OBJ_FLAGS_SEQ_MASK           MCP251XFD_OBJ_FLAGS_SEQ_MCP2518FD_MASK
#define MCP251XFD_OBJ_FLAGS_ESI                BIT(8)
#define MCP251XFD_OBJ_FLAGS_FDF                BIT(7)
#define MCP251XFD_OBJ_FLAGS_BRS                BIT(6)
#define MCP251XFD_OBJ_FLAGS_RTR                BIT(5)
#define MCP251XFD_OBJ_FLAGS_IDE                BIT(4)
#define MCP251XFD_OBJ_FLAGS_DLC_MASK           GENMASK(3, 0)
#define MCP251XFD_OBJ_FILHIT_MASK	       GENMASK(15, 11)

#define MCP251XFD_OBJ_DATA_OFFSET	       2 /* offset to the data in sizeof(uint32_t) */
#define MCP251XFD_OBJ_HEADER_SIZE	       (MCP251XFD_OBJ_DATA_OFFSET * MCP251XFD_REG_SIZE)

#define MCP251XFD_REG_FRAME_EFF_SID_MASK       GENMASK(28, 18)
#define MCP251XFD_REG_FRAME_EFF_EID_MASK       GENMASK(17, 0)

/* MCP2517/18FD SFR */
#define MCP251XFD_REG_OSC                      0xe00
#define MCP251XFD_REG_OSC_SCLKRDY              BIT(12)
#define MCP251XFD_REG_OSC_OSCRDY               BIT(10)
#define MCP251XFD_REG_OSC_PLLRDY               BIT(8)
#define MCP251XFD_REG_OSC_CLKODIV_10           3
#define MCP251XFD_REG_OSC_CLKODIV_4            2
#define MCP251XFD_REG_OSC_CLKODIV_2            1
#define MCP251XFD_REG_OSC_CLKODIV_1            0
#define MCP251XFD_REG_OSC_CLKODIV_MASK         GENMASK(6, 5)
#define MCP251XFD_REG_OSC_SCLKDIV              BIT(4)
#define MCP251XFD_REG_OSC_LPMEN                BIT(3) /* MCP2518FD only */
#define MCP251XFD_REG_OSC_OSCDIS               BIT(2)
#define MCP251XFD_REG_OSC_PLLEN                BIT(0)

#define MCP251XFD_REG_IOCON                    0xe04
#define MCP251XFD_REG_IOCON_INTOD              BIT(30)
#define MCP251XFD_REG_IOCON_SOF                BIT(29)
#define MCP251XFD_REG_IOCON_TXCANOD            BIT(28)
#define MCP251XFD_REG_IOCON_PM1                BIT(25)
#define MCP251XFD_REG_IOCON_PM0                BIT(24)
#define MCP251XFD_REG_IOCON_GPIO1              BIT(17)
#define MCP251XFD_REG_IOCON_GPIO0              BIT(16)
#define MCP251XFD_REG_IOCON_LAT1               BIT(9)
#define MCP251XFD_REG_IOCON_LAT0               BIT(8)
#define MCP251XFD_REG_IOCON_XSTBYEN            BIT(6)
#define MCP251XFD_REG_IOCON_TRIS1              BIT(1)
#define MCP251XFD_REG_IOCON_TRIS0              BIT(0)

#define MCP251XFD_REG_CRC                      0xe08
#define MCP251XFD_REG_CRC_FERRIE               BIT(25)
#define MCP251XFD_REG_CRC_CRCERRIE             BIT(24)
#define MCP251XFD_REG_CRC_FERRIF               BIT(17)
#define MCP251XFD_REG_CRC_CRCERRIF             BIT(16)
#define MCP251XFD_REG_CRC_IF_MASK              GENMASK(17, 16)
#define MCP251XFD_REG_CRC_MASK                 GENMASK(15, 0)

#define MCP251XFD_REG_ECCCON                   0xe0c
#define MCP251XFD_REG_ECCCON_PARITY_MASK       GENMASK(14, 8)
#define MCP251XFD_REG_ECCCON_DEDIE             BIT(2)
#define MCP251XFD_REG_ECCCON_SECIE             BIT(1)
#define MCP251XFD_REG_ECCCON_ECCEN             BIT(0)

#define MCP251XFD_REG_ECCSTAT                  0xe10
#define MCP251XFD_REG_ECCSTAT_ERRADDR_MASK     GENMASK(27, 16)
#define MCP251XFD_REG_ECCSTAT_IF_MASK          GENMASK(2, 1)
#define MCP251XFD_REG_ECCSTAT_DEDIF            BIT(2)
#define MCP251XFD_REG_ECCSTAT_SECIF            BIT(1)

#define MCP251XFD_REG_DEVID                    0xe14 /* MCP2518FD only */
#define MCP251XFD_REG_DEVID_ID_MASK            GENMASK(7, 4)
#define MCP251XFD_REG_DEVID_REV_MASK           GENMASK(3, 0)

/* SPI commands */
#define MCP251XFD_SPI_INSTRUCTION_RESET          0x0000
#define MCP251XFD_SPI_INSTRUCTION_WRITE          0x2000
#define MCP251XFD_SPI_INSTRUCTION_READ           0x3000
#define MCP251XFD_SPI_INSTRUCTION_WRITE_CRC      0xa000
#define MCP251XFD_SPI_INSTRUCTION_READ_CRC       0xb000
#define MCP251XFD_SPI_INSTRUCTION_WRITE_CRC_SAFE 0xc000
#define MCP251XFD_SPI_ADDRESS_MASK               GENMASK(11, 0)

#define MCP251XFD_REG_FIFOCON_TO_STA(addr) (addr + 0x4)

#define MCP251XFD_REG_FLTCON(m) (0x1d0 + m)

struct mcp251xfd_txobj {
	uint32_t id;
	uint32_t flags;
	uint8_t data[CAN_MAX_DLEN];
} __packed;

struct mcp251xfd_rxobj {
	uint32_t id;
	uint32_t flags;
#if defined(CONFIG_CAN_RX_TIMESTAMP)
	uint32_t timestamp;
#endif
	uint8_t data[CAN_MAX_DLEN];
} __packed;

struct mcp251xfd_tefobj {
	uint32_t id;
	uint32_t flags;
} __packed;

#define MCP251XFD_MAX_READ_FIFO_BUF_SIZE                                                           \
	MAX((MCP251XFD_RX_FIFO_ITEM_SIZE * MCP251XFD_RX_FIFO_ITEMS),                               \
	    (MCP251XFD_TEF_FIFO_ITEM_SIZE * MCP251XFD_TEF_FIFO_ITEMS))

#define MCP251XFD_MAX_READ_CRC_BUF_SIZE                                                            \
	(MCP251XFD_SPI_CRC_LEN + 2 * MCP251XFD_REG_SIZE)

#define MCP251XFD_SPI_BUF_SIZE                                                                     \
	MAX(MCP251XFD_MAX_READ_FIFO_BUF_SIZE, MCP251XFD_MAX_READ_CRC_BUF_SIZE)
#define MCP251XFD_SPI_HEADER_LEN (MCP251XFD_SPI_CMD_LEN + MCP251XFD_SPI_LEN_FIELD_LEN)

struct mcp251xfd_spi_data {
	uint8_t _unused[4 - (MCP251XFD_SPI_HEADER_LEN % 4)]; /* so that buf is 4-byte aligned */
	uint8_t header[MCP251XFD_SPI_HEADER_LEN]; /* contains spi_cmd and length field (if used) */
	uint8_t buf[MCP251XFD_SPI_BUF_SIZE];
} __packed __aligned(4);

struct mcp251xfd_fifo {
	uint32_t ram_start_addr;
	uint16_t reg_fifocon_addr;
	uint8_t capacity;
	uint8_t item_size;
	void (*msg_handler)(const struct device *dev, void *data);
};

struct mcp251xfd_data {
	struct can_driver_data common;

	/* Interrupt Data */
	struct gpio_callback int_gpio_cb;
	struct k_thread int_thread;
	k_thread_stack_t *int_thread_stack;
	struct k_sem int_sem;

	/* General */
	enum can_state state;
	struct k_mutex mutex;

	/* TX Callback */
	struct k_sem tx_sem;
	uint32_t mailbox_usage;
	struct mcp251xfd_mailbox mailbox[CONFIG_CAN_MCP251XFD_MAX_TX_QUEUE];

	/* Filter Data */
	uint32_t filter_usage;
	struct can_filter filter[CONFIG_CAN_MAX_FILTER];
	can_rx_callback_t rx_cb[CONFIG_CAN_MAX_FILTER];
	void *cb_arg[CONFIG_CAN_MAX_FILTER];

	const struct device *dev;

	uint8_t next_mcp251xfd_mode;
	uint8_t current_mcp251xfd_mode;
	int tdco;

	struct mcp251xfd_spi_data spi_data;

};

struct mcp251xfd_config {
	const struct can_driver_config common;

	/* spi configuration */
	struct spi_dt_spec bus;
	struct gpio_dt_spec int_gpio_dt;

	uint32_t osc_freq;

	/* IO Config */
	bool sof_on_clko;
	bool pll_enable;
	uint8_t clko_div;

	uint16_t timestamp_prescaler;

	const struct device *clk_dev;
	uint8_t clk_id;

	struct mcp251xfd_fifo rx_fifo;
	struct mcp251xfd_fifo tef_fifo;
};

#endif /* ZEPHYR_DRIVERS_CAN_MICROCHIP_MCP251XFD_H_ */
