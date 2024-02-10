/*
 * Copyright (c) 2019 ML!PA Consulting GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef MCLK_APBAMASK_TC0
#define MCLK_TC0 (&MCLK->APBAMASK.reg)
#define MCLK_TC0_MASK ((1 << MCLK_APBAMASK_TC0_Pos) | (1 << MCLK_APBAMASK_TC1_Pos))
#endif
#ifdef MCLK_APBBMASK_TC0
#define MCLK_TC0 (&MCLK->APBBMASK.reg)
#define MCLK_TC0_MASK ((1 << MCLK_APBBMASK_TC0_Pos) | (1 << MCLK_APBBMASK_TC1_Pos))
#endif
#ifdef MCLK_APBCMASK_TC0
#define MCLK_TC0 (&MCLK->APBCMASK.reg)
#define MCLK_TC0_MASK ((1 << MCLK_APBCMASK_TC0_Pos) | (1 << MCLK_APBCMASK_TC1_Pos))
#endif
#ifdef MCLK_APBDMASK_TC0
#define MCLK_TC0 (&MCLK->APBDMASK.reg)
#define MCLK_TC0_MASK ((1 << MCLK_APBDMASK_TC0_Pos) | (1 << MCLK_APBDMASK_TC1_Pos))
#endif

#ifdef MCLK_APBAMASK_TC2
#define MCLK_TC2 (&MCLK->APBAMASK.reg)
#define MCLK_TC2_MASK ((1 << MCLK_APBAMASK_TC2_Pos) | (1 << MCLK_APBAMASK_TC3_Pos))
#endif
#ifdef MCLK_APBBMASK_TC2
#define MCLK_TC2 (&MCLK->APBBMASK.reg)
#define MCLK_TC2_MASK ((1 << MCLK_APBBMASK_TC2_Pos) | (1 << MCLK_APBBMASK_TC3_Pos))
#endif
#ifdef MCLK_APBCMASK_TC2
#define MCLK_TC2 (&MCLK->APBCMASK.reg)
#define MCLK_TC2_MASK ((1 << MCLK_APBCMASK_TC2_Pos) | (1 << MCLK_APBCMASK_TC3_Pos))
#endif
#ifdef MCLK_APBDMASK_TC2
#define MCLK_TC2 (&MCLK->APBDMASK.reg)
#define MCLK_TC2_MASK ((1 << MCLK_APBDMASK_TC2_Pos) | (1 << MCLK_APBDMASK_TC3_Pos))
#endif

#ifdef MCLK_APBAMASK_TC4
#define MCLK_TC4 (&MCLK->APBAMASK.reg)
#define MCLK_TC4_MASK ((1 << MCLK_APBAMASK_TC4_Pos) | (1 << MCLK_APBAMASK_TC5_Pos))
#endif
#ifdef MCLK_APBBMASK_TC4
#define MCLK_TC4 (&MCLK->APBBMASK.reg)
#define MCLK_TC4_MASK ((1 << MCLK_APBBMASK_TC4_Pos) | (1 << MCLK_APBBMASK_TC5_Pos))
#endif
#ifdef MCLK_APBCMASK_TC4
#define MCLK_TC4 (&MCLK->APBCMASK.reg)
#define MCLK_TC4_MASK ((1 << MCLK_APBCMASK_TC4_Pos) | (1 << MCLK_APBCMASK_TC5_Pos))
#endif
#ifdef MCLK_APBDMASK_TC4
#define MCLK_TC4 (&MCLK->APBDMASK.reg)
#define MCLK_TC4_MASK ((1 << MCLK_APBDMASK_TC4_Pos) | (1 << MCLK_APBDMASK_TC5_Pos))
#endif

#ifdef MCLK_APBAMASK_TC6
#define MCLK_TC6 (&MCLK->APBAMASK.reg)
#define MCLK_TC6_MASK ((1 << MCLK_APBAMASK_TC6_Pos) | (1 << MCLK_APBAMASK_TC7_Pos))
#endif
#ifdef MCLK_APBBMASK_TC6
#define MCLK_TC6 (&MCLK->APBBMASK.reg)
#define MCLK_TC6_MASK ((1 << MCLK_APBBMASK_TC6_Pos) | (1 << MCLK_APBBMASK_TC7_Pos))
#endif
#ifdef MCLK_APBCMASK_TC6
#define MCLK_TC6 (&MCLK->APBCMASK.reg)
#define MCLK_TC6_MASK ((1 << MCLK_APBCMASK_TC6_Pos) | (1 << MCLK_APBCMASK_TC7_Pos))
#endif
#ifdef MCLK_APBDMASK_TC6
#define MCLK_TC6 (&MCLK->APBDMASK.reg)
#define MCLK_TC6_MASK ((1 << MCLK_APBDMASK_TC6_Pos) | (1 << MCLK_APBDMASK_TC7_Pos))
#endif
