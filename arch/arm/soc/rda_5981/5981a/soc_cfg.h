/*
 * Copyright (c) 2017 RDA
 * Copyright (c) 2016-2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RDA_CCFG_H
#define RDA_CCFG_H

/** Config always-on Timer
 *
 */
void rda_ccfg_aontmr(void);

/** Config pin gp6
 *
 */
void rda_ccfg_gp6(void);

/** Config pin gp7
 *
 */
void rda_ccfg_gp7(void);

void rda_ccfg_ckrst(void);
void rda_ccfg_adc_init(unsigned char ch);
unsigned short rda_ccfg_adc_read(unsigned char ch);
int rda_ccfg_boot(void);

#endif

