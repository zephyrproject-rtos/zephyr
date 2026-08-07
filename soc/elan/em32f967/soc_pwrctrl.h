/*
 * Copyright (c) 2025 Elan Microelectronics Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ELAN_EM32F967_SOC_PWRCTRL_H_
#define ZEPHYR_SOC_ELAN_EM32F967_SOC_PWRCTRL_H_

#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>

/* Devicetree node label */
#define PWRCTRL_DT_NODE DT_NODELABEL(pwrctrl)

/* Register Offsets */
#define PWRCTRL_POWER_SW_CTRL_OFF 0x0000

/* Field Masks for POWER_SW_CTRL */
#define PWRCTRL_POWER_SW_MASK   GENMASK(2, 0) /* [2:0]   POWERSW */
#define PWRCTRL_WARMUP_CNT_MASK GENMASK(5, 3) /* [5:3]   WARMUPCNT */
#define PWRCTRL_PD_SW_ACK_EN    BIT(6)        /* [6]     PD_SW_ACK_EN */
#define PWRCTRL_STANDBY1_S      BIT(7)        /* [7]     StandBy1_S */
#define PWRCTRL_STANDBY2_S      BIT(8)        /* [8]     StandBy2_S */
#define PWRCTRL_SIP_PD_ENABLE   BIT(9)        /* [9]     SIPPDEnable */
#define PWRCTRL_LDO_IDLE        BIT(10)       /* [10]    LDOIdle */
#define PWRCTRL_HIRC_PD         BIT(11)       /* [11]    HIRCPD */
#define PWRCTRL_SIRC32_PD       BIT(12)       /* [12]    SIRC32PD */
#define PWRCTRL_BOR_PD          BIT(13)       /* [13]    BORPD */
#define PWRCTRL_LDO2_PD         BIT(14)       /* [14]    LDO2PD */
#define PWRCTRL_RAM_PD_ENABLE   BIT(15)       /* [15]    RAMPDEnable */

#endif /* ZEPHYR_SOC_ELAN_EM32F967_SOC_PWRCTRL_H_ */
