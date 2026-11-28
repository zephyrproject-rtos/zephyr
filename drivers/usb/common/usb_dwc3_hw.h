/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_COMMON_USB_DWC3_HW
#define ZEPHYR_DRIVERS_USB_COMMON_USB_DWC3_HW

#include <stdint.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Register set of the Synopsys DesignWare Cores SuperSpeed USB 3.x controller
 * (DWC_usb3, DWC_usb31 and DWC_usb32), global and device parts.
 *
 * Offsets are relative to the start of the controller register space, which
 * begins with the xHCI registers. The names after the USB_ prefix are those of
 * the Linux dwc3 driver, so that code and errata workarounds can be compared
 * with it directly.
 */

/* IP identifiers in GSNPSID[31:16] */
#define USB_DWC3_IP  0x5533
#define USB_DWC31_IP 0x3331
#define USB_DWC32_IP 0x3332
#define USB_DWC4_IP  0x3430

/* DWC_usb3 revisions, the full GSNPSID value */
#define USB_DWC3_REVISION_ANY  0x0
#define USB_DWC3_REVISION_173A 0x5533173a
#define USB_DWC3_REVISION_175A 0x5533175a
#define USB_DWC3_REVISION_180A 0x5533180a
#define USB_DWC3_REVISION_183A 0x5533183a
#define USB_DWC3_REVISION_185A 0x5533185a
#define USB_DWC3_REVISION_187A 0x5533187a
#define USB_DWC3_REVISION_188A 0x5533188a
#define USB_DWC3_REVISION_190A 0x5533190a
#define USB_DWC3_REVISION_194A 0x5533194a
#define USB_DWC3_REVISION_200A 0x5533200a
#define USB_DWC3_REVISION_202A 0x5533202a
#define USB_DWC3_REVISION_210A 0x5533210a
#define USB_DWC3_REVISION_220A 0x5533220a
#define USB_DWC3_REVISION_230A 0x5533230a
#define USB_DWC3_REVISION_240A 0x5533240a
#define USB_DWC3_REVISION_250A 0x5533250a
#define USB_DWC3_REVISION_260A 0x5533260a
#define USB_DWC3_REVISION_270A 0x5533270a
#define USB_DWC3_REVISION_280A 0x5533280a
#define USB_DWC3_REVISION_290A 0x5533290a
#define USB_DWC3_REVISION_300A 0x5533300a
#define USB_DWC3_REVISION_310A 0x5533310a
#define USB_DWC3_REVISION_320A 0x5533320a
#define USB_DWC3_REVISION_330A 0x5533330a

/* DWC_usb31 and DWC_usb32 revisions, the VER_NUMBER value */
#define USB_DWC31_REVISION_ANY  0x0
#define USB_DWC31_REVISION_110A 0x3131302a
#define USB_DWC31_REVISION_120A 0x3132302a
#define USB_DWC31_REVISION_160A 0x3136302a
#define USB_DWC31_REVISION_170A 0x3137302a
#define USB_DWC31_REVISION_180A 0x3138302a
#define USB_DWC31_REVISION_190A 0x3139302a
#define USB_DWC31_REVISION_200A 0x3230302a

#define USB_DWC32_REVISION_ANY  0x0
#define USB_DWC32_REVISION_100A 0x3130302a

/* DWC_usb31 version types, the VER_TYPE value */
#define USB_DWC31_VERSIONTYPE_ANY  0x0
#define USB_DWC31_VERSIONTYPE_EA01 0x65613031
#define USB_DWC31_VERSIONTYPE_EA02 0x65613032
#define USB_DWC31_VERSIONTYPE_EA03 0x65613033
#define USB_DWC31_VERSIONTYPE_EA04 0x65613034
#define USB_DWC31_VERSIONTYPE_EA05 0x65613035
#define USB_DWC31_VERSIONTYPE_EA06 0x65613036

/* Number of physical endpoints: EP0 OUT, EP0 IN, EP1 OUT, EP1 IN, ... */
#define USB_DWC3_ENDPOINTS_NUM 32

/* Register space boundaries */
#define USB_DWC3_XHCI_REGS_START    0x0
#define USB_DWC3_XHCI_REGS_END      0x7fff
#define USB_DWC3_GLOBALS_REGS_START 0xc100
#define USB_DWC3_GLOBALS_REGS_END   0xc6ff
#define USB_DWC3_DEVICE_REGS_START  0xc700
#define USB_DWC3_DEVICE_REGS_END    0xcbff

/* Global registers */
#define USB_DWC3_GSBUSCFG0    0xc100
#define USB_DWC3_GSBUSCFG1    0xc104
#define USB_DWC3_GTXTHRCFG    0xc108
#define USB_DWC3_GRXTHRCFG    0xc10c
#define USB_DWC3_GCTL         0xc110
#define USB_DWC3_GEVTEN       0xc114
#define USB_DWC3_GSTS         0xc118
#define USB_DWC3_GUCTL1       0xc11c
#define USB_DWC3_GSNPSID      0xc120
#define USB_DWC3_GGPIO        0xc124
#define USB_DWC3_GUID         0xc128
#define USB_DWC3_GUCTL        0xc12c
#define USB_DWC3_GBUSERRADDR0 0xc130
#define USB_DWC3_GBUSERRADDR1 0xc134
#define USB_DWC3_GHWPARAMS0   0xc140
#define USB_DWC3_GHWPARAMS1   0xc144
#define USB_DWC3_GHWPARAMS2   0xc148
#define USB_DWC3_GHWPARAMS3   0xc14c
#define USB_DWC3_GHWPARAMS4   0xc150
#define USB_DWC3_GHWPARAMS5   0xc154
#define USB_DWC3_GHWPARAMS6   0xc158
#define USB_DWC3_GHWPARAMS7   0xc15c
#define USB_DWC3_GDBGLTSSM    0xc164
#define USB_DWC3_GUCTL2       0xc19c
#define USB_DWC3_VER_NUMBER   0xc1a0
#define USB_DWC3_VER_TYPE     0xc1a4

#define USB_DWC3_GUSB2PHYCFG(n)  (0xc200 + ((n) * 0x04))
#define USB_DWC3_GUSB3PIPECTL(n) (0xc2c0 + ((n) * 0x04))
#define USB_DWC3_GTXFIFOSIZ(n)   (0xc300 + ((n) * 0x04))
#define USB_DWC3_GRXFIFOSIZ(n)   (0xc380 + ((n) * 0x04))

#define USB_DWC3_GEVNTADRLO(n) (0xc400 + ((n) * 0x10))
#define USB_DWC3_GEVNTADRHI(n) (0xc404 + ((n) * 0x10))
#define USB_DWC3_GEVNTSIZ(n)   (0xc408 + ((n) * 0x10))
#define USB_DWC3_GEVNTCOUNT(n) (0xc40c + ((n) * 0x10))

#define USB_DWC3_GHWPARAMS8 0xc600
#define USB_DWC3_GFLADJ     0xc630
#define USB_DWC3_GHWPARAMS9 0xc6e0

/* Device registers */
#define USB_DWC3_DCFG     0xc700
#define USB_DWC3_DCTL     0xc704
#define USB_DWC3_DEVTEN   0xc708
#define USB_DWC3_DSTS     0xc70c
#define USB_DWC3_DGCMDPAR 0xc710
#define USB_DWC3_DGCMD    0xc714
#define USB_DWC3_DALEPENA 0xc720
#define USB_DWC3_DCFG1    0xc740 /* DWC_usb32 only */

/* Physical endpoint command registers */
#define USB_DWC3_DEP_BASE(n)   (0xc800 + ((n) * 0x10))
#define USB_DWC3_DEPCMDPAR2(n) (USB_DWC3_DEP_BASE(n) + 0x00)
#define USB_DWC3_DEPCMDPAR1(n) (USB_DWC3_DEP_BASE(n) + 0x04)
#define USB_DWC3_DEPCMDPAR0(n) (USB_DWC3_DEP_BASE(n) + 0x08)
#define USB_DWC3_DEPCMD(n)     (USB_DWC3_DEP_BASE(n) + 0x0c)

/* Device interrupt moderation, one register per interrupter */
#define USB_DWC3_DEV_IMOD(n) (0xca00 + ((n) * 0x4))

/* GEVNTCOUNT */
#define USB_DWC3_GEVNTCOUNT_MASK 0xfffc
#define USB_DWC3_GEVNTCOUNT_EHB  BIT(31)

/* GSNPSID */
#define USB_DWC3_GSNPSID_MASK  0xffff0000
#define USB_DWC3_GSNPSREV_MASK 0xffff
#define USB_DWC3_GSNPS_ID(p)   (((p) & USB_DWC3_GSNPSID_MASK) >> 16)

/* GSBUSCFG0 */
#define USB_DWC3_GSBUSCFG0_INCR256BRSTENA      BIT(7)
#define USB_DWC3_GSBUSCFG0_INCR128BRSTENA      BIT(6)
#define USB_DWC3_GSBUSCFG0_INCR64BRSTENA       BIT(5)
#define USB_DWC3_GSBUSCFG0_INCR32BRSTENA       BIT(4)
#define USB_DWC3_GSBUSCFG0_INCR16BRSTENA       BIT(3)
#define USB_DWC3_GSBUSCFG0_INCR8BRSTENA        BIT(2)
#define USB_DWC3_GSBUSCFG0_INCR4BRSTENA        BIT(1)
#define USB_DWC3_GSBUSCFG0_INCRBRSTENA         BIT(0) /* undefined length INCR */
#define USB_DWC3_GSBUSCFG0_INCRBRST_MASK       0xff
#define USB_DWC3_GSBUSCFG0_REQINFO(n)          (((n) & 0xffff) << 16)
/* Software value meaning "leave GSBUSCFG0 request info untouched" */
#define USB_DWC3_GSBUSCFG0_REQINFO_UNSPECIFIED 0xffffffff

/* GRXTHRCFG, DWC_usb3 layout */
#define USB_DWC3_GRXTHRCFG_MAXRXBURSTSIZE(n) (((n) & 0x1f) << 19)
#define USB_DWC3_GRXTHRCFG_RXPKTCNT(n)       (((n) & 0xf) << 24)
#define USB_DWC3_GRXTHRCFG_PKTCNTSEL         BIT(29)

/* GTXTHRCFG, DWC_usb3 layout */
#define USB_DWC3_GTXTHRCFG_MAXTXBURSTSIZE(n) (((n) & 0xff) << 16)
#define USB_DWC3_GTXTHRCFG_TXPKTCNT(n)       (((n) & 0xf) << 24)
#define USB_DWC3_GTXTHRCFG_PKTCNTSEL         BIT(29)

/* GRXTHRCFG, DWC_usb31 and later layout */
#define USB_DWC31_GRXTHRCFG_MAXRXBURSTSIZE(n) (((n) & 0x1f) << 16)
#define USB_DWC31_GRXTHRCFG_RXPKTCNT(n)       (((n) & 0x1f) << 21)
#define USB_DWC31_GRXTHRCFG_PKTCNTSEL         BIT(26)

/* GTXTHRCFG, DWC_usb31 and later layout */
#define USB_DWC31_GTXTHRCFG_MAXTXBURSTSIZE(n) (((n) & 0x1f) << 16)
#define USB_DWC31_GTXTHRCFG_TXPKTCNT(n)       (((n) & 0x1f) << 21)
#define USB_DWC31_GTXTHRCFG_PKTCNTSEL         BIT(26)

/* GCTL */
#define USB_DWC3_GCTL_PWRDNSCALE(n)    ((n) << 19)
#define USB_DWC3_GCTL_PWRDNSCALE_MASK  GENMASK(31, 19)
#define USB_DWC3_GCTL_U2RSTECN         BIT(16)
#define USB_DWC3_GCTL_RAMCLKSEL(x)     (((x) & USB_DWC3_GCTL_CLK_MASK) << 6)
#define USB_DWC3_GCTL_CLK_BUS          0
#define USB_DWC3_GCTL_CLK_PIPE         1
#define USB_DWC3_GCTL_CLK_PIPEHALF     2
#define USB_DWC3_GCTL_CLK_MASK         3
#define USB_DWC3_GCTL_PRTCAP(n)        (((n) & (3 << 12)) >> 12)
#define USB_DWC3_GCTL_PRTCAPDIR(n)     ((n) << 12)
#define USB_DWC3_GCTL_PRTCAP_HOST      1
#define USB_DWC3_GCTL_PRTCAP_DEVICE    2
#define USB_DWC3_GCTL_PRTCAP_OTG       3
#define USB_DWC3_GCTL_CORESOFTRESET    BIT(11)
#define USB_DWC3_GCTL_SOFITPSYNC       BIT(10)
#define USB_DWC3_GCTL_SCALEDOWN(n)     ((n) << 4)
#define USB_DWC3_GCTL_SCALEDOWN_MASK   USB_DWC3_GCTL_SCALEDOWN(3)
#define USB_DWC3_GCTL_DISSCRAMBLE      BIT(3)
#define USB_DWC3_GCTL_U2EXIT_LFPS      BIT(2)
#define USB_DWC3_GCTL_GBLHIBERNATIONEN BIT(1)
#define USB_DWC3_GCTL_DSBLCLKGTNG      BIT(0)

/* GUCTL1 */
#define USB_DWC3_GUCTL1_DEV_DECOUPLE_L1L2_EVT       BIT(31)
#define USB_DWC3_GUCTL1_TX_IPGAP_LINECHECK_DIS      BIT(28)
#define USB_DWC3_GUCTL1_DEV_FORCE_20_CLK_FOR_30_CLK BIT(26)
#define USB_DWC3_GUCTL1_DEV_L1_EXIT_BY_HW           BIT(24)
#define USB_DWC3_GUCTL1_PARKMODE_DISABLE_SS         BIT(17)
#define USB_DWC3_GUCTL1_PARKMODE_DISABLE_HS         BIT(16)

/* GSTS */
#define USB_DWC3_GSTS_CSR_TIMEOUT      BIT(5)
#define USB_DWC3_GSTS_BUS_ERR_ADDR_VLD BIT(4)
#define USB_DWC3_GSTS_CURMOD(n)        ((n) & 0x3)
#define USB_DWC3_GSTS_CURMOD_DEVICE    0
#define USB_DWC3_GSTS_CURMOD_HOST      1

/* GUSB2PHYCFG */
#define USB_DWC3_GUSB2PHYCFG_PHYSOFTRST        BIT(31)
#define USB_DWC3_GUSB2PHYCFG_U2_FREECLK_EXISTS BIT(30)
#define USB_DWC3_GUSB2PHYCFG_ULPIEXTVBUSDRV    BIT(17)
#define USB_DWC3_GUSB2PHYCFG_USBTRDTIM(n)      ((n) << 10)
#define USB_DWC3_GUSB2PHYCFG_USBTRDTIM_MASK    USB_DWC3_GUSB2PHYCFG_USBTRDTIM(0xf)
#define USB_DWC3_GUSB2PHYCFG_XCVRDLY           BIT(9)
#define USB_DWC3_GUSB2PHYCFG_ENBLSLPM          BIT(8)
#define USB_DWC3_GUSB2PHYCFG_SUSPHY            BIT(6)
#define USB_DWC3_GUSB2PHYCFG_ULPI_UTMI         BIT(4)
#define USB_DWC3_GUSB2PHYCFG_PHYIF(n)          ((n) << 3)
#define USB_DWC3_GUSB2PHYCFG_PHYIF_MASK        USB_DWC3_GUSB2PHYCFG_PHYIF(1)

/* USB turnaround time and UTMI data width for GUSB2PHYCFG */
#define USB_DWC3_USBTRDTIM_UTMI_8_BIT  9
#define USB_DWC3_USBTRDTIM_UTMI_16_BIT 5
#define USB_DWC3_UTMI_PHYIF_16_BIT     1
#define USB_DWC3_UTMI_PHYIF_8_BIT      0

/* GUSB3PIPECTL */
#define USB_DWC3_GUSB3PIPECTL_PHYSOFTRST    BIT(31)
#define USB_DWC3_GUSB3PIPECTL_U2SSINP3OK    BIT(29)
#define USB_DWC3_GUSB3PIPECTL_DISRXDETINP3  BIT(28)
#define USB_DWC3_GUSB3PIPECTL_UX_EXIT_PX    BIT(27)
#define USB_DWC3_GUSB3PIPECTL_REQP1P2P3     BIT(24)
#define USB_DWC3_GUSB3PIPECTL_DEP1P2P3(n)   ((n) << 19)
#define USB_DWC3_GUSB3PIPECTL_DEP1P2P3_MASK USB_DWC3_GUSB3PIPECTL_DEP1P2P3(7)
#define USB_DWC3_GUSB3PIPECTL_DEP1P2P3_EN   USB_DWC3_GUSB3PIPECTL_DEP1P2P3(1)
#define USB_DWC3_GUSB3PIPECTL_DEPOCHANGE    BIT(18)
#define USB_DWC3_GUSB3PIPECTL_SUSPHY        BIT(17)
#define USB_DWC3_GUSB3PIPECTL_LFPSFILT      BIT(9)
#define USB_DWC3_GUSB3PIPECTL_RX_DETOPOLL   BIT(8)
#define USB_DWC3_GUSB3PIPECTL_TX_DEEPH(n)   ((n) << 1)
#define USB_DWC3_GUSB3PIPECTL_TX_DEEPH_MASK USB_DWC3_GUSB3PIPECTL_TX_DEEPH(3)

/* GTXFIFOSIZ */
#define USB_DWC31_GTXFIFOSIZ_TXFRAMNUM   BIT(15)        /* DWC_usb31 and later */
#define USB_DWC31_GTXFIFOSIZ_TXFDEP(n)   ((n) & 0x7fff) /* DWC_usb31 and later */
#define USB_DWC3_GTXFIFOSIZ_TXFDEP(n)    ((n) & 0xffff)
#define USB_DWC3_GTXFIFOSIZ_TXFSTADDR(n) ((n) & 0xffff0000)

/* GRXFIFOSIZ */
#define USB_DWC31_GRXFIFOSIZ_RXFDEP(n) ((n) & 0x7fff) /* DWC_usb31 and later */
#define USB_DWC3_GRXFIFOSIZ_RXFDEP(n)  ((n) & 0xffff)

/* GEVNTSIZ */
#define USB_DWC3_GEVNTSIZ_INTMASK BIT(31)
#define USB_DWC3_GEVNTSIZ_SIZE(n) ((n) & 0xffff)

/* GHWPARAMS0 */
#define USB_DWC3_GHWPARAMS0_MODE(n)      ((n) & 0x3)
#define USB_DWC3_GHWPARAMS0_MODE_GADGET  0
#define USB_DWC3_GHWPARAMS0_MODE_HOST    1
#define USB_DWC3_GHWPARAMS0_MODE_DRD     2
#define USB_DWC3_GHWPARAMS0_MBUS_TYPE(n) (((n) >> 3) & 0x7)
#define USB_DWC3_GHWPARAMS0_SBUS_TYPE(n) (((n) >> 6) & 0x3)
#define USB_DWC3_GHWPARAMS0_MDWIDTH(n)   (((n) >> 8) & 0xff)
#define USB_DWC3_GHWPARAMS0_SDWIDTH(n)   (((n) >> 16) & 0xff)
#define USB_DWC3_GHWPARAMS0_AWIDTH(n)    (((n) >> 24) & 0xff)

/* GHWPARAMS1 */
#define USB_DWC3_GHWPARAMS1_EN_PWROPT(n)  (((n) & (3 << 24)) >> 24)
#define USB_DWC3_GHWPARAMS1_EN_PWROPT_NO  0
#define USB_DWC3_GHWPARAMS1_EN_PWROPT_CLK 1
#define USB_DWC3_GHWPARAMS1_EN_PWROPT_HIB 2
#define USB_DWC3_GHWPARAMS1_PWROPT(n)     ((n) << 24)
#define USB_DWC3_GHWPARAMS1_PWROPT_MASK   USB_DWC3_GHWPARAMS1_PWROPT(3)
#define USB_DWC3_GHWPARAMS1_ENDBC         BIT(31)
#define USB_DWC3_SPRAM_TYPE(n)            (((n) >> 23) & 1)
#define USB_DWC3_NUM_INT(n)               (((n) & (0x3f << 15)) >> 15)

/* GHWPARAMS3 */
#define USB_DWC3_GHWPARAMS3_SSPHY_IFC(n)        ((n) & 3)
#define USB_DWC3_GHWPARAMS3_SSPHY_IFC_DIS       0
#define USB_DWC3_GHWPARAMS3_SSPHY_IFC_GEN1      1
#define USB_DWC3_GHWPARAMS3_SSPHY_IFC_GEN2      2 /* DWC_usb31 only */
#define USB_DWC3_GHWPARAMS3_HSPHY_IFC(n)        (((n) & (3 << 2)) >> 2)
#define USB_DWC3_GHWPARAMS3_HSPHY_IFC_DIS       0
#define USB_DWC3_GHWPARAMS3_HSPHY_IFC_UTMI      1
#define USB_DWC3_GHWPARAMS3_HSPHY_IFC_ULPI      2
#define USB_DWC3_GHWPARAMS3_HSPHY_IFC_UTMI_ULPI 3
#define USB_DWC3_GHWPARAMS3_FSPHY_IFC(n)        (((n) & (3 << 4)) >> 4)
#define USB_DWC3_GHWPARAMS3_FSPHY_IFC_DIS       0
#define USB_DWC3_GHWPARAMS3_FSPHY_IFC_ENA       1
/* Unlike Linux, these take the GHWPARAMS3 value rather than the cached set */
#define USB_DWC3_NUM_IN_EPS_MASK                (0x1f << 18)
#define USB_DWC3_NUM_EPS_MASK                   (0x3f << 12)
#define USB_DWC3_NUM_EPS(n)                     (((n) & USB_DWC3_NUM_EPS_MASK) >> 12)
#define USB_DWC3_NUM_IN_EPS(n)                  (((n) & USB_DWC3_NUM_IN_EPS_MASK) >> 18)

/* GHWPARAMS6 */
#define USB_DWC3_GHWPARAMS6_EN_FPGA    BIT(7)
#define USB_DWC3_GHWPARAMS6_MDWIDTH(n) ((n) & (0x3 << 8)) /* DWC_usb32 only */
#define USB_DWC3_RAM0_DEPTH(n)         (((n) & 0xffff0000) >> 16)

/* GHWPARAMS7 */
#define USB_DWC3_GHWPARAMS7_RAM1_DEPTH(n) ((n) & 0xffff)
#define USB_DWC3_GHWPARAMS7_RAM2_DEPTH(n) (((n) >> 16) & 0xffff)

/* GHWPARAMS9 */
#define USB_DWC3_GHWPARAMS9_DEV_TXF_FLUSH_BYPASS BIT(0)
#define USB_DWC3_GHWPARAMS9_DEV_MST              BIT(1)

/* GFLADJ */
#define USB_DWC3_GFLADJ_30MHZ_SDBND_SEL   BIT(7)
#define USB_DWC3_GFLADJ_30MHZ_MASK        0x3f
#define USB_DWC3_GFLADJ_REFCLK_FLADJ_MASK GENMASK(21, 8)
#define USB_DWC3_GFLADJ_REFCLK_LPM_SEL    BIT(23)
#define USB_DWC3_GFLADJ_240MHZDECR        GENMASK(30, 24)
#define USB_DWC3_GFLADJ_240MHZDECR_PLS1   BIT(31)

/* GUCTL */
#define USB_DWC3_GUCTL_REFCLKPER_MASK 0xffc00000
#define USB_DWC3_GUCTL_REFCLKPER_SEL  22

/* GUCTL2 */
#define USB_DWC3_GUCTL2_RST_ACTBITLATER BIT(14)
#define USB_DWC3_GUCTL2_LC_TIMER        BIT(19)

/* DCFG */
#define USB_DWC3_DCFG_NUMLANES(n)     (((n) & 0x3) << 30) /* DWC_usb32 only */
#define USB_DWC3_DCFG_IGNSTRMPP       BIT(23)
#define USB_DWC3_DCFG_LPM_CAP         BIT(22)
#define USB_DWC3_DCFG_NUMP_SHIFT      17
#define USB_DWC3_DCFG_NUMP(n)         (((n) >> USB_DWC3_DCFG_NUMP_SHIFT) & 0x1f)
#define USB_DWC3_DCFG_NUMP_MASK       (0x1f << USB_DWC3_DCFG_NUMP_SHIFT)
#define USB_DWC3_DCFG_DEVADDR(addr)   ((addr) << 3)
#define USB_DWC3_DCFG_DEVADDR_MASK    USB_DWC3_DCFG_DEVADDR(0x7f)
#define USB_DWC3_DCFG_SPEED_MASK      (7 << 0)
#define USB_DWC3_DCFG_SUPERSPEED_PLUS (5 << 0) /* DWC_usb31 and later */
#define USB_DWC3_DCFG_SUPERSPEED      (4 << 0)
#define USB_DWC3_DCFG_HIGHSPEED       (0 << 0)
#define USB_DWC3_DCFG_FULLSPEED       BIT(0)

/* DCTL */
#define USB_DWC3_DCTL_RUN_STOP          BIT(31)
#define USB_DWC3_DCTL_CSFTRST           BIT(30)
#define USB_DWC3_DCTL_LSFTRST           BIT(29)
#define USB_DWC3_DCTL_HIRD_THRES_MASK   (0x1f << 24)
#define USB_DWC3_DCTL_HIRD_THRES(n)     ((n) << 24)
#define USB_DWC3_DCTL_APPL1RES          BIT(23)
/* Target link state, core versions 1.87a and earlier */
#define USB_DWC3_DCTL_TRGTULST_MASK     (0x0f << 17)
#define USB_DWC3_DCTL_TRGTULST(n)       ((n) << 17)
#define USB_DWC3_DCTL_TRGTULST_U2       USB_DWC3_DCTL_TRGTULST(2)
#define USB_DWC3_DCTL_TRGTULST_U3       USB_DWC3_DCTL_TRGTULST(3)
#define USB_DWC3_DCTL_TRGTULST_SS_DIS   USB_DWC3_DCTL_TRGTULST(4)
#define USB_DWC3_DCTL_TRGTULST_RX_DET   USB_DWC3_DCTL_TRGTULST(5)
#define USB_DWC3_DCTL_TRGTULST_SS_INACT USB_DWC3_DCTL_TRGTULST(6)
/* Core versions 1.94a and later */
#define USB_DWC3_DCTL_NYET_THRES_MASK   (0xf << 20)
#define USB_DWC3_DCTL_NYET_THRES(n)     (((n) & 0xf) << 20)
#define USB_DWC3_DCTL_KEEP_CONNECT      BIT(19)
#define USB_DWC3_DCTL_L1_HIBER_EN       BIT(18)
#define USB_DWC3_DCTL_CRS               BIT(17)
#define USB_DWC3_DCTL_CSS               BIT(16)
#define USB_DWC3_DCTL_INITU2ENA         BIT(12)
#define USB_DWC3_DCTL_ACCEPTU2ENA       BIT(11)
#define USB_DWC3_DCTL_INITU1ENA         BIT(10)
#define USB_DWC3_DCTL_ACCEPTU1ENA       BIT(9)
#define USB_DWC3_DCTL_ULSTCHNGREQ_MASK  (0x0f << 5)
#define USB_DWC3_DCTL_ULSTCHNGREQ(n)    (((n) << 5) & USB_DWC3_DCTL_ULSTCHNGREQ_MASK)
#define USB_DWC3_DCTL_TSTCTRL_MASK      (0xf << 1)

#define USB_DWC3_DCTL_ULSTCHNG_NO_ACTION   USB_DWC3_DCTL_ULSTCHNGREQ(0)
#define USB_DWC3_DCTL_ULSTCHNG_SS_DISABLED USB_DWC3_DCTL_ULSTCHNGREQ(4)
#define USB_DWC3_DCTL_ULSTCHNG_RX_DETECT   USB_DWC3_DCTL_ULSTCHNGREQ(5)
#define USB_DWC3_DCTL_ULSTCHNG_SS_INACTIVE USB_DWC3_DCTL_ULSTCHNGREQ(6)
#define USB_DWC3_DCTL_ULSTCHNG_RECOVERY    USB_DWC3_DCTL_ULSTCHNGREQ(8)
#define USB_DWC3_DCTL_ULSTCHNG_COMPLIANCE  USB_DWC3_DCTL_ULSTCHNGREQ(10)
#define USB_DWC3_DCTL_ULSTCHNG_LOOPBACK    USB_DWC3_DCTL_ULSTCHNGREQ(11)

/* DEVTEN */
#define USB_DWC3_DEVTEN_VNDRDEVTSTRCVEDEN   BIT(12)
#define USB_DWC3_DEVTEN_EVNTOVERFLOWEN      BIT(11)
#define USB_DWC3_DEVTEN_CMDCMPLTEN          BIT(10)
#define USB_DWC3_DEVTEN_ERRTICERREN         BIT(9)
#define USB_DWC3_DEVTEN_SOFEN               BIT(7)
#define USB_DWC3_DEVTEN_U3L2L1SUSPEN        BIT(6)
#define USB_DWC3_DEVTEN_HIBERNATIONREQEVTEN BIT(5)
#define USB_DWC3_DEVTEN_WKUPEVTEN           BIT(4)
#define USB_DWC3_DEVTEN_ULSTCNGEN           BIT(3)
#define USB_DWC3_DEVTEN_CONNECTDONEEN       BIT(2)
#define USB_DWC3_DEVTEN_USBRSTEN            BIT(1)
#define USB_DWC3_DEVTEN_DISCONNEVTEN        BIT(0)

/* DSTS */
#define USB_DWC3_DSTS_CONNLANES(n)    (((n) >> 30) & 0x3) /* DWC_usb32 only */
#define USB_DWC3_DSTS_DCNRD           BIT(29)
#define USB_DWC3_DSTS_RSS             BIT(25) /* 1.94a and later */
#define USB_DWC3_DSTS_SSS             BIT(24) /* 1.94a and later */
#define USB_DWC3_DSTS_PWRUPREQ        BIT(24) /* 1.87a and earlier */
#define USB_DWC3_DSTS_COREIDLE        BIT(23)
#define USB_DWC3_DSTS_DEVCTRLHLT      BIT(22)
#define USB_DWC3_DSTS_USBLNKST_MASK   (0x0f << 18)
#define USB_DWC3_DSTS_USBLNKST(n)     (((n) & USB_DWC3_DSTS_USBLNKST_MASK) >> 18)
#define USB_DWC3_DSTS_RXFIFOEMPTY     BIT(17)
#define USB_DWC3_DSTS_SOFFN_MASK      (0x3fff << 3)
#define USB_DWC3_DSTS_SOFFN(n)        (((n) & USB_DWC3_DSTS_SOFFN_MASK) >> 3)
#define USB_DWC3_DSTS_CONNECTSPD      (7 << 0)
#define USB_DWC3_DSTS_SUPERSPEED_PLUS (5 << 0) /* DWC_usb31 and later */
#define USB_DWC3_DSTS_SUPERSPEED      (4 << 0)
#define USB_DWC3_DSTS_HIGHSPEED       (0 << 0)
#define USB_DWC3_DSTS_FULLSPEED       BIT(0)

/* DGCMD */
#define USB_DWC3_DGCMD_SET_LMP                0x01
#define USB_DWC3_DGCMD_SET_PERIODIC_PAR       0x02
#define USB_DWC3_DGCMD_XMIT_FUNCTION          0x03
#define USB_DWC3_DGCMD_SET_SCRATCHPAD_ADDR_LO 0x04
#define USB_DWC3_DGCMD_SET_SCRATCHPAD_ADDR_HI 0x05
#define USB_DWC3_DGCMD_DEV_NOTIFICATION       0x07
#define USB_DWC3_DGCMD_SELECTED_FIFO_FLUSH    0x09
#define USB_DWC3_DGCMD_ALL_FIFO_FLUSH         0x0a
#define USB_DWC3_DGCMD_SET_ENDPOINT_NRDY      0x0c
#define USB_DWC3_DGCMD_SET_ENDPOINT_PRIME     0x0d
#define USB_DWC3_DGCMD_RUN_SOC_BUS_LOOPBACK   0x10
#define USB_DWC3_DGCMD_STATUS(n)              (((n) >> 12) & 0x0f)
#define USB_DWC3_DGCMD_CMDACT                 BIT(10)
#define USB_DWC3_DGCMD_CMDIOC                 BIT(8)

/* DGCMDPAR */
#define USB_DWC3_DGCMDPAR_FORCE_LINKPM_ACCEPT BIT(0)
#define USB_DWC3_DGCMDPAR_FIFO_NUM(n)         ((n) << 0)
#define USB_DWC3_DGCMDPAR_RX_FIFO             (0 << 5)
#define USB_DWC3_DGCMDPAR_TX_FIFO             BIT(5)
#define USB_DWC3_DGCMDPAR_DN_FUNC_WAKE        BIT(0)
#define USB_DWC3_DGCMDPAR_INTF_SEL(n)         ((n) << 4)

/* DEPCMD */
#define USB_DWC3_DEPCMD_PARAM_SHIFT    16
#define USB_DWC3_DEPCMD_PARAM(x)       ((x) << USB_DWC3_DEPCMD_PARAM_SHIFT)
#define USB_DWC3_DEPCMD_GET_RSC_IDX(x) (((x) >> USB_DWC3_DEPCMD_PARAM_SHIFT) & 0x7f)
#define USB_DWC3_DEPCMD_STATUS(x)      (((x) >> 12) & 0x0f)
#define USB_DWC3_DEPCMD_HIPRI_FORCERM  BIT(11)
#define USB_DWC3_DEPCMD_CLEARPENDIN    BIT(11)
#define USB_DWC3_DEPCMD_CMDACT         BIT(10)
#define USB_DWC3_DEPCMD_CMDIOC         BIT(8)
#define USB_DWC3_DEPCMD_CMD(x)         ((x) & 0xf)

#define USB_DWC3_DEPCMD_DEPSTARTCFG       (0x09 << 0)
#define USB_DWC3_DEPCMD_ENDTRANSFER       (0x08 << 0)
#define USB_DWC3_DEPCMD_UPDATETRANSFER    (0x07 << 0)
#define USB_DWC3_DEPCMD_STARTTRANSFER     (0x06 << 0)
#define USB_DWC3_DEPCMD_CLEARSTALL        (0x05 << 0)
#define USB_DWC3_DEPCMD_SETSTALL          (0x04 << 0)
#define USB_DWC3_DEPCMD_GETSEQNUMBER      (0x03 << 0) /* 1.90a and earlier */
#define USB_DWC3_DEPCMD_GETEPSTATE        (0x03 << 0) /* 1.94a and later */
#define USB_DWC3_DEPCMD_SETTRANSFRESOURCE (0x02 << 0)
#define USB_DWC3_DEPCMD_SETEPCONFIG       (0x01 << 0)

#define USB_DWC3_DEPCMD_TYPE_CONTROL 0
#define USB_DWC3_DEPCMD_TYPE_ISOC    1
#define USB_DWC3_DEPCMD_TYPE_BULK    2
#define USB_DWC3_DEPCMD_TYPE_INTR    3

/* DALEPENA, one bit per physical endpoint */
#define USB_DWC3_DALEPENA_EP(n) BIT(n)

/* DCFG1, DWC_usb32 only */
#define USB_DWC3_DCFG1_DIS_MST_ENH BIT(1)

/* DEV_IMOD */
#define USB_DWC3_DEV_IMOD_COUNT_SHIFT    16
#define USB_DWC3_DEV_IMOD_COUNT_MASK     (0xffff << 16)
#define USB_DWC3_DEV_IMOD_INTERVAL_SHIFT 0
#define USB_DWC3_DEV_IMOD_INTERVAL_MASK  (0xffff << 0)

/* DEPCFG (Set Endpoint Configuration) parameter 0 */
#define USB_DWC3_DEPCFG_EP_TYPE(n)         (((n) & 0x3) << 1)
#define USB_DWC3_DEPCFG_MAX_PACKET_SIZE(n) (((n) & 0x7ff) << 3)
#define USB_DWC3_DEPCFG_FIFO_NUMBER(n)     (((n) & 0x1f) << 17)
#define USB_DWC3_DEPCFG_BURST_SIZE(n)      (((n) & 0xf) << 22)
#define USB_DWC3_DEPCFG_DATA_SEQ_NUM(n)    ((n) << 26)
#define USB_DWC3_DEPCFG_IGN_SEQ_NUM        BIT(31)   /* before 1.94a */
#define USB_DWC3_DEPCFG_ACTION_INIT        (0 << 30) /* 1.94a and later */
#define USB_DWC3_DEPCFG_ACTION_RESTORE     BIT(30)   /* 1.94a and later */
#define USB_DWC3_DEPCFG_ACTION_MODIFY      (2 << 30) /* 1.94a and later */

/* DEPCFG parameter 1 */
#define USB_DWC3_DEPCFG_INT_NUM(n)          (((n) & 0x1f) << 0)
#define USB_DWC3_DEPCFG_XFER_COMPLETE_EN    BIT(8)
#define USB_DWC3_DEPCFG_XFER_IN_PROGRESS_EN BIT(9)
#define USB_DWC3_DEPCFG_XFER_NOT_READY_EN   BIT(10)
#define USB_DWC3_DEPCFG_FIFO_ERROR_EN       BIT(11)
#define USB_DWC3_DEPCFG_STREAM_EVENT_EN     BIT(13)
#define USB_DWC3_DEPCFG_BINTERVAL_M1(n)     (((n) & 0xff) << 16)
#define USB_DWC3_DEPCFG_STREAM_CAPABLE      BIT(24)
#define USB_DWC3_DEPCFG_EP_NUMBER(n)        (((n) & 0x1f) << 25)
#define USB_DWC3_DEPCFG_BULK_BASED          BIT(30)
#define USB_DWC3_DEPCFG_FIFO_BASED          BIT(31)

/* DEPXFERCFG (Set Endpoint Transfer Resource Configuration) parameter 0 */
#define USB_DWC3_DEPXFERCFG_NUM_XFER_RES(n) ((n) & 0xffff)

/* Link states in DSTS.USBLNKST and in link state events */
#define USB_DWC3_LINK_STATE_U0       0x00 /* HS: On */
#define USB_DWC3_LINK_STATE_U1       0x01
#define USB_DWC3_LINK_STATE_U2       0x02 /* HS: Sleep (L1) */
#define USB_DWC3_LINK_STATE_U3       0x03 /* HS: Suspend (L2) */
#define USB_DWC3_LINK_STATE_SS_DIS   0x04
#define USB_DWC3_LINK_STATE_RX_DET   0x05 /* HS: Early Suspend */
#define USB_DWC3_LINK_STATE_SS_INACT 0x06
#define USB_DWC3_LINK_STATE_POLL     0x07
#define USB_DWC3_LINK_STATE_RECOV    0x08
#define USB_DWC3_LINK_STATE_HRESET   0x09
#define USB_DWC3_LINK_STATE_CMPLY    0x0a
#define USB_DWC3_LINK_STATE_LPBK     0x0b
#define USB_DWC3_LINK_STATE_RESET    0x0e
#define USB_DWC3_LINK_STATE_RESUME   0x0f
#define USB_DWC3_LINK_STATE_MASK     0x0f

/* Transfer Request Block, 16-byte aligned in memory */
struct usb_dwc3_trb {
	uint32_t bpl;
	uint32_t bph;
	uint32_t size;
	uint32_t ctrl;
} __packed;

/* TRB size word: buffer length, packet count and completion status */
#define USB_DWC3_TRB_SIZE_MASK      0x00ffffff
#define USB_DWC3_TRB_SIZE_LENGTH(n) ((n) & USB_DWC3_TRB_SIZE_MASK)
#define USB_DWC3_TRB_SIZE_PCM1(n)   (((n) & 0x03) << 24)
#define USB_DWC3_TRB_SIZE_TRBSTS(n) (((n) & (0x0f << 28)) >> 28)

#define USB_DWC3_TRBSTS_OK            0
#define USB_DWC3_TRBSTS_MISSED_ISOC   1
#define USB_DWC3_TRBSTS_SETUP_PENDING 2
#define USB_DWC3_TRB_STS_XFER_IN_PROG 4

/* TRB control word */
#define USB_DWC3_TRB_CTRL_HWO             BIT(0)
#define USB_DWC3_TRB_CTRL_LST             BIT(1)
#define USB_DWC3_TRB_CTRL_CHN             BIT(2)
#define USB_DWC3_TRB_CTRL_CSP             BIT(3)
#define USB_DWC3_TRB_CTRL_TRBCTL(n)       (((n) & 0x3f) << 4)
#define USB_DWC3_TRB_CTRL_ISP_IMI         BIT(10)
#define USB_DWC3_TRB_CTRL_IOC             BIT(11)
#define USB_DWC3_TRB_CTRL_SID_SOFN(n)     (((n) & 0xffff) << 14)
#define USB_DWC3_TRB_CTRL_GET_SID_SOFN(n) (((n) & (0xffff << 14)) >> 14)

#define USB_DWC3_TRBCTL_TYPE(n)           ((n) & (0x3f << 4))
#define USB_DWC3_TRBCTL_NORMAL            USB_DWC3_TRB_CTRL_TRBCTL(1)
#define USB_DWC3_TRBCTL_CONTROL_SETUP     USB_DWC3_TRB_CTRL_TRBCTL(2)
#define USB_DWC3_TRBCTL_CONTROL_STATUS2   USB_DWC3_TRB_CTRL_TRBCTL(3)
#define USB_DWC3_TRBCTL_CONTROL_STATUS3   USB_DWC3_TRB_CTRL_TRBCTL(4)
#define USB_DWC3_TRBCTL_CONTROL_DATA      USB_DWC3_TRB_CTRL_TRBCTL(5)
#define USB_DWC3_TRBCTL_ISOCHRONOUS_FIRST USB_DWC3_TRB_CTRL_TRBCTL(6)
#define USB_DWC3_TRBCTL_ISOCHRONOUS       USB_DWC3_TRB_CTRL_TRBCTL(7)
#define USB_DWC3_TRBCTL_LINK_TRB          USB_DWC3_TRB_CTRL_TRBCTL(8)

/*
 * Event buffer entries. Bit 0 tells endpoint events (0) from the others (1),
 * bits 7:1 then give the type of a non-endpoint event.
 */
#define USB_DWC3_EVENT_IS_DEVSPEC BIT(0)
#define USB_DWC3_EVENT_TYPE(e)    (((e) >> 1) & 0x7f)

#define USB_DWC3_EVENT_TYPE_DEV    0
#define USB_DWC3_EVENT_TYPE_CARKIT 3
#define USB_DWC3_EVENT_TYPE_I2C    4

/* Device endpoint event fields */
#define USB_DWC3_DEPEVT_ENDPOINT_NUMBER(e) (((e) >> 1) & 0x1f)
#define USB_DWC3_DEPEVT_ENDPOINT_EVENT(e)  (((e) >> 6) & 0x0f)
#define USB_DWC3_DEPEVT_STATUS(e)          (((e) >> 12) & 0x0f)
#define USB_DWC3_DEPEVT_PARAMETERS(e)      (((e) >> 16) & 0xffff)

#define USB_DWC3_DEPEVT_XFERCOMPLETE   0x01
#define USB_DWC3_DEPEVT_XFERINPROGRESS 0x02
#define USB_DWC3_DEPEVT_XFERNOTREADY   0x03
#define USB_DWC3_DEPEVT_RXTXFIFOEVT    0x04
#define USB_DWC3_DEPEVT_STREAMEVT      0x06
#define USB_DWC3_DEPEVT_EPCMDCMPLT     0x07

/* Endpoint event status: XferNotReady */
#define USB_DWC3_DEPEVT_STATUS_TRANSFER_ACTIVE  BIT(3)
/* Endpoint event status: XferComplete and XferInProgress */
#define USB_DWC3_DEPEVT_STATUS_BUSERR           BIT(0)
#define USB_DWC3_DEPEVT_STATUS_SHORT            BIT(1)
#define USB_DWC3_DEPEVT_STATUS_IOC              BIT(2)
#define USB_DWC3_DEPEVT_STATUS_LST              BIT(3) /* XferComplete */
#define USB_DWC3_DEPEVT_STATUS_MISSED_ISOC      BIT(3) /* XferInProgress */
/* Endpoint event status: StreamEvt */
#define USB_DWC3_DEPEVT_STREAMEVT_FOUND         1
#define USB_DWC3_DEPEVT_STREAMEVT_NOTFOUND      2
/* Endpoint event parameters: StreamEvt */
#define USB_DWC3_DEPEVT_STREAM_PRIME            0xfffe
#define USB_DWC3_DEPEVT_STREAM_NOSTREAM         0x0
/* Endpoint event status: XferNotReady on a control endpoint */
#define USB_DWC3_DEPEVT_STATUS_CONTROL_DATA     1
#define USB_DWC3_DEPEVT_STATUS_CONTROL_STATUS   2
#define USB_DWC3_DEPEVT_STATUS_CONTROL_PHASE(n) ((n) & 3)
/* Endpoint event status: completion of a Start Transfer command */
#define USB_DWC3_DEPEVT_TRANSFER_NO_RESOURCE    1
#define USB_DWC3_DEPEVT_TRANSFER_BUS_EXPIRY     2
/* Endpoint event parameters: EPCmdCmplt, the completed command */
#define USB_DWC3_DEPEVT_PARAMETER_CMD(n)        (((n) & (0xf << 8)) >> 8)

/* Device event fields */
#define USB_DWC3_DEVT_TYPE(e)       (((e) >> 8) & 0x0f)
#define USB_DWC3_DEVT_EVENT_INFO(e) (((e) >> 16) & 0x1ff)

#define USB_DWC3_DEVICE_EVENT_DISCONNECT         0
#define USB_DWC3_DEVICE_EVENT_RESET              1
#define USB_DWC3_DEVICE_EVENT_CONNECT_DONE       2
#define USB_DWC3_DEVICE_EVENT_LINK_STATUS_CHANGE 3
#define USB_DWC3_DEVICE_EVENT_WAKEUP             4
#define USB_DWC3_DEVICE_EVENT_HIBER_REQ          5
#define USB_DWC3_DEVICE_EVENT_SUSPEND            6
#define USB_DWC3_DEVICE_EVENT_SOF                7
#define USB_DWC3_DEVICE_EVENT_ERRATIC_ERROR      9
#define USB_DWC3_DEVICE_EVENT_CMD_CMPL           10
#define USB_DWC3_DEVICE_EVENT_OVERFLOW           11

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_USB_COMMON_USB_DWC3_HW */
