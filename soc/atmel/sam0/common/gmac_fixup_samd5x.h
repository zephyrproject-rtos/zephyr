/*
 * Copyright (c) 2020 Stephanos Ioannidis <root@stephanos.io>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The following GMAC clock configuration fix-up symbols map to the applicable
 * APB-specific symbols, in order to accommodate different SoC series with the
 * GMAC core connected to different APBs.
 */
#ifdef MCLK_APBAMASK_GMAC
#define MCLK_GMAC (&MCLK->APBAMASK.reg)
#define MCLK_GMAC_MASK (MCLK_APBAMASK_GMAC)
#endif
#ifdef MCLK_APBBMASK_GMAC
#define MCLK_GMAC (&MCLK->APBBMASK.reg)
#define MCLK_GMAC_MASK (MCLK_APBBMASK_GMAC)
#endif
#ifdef MCLK_APBCMASK_GMAC
#define MCLK_GMAC (&MCLK->APBCMASK.reg)
#define MCLK_GMAC_MASK (MCLK_APBCMASK_GMAC)
#endif
#ifdef MCLK_APBDMASK_GMAC
#define MCLK_GMAC (&MCLK->APBDMASK.reg)
#define MCLK_GMAC_MASK (MCLK_APBDMASK_GMAC)
#endif
