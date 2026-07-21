/*
 * Copyright (c) 2025 Elan Microelectronics Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ELAN_EM32F967_SOC_USBCTRL_H_
#define ZEPHYR_SOC_ELAN_EM32F967_SOC_USBCTRL_H_

#define EM32_USBD_BASE     DT_REG_ADDR(DT_NODELABEL(usbd))
#define EM32_SYS_CTRL_BASE DT_REG_ADDR(DT_NODELABEL(sysctrl))
#define EM32_AIP_BASE      DT_REG_ADDR(DT_NODELABEL(clkctrl))

/* Where the special irc property code is */
#define SPECIAL_IRC_PROP_CODE_ADDR_1 0x100A6090
#define SPECIAL_IRC_PROP_CODE_ADDR_2 0x100A60F0

/* Sys Ctrl Group */
/* Register Offsets */
#define SYSCTRL_SYS_CTRL_OFF   0x0000
#define SYSCTRL_CLK_GATE_0_OFF 0x0100
#define SYSCTRL_CLK_GATE_1_OFF 0x0104

/* Register REG_EM32_SYS_CTRL bits position */
#define SYSCTRL_XTAL_LJIRC_SEL_Pos 1
#define SYSCTRL_USB_CLK_SEL_Pos    4

/* AIP Group */
/* Register Offsets */
#define AIP_LJIRC_CTRL_OFF   0x04
#define AIP_USB_PLL_CTRL_OFF 0x400
#define AIP_USB_PHY_CTRL_OFF 0x700

/* Register REG_LJIRC_CTRL bits position */
#define AIP_LJIRC_CTRL_LJIRC_CODE_Msk 0x1FFF8
#define AIP_LJIRC_CTRL_LJIRC_PD_Pos   0
#define AIP_LJIRC_CTRL_LJIRC_CODE_Pos 3

/* Register REG_USB_PLL_CTRL bits position */
#define AIP_USB_PLL_CTRL_PD_Pos     0
#define AIP_USB_PLL_CTRL_STABLE_Pos 7

/* Register REG_AIP_USB_PHY bits position */
#define AIP_USB_PHY_CTRL_RTRIM_Pos 4
#define AIP_USB_PHY_CTRL_RTRIM_Msk 0x00F0
#define AIP_USB_PHY_CTRL_PD_Pos    8
#define AIP_USB_PHY_CTRL_RSW_Pos   10

/* USBD Group */
/* Register Offsets */
#define USBD_REG_USB_CTRL_OFF     0x00
#define USBD_REG_CF_DATA_OFF      0x04
#define USBD_REG_USB_INT_EN_OFF   0x08
#define USBD_REG_EP0_INT_EN_OFF   0x0C
#define USBD_REG_EP1_INT_EN_OFF   0x10
#define USBD_REG_EP2_INT_EN_OFF   0x14
#define USBD_REG_EP3_INT_EN_OFF   0x18
#define USBD_REG_EP4_INT_EN_OFF   0x1C
#define USBD_REG_USB_INT_STA_OFF  0x20
#define USBD_REG_EP0_INT_STA_OFF  0x24
#define USBD_REG_EP1_INT_STA_OFF  0x28
#define USBD_REG_EP2_INT_STA_OFF  0x2C
#define USBD_REG_EP3_INT_STA_OFF  0x30
#define USBD_REG_EP4_INT_STA_OFF  0x34
#define USBD_REG_EP0_DATA_BUF_OFF 0x38
#define USBD_REG_EP1_DATA_BUF_OFF 0x3C
#define USBD_REG_EP2_DATA_BUF_OFF 0x40
#define USBD_REG_EP3_DATA_BUF_OFF 0x44
#define USBD_REG_EP4_DATA_BUF_OFF 0x48
#define USBD_REG_EP_BUF_STA_OFF   0x4C
#define USBD_REG_EP1_DATA_CNT_OFF 0x50
#define USBD_REG_EP2_DATA_CNT_OFF 0x54
#define USBD_REG_EP3_DATA_CNT_OFF 0x58
#define USBD_REG_EP4_DATA_CNT_OFF 0x5C
#define USBD_REG_EP_BUF_SET_0_OFF 0x60
#define USBD_REG_EP_BUF_SET_1_OFF 0x64
#define USBD_REG_PHY_TEST_OFF     0x6C
#define USBD_REG_USB_CTRL_EXT_OFF 0x74

/* Register REG_USB_CTRL bits position */
#define REG_USB_CTRL_UDC_RST_RDY_Pos 1
#define REG_USB_CTRL_EP1_EN_Pos      3
#define REG_USB_CTRL_EP2_EN_Pos      4
#define REG_USB_CTRL_EP3_EN_Pos      5
#define REG_USB_CTRL_EP4_EN_Pos      6
#define REG_USB_CTRL_UDC_EN_Pos      31

/* Register REG_USB_CF_DATA bits position */
#define USBD_CF_DATA_CONFIG_DATA_Msk    0x00FF
#define USBD_CF_DATA_EP_CONFIG_DONE_Pos 8
#define USBD_CF_DATA_EP_CONFIG_RDY_Pos  9

/* Register REG_USB_INT_EN bits position */
#define REG_USB_INT_EN_RST_INT_EN_Pos    0
#define REG_USB_INT_EN_SUS_INT_EN_Pos    1
#define REG_USB_INT_EN_RESUME_INT_EN_Pos 2

/* Register REG_EP0_INT_EN bits position */
#define REG_EP0_INT_EN_SETUP_INT_EN_Pos 0
#define REG_EP0_INT_EN_IN_INT_EN_Pos    1
#define REG_EP0_INT_EN_OUT_INT_EN_Pos   2
#define REG_EP0_INT_EN_DATA_READY_Pos   3
#define REG_EP0_INT_EN_BUF_CLR_Pos      4

/* Register REG_EPX_INT_EN bits position */
#define REG_EPX_INT_EN_IN_INT_EN_Pos  0
#define REG_EPX_INT_EN_OUT_INT_EN_Pos 1
#define REG_EPX_INT_EN_DATA_READY_Pos 3
#define REG_EPX_INT_EN_BUF_CLR_Pos    4

/* Register REG_USB_INT_STA bits position */
#define REG_USB_INT_STA_RST_INT_SF_Pos        0
#define REG_USB_INT_STA_SUS_INT_SF_Pos        1
#define REG_USB_INT_STA_RESUME_INT_SF_Pos     2
#define REG_USB_INT_STA_RST_INT_SF_CLR_Pos    8
#define REG_USB_INT_STA_SUS_INT_SF_CLR_Pos    9
#define REG_USB_INT_STA_RESUME_INT_SF_CLR_Pos 10

/* Register REG_EP0_INT_STA bits position */
#define REG_EP0_INT_STA_EP0_IN_INT_SF_Pos      1
#define REG_EP0_INT_STA_EP0_OUT_INT_SF_Pos     2
#define REG_EP0_INT_STA_SETUP_INT_SF_CLR_Pos   8
#define REG_EP0_INT_STA_EP0_IN_INT_SF_CLR_Pos  9
#define REG_EP0_INT_STA_EP0_OUT_INT_SF_CLR_Pos 10

/* Register REG_EPX_INT_STA bits position */
#define REG_EPX_INT_STA_IN_INT_SF_Pos      0
#define REG_EPX_INT_STA_OUT_INT_SF_Pos     1
#define REG_EPX_INT_STA_IN_INT_SF_CLR_Pos  8
#define REG_EPX_INT_STA_OUT_INT_SF_CLR_Pos 9

/* Register REG_EP_BUF_STA bits position */
#define REG_EP_BUF_STA_EP0_OUTBUF_EMPTY_Pos 11

/* Register REG_PHY_TEST bits position */
#define PHYTEST_USB_WAKEUP_EN_Pos 30

/* Register REG_USB_CTRL_EXT bits position */
#define REG_USB_CTRL_EXT_EP0_STALL_Pos     9
#define REG_USB_CTRL_EXT_DEV_RESUME_Pos    10
#define REG_USB_CTRL_EXT_EP1_STALL_Pos     13
#define REG_USB_CTRL_EXT_EP2_STALL_Pos     14
#define REG_USB_CTRL_EXT_EP3_STALL_Pos     15
#define REG_USB_CTRL_EXT_EP4_STALL_Pos     16
#define REG_USB_CTRL_EXT_EP_IN_PREHOLD_Pos 17

/* Registers */
/* Special Group */
#define USB_IRC_PROP_CODE_1 ((uint32_t)(SPECIAL_IRC_PROP_CODE_ADDR_1))
#define USB_IRC_PROP_CODE_2 ((uint32_t)(SPECIAL_IRC_PROP_CODE_ADDR_2))

/* Usbd Group */
#define REG_USB_CTRL         ((uint32_t)(EM32_USBD_BASE + USBD_REG_USB_CTRL_OFF))
#define REG_USB_CF_DATA      ((uint32_t)(EM32_USBD_BASE + USBD_REG_CF_DATA_OFF))
#define REG_USB_INT_EN       ((uint32_t)(EM32_USBD_BASE + USBD_REG_USB_INT_EN_OFF))
#define REG_EP0_INT_EN       ((uint32_t)(EM32_USBD_BASE + USBD_REG_EP0_INT_EN_OFF))
#define REG_EP1_INT_EN       ((uint32_t)(EM32_USBD_BASE + USBD_REG_EP1_INT_EN_OFF))
#define REG_EP2_INT_EN       ((uint32_t)(EM32_USBD_BASE + USBD_REG_EP2_INT_EN_OFF))
#define REG_EP3_INT_EN       ((uint32_t)(EM32_USBD_BASE + USBD_REG_EP3_INT_EN_OFF))
#define REG_EP4_INT_EN       ((uint32_t)(EM32_USBD_BASE + USBD_REG_EP4_INT_EN_OFF))
#define REG_USB_INT_STA      ((uint32_t)(EM32_USBD_BASE + USBD_REG_USB_INT_STA_OFF))
#define REG_EP0_INT_STA      ((uint32_t)(EM32_USBD_BASE + USBD_REG_EP0_INT_STA_OFF))
#define REG_EP1_INT_STA      ((uint32_t)(EM32_USBD_BASE + USBD_REG_EP1_INT_STA_OFF))
#define REG_EP2_INT_STA      ((uint32_t)(EM32_USBD_BASE + USBD_REG_EP2_INT_STA_OFF))
#define REG_EP3_INT_STA      ((uint32_t)(EM32_USBD_BASE + USBD_REG_EP3_INT_STA_OFF))
#define REG_EP4_INT_STA      ((uint32_t)(EM32_USBD_BASE + USBD_REG_EP4_INT_STA_OFF))
#define REG_EP0_DATA_BUF     ((uint32_t)(EM32_USBD_BASE + USBD_REG_EP0_DATA_BUF_OFF))
#define REG_EP1_DATA_BUF     ((uint32_t)(EM32_USBD_BASE + USBD_REG_EP1_DATA_BUF_OFF))
#define REG_EP2_DATA_BUF     ((uint32_t)(EM32_USBD_BASE + USBD_REG_EP2_DATA_BUF_OFF))
#define REG_EP3_DATA_BUF     ((uint32_t)(EM32_USBD_BASE + USBD_REG_EP3_DATA_BUF_OFF))
#define REG_EP4_DATA_BUF     ((uint32_t)(EM32_USBD_BASE + USBD_REG_EP4_DATA_BUF_OFF))
#define REG_EP_BUF_STA       ((uint32_t)(EM32_USBD_BASE + USBD_REG_EP_BUF_STA_OFF))
#define REG_EP1_DATA_CNT     ((uint32_t)(EM32_USBD_BASE + USBD_REG_EP1_DATA_CNT_OFF))
#define REG_EP2_DATA_CNT     ((uint32_t)(EM32_USBD_BASE + USBD_REG_EP2_DATA_CNT_OFF))
#define REG_EP3_DATA_CNT     ((uint32_t)(EM32_USBD_BASE + USBD_REG_EP3_DATA_CNT_OFF))
#define REG_EP4_DATA_CNT     ((uint32_t)(EM32_USBD_BASE + USBD_REG_EP4_DATA_CNT_OFF))
#define REG_USB_EP_BUF_SET_0 ((uint32_t)(EM32_USBD_BASE + USBD_REG_EP_BUF_SET_0_OFF))
#define REG_USB_EP_BUF_SET_1 ((uint32_t)(EM32_USBD_BASE + USBD_REG_EP_BUF_SET_1_OFF))
#define REG_PHY_TEST         ((uint32_t)(EM32_USBD_BASE + USBD_REG_PHY_TEST_OFF))
#define REG_USB_CTRL_EXT     ((uint32_t)(EM32_USBD_BASE + USBD_REG_USB_CTRL_EXT_OFF))

/* Sys Ctrl Group */
#define REG_EM32_SYS_CTRL ((uint32_t)(EM32_SYS_CTRL_BASE + SYSCTRL_SYS_REG_CTRL_OFF))
#define REG_CLK_GATE_0    ((uint32_t)(EM32_SYS_CTRL_BASE + SYSCTRL_CLK_GATE_0_OFF))
#define REG_CLK_GATE_1    ((uint32_t)(EM32_SYS_CTRL_BASE + SYSCTRL_CLK_GATE_1_OFF))

/* AIP Group */
#define REG_LJIRC_CTRL   ((uint32_t)(EM32_AIP_BASE + AIP_LJIRC_CTRL_OFF))
#define REG_USB_PLL_CTRL ((uint32_t)(EM32_AIP_BASE + AIP_USB_PLL_CTRL_OFF))
#define REG_AIP_USB_PHY  ((uint32_t)(EM32_AIP_BASE + AIP_USB_PHY_CTRL_OFF))

enum usb_clk_gate_type {
	EM32_GATE_PCLKG_UDC = 21,
	EM32_GATE_PCLKG_ATRIM = 22,
	EM32_GATE_PCLKG_AIP = 28,
};

#endif
