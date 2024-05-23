/*
 * Copyright (c) 2024 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_NPCM_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_NPCM_CLOCK_H_

/* clock bus references */
#define NPCM_CLOCK_BUS_FREERUN     0
#define NPCM_CLOCK_BUS_LFCLK       1
#define NPCM_CLOCK_BUS_OSC         2
#define NPCM_CLOCK_BUS_FIU         3
#define NPCM_CLOCK_BUS_CORE        4
#define NPCM_CLOCK_BUS_APB1        5
#define NPCM_CLOCK_BUS_APB2        6
#define NPCM_CLOCK_BUS_APB3        7
#define NPCM_CLOCK_BUS_APB4        8
#define NPCM_CLOCK_BUS_AHB6        9
#define NPCM_CLOCK_BUS_FMCLK       10

/* clock enable/disable references */
#define NPCM_PWDWN_CTL0            0
#define NPCM_PWDWN_CTL1            1
#define NPCM_PWDWN_CTL2            2
#define NPCM_PWDWN_CTL3            3
#define NPCM_PWDWN_CTL4            4
#define NPCM_PWDWN_CTL5            5
#define NPCM_PWDWN_CTL6            6
#define NPCM_PWDWN_CTL7            7
#define NPCM_PWDWN_CTL8            8
#define NPCM_PWDWN_CTL_COUNT       9

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_NPCX_CLOCK_H_ */
