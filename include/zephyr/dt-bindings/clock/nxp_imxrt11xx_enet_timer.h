/*
 * Copyright (c) 2026 Powersoft S.p.A.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_NXP_IMXRT11XX_ENET_TIMER_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_NXP_IMXRT11XX_ENET_TIMER_H_

/*
 * Sources of the ENET_TIMER1 and ENET_TIMER2 clock roots of the i.MX RT11xx, for the
 * clock-mux property of nxp,imxrt11xx-enet-timer-root nodes. The values are the mux
 * field of the root, the same as kCLOCK_ENET_TIMERx_ClockRoot_Mux* in fsl_clock.h.
 */
#define IMXRT11XX_ENET_TIMER_SRC_OSC_RC_48M_DIV2 0
#define IMXRT11XX_ENET_TIMER_SRC_OSC_24M         1
#define IMXRT11XX_ENET_TIMER_SRC_OSC_RC_400M     2
#define IMXRT11XX_ENET_TIMER_SRC_OSC_RC_16M      3
#define IMXRT11XX_ENET_TIMER_SRC_SYS_PLL1_DIV2   4
#define IMXRT11XX_ENET_TIMER_SRC_AUDIO_PLL       5
#define IMXRT11XX_ENET_TIMER_SRC_SYS_PLL1_DIV5   6
#define IMXRT11XX_ENET_TIMER_SRC_SYS_PLL2_PFD1   7

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_NXP_IMXRT11XX_ENET_TIMER_H_ */
