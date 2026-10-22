/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

void lll_prof_enter_radio(void);
void lll_prof_exit_radio(void);
uint16_t lll_prof_radio_get(void);
void lll_prof_enter_lll(void);
void lll_prof_exit_lll(void);
uint16_t lll_prof_lll_get(void);
void lll_prof_enter_ull_high(void);
void lll_prof_exit_ull_high(void);
uint16_t lll_prof_ull_high_get(void);
void lll_prof_enter_ull_low(void);
void lll_prof_exit_ull_low(void);
uint16_t lll_prof_ull_low_get(void);

void lll_prof_latency_capture(void);
uint16_t lll_prof_latency_get(void);
void lll_prof_radio_end_backup(void);
void lll_prof_cputime_capture(void);
uint16_t lll_prof_cputime_get(void);
void lll_prof_send(void);
struct node_rx_pdu *lll_prof_reserve(void);
void lll_prof_reserve_send(struct node_rx_pdu *rx);
