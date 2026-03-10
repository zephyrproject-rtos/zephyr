/*
 * Copyright (c) 2020 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_NPCX_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_NPCX_CLOCK_H_

/* clock bus references */
#define NPCX_CLOCK_BUS_FREERUN     0
#define NPCX_CLOCK_BUS_LFCLK       1
#define NPCX_CLOCK_BUS_OSC         2
#define NPCX_CLOCK_BUS_FIU         3
#define NPCX_CLOCK_BUS_CORE        4
#define NPCX_CLOCK_BUS_APB1        5
#define NPCX_CLOCK_BUS_APB2        6
#define NPCX_CLOCK_BUS_APB3        7
#define NPCX_CLOCK_BUS_APB4        8
#define NPCX_CLOCK_BUS_AHB6        9
#define NPCX_CLOCK_BUS_FMCLK       10
#define NPCX_CLOCK_BUS_FIU0        NPCX_CLOCK_BUS_FIU
#define NPCX_CLOCK_BUS_FIU1        11
#define NPCX_CLOCK_BUS_MCLKD       12

/* clock enable/disable references */
#define NPCX_PWDWN_CTL0            0
#define NPCX_PWDWN_CTL1            1
#define NPCX_PWDWN_CTL2            2
#define NPCX_PWDWN_CTL3            3
#define NPCX_PWDWN_CTL4            4
#define NPCX_PWDWN_CTL5            5
#define NPCX_PWDWN_CTL6            6
#define NPCX_PWDWN_CTL7            7
#define NPCX_PWDWN_CTL8            8
#define NPCX_PWDWN_CTL9            9
#define NPCX_PWDWN_CTL_COUNT       10

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_NPCX_CLOCK_H_ */
