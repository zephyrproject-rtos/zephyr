/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _RAND_H_
#define _RAND_H_

void rand_init(uint8_t *context, uint8_t context_len);
size_t rand_get(size_t octets, uint8_t *rand);
void isr_rand(void *param);

#endif /* _RAND_H_ */
