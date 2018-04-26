/* ieee802154_cc1101_regs.h - Registers definition for TI CC1101 */

/*
 * Copyright (c) 2018 Matthias Boesl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __IEEE802154_CC1101_REGS_H__
#define __IEEE802154_CC1101_REGS_H__

/* Access types */

#define CC1101_ACCESS_RD                        BIT(7)
#define CC1101_ACCESS_WR                        (0x00)
#define CC1101_ACCESS_BURST                     BIT(6)

/* Configuration registers */

#define CC1101_REG_IOCFG2                       (0x00)
#define GPIO2_ATRAN                             BIT(7)
#define GPIO2_INV                               BIT(6)
#define GPIO2_CFG(_cfg_)                        (_cfg_ & 0x3F)

#define CC1101_REG_IOCFG1                       (0x01)
#define GPIO1_ATRAN                             BIT(7)
#define GPIO1_INV                               BIT(6)
#define GPIO1_CFG(_cfg_)                        (_cfg_ & 0x3F)

#define CC1101_REG_IOCFG0                       (0x02)
#define GPIO0_ATRAN                             BIT(7)
#define GPIO0_INV                               BIT(6)
#define GPIO0_CFG(_cfg_)                        (_cfg_ & 0x3F)

#define CC1101_REG_FIFOTHR          (0x03)
#define CC1101_REG_SYNC1                        (0x04)
#define CC1101_REG_SYNC0                        (0x05)

#define CC1101_REG_PKTLEN                       (0x06)
#define CC1101_REG_PKT_CTRL1            (0x07)
#define CC1101_REG_PKT_CTRL0            (0x08)

#define CC1101_REG_DEV_ADDR             (0x09)

#define CC1101_REG_CHANNEL                      (0x0A)

#define CC1101_REG_FSCTRL1                      (0x0B)
#define CC1101_REG_FSCTRL0                      (0x0C)

#define CC1101_REG_FREQ2                        (0x0D)
#define CC1101_REG_FREQ1                        (0x0E)
#define CC1101_REG_FREQ0                        (0x0F)

#define CC1101_REG_MDMCFG4                      (0x10)
#define CC1101_REG_MDMCFG3                      (0x11)
#define CC1101_REG_MDMCFG2                      (0x12)
#define CC1101_REG_MDMCFG1                      (0x13)
#define CC1101_REG_MDMCFG0                      (0x14)

#define CC1101_REG_DEVIATN          (0x15)
#define CC1101_REG_MCSM2            (0x16)
#define CC1101_REG_MCSM1            (0x17)
#define CC1101_REG_MCSM0            (0x18)
#define CC1101_REG_FOCCFG           (0x19)
#define CC1101_REG_BSCFG            (0x1A)
#define CC1101_REG_AGCCTRL2         (0x1B)
#define CC1101_REG_AGCCTRL1         (0x1C)
#define CC1101_REG_AGCCTRL0         (0x1D)
#define CC1101_REG_WOREVT1          (0x1E)
#define CC1101_REG_WOREVT0          (0x1F)
#define CC1101_REG_WORCTRL          (0x20)
#define CC1101_REG_FREND1           (0x21)
#define CC1101_REG_FREND0           (0x22)
#define CC1101_REG_FSCAL3           (0x23)
#define CC1101_REG_FSCAL2           (0x24)
#define CC1101_REG_FSCAL1           (0x25)
#define CC1101_REG_FSCAL0           (0x26)
#define CC1101_REG_RCCTRL1          (0x27)
#define CC1101_REG_RCCTRL0          (0x28)
#define CC1101_REG_FSTEST           (0x29)
#define CC1101_REG_PTEST            (0x2A)
#define CC1101_REG_AGCTEST          (0x2B)
#define CC1101_REG_TEST2            (0x2C)
#define CC1101_REG_TEST1            (0x2D)
#define CC1101_REG_TEST0            (0x2E)

/* Command strobes */
#define CC1101_INS_SRES                         (0x30)
#define CC1101_INS_SFSTXON                      (0x31)
#define CC1101_INS_SXOFF                        (0x32)
#define CC1101_INS_SCAL                         (0x33)
#define CC1101_INS_SRX                          (0x34)
#define CC1101_INS_STX                          (0x35)
#define CC1101_INS_SIDLE                        (0x36)
#define CC1101_INS_SAFC                         (0x37)
#define CC1101_INS_SWOR                         (0x38)
#define CC1101_INS_SPWD                         (0x39)
#define CC1101_INS_SFRX                         (0x3A)
#define CC1101_INS_SFTX                         (0x3B)
#define CC1101_INS_SWORRST                      (0x3C)
#define CC1101_INS_SNOP                         (0x3D)

/* Status registers accessed with burst access */
#define CC1101_REG_TXBYTES                      (0x3A)
#define CC1101_REG_RXBYTES                      (0x3B)

/* PA Table */
#define CC1101_REG_PATABLE                      (0x3E)

/* FIFO access */
#define CC1101_REG_TXFIFO                       (0x3F)
#define CC1101_REG_RXFIFO                       (0x3F)


#define CC1101_REG_MARCSTATE        (0x35)

#define CC1101_REG_PKTSTATUS        (0x38)
#define     CHANNEL_IS_CLEAR        (1 << 4)


/* GDO PIN config settings */
#define CC1101_GPIO_SIG_RXFIFO_THR                  (0x00)
#define CC1101_GPIO_SIG_RXFIFO_THR_PKT          (0x01)
#define CC1101_GPIO_SIG_TXFIFO_THR                  (0x02)
#define CC1101_GPIO_SIG_TXFIFO_THR_PKT          (0x03)
#define CC1101_GPIO_SIG_RXFIFO_OVERFLOW         (0x04)
#define CC1101_GPIO_SIG_TXFIFO_UNDERFLOW        (0x05)
#define CC1101_GPIO_SIG_PKT_SYNC_RXTX           (0x06)
/* ... */
#define CC1101_GPIO_SIG_CRC_OK                      (0x0F)
/* ... */
#define CC1101_GPIO_SIG_CHIP_RDY                    (0x29)
#define CC1101_GPIO_SIG_HIGH_IMPEDANCE      (0x2E)

/* MARCSTATE (0x35) Register Content */
/* Main Radio Control State Machine State */
#define CC1101_STATUS_MASK              (0x1F)
#define CC1101_STATUS_SLEEP             (0x00)
#define CC1101_STATUS_IDLE                          (0x01)
#define CC1101_STATUS_XOFF                          (0x02)
#define CC1101_STATUS_VCOON_MC                  (0x03)
#define CC1101_STATUS_REGON_MC                  (0x04)
#define CC1101_STATUS_MANCAL                    (0x05)
#define CC1101_STATUS_VCOON                         (0x06)
#define CC1101_STATUS_REGON                     (0x07)
#define CC1101_STATUS_STARTCAL                  (0x08)
#define CC1101_STATUS_BWBOOST                   (0x09)
#define CC1101_STATUS_FS_LOCK           (0x0A)
#define CC1101_STATUS_IFADCON           (0x0B)
#define CC1101_STATUS_ENDCAL            (0x0C)
#define CC1101_STATUS_RX                            (0x0D)
#define CC1101_STATUS_RX_END            (0x0E)
#define CC1101_STATUS_RX_RST            (0x0F)
#define CC1101_STATUS_TXRX_SWITCH       (0x10)
#define CC1101_STATUS_RXFIFO_OVERFLOW   (0x11)
#define CC1101_STATUS_FSTXON            (0x12)
#define CC1101_STATUS_TX                            (0x13)
#define CC1101_STATUS_TX_END            (0x14)
#define CC1101_STATUS_RXTX_SWITCH       (0x15)
#define CC1101_STATUS_TXFIFO_UNDERFLOW  (0x16)
/* SW define */
#define CC1101_STATUS_CHIP_NOT_READY    (0x17)

/* PA Table (0x3E) Register Content */
/* PA table content is lost after entering sleep */
#ifdef CC1101_MULTILAYER_INDUCTOR_ANTENNA
#define CC1101_PA_MINUS_30              (0x12)
#define CC1101_PA_MINUS_20              (0x0D)
#define CC1101_PA_MINUS_15              (0x1C)
#define CC1101_PA_MINUS_10              (0x34)
#define CC1101_PA_MINUS_6               (0x40)
#define CC1101_PA_0                     (0x51)
#define CC1101_PA_5                     (0x85)
#define CC1101_PA_7                     (0xCB)
#define CC1101_PA_10                    (0xC2)
#define CC1101_PA_11                    (0xC0)
#else
#define CC1101_PA_MINUS_30              (0x30)
#define CC1101_PA_MINUS_20              (0x17)
#define CC1101_PA_MINUS_15              (0x1D)
#define CC1101_PA_MINUS_10              (0x26)
#define CC1101_PA_MINUS_6               (0x37)
#define CC1101_PA_0                     (0x50)
#define CC1101_PA_5                     (0x86)
#define CC1101_PA_7                     (0xCD)
#define CC1101_PA_10                    (0xC5)
#define CC1101_PA_11                    (0xC0)
#endif /* CC1101_CHIP_ANTENNA */

/* Appended FCS - See Section 8 */
#define CC1101_FCS_LEN                          (2)
#define CC1101_FCS_CRC_OK                       BIT(7)
#define CC1101_FCS_LQI_MASK                     (0x7F)

/* ToDo: supporting 802.15.4g will make this header of a different size */
#define CC1101_PHY_HDR_LEN                      (1)

#endif /* __IEEE802154_CC1101_REGS_H__ */
