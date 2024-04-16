/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_DEVICE_USB_DW_REGISTERS_H_
#define ZEPHYR_DRIVERS_USB_DEVICE_USB_DW_REGISTERS_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * This file describes register set for the DesignWare USB 2.0 controller IP,
 * other known names are OTG_FS, OTG_HS.
 */

/* USB IN EP Register block type */
struct usb_dw_in_ep_reg {
	volatile uint32_t diepctl;
	uint32_t reserved;
	volatile uint32_t diepint;
	uint32_t reserved1;
	volatile uint32_t dieptsiz;
	volatile uint32_t diepdma;
	volatile uint32_t dtxfsts;
	uint32_t reserved2;
};

/* USB OUT EP Register block type */
struct usb_dw_out_ep_reg {
	volatile uint32_t doepctl;
	uint32_t reserved;
	volatile uint32_t doepint;
	uint32_t reserved1;
	volatile uint32_t doeptsiz;
	volatile uint32_t doepdma;
	uint32_t reserved2;
	uint32_t reserved3;
};

/* USB Register block type */
struct usb_dw_reg {
	volatile uint32_t gotgctl;
	volatile uint32_t gotgint;
	volatile uint32_t gahbcfg;
	volatile uint32_t gusbcfg;
	volatile uint32_t grstctl;
	volatile uint32_t gintsts;
	volatile uint32_t gintmsk;
	volatile uint32_t grxstsr;
	volatile uint32_t grxstsp;
	volatile uint32_t grxfsiz;
	volatile uint32_t gnptxfsiz;
	uint32_t reserved[3];
	volatile uint32_t ggpio;
	volatile uint32_t guid;
	volatile uint32_t gsnpsid;
	volatile uint32_t ghwcfg1;
	volatile uint32_t ghwcfg2;
	volatile uint32_t ghwcfg3;
	volatile uint32_t ghwcfg4;
	volatile uint32_t gdfifocfg;
	uint32_t reserved1[43];
	volatile uint32_t dieptxf1;
	volatile uint32_t dieptxf2;
	volatile uint32_t dieptxf3;
	volatile uint32_t dieptxf4;
	volatile uint32_t dieptxf5;
	/* Host mode register 0x0400 .. 0x0670 */
	uint32_t reserved2[442];
	/* Device mode register 0x0800 .. 0x0D00 */
	volatile uint32_t dcfg;
	volatile uint32_t dctl;
	volatile uint32_t dsts;
	uint32_t reserved3;
	volatile uint32_t diepmsk;
	volatile uint32_t doepmsk;
	volatile uint32_t daint;
	volatile uint32_t daintmsk;
	uint32_t reserved4[2];
	volatile uint32_t dvbusdis;
	volatile uint32_t dvbuspulse;
	volatile uint32_t dthrctl;
	volatile uint32_t diepempmsk;
	uint32_t reserved5[50];
	struct usb_dw_in_ep_reg in_ep_reg[16];
	struct usb_dw_out_ep_reg out_ep_reg[16];
};

/*
 * With the maximum number of supported endpoints, register set
 * of the controller can occupy the region up to 0x0D00.
 */
BUILD_ASSERT(sizeof(struct usb_dw_reg) <= 0x0D00);

/* AHB configuration register, offset: 0x0008 */
#define USB_DW_GAHBCFG_DMA_EN			BIT(5)
#define USB_DW_GAHBCFG_GLB_INTR_MASK		BIT(0)

/* USB configuration register, offset: 0x000C */
#define USB_DW_GUSBCFG_FORCEDEVMODE		BIT(30)
#define USB_DW_GUSBCFG_FORCEHSTMODE		BIT(29)
#define USB_DW_GUSBCFG_PHY_IF_MASK		BIT(3)
#define USB_DW_GUSBCFG_PHY_IF_8_BIT		0
#define USB_DW_GUSBCFG_PHY_IF_16_BIT		BIT(3)

/* Reset register, offset: 0x0010 */
#define USB_DW_GRSTCTL_AHB_IDLE			BIT(31)
#define USB_DW_GRSTCTL_TX_FNUM_OFFSET		6
#define USB_DW_GRSTCTL_TX_FFLSH			BIT(5)
#define USB_DW_GRSTCTL_C_SFT_RST		BIT(0)

/* Core interrupt register, offset: 0x0014 */
#define USB_DW_GINTSTS_WK_UP_INT		BIT(31)
#define USB_DW_GINTSTS_OEP_INT			BIT(19)
#define USB_DW_GINTSTS_IEP_INT			BIT(18)
#define USB_DW_GINTSTS_ENUM_DONE		BIT(13)
#define USB_DW_GINTSTS_USB_RST			BIT(12)
#define USB_DW_GINTSTS_USB_SUSP			BIT(11)
#define USB_DW_GINTSTS_RX_FLVL			BIT(4)
#define USB_DW_GINTSTS_OTG_INT			BIT(2)

/* Status read and pop registers (device mode), offset: 0x001C 0x0020 */
#define USB_DW_GRXSTSR_PKT_STS_MASK		(0xF << 17)
#define USB_DW_GRXSTSR_PKT_STS_OFFSET		17
#define USB_DW_GRXSTSR_PKT_STS_OUT_DATA		2
#define USB_DW_GRXSTSR_PKT_STS_OUT_DATA_DONE	3
#define USB_DW_GRXSTSR_PKT_STS_SETUP_DONE	4
#define USB_DW_GRXSTSR_PKT_STS_SETUP		6
#define USB_DW_GRXSTSR_PKT_CNT_MASK		(0x7FF << 4)
#define USB_DW_GRXSTSR_PKT_CNT_OFFSET		4
#define USB_DW_GRXSTSR_EP_NUM_MASK		(0xF << 0)

/* Application (vendor) general purpose registers, offset: 0x0038 */
#define USB_DW_GGPIO_STM32_VBDEN		BIT(21)
#define USB_DW_GGPIO_STM32_PWRDWN		BIT(16)

/* GHWCFG1 register, offset: 0x0044 */
#define USB_DW_GHWCFG1_EPDIR_MASK(i)		(0x3 << (i * 2))
#define USB_DW_GHWCFG1_EPDIR_SHIFT(i)		(i * 2)
#define USB_DW_GHWCFG1_OUTENDPT			2
#define USB_DW_GHWCFG1_INENDPT			1
#define USB_DW_GHWCFG1_BDIR			0

/* GHWCFG2 register, offset: 0x0048 */
#define USB_DW_GHWCFG2_NUMDEVEPS_MASK		(0xF << 10)
#define USB_DW_GHWCFG2_NUMDEVEPS_SHIFT		10
#define USB_DW_GHWCFG2_FSPHYTYPE_MASK		(0x3 << 8)
#define USB_DW_GHWCFG2_FSPHYTYPE_SHIFT		8
#define USB_DW_GHWCFG2_FSPHYTYPE_FSPLUSULPI	3
#define USB_DW_GHWCFG2_FSPHYTYPE_FSPLUSUTMI	2
#define USB_DW_GHWCFG2_FSPHYTYPE_FS		1
#define USB_DW_GHWCFG2_FSPHYTYPE_NO_FS		0
#define USB_DW_GHWCFG2_HSPHYTYPE_MASK		(0x3 << 6)
#define USB_DW_GHWCFG2_HSPHYTYPE_SHIFT		6
#define USB_DW_GHWCFG2_HSPHYTYPE_UTMIPLUSULPI	3
#define USB_DW_GHWCFG2_HSPHYTYPE_ULPI		2
#define USB_DW_GHWCFG2_HSPHYTYPE_UTMIPLUS	1
#define USB_DW_GHWCFG2_HSPHYTYPE_NO_HS		0

/* GHWCFG3 register, offset: 0x004C */
#define USB_DW_GHWCFG3_DFIFODEPTH_MASK		(0xFFFFU << 16)
#define USB_DW_GHWCFG3_DFIFODEPTH_SHIFT		16

/* GHWCFG4 register, offset: 0x0050 */
#define USB_DW_GHWCFG4_INEPS_MASK		(0xF << 26)
#define USB_DW_GHWCFG4_INEPS_SHIFT		26
#define USB_DW_GHWCFG4_DEDFIFOMODE		BIT(25)

/* Device configuration registers, offset: 0x0800 */
#define USB_DW_DCFG_DEV_ADDR_MASK		(0x7F << 4)
#define USB_DW_DCFG_DEV_ADDR_OFFSET		4
#define USB_DW_DCFG_DEV_SPD_USB2_HS		0
#define USB_DW_DCFG_DEV_SPD_USB2_FS		1
#define USB_DW_DCFG_DEV_SPD_LS			2
#define USB_DW_DCFG_DEV_SPD_FS			3

/* Device control register, offset 0x0804 */
#define USB_DW_DCTL_SFT_DISCON			BIT(1)

/* Device status register, offset 0x0808 */
#define USB_DW_DSTS_ENUM_SPD_MASK		(0x3 << 1)
#define USB_DW_DSTS_ENUM_SPD_OFFSET		1
#define USB_DW_DSTS_ENUM_LS			2
#define USB_DW_DSTS_ENUM_FS			3

/* Device all endpoints interrupt register, offset 0x0818 */
#define USB_DW_DAINT_OUT_EP_INT(ep)		(0x10000 << (ep))
#define USB_DW_DAINT_IN_EP_INT(ep)		(1 << (ep))

/*
 * Device IN/OUT endpoint control register
 * IN endpoint offsets 0x0900 + (0x20 * n), n = 0 .. x,
 * offset 0x0900 and 0x0B00 are hardcoded to control type.
 *
 * REVISE: Better own definitions for DIEPTCTL0, DOEPTCTL0...
 */
#define USB_DW_DEPCTL_EP_ENA			BIT(31)
#define USB_DW_DEPCTL_EP_DIS			BIT(30)
#define USB_DW_DEPCTL_SETDOPID			BIT(28)
#define USB_DW_DEPCTL_SNAK			BIT(27)
#define USB_DW_DEPCTL_CNAK			BIT(26)
#define USB_DW_DEPCTL_STALL			BIT(21)
#define USB_DW_DEPCTL_TXFNUM_OFFSET		22
#define USB_DW_DEPCTL_TXFNUM_MASK		(0xF << 22)
#define USB_DW_DEPCTL_EP_TYPE_MASK		(0x3 << 18)
#define USB_DW_DEPCTL_EP_TYPE_OFFSET		18
#define USB_DW_DEPCTL_EP_TYPE_INTERRUPT		3
#define USB_DW_DEPCTL_EP_TYPE_BULK		2
#define USB_DW_DEPCTL_EP_TYPE_ISO		1
#define USB_DW_DEPCTL_EP_TYPE_CONTROL		0
#define USB_DW_DEPCTL_USB_ACT_EP		BIT(15)
#define USB_DW_DEPCTL0_MSP_MASK			0x3
#define USB_DW_DEPCTL0_MSP_8			3
#define USB_DW_DEPCTL0_MSP_16			2
#define USB_DW_DEPCTL0_MSP_32			1
#define USB_DW_DEPCTL0_MSP_64			0
#define USB_DW_DEPCTLn_MSP_MASK			0x3FF
#define USB_DW_DEPCTL_MSP_OFFSET		0

/*
 * Device IN endpoint interrupt register
 * offsets 0x0908 + (0x20 * n), n = 0 .. x
 */
#define USB_DW_DIEPINT_TX_FEMP			BIT(7)
#define USB_DW_DIEPINT_XFER_COMPL		BIT(0)

/*
 * Device OUT endpoint interrupt register
 * offsets 0x0B08 + (0x20 * n), n = 0 .. x
 */
#define USB_DW_DOEPINT_SET_UP			BIT(3)
#define USB_DW_DOEPINT_XFER_COMPL		BIT(0)

/*
 * Device IN/OUT endpoint transfer size register
 * IN at offsets 0x0910 + (0x20 * n), n = 0 .. x,
 * OUT at offsets 0x0B10 + (0x20 * n), n = 0 .. x
 *
 * REVISE: Better own definitions for DIEPTSIZ0, DOEPTSIZ0...
 */
#define USB_DW_DEPTSIZ_PKT_CNT_OFFSET		19
#define USB_DW_DIEPTSIZ0_PKT_CNT_MASK		(0x3 << 19)
#define USB_DW_DIEPTSIZn_PKT_CNT_MASK		(0x3FF << 19)
#define USB_DW_DOEPTSIZn_PKT_CNT_MASK		(0x3FF << 19)
#define USB_DW_DOEPTSIZ0_PKT_CNT_MASK		(0x1 << 19)
#define USB_DW_DOEPTSIZ_SUP_CNT_OFFSET		29
#define USB_DW_DOEPTSIZ_SUP_CNT_MASK		(0x3 << 29)
#define USB_DW_DEPTSIZ_XFER_SIZE_OFFSET		0
#define USB_DW_DEPTSIZ0_XFER_SIZE_MASK		0x7F
#define USB_DW_DEPTSIZn_XFER_SIZE_MASK		0x7FFFF

/*
 * Device IN endpoint transmit FIFO status register,
 * offsets 0x0918 + (0x20 * n), n = 0 .. x
 */
#define USB_DW_DTXFSTS_TXF_SPC_AVAIL_MASK	0xFFFF

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_USB_DEVICE_USB_DW_REGISTERS_H_ */
