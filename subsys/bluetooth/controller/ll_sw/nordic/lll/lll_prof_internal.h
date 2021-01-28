/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_BT_CTLR_PROFILE_ISR)
void lll_prof_enter_radio(void);
void lll_prof_exit_radio(void);
void lll_prof_enter_lll(void);
void lll_prof_exit_lll(void);
void lll_prof_enter_ull_high(void);
void lll_prof_exit_ull_high(void);
void lll_prof_enter_ull_low(void);
void lll_prof_exit_ull_low(void);
#else
static inline void lll_prof_enter_radio(void) {}
static inline void lll_prof_exit_radio(void) {}
static inline void lll_prof_enter_lll(void) {}
static inline void lll_prof_exit_lll(void) {}
static inline void lll_prof_enter_ull_high(void) {}
static inline void lll_prof_exit_ull_high(void) {}
static inline void lll_prof_enter_ull_low(void) {}
static inline void lll_prof_exit_ull_low(void) {}
#endif

void lll_prof_latency_capture(void);
void lll_prof_radio_end_backup(void);
void lll_prof_cputime_capture(void);
void lll_prof_send(void);
