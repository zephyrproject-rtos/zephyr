/*
 * Copyright (c) 2024 Jianxiong Gu <jianxiong.gu@outlook.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USBC_TCPC_RT1715_H_
#define ZEPHYR_DRIVERS_USBC_TCPC_RT1715_H_

#define RT1715_REG_SYS_CTRL_1                    0x90
/** VCONN OVP occurs and discharge path turn-on */
#define RT1715_REG_SYS_CTRL_1_VCONN_DISCHARGE_EN BIT(5)
/** Low power mode Rd or Rp */
#define RT1715_REG_SYS_CTRL_1_BMCIO_LPR_PRD      BIT(4)
/** Low power mode enable */
#define RT1715_REG_SYS_CTRL_1_BMCIO_LP_EN        BIT(3)
/** BMCIO BandGap enable */
#define RT1715_REG_SYS_CTRL_1_BMCIO_BG_EN        BIT(2)
/** VBUS detection enable */
#define RT1715_REG_SYS_CTRL_1_VBUS_DETECT_EN     BIT(1)
/** 24M oscillator for BMC communication */
#define RT1715_REG_SYS_CTRL_1_BMCIO_OSC_EN       BIT(0)

#define RT1715_REG_OCP                0x93
/** VCONN over-current control selection */
#define RT1715_REG_OCP_BMCIO_VCON_OCP GENMASK(7, 5)
#define RT1715_VCON_OCP_200MA         (0 << 5)
#define RT1715_VCON_OCP_300MA         (1 << 5)
#define RT1715_VCON_OCP_400MA         (2 << 5)
#define RT1715_VCON_OCP_500MA         (3 << 5)
#define RT1715_VCON_OCP_600MA         (4 << 5)

#define RT1715_REG_RT_ST         0x97
/** If VBUS under 0.8V */
#define RT1715_REG_RT_ST_VBUS_80 BIT(1)

#define RT1715_REG_RT_INT           0x98
/** Ra detach */
#define RT1715_REG_RT_INT_RA_DETACH BIT(5)
/** VBUS under 0.8V */
#define RT1715_REG_RT_INT_VBUS_80   BIT(1)
/** Low power mode exited */
#define RT1715_REG_RT_INT_WAKEUP    BIT(0)

#define RT1715_REG_RT_INT_MASK 0x99

#define RT1715_REG_LP_CTRL                  0x9B
/** Clock_300K divided from Clock_24M */
#define RT1715_REG_LP_CTRL_CK_300K_SEL      BIT(7)
/** Non-Shutdown mode */
#define RT1715_REG_LP_CTRL_SHUTDOWN_OFF     BIT(5)
/** Enable PD3.0 Extended message */
#define RT1715_REG_LP_CTRL_ENEXTMSG         BIT(4)
/** Auto enter idle mode enable */
#define RT1715_REG_LP_CTRL_AUTOIDLE_EN      BIT(3)
/** Enter idle mode timeout time */
#define RT1715_REG_LP_CTRL_AUTOIDLE_TIMEOUT GENMASK(2, 0)
#define RT1715_AUTOIDLE_TIMEOUT_96P0_MS     7
#define RT1715_AUTOIDLE_TIMEOUT_83P2_MS     6
#define RT1715_AUTOIDLE_TIMEOUT_70P4_MS     5
#define RT1715_AUTOIDLE_TIMEOUT_57P6_MS     4
#define RT1715_AUTOIDLE_TIMEOUT_44P8_MS     3
#define RT1715_AUTOIDLE_TIMEOUT_32P0_MS     2
#define RT1715_AUTOIDLE_TIMEOUT_19P2_MS     1
#define RT1715_AUTOIDLE_TIMEOUT_6P4_MS      0

#define RT1715_REG_SYS_WAKEUP    0x9F
/** Wakeup function enable */
#define RT1715_REG_SYS_WAKEUP_EN BIT(7)

#define RT1715_REG_SW_RST    0xA0
/** Write 1 to trigger software reset */
#define RT1715_REG_SW_RST_EN BIT(0)

#define RT1715_REG_DRP_CTRL_1      0xA2
/**
 * The period a DRP will complete a Source to Sink and back advertisement.
 * (Period = TDRP * 6.4 + 51.2ms)
 */
#define RT1715_REG_DRP_CTRL_1_TDRP GENMASK(3, 0)

#define RT1715_REG_DRP_CTRL_2          0xA3
/**
 * The percent of time that a DRP will advertise Source during tDRP.
 * (DUTY = (DCSRCDRP[9:0] + 1) / 1024)
 */
#define RT1715_REG_DRP_CTRL_2_DCSRCDRP GENMASK(9, 0)

#endif /* ZEPHYR_DRIVERS_USBC_TCPC_UCPD_NUMAKER_H_ */
