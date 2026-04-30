/*
 * Copyright (c) 2026 Realtek Semiconductor, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_UDC_RTS5817_H
#define ZEPHYR_DRIVERS_USB_UDC_RTS5817_H

#include <zephyr/device.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/sys/byteorder.h>

/* Extra registers */
#define R_AL_SIE_STATE 0x401E205C

/* Dlink registers */
#define R_USB2_ANA_CFG0   0x00
#define R_USB2_ANA_CFG1   0x04
#define R_USB2_ANA_CFG2   0x08
#define R_USB2_ANA_CFG3   0x0C
#define R_USB2_ANA_CFG4   0x10
#define R_USB2_ANA_CFG5   0x14
#define R_USB2_ANA_CFG6   0x18
#define R_USB2_ANA_CFG7   0x1C
#define R_USB2_CTL_CFG0   0x20
#define R_USB2_CTL_CFG1   0x24
#define R_USB2_CTL_CFG2   0x28
#define R_USB2_PHY_CFG0   0x2C
#define R_USB2_PHY_CFG1   0x30
#define R_USB2_PHY_CFG2   0x34
#define R_USB2_PHY_STATUS 0x38
#define R_USB2_PHY_DUMMY0 0x3C
#define R_USB2_PHY_DUMMY1 0x40

/* Bits of R_USB2_ANA_CFG0 (0x00) */
#define REG_AUTO_K_OFFSET 16
#define REG_AUTO_K        BIT(16)

#define REG_ADJR_OFFSET 24
#define REG_ADJR_MASK   GENMASK(27, 24)

/* Bits of R_USB2_CTL_CFG2 (0x28) */
#define REG_SEN_NORM_OFFSET 0
#define REG_SEN_NORM_MASK   GENMASK(3, 0)

/* Bits of R_USB2_ANA_CFG7 (0x1C) */
#define REG_SRC_0_OFFSET 26
#define REG_SRC_0_MASK   GENMASK(28, 26)

/* Bits of R_USB2_ANA_CFG6 (0x18) */
#define REG_SH_0_OFFSET 8
#define REG_SH_0_MASK   GENMASK(11, 8)

/* SIE EP0 registers */
#define R_U2SIE_EP0_IRQ_EN      0x00
#define R_U2SIE_EP0_IRQ_STATUS  0x08
#define R_U2SIE_EP0_CFG         0x0C
#define R_U2SIE_EP0_CTL0        0x10
#define R_U2SIE_EP0_CTL1        0x14
#define R_U2SIE_EP0_MAXPKT      0x18
#define R_U2SIE_EP0_SETUP_DATA0 0x1C
#define R_U2SIE_EP0_SETUP_DATA1 0x20

/* Bits of R_U2SIE_EP0_IRQ_EN (0x00) and R_U2SIE_EP0_IRQ_STATUS (0x08) */
#define USB_EP0_INTOKEN_INT           BIT(0)
#define USB_EP0_OUTTOKEN_INT          BIT(1)
#define USB_EP0_DATAPKT_TRANS_INT     BIT(2)
#define USB_EP0_DATAPKT_RECV_INT      BIT(3)
#define USB_EP0_OUT_SHORTPKT_RECV_INT BIT(4)
#define USB_EP0_CTRL_STATUS_END_INT   BIT(5)
#define USB_EP0_SETUP_PACKET_INT      BIT(6)
#define USB_EP0_CTRL_STATUS_INT       BIT(7)
#define USB_EP0_INT_MASK              GENMASK(7, 0)

/* Bits of R_U2SIE_EP0_MAXPKT (0x18) */
#define EP0_MAXPKT_OFFSET 0
#define EP0_MAXPKT_MASK   GENMASK(6, 0)

/* Bits of R_U2SIE_EP0_CTL0 (0x10) */
#define EP0_STALL_OFFSET 1
#define EP0_STALL        BIT(1)

#define EP0_RESET_OFFSET 2
#define EP0_RESET        BIT(2)

/* Bits of R_U2SIE_EP0_CFG (0x0C) */
#define EP0_NAKOUT_MODE_OFFSET 0
#define EP0_NAKOUT_MODE        BIT(0)

/* Bits of R_U2SIE_EP0_CTL1 (0x14) */
#define EP0_CSH_OFFSET 0
#define EP0_CSH        BIT(0)

/* MC EP0 registers */
#define R_U2MC_EP0_CTL      0x0000
#define R_U2MC_EP0_BC       0x0004
#define R_U2MC_EP0_DUMMY    0x0008
#define R_U2MC_EP0_BUF_BASE 0x0400
#define R_U2MC_EP0_BUF_TOP  0x043F

/* Bits of R_U2MC_EP0_CTL (0x0000) */
#define U_BUF0_EP0_TX_EN_OFFSET 0
#define U_BUF0_EP0_TX_EN        BIT(0)

#define U_BUF0_EP0_RX_EN_OFFSET 16
#define U_BUF0_EP0_RX_EN        BIT(16)

/* SIE SYS registers */
#define R_U2SIE_SYS_CTRL       0x00
#define R_U2SIE_SYS_ADDR       0x04
#define R_U2SIE_SYS_IRQ_EN     0x08
#define R_U2SIE_SYS_IRQ_STATUS 0x10
#define R_U2SIE_SYS_FORCE_CMD  0x14
#define R_U2SIE_SYS_UTMI_CTRL  0x18
#define R_U2SIE_SYS_UTMI_CFG   0x1C
#define R_U2SIE_SYS_UTMI_STAT  0x20
#define R_U2SIE_SYS_PHY_CTRL   0x24
#define R_U2SIE_SYS_PHY_N_F    0x28
#define R_U2SIE_SYS_SLBTEST    0x2C
#define R_U2SIE_SYS_PKERR_CNT  0x30
#define R_U2SIE_SYS_RXERR_CNT  0x34
#define R_U2SIE_SYS_LPM_CFG0   0x38
#define R_U2SIE_SYS_LPM_DUMMY  0x3C
#define R_U2SIE_SYS_SIE_DUMMY0 0x40
#define R_U2SIE_SYS_SIE_DUMMY1 0x44
#define R_U2SIE_SYS_DPHY_CFG   0x54
#define R_U2SIE_SYS_ERROR_IN   0x58

/* Bits of R_U2SIE_SYS_CTRL (0x00) */
#define CONNECT_EN_OFFSET 0
#define CONNECT_EN        BIT(0)

#define WAKEUP_EN_OFFSET 1
#define WAKEUP_EN        BIT(1)

#define SUSPEND_EN_OFFSET 2
#define SUSPEND_EN        BIT(2)

#define CFG_FORCE_FS_JMP_SPD_NEG_FS_OFFSET 3
#define CFG_FORCE_FS_JMP_SPD_NEG_FS        BIT(3)

#define MODE_HS_OFFSET 4
#define MODE_HS        BIT(4)

#define CFG_FORCE_FW_REMOTE_WAKEUP_OFFSET 6
#define CFG_FORCE_FW_REMOTE_WAKEUP        BIT(6)

/* Bits of R_U2SIE_SYS_UTMI_CTRL (0x18) */
#define FORCE_FS_OFFSET 6
#define FORCE_FS        BIT(6)

/* Bits of R_U2SIE_SYS_IRQ_EN (0x08) and R_U2SIE_SYS_IRQ_STATUS (0x10)*/
#define USB_LS_LINE_STATE_INT   BIT(0)
#define USB_LS_SOF_INT          BIT(1)
#define USB_LS_SUSPEND_INT      BIT(2)
#define USB_LS_RESUME_INT       BIT(3)
#define USB_LS_PORT_RST_INT     BIT(4)
#define USB_LS_L1_SLEEP_INT     BIT(5)
#define USB_LS_L1_RESUME_INT    BIT(6)
#define USB_LS_SOF_INTERVAL_INT BIT(7)
#define USB_LS_INT_MASK         GENMASK(7, 0)

/* Endpoint registers */
#define U2SIE_EP_REG_INTERVAL 0x100

#define U2SIE_EP0_REG_OFFSET 0
#define U2SIE_EPA_REG_OFFSET 0x100
#define U2SIE_EPB_REG_OFFSET 0x200
#define U2SIE_EPC_REG_OFFSET 0x300
#define U2SIE_EPD_REG_OFFSET 0x400
#define U2SIE_EPE_REG_OFFSET 0x500
#define U2SIE_EPF_REG_OFFSET 0x600
#define U2SIE_EPG_REG_OFFSET 0x700

#define U2MC_EP0_REG_OFFSET 0
#define U2MC_EPA_REG_OFFSET 0x800
#define U2MC_EPB_REG_OFFSET 0x1000
#define U2MC_EPC_REG_OFFSET 0x1800
#define U2MC_EPD_REG_OFFSET 0x1C00
#define U2MC_EPE_REG_OFFSET 0x2000
#define U2MC_EPF_REG_OFFSET 0x2800

#define R_EP_U2SIE_CTL        0x00
#define R_EP_U2SIE_IRQ_EN     0x04
#define R_EP_U2SIE_IRQ_STATUS 0x0C

/* Note: some regs of EPG are different */
#define R_EPG_U2SIE_IRQ_STATUS  0x08
#define R_EPG_U2SIE_INTOUT_MC   0x0C
#define R_EPG_U2SIE_INTOUT_BUF0 0x10
#define R_EPG_U2SIE_INTOUT_LEN  0x90

#define R_EP_U2MC_BULK_IRQ        0x00
#define R_EP_U2MC_BULK_EN         0x04
#define R_EP_U2MC_BULK_FIFO_CTL   0x08
#define R_EP_U2MC_BULK_FIFO_BC    0x0C
#define R_EP_U2MC_BULK_FIFO_MODE  0x14
#define R_EP_U2MC_BULK_DMA_CTRL   0x20
#define R_EP_U2MC_BULK_DMA_LENGTH 0x24
#define R_EP_U2MC_BULK_DMA_ADDR   0x28

#define R_EP_U2MC_INIIN_CTL         0x00
#define R_EP_U2MC_INTIN_BC_OFFSET   0x04
#define R_EP_U2MC_INTIN_BUF0_OFFSET 0x08

/* Bits of R_EP_U2SIE_CTL (0x0) */
#define EP_EN_OFFSET 0
#define EP_EN        BIT(0)

#define EP_STALL_OFFSET 1
#define EP_STALL        BIT(1)

#define EP_RESET_OFFSET 2
#define EP_RESET        BIT(2)

#define EP_FORCE_TOGGLE_OFFSET 3
#define EP_FORCE_TOGGLE        BIT(3)

#define EP_NAKOUT_MODE_OFFSET 4
#define EP_NAKOUT_MODE        BIT(4)

#define EP_EPNUM_OFFSET 8
#define EP_EPNUM_MASK   GENMASK(11, 8)

#define EP_MAXPKT_OFFSET 16
#define EP_MAXPKT_MASK   GENMASK(25, 16)

/* Bits of R_EP_U2SIE_IRQ_EN (0x04) and R_EP_U2SIE_IRQ_STATUS (0x08)*/
#define USB_EPA_INT_MASK (BIT(0) | BIT(1) | BIT(2))
#define USB_EPB_INT_MASK (BIT(0) | BIT(1))
#define USB_EPC_INT_MASK (BIT(0) | BIT(1))
#define USB_EPD_INT_MASK (BIT(0) | BIT(1))
#define USB_EPE_INT_MASK (BIT(0) | BIT(1) | BIT(2))
#define USB_EPF_INT_MASK (BIT(0) | BIT(1))
#define USB_EPG_INT_MASK (BIT(0) | BIT(1))

#define USB_BULKOUT_DATAPKT_RECV_INT  BIT(1)
#define USB_BULKOUT_SHORTPKT_RECV_INT BIT(2)
#define USB_BULKIN_TRANS_END_INT      BIT(1)

#define USB_INTOUT_DATAPKT_RECV_INT BIT(1)
#define USB_INTIN_DATAPKT_TRANS_INT BIT(1)

/* Bits of R_EP_U2MC_BULK_IRQ (0x00) and R_EP_U2MC_BULK_EN (0x04) */
#define USB_EPA_MC_INT_MASK (BIT(0))
#define USB_EPB_MC_INT_MASK (BIT(0) | BIT(5))
#define USB_EPE_MC_INT_MASK (BIT(0))
#define USB_EPF_MC_INT_MASK (BIT(0) | BIT(5))

/* Bits of R_EP_U2MC_BULK_FIFO_CTL (0x08) */
#define EP_FIFO_FLUSH_OFFSET 0
#define EP_FIFO_FLUSH        BIT(0)

#define EP_FIFO_VALID_OFFSET 1
#define EP_FIFO_VALID        BIT(1)

#define EP_CFG_AUTO_FIFO_VALID_OFFSET 8
#define EP_CFG_AUTO_FIFO_VALID        BIT(8)

/* Bits of R_EP_U2MC_BULK_FIFO_MODE (0x14) */
#define EP_FIFO_EN_OFFSET 0
#define EP_FIFO_EN        BIT(0)

/* Bits of R_EP_U2MC_BULK_DMA_CTRL (0x20) */
#define EP_U_PE_TRANS_EN_OFFSET 0
#define EP_U_PE_TRANS_EN        BIT(0)

#define EP_U_PE_TRANS_DIR_OFFSET 1
#define EP_U_PE_TRANS_DIR        BIT(1)

/* Bits of R_EP_U2MC_BULK_DMA_LENGTH (0x24) */
#define EP_U_PE_TRANS_LEN_OFFSET 0
#define EP_U_PE_TRANS_LEN_MASK   GENMASK(15, 0)

/* Bits of R_EP_U2MC_INIIN_CTL (0x00) */
#define EP_U_INT_BUF_EN_OFFSET 0
#define EP_U_INT_BUF_EN        BIT(0)

/* Bits of R_EP_U2MC_INTIN_BC_OFFSET (0x04) */
#define EP_U_INT_BUF_TX_BC_OFFSET 0
#define EP_U_INT_BUF_TX_BC_MASK   GENMASK(7, 0)

/* Bits of R_EPG_U2SIE_INTOUT_MC (0x0C) */
#define EPG_EP_EP_OUT_DATA_DONE_OFFSET 0
#define EPG_EP_EP_OUT_DATA_DONE        BIT(0)

/* Parameters */
#define EP_DMA_DIR_BULK_OUT 0x00
#define EP_DMA_DIR_BULK_IN  0x01

/* Usb endpoint FIFO depth, only EPA/EPB/EPE/EPF have FIFO */
#define USB_EPA_FIFO_DEPTH 1024
#define USB_EPB_FIFO_DEPTH 1024
#define USB_EPE_FIFO_DEPTH 1024
#define USB_EPF_FIFO_DEPTH 1024

/* Usb endpoint logic number config */
#define USB_EPA_DIR USB_EP_DIR_OUT
#define USB_EPB_DIR USB_EP_DIR_IN
#define USB_EPC_DIR USB_EP_DIR_IN
#define USB_EPD_DIR USB_EP_DIR_IN
#define USB_EPE_DIR USB_EP_DIR_OUT
#define USB_EPF_DIR USB_EP_DIR_IN
#define USB_EPG_DIR USB_EP_DIR_OUT

/* Memory Control */
#define EP_MC_INT_DMA_DONE BIT(0)
#define EP_MC_INT_SIE_DONE BIT(5)

/* Usb endpoint define */
#define USB_EP0_MPS        0x40
#define USB_BULK_MPS_HS    0x0200
#define USB_BULK_MPS_FS    0x0040
#define USB_INT_EPG_MPS    0x80
#define USB_INT_EPC_MPS_HS 0x80
#define USB_INT_EPC_MPS_FS 0x40
#define USB_INT_EPD_MPS    0x40

#define USB_EP_DATA_ID_DATA0 0
#define USB_EP_DATA_ID_DATA1 1

#endif /* ZEPHYR_DRIVERS_USB_UDC_RTS5817_H */
