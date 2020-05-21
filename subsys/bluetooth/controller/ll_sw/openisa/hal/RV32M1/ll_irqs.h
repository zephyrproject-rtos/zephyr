/*
 * Copyright 2019 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/* Needed for the DT_INST_* defines below */
#include <devicetree.h>

#define LL_SWI4_IRQn EMVSIM0_IRQn
#define LL_SWI5_IRQn MUA_IRQn

/*
 * LPTMR1 -> INTMUX_CH2. We expect it to be the only IRQ source for this channel
 * We'll use the INTMUX ISR for channel 2 instead of LPTMR1 ISR
 */
#define LL_RTC0_IRQn_1st_lvl DT_IRQN(DT_NODELABEL(intmux0_ch2))
#define LL_RTC0_IRQn_2nd_lvl DT_IRQN(DT_NODELABEL(lptmr1))

/*
 * radio -> INTMUX_CH3. We expect it to be the only IRQ source for this channel
 * We'll use the INTMUX ISR for channel 3 instead of radio ISR
 */
#define LL_RADIO_IRQn_1st_lvl DT_IRQN(DT_NODELABEL(intmux0_ch3))
#define LL_RADIO_IRQn_2nd_lvl DT_IRQN(DT_NODELABEL(generic_fsk))

#define LL_RTC0_IRQn LL_RTC0_IRQn_1st_lvl
#define LL_RADIO_IRQn LL_RADIO_IRQn_1st_lvl
