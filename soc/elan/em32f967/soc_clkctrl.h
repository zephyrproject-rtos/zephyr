/*
 * Copyright (c) 2025 Elan Microelectronics Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ELAN_EM32F967_SOC_CLKCTRL_H_
#define ZEPHYR_SOC_ELAN_EM32F967_SOC_CLKCTRL_H_

#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>

/* Devicetree node label */
#define CLKCTRL_DT_NODE DT_NODELABEL(clkctrl)

/* Register Offsets */
#define CLKCTRL_MIRC_CTRL_OFF    0x0000
#define CLKCTRL_MIRC_CTRL2_OFF   0x0000
#define CLKCTRL_LJIRC_CTRL_OFF   0x0004
#define CLKCTRL_XTAL_CTRL_OFF    0x0200
#define CLKCTRL_LDO_PLL_OFF      0x030c
#define CLKCTRL_USB_PLL_CTRL_OFF 0x0400
#define CLKCTRL_SYS_PLL_CTRL_OFF 0x0404
#define CLKCTRL_PHY_CTRL_OFF     0x0700

/* Field Masks for MIRC_CTRL */
#define CLKCTRL_MIRC_PD        BIT(0)          /* [0]    MIRCPD */
#define CLKCTRL_HIRC_TESTV     BIT(1)          /* [1]    HIRC_TESTV */
#define CLKCTRL_MIRC_RCM_MASK  GENMASK(4, 2)   /* [4:2]  MIRCRCM */
#define CLKCTRL_MIRC_CA_MASK   GENMASK(10, 5)  /* [10:5] MIRCCA */
#define CLKCTRL_MIRC_TBG_MASK  GENMASK(12, 11) /* [12:11] MIRCTBG */
#define CLKCTRL_MIRC_TCF_MASK  GENMASK(14, 13) /* [14:13] MIRCTCF */
#define CLKCTRL_MIRC_TV12_MASK GENMASK(17, 15) /* [17:15] MIRCTV12 */

/* Field Masks for MIRC_CTRL2 */
#define CLKCTRL_MIRC2_PD        BIT(0)          /* [0]    MIRCPD */
#define CLKCTRL_HIRC2_TESTV     BIT(1)          /* [1]    HIRC_TESTV */
#define CLKCTRL_MIRC2_RCM_MASK  GENMASK(4, 2)   /* [4:2]  MIRCRCM */
#define CLKCTRL_MIRC2_TALL_MASK GENMASK(14, 5)  /* [14:5] MIRCTall */
#define CLKCTRL_MIRC2_TV12_MASK GENMASK(17, 15) /* [17:15] MIRCTV12 */

/* Field Masks for LJIRC_CTRL */
#define CLKCTRL_LJIRC_PD         BIT(0)          /* [0]    LJIRCPD */
#define CLKCTRL_LJIRC_RCM_MASK   GENMASK(2, 1)   /* [2:1]  LJIRCRCM */
#define CLKCTRL_LJIRC_FR_MASK    GENMASK(6, 3)   /* [6:3]  LJIRCFR */
#define CLKCTRL_LJIRC_CA_MASK    GENMASK(11, 7)  /* [11:7] LJIRCCA */
#define CLKCTRL_LJIRC_FC_MASK    GENMASK(14, 12) /* [14:12] LJIRCFC */
#define CLKCTRL_LJIRC_TMV10_MASK GENMASK(16, 15) /* [16:15] LJIRCTMV10 */
#define CLKCTRL_LJIRC_TESTV10B   BIT(17)         /* [17]   LJIRCTESTV10B */

/* Field Masks for XTAL_CTRL */
#define CLKCTRL_XTAL_FREQ_SEL_MASK GENMASK(1, 0) /* [1:0]  XTALFREQSEL */
#define CLKCTRL_XTAL_PD            BIT(2)        /* [2]    XTALPD */
#define CLKCTRL_XTAL_HZ            BIT(3)        /* [3]    XTALHZ */
#define CLKCTRL_XTAL_STABLE        BIT(4)        /* [4]    XTALSTABLE */
#define CLKCTRL_XTAL_COUNTER_MASK  GENMASK(6, 5) /* [6:5]  XTALCounter */

/* Field Masks for LDO_PLL */
#define CLKCTRL_PLL_LDO_PD        BIT(0)        /* [0]    PLLLDO_PD */
#define CLKCTRL_PLL_LDO_VP_SEL    BIT(1)        /* [1]    PLLLDO_VP_SEL */
#define CLKCTRL_PLL_LDO_VS_MASK   GENMASK(4, 2) /* [4:2]  PLLLDO_VS */
#define CLKCTRL_PLL_LDO_TV12_MASK GENMASK(8, 5) /* [8:5]  PLLLDO_TV12 */

/* Field Masks for USB_PLL_CTRL */
#define CLKCTRL_USB_PLL_PD              BIT(0)        /* [0]  USBPLLPD */
#define CLKCTRL_USB_PLL_FAST_LOCK       BIT(1)        /* [1]  USBPLLFASTLOCK */
#define CLKCTRL_USB_PLL_PSET_MASK       GENMASK(4, 2) /* [4:2]USBPLLPSET */
#define CLKCTRL_USB_PLL_STABLE_CNT_MASK GENMASK(6, 5) /* [6:5]USBPLLSTABLECNT */
#define CLKCTRL_USB_PLL_STABLE          BIT(7)        /* [7]  USBPLLSTABLE */

/* Field Masks for SYS_PLL_CTRL */
#define CLKCTRL_SYS_PLL_PD              BIT(0)        /* [0]  SYSPLLPD */
#define CLKCTRL_SYS_PLL_PSET_MASK       GENMASK(2, 1) /* [2:1]SYSPLLPSET */
#define CLKCTRL_SYS_PLL_FSET_MASK       GENMASK(6, 3) /* [6:3]SYSPLLFSET */
#define CLKCTRL_SYS_PLL_STABLE_CNT_MASK GENMASK(8, 7) /* [8:7]SYSPLLSTABLECNT */
#define CLKCTRL_SYS_PLL_STABLE          BIT(9)        /* [9]  SYSPLLSTABLE */

/* Field Masks for PHY_CTRL */
#define CLKCTRL_PHY_BUF_NSEL_MASK GENMASK(1, 0) /* [1:0]  PHYBUFNSEL */
#define CLKCTRL_PHY_BUF_PSEL_MASK GENMASK(3, 2) /* [3:2]  PHYBUFPSEL */
#define CLKCTRL_PHY_RTRIM_MASK    GENMASK(7, 4) /* [7:4]  PHYRTRIM */
#define CLKCTRL_USB_PHY_PDB       BIT(8)        /* [8]    USBPHYPDB */
#define CLKCTRL_USB_PHY_RESET     BIT(9)        /* [9]    USBPHYRESET */
#define CLKCTRL_USB_PHY_RSW       BIT(10)       /* [10]   USBPHYRSW */

#endif /* ZEPHYR_SOC_ELAN_EM32F967_SOC_CLKCTRL_H_ */
