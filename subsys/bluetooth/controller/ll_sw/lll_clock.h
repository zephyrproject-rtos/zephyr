/*
 * Copyright (c) 2018-2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

int lll_clock_init(void);
int lll_clock_deinit(void);
int lll_clock_wait(void);
int lll_hfclock_on(void);
int lll_hfclock_on_wait(void);
int lll_hfclock_off(void);
uint8_t lll_clock_sca_local_get(void);
uint32_t lll_clock_ppm_local_get(void);
uint32_t lll_clock_ppm_get(uint8_t sca);
