/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * DWC3 (DesignWare USB3 DRD) register definitions.
 * Derived from xHCI drivers/usb/dwc3/core.h
 */

#ifndef ZEPHYR_USB_DWC3_REGS_H
#define ZEPHYR_USB_DWC3_REGS_H

#include <zephyr/sys/util.h>

/* Global register offsets (from DWC3 base) */
#define DWC3_GSBUSCFG0  0xc100
#define DWC3_GSBUSCFG1  0xc104
#define DWC3_GTXTHRCFG  0xc108
#define DWC3_GRXTHRCFG  0xc10c
#define DWC3_GCTL       0xc110
/* OTG control: clear PERIMODE for host-only / host mode selection (BURST flow) */
#define DWC3_OCTL       0xcc04
#define DWC3_GEVTEN     0xc114
#define DWC3_GSTS       0xc118
#define DWC3_GUCTL1     0xc11c
#define DWC3_GSNPSID    0xc120
#define DWC3_GUCTL      0xc12c
#define DWC3_GHWPARAMS0 0xc140
#define DWC3_GHWPARAMS1 0xc144
#define DWC3_GHWPARAMS2 0xc148
#define DWC3_GHWPARAMS3 0xc14c
#define DWC3_GHWPARAMS4 0xc150
#define DWC3_GHWPARAMS5 0xc154
#define DWC3_GHWPARAMS6 0xc158
#define DWC3_GHWPARAMS7 0xc15c
#define DWC3_GHWPARAMS8 0xc600
#define DWC3_GUCTL3     0xc60c
#define DWC3_GFLADJ     0xc630

/* GDBG block (Versal integrated DWC3+xHCI; XRegDB usb_xhci @ 0xFE200000) */
#define DWC3_GDBGFIFOSPACE  0xc160
#define DWC3_GDBGLTSSM      0xc164
#define DWC3_GDBGBMU        0xc16c
#define DWC3_GDBGLSPMUX_HST 0xc170
#define DWC3_GDBGLSP        0xc174
#define DWC3_GDBGEPINFO0    0xc178
#define DWC3_GDBGEPINFO1    0xc17c
#define DWC3_GUCTL2         0xc19c

#define DWC3_GDBGFIFOSPACE_FIFO_QUEUE_SELECT(n) ((n) & 0x1ffU)
#define DWC3_GDBGFIFOSPACE_SPACE_AVAILABLE(r)   (((r) >> 16) & 0xffffU)
#define DWC3_GDBGLSPMUX_HOSTSELECT(n)           ((n) & 0x3fffU)
#define DWC3_GUCTL_USBHSTINAUTORETRYEN          BIT(14)

#define DWC3_DSTS_RXFIFOEMPTY BIT(17)
#define DWC3_DSTS_COREIDLE    BIT(23)
#define DWC3_DSTS_USBLNKST(r) (((r) >> 18) & 0xfU)

#define DWC3_GUSB2PHYCFG(n)  (0xc200 + ((n) * 0x04))
#define DWC3_GUSB3PIPECTL(n) (0xc2c0 + ((n) * 0x04))
#define DWC3_GTXFIFOSIZ(n)   (0xc300 + ((n) * 0x04))
#define DWC3_GRXFIFOSIZ(n)   (0xc380 + ((n) * 0x04))
#define DWC3_GEVNTADRLO(n)   (0xc400 + ((n) * 0x10))
#define DWC3_GEVNTADRHI(n)   (0xc404 + ((n) * 0x10))
#define DWC3_GEVNTSIZ(n)     (0xc408 + ((n) * 0x10))
#define DWC3_GEVNTCOUNT(n)   (0xc40c + ((n) * 0x10))

/* Device registers */
#define DWC3_DCFG   0xc700
#define DWC3_DCTL   0xc704
#define DWC3_DEVTEN 0xc708
#define DWC3_DSTS   0xc70c

/* GSBUSCFG0 */
#define DWC3_GSBUSCFG0_INCR256BRSTENA BIT(7)
#define DWC3_GSBUSCFG0_INCR128BRSTENA BIT(6)
#define DWC3_GSBUSCFG0_INCR64BRSTENA  BIT(5)
#define DWC3_GSBUSCFG0_INCR32BRSTENA  BIT(4)
#define DWC3_GSBUSCFG0_INCR16BRSTENA  BIT(3)
#define DWC3_GSBUSCFG0_INCR8BRSTENA   BIT(2)
#define DWC3_GSBUSCFG0_INCR4BRSTENA   BIT(1)
#define DWC3_GSBUSCFG0_INCRBRSTENA    BIT(0)
#define DWC3_GSBUSCFG0_INCRBRST_MASK  GENMASK(7, 0)

/* GCTL */
#define DWC3_GCTL_PWRDNSCALE(n)    ((n) << 19)
#define DWC3_GCTL_PWRDNSCALE_MASK  GENMASK(31, 19)
#define DWC3_GCTL_U2RSTECN         BIT(16)
#define DWC3_GCTL_PRTCAPDIR(n)     ((n) << 12)
#define DWC3_GCTL_PRTCAP_MASK      GENMASK(13, 12)
#define DWC3_GCTL_PRTCAP_HOST      1
#define DWC3_GCTL_PRTCAP_DEVICE    2
#define DWC3_GCTL_PRTCAP_OTG       3
#define DWC3_GCTL_CORESOFTRESET    BIT(11)
#define DWC3_GCTL_SOFITPSYNC       BIT(10)
#define DWC3_GCTL_SCALEDOWN(n)     ((n) << 4)
#define DWC3_GCTL_SCALEDOWN_MASK   DWC3_GCTL_SCALEDOWN(3)
#define DWC3_GCTL_DISSCRAMBLE      BIT(3)
#define DWC3_GCTL_U2EXIT_LFPS      BIT(2)
#define DWC3_GCTL_GBLHIBERNATIONEN BIT(1)
#define DWC3_GCTL_DSBLCLKGTNG      BIT(0)

/* GUCTL1 — xHCI drivers/usb/dwc3/core.c dwc3_core_init (host-relevant bits) */
#define DWC3_GUCTL1_DEV_DECOUPLE_L1L2_EVT BIT(31)
#define DWC3_GUCTL1_DEV_L1_EXIT_BY_HW     BIT(24)
#define DWC3_GUCTL1_RESUME_OPMODE_HS_HOST BIT(10)

/* OCTL */
#define DWC3_OCTL_PERIMODE BIT(6)

/* GUSB2PHYCFG */
#define DWC3_GUSB2PHYCFG_PHYSOFTRST     BIT(31)
#define DWC3_GUSB2PHYCFG_USBTRDTIM(n)   ((n) << 10)
#define DWC3_GUSB2PHYCFG_USBTRDTIM_MASK GENMASK(13, 10)
#define DWC3_GUSB2PHYCFG_ENBLSLPM       BIT(8)
#define DWC3_GUSB2PHYCFG_SUSPHY         BIT(6)
#define DWC3_GUSB2PHYCFG_PHYIF(n)       ((n) << 3)
#define DWC3_GUSB2PHYCFG_ULPI_UTMI      BIT(4)
#define USBTRDTIM_UTMI_8_BIT            9
#define USBTRDTIM_UTMI_16_BIT           5

/* GUSB3PIPECTL */
#define DWC3_GUSB3PIPECTL_PHYSOFTRST   BIT(31)
#define DWC3_GUSB3PIPECTL_U2SSINP3OK   BIT(29)
#define DWC3_GUSB3PIPECTL_DISRXDETINP3 BIT(28)
#define DWC3_GUSB3PIPECTL_UX_EXIT_PX   BIT(27)
#define DWC3_GUSB3PIPECTL_REQP1P2P3    BIT(24)
#define DWC3_GUSB3PIPECTL_SUSPHY       BIT(17)
#define DWC3_GUSB3PIPECTL_LFPSFILT     BIT(9)
#define DWC3_GUSB3PIPECTL_RX_DETOPOLL  BIT(8)

/* GEVNTSIZ */
#define DWC3_GEVNTSIZ_INTMASK BIT(31)
#define DWC3_GEVNTSIZ_SIZE(n) ((n) & 0xffff)

/* GSTS */
#define DWC3_GSTS_CURMOD(n)   ((n) & 0x3)
#define DWC3_GSTS_CURMOD_HOST 1

/* GHWPARAMS field extraction */
#define DWC3_GHWPARAMS0_MODE(n)       ((n) & 0x3)
#define DWC3_GHWPARAMS0_MODE_HOST     1
#define DWC3_GHWPARAMS0_MODE_DRD      2
#define DWC3_GHWPARAMS1_EN_PWROPT(n)  (((n) >> 24) & 0x3)
#define DWC3_GHWPARAMS1_EN_PWROPT_NO  0
#define DWC3_GHWPARAMS1_EN_PWROPT_CLK 1
#define DWC3_GHWPARAMS1_EN_PWROPT_HIB 2
#define DWC3_GHWPARAMS3_SSPHY_IFC(n)  ((n) & 3)
#define DWC3_GHWPARAMS3_HSPHY_IFC(n)  (((n) >> 2) & 3)

/* GFLADJ */
#define DWC3_GFLADJ_30MHZ_SDBND_SEL   BIT(7)
#define DWC3_GFLADJ_30MHZ_MASK        0x3f
#define DWC3_GFLADJ_REFCLK_FLADJ_MASK GENMASK(21, 8)
#define DWC3_GFLADJ_REFCLK_LPM_SEL    BIT(23)
#define DWC3_GFLADJ_WRITABLE_MASK                                                                  \
	(DWC3_GFLADJ_30MHZ_MASK | DWC3_GFLADJ_30MHZ_SDBND_SEL | DWC3_GFLADJ_REFCLK_FLADJ_MASK |    \
	 DWC3_GFLADJ_REFCLK_LPM_SEL)

/* DCTL */
#define DWC3_DCTL_RUN_STOP BIT(31)
#define DWC3_DCTL_CSFTRST  BIT(30)

#endif /* ZEPHYR_USB_DWC3_REGS_H */
