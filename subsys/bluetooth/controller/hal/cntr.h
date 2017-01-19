/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _CNTR_H_
#define _CNTR_H_

void cntr_init(void);
uint32_t cntr_start(void);
uint32_t cntr_stop(void);
uint32_t cntr_cnt_get(void);
void cntr_cmp_set(uint8_t cmp, uint32_t value);

#endif /* _CNTR_H_ */
